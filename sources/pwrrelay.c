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

/** \file pwrrelay.c
 * Power management using external relay, allows SECU-3 to be turned on some time
 * after ignition is off. So, for instance electric colling fan can work even ignition is off
 * (���������� ���������� �������� ��������� ������� ����).
 */

#include "port/port.h"
#include "adc.h"
#include "bitmask.h"
#include "eeprom.h"
#include "ioconfig.h"
#include "magnitude.h"
#include "secu3.h"

void pwrrelay_init_ports(void)
{
 IOCFG_INIT(IOP_PWRRELAY, 1); //power relay is turned on (���� ��������)
}

void pwrrelay_control(struct ecudata_t* d)
{
 //if this feature is disabled, then do nothing
 if (!IOCFG_CHECK(IOP_PWRRELAY))
  return;

 if (d->sens.voltage < VOLTAGE_MAGNITUDE(4.5))
 {//ignition is off

  //We will wait while temperature is high only if temperature sensor is enabled
  //and control of electric cooling fan is used.
  uint8_t temperature_ok = 1;
  if (d->param.tmp_use && IOCFG_CHECK(IOP_ECF))
  {
#ifdef COOLINGFAN_PWM
   if (d->param.vent_pwm) //PWM is available and enabled
    temperature_ok = (d->sens.temperat <= (d->param.vent_on - TEMPERATURE_MAGNITUDE(2.0)));
   else //PWM is available, but disabled
    temperature_ok = (d->sens.temperat <= (d->param.vent_off));
#else
   //PWM is not available
   temperature_ok = (d->sens.temperat <= (d->param.vent_off));
#endif
  }

  if (temperature_ok && eeprom_is_idle())
   IOCFG_SET(IOP_PWRRELAY, 0); //turn off relay
 }

 //TODO: eliminate CKPS error
 //TODO: eliminate voltage error
}
