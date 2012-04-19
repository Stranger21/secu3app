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

/** \file ce_errors.h
 * Control of CE, errors detection and related functionality.
 * (���������� ������ CE, ����������� ������ � ��������������� ����������������).
 */

#ifndef _CE_ERRORS_H_
#define _CE_ERRORS_H_

#include <stdint.h>
#include "vstimer.h"

//define bits (numbers of bits) of errors (Check Engine)
//(���������� ���� (������ �����) ������ (Check Engine)).
#define ECUERROR_CKPS_MALFUNCTION       0  //!< CKP sensor malfunction
#define ECUERROR_EEPROM_PARAM_BROKEN    1  //!< Parameters in EEPROM are broken. CE turns off after a few seconds after the engine starts (�E �������� ����� ��������� ������ ����� ������� ���������)
#define ECUERROR_PROGRAM_CODE_BROKEN    2  //!< FLASH is broken. CE turns off after a few seconds after the engine starts (�� �������� ����� ��������� ������ ����� ������� ���������)
#define ECUERROR_KSP_CHIP_FAILED        3  //!< Knock detection chip does not work properly
#define ECUERROR_KNOCK_DETECTED         4  //!< Knock was detected (one or many times)
#define ECUERROR_MAP_SENSOR_FAIL        5  //!< MAP sensor does not work
#define ECUERROR_TEMP_SENSOR_FAIL       6  //!< Coolant temperature sensor does not work
#define ECUERROR_VOLT_SENSOR_FAIL       7  //!< Voltage is wrong or sensing is disconnected
#define ECUERROR_DWELL_CONTROL          8  //!< Problems with dwell control (overcharge etc)
#define ECUERROR_CAMS_MALFUNCTION       9  //!< CAM sensor malfunction

struct ecudata_t;

/**checks for errors and manages the CE lamp
 * (���������� �������� ������� ������ � ��������� ������ CE).
 * \param d pointer to ECU data structure
 * \param ce_control_time_counter time counter object
 */
void ce_check_engine(struct ecudata_t* d, volatile s_timer8_t* ce_control_time_counter);

/**Set specified error (number of bit)
 * (��������� ��������� ������ (����� ����)).
 * \param error code of error
 */
void ce_set_error(uint8_t error);

/**Reset specified error (number of bit)
 * (C���� ��������� ������ (����� ����)).
 * \param error code of error
 */
void ce_clear_error(uint8_t error);

/**Performs preservation of all stockpiled in temporary memory errors in the EEPROM.
 * Call only if EEPROM is ready!
 * (���������� ���������� ���� ����������� �� ��������� ������ ������ � EEPROM.
 * �������� ������ ���� EEPROM ������!).
 * \param p_merged_errors merged errors's bits to save
 */
void ce_save_merged_errors(uint16_t* p_merged_errors);

/**Clears errors saved in EEPROM (������� ������ ����������� � EEPROM). */
void ce_clear_errors(void);

/**Initialization of used I/O ports (������������� ������������ ������ �����/������). */
void ce_init_ports(void);

#ifdef SECU3T  /*SECU-3T*/
 #define CE_STATE_ON  0
 #define CE_STATE_OFF 1
#else          /*SECU-3*/
 #define CE_STATE_ON  1
 #define CE_STATE_OFF 0
#endif

/**Turns on/off CE lamp (��������/��������� ����� Check Engine).*/
#define ce_set_state(s) {PORTB_Bit2 = s;}

/**Retrieves state of CE lamp (�������� ��������� ����� CE). */
#define ce_get_state() (PINB_Bit2)

#endif //_CE_ERRORS_H_
