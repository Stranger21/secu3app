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

/** \file ufcodes.h
 * Codes of descriptors of packets used in communication via serial interface
 * (���� ������������ ������� ������������ ��� �������� ������ ����� ���������������� ���������).
 */

#ifndef _UFCODES_H_
#define _UFCODES_H_

#define   CHANGEMODE   'h'   //!< change mode (type of default packet)
#define   BOOTLOADER   'i'   //!< start boot loader

#define   TEMPER_PAR   'j'   //!< temperature parameters (coolant sensor, engine cooling etc)
#define   CARBUR_PAR   'k'   //!< carburetor's parameters
#define   IDLREG_PAR   'l'   //!< idling regulator parameters
#define   ANGLES_PAR   'm'   //!< advance angle (ign. timing) parameters
#define   FUNSET_PAR   'n'   //!< parametersrelated to set of functions (lookup tables)
#define   STARTR_PAR   'o'   //!< engine start parameters

#define   FNNAME_DAT   'p'   //!< used for transfering of names of set of functions (lookup tables)
#define   SENSOR_DAT   'q'   //!< used for transfering of sensors data

#define   ADCCOR_PAR   'r'   //!< parameters related to ADC corrections
#define   ADCRAW_DAT   's'   //!< used for transfering 'raw' values directly from ADC

#define   CKPS_PAR     't'   //!< CKP sensor parameters
#define   OP_COMP_NC   'u'   //!< used to indicate that specified (suspended) operation completed

#define   CE_ERR_CODES 'v'   //!< used for transfering of CE codes

#define   KNOCK_PAR    'w'   //!< parameters related to knock detection and knock chip

#define   CE_SAVED_ERR 'x'   //!< used for transfering of CE codes stored in the EEPROM
#define   FWINFO_DAT   'y'   //!< used for transfering information about firmware
#define   MISCEL_PAR   'z'   //!< miscellaneous parameters
#define   EDITAB_PAR   '{'   //!< used for transferring of data for realtime tables editing
#define   ATTTAB_PAR   '}'   //!< used for transferring of attenuator map (knock detection related)
#define   DBGVAR_DAT   ':'   //!< for watching of firmware variables (used for debug purposes)

#define   DIAGINP_DAT  '='   //!< diagnostics: send input values (analog & digital values)
#define   DIAGOUT_DAT  '^'   //!< diagnostics: receive output states (bits)

#define   CHOKE_PAR    '%'   //!< parameters  related to choke control

#endif //_UFCODES_H_
