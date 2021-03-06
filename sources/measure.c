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

/** \file measure.c
 * Implementation pf processing (averaging, corrections etc) of data comes from ADC and sensors
 * (���������� ��������� (����������, ������������� � �.�.) ������ ����������� �� ��� � ��������).
 */

#include "port/avrio.h"
#include "port/interrupt.h"
#include "port/intrinsic.h"
#include "port/port.h"
#include "adc.h"
#include "bitmask.h"
#include "funconv.h"    //thermistor_lookup()
#include "ioconfig.h"
#include "magnitude.h"
#include "measure.h"
#include "secu3.h"

/**Reads state of gas valve (��������� ��������� �������� �������) */
#define GET_GAS_VALVE_STATE(s) (CHECKBIT(PINC, PINC6) > 0)

/**Reads state of throttle gate (only the value, without inversion)
 * ��������� ��������� ����������� �������� (������ ��������, ��� ��������)
 */
#ifdef SECU3T  /*SECU-3T*/
 #define GET_THROTTLE_GATE_STATE() (CHECKBIT(PINA, PINA7) > 0)
#else          /*SECU-3*/
 #define GET_THROTTLE_GATE_STATE() (CHECKBIT(PINC, PINC5) > 0)
#endif

/**Number of values for averaging of RPM for tachometer
 * ���-�� �������� ��� ���������� ������� �������� �.�. ��� �������� ��������� */
#define FRQ_AVERAGING           16

/**Number of values for avaraging of RPM for starter blocking
 * ���-�� �������� ��� ���������� ������� �������� �.�. ��� ���������� ��������� */
#define FRQ4_AVERAGING          4

//������ ������� ���������� �� ������� ����������� �������
#define MAP_AVERAGING           4                 //!< Number of values for averaging of pressure (MAP)
#define BAT_AVERAGING           4                 //!< Number of values for averaging of board voltage
#define TMP_AVERAGING           8                 //!< Number of values for averaging of coolant temperature
#ifdef TPS_SENSOR
#define TPS_AVERAGING           4                 //!< Number of values for averaging of TPS
#endif
#ifdef SECU3T
#define TPS_AVERAGING           4                 //!< Number of values for averaging of throttle position
#define AI1_AVERAGING           4                 //!< Number of values for averaging of ADD_IO1
#define AI2_AVERAGING           4                 //!< Number of values for averaging of ADD_IO2
#endif

uint16_t freq_circular_buffer[FRQ_AVERAGING];     //!< Ring buffer for RPM averaging for tachometer (����� ���������� ������� �������� ��������� ��� ���������)
uint16_t freq4_circular_buffer[FRQ4_AVERAGING];   //!< Ring buffer for RPM averaging for starter blocking (����� ���������� ������� �������� ��������� ��� ���������� ��������)
uint16_t map_circular_buffer[MAP_AVERAGING];      //!< Ring buffer for averaring of MAP sensor (����� ���������� ����������� ��������)
uint16_t ubat_circular_buffer[BAT_AVERAGING];     //!< Ring buffer for averaring of voltage (����� ���������� ���������� �������� ����)
uint16_t temp_circular_buffer[TMP_AVERAGING];     //!< Ring buffer for averaring of coolant temperature (����� ���������� ����������� ����������� ��������)
#ifdef TPS_SENSOR
uint16_t tps_circular_buffer[TPS_AVERAGING];     //!< Ring buffer for averaring of tps (����� ���������� ���������� �� ����)
#endif
#ifdef SECU3T
uint16_t tps_circular_buffer[TPS_AVERAGING];      //!< Ring buffer for averaring of TPS
uint16_t ai1_circular_buffer[AI1_AVERAGING];      //!< Ring buffer for averaring of ADD_IO1
uint16_t ai2_circular_buffer[AI2_AVERAGING];      //!< Ring buffer for averaring of ADD_IO2
#endif

//���������� ������� ���������� (������� ��������, �������...)
void meas_update_values_buffers(struct ecudata_t* d, uint8_t rpm_only)
{
 static uint8_t  map_ai  = MAP_AVERAGING-1;
 static uint8_t  bat_ai  = BAT_AVERAGING-1;
 static uint8_t  tmp_ai  = TMP_AVERAGING-1;
 static uint8_t  frq_ai  = FRQ_AVERAGING-1;
 static uint8_t  frq4_ai = FRQ4_AVERAGING-1;
#ifdef TPS_SENSOR
 static uint8_t  tps_ai  = TPS_AVERAGING-1;
#endif
 
#ifdef SECU3T
 static uint8_t  tps_ai  = TPS_AVERAGING-1;
 static uint8_t  ai1_ai  = AI1_AVERAGING-1;
 static uint8_t  ai2_ai  = AI2_AVERAGING-1;
#endif

 freq_circular_buffer[frq_ai] = d->sens.inst_frq;
 (frq_ai==0) ? (frq_ai = FRQ_AVERAGING - 1): frq_ai--;

 freq4_circular_buffer[frq4_ai] = d->sens.inst_frq;
 (frq4_ai==0) ? (frq4_ai = FRQ4_AVERAGING - 1): frq4_ai--;

 if (rpm_only)
  return;

 map_circular_buffer[map_ai] = adc_get_map_value();
 (map_ai==0) ? (map_ai = MAP_AVERAGING - 1): map_ai--;

 ubat_circular_buffer[bat_ai] = adc_get_ubat_value();
 (bat_ai==0) ? (bat_ai = BAT_AVERAGING - 1): bat_ai--;

 temp_circular_buffer[tmp_ai] = adc_get_temp_value();
 (tmp_ai==0) ? (tmp_ai = TMP_AVERAGING - 1): tmp_ai--;

#ifdef TPS_SENSOR 
 tps_circular_buffer[tps_ai] = adc_get_tps_value();
 (tps_ai==0) ? (tps_ai = TPS_AVERAGING - 1): tps_ai--; 
#endif 
#ifdef SECU3T
 tps_circular_buffer[tps_ai] = adc_get_carb_value();
 (tps_ai==0) ? (tps_ai = TPS_AVERAGING - 1): tps_ai--;

 ai1_circular_buffer[ai1_ai] = adc_get_add_io1_value();
 (ai1_ai==0) ? (ai1_ai = AI1_AVERAGING - 1): ai1_ai--;

 ai2_circular_buffer[ai2_ai] = adc_get_add_io2_value();
 (ai2_ai==0) ? (ai2_ai = AI2_AVERAGING - 1): ai2_ai--;
#endif

 d->sens.knock_k = adc_get_knock_value() * 2;
}

//���������� ���������� ������� ��������� ������� �������� ��������� ������� ����������, �����������
//������������ ���, ������� ���������� �������� � ���������� ��������.
void meas_average_measured_values(struct ecudata_t* d)
{
 uint8_t i;  uint32_t sum;

 for (sum=0,i = 0; i < MAP_AVERAGING; i++)  //��������� �������� � ������� ����������� ��������
  sum+=map_circular_buffer[i];
 d->sens.map_raw = adc_compensate((sum/MAP_AVERAGING)*2,d->param.map_adc_factor,d->param.map_adc_correction);
 d->sens.map = map_adc_to_kpa(d->sens.map_raw, d->param.map_curve_offset, d->param.map_curve_gradient);

 for (sum=0,i = 0; i < BAT_AVERAGING; i++)   //��������� ���������� �������� ����
  sum+=ubat_circular_buffer[i];
 d->sens.voltage_raw = adc_compensate((sum/BAT_AVERAGING)*6,d->param.ubat_adc_factor,d->param.ubat_adc_correction);
 d->sens.voltage = ubat_adc_to_v(d->sens.voltage_raw);

#ifdef TPS_SENSOR 
 for (sum=0,i = 0; i < TPS_AVERAGING; i++)   //��������� ���������� ����
  sum+=tps_circular_buffer[i];
 d->sens.v_tps_raw = adc_compensate((5*(sum/TPS_AVERAGING))/3,d->param.ubat_adc_factor,d->param.ubat_adc_correction);
 d->sens.v_tps = tps_adc_to_v(d->sens.v_tps_raw);
 user_var3 = d->sens.v_tps;
#endif
 
 if (d->param.tmp_use)
 {
  for (sum=0,i = 0; i < TMP_AVERAGING; i++) //��������� ����������� (����)
   sum+=temp_circular_buffer[i];
  d->sens.temperat_raw = adc_compensate((5*(sum/TMP_AVERAGING))/3,d->param.temp_adc_factor,d->param.temp_adc_correction);
#ifndef THERMISTOR_CS
  d->sens.temperat = temp_adc_to_c(d->sens.temperat_raw);
#else
  if (!d->param.cts_use_map) //use linear sensor
   d->sens.temperat = temp_adc_to_c(d->sens.temperat_raw);
  else //use lookup table (actual for thermistor sensors)
   d->sens.temperat = thermistor_lookup(d->sens.temperat_raw);
#endif
 }
 else                                       //���� �� ������������
  d->sens.temperat = 0;

 for (sum=0,i = 0; i < FRQ_AVERAGING; i++)  //��������� ������� �������� ���������
  sum+=freq_circular_buffer[i];
 d->sens.frequen=(sum/FRQ_AVERAGING);

 for (sum=0,i = 0; i < FRQ4_AVERAGING; i++) //��������� ������� �������� ���������
  sum+=freq4_circular_buffer[i];
 d->sens.frequen4=(sum/FRQ4_AVERAGING);

#ifdef SECU3T
 for (sum=0,i = 0; i < TPS_AVERAGING; i++)   //average throttle position
  sum+=tps_circular_buffer[i];
 d->sens.tps_raw = adc_compensate((sum/TPS_AVERAGING)*2,d->param.tps_adc_factor,d->param.tps_adc_correction);
 d->sens.tps = tps_adc_to_pc(d->sens.tps_raw, d->param.tps_curve_offset, d->param.tps_curve_gradient);
 if (d->sens.tps > TPS_MAGNITUDE(100))
  d->sens.tps = TPS_MAGNITUDE(100);

 for (sum=0,i = 0; i < AI1_AVERAGING; i++)   //average ADD_IO1 input
  sum+=ai1_circular_buffer[i];
 d->sens.add_i1_raw = adc_compensate((sum/AI1_AVERAGING)*2,d->param.ai1_adc_factor,d->param.ai1_adc_correction);
 d->sens.add_i1 = d->sens.add_i1_raw;

 for (sum=0,i = 0; i < AI2_AVERAGING; i++)   //average ADD_IO2 input
  sum+=ai2_circular_buffer[i];
 d->sens.add_i2_raw = adc_compensate((sum/AI2_AVERAGING)*2,d->param.ai2_adc_factor,d->param.ai2_adc_correction);
 d->sens.add_i2 = d->sens.add_i2_raw;
#endif
}

//�������� ��� ���������������� ��������� ����� ������ ���������. �������� ������ �����
//������������� ���.
void meas_initial_measure(struct ecudata_t* d)
{
 uint8_t _t,i = 16;
 _t = _SAVE_INTERRUPT();
 _ENABLE_INTERRUPT();
 do
 {
  adc_begin_measure(0); //<--normal speed
  while(!adc_is_measure_ready());

  meas_update_values_buffers(d, 0); //<-- all
 }while(--i);
 _RESTORE_INTERRUPT(_t);
 meas_average_measured_values(d);
}

void meas_take_discrete_inputs(struct ecudata_t *d)
{
 //--�������� ��������� ����������� ���� ����������
 if (0==d->param.tps_threshold)
  d->sens.carb=d->param.carb_invers^GET_THROTTLE_GATE_STATE(); //���������: 0 - �������� ������, 1 - ������
 else
 {//using TPS (������������� � ����)
  d->sens.carb=d->param.carb_invers^(d->sens.tps > d->param.tps_threshold);
 }

//��������� � ��������� ��������� �������� �������
uint8_t sensgas = GET_GAS_VALVE_STATE();

//��������� ����������� ������������ �� ��� � �������������� ������ , 
//���� �<10 �� ������
// ���� ������ �� ��������� ������� ��������� , ���� ���� 1800 �� 
//�������� ������� ������� , ����������� �������, ���� ��������� ������ , �� ��������� ������������ �� ������

//�������� ��������� ���������
if (!s_timer_is_action(engine_rotation_timeout_counter))
{
//�������� ������������ �� ������ �����������  
if (d->param.tmp_use)
{
  if ((sensgas)&&(d->sens.temperat >= TEMPERATURE_MAGNITUDE(10))&&(d->sens.frequen >= 1800))
{
  d->sens.gas = 1;
  fe_valve_state(1);
}
else
  if (!sensgas)
  {
  d->sens.gas = 0;
  fe_valve_state(0);
  }
}
else
   {
//���� �� ������������ ������ ����������� �� ������������ ������ ��������� ����� ������� ������ � �������     
 if ((sensgas)&&(d->sens.frequen >= 1800))
{
  d->sens.gas = 1;
  fe_valve_state(1);
}
else
  if (!sensgas)
  {
  d->sens.gas = 0;
  fe_valve_state(0);
  }
  }
}//���� ��������� ������������
else
{
  d->sens.gas = 0;
  fe_valve_state(0); 
}
 //����������� ��� ������� � ����������� �� ��������� �������� �������
#ifndef REALTIME_TABLES
 if (!IOCFG_CHECK(IOP_MAPSEL0))
 { //without additioanl selection input
  if (d->sens.gas)
   d->fn_dat = &fw_data.tables[d->param.fn_gas];     //�� ����
  else
   d->fn_dat = &fw_data.tables[d->param.fn_gasoline];//�� �������
 }
 else
 {
  uint8_t mapsel0 = IOCFG_GET(IOP_MAPSEL0);
  if (d->sens.gas) //�� ����
   d->fn_dat = mapsel0 ? &fw_data.tables[1] : &fw_data.tables[d->param.fn_gas];
  else             //�� �������
   d->fn_dat = mapsel0 ? &fw_data.tables[0] : &fw_data.tables[d->param.fn_gasoline];
 }
#else //use tables from RAM
 if (d->sens.gas)
  d->fn_dat = &d->tables_ram[1]; //using gas(�� ����)
 else
  d->fn_dat = &d->tables_ram[0]; //using petrol(�� �������)
#endif
}

