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

/** \file fuelpump.c
 * Implementation of controling of electric fuel pump.
 * (���������� ���������� �������������������).
 */

#ifdef IDL_REGUL

#include "port/avrio.h"
#include "port/port.h"
#include "bitmask.h"
#include "idlregul.h"
#include "secu3.h"
#include "vstimer.h"
#include "adc.h"
#include "magnitude.h"
#include <stdint.h>

/** Turn on/turn off PIN IDL_REGUL */
#define OUT_IDL_REGUL1(s) {PORTC_Bit0 = s;}
#define OUT_IDL_REGUL2(s) {PORTC_Bit1 = s;}

// ��������� ������ ���������� ��� �� ������
uint8_t idl_start_en;
//����� ��� �������� ���������� �� ����
uint16_t tps_v_buf;

//������������� ������ ��� ���
void idlregul_init_ports(void)
{

   PORTC|= _BV(PC1)|_BV(PC0);
   DDRC|= _BV(DDC1)|_BV(DDC0);
   idl_start_en = 1;
}

// ��������� ���������� ��� ������� ����� 1 - ��������� , 2 ��������� , 3 ����
void idlregul_set_state(uint8_t i_state, struct ecudata_t* d)
{
// ���� ���������� �� ���� ������ ������������ - ���� ��� 
#ifdef TPS_SENSOR  
  if ((d->sens.v_tps <= V_TPS_MAGNITUDE(TPS_V_MIN))||(d->sens.v_tps >= V_TPS_MAGNITUDE(TPS_V_MAX)))
    i_state =3;
#else
    i_state = 3;
#endif    
  if (i_state == 1) 
  {
  OUT_IDL_REGUL1(0);
  OUT_IDL_REGUL2(0);
  }
  if (i_state == 2) 
  {
  OUT_IDL_REGUL1(0);
  OUT_IDL_REGUL2(1);
  }
  if (i_state == 3) 
  {
  OUT_IDL_REGUL1(1);
  OUT_IDL_REGUL2(0);
  }  
}

//��������� �������� ��� � ��������� ������� ��� ������� � ������� 1800 ��������
void idlregul_control(struct ecudata_t* d)
{
  // ���� ����� �������� , � ���������� �� ���� ������ ������� �� �������� ���
 // ����� ���� ��� � ������ ������
#ifdef TPS_SENSOR 
  if (d->sens.v_tps < V_TPS_MAGNITUDE(TPS_V_START))
 
   idlregul_set_state(1,d); 
 
 else
 {
   idlregul_set_state(3,d);
   idl_start_en = 0;
   
 }
#endif  
}

//��������� ���������� ������ �������� ���
void idlregul_dempfer(struct ecudata_t* d, volatile s_timer8_t* io_timer)
{
 
  
}

#endif //IDL_REGUL
