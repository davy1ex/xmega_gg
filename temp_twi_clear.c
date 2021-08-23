// опрашивает датчик температуры по twi и печатает по usart

/* 

	asf requirements:
		-- system clock control (service)
		-- USART - serial interface (service)
		-- TWI - Two-Wire Interface (Common API) (service)
		-- TWI - Two-Wire Interface (driver) (both)
		-- IOPORT - Input/Output Port Controller (driver)

*/

#include <asf.h>
#include <avr/delay.h>
#include <string.h>

#define MY_LED	IOPORT_CREATE_PIN(PORTD, 5) // один из светодоиодов на плате

// "константы" для настройки usart
#define USART_SERIAL                     &USARTC1 
#define USART_SERIAL_BAUDRATE            115200
#define USART_SERIAL_CHAR_LENGTH         USART_CHSIZE_8BIT_gc
#define USART_SERIAL_PARITY              USART_PMODE_DISABLED_gc
#define USART_SERIAL_STOP_BIT            false

// порты для usart в соответсвии с документацией
#define SET_TX() ioport_configure_pin(IOPORT_CREATE_PIN(PORTC, 7), IOPORT_DIR_OUTPUT )
#define SET_RX() ioport_configure_pin(IOPORT_CREATE_PIN(PORTC, 6), IOPORT_DIR_INPUT)


// константы для настройки twi. Про скорость в 50к (TWI_SPEED) хз. 
// Работало всё и на 400 и на 400000. Разница была лишь в периоде опроса
// т.е. буквально быстрее/медленее опрашивается
#define TWI_MASTER       TWIE
#define TWI_MASTER_PORT  PORTE
#define TWI_SLAVE        TWIE
#define TWI_SPEED        50000
#define TWI_MASTER_ADDR  0x50
#define TWI_SLAVE_ADDR   0b1001000
#define DATA_LENGTH     8

// TWI_Slave_t slave; // эта строка не нужна для датчика температуры

unsigned char read_data_cmd[] = {0x00}; // копипаст с инета для запроса температуры с датчика
unsigned char cfg_data_cmd[] = {0x01, 0xA1}; // копипаст с инета для задания какой-то там битности передачи. Брал отсюда: github.com/ncb85/utilis-and-examples/blob/master/cpm_env/i2c/tmp275.c
unsigned char temperature_buff[2]; // сюда будем читать температуру. Копипаст из ссылки выше

// мигает светодиодом. 
void blink() {
	ioport_set_pin_level(MY_LED,1);
	_delay_ms(500);
	ioport_set_pin_level(MY_LED,0);
	_delay_ms(500);
}

// кидает первый пакет tmp275 для его настройки
// возвращает status_code_t
status_code_t tmp275_init() {
    twi_package_t packet_init = {		
		.addr_length = 0,
		.chip        = TWI_SLAVE_ADDR,
		.buffer      = (void *) cfg_data_cmd,
		.length      = 1,
		.no_wait     = false
	};
	return twi_master_write(&TWI_MASTER, &packet_init); // init conf to tmp	
}

// инициализирует twi 
void twi_init() {
    // master options
    twi_options_t m_options = {
		.speed     = TWI_SPEED,
		.chip      = TWI_MASTER_ADDR,
		.speed_reg = TWI_BAUD(sysclk_get_cpu_hz(), TWI_SPEED)
	};
	
	irq_initialize_vectors();
	sysclk_enable_peripheral_clock(&TWI_MASTER);
	
	TWI_MASTER_PORT.PIN0CTRL = PORT_OPC_WIREDANDPULL_gc;
	TWI_MASTER_PORT.PIN1CTRL = PORT_OPC_WIREDANDPULL_gc;

    twi_master_init(&TWI_MASTER, &m_options);
	twi_master_enable(&TWI_MASTER);
	cpu_irq_enable();
}


// возвращает температуру в виде числа
int get_temp() {
	status_code_t master_status;
	
    // пакет для запроса температуры с датчика
	twi_package_t packet_to_request_temp = {
		.addr_length = 0,
		.chip        = TWI_SLAVE_ADDR,
		.buffer      = read_data_cmd,
		.length      = 1,
		.no_wait     = false
	};
	while (twi_master_write(&TWI_MASTER, &packet_to_request_temp) != TWI_SUCCESS);
	
    //int temp = 0;

    // пакет для чтения температуры
	twi_package_t packet_to_recieve_temp = {
		.addr_length = 0,
		.chip        = TWI_SLAVE_ADDR,
		.buffer      = temperature_buff,
		.length      = 2,
		.no_wait     = false
	};
	while (twi_master_write(&TWI_MASTER, &packet_to_recieve_temp) != TWI_SUCCESS); // читает температуру в массив temperature_buff
	
	if (master_status == STATUS_OK) {blink();} // сигнал, что данные считались успешно (для дебага)
	
	// parse data to int (парс данные в число - это всё делает этот блок кода)
	volatile int result, tempr, whole, fract;
	whole = temperature_buff[0] << 8;
	fract = temperature_buff[1];
	tempr = whole | fract;
	tempr = tempr >> 4;

    return tempr;
}


// инициализирует usart и печатает тестовое "usart inited"
void usart_init() {
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


int main (void)
{		
	board_init();
	ioport_init(); 
	
    ioport_set_pin_dir(MY_LED, IOPORT_DIR_OUTPUT); // output dir

    SET_RX();
    SET_TX();

    usart_init();
    twi_init();

    char temp_str[33]; // переменная для записи в неё символьного значения температуры (см. дальше)
    while (1) {
        int temp = get_temp();
        itoa(temp / 16, temp_str, 10); // преобразует число в char для вывода по usart
        
        for (int i=0; i < 3; i++) {
            usart_putchar(USART_SERIAL, temp_str[i]);
        }

        memset(temp_str, 0, 33); // очищает temp_str
    }

    
	 /*while (1) {
		 ioport_set_pin_level(MY_LED,1);
		 _delay_ms(500);
		 ioport_set_pin_level(MY_LED,0);
		 _delay_ms(500);
	 }*/
}