#include <asf.h>
#include <avr/delay.h>

#define USART_SERIAL                     &USARTC1
#define USART_SERIAL_BAUDRATE            115200
#define USART_SERIAL_CHAR_LENGTH         USART_CHSIZE_8BIT_gc
#define USART_SERIAL_PARITY              USART_PMODE_DISABLED_gc
#define USART_SERIAL_STOP_BIT            false

#define MY_LED	IOPORT_CREATE_PIN(PORTD, 5) // first led


int main (void)
{
	ioport_set_pin_dir(MY_LED, IOPORT_DIR_OUTPUT); // output dir

	board_init();

	uint8_t tx_buf[] = "\n\rHello AVR world ! : ";
	uint8_t tx_length = 22;
	volatile uint8_t received_byte;
	uint8_t i;
	
	ioport_init();                      // call before using IOPORT service
	
	// configure UART pins
	//ioport_set_port_mode(IOPORT_PORTC, PORTC.PIN7CTRL | PORTC.PIN7CTRL, IOPORT_MODE_TOTEM);
	//ioport_disable_port(IOPORT_PORTC, PORTC.PIN6CTRL | PORTC.PIN7CTRL);
	

	// USART options.
	static usart_serial_options_t  USART_SERIAL_OPTIONS = {
		.baudrate = USART_SERIAL_BAUDRATE,
		.charlength = USART_SERIAL_CHAR_LENGTH,
		.paritytype = USART_SERIAL_PARITY,
		.stopbits = USART_SERIAL_STOP_BIT
	};
	sysclk_enable_module(SYSCLK_PORT_C, PR_USART1_bm);
	usart_serial_init(USART_SERIAL, &USART_SERIAL_OPTIONS);
	
	sysclk_init();
	
	printf("\r\nHello, world!\r\n");

	 const char str1[] = "Type 'a' to continue...\r\n";
	 uint8_t rx_char = 0;
	 usart_serial_write_packet(USART_SERIAL, (const uint8_t*)str1, sizeof(str1) - 1);
	 ioport_set_pin_level(MY_LED,1);
	  usart_serial_putchar(USART_SERIAL, 'A');
	  usart_putchar(USART_SERIAL, 'a'); 
	  ioport_set_pin_level(MY_LED,0);
	 do {
		 // get a single character
		 usart_serial_getchar(USART_SERIAL, &rx_char);
	 } while (rx_char != 'a');
	 ioport_set_pin_level(MY_LED,1);
	 // send a single character
	 usart_serial_putchar(USART_SERIAL, 'A');
	 
	 while (1) {
		  ioport_set_pin_level(MY_LED,1);
		  _delay_ms(500);
		   ioport_set_pin_level(MY_LED,0);
		   _delay_ms(500);
	 }
	 
/*
	// Send "message header"
	for (i = 0; i < tx_length; i++) {
		usart_putchar(USART_SERIAL, tx_buf[i]);
	}
	// Get and echo a character forever, specific '\r' processing.
	while (true) {
		received_byte = usart_getchar(USART_SERIAL);
		if (received_byte == '\r') {
			for (i = 0; i < tx_length; i++) {
				usart_putchar(USART_SERIAL, tx_buf[i]);
			}
		} else
			usart_putchar(USART_SERIAL, received_byte);
	}
	*/
}