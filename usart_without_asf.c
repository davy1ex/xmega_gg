#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#define BAUDCTRL_VALUE		1	// set accordingly

void clock_init(void);
FILE* usart_init(void);
int usart_get(FILE* f);
int usart_put(char c, FILE* f);

FILE us = FDEV_SETUP_STREAM(usart_put, usart_get, _FDEV_SETUP_RW);

int main(void)
{
	char line[128];

	clock_init();
	stdin = stdout = stderr = usart_init();

	do
	{
		puts_P(PSTR("\n\nHello, World!\nType something then press 'Enter'.\n"));
		gets(line);
		puts_P(PSTR("You typed:\n"));
		puts(line);

	} while (1);

	return 0;
}

void clock_init(void)
{
	OSC_XOSCCTRL = OSC_FRQRANGE_12TO16_gc | OSC_XOSCSEL_XTAL_16KCLK_gc;
	OSC_CTRL |= OSC_XOSCEN_bm;
	while(!(OSC_STATUS & OSC_XOSCRDY_bm));
	_PROTECTED_WRITE(CLK_CTRL, CLK_SCLKSEL_XOSC_gc);
	OSC_CTRL &= ~OSC_RC2MEN_bm;
}

FILE* usart_init(void)
{
	PORTC.OUTSET = PIN3_bm;
	PORTC.DIRSET = PIN3_bm;
	PORTC.DIRCLR = PIN2_bm;
	USARTC0.BAUDCTRLB = BAUDCTRL_VALUE >> 8;
	USARTC0.BAUDCTRLA = BAUDCTRL_VALUE & 255;
	USARTC0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc;
	USARTC0.CTRLB = USART_RXEN_bm | USART_TXEN_bm;

	return &us;
}

int usart_get(FILE* f)
{
	int c;

	while (!(USARTC0.STATUS & USART_RXCIF_bm));
	c = USARTC0.DATA;
	if (c == '\r') c = '\n';
	usart_put((char)c, f);

	return c;
}

int usart_put(char c, FILE* f)
{
	if (c == '\n') usart_put('\r', f);
	while (!(USARTC0.STATUS & USART_DREIF_bm));
	USARTC0.DATA = c;

	return 0;
}