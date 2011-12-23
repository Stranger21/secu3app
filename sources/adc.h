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

/** \file adc.h
 * ADC related functions (API).
 * Functions for read values from ADC, perform conversion to phisical values etc
 * (������� ��� ������ � ���, ���������� ��������, �������������� � ���������� �������� � �.�.).
 */

#ifndef _ADC_H_
#define _ADC_H_

#include <stdint.h>

extern uint16_t user_var3;

/**���� �������� ��� � ������� */
#define ADC_DISCRETE            0.0025

/**������ ������ ������� ����������� �����/������ */
#define TSENS_SLOPP             0.01

#ifndef THERMISTOR_CS
/**���������� �� ������ ������� ����������� ��� 0 �������� ������� */
 #define TSENS_ZERO_POINT        2.73
#else
 /**Voltage which corresponds to minimum temperature */
 #define TSENS_V_TMIN            4.37
 /**Voltage which is step between interpolation nodes in table */
 #define TSENS_STEP              0.27
#endif

/**��������� ��� ������ ��������� �������� ���������� */
#define ADC_VREF_TYPE           0xC0

/**������������ ���������� �������� - ��� */
#define MAP_PHYSICAL_MAGNITUDE_MULTIPLAYER  64

/**������������ ���������� �������� - ���������� */
#define UBAT_PHYSICAL_MAGNITUDE_MULTIPLAYER (1.0/ADC_DISCRETE) //=400

#ifdef TPS_SENSOR
/**������������ ���������� �������� - ���������� ���� */
#define TPS_PHYSICAL_MAGNITUDE_MULTIPLAYER (1.0/ADC_DISCRETE) //=400
#endif

/**������������ ���������� �������� - ���� */
#define TEMP_PHYSICAL_MAGNITUDE_MULTIPLAYER (TSENS_SLOPP / ADC_DISCRETE) //=4

/** ��������� ���������� ����������� �������� � ���
 * \return �������� � ��������� ���
 */
uint16_t adc_get_map_value(void);

/** ��������� ���������� ����������� �������� ���������� �������� ����
 * \return �������� � ��������� ���
 */
uint16_t adc_get_ubat_value(void);

/** ��������� ���������� ����������� �������� � ����
 * \return �������� � ��������� ���
 */
uint16_t adc_get_temp_value(void);

/** ��������� ���������� ����������� �������� ������� ���������
 * \return �������� � ��������� ���
 */
uint16_t adc_get_knock_value(void);

#ifdef TPS_SENSOR
/** ��������� ���������� ����������� �������� ������� ����
 * \return �������� � ��������� ���
 */
uint16_t adc_get_tps_value(void);
#endif

/**��������� ��������� �������� � ��������, �� ������ ���� ����������
 * ��������� ���������.
 */
void adc_begin_measure(void);

/**��������� ��������� �������� � ����������� ������ ���������. ��� ��� ����� ���������
 * ������� INT/HOLD � 0 ����� INTOUT �������� � ��������� ���������� ��������� ������ �����
 * 20��� (��������������), � ������ ��������� ����� ���� ���������� �����, �� ������ ������
 * ��������� ��������.
 */
void adc_begin_measure_knock(void);

/**��������� ��������� �������� � �������� � ������� � ��. ������� ��������� ��������
 * � ��������, ��������� ������ � ��
 */
void adc_begin_measure_all(void);

/**�������� ���������� ���
 *\return ���������� �� 0 ���� ��������� ������ (��� �� ������)
 */
uint8_t adc_is_measure_ready(void);

/**������������� ��� � ��� ���������� ��������� */
void adc_init(void);

/**����������� ������������ ��� ��� ������� ����� (����������� �������� � ������������ �����������)
 * \param adcvalue �������� ��� ��� �����������
 * \param factor ���������� ���������������
 * \param correction ��������
 * \return compensated value (���������������� ��������)
 * \details
 * factor = 2^14 * gainfactor,
 * correction = 2^14 * (0.5 - offset * gainfactor),
 * 2^16 * realvalue = 2^2 * (adcvalue * factor + correction)
 */
int16_t adc_compensate(int16_t adcvalue, int16_t factor, int32_t correction);

/**��������� �������� ��� � ���������� �������� - ��������
 * \param adcvalue �������� � ��������� ���
 * \param offset �������� ������ ��� (Curve offset. Can be negative)
 * \param gradient ������ ������ ��� (Curve gradient. If < 0, then it means characteristic curve is inverse)
 * \return ���������� �������� * MAP_PHYSICAL_MAGNITUDE_MULTIPLAYER
 * \details
 * offset  = offset_volts / ADC_DISCRETE, ��� offset_volts - �������� � �������;
 * gradient = 128 * gradient_kpa * MAP_PHYSICAL_MAGNITUDE_MULTIPLAYER * ADC_DISCRETE, ��� gradient_kpa �������� � ����-��������
 */
uint16_t map_adc_to_kpa(int16_t adcvalue, int16_t offset, int16_t gradient);

/**��������� �������� ��� � ���������� �������� - ����������
 * \param adcvalue �������� � ��������� ���
 * \return ���������� �������� * UBAT_PHYSICAL_MAGNITUDE_MULTIPLAYER
 */
uint16_t ubat_adc_to_v(int16_t adcvalue);

#ifdef TPS_SENSOR
/**��������� �������� ��� � ���������� �������� - ���������� ����
 * \param adcvalue �������� � ��������� ���
 * \return ���������� �������� * TPS_PHYSICAL_MAGNITUDE_MULTIPLAYER
 */
uint16_t tps_adc_to_v(int16_t adcvalue);
#endif
#ifndef THERMISTOR_CS
/**Converts ADV value into phisical magnituge - temperature (given from linear sensor)
 * ��������� �������� ��� � ���������� �������� - �����������, ��� ��������� �������
 * \param adcvalue Voltage from sensor (���������� � ������� - �������� � ��������� ���)
 * \return ���������� �������� * TEMP_PHYSICAL_MAGNITUDE_MULTIPLAYER
 */
int16_t temp_adc_to_c(int16_t adcvalue);
#else
/**Converts ADC value into phisical magnitude - temperature (given from thermistor)
 * (��������� �������� ��� � ���������� �������� - ����������� ��� ������������ ������� (���������))
 * \param start Voltage value at lowest temperature in ADC discretes (�������� ���������� ��� ����������� ����������� � ��������� ���)
 * \param step Voltage step in ADC discretes (�������� ���� �� ���������� � ������� , � ��������� ���)
 * \param adcvalue Voltage from sensor (���������� � ������� - �������� � ��������� ���))
 * \return ���������� �������� * TEMP_PHYSICAL_MAGNITUDE_MULTIPLAYER
 */
int16_t thermistor_lookup(uint16_t start, uint16_t step, uint16_t adcvalue);
#endif

#endif //_ADC_H_
