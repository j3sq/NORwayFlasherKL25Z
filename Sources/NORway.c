/************************************************************************
 norflash.v - NOR flasher for PS3

 Copyright (C) 2010-2011  Hector Martin "marcan" <hector@marcansoft.com>

 This code is licensed to you under the terms of the GNU GPL, version 2;
 see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
 *************************************************************************
 NORway.c (v0.7) - Teensy++ 2.0 port by judges@eEcho.com
 *************************************************************************/

#include "NORway.h"
#include "derivative.h"
#include "USB1.h"


// Define data ports
//#define DATA1_PORT	GPIOD
//#define DATA1_PIN	PIND
#define DATA1_DDR	GPIOD_PDDR
#define DATA1_MASK	0xFFFFFF00

//#define DATA2_PORT	PORTC
//#define DATA2_PIN	PINC
#define DATA2_DDR	GPIOE_PDDR
#define DATA2_MASK	0xFFCFFFC0

// Define address line ports
//#define ADDR1_PORT	PORTF
//#define ADDR1_PIN	PINF
#define ADDR1_DDR	GPIOB_PDDR
#define ADDR1_MASK	0xFFFFF0F0

//#define ADDR2_PORT	PORTA
//#define ADDR2_PIN	PINA
#define ADDR2_DDR	GPIOC_PDDR
#define ADDR2_MASK  0xFFFFFF00

//#define ADDR3_PORT	PORTB
//#define ADDR3_PIN	PINB
#define ADDR3_DDR	GPIOC_PDDR
#define ADDR3_MASK  0xFFFCC0FF

// Define control port and pins
#define CONT_PORT_SET	GPIOA_PSOR
#define CONT_PORT_CLEAR	GPIOA_PCOR
#define CONT_PORT_READ	GPIOA_PDIR
#define CONT_PORT_WRITE	GPIOA_PDOR
#define CONT_DDR	GPIOA_PDDR
//#define CONT_PIN	GPIOA_PDIR
#define CONT_MASK	0xFFFCCFCF

#define CONT_CE		4		// Chip Enable
#define CONT_OE		5		// Output Enable
#define CONT_RESET	12		// Reset
#define CONT_WE		13		// Write Enable
#define CONT_RYBY	16		// Ready/Busy
#define CONT_TRI	17		// Tristate
// Define state
#define S_IDLE			0
#define S_DELAY			1
#define S_ADDR2			2
#define S_ADDR3			3
#define S_READING		4
#define S_WDATA1		5
#define S_WDATA2		6
#define S_WRITING		7
#define S_WAITING		8
#define S_WRITEWORD		9
#define S_WRITEWORDUBM	10
#define S_WRITEWBP		11

// Define block/sector size for reading/writing
#define BSS_4		0x01000	//2Kwords = 4KB
#define BSS_8		0x02000	//4Kwords = 8KB
#define BSS_64		0x10000	//32Kwords = 64KB
#define BSS_128		0x20000	//64Kwords = 128KB
#define BSS_WORD	0x00002	//word = 2Bytes




//delays macros
//each nop is approx 21nS
#define delay_100ns	asm("nop");asm("nop");asm("nop");asm("nop");asm("nop"); 
#define delay_200ns	asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");

void delay_ms(unsigned int delay) {
	int i;
	delay=delay*100;
	for (i = 0; i < delay; i++) {
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;
		delay_200ns;

	}
}
void init_IO(void){
	SIM_SCGC5 |=
			SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTC_MASK
					| SIM_SCGC5_PORTD_MASK | SIM_SCGC5_PORTE_MASK;
	
	//Disable interrupt on all used pins
	//PORTA
	PORTA_PCR4 =PORT_PCR_MUX(0x01); //No OR (look down) used here as the default is 111 for this pin
	PORTA_PCR4 &=~PORT_PCR_ISF_MASK;
	PORTA_PCR5 &=~PORT_PCR_ISF_MASK;
	PORTA_PCR12 &=~PORT_PCR_ISF_MASK;
	PORTA_PCR13 &=~PORT_PCR_ISF_MASK;
	PORTA_PCR16 &=~PORT_PCR_ISF_MASK;
	PORTA_PCR17 &=~PORT_PCR_ISF_MASK;
	//PORTB
	PORTB_PCR0 &=~PORT_PCR_ISF_MASK;
	PORTB_PCR1 &=~PORT_PCR_ISF_MASK;
	PORTB_PCR2 &=~PORT_PCR_ISF_MASK;
	PORTB_PCR3 &=~PORT_PCR_ISF_MASK;
	PORTB_PCR8 &=~PORT_PCR_ISF_MASK;
	PORTB_PCR9 &=~PORT_PCR_ISF_MASK;
	PORTB_PCR10 &=~PORT_PCR_ISF_MASK;
	PORTB_PCR11 &=~PORT_PCR_ISF_MASK;
	//PORTC
	PORTC_PCR0 &=~PORT_PCR_ISF_MASK;
	PORTC_PCR1 &=~PORT_PCR_ISF_MASK;
	PORTC_PCR2 &=~PORT_PCR_ISF_MASK;
	PORTC_PCR3 &=~PORT_PCR_ISF_MASK;
	PORTC_PCR4 &=~PORT_PCR_ISF_MASK;
	PORTC_PCR5 &=~PORT_PCR_ISF_MASK;
	PORTC_PCR6 &=~PORT_PCR_ISF_MASK;
	PORTC_PCR7 &=~PORT_PCR_ISF_MASK;
	PORTC_PCR8 &=~PORT_PCR_ISF_MASK;
	PORTC_PCR9 &=~PORT_PCR_ISF_MASK;
	PORTC_PCR10 &=~PORT_PCR_ISF_MASK;
	PORTC_PCR11 &=~PORT_PCR_ISF_MASK;
	PORTC_PCR12 &=~PORT_PCR_ISF_MASK;
	PORTC_PCR13 &=~PORT_PCR_ISF_MASK;
	PORTC_PCR16 &=~PORT_PCR_ISF_MASK;
	PORTC_PCR17 &=~PORT_PCR_ISF_MASK;
	//PORTD
	PORTD_PCR0 &=~PORT_PCR_ISF_MASK;
	PORTD_PCR1 &=~PORT_PCR_ISF_MASK;
	PORTD_PCR2 &=~PORT_PCR_ISF_MASK;
	PORTD_PCR3 &=~PORT_PCR_ISF_MASK;
	PORTD_PCR4 &=~PORT_PCR_ISF_MASK;
	PORTD_PCR5 &=~PORT_PCR_ISF_MASK;
	PORTD_PCR6 &=~PORT_PCR_ISF_MASK;
	PORTD_PCR7 &=~PORT_PCR_ISF_MASK;
	//PORTE
	PORTE_PCR0 &=~PORT_PCR_ISF_MASK;
	PORTE_PCR1 &=~PORT_PCR_ISF_MASK;
	PORTE_PCR2 &=~PORT_PCR_ISF_MASK;
	PORTE_PCR3 &=~PORT_PCR_ISF_MASK;
	PORTE_PCR4 &=~PORT_PCR_ISF_MASK;
	PORTE_PCR5 &=~PORT_PCR_ISF_MASK;
	PORTE_PCR20 &=~PORT_PCR_ISF_MASK;
	PORTE_PCR21 &=~PORT_PCR_ISF_MASK;

	//PORTA
	 
	PORTA_PCR5 |=PORT_PCR_MUX(0x01);
	PORTA_PCR12 |=PORT_PCR_MUX(0x01);
	PORTA_PCR13 |=PORT_PCR_MUX(0x01);
	PORTA_PCR16 |=PORT_PCR_MUX(0x01);
	PORTA_PCR17 |=PORT_PCR_MUX(0x01);
	//PORTB
	PORTB_PCR0 |=PORT_PCR_MUX(0x01);
	PORTB_PCR1 |=PORT_PCR_MUX(0x01);
	PORTB_PCR2 |=PORT_PCR_MUX(0x01);
	PORTB_PCR3 |=PORT_PCR_MUX(0x01);
	PORTB_PCR8 |=PORT_PCR_MUX(0x01);
	PORTB_PCR9 |=PORT_PCR_MUX(0x01);
	PORTB_PCR10 |=PORT_PCR_MUX(0x01);
	PORTB_PCR11 |=PORT_PCR_MUX(0x01);
	//PORTC
	PORTC_PCR0 |=PORT_PCR_MUX(0x01);
	PORTC_PCR1 |=PORT_PCR_MUX(0x01);
	PORTC_PCR2 |=PORT_PCR_MUX(0x01);
	PORTC_PCR3 |=PORT_PCR_MUX(0x01);
	PORTC_PCR4 |=PORT_PCR_MUX(0x01);
	PORTC_PCR5 |=PORT_PCR_MUX(0x01);
	PORTC_PCR6 |=PORT_PCR_MUX(0x01);
	PORTC_PCR7 |=PORT_PCR_MUX(0x01);
	PORTC_PCR8 |=PORT_PCR_MUX(0x01);
	PORTC_PCR9 |=PORT_PCR_MUX(0x01);
	PORTC_PCR10 |=PORT_PCR_MUX(0x01);
	PORTC_PCR11 |=PORT_PCR_MUX(0x01);
	PORTC_PCR12 |=PORT_PCR_MUX(0x01);
	PORTC_PCR13 |=PORT_PCR_MUX(0x01);
	PORTC_PCR16 |=PORT_PCR_MUX(0x01);
	PORTC_PCR17 |=PORT_PCR_MUX(0x01);
	//PORTD
	PORTD_PCR0 |=PORT_PCR_MUX(0x01);
	PORTD_PCR1 |=PORT_PCR_MUX(0x01);
	PORTD_PCR2 |=PORT_PCR_MUX(0x01);
	PORTD_PCR3 |=PORT_PCR_MUX(0x01);
	PORTD_PCR4 |=PORT_PCR_MUX(0x01);
	PORTD_PCR5 |=PORT_PCR_MUX(0x01);
	PORTD_PCR6 |=PORT_PCR_MUX(0x01);
	PORTD_PCR7 |=PORT_PCR_MUX(0x01);
	//PORTE
	PORTE_PCR0 |=PORT_PCR_MUX(0x01);
	PORTE_PCR1 |=PORT_PCR_MUX(0x01);
	PORTE_PCR2 |=PORT_PCR_MUX(0x01);
	PORTE_PCR3 |=PORT_PCR_MUX(0x01);
	PORTE_PCR4 |=PORT_PCR_MUX(0x01);
	PORTE_PCR5 |=PORT_PCR_MUX(0x01);
	PORTE_PCR20 |=PORT_PCR_MUX(0x01);
	PORTE_PCR21 |=PORT_PCR_MUX(0x01);
	
	
	//Enable Pull up for RY/BY
	PORTA_PCR16 |=(PORT_PCR_PE_MASK | PORT_PCR_PS_MASK) ;
}
void test_IO(void){
	DATA1_DDR |= (~DATA1_MASK); // set for output
	DATA2_DDR |= (~DATA2_MASK); // set for output
	ADDR1_DDR |= (~ADDR1_MASK); //set for output
	ADDR2_DDR |= (~ADDR2_MASK); //set for output
	ADDR3_DDR |= (~ADDR3_MASK); //set for output
	CONT_DDR |= (~CONT_MASK); //all control ports are  output
	for(;;){
		set_ADDR1(0xFF);
		set_ADDR2(0xFF);
		set_ADDR3(0xFF);
		set_DATA1(0xFF);
		set_DATA2(0xFF);
		CONT_PORT_WRITE |= ((1 << CONT_WE) | (1 << CONT_OE) | (1 << CONT_RESET)|(1 << CONT_CE)|(1 << CONT_RYBY)|(1 << CONT_TRI)); //HIGH
		delay_ms(5000);
		set_ADDR1(0x00);
		set_ADDR2(0x00);
		set_ADDR3(0x00);
		set_DATA1(0x00);
		set_DATA2(0x00);
		CONT_PORT_WRITE &=~ ((1 << CONT_WE) | (1 << CONT_OE) | (1 << CONT_RESET)|(1 << CONT_CE)|(1 << CONT_RYBY)|(1 << CONT_TRI)); //HIGH
		delay_ms(1000);
		
	}
}
static uint8_t cdc_buffer[USB1_DATA_BUFF_SIZE];
static uint8_t in_buffer[USB1_DATA_BUFF_SIZE];
void serial_write( unsigned char *str, unsigned char len) {
	CDC1_SendString(str);
	while (CDC1_App_Task(cdc_buffer, sizeof(cdc_buffer)) != ERR_OK) ;	
	//
}
void serial_putchar(const unsigned val) {

	CDC1_SendChar(val);
	while (CDC1_App_Task(cdc_buffer, sizeof(cdc_buffer)) != ERR_OK) ;	


}
char serial_getchar(){
	CDC1_App_Task(cdc_buffer, sizeof(cdc_buffer));
	char reply;
	CDC1_RecvChar(&reply);
	return reply;
}
void set_ADDR3(unsigned char address8) {
	unsigned int address32 = (address8 & 0x3F)<<8;
	address32 |=  (address8 & 0xC0)<<10;
	unsigned int oldValue = GPIOC_PDOR;
	oldValue &= ADDR3_MASK;
	GPIOC_PDOR = oldValue | address32;

}
void set_ADDR2(unsigned char address8) {
	unsigned int address32 = address8;    //& 0x3F;
	//address32=address32<<8;
	//if (address8 & 0x40) address32|=0x00010000;
	unsigned int oldValue = GPIOC_PDOR;
	oldValue &= ADDR2_MASK;
	GPIOC_PDOR = oldValue | address32;

}
void set_ADDR1(unsigned char address8) {
	unsigned int address32 = address8 & 0xF0;
	address32 = (address32 << 4) | (address8 & 0x0F);
	//if (address8 & 0x40) address32|=0x00010000;
	unsigned int oldValue = GPIOB_PDOR;
	oldValue &= ADDR1_MASK;
	GPIOB_PDOR = oldValue | address32;

}

void set_DATA1(unsigned char data8) {
	unsigned int data32 = data8;	//& 0x3F;
	//address32=address32<<8;
	//if (address8 & 0x40) address32|=0x00010000;
	unsigned int oldValue = GPIOD_PDOR;
	oldValue &= DATA1_MASK;
	GPIOD_PDOR = oldValue | data32;

}
unsigned char get_DATA1(void) {
	return (unsigned char) GPIOD_PDIR ;

}
void set_DATA2(unsigned char data8) {
	unsigned int data32 = data8 & 0x3F;
	//address32=address32<<8;
	//if (address8 & 0x40) address32|=0x00010000;
	data32 |= (data8 & 0xC0) << 14;
	unsigned int oldValue = GPIOE_PDOR;
	oldValue &= DATA2_MASK;
	GPIOE_PDOR = oldValue | data32;

}
unsigned char get_DATA2(void) {
	unsigned int tmp = GPIOE_PDIR;
	unsigned char dataR = tmp & 0x3F;
	unsigned char dataL = (tmp >> 14) & 0xC0;
	return (dataL | dataR);

}
void put_address(unsigned char address3, unsigned char address2,
		unsigned char address1) {
	/*//CONT_PORT |= (1<<CONT_CE); //HIGH
	 ADDR3_PORT = address3;
	 ADDR2_PORT = address2;
	 ADDR1_PORT = address1;
	 //CONT_PORT &= ~(1<<CONT_CE); //LOW*/
	//Ported Implementation
	set_ADDR3(address3);
	set_ADDR2(address2);
	set_ADDR1(address1);

}

void put_data(unsigned char data2, unsigned char data1) {
	/*
	 DATA2_PORT = data2;
	 DATA1_PORT = data1;
	 _delay_ns(100);
	 CONT_PORT &= ~(1<<CONT_WE); //LOW
	 CONT_PORT |= (1<<CONT_WE); //HIGH
	 */
	//Ported Implementation
	set_DATA2(data2);
	set_DATA1(data1);
	delay_100ns;
	CONT_PORT_CLEAR = (1 << CONT_WE); //LOW
	CONT_PORT_SET = (1 << CONT_WE); //HIGH
}

unsigned char ADDR1, ADDR2, ADDR3;
void addr_increment(unsigned char lock_address) {
	if (ADDR1 < 0xFF)
		ADDR1++;
	else {
		ADDR1 = 0;
		if (ADDR2 < 0xFF)
			ADDR2++;
		else {
			ADDR2 = 0;
			if (ADDR3 < 0x7F)
				ADDR3++;
			else
				ADDR3 = 0;
		}
	}

	if (lock_address == 0)
		return;

	//CONT_PORT |= (1<<CONT_CE); //HIGH

	set_ADDR3(ADDR3);
	set_ADDR2(ADDR2);
	set_ADDR1(ADDR1);

	//CONT_PORT &= ~(1<<CONT_CE); //LOW
}

unsigned char state_waiting1(unsigned char do_increment) {
	//wait 200ns for RY/BY to become active
	delay_200ns;
	
	unsigned int cnt = 0xFFFFFF; //approx. 17secs
	while (cnt > 0) {
		if (CONT_PORT_READ & (1 << CONT_RYBY)) {
			if (do_increment)
				addr_increment(1);
			return 1;
		}
		cnt--;
		delay_200ns;
		delay_200ns;

	}
	return 0;
}

unsigned char state_waiting2(unsigned char do_increment) {
	//wait 200ns for RY/BY to become active
	delay_200ns;

	while (1) {
		if (CONT_PORT_READ & (1 << CONT_RYBY)) {
			if (do_increment)
				addr_increment(1);
			break;
		}
		CDC1_App_Task(cdc_buffer, sizeof(cdc_buffer)); //added for testing
		/*		else {
		 //_delay_us(5);
		 DATA1_DDR = DATA2_DDR = 0x00; // set for input
		 CONT_PORT &= ~(1<<CONT_OE); //LOW
		 _delay_ns(100); //better safe than sorry

		 unsigned char status;
		 status = DATA1_PIN;

		 CONT_PORT |= (1<<CONT_OE); //HIGH
		 DATA1_DDR = DATA2_DDR = 0xFF;	// set for output

		 if (status & 0x20) { //time limits exceeded
		 if (do_increment)
		 addr_increment(1);
		 break;
		 }
		 } */
	}
	return S_IDLE;
}

unsigned char state_byte() {
	return ((CONT_PORT_READ & (1 << CONT_TRI)) ? 0x20 : 0)
			| ((CONT_PORT_READ & (1 << CONT_RESET)) ? 0x10 : 0)
			| ((CONT_PORT_READ & (1 << CONT_RYBY)) ? 0x08 : 0)
			| ((CONT_PORT_READ & (1 << CONT_CE)) ? 0x04 : 0)
			| ((CONT_PORT_READ & (1 << CONT_WE)) ? 0x02 : 0)
			| ((CONT_PORT_READ & (1 << CONT_OE)) ? 0x01 : 0);
}

//Porting Reached Here	
// pure serial receive takes 120secs for 16MB
void speedtest_receive() {
	short in_data;
	unsigned char buf_write[BSS_4];
	unsigned short i = 0;

	while (i < BSS_4) {	//receive buffer data
		if ((in_data = serial_getchar()) != -1)
			buf_write[i++] = in_data;
		else {
			serial_putchar('T');
			return;
		}
	}
	if (i != BSS_4) {	//if receiving timeout, prepare to send FAIL!
		serial_putchar('R');
		return;
	}
	serial_putchar('K');
}

// pure serial transmit takes 49secs for 16MB
void speedtest_send() {
	unsigned char buf_read[64];
	unsigned char buf_ix;
	unsigned int addr;

	addr = 0;
	buf_ix = 0;
	while (1) {
		buf_read[buf_ix++] = buf_ix;
		buf_read[buf_ix++] = buf_ix;
		if (buf_ix == 64) {
			serial_write(buf_read, buf_ix);
			buf_ix = 0;
		}
		if (addr++ == (BSS_128 / 2 - 1))
			break;
	}
}

unsigned char verify(const unsigned char *buffer, uint16_t len) {
	DATA1_DDR &= DATA1_MASK; // set for input
	DATA2_DDR &= DATA2_MASK; // set for input
	while (len) {
		CONT_PORT_WRITE &= ~(1 << CONT_OE); //LOW
		delay_100ns; //better safe than sorry

		if ((*buffer++ != get_DATA2()) || (*buffer++ != get_DATA1())) {
			CONT_PORT_WRITE |= (1 << CONT_OE); //HIGH
			return 1;
		}

		CONT_PORT_WRITE |= (1 << CONT_OE); //HIGH
		addr_increment(1);
		len -= 2;
	}

	DATA1_DDR |= (~DATA1_MASK); // set for output
	DATA2_DDR |= (~DATA2_MASK); // set for output
	return 0;
}

void initports() {
	DATA1_DDR |= (~DATA1_MASK); // set for output
	DATA2_DDR |= (~DATA2_MASK); // set for output
	ADDR1_DDR |= (~ADDR1_MASK); //set for output
	ADDR2_DDR |= (~ADDR2_MASK); //set for output
	ADDR3_DDR |= (~ADDR3_MASK); //set for output
	set_ADDR1(0);
	set_ADDR2(0);
	set_ADDR3(0);

	CONT_DDR |= (~CONT_MASK); //all control ports are always output

	CONT_DDR &= ~(1 << CONT_RYBY); //except RY/BY# (input)
	CONT_PORT_WRITE |= (1 << CONT_RYBY); //enable pull up

	CONT_PORT_WRITE &= ~((1 << CONT_TRI) | (1 << CONT_CE)); //LOW
	CONT_PORT_WRITE |= ((1 << CONT_WE) | (1 << CONT_OE) | (1 << CONT_RESET)); //HIGH

//	ADDR1_DDR = ADDR2_DDR = ADDR3_DDR = DATA1_DDR = DATA2_DDR = CONT_DDR = 0; //all ports are always input
//	ADDR1_PORT = ADDR2_PORT = ADDR3_PORT = DATA1_PORT = DATA2_PORT = CONT_PORT = 0; //disable pull ups for all ports

//	CONT_DDR |= (1<<CONT_TRI);
//	CONT_PORT &= ~(1<<CONT_TRI); //LOW
//	CONT_PORT |= (1<<CONT_TRI); //HIGH
}

void releaseports() {
	DATA1_DDR &= DATA1_MASK; // set for input
	DATA2_DDR &= DATA2_MASK; // set for input
	ADDR1_DDR &= ADDR1_MASK; // set for input
	ADDR2_DDR &= ADDR2_MASK; // set for input
	ADDR3_DDR &= ADDR3_MASK; // set for input

	set_ADDR1(0x00);
	set_ADDR2(0x00);
	set_ADDR3(0x00);
	set_DATA1(0x00);
	set_DATA2(0x00);

	//CONT_DDR |= (1<<CONT_TRI);
	//CONT_PORT |= (1<<CONT_TRI); //HIGH
}
void test_input(){
	DATA1_DDR &= DATA1_MASK; // set for input
	DATA2_DDR &= DATA2_MASK; // set for input
	ADDR1_DDR |= (~ADDR1_MASK); //set for output
	ADDR2_DDR |= (~ADDR2_MASK); //set for output
	ADDR3_DDR |= (~ADDR3_MASK); //set for output
	unsigned char tmp1; unsigned char tmp2;
	set_ADDR1(0x55);
	set_ADDR2(0x05);
	//set_ADDR3(0x01);
	for (;;){
		tmp1=get_DATA1();
		tmp2=get_DATA2();
		serial_putchar(tmp2);
		serial_putchar(tmp1);
		serial_putchar (0xDD);
		delay_ms(100);
	}
}
int NORway(void) {
	/*
	DATA1_DDR |= (~DATA1_MASK);	// set for output

	while (1) {
		set_DATA1(0x02);
		delay_ms(500);
		set_DATA1(0x00);
		delay_ms(500);
	}
	*/
	init_IO();
	releaseports();
	//CONT_DDR &=(CONT_MASK);
	//CONT_DDR|=(1<<CONT_TRI);
	//CONT_PORT_SET|=(1<<CONT_TRI);
	test_IO();
	//test_input();
	int16_t in_data;
	uint16_t addr, i;
	unsigned int bss;
	unsigned char vaddr1, vaddr2, vaddr3;
	unsigned char state, cycle, tx_data, tx_wr, buf_ix, do_increment, do_verify,
			offset_2nddie;
	unsigned char buf_read[64];
	unsigned char buf_write[BSS_4];
	unsigned char tmp1;
	unsigned char tmp2;

	// set for 8 MHz clock
	//CPU_PRESCALE(1);
	// set for 16 MHz clock
	//CPU_PRESCALE(0);

	//disable JTAG
	//MCUCR = (1<<JTD) | (1<<IVCE) | (0<<PUD);
	//MCUCR = (1<<JTD) | (0<<IVSEL) | (0<<IVCE) | (0<<PUD);

	//set all i/o lines to input
	releaseports();

	// Initialize the USB, and then wait for the host to set configuration.
	// If the Teensy is powered without a PC connected to the USB port,
	// this will wait forever.
	//UART_init();
	//while (!usb_configured()) /* wait */ ;

	//configure all i/o lines (and set tristate=low)
	initports();

	// Wait an extra second for the PC's operating system to load drivers
	// and do whatever it does to actually be ready for input
	delay_ms(1000);

	state = S_IDLE;
	bss = BSS_128;
	cycle = tx_data = do_increment = do_verify = tx_wr = buf_ix =
			offset_2nddie = 0;

	ADDR1 = ADDR2 = ADDR3 = 0;

	while (1) {
		// wait for the user to run client app
		// which sets DTR to indicate it is ready to receive.
		//while (!(usb_serial_get_control() & USB_SERIAL_RTS)) /* wait */ ;

		// discard anything that was received prior.  Sometimes the
		// operating system or other software will send a modem
		// "AT command", which can still be buffered.
		//usb_serial_flush_input();

		//while (usb_configured() && (usb_serial_get_control() & USB_SERIAL_RTS)) { // is user still connected?
		while (1) { // is user still connected?
			tx_wr = 0;
			switch (state) {
			case S_IDLE:
				if ((in_data = serial_getchar()) != -1) { 
					// command
					if (in_data == 0) {				//8'b00000000: NOP
					} else if (in_data == 1) {		//8'b00000001: READSTATE
						tx_data = state_byte();
						tx_wr = 1;
					} else if (in_data == 2) {		//8'b00000010: PING1
						tx_data = 0x42;
						tx_wr = 1;
					} else if (in_data == 3) {		//8'b00000011: PING2
						tx_data = 0xbd;
						tx_wr = 1;
					} else if (in_data == 4) {		//8'b00000100: BOOTLOADER
						//bootloader();
					} else if (in_data == 5) {		//8'b00000101: ADDR_INCR
						addr_increment(1);
					} else if ((in_data >> 1) == 3) {	//8'b0000011z: TRISTATE
						if (in_data & 1)
							initports();
						//CONT_PORT &= ~(1<<CONT_TRI); //LOW
						else
							releaseports();
						//CONT_PORT |= (1<<CONT_TRI); //HIGH
					} else if ((in_data >> 1) == 4) {		//8'b0000100z: RESET
						if (in_data & 1)
							CONT_PORT_WRITE &= ~(1 << CONT_RESET);
						else
							CONT_PORT_WRITE |= (1 << CONT_RESET);
					}
					/* else if ((in_data>>1)==6) {		//8'b0000110z: INIT/RELEASE PORTS
					 if (in_data & 1)
					 initports();
					 else
					 releaseports();
					 } */
					/* else if (in_data == 12) {		//8'b00001101: SPEEDTEST_READ
					 speedtest_send();
					 }
					 else if (in_data == 13) {		//8'b00001100: SPEEDTEST_WRITE
					 speedtest_receive();
					 } */
					else if ((in_data >> 1) == 6) {		//8'b0000110z: VERIFY
						do_verify = (in_data & 1);
					} else if ((in_data >> 1) == 7) {		//8'b0000111z: WAIT
						do_increment = (in_data & 1);
						state = S_WAITING;
					} else if ((in_data >> 3) == 2) {		//8'b00010zzz: READ
						if (in_data == 0x10)
							bss = BSS_4;
						else if (in_data == 0x11)
							bss = BSS_8;
						else if (in_data == 0x12)
							bss = BSS_64;
						else if (in_data == 0x13)
							bss = BSS_128;
						else
							bss = BSS_WORD;

						DATA1_DDR &= DATA1_MASK; // set for input
						DATA2_DDR &= DATA2_MASK; // set for input
						state = S_READING;
					} else if ((in_data >> 1) == 12) {	//8'b0001100z: WRITE
						CONT_PORT_WRITE |= (1 << CONT_OE); //HIGH
						CONT_PORT_WRITE &= ~((1 << CONT_WE) | (1 << CONT_CE)); //LOW
						do_increment = (in_data & 1);
						state = S_WDATA1;
					} else if ((in_data >> 1) == 0x0D) {//8'b0001101z: WRITEWORD
						CONT_PORT_WRITE |= (1 << CONT_OE); //HIGH
						CONT_PORT_WRITE &= ~(1 << CONT_CE); //LOW
						offset_2nddie = (in_data & 1) ? 0x40 : 0; //A22=HIGH for Samsung K8Q2815
						do_increment = 0;
						state = S_WRITEWORD;
					} else if ((in_data >> 1) == 0x0E) {//8'b0001110z: WRITEWORDUBM
						CONT_PORT_WRITE |= (1 << CONT_OE); //HIGH
						CONT_PORT_WRITE &= ~(1 << CONT_CE); //LOW
						offset_2nddie = (in_data & 1) ? 0x40 : 0; //A22=HIGH for Samsung K8Q2815
						do_increment = 0;
						state = S_WRITEWORDUBM;
					} else if ((in_data >> 1) == 0x0F) {//8'b0001111z: WRITEWBP
						CONT_PORT_WRITE |= (1 << CONT_OE); //HIGH
						CONT_PORT_WRITE &= ~(1 << CONT_CE); //LOW
						offset_2nddie = do_increment = 0;
						state = S_WRITEWBP;
					} else if ((in_data >> 6) == 1) {		//8'b01zzzzzz: DELAY
						cycle = (in_data << 2) >> 2;
						//cycle -=25;//-10; //added -10 to avoid usb dead locking
						state = S_DELAY;
					} else if ((in_data >> 7) == 1) {		//8'b1zzzzzzz: ADDR
						//CONT_PORT |= (1<<CONT_CE); //HIGH
						ADDR3 = ((in_data << 1) >> 1);
						set_ADDR3(ADDR3);
						state = S_ADDR2;
					}
				}
				break;

			case S_DELAY:
				if (cycle == 0)
					state = S_IDLE;
				else
					cycle -= 1;
				break;

			case S_ADDR2:
				if ((in_data = serial_getchar()) != -1) {
					ADDR2 = in_data;
					set_ADDR2(ADDR2);
					state = S_ADDR3;
				}
				break;

			case S_ADDR3:
				if ((in_data = serial_getchar()) != -1) {
					ADDR1 = in_data;
					set_ADDR1(ADDR1);
					//CONT_PORT &= ~(1<<CONT_CE); //LOW
					state = S_IDLE;
				}
				break;

			case S_READING:
				if (bss == BSS_WORD) {
					CONT_PORT_WRITE &= ~(1 << CONT_OE); //LOW
					delay_100ns; //better safe than sorry
					serial_putchar(get_DATA2());
					serial_putchar(get_DATA1());
					CONT_PORT_WRITE |= (1 << CONT_OE); //HIGH
				} else {
					addr = buf_ix = 0;
					while (1) {
						CONT_PORT_WRITE &= ~(1 << CONT_OE); //LOW
						delay_100ns; //better safe than sorry
						tmp2=get_DATA2();
						tmp1=get_DATA1();
						buf_read[buf_ix++] = tmp2;//get_DATA2();
						buf_read[buf_ix++] = tmp1;//get_DATA1();
						CONT_PORT_WRITE |= (1 << CONT_OE); //HIGH
						addr_increment(1);

						if (buf_ix == 64) {
							//serial_write(buf_read, buf_ix);
							for (buf_ix=0;buf_ix<64-1;buf_ix++)
								Tx1_Put(buf_read[buf_ix]);
							serial_putchar(buf_read[63]);
							buf_ix = 0;
						}
						if (addr++ == (bss / 2 - 1))
							break;
					}
				}

				DATA1_DDR |= (~DATA1_MASK); // set for output
				DATA2_DDR |= (~DATA2_MASK); // set for output
				state = S_IDLE;
				break;

			case S_WDATA1:
				if ((in_data = serial_getchar()) != -1) {
					set_DATA2(in_data);
					state = S_WDATA2;
				}
				break;

			case S_WDATA2:
				if ((in_data = serial_getchar()) != -1) {
					set_DATA1(in_data);
					delay_100ns;
					state = S_WRITING;
				}
				break;

			case S_WRITING:
				CONT_PORT_WRITE |= (1 << CONT_WE); //HIGH
				if (do_increment)
					addr_increment(1);
				state = S_IDLE;
				break;

			case S_WRITEWORD: //"single word program mode"
				state = S_IDLE;

				for (i = 0; i < BSS_4; i++) {	//receive buffer data
					if ((in_data = serial_getchar()) != -1)
						buf_write[i] = in_data;
					else
						break;
					
					if (((i & 0x007F)==0x7F) && (i != (BSS_4-1))){		//send conformation at every 32 byte interval
						serial_putchar(0x7C);
					}
				}
				if (i < BSS_4) {//if receiving timeout, prepare to send FAIL!
					tx_data = 'R';
					tx_wr = 1;
					break;			//and exit case
				}

				vaddr1 = ADDR1;
				vaddr2 = ADDR2;
				vaddr3 = ADDR3;

				for (i = 0; i < BSS_4; i += 2) {
					if ((buf_write[i] == 0xFF) && (buf_write[i + 1] == 0xFF)) {
						addr_increment(0);
						continue;
					}

					put_address(offset_2nddie, 0x5, 0x55);
					put_data(0x0, 0xAA);
					put_address(offset_2nddie, 0x2, 0xAA);
					put_data(0x0, 0x55);
					put_address(offset_2nddie, 0x5, 0x55);
					put_data(0x0, 0xA0);
					put_address(ADDR3, ADDR2, ADDR1);
					put_data(buf_write[i], buf_write[i + 1]);

					addr_increment(0);
					if (!state_waiting1(0))
						break;
				}
				if (i < BSS_4) {	//if ack timeout, prepare to send FAIL!
					tx_data = 'T';
					tx_wr = 1;
					break;			//and exit case
				}

				if (do_verify == 1) {
					ADDR1 = vaddr1;
					ADDR2 = vaddr2;
					ADDR3 = vaddr3;
					put_address(vaddr3, vaddr2, vaddr1);

					if (verify(buf_write, BSS_4) == 1) {
						tx_data = 'V';
						tx_wr = 1;
						break;			//and exit case
					}
				}

				tx_data = 'K';		//ALL OK! prepare OK response
				tx_wr = 1;
				break;

			case S_WRITEWORDUBM: //"single word unlock bypass mode"
				state = S_IDLE;

				for (i = 0; i < BSS_4; i++) {	//receive buffer data
					if ((in_data = serial_getchar()) != -1)
						buf_write[i] = in_data;
					else
						break;
					
					if (((i & 0x007F)==0x7F) && (i != (BSS_4-1))){		//send conformation at every 128 byte interval
						serial_putchar(0x7C);
					}
				}

				if (i < BSS_4) {//if receiving timeout, prepare to send FAIL!
					tx_data = 'R';
					tx_wr = 1;
					break;			//and exit case
				}

				vaddr1 = ADDR1;
				vaddr2 = ADDR2;
				vaddr3 = ADDR3;

				// enter unlock bypass mode
				put_address(offset_2nddie, 0x5, 0x55);
				put_data(0x0, 0xAA);
				put_address(offset_2nddie, 0x2, 0xAA);
				put_data(0x0, 0x55);
				put_address(offset_2nddie, 0x5, 0x55);
				put_data(0x0, 0x20);
				put_address(ADDR3, ADDR2, ADDR1);

				for (i = 0; i < BSS_4; i += 2) {
					if ((buf_write[i] == 0xFF) && (buf_write[i + 1] == 0xFF)) {
						addr_increment(1);
						continue;
					}

					put_data(0x0, 0xA0);
					delay_100ns;
					put_data(buf_write[i], buf_write[i + 1]);

					//wait for RY/BY and increment address
					if (!state_waiting1(1))
						break;
				}

				//exit unlock bypass mode
				put_data(0x0, 0x90);
				put_data(0x0, 0x0);

				if (i < BSS_4) {	//if ack timeout, prepare to send FAIL!
					tx_data = 'T';
					tx_wr = 1;
					break;			//and exit case
				}

				if (do_verify == 1) {
					ADDR1 = vaddr1;
					ADDR2 = vaddr2;
					ADDR3 = vaddr3;
					put_address(vaddr3, vaddr2, vaddr1);

					if (verify(buf_write, BSS_4) == 1) {
						tx_data = 'V';
						tx_wr = 1;
						break;			//and exit case
					}
				}

				tx_data = 'K';		//ALL OK! prepare OK response
				tx_wr = 1;
				break;

			case S_WRITEWBP: //"write buffer programming"
				state = S_IDLE;

				for (i = 0; i < BSS_4; i++) {	//receive buffer data
					if ((in_data = serial_getchar()) != -1)
						buf_write[i] = in_data;
					else
						break;
					
					if (((i & 0x007F)==0x7F) && (i != (BSS_4-1))){		//send conformation at every 32 byte interval
						serial_putchar(0x7C);
					}
				}
				if (i < BSS_4) {//if receiving timeout, prepare to send FAIL!
					tx_data = 'R';
					tx_wr = 1;
					break;			//and exit case
				}

				unsigned char saddr1, saddr2, saddr3, k;
				vaddr1 = ADDR1;
				vaddr2 = ADDR2;
				vaddr3 = ADDR3;

				for (i = 0; i < BSS_4; i += 64) {
					saddr1 = ADDR1;
					saddr2 = ADDR2;
					saddr3 = ADDR3;

					// enter write buffer programming mode
					put_address(0, 0x5, 0x55);
					put_data(0x0, 0xAA);
					put_address(0, 0x2, 0xAA);
					put_data(0x0, 0x55);
					put_address(saddr3, saddr2, saddr1);
					put_data(0x0, 0x25);
					put_address(saddr3, saddr2, saddr1);
					put_data(0x0, 0x1F);

					//put_address(saddr3, saddr2, saddr1);
					for (k = 0; k < 64; k += 2) {
						put_data(buf_write[i + k], buf_write[i + k + 1]);
						addr_increment(1);
					}

					put_address(saddr3, saddr2, saddr1);
					put_data(0x0, 0x29);

					//wait for RY/BY
					if (!state_waiting1(0))
						break;
				}

				if (i < BSS_4) {	//if ack timeout, prepare to send FAIL!
					tx_data = 'T';
					tx_wr = 1;
					break;			//and exit case
				}

				if (do_verify == 1) {
					ADDR1 = vaddr1;
					ADDR2 = vaddr2;
					ADDR3 = vaddr3;
					put_address(vaddr3, vaddr2, vaddr1);

					if (verify(buf_write, BSS_4) == 1) {
						tx_data = 'V';
						tx_wr = 1;
						break;			//and exit case
					}
				}

				tx_data = 'K';		//ALL OK! prepare OK response
				tx_wr = 1;
				break;

			case S_WAITING:
				state = state_waiting2(do_increment);
				break;

			default:
				break;
			}

			if (tx_wr == 1) {
				serial_putchar(tx_data);
			}
		}
	}
}
