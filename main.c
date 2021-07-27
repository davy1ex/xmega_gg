#include <asf.h>


#define MY_LED	IOPORT_CREATE_PIN(PORTD, 5) // first led
#define MY_LED2	IOPORT_CREATE_PIN(PORTD, 7) // second led

#define sw1		IOPORT_CREATE_PIN(PORTA, 0) // first switch
#define sw2		IOPORT_CREATE_PIN(PORTA, 1) // second switch


void setup_ports(void) {
	ioport_set_pin_dir(MY_LED, IOPORT_DIR_OUTPUT); // output dir
	ioport_set_pin_dir(MY_LED2, IOPORT_DIR_OUTPUT); // output dir
	
	/* SWITCHES */
	ioport_set_pin_dir(sw1, IOPORT_DIR_INPUT);     // input dir
	ioport_set_pin_mode(sw1, IOPORT_MODE_PULLDOWN);// pulldown for pin
	
	ioport_set_pin_dir(sw2, IOPORT_DIR_INPUT);     // input dir
	ioport_set_pin_mode(sw2, IOPORT_MODE_PULLDOWN);// pulldown for pin
	
	
	//PORTA.PIN0CTRL = PORT_OPC_PULLUP_gc | PORT_ISC_BOTHEDGES_gc;
	PORTA.INT0MASK = PIN0_bm | PIN1_bm; // port for interrupts
	PORTA.INTCTRL = PORT_INT0LVL_LO_gc;
	PORTA.PIN0CTRL=PORT_ISC_BOTHEDGES_gc; // lvl for enable interrupt
	PORTA.PIN1CTRL=PORT_ISC_BOTHEDGES_gc; // lvl for enable interrupt
}

ISR(PORTA_INT0_vect) {
	if (ioport_get_pin_level(sw1)) { // if enable sw1
		ioport_set_pin_level(MY_LED,1);
		
	} else {
		ioport_set_pin_level(MY_LED,0); // else disable	 sw1
	}
	
	if (ioport_get_pin_level(sw2)) { // if enable sw2
		ioport_set_pin_level(MY_LED2,1);
	} else {
		ioport_set_pin_level(MY_LED2,0); // else disable sw2
	}
	
	
}



int main (void) {
	board_init();
	ioport_init();

	setup_ports();
	
	pmic_init();
	pmic_set_scheduling(PMIC_SCH_ROUND_ROBIN);
	
	cpu_irq_enable();
	
	while (1) {
		nop();
	}
	
}


/*
// Red LED on XmegaA3BU Xplained
#define LEDPORT PORTD
#define LEDPIN PIN4_bm //PORTD.4


ISR(PORTC_INT0_vect)
{
    // Toggle LED on interrupt
    LEDPORT.OUTTGL = LEDPIN;
}

int main(void)
{
    // External interrupt 0 on PC0, enable pullup, sence falling edge
    PORTC.PIN0CTRL = PORT_OPC_PULLUP_gc | PORT_ISC_FALLING_gc;
    PORTC.INT0MASK = PIN0_bm;
    PORTC.INTCTRL = PORT_INT0LVL_LO_gc;

    // LEDPORT output, toggled in ISR
    LEDPORT.DIRSET = LEDPIN;

    // Interupt signal on PC1, connect to PC0
    PORTC.DIRSET = PIN1_bm;

    // Enable low level interrupts
    PMIC.CTRL |= PMIC_LOLVLEN_bm;
    sei();

    while(1)
    {
        // Generate signal for interrupt, connect PC1 to PC0
        //PORTC.OUTTGL = PIN1_bm;
		nop();
        
    }
}
*/