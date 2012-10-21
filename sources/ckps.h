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

/** \file ckps.h
 * Processing of crankshaft position sensor.
 * (��������� ������� ��������� ���������).
 */

#ifndef _CKPS_H_
#define _CKPS_H_

#include <stdint.h>

/**Scaling factor of crankshaft rotation angle, appears in the calculations and operations of the division
 * so it should be a multiple of degree of 2 (����������� ��������������� ����� �������� ���������, ����������
 * � ����������� � ��������� ������� ������� �� ������ ���� ������ ������� 2).
 */
#define ANGLE_MULTIPLAYER   32

/**Initialization of CKP module (hardware & variables)
 * (������������� ��������� ������/��������� ���� � ������ �� ������� �� �������)
 */
void ckps_init_state(void);

/** Set edge type for CKP sensor (���������� ��� ������ ����)
 * \param edge_type 0 - falling (�������������), 1 - rising (�������������)
 */
void ckps_set_edge_type(uint8_t edge_type);

/** Set count of teeth before TDC.
 * \param cogs_btdc E.g values for 60-2 wheel: 17,18,19,20,21,22,23 
                               for 36-1 wheel: 8,9,10,11,12,13,14
 * \details For 4-cylinder engine. If the crankshaft is in a position corresponding to top dead center (tdc) of the first cylinder's piston,
 * then according to the standards the middle of 20th tooth of sync wheel must be in the opposite to the CKP's core
 * (counting out against the direction of rotation from the place of the cutout).
 * (��� 4-� ������������ ���������. ���� ���������� ��� ���������� � ���������, ��������������� ������� ������� �����(t.d.c.) ������ ������� ��������, ��
 * �� ��������� �������� �������� ���������� ���� ������ ���������� 20-� ��� ����� ������������� (������� ������
 * ����������� �������� �� ����� ������)).
 */
void ckps_set_cogs_btdc(uint8_t cogs_btdc);


#ifndef DWELL_CONTROL
/** Set duration of ignition pulse drive (������������� ������������ �������� ��������� � ������)
 * \param cogs duration of pulse, countable in the teeth of wheel
 * \details For standard igniters duration of driving pulses must be 1/3, when significant deviation to the smaller side
 * is present then igniters can became out of action. If you connect two outputs together to one igniter, you must put
 * a value of 10, if double channel mode then 40. The values given for the 60-2 wheel and 4 cylinder engine.
 * (��� ����������� ������������ ������������ �������� ������� ������ ���� 1/3, ��� ������������ ���������� � ������� �������
 * �������� ����� ����������� �� �����.  ���� ��������� ��� ������ ������ ��� ������ �����������, �� ���������� �������
 * �������� 10, ���� ������������� ����� �� 40. �������� ������� ��� ����� 60-2 � 4-� ������������ ���������).
 */
void ckps_set_ignition_cogs(uint8_t cogs);
#else
/**Dwell control. Set accumulation time
 * \param i_acc_time accumulation time in timer's ticks (1 tick = 4uS)
 */
void ckps_set_acc_time(uint16_t i_acc_time);
#endif

/** Set andvance angle
 * (������������� ��� ��� ���������� � ���������)
 * \param angle advance angle * ANGLE_MULTIPLAYER
 */
void ckps_set_advance_angle(int16_t angle);

/** Calculate instant RPM using last measured period
 * (������������ ���������� ������� �������� ��������� ����������� �� ��������� ���������� �������� �������)
 * \return RPM (min-1)
 */
uint16_t ckps_calculate_instant_freq(void);

/** Set pahse selection window for detonation (��������� ���� ������� �������� ���������)
 * \param begin begin of window (degrees relatively to t.d.c) (������ ���� � �������� ������������ �.�.�)
 * \param end end of window (degrees relatively to t.d.c) (����� ���� � �������� ������������ �.�.�)
 */
void ckps_set_knock_window(int16_t begin, int16_t end);

/** Set to use or not to use knock detection (������������� ����������� ��� �� ����������� ����� ���������)
 * \param use_knock_channel 1 - use, 0 - do not use
 */
void ckps_use_knock_channel(uint8_t use_knock_channel);

/** \return nonzero if error was detected */
uint8_t ckps_is_error(void);

/** Reset detected errors */
void ckps_reset_error(void);

/**\return 1 if there was engine stroke and reset flag!
 * (��� ������� ���������� 1 ���� ��� ����� ���� ��������� � ����� ���������� �������!)
 * \details Used to perform synchronization with rotation of crankshaft.
 */
uint8_t ckps_is_stroke_event_r(void);

/** Initialization of state variables */
void ckps_init_state_variables(void);

/** \return Number of current tooth (���������� ����� �������� ����) */
uint8_t ckps_get_current_cog(void);

/** \return returns 1 if number of current tooth has been changed
 *  (���������� 1, ���� ����� �������� ���� ���������)
 */
uint8_t ckps_is_cog_changed(void);

/** Set number of engine's cylinders (��������� ���-�� ��������� ��������� (������ �����))
 * \param i_cyl_number allowed values(���������� ��������): *1,2,*3,4,*5,6,8
 * * these values are allowed only if firmware compliled with PHASED_IGNITION option
 */
void ckps_set_cyl_number(uint8_t i_cyl_number);

/** Initialization of used I/O ports (���������� ������������� ����� ������) */
void ckps_init_ports(void);

/** Enable/disable ignition 
 * \param i_cutoff 1 - enable ignition, 0 - disable ignition
 */
void ckps_enable_ignition(uint8_t i_cutoff);

/** Enable/disbale merging of ignition outputs
 * \param i_merge 1 - merge, 0 - normal mode
 */
void ckps_set_merge_outs(uint8_t i_merge);

#ifdef HALL_OUTPUT
/** Set parameters for Hall output pulse
 * \param i_offset - offset in tooth relatively to TDC (if > 0, then BTDC)
 * \param i_duration - duration of pulse in tooth
 */
void ckps_set_hall_pulse(int8_t i_offset, uint8_t i_duration);
#endif

/** Set number of cranck wheel's teeth
 * \param norm_num Number of cranck wheel's teeth, including missing teeth (16...200)
 * \param miss_num Number of missing cranck wheel's teeth (0, 1, 2)
 */
void ckps_set_cogs_num(uint8_t norm_num, uint8_t miss_num);

#endif //_CKPS_H_
