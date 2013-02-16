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

/** \file secu3.h
 * Main module of the firmware.
 * This file contains main data structures used in the firmware.
 * (������� ������ ��������. ���� ���� �������� �������� ��������� ������ ������������ � ��������).
 */

#ifndef _SECU3_H_
#define _SECU3_H_

#include "tables.h"

#define SAVE_PARAM_TIMEOUT_VALUE      3000  //!< timeout value used to count time before automatic saving of parameters
#define FORCE_MEASURE_TIMEOUT_VALUE   20    //!< timeout value used to perform measurements when engine is stopped
#define CE_CONTROL_STATE_TIME_VALUE   50    //!< used for CE (flashing)
#define ENGINE_ROTATION_TIMEOUT_VALUE 20    //!< timeout value used to determine that engine is stopped (this value must not exceed 25)
#define IDLE_PERIOD_TIME_VALUE        50    //!< used by idling regulator

#ifdef DIAGNOSTICS
/**Describes diagnostics inputs data */
typedef struct diagnost_inp_t
{
 uint16_t voltage;                       //!< board voltage
 uint16_t map;                           //!< MAP sensor
 uint16_t temp;                          //!< coolant temperature
 uint16_t add_io1;                       //!< additional input 1 (analog)
 uint16_t add_io2;                       //!< additional input 2 (analog)
 uint16_t carb;                          //!< carburetor switch, throttle position sensor (analog)
 uint8_t  bits;                          //!< bits describing states of: gas valve, CKP sensor, VR type cam sensor, Hall-effect cam sensor, BL jmp, DE jmp
 uint16_t ks_1;                          //!< knock sensor 1
 uint16_t ks_2;                          //!< knock sensor 2
}diagnost_inp_t;
#endif

/**Describes all system's inputs - theirs derivative and integral magnitudes
 * ��������� ��� ����� ������� - �� ����������� � ������������ ��������
 */
typedef struct sensors_t
{
 uint16_t map;                           //!< Input Manifold Pressure (�������� �� �������� ���������� (�����������))
 uint16_t voltage;                       //!< Board voltage (���������� �������� ���� (�����������))
 int16_t  temperat;                      //!< Coolant temperature (����������� ����������� �������� (�����������))
 uint16_t frequen;                       //!< Averaged RPM (������� �������� ��������� (�����������))
 uint16_t inst_frq;                      //!< Instant RPM - not averaged  (���������� ������� ��������)
 uint8_t  carb;                          //!< State of carburetor's limit switch (��������� ��������� �����������)
 uint8_t  gas;                           //!< State of gas valve (��������� �������� �������)
 uint16_t frequen4;                      //!< RPM averaged by using only 4 samples (������� ����������� ����� �� 4-� ��������)
 uint16_t knock_k;                       //!< Knock signal level (������� ������� ���������)
#ifdef TPS_SENSOR
 uint16_t v_tps;                         //!< Board voltage (���������� ���� (�����������))
#endif
 
 uint8_t  tps;                           //!< Throttle position sensor (0...100%, x2)
 uint16_t add_i1;                        //!< ADD_I1 input voltage
 uint16_t add_i2;                        //!< ADD_I2 input voltage
 
 //����� �������� �������� (�������� ��� � ����������������� �������������)
 int16_t  map_raw;                       //!< raw ADC value from MAP sensor
 int16_t  voltage_raw;                   //!< raw ADC value from voltage
 int16_t  temperat_raw;                  //!< raw ADC value from coolant temperature sensor
#ifdef TPS_SENSOR
 int16_t  v_tps_raw;                     //!< raw ADC value from tps
#endif
 int16_t  tps_raw;                       //!< raw ADC value from TPS sensor
 int16_t  add_i1_raw;                    //!< raw ADC value from ADD_I1 input
 int16_t  add_i2_raw;                    //!< raw ADC value from ADD_I2 input
}sensors_t;

/**Describes system's data (main ECU data structure)
 * ��������� ������ �������, ������������ ������ ��������� ������
 */
typedef struct ecudata_t
{
 struct params_t  param;                 //!< --parameters (���������)
 struct sensors_t sens;                  //!< --sensors (�������)

 uint8_t  ie_valve;                      //!< State of Idle Economizer valve (��������� ������� ����)
 uint8_t  fe_valve;                      //!< State of Fuel Economizer valve (��������� ������� ���)
 uint8_t  cool_fan;                      //!< State of the cooling fan (��������� ������������������)
 uint8_t  st_block;                      //!< State of the starter blocking output (��������� ������ ���������� ��������)
 uint8_t  ce_state;                      //!< State of CE lamp (��������� ����� "CE")
 uint8_t  airflow;                       //!< Air flow (������ �������)
 int16_t  curr_angle;                    //!< Current advance angle (������� ���� ����������)
 int16_t  knock_retard;                  //!< Correction of advance angle from knock detector (�������� ��� �� ���������� �� ���������)


#ifndef REALTIME_TABLES
 f_data_t _PGM *fn_dat;                  //!< Pointer to the set of tables (��������� �� ����� �������������)
#else
 f_data_t tables_ram[2];                 //!< set of tables in RAM for petrol(0) and gas(1)
 f_data_t* fn_dat;                       //!< pointer to current set of tables in RAM
 uint8_t  fn_gas_prev;                   //!< previous index of tables set used for gas
 uint8_t  fn_gasoline_prev;              //!< previous index of tables set used for petrol
#endif

 uint16_t op_comp_code;                  //!< Contains code of operation for packet being sent - OP_COMP_NC (�������� ��� ������� ���������� ����� UART (����� OP_COMP_NC))
 uint16_t op_actn_code;                  //!< Contains code of operation for packet being received - OP_COMP_NC (�������� ��� ������� ����������� ����� UART (����� OP_COMP_NC))
 uint16_t ecuerrors_for_transfer;        //!< Buffering of error codes being sent via UART in real time (������������ ���� ������ ������������ ����� UART � �������� �������)
 uint16_t ecuerrors_saved_transfer;      //!< Buffering of error codes for read/write from/to EEPROM which is being sent/received (������������ ���� ������ ��� ������/������ � EEPROM, ������������/����������� ����� UART)
 uint8_t  use_knock_channel_prev;        //!< Previous state of knock channel's usage flag (���������� ��������� �������� ������������� ������ ���������)

 //TODO: To reduce memory usage it is possible to use crc or some simple hash algorithms to control state of memory(changes). So this variable becomes unneeded.
 uint8_t* eeprom_parameters_cache;       //!< Pointer to an array of EEPROM parameters cache (reduces redundant write operations)

 uint8_t engine_mode;                    //!< Current engine mode(start, idle, work) (������� ����� ��������� (����, ��, ��������))

#ifdef DIAGNOSTICS
 diagnost_inp_t diag_inp;                //!< diagnostic mode: values of inputs 
 uint16_t       diag_out;                //!< diagnostic mode: values of outputs
#endif

 uint8_t choke_testing;                  //!< Use to indcated that choke testing if on/off (so it is applicable only if SM_CONTROL compile option is used)
}ecudata_t;

extern struct ecudata_t edat;

#endif  //_SECU3_H_
