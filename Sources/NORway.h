/*
 * NORway.h
 *
 *  Created on: Sep 29, 2014
 *      Author: Jafar
 */

#ifndef NORWAY_H_
#define NORWAY_H_


void init_pins(void);
void delay_ns(unsigned int delay);
void delay_ms(unsigned int delay);
void set_ADDR3(unsigned char address8);
void set_ADDR2(unsigned char address8);
void set_ADDR1(unsigned char address8);
void set_DATA2(unsigned char address8);
void set_DATA1(unsigned char address8);
int NORway(void);
void delay_ms(unsigned int delay);
#endif /* NORWAY_H_ */
