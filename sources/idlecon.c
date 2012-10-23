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

/** \file idlecon.c
 * Implementation of controling of Idle Econimizer (IE) valve.
 * (–еализаци€ управлени€ клапаном принудительного холостого хода(Ёѕ’’)).
 */

#include "port/avrio.h"
#include "port/port.h"
#include "bitmask.h"
#include "idlecon.h"
#include "ioconfig.h"
#include "secu3.h"
#include "vstimer.h"

/** Opens/closes IE valve (открывает/закрывает клапан Ёѕ’’) */
#define SET_IE_VALVE_STATE(s) IOCFG_SET(IOP_IE, s)

void idlecon_init_ports(void)
{
 IOCFG_INIT(IOP_IE, 0); //valve is turned on (клапан Ёѕ’’ включен)//у мен€ выключен так как использую только √аз
}

//Implementation of IE functionality. If throttle gate is closed AND frq > [up.threshold] OR
//throttle gate is closed AND frq > [lo.threshold] BUT valve is already closed, THEN turn off
//fuel supply by stopping to apply voltage to valve's coil. ELSE - fuel supply is turned on.

//реализаци€ функции Ёѕ’’. ≈сли заслонка карбюратора закрыта и frq > [верх.порог] или
//заслонка карбюратора закрыта и frq > [ниж.порог] но клапан уже закрыт, то производитс€
//выключение подачи топлива путем прекращени€ подачи напр€жени€ на обмотку эл.клапана. »наче - подача топлива.
void idlecon_control(struct ecudata_t* d)
{
 //if throttle gate is opened, then onen valve,reload timer and exit from condition
 //(если дроссель открыт, то открываем клапан, зар€жаем таймер и выходим из услови€).
 if ((d->sens.carb)&&(d->sens.gas))// только с газом
 {
  d->ie_valve = 1;
  s_timer_set(epxx_delay_time_counter, d->param.shutoff_delay);
 }
 //if throttle gate is closed, then state of valve depends on RPM,previous state of valve,timer and type of fuel
 //(если дроссель закрыт, то состо€ние клапана зависит от оборотов, предыдущего состо€ни€ клапана, таймера и вида топлива).
 else
  if (d->sens.gas) //gas (газовое топливо)
   d->ie_valve = ((s_timer_is_action(epxx_delay_time_counter))
   &&(((d->sens.frequen > d->param.ie_lot_g)&&(!d->ie_valve))||(d->sens.frequen > d->param.ie_hit_g)))?0:1;
  else //gasoline (бензин) не использую карбюратор , поэтому на бензине выключаю Ёѕ’’
   //d->ie_valve = ((s_timer_is_action(epxx_delay_time_counter))
   //&&(((d->sens.frequen > d->param.ie_lot)&&(!d->ie_valve))||(d->sens.frequen > d->param.ie_hit)))?0:1;
    d->ie_valve = 0;
 SET_IE_VALVE_STATE(d->ie_valve);
}
