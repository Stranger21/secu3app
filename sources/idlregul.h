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

/** \file fuelpump.h
 * Control of electric fuel pump.
 * (Управление электробензонасосом).
 */

#ifndef _IDLREGUL_H_
#define _IDLREGUL_H_

#ifdef IDL_REGUL
#include <stdint.h>
#include "vstimer.h"

// Напряжение на ДПДЗ в момент запуска , соответсвующее 1800оборотам
#define TPS_V_START            1.7

// Напряжение на ДПДЗ минимальное для РХХ ограничение
#define TPS_V_MIN              0.3

// Напряжение на ДПДЗ максимальное для РХХ ограничение
#define TPS_V_MAX              4.0

// Время для перемещения ДПДЗ на 0.1 вольт , ms
#define IDL_TIME_MIN          40.0

extern uint8_t idl_start_en;
extern uint16_t tps_v_buf;

struct ecudata_t;

/** Initialization of used I/O ports (инициализация используемых портов) */
void idlregul_init_ports(void);

// процедура управления РХХ входное число 1 - выдвигать , 2 задвигать , 3 стоп
void idlregul_set_state(uint8_t i_state, struct ecudata_t* d);

//Процедура устаовки РХХ в стартовую позицию для запуска и задания 1800 оборотов
void idlregul_control(struct ecudata_t* d);

//Процедура реализации режима демпфера РХХ
void idlregul_dempfer(struct ecudata_t* d, volatile s_timer8_t* io_timer);

#endif //IDL_REGUL
#endif //_IDLREGUL_H_
