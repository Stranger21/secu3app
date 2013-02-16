/* SECU-3  - An open source, free engine control unit
   Copyright (C) 2007 Alexey A. Shabelnikov. Ukraine, Gorlovka

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   contacts:
              http://secu-3.org
              email: shabelnikov@secu-3.org
*/

/** \file ioconfig.�
 * Implementation of I/O configuration. Allows I/O functions remapping
 * (���������� �������������� �������).
 */

#include "port/avrio.h"
#include "port/port.h"
#include "bitmask.h"
#include <stdint.h>

void iocfg_i_ign_out1(uint8_t value)
{
 WRITEBIT(PORTD, PD4, value);
 DDRD |= _BV(DDD4);
}

void iocfg_i_ign_out1i(uint8_t value)  //inverted version
{
 WRITEBIT(PORTD, PD4, !value);
 DDRD |= _BV(DDD4);
}

void iocfg_i_ign_out2(uint8_t value)
{
 WRITEBIT(PORTD, PD5, value);
 DDRD |= _BV(DDD5);
}

void iocfg_i_ign_out2i(uint8_t value)  //inverted version
{
 WRITEBIT(PORTD, PD5, !value);
 DDRD |= _BV(DDD5);
}

void iocfg_s_ign_out1(uint8_t value)
{
 WRITEBIT(PORTD, PD4, value);
}

void iocfg_s_ign_out1i(uint8_t value)  //inverted version
{
 WRITEBIT(PORTD, PD4, !value);
}

void iocfg_s_ign_out2(uint8_t value)
{
 WRITEBIT(PORTD, PD5, value);
}

void iocfg_s_ign_out2i(uint8_t value)  //inverted version
{
 WRITEBIT(PORTD, PD5, !value);
}

void iocfg_i_ign_out3(uint8_t value)
{
 WRITEBIT(PORTC, PC0, value);
 DDRC |= _BV(DDC0);
}

void iocfg_i_ign_out3i(uint8_t value)  //inverted version
{
 WRITEBIT(PORTC, PC0, !value);
 DDRC |= _BV(DDC0);
}

void iocfg_s_ign_out3(uint8_t value)
{
 WRITEBIT(PORTC, PC0, value);
}

void iocfg_s_ign_out3i(uint8_t value)  //inverted version
{
 WRITEBIT(PORTC, PC0, !value);
}

void iocfg_i_ign_out4(uint8_t value)
{
 WRITEBIT(PORTC, PC1, value);
 DDRC |= _BV(DDC1);
}

void iocfg_i_ign_out4i(uint8_t value)  //inverted version
{
 WRITEBIT(PORTC, PC1, !value);
 DDRC |= _BV(DDC1);
}

void iocfg_s_ign_out4(uint8_t value)
{
 WRITEBIT(PORTC, PC1, value);
}

void iocfg_s_ign_out4i(uint8_t value)  //inverted version
{
 WRITEBIT(PORTC, PC1, !value);
}

#ifdef SECU3T
void iocfg_i_add_io1(uint8_t value)
{
 WRITEBIT(PORTC, PC5, value);
 DDRC |= _BV(DDC5);
}

void iocfg_i_add_io1i(uint8_t value)   //inverted version
{
 WRITEBIT(PORTC, PC5, !value);
 DDRC |= _BV(DDC5);
}

void iocfg_s_add_io1(uint8_t value)
{
 WRITEBIT(PORTC, PC5, value);
}

void iocfg_s_add_io1i(uint8_t value)   //inverted version
{
 WRITEBIT(PORTC, PC5, !value);
}

void iocfg_i_add_io2(uint8_t value)
{
 WRITEBIT(PORTA, PA4, value);
 DDRA |= _BV(DDA4);
}

void iocfg_i_add_io2i(uint8_t value)   //inverted version
{
 WRITEBIT(PORTA, PA4, !value);
 DDRA |= _BV(DDA4);
}

void iocfg_s_add_io2(uint8_t value)
{
 WRITEBIT(PORTA, PA4, value);
}

void iocfg_s_add_io2i(uint8_t value)   //inverted version
{
 WRITEBIT(PORTA, PA4, !value);
}
#endif

void iocfg_i_ecf(uint8_t value)
{
#ifdef SECU3T /*SECU-3T*/
#ifdef REV9_BOARD
 WRITEBIT(PORTD, PD7, value);
#else
 WRITEBIT(PORTD, PD7, !(value));
#endif
 DDRD |= _BV(DDD7);
#else         /*SECU-3*/
 WRITEBIT(PORTB, PB1, value);
 DDRB |= _BV(DDB1);
#endif
}

void iocfg_i_ecfi(uint8_t value)       //inverted version
{
#ifdef SECU3T /*SECU-3T*/
#ifdef REV9_BOARD
 WRITEBIT(PORTD, PD7, !value);
#else
 WRITEBIT(PORTD, PD7, value);
#endif
 DDRD |= _BV(DDD7);
#else         /*SECU-3*/
 WRITEBIT(PORTB, PB1, !value);
 DDRB |= _BV(DDB1);
#endif
}

void iocfg_s_ecf(uint8_t value)
{
#ifdef SECU3T /*SECU-3T*/
#ifdef REV9_BOARD
 WRITEBIT(PORTD, PD7, value);
#else
 WRITEBIT(PORTD, PD7, !(value));
#endif
#else         /*SECU-3*/
 WRITEBIT(PORTB, PB1, value);
#endif
}

void iocfg_s_ecfi(uint8_t value)       //inverted version
{
#ifdef SECU3T /*SECU-3T*/
#ifdef REV9_BOARD
 WRITEBIT(PORTD, PD7, !value);
#else
 WRITEBIT(PORTD, PD7, value);
#endif
#else         /*SECU-3*/
 WRITEBIT(PORTB, PB1, !value);
#endif
}

void iocfg_i_st_block(uint8_t value)
{
#ifdef SECU3T /*SECU-3T*/
#ifdef REV9_BOARD
 WRITEBIT(PORTB, PB1, value);
#else
 WRITEBIT(PORTB, PB1, !(value));
#endif
 DDRB |= _BV(DDB1);
#else         /*SECU-3*/
 WRITEBIT(PORTD, PD7, value); //! ���� �������� �� � ���� �� ��� 
 DDRD |= _BV(DDD7);
#endif
}

void iocfg_i_st_blocki(uint8_t value)  //inverted version
{
#ifdef SECU3T /*SECU-3T*/
#ifdef REV9_BOARD
 WRITEBIT(PORTB, PB1, !value);
#else
 WRITEBIT(PORTB, PB1, value);
#endif
 DDRB |= _BV(DDB1);
#else         /*SECU-3*/
 WRITEBIT(PORTD, PD7, value);
 DDRD |= _BV(DDD7);
#endif
}

void iocfg_s_st_block(uint8_t value)
{
#ifdef SECU3T /*SECU-3T*/
#ifdef REV9_BOARD
 WRITEBIT(PORTB, PB1, value);
#else
 WRITEBIT(PORTB, PB1, !(value));
#endif
#else         /*SECU-3*/
 WRITEBIT(PORTD, PD7, value); //! ���� �������� �� � ���� �� ���
#endif
}

void iocfg_s_st_blocki(uint8_t value)  //inverted version
{
#ifdef SECU3T /*SECU-3T*/
#ifdef REV9_BOARD
 WRITEBIT(PORTB, PB1, !value);
#else
 WRITEBIT(PORTB, PB1, value);
#endif
#else         /*SECU-3*/
 WRITEBIT(PORTD, PD7, value);
#endif
}

void iocfg_i_ie(uint8_t value)
{
 WRITEBIT(PORTB, PB0, value);
 DDRB |= _BV(DDB0);
}

void iocfg_i_iei(uint8_t value)        //inverted version
{
 WRITEBIT(PORTB, PB0, !value);
 DDRB |= _BV(DDB0);
}

void iocfg_s_ie(uint8_t value)         
{
 WRITEBIT(PORTB, PB0, value);
}

void iocfg_s_iei(uint8_t value)        //inverted version
{
 WRITEBIT(PORTB, PB0, !value);
}

void iocfg_i_fe(uint8_t value)
{
 WRITEBIT(PORTC, PC7, value);
 DDRC |= _BV(DDC7);
}

void iocfg_i_fei(uint8_t value)        //inverted version
{
 WRITEBIT(PORTC, PC7, !value);
 DDRC |= _BV(DDC7);
}

void iocfg_s_fe(uint8_t value)
{
 WRITEBIT(PORTC, PC7, value);
}

void iocfg_s_fei(uint8_t value)        //inverted version
{
 WRITEBIT(PORTC, PC7, !value);
}

void iocfg_s_stub(uint8_t nil)
{
 //this is a stub!
}

void iocfg_i_ps(uint8_t value)
{
#ifdef SECU3T
 WRITEBIT(PORTD, PD3, value);  //controlls pullup resistor
 DDRD &= ~_BV(DDD3);           //input
#else /*SECU-3*/
 WRITEBIT(PORTC, PC4, value);
 DDRC &= ~_BV(DDC4);
#endif
}

void iocfg_i_psi(uint8_t value)
{
#ifdef SECU3T
 WRITEBIT(PORTD, PD3, value);  //controlls pullup resistor
 DDRD &= ~_BV(DDD3);           //input
#else /*SECU-3*/
 WRITEBIT(PORTC, PC4, value);
 DDRC &= ~_BV(DDC4);
#endif
}

uint8_t iocfg_g_ps(void)
{
#ifdef SECU3T
 return !!CHECKBIT(PIND, PIND3);
#else /*SECU-3*/
 return !!CHECKBIT(PINC, PINC4);
#endif
}

uint8_t iocfg_g_psi(void)              //inverted version
{
#ifdef SECU3T
 return !CHECKBIT(PIND, PIND3);
#else /*SECU-3*/
 return !CHECKBIT(PINC, PINC4);
#endif
}

#ifdef SECU3T
//value bits: xxxx xxoi
//o - point than pullup resistor must be connected (1), or disconnected (0)
//i - point than linked output must be set to 1, or 0 (this bit applicable only for ADD_IOx lines)
void iocfg_i_add_i1(uint8_t value)
{
 WRITEBIT(PORTA, PA6, (value & _BV(0))); //controlls pullup resistor
 CLEARBIT(DDRA, DDA6);                   //input
 WRITEBIT(PORTC, PC5, (value & _BV(1))); //set output value
 DDRC |= _BV(DDC5);                      //output
}

void iocfg_i_add_i1i(uint8_t value)
{
 WRITEBIT(PORTA, PA6, (value & _BV(0))); //controlls pullup resistor
 CLEARBIT(DDRA, DDA6);                   //input
 WRITEBIT(PORTC, PC5, (value & _BV(1))); //set output value
 DDRC |= _BV(DDC5);                      //output
}

uint8_t iocfg_g_add_i1(void)
{
 return !!CHECKBIT(PINA, PINA6);
}

uint8_t iocfg_g_add_i1i(void)          //inverted version
{
 return !CHECKBIT(PINA, PINA6);
}

//value bits: xxxx xxoi
//o - point than pullup resistor must be connected (1), or disconnected (0)
//i - point than linked output must be set to 1, or 0 (this bit applicable only for ADD_IOx lines)
void iocfg_i_add_i2(uint8_t value)
{
 WRITEBIT(PORTA, PA5, (value & _BV(0))); //controlls pullup resistor
 CLEARBIT(DDRA, DDA5);                   //input
 WRITEBIT(PORTA, PA4, (value & _BV(1))); //set output value
 DDRA |= _BV(DDA4);                      //output
}

void iocfg_i_add_i2i(uint8_t value)
{
 WRITEBIT(PORTA, PA5, (value & _BV(0))); //controlls pullup resistor
 CLEARBIT(DDRA, DDA5);                   //input
 WRITEBIT(PORTA, PA4, (value & _BV(1))); //set output value
 DDRA |= _BV(DDA4);                      //output
}

uint8_t iocfg_g_add_i2(void)
{
 return !!CHECKBIT(PINA, PINA5);
}

uint8_t iocfg_g_add_i2i(void)          //inverted version
{
 return !CHECKBIT(PINA, PINA5);
}
#endif

uint8_t iocfg_g_stub(void)
{
 //this is a stub! Always return 0
 return 0;
}
