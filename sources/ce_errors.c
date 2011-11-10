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

/** \file ce_errors.c
 * Implementation of controling of CE, errors detection and related functionality.
 * (���������� ���������� ������ CE, ����������� ������ � ��������������� ����������������).
 */

#include "port/avrio.h"
#include "port/port.h"
#include <string.h>
#include "adc.h"
#include "bitmask.h"
#include "camsens.h"
#include "ce_errors.h"
#include "ckps.h"
#include "eeprom.h"
#include "knock.h"
#include "magnitude.h"
#include "secu3.h"
#include "suspendop.h"
#include "vstimer.h"

/**CE state variables structure */
typedef struct
{
 uint16_t ecuerrors;         //!< 16 error codes maximum (�������� 16 ����� ������)
 uint16_t merged_errors;     //!< caching errors to preserve resource of the EEPROM (�������� ������ ��� ���������� ������� EEPROM)
 uint16_t write_errors;      //!< �. eeprom_start_wr_data() launches background process! (��������� ������� �������!)
}ce_state_t;

/**State variables */
ce_state_t ce_state = {0,0,0};

//operations under errors (�������� ��� ��������)
/*#pragma inline*/
void ce_set_error(uint8_t error)
{
 SETBIT(ce_state.ecuerrors, error);
}

/*#pragma inline*/
void ce_clear_error(uint8_t error)
{
 CLEARBIT(ce_state.ecuerrors, error);
}

/** Internal function. Contains checking logic */
void check(struct ecudata_t* d)
{
 //If error of CKP sensor was, then set corresponding bit of error
 //���� ���� ������ ���� �� ������������� ��� ��������������� ������
 if (ckps_is_error())
 {
  ce_set_error(ECUERROR_CKPS_MALFUNCTION);
  ckps_reset_error();
 }
 else
 {
  ce_clear_error(ECUERROR_CKPS_MALFUNCTION);
 }

#ifdef PHASE_SENSOR
 if (cams_is_error())
 {
  ce_set_error(ECUERROR_CAMS_MALFUNCTION);
  cams_reset_error();
 }
 else
 {
  ce_clear_error(ECUERROR_CAMS_MALFUNCTION);
 }
#endif

 //if error of knock channel was
 //���� ���� ������ ������ ���������
 if (d->param.knock_use_knock_channel)
 {
  if (knock_is_error())
  {
   ce_set_error(ECUERROR_KSP_CHIP_FAILED);
   knock_reset_error();
  }
  else
   ce_clear_error(ECUERROR_KSP_CHIP_FAILED);
 }
 else
  ce_clear_error(ECUERROR_KSP_CHIP_FAILED);

 //checking MAP sensor. TODO: implement additional check
 // error if voltage < 0.2v
 if (d->sens.map_raw < ROUND(0.2 / ADC_DISCRETE) && d->sens.carb)
  ce_set_error(ECUERROR_MAP_SENSOR_FAIL);
 else
  ce_clear_error(ECUERROR_MAP_SENSOR_FAIL);

 //checking coolant temperature sensor
 if (d->param.tmp_use)
 {
#ifndef THERMISTOR_CS 
   // error if (2.28v > voltage > 3.93v)
  if (d->sens.temperat_raw < ROUND(2.28 / ADC_DISCRETE) || d->sens.temperat_raw > ROUND(3.93 / ADC_DISCRETE))
#else
   // error if (0.2v > voltage > 4.7v) for thermistor
  if (d->sens.temperat_raw < ROUND(0.2 / ADC_DISCRETE) || d->sens.temperat_raw > ROUND(4.7 / ADC_DISCRETE))  
#endif   
    ce_set_error(ECUERROR_TEMP_SENSOR_FAIL);
  else
   ce_clear_error(ECUERROR_TEMP_SENSOR_FAIL);
 }
 else
  ce_clear_error(ECUERROR_TEMP_SENSOR_FAIL);

 //checking voltqage
 // error if voltage < 4.5v
 if ( (d->sens.voltage_raw < ROUND(4.5 / ADC_DISCRETE)) ||
  (d->sens.voltage_raw < ROUND(12.0 / ADC_DISCRETE) && d->sens.inst_frq > 2500) ||
  (d->sens.voltage_raw > ROUND(16.0 / ADC_DISCRETE)) )
  ce_set_error(ECUERROR_VOLT_SENSOR_FAIL);
 else
  ce_clear_error(ECUERROR_VOLT_SENSOR_FAIL);
}


//If any error occurs, the CE is light up for a fixed time. If the problem persists (eg corrupted the program code),
//then the CE will be turned on continuously. At the start of programm the CE lights up for 0.5 seconds. for indicating
//of the operability.
//��� ������������� ����� ������, �� ���������� �� ������������� �����. ���� ������ �� �������� (�������� �������� ��� ���������),
//�� CE ����� ������ ����������. ��� ������� ��������� �� ���������� �� 0.5 ���. ��� ������������� �����������������.
void ce_check_engine(struct ecudata_t* d, volatile s_timer8_t* ce_control_time_counter)
{
 uint16_t temp_errors;

 check(d);

 //If the timer counted the time, then turn off the CE
 //���� ������ �������� �����, �� ����� ��
 if (s_timer_is_action(*ce_control_time_counter))
 {
  ce_set_state(0);
  d->ce_state = 0; //<--doubling
 }

 //If at least one error is present  - turn on CE and start timer
 //���� ���� ���� �� ���� ������ - �������� �� � ��������� ������
 if (ce_state.ecuerrors!=0)
 {
  s_timer_set(*ce_control_time_counter, CE_CONTROL_STATE_TIME_VALUE);
  ce_set_state(1);
  d->ce_state = 1;  //<--doubling
 }

 temp_errors = (ce_state.merged_errors | ce_state.ecuerrors);
 //check for error which is still not in merged_errors
 //��������� �� ������ ������� ��� � merged_errors?
 if (temp_errors!=ce_state.merged_errors)
 {
  //Because at the time of emergence of a new error, EEPROM can be busy (for example, saving options),
  //then it is necessary to run deffered operation, which will be automatically executed as soon as the EEPROM
  //will be released.
  //��� ��� �� ������ ������������� ����� ������ EEPROM ����� ���� ������ (�������� ����������� ����������),
  //�� ���������� ��������� ���������� ��������, ������� ����� ������������� ��������� ��� ������ EEPROM
  //�����������.
  sop_set_operation(SOP_SAVE_CE_MERGED_ERRORS);
 }

 ce_state.merged_errors = temp_errors;

 //copy error's bits into the cache for transferring
 //��������� ���� ������ � ��� ��� ��������.
 d->ecuerrors_for_transfer|= ce_state.ecuerrors;
}

void ce_save_merged_errors(uint16_t* p_merged_errors)
{
 uint16_t temp_errors;

 if (!p_merged_errors) //overwrite with parameter?
 {
  eeprom_read(&temp_errors, EEPROM_ECUERRORS_START, sizeof(uint16_t));
  ce_state.write_errors = temp_errors | ce_state.merged_errors;
  if (ce_state.write_errors!=temp_errors)
    eeprom_start_wr_data(0, EEPROM_ECUERRORS_START, &ce_state.write_errors, sizeof(uint16_t));
 }
 else
 {
  ce_state.merged_errors = *p_merged_errors;
  eeprom_start_wr_data(OPCODE_CE_SAVE_ERRORS, EEPROM_ECUERRORS_START, (uint8_t*)&ce_state.merged_errors, sizeof(uint16_t));
 }
}

void ce_clear_errors(void)
{
 memset(&ce_state, 0, sizeof(ce_state_t));
 eeprom_write((uint8_t*)&ce_state.write_errors, EEPROM_ECUERRORS_START, sizeof(uint16_t));
}

void ce_init_ports(void)
{
 PORTB|= _BV(PB2);  //CE is ON (for checking)
 DDRB |= _BV(DDB2); //output for CE
}
