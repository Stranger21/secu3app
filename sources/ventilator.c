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

/** \file ventilator.c
 * Implementation of cooling fan's control related functions.
 * (���������� ������� ��� ���������� ������������� ������������).
 */

#include "port/avrio.h"
#include "port/interrupt.h"
#include "port/intrinsic.h"
#include "port/port.h"
#include "bitmask.h"
#include "ioconfig.h"
#include "secu3.h"
#include "ventilator.h"

/**Turns on/off cooling fan
 * This is redundant definitions (see ioconfig.c), but it is opportunity to
 * speed up corresponding ISR
 */
#ifdef SECU3T /*SECU-3T*/

#ifdef REV9_BOARD
#ifdef COOLINGFAN_PWM
 #define REL_DUTY(duty) (PWM_STEPS - duty)
 #define ECF_VAL(value) !(value)
 #define COOLINGFAN_TURNON()  CLEARBIT(PORTD, PD7)
 #define COOLINGFAN_TURNOFF() SETBIT(PORTD, PD7)
#else
 #define ECF_VAL(value) (value)
 #define COOLINGFAN_TURNON()  SETBIT(PORTD, PD7)
 #define COOLINGFAN_TURNOFF() CLEARBIT(PORTD, PD7)
#endif
#else //REV6
#ifdef COOLINGFAN_PWM
 #define REL_DUTY(duty) (PWM_STEPS - duty)
 #define ECF_VAL(value) !(value)
 #define COOLINGFAN_TURNON()  SETBIT(PORTD, PD7)
 #define COOLINGFAN_TURNOFF() CLEARBIT(PORTD, PD7)
#else
 #define ECF_VAL(value) (value)
 #define COOLINGFAN_TURNON()  CLEARBIT(PORTD, PD7)
 #define COOLINGFAN_TURNOFF() SETBIT(PORTD, PD7)
#endif
#endif

#else         /*SECU-3*/
 #define REL_DUTY(duty) (duty)
 #define ECF_VAL(value) (value)
 #define COOLINGFAN_TURNON()  SETBIT(PORTB, PB1)
 #define COOLINGFAN_TURNOFF() CLEARBIT(PORTB, PB1)
#endif

/**Warning must be the same as another definition in vstimer.h!*/
#define TIMER2_RELOAD_VALUE  6

/**will be added to TIMER2_RELOAD_VALUE at the initialization */
#define COMPADD 5

/**number of PWM discretes */
#define PWM_STEPS 25

volatile uint8_t pwm_state; //!< For state machine. 0 - passive, 1 - active
volatile uint8_t pwm_duty;  //!< current duty value

void vent_init_ports(void)
{
 IOCFG_INIT(IOP_ECF, ECF_VAL(0)); //coolong fan is turned Off
}

void vent_init_state(void)
{
 pwm_state = 0;  //begin from active level
 pwm_duty = 0;   // 0%
 OCR2 = TIMER2_RELOAD_VALUE + COMPADD;
}

/**Sets duty value
 * \param duty value to be set
 */
void vent_set_duty(uint8_t duty)
{
 //TODO: Maybe we need double buffering?
 pwm_duty = duty;

 //We don't need interrupts if duty is 0 or 100%
 if (duty == 0)
 {
  _DISABLE_INTERRUPT();
  TIMSK&=~_BV(OCIE2);
  _ENABLE_INTERRUPT();
  COOLINGFAN_TURNOFF();
 }
 else if (duty == PWM_STEPS)
 {
  _DISABLE_INTERRUPT();
  TIMSK&=~_BV(OCIE2);
  _ENABLE_INTERRUPT();
  COOLINGFAN_TURNON();
 }
 else
 {
  _DISABLE_INTERRUPT();
  TIMSK|=_BV(OCIE2);
  _ENABLE_INTERRUPT();
 }
}

/**T/C 2 Compare interrupt for renerating of PWM (cooling fan control)*/
ISR(TIMER2_COMP_vect)
{
 if (0 == pwm_state)
 { //start active part
  COOLINGFAN_TURNON();
  OCR2+= pwm_duty;
  ++pwm_state;
 }
 else
 { //start passive part
  COOLINGFAN_TURNOFF();
  OCR2+= (PWM_STEPS - pwm_duty);
  --pwm_state;
 }
}

//Control of electric cooling fan (engine cooling), only in case if coolant temperature
//sensor is present in system
void vent_control(struct ecudata_t *d)
{
 //exit if coolant temperature sensor is disabled or there is no I/O assigned to
 //electric cooling fan
 if (!d->param.tmp_use || !IOCFG_CHECK(IOP_ECF))
  return;

#ifndef COOLINGFAN_PWM //control cooling fan by using relay only
 if (d->sens.temperat >= d->param.vent_on)
  COOLINGFAN_TURNON();
 if (d->sens.temperat <= d->param.vent_off)
  COOLINGFAN_TURNOFF();
#else //control cooling fan either by using relay or PWM
 if (!d->param.vent_pwm)
 { //relay
  if (d->sens.temperat >= d->param.vent_on)
   vent_set_duty(REL_DUTY(PWM_STEPS));
  if (d->sens.temperat <= d->param.vent_off)
   vent_set_duty(REL_DUTY(0));
 }
 else
 {
  //note: We skip 1 and 24 values of duty
  int16_t dd = d->param.vent_on - d->sens.temperat;
  if (dd < 2)
   dd = 0;         //restrict to max.
  if (dd > (PWM_STEPS-2))
   dd = PWM_STEPS; //restrict to min.
  //TODO: implement kick on turn on
  vent_set_duty(PWM_STEPS - dd);
 }
#endif
}
