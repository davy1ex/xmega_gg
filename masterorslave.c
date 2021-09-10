#include <asf.h>
#include <avr/delay.h>
#include <string.h>

#define led_1	IOPORT_CREATE_PIN(PORTD, 5)

TWI_Slave_t slave;


// "���������" ��� ��������� usart
#define USART_SERIAL &USARTC1
#define USART_SERIAL_BAUDRATE            115200
#define USART_SERIAL_CHAR_LENGTH USART_CHSIZE_8BIT_gc
#define USART_SERIAL_PARITY USART_PMODE_DISABLED_gc
#define USART_SERIAL_STOP_BIT            false

// ����� ��� usart � ����������� � �������������
#define SET_TX() ioport_configure_pin(IOPORT_CREATE_PIN(PORTC, 7), IOPORT_DIR_OUTPUT )
#define SET_RX() ioport_configure_pin(IOPORT_CREATE_PIN(PORTC, 6), IOPORT_DIR_INPUT)


#define MASTER
#define SLAVE
// ��������� ��� ��������� twi. ��� �������� � 50� (TWI_SPEED) ��.
// �������� �� � �� 400 � �� 400000. ������� ���� ���� � ������� ������
// �.�. ��������� �������/�������� ������������

#define TWI_SPEED        50000

// TWI MASTER
#define TWI_MASTER TWIE
#define TWI_MASTER_PORT PORTE
#define TWI_MASTER_ADDR  0x50

// TWI SLAVE
#define TWI_SLAVE TWIE
#define TWI_SLAVE_ADDR_mk 0x42 // ��� ������� ��
#define TWI_SLAVE_ADDR1   0b1001000 //����� ������� U7
#define TWI_SLAVE_ADDR2   0b1001111 //����� ������� U8


#define DATA_LENGTH     8


unsigned char read_data_cmd[] = {0x00}; // �������� � ����� ��� ������� ����������� � �������
unsigned char cfg_data_cmd[] = {0x01, 0xA1}; // �������� � ����� ��� ������� �����-�� ��� �������� ��������. ���� ������: github.com/ncb85/utilis-and-examples/blob/master/cpm_env/i2c/tmp275.c
unsigned char temperature_buff[2]; // ���� ����� ������ �����������. �������� �� ������ ����
unsigned char temperature_buff2[2]; // ���� ����� ������ �����������. �������� �� ������ ����


void blink() {
	ioport_set_pin_level(led_1,1);
	_delay_ms(500);
	ioport_set_pin_level(led_1,0);
	_delay_ms(500);
}


void setup_ports(void) {
	/* ������������ ������ ��� ������ �� ����� */
	ioport_set_pin_dir(led_1, IOPORT_DIR_OUTPUT);
}


// �������������� usart � �������� �������� "usart inited"
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


static void twi_masteer_init() {
	// master options
	twi_options_t m_options = {
		.speed     = TWI_SPEED,
		.chip      = TWI_MASTER_ADDR ,
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


ISR(TWIE_TWIS_vect) {
	TWI_SlaveInterruptHandler(&slave);
}


static void slave_process() {
	slave.status = STATUS_OK;
	for(int i = 0; i < DATA_LENGTH; i++) {
		recv_data[i] = slave.receivedData[i];
		usart_putchar(USART_SERIAL, recv_data[i]);
	}
}


static void twi_slave_init() {
	twi_options_t m_options = {
		.speed     = 50000,  //speed
		.chip      = TWI_SLAVE_ADDR_mk,	//address
		.speed_reg = TWI_BAUD(sysclk_get_cpu_hz(), 50000)
	};
	sysclk_enable_peripheral_clock(&TWI_SLAVE); // ��� ����������
	twi_slave_setup(&TWIE, &m_options, &slave, &slave_process, 0x60,TWI_SLAVE_INTLVL_LO_gc);
}


void temp_init() {
	twi_package_t packet_init = {
		.addr_length = 0,
		.chip        = TWI_SLAVE_ADDR1,
		.buffer      = (void *) cfg_data_cmd,
		.length      = 1,
		.no_wait     = false
	};
	
	twi_package_t packet_init2 = {
		.addr_length = 0,
		.chip        = TWI_SLAVE_ADDR2,
		.buffer      = (void *) cfg_data_cmd,
		.length      = 1,
		.no_wait     = false
	};
	twi_master_write(&TWI_MASTER, &packet_init); // init conf to tmp	
	twi_master_write(&TWI_MASTER, &packet_init2); // init conf to tmp	
	
}


int get_temp7() {	
	// ����� ��� ������� ����������� � �������
	twi_package_t packet_to_request_temp = {
		.addr_length = 0,
		.chip        = TWI_SLAVE_ADDR1,
		.buffer      = read_data_cmd,
		.length      = 1,
		.no_wait     = false
	};
	twi_master_write(&TWI_MASTER, &packet_to_request_temp) != TWI_SUCCESS;
	
	// receive temp	
	twi_package_t packet_to_recieve_temp = {
		.addr_length = 0,
		.chip        = TWI_SLAVE_ADDR1,
		.buffer      = temperature_buff,
		.length      = 2,
		.no_wait     = false
	};
	twi_master_read(&TWI_MASTER, &packet_to_recieve_temp);

	int16_t temp;
	// parse data to int (���� ������ � ����� - ��� �� ������ ���� ���� ����)	
	temp=(int16_t)((uint16_t)( temperature_buff[0]<<8 | temperature_buff[1] ));

	return temp;
}


int get_temp8() {
	//status_code_t master_status;
	
	// ����� ��� ������� ����������� � �������
	twi_package_t packet_to_request_temp2 = {
		.addr_length = 0,
		.chip        = TWI_SLAVE_ADDR2,
		.buffer      = read_data_cmd,
		.length      = 1,
		.no_wait     = false
	};
	twi_master_write(&TWI_MASTER, &packet_to_request_temp2) != TWI_SUCCESS;
	
	
	// receive temp
	
	twi_package_t packet_to_recieve_temp2 = {
		.addr_length = 0,
		.chip        = TWI_SLAVE_ADDR2,
		.buffer      = temperature_buff2,
		.length      = 2,
		.no_wait     = false
	};
	twi_master_read(&TWI_MASTER, &packet_to_recieve_temp2); // ������ ����������� � ������ temperature_buff
	
	//
	//if (master_status == STATUS_OK) {blink();} // ������, ��� ������ ��������� ������� (��� ������)
	
	// parse data to int (���� ������ � ����� - ��� �� ������ ���� ���� ����)
	int16_t temp;
	// parse data to int (���� ������ � ����� - ��� �� ������ ���� ���� ����)
	temp=(int16_t)((uint16_t)( temperature_buff2[0]<<8 | temperature_buff2[1] ));
	return temp;
}


int main (void) {
	board_init();
	ioport_init();
	sysclk_init();
	setup_ports();
	
	
	

	usart_init();
	
	twi_init();
	temp_init();
	
	pmic_init();
	pmic_set_scheduling(PMIC_SCH_ROUND_ROBIN);

    bool sw1 = ioport_get_pin_level(sw1_pin);
    bool sw2 = ioport_get_pin_level(sw2_pin);
    bool sw3 = ioport_get_pin_level(sw3_pin);
    bool sw4 = ioport_get_pin_level(sw4_pin);
    bool sw5 = ioport_get_pin_level(sw5_pin);
    bool sw6 = ioport_get_pin_level(sw6_pin);
    bool sw7 = ioport_get_pin_level(sw7_pin);
    bool sw8 = ioport_get_pin_level(sw8_pin);
    bool sw9 = ioport_get_pin_level(sw9_pin);
    bool sw10 = ioport_get_pin_level(sw10_pin);
    bool sw11 = ioport_get_pin_level(sw11_pin);
    bool sw12 = ioport_get_pin_level(sw12_pin);

	char str[33];

	while (1)
	{ 
		sw1 = ioport_get_pin_level(sw1_pin);
		sw2 = ioport_get_pin_level(sw2_pin);
		sw3 = ioport_get_pin_level(sw3_pin);
		sw4 = ioport_get_pin_level(sw4_pin);
		sw5 = ioport_get_pin_level(sw5_pin);
		sw6 = ioport_get_pin_level(sw6_pin);
		sw7 = ioport_get_pin_level(sw7_pin);
		sw8 = ioport_get_pin_level(sw8_pin);
		sw9 = ioport_get_pin_level(sw9_pin);
		sw10 = ioport_get_pin_level(sw10_pin);
		sw11 = ioport_get_pin_level(sw11_pin);
		sw12 = ioport_get_pin_level(sw12_pin);
		sw_status = sw_status | (sw1 | (sw2<<1) | (sw3<<2) | (sw4<<3) | (sw5<<4) | (sw6<<5) |
		(sw7<<6)| (sw8<<7)| (sw9<<8) | (sw10<<9)| (sw11<<10) | (sw12<<11));
		sw1 = ioport_get_pin_level(sw1_pin);
	/*	itoa(sw_status / 16, str, 10); // ����������� ����� � char ��� ������ �� usart
		for (int i=0; i < 3; i++)
		{
			usart_putchar(USART_SERIAL, str[i]);
		}
		
		itoa(temp_u7_timer / 16, str, 10); // ����������� ����� � char ��� ������ �� usart
		for (int i=0; i < 3; i++)
		{
			usart_putchar(USART_SERIAL, str[i]);
		}
		
		itoa(temp_u8_timer / 16, str, 10); // ����������� ����� � char ��� ������ �� usart
		for (int i=0; i < 3; i++)
		{
			usart_putchar(USART_SERIAL, str[i]);
		}
		
		itoa(temp_u7_sw / 16, str, 10); // ����������� ����� � char ��� ������ �� usart
		for (int i=0; i < 3; i++)
		{
			usart_putchar(USART_SERIAL, str[i]);
		}
		
		itoa(temp_u8_sw / 16, str, 10); // ����������� ����� � char ��� ������ �� usart
		for (int i=0; i < 3; i++)
		{
			usart_putchar(USART_SERIAL, str[i]);
		}*/
	} 
}
