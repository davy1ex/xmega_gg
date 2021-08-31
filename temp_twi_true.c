#include <asf.h>
#include <avr/delay.h>

#define MY_LED	IOPORT_CREATE_PIN(PORTD, 5) // first led

#define USART_SERIAL                     &USARTC1
#define USART_SERIAL_BAUDRATE            115200
#define USART_SERIAL_CHAR_LENGTH         USART_CHSIZE_8BIT_gc
#define USART_SERIAL_PARITY              USART_PMODE_DISABLED_gc
#define USART_SERIAL_STOP_BIT            false

#define SET_TX() ioport_configure_pin(IOPORT_CREATE_PIN(PORTC, 7), IOPORT_DIR_OUTPUT )
#define SET_RX() ioport_configure_pin(IOPORT_CREATE_PIN(PORTC, 6), IOPORT_DIR_INPUT)

#define TWI_MASTER       TWIE
#define TWI_MASTER_PORT  PORTE
#define TWI_SLAVE        TWIE
#define TWI_SPEED        400
#define TWI_MASTER_ADDR  0x50
#define TWI_SLAVE_ADDR   0b1001000
#define DATA_LENGTH     8
TWI_Slave_t slave;

unsigned char read_data_cmd[] = {0x00};
unsigned char cfg_data_cmd[] = {0x01, 0xC1};// one shot, shut down, 10bits
unsigned char temperature_buff[3];

void blink() {
	ioport_set_pin_level(MY_LED,1);
	_delay_ms(500);
	ioport_set_pin_level(MY_LED,0);
	_delay_ms(500);
}




void send_and_recv_twi()
{
	status_code_t master_status;
	// master options
	twi_options_t m_options = {
		.speed     = TWI_SPEED,
		.chip      = TWI_MASTER_ADDR,
		.speed_reg = TWI_BAUD(sysclk_get_cpu_hz(), TWI_SPEED)
	};
	
	irq_initialize_vectors();
	sysclk_enable_peripheral_clock(&TWI_MASTER);
	
	uint8_t i;
	TWI_MASTER_PORT.PIN0CTRL = PORT_OPC_WIREDANDPULL_gc;
	TWI_MASTER_PORT.PIN1CTRL = PORT_OPC_WIREDANDPULL_gc;
	// Initialize TWI_MASTER
	/*sysclk_enable_peripheral_clock(&TWI_SLAVE);
	TWI_SlaveInitializeDriver(&slave, &TWI_SLAVE, *slave_process);
	TWI_SlaveInitializeModule(&slave, TWI_SLAVE_ADDR,
	TWI_SLAVE_INTLVL_MED_gc);C*/
	twi_master_init(&TWI_MASTER, &m_options);
	twi_master_enable(&TWI_MASTER);
	cpu_irq_enable();
	
	// configuraton tmp
	twi_package_t packet_init = {		
		.addr_length = 0,
		.chip        = TWI_SLAVE_ADDR,
		.buffer      = (void *) cfg_data_cmd,
		.length      = 2,
		.no_wait     = false
	};
	master_status = twi_master_write(&TWI_MASTER, &packet_init); // init conf to tmp	
	
	
	twi_package_t packet1 = {
		.addr_length = 0,
		.chip        = TWI_SLAVE_ADDR,
		.buffer      = read_data_cmd,
		.length      = 1,
		.no_wait     = false
	};
	while (twi_master_write(&TWI_MASTER, &packet1) != TWI_SUCCESS);
	//int temp = 0;
	uint8_t buf[2];
	int16_t temp;
	twi_package_t packet_to_recieve_temp = {
		.addr_length = 0,
		.chip        = TWI_SLAVE_ADDR,
		.buffer      = buf,
		.length      = 2,
		.no_wait     = false
	};
	
	
	master_status = twi_master_read(&TWI_MASTER, &packet_to_recieve_temp);
	if (master_status == STATUS_OK) {blink();}
	temp=(int16_t)((uint16_t)( buf[0]<<8 | buf[1] ));
	// send to usart raw data
	/*for (int i=0; i < 2; i++) {
		usart_putchar(USART_SERIAL, temperature_buff[i]);
	}
	usart_putchar(USART_SERIAL, "\n");*/
	
	// parse data and send by usart
	//volatile int result, tempr, whole, fract;
	//whole = temperature_buff[0] << 8;
	//fract = temperature_buff[1];
	//tempr = whole | fract;
	//tempr = tempr >> 4;
	
	//uint16_t tempr;
	//tempr = (temperature_buff[0] << 4) | (temperature_buff[1] >> 4);

char my_string[33];
	char my_string2[33];
	char my_string3[33];
	itoa(temp / 400, my_string, 10);
	
	//itoa(tempr / 16, my_string2, 16);
	//itoa(tempr / 16, my_string3, 2);
	//char my_string4 = tempr + '0';
	for (int i=0; i < 10; i++) {
		usart_putchar(USART_SERIAL, my_string[i]);
	}
	/*for (int i=0; i < 15; i++) {
		usart_putchar(USART_SERIAL, my_string2[i]);
	}
	for (int i=0; i < 15; i++) {
		usart_putchar(USART_SERIAL, my_string3[i]);
	}
	for (int i=0; i < 15; i++) {
		usart_putchar(USART_SERIAL, my_string4);
	}*/
	
	/*twi_package_t packet1 = {
		
		.addr_length = 0,
		.chip        = TWI_SLAVE_ADDR,
		.buffer      = TWI_SLAVE_ADDR,
		.length      = sizeof(TWI_SLAVE_ADDR),
		.no_wait     = false
	};
	twi_master_write(&TWI_MASTER, &packet1);
	blink();*/
	
	/*volatile int result, tempr, whole, fract;
	
	twi_master_read(&TWI_MASTER, &packet_to_recieve_temp);
	
	for (int i=0; i < 2; i++) {
		usart_putchar(USART_SERIAL, tempr);
	}*/
	
	
	// get temp
	//master_status = twi_master_write(&TWI_MASTER, &packet);
	
	//twi_master_write(&TWI_MASTER, &packet);
	/*do {
		// Nothing
	} while(slave.result != TWIS_RESULT_OK);
	ioport_set_pin_level(MY_LED,1);*/
}



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
	
	/* Insert system clock initialization code here (sysclk_init()). */
	
	
	board_init();
	ioport_init(); 
	ioport_set_pin_dir(MY_LED, IOPORT_DIR_OUTPUT); // output dir


SET_RX();
SET_TX();
usart_init();
while (1) {
	send_and_recv_twi();	
	_delay_ms(500);
}

	
	 /*while (1) {
		 ioport_set_pin_level(MY_LED,1);
		 _delay_ms(500);
		 ioport_set_pin_level(MY_LED,0);
		 _delay_ms(500);
	 }*/
}
