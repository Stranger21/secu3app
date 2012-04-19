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

#ifndef _SECU3_INTRINSIC_H_
#define _SECU3_INTRINSIC_H_

#ifdef __ICCAVR__
 #include <inavr.h> //IAR's intrinsics

 //abstracting intrinsics
 #define _ENABLE_INTERRUPT() __enable_interrupt()
 #define _DISABLE_INTERRUPT() __disable_interrupt()
 #define _SAVE_INTERRUPT() __save_interrupt()
 #define _RESTORE_INTERRUPT(s) __restore_interrupt(s)
 #define _NO_OPERATION() __no_operation()
 #define _DELAY_CYCLES(cycles) __delay_cycles(cycles)
 #define _WATCHDOG_RESET() __watchdog_reset()

#else //AVR GCC
 #include <avr/eeprom.h>       //__EEGET(), __EEPUT() etc
 #include <util/delay_basic.h> //for _DELAY_CYCLES()

 //abstracting intrinsics
 #define _ENABLE_INTERRUPT() sei()
 #define _DISABLE_INTERRUPT() cli()
 #define _SAVE_INTERRUPT() SREG
 #define _RESTORE_INTERRUPT(s) SREG = (s)
 #define _NO_OPERATION() __asm__ __volatile__ ("nop" ::)
 #define _DELAY_CYCLES(cycles) _delay_loop_2(cycles / 4)
 #define _WATCHDOG_RESET() __asm__ __volatile__ ("wdr")

#endif

#endif //_SECU3_INTRINSIC_H_
