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

/** \file adc.c
 * Implementation of ADC related functions (API).
 * Functions for read values from ADC, perform conversion to phisical values etc
 * (������� ��� ������ � ���, ���������� ��������, �������������� � ���������� �������� � �.�.).
 */

#include "port/avrio.h"
#include "port/interrupt.h"
#include "port/intrinsic.h"
#include "port/port.h"
#include "adc.h"
#include "bitmask.h"
#include "magnitude.h"
#include "secu3.h"
#include "funconv.h"

/**����� ������ ������������� ��� ��� */
#define ADCI_MAP                2
/**����� ������ ������������� ��� ���������� �������� ���� */
#define ADCI_UBAT               1
/**����� ������ ������������� ��� ���� */
#define ADCI_TEMP               0
/**����� ������ ������������� ��� ������ ��������� */
#define ADCI_KNOCK              3
/**��������, ������������ ��� ADCI_KNOCK ����� ������������ �������� */
#define ADCI_STUB               4

uint16_t user_var3;

/**C�������� ������ ��������� ��� */
typedef struct
{
 volatile uint16_t map_value;   //!< ��������� ���������� �������� ����������� ��������
 volatile uint16_t ubat_value;  //!< ��������� ���������� �������� ���������� �������� ����
 volatile uint16_t temp_value;  //!< ��������� ���������� �������� ����������� ����������� ��������
 volatile uint16_t knock_value; //!< ��������� ���������� �������� ������� ���������

 volatile uint8_t sensors_ready;//!< ������� ���������� � �������� ������ � ����������
 uint8_t  measure_all;          //!< ���� 1, �� ������������ ��������� ���� ��������
}adcstate_t;

/** ���������� ��������� ��� */
adcstate_t adc;

uint16_t adc_get_map_value(void)
{
 uint16_t value;
 _BEGIN_ATOMIC_BLOCK();
 value = adc.map_value;
 _END_ATOMIC_BLOCK();
 return value;
}

uint16_t adc_get_ubat_value(void)
{
 uint16_t value;
 _BEGIN_ATOMIC_BLOCK();
 value = adc.ubat_value;
 _END_ATOMIC_BLOCK();
 return value;
}

uint16_t adc_get_temp_value(void)
{
 uint16_t value;
 _BEGIN_ATOMIC_BLOCK();
 value = adc.temp_value;
 _END_ATOMIC_BLOCK();
 return value;
}

uint16_t adc_get_knock_value(void)
{
 uint16_t value;
 _BEGIN_ATOMIC_BLOCK();
 value = adc.knock_value;
 _END_ATOMIC_BLOCK();
 return value;
}

void adc_begin_measure(void)
{
 //�� �� ����� ��������� ����� ���������, ���� ��� �� �����������
 //���������� ���������
 if (!adc.sensors_ready)
  return;

 adc.sensors_ready = 0;
 ADMUX = ADCI_MAP|ADC_VREF_TYPE;
 SETBIT(ADCSRA,ADSC);
}

void adc_begin_measure_knock(void)
{
 //�� �� ����� ��������� ����� ���������, ���� ��� �� �����������
 //���������� ���������
 if (!adc.sensors_ready)
  return;

 adc.sensors_ready = 0;
 ADMUX = ADCI_STUB|ADC_VREF_TYPE;
 SETBIT(ADCSRA,ADSC);
}

void adc_begin_measure_all(void)
{
 adc.measure_all = 1;
 adc_begin_measure();
}

uint8_t adc_is_measure_ready(void)
{
 return adc.sensors_ready;
}

void adc_init(void)
{
 adc.knock_value = 0;
 adc.measure_all = 0;

 //������������� ���, ���������: f = 125.000 kHz,
 //���������� �������� �������� ���������� - 2.56V, ���������� ���������
 ADMUX=ADC_VREF_TYPE;
 ADCSRA=_BV(ADEN)|_BV(ADIE)|_BV(ADPS2)|_BV(ADPS1)|_BV(ADPS0);

 //������ ��� ����� � ������ ���������
 adc.sensors_ready = 1;

 //��������� ���������� - �� ��� �� �����
 ACSR=_BV(ACD);
}

/**���������� �� ���������� �������������� ���. ��������� �������� ���� ���������� ��������. ����� �������
 * ��������� ��� ���������� ����� ��������� ��� ������� �����, �� ��� ��� ���� ��� ����� �� ����� ����������.
 */
ISR(ADC_vect)
{
 _ENABLE_INTERRUPT();

 switch(ADMUX&0x07)
 {
  case ADCI_MAP: //��������� ��������� ����������� ��������
   adc.map_value = ADC;
   ADMUX = ADCI_UBAT|ADC_VREF_TYPE;
   SETBIT(ADCSRA,ADSC);
   break;

  case ADCI_UBAT://��������� ��������� ���������� �������� ����
   adc.ubat_value = ADC;
   ADMUX = ADCI_TEMP|ADC_VREF_TYPE;
   SETBIT(ADCSRA,ADSC);
   break;

  case ADCI_TEMP://��������� ��������� ����������� ����������� ��������
   adc.temp_value = ADC;
   if (0==adc.measure_all)
   {
    ADMUX = ADCI_MAP|ADC_VREF_TYPE;
    adc.sensors_ready = 1;
   }
   else
   {
    adc.measure_all = 0;
    ADMUX = ADCI_KNOCK|ADC_VREF_TYPE;
    SETBIT(ADCSRA,ADSC);
   }
   break;

  case ADCI_STUB: //��� �������� ��������� ���������� ������ ��� �������� ����� ���������� ������� ���������
   ADMUX = ADCI_KNOCK|ADC_VREF_TYPE;
   SETBIT(ADCSRA,ADSC);
   break;

  case ADCI_KNOCK://��������� ��������� ������� � ����������� ������ ���������
   adc.knock_value = ADC;
   adc.sensors_ready = 1;
   break;
 }
}

int16_t adc_compensate(int16_t adcvalue, int16_t factor, int32_t correction)
{
 return (((((int32_t)adcvalue*factor)+correction)<<2)>>16);
}

uint16_t map_adc_to_kpa(int16_t adcvalue, int16_t offset, int16_t gradient)
{
 int16_t t;
 //��� �� �������� ������������� ����������, ������ ������������� �������� ����� �������� ����� ����������� ������������.
 //����� ��� ������� ���������� �������������.
 if (adcvalue < 0)
  adcvalue = 0;

 //��������� �������� ���: ((adcvalue + K1) * K2 ) / 128, ��� K1,K2 - ���������.
 //���
 //��������� �������� ���: ((-5.0 + adcvalue + K1) * K2 ) / 128, ��� K1,K2 - ���������.
 if (gradient > 0)
 {
  t = adcvalue + offset;
  if (t < 0)
   t = 0;
 }
 else
 {
  t = -ROUND(5.0/ADC_DISCRETE) + adcvalue + offset;
  if (t > 0)
   t = 0;
 }
 return ( ((int32_t)t) * gradient ) >> 7;
}

uint16_t ubat_adc_to_v(int16_t adcvalue)
{
 if (adcvalue < 0)
  adcvalue = 0;
 return adcvalue;
}

#ifndef THERMISTOR_CS

int16_t temp_adc_to_c(int16_t adcvalue)
{
 if (adcvalue < 0)
  adcvalue = 0;
user_var3 = adcvalue;
 return (adcvalue - ((int16_t)((TSENS_ZERO_POINT / ADC_DISCRETE)+0.5)) );
}
#else
//�������� ��������� ������������ ������� �����������


#define THERMISTOR_LOOKUP_TABLE_SIZE 16

//������� �������� ����������� � ����� �� ����������
PGM_DECLARE(int16_t therm_cs_temperature[THERMISTOR_LOOKUP_TABLE_SIZE]) = 
{TEMPERATURE_MAGNITUDE(-27.9),TEMPERATURE_MAGNITUDE(-13.6),TEMPERATURE_MAGNITUDE(-3.7),TEMPERATURE_MAGNITUDE(2.4),
TEMPERATURE_MAGNITUDE(8.5),TEMPERATURE_MAGNITUDE(14.1),TEMPERATURE_MAGNITUDE(19.5),TEMPERATURE_MAGNITUDE(24.7),
TEMPERATURE_MAGNITUDE(30.0),TEMPERATURE_MAGNITUDE(35.6),TEMPERATURE_MAGNITUDE(41.4),TEMPERATURE_MAGNITUDE(47.8),
TEMPERATURE_MAGNITUDE(55.8),TEMPERATURE_MAGNITUDE(65.5),TEMPERATURE_MAGNITUDE(78.1),TEMPERATURE_MAGNITUDE(100.0)};

int16_t thermistor_lookup(uint16_t start, uint16_t step, uint16_t adcvalue)
{
int16_t i, i1;

if (adcvalue > start) 
  adcvalue = start;

i = ((start - adcvalue) / step);

if (i >= THERMISTOR_LOOKUP_TABLE_SIZE-1) i = i1 = THERMISTOR_LOOKUP_TABLE_SIZE-1;
  else i1 = i + 1;
user_var3 = adcvalue;
return (simple_interpolation(adcvalue, PGM_GET_WORD(&therm_cs_temperature[i1]), PGM_GET_WORD(&therm_cs_temperature[i]), 
        start-(i1 * step), step))/16;
}
#endif


