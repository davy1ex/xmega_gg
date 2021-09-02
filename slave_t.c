#include <asf.h>
#include <string.h>
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
#define TWI_SPEED        50000
#define TWI_MASTER_ADDR  0x50
#define TWI_SLAVE_ADDR   0x60
#define DATA_LENGTH     1
TWI_Slave_t slave;
int data[DATA_LENGTH] = {0};
	int data2[DATA_LENGTH] = {1};
uint8_t recv_data[DATA_LENGTH] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
void blink() {
	ioport_set_pin_level(MY_LED,1);
	_delay_ms(500);
	ioport_set_pin_level(MY_LED,0);
	_delay_ms(500);
}
static void slave_process(void) {
	int i;
	slave.status = STATUS_OK;
	for(i = 0; i < DATA_LENGTH; i++) {
		recv_data[i] = slave.receivedData[i];
		usart_putchar(USART_SERIAL, recv_data[i]);
	}
	if (recv_data[0] == 1) {
		blink();
		blink();
		blink();
		int data[1] = {0x42};
		slave.sendData[1] = 48;
	}
	
	
	
}
ISR(TWIE_TWIS_vect) {
	TWI_SlaveInterruptHandler(&slave);
}


void send_and_recv_twi()
{
	status_code_t master_status;
	twi_package_t packet = {
		.addr_length = 0,
		.chip        = TWI_SLAVE_ADDR,
		.buffer      = (void *)data,
		.length      = DATA_LENGTH,
		.no_wait     = false
	};
	
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
	
	blink();
	
	twi_master_init(&TWI_MASTER, &m_options);
	twi_master_enable(&TWI_MASTER);

// Initialize TWI_SLAVE
	sysclk_enable_peripheral_clock(&TWI_SLAVE);
	TWI_SlaveInitializeDriver(&slave, &TWI_SLAVE, *slave_process);
	TWI_SlaveInitializeModule(&slave, TWI_SLAVE_ADDR,
	TWI_SLAVE_INTLVL_MED_gc);
	/*for (i = 0; i < TWIS_SEND_BUFFER_SIZE; i++) {
		slave.receivedData[i] = 0;
	}*/
	
	cpu_irq_enable();
	
	
	
	
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


SET_TX();
SET_RX();
sysclk_init();
usart_init();
send_and_recv_twi();
twi_package_t packet = {
	.addr_length = 0,
	.chip        = TWI_SLAVE_ADDR,
	.buffer      = (void *)data,
	.length      = DATA_LENGTH,
	.no_wait     = false
};

twi_package_t packet2 = {
	.addr_length = 0,
	.chip        = TWI_SLAVE_ADDR,
	.buffer      = (void *)data2,
	.length      = DATA_LENGTH,
	.no_wait     = false
};
char buff[2];
twi_package_t packet3 = {
	.addr_length = 0,
	.chip        = TWI_SLAVE_ADDR,
	.buffer      = buff,
	.length      = 2,
	.no_wait     = false
};
while (1) {
	memset(buff, 0, 2);
	 twi_master_write(&TWI_MASTER, &packet2);
	blink();
	_delay_ms(500);
	twi_master_write(&TWI_MASTER, &packet2);
	_delay_ms(500);
	twi_master_read(&TWI_MASTER, &packet3);
	usart_putchar(USART_SERIAL, buff[0]);
	usart_putchar(USART_SERIAL, buff[1]);
	usart_putchar(USART_SERIAL, buff[2]);
	
	_delay_ms(20000);
}
	/* Insert application code here, after the board has been initialized. */
	 /*while (1) {
		 ioport_set_pin_level(MY_LED,1);
		 _delay_ms(500);
		 ioport_set_pin_level(MY_LED,0);
		 _delay_ms(500);
	 }*/
}