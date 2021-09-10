#include <asf.h>
#include <avr/delay.h>
#include <string.h>

#define led_1	IOPORT_CREATE_PIN(PORTD, 5)

TWI_Slave_t slave;


// "константы" для настройки usart
#define USART_SERIAL &USARTC1
#define USART_SERIAL_BAUDRATE            115200
#define USART_SERIAL_CHAR_LENGTH USART_CHSIZE_8BIT_gc
#define USART_SERIAL_PARITY USART_PMODE_DISABLED_gc
#define USART_SERIAL_STOP_BIT            false

// порты для usart в соответсвии с документацией
#define SET_TX() ioport_configure_pin(IOPORT_CREATE_PIN(PORTC, 7), IOPORT_DIR_OUTPUT )
#define SET_RX() ioport_configure_pin(IOPORT_CREATE_PIN(PORTC, 6), IOPORT_DIR_INPUT)


#define MASTER
#define SLAVE
// константы для настройки twi. Про скорость в 50к (TWI_SPEED) хз.
// Работало всё и на 400 и на 400000. Разница была лишь в периоде опроса
// т.е. буквально быстрее/медленее опрашивается

#define TWI_SPEED        50000

// TWI MASTER
#define TWI_MASTER       TWIE
#define TWI_MASTER_ADDR  0x50
#define TWI_MASTER_PORT  PORTE


// TWI SLAVES
#define TWI_SLAVE_ADDR   0x42
#define TWI_SLAVE        TWIC
#define TWI_SLAVE_ADDR_tmp1   0b1001000 //адрес датчика U7
#define TWI_SLAVE_ADDR_tmp2   0b1001111 //адрес датчика U8

#define DATA_LENGTH     8

uint8_t recv_data[DATA_LENGTH];


unsigned char read_data_cmd[] = {0x00}; // копипаст с инета для запроса температуры с датчика
unsigned char cfg_data_cmd[] = {0x01, 0xA1}; // копипаст с инета для задания какой-то там битности передачи. Брал отсюда: github.com/ncb85/utilis-and-examples/blob/master/cpm_env/i2c/tmp275.c
unsigned char temperature_buff[2]; // сюда будем читать температуру. Копипаст из ссылки выше
unsigned char temperature_buff2[2]; // сюда будем читать температуру. Копипаст из ссылки выше


void blink() {
	ioport_set_pin_level(led_1, 1);
	_delay_ms(500);
	ioport_set_pin_level(led_1, 0);
	_delay_ms(500);
}


int get_temp7() {
	// пакет для запроса температуры с датчика
	twi_package_t packet_to_request_temp = {
		.addr_length = 0,
		.chip        = TWI_SLAVE_ADDR_tmp1,
		.buffer      = read_data_cmd,
		.length      = 1,
		.no_wait     = false
	};
	twi_master_write(&TWI_MASTER, &packet_to_request_temp) != TWI_SUCCESS;
	
	// receive temp
	twi_package_t packet_to_recieve_temp = {
		.addr_length = 0,
		.chip        = TWI_SLAVE_ADDR_tmp1,
		.buffer      = temperature_buff,
		.length      = 2,
		.no_wait     = false
	};
	twi_master_read(&TWI_MASTER, &packet_to_recieve_temp);

	int16_t temp;
	// parse data to int (парс данные в число - это всё делает этот блок кода)
	temp=(int16_t)((uint16_t)( temperature_buff[0]<<8 | temperature_buff[1] ));

	return temp;
}


char convert_num_to_char(int i) {
	char string[33];
	itoa(i, string, 10);
	
	return i;
}


void setup_ports(void) {
	/* Иницализация портов для диодов на выход */
	ioport_set_pin_dir(led_1, IOPORT_DIR_OUTPUT);
}


// инициализирует usart и печатает тестовое "usart inited"
void usart_init() {
	SET_RX();
	SET_TX();
	static usart_serial_options_t  USART_SERIAL_OPTIONS = {
		.baudrate = USART_SERIAL_BAUDRATE,
		.charlength = USART_SERIAL_CHAR_LENGTH,
		.paritytype = USART_SERIAL_PARITY,
		.stopbits = USART_SERIAL_STOP_BIT
	};
	sysclk_enable_module(SYSCLK_PORT_C, PR_USART1_bm);
	usart_serial_init(USART_SERIAL, &USART_SERIAL_OPTIONS);
	
	char test_msg[12] = "usart inited";
	
	for (int i = 0; i < 12; i++) {
		usart_putchar(USART_SERIAL, test_msg[i]);
	}
	usart_putchar(USART_SERIAL, '\n');
}


static void slave_process() {
	slave.status = STATUS_OK;
	for(int i = 0; i < DATA_LENGTH; i++) {
		recv_data[i] = slave.receivedData[i];
		usart_putchar(USART_SERIAL, recv_data[i]);
	}
	ioport_set_pin_level(led_1, 1);
	
	char temp = convert_num_to_char(get_temp7());
	slave.sendData[0] = temp;
	
	 uint8_t data = slave.sendData[0];
}


ISR(TWIE_TWIS_vect) {
	TWI_SlaveInterruptHandler(&slave);
	
}


static void twi_init() {
	// master options
	twi_options_t m_options = {
		.speed     = TWI_SPEED,
		.chip      = TWI_MASTER_ADDR ,
		.speed_reg = TWI_BAUD(sysclk_get_cpu_hz(), TWI_SPEED)
	};
	
	TWI_MASTER_PORT.PIN0CTRL = PORT_OPC_WIREDANDPULL_gc;
	TWI_MASTER_PORT.PIN1CTRL = PORT_OPC_WIREDANDPULL_gc;
	irq_initialize_vectors();
	sysclk_enable_peripheral_clock(&TWI_MASTER);
	twi_master_init(&TWI_MASTER, &m_options);
	twi_master_enable(&TWI_MASTER);
	sysclk_enable_peripheral_clock(&TWI_SLAVE);
	TWI_SlaveInitializeDriver(&slave, &TWI_SLAVE, *slave_process);
	TWI_SlaveInitializeModule(&slave, TWI_SLAVE_ADDR,
	TWI_SLAVE_INTLVL_MED_gc);
	
	for (int i = 0; i < TWIS_SEND_BUFFER_SIZE; i++) {
		slave.receivedData[i] = 0;
	}
	cpu_irq_enable();
}


void temp_init() {
	twi_package_t packet_init = {
		.addr_length = 0,
		.chip        = TWI_SLAVE_ADDR_tmp1,
		.buffer      = (void *) cfg_data_cmd,
		.length      = 1,
		.no_wait     = false
	};
	
	twi_package_t packet_init2 = {
		.addr_length = 0,
		.chip        = TWI_SLAVE_ADDR_tmp2,
		.buffer      = (void *) cfg_data_cmd,
		.length      = 1,
		.no_wait     = false
	};
	twi_master_write(&TWI_MASTER, &packet_init); // init conf to tmp
	twi_master_write(&TWI_MASTER, &packet_init2); // init conf to tmp
	
}


int get_temp8() {
	//status_code_t master_status;
	
	// пакет для запроса температуры с датчика
	twi_package_t packet_to_request_temp2 = {
		.addr_length = 0,
		.chip        = TWI_SLAVE_ADDR_tmp2,
		.buffer      = read_data_cmd,
		.length      = 1,
		.no_wait     = false
	};
	twi_master_write(&TWI_MASTER, &packet_to_request_temp2) != TWI_SUCCESS;
	
	
	// receive temp
	
	twi_package_t packet_to_recieve_temp2 = {
		.addr_length = 0,
		.chip        = TWI_SLAVE_ADDR_tmp2,
		.buffer      = temperature_buff2,
		.length      = 2,
		.no_wait     = false
	};
	twi_master_read(&TWI_MASTER, &packet_to_recieve_temp2); // читает температуру в массив temperature_buff
	
	//
	//if (master_status == STATUS_OK) {blink();} // сигнал, что данные считались успешно (для дебага)
	
	// parse data to int (парс данные в число - это всё делает этот блок кода)
	int16_t temp;
	// parse data to int (парс данные в число - это всё делает этот блок кода)
	temp=(int16_t)((uint16_t)( temperature_buff2[0]<<8 | temperature_buff2[1] ));
	return temp;
}


// test func to pretty syntax send from master
status_code_t twi_master_send(int slave_addr, int data) {
	twi_package_t packet = {
		.addr_length = 0,
		.chip        = slave_addr,
		.buffer      =  data,
		.length      = sizeof(data),
		.no_wait     = false
	};
	
	return twi_master_write(&TWI_MASTER, &packet);
}


// test func to pretty syntax to master read
status_code_t twi_master_read(int slave_addr, int var_to_read) {
	twi_package_t packet = {
		.addr_length = 0,
		.chip        = slave_addr,
		.buffer      = var_to_read,
		.length      = sizeof(var_to_read),
		.no_wait     = false
	};
	
	return twi_master_write(&TWI_MASTER, &packet);
}


static void test_twi() {
	uint8_t buff[0]; // var to get data from slave
	
	twi_master_send(TWI_SLAVE_ADDR, {0x00});
	twi_master_read(TWI_SLAVE_ADDR, buff);
	
	convert_num_to_char(buff[0])
}


int main (void) {
	board_init();
	ioport_init();
	sysclk_init();
	setup_ports();
	usart_init();
	
	twi_init();
	temp_init();
	
	test_twi();
}
