
#include <asf.h>


#define USART_SERIAL                     &USARTD0
#define USART_SERIAL_BAUDRATE            9600
#define USART_SERIAL_CHAR_LENGTH         USART_CHSIZE_8BIT_gc
#define USART_SERIAL_PARITY              USART_PMODE_DISABLED_gc
#define USART_SERIAL_STOP_BIT            false


int main (void)
{
	/* Insert system clock initialization code here (sysclk_init()). */

	board_init();

	uint8_t tx_buf[] = "\n\rHello AVR world ! : ";
	uint8_t tx_length = 22;
	volatile uint8_t received_byte;
	uint8_t i;

	/* Initialize the board.
	 * The board-specific conf_board.h file contains the configuration of
	 * the board initialization.
	 */
	board_init();
	sysclk_init();
	
	ioport_init();                      // call before using IOPORT service
	
	// configure UART pins
	ioport_set_port_mode(IOPORT_PORTA, PORTA.PIN0CTRL | PORTA.PIN1CTRL, IOPORT_MODE_TOTEM);
	ioport_disable_port(IOPORT_PORTA, PORTA.PIN0CTRL | PORTA.PIN1CTRL);
	

	// USART options.
	static usart_rs232_options_t USART_SERIAL_OPTIONS = {
		.baudrate = USART_SERIAL_BAUDRATE,
		.charlength = USART_SERIAL_CHAR_LENGTH,
		.paritytype = USART_SERIAL_PARITY,
		.stopbits = USART_SERIAL_STOP_BIT
	};

	// Initialize usart driver in RS232 mode
	//usart_init_rs232(USART_SERIAL, &USART_SERIAL_OPTIONS);
	stdio_serial_init(USART_SERIAL, &USART_SERIAL_OPTIONS);
	printf("\r\nHello, world!\r\n");

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
}
