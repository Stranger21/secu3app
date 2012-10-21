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

/** \file ckps.c
 * Implementation of crankshaft position sensor's processing.
 * (���������� ��������� ������� ��������� ���������).
 */

#include <stdlib.h>
#include "port/avrio.h"
#include "port/interrupt.h"
#include "port/intrinsic.h"
#include "port/port.h"
#include "adc.h"
#include "bitmask.h"
#include "camsens.h"
#include "ckps.h"
#include "ioconfig.h"
#include "magnitude.h"

#include "secu3.h"
#include "knock.h"

//PHASED_IGNITION can't be used without PHASE_SENSOR
#if defined(PHASED_IGNITION) && !defined(PHASE_SENSOR)
 #error "You can not use phased ignition without phase sensor. Define PHASE_SENSOR if it is present in the system or do not use phased ignition!"
#endif

/**Maximum number of ignition channels */
#define IGN_CHANNELS_MAX      8

/** Barrier for detecting of missing teeth (������ ��� �������� �����������) 
 * e.g. for 60-2 crank wheel, p * 2.5
 *      for 36-1 crank wheel, p * 1.5
 */
#define CKPS_GAP_BARRIER(p) ( ((p) << (ckps.miss_cogs_num==2)) + ((p) >> 1) )

/** number of teeth that will be skipped at the start before synchronization
 * (���������� ������ ������� ����� ������������ ��� ������ ����� ��������������) */
#define CKPS_ON_START_SKIP_COGS      5

/** Access Input Capture Register */
#define GetICR() (ICR1)

/** Used to indicate that none from ignition channels are selected
 * (������������ ��� �������� ���� ��� �� ���� ����� ��������� �� ������) */
#define CKPS_CHANNEL_MODENA  255

//Specify values depending on invertion option
#ifndef INVERSE_IGN_OUTPUTS
 #define IGN_OUTPUTS_INIT_VAL 1  //!< value used for initialization
 #define IGN_OUTPUTS_ON_VAL   1  //!< value used to turn on ignition channel
 #define IGN_OUTPUTS_OFF_VAL  0  //!< value used to turn off ignition channel
#else //outputs inversion mode (����� �������� �������)
 #define IGN_OUTPUTS_INIT_VAL 0
 #define IGN_OUTPUTS_ON_VAL   0
 #define IGN_OUTPUTS_OFF_VAL  1
#endif

/**�������� ����� � ���������� COMPA � ��������� ������ �� �����. ����� ����� � ����� �������.
 * ������������ ��� ����������� �������. */
#define COMPA_VECT_DELAY 2

// Flags (see flags variable)
#define F_ERROR     0                 //!< CKP error flag, set in the CKP's interrupt, reset after processing (������� ������ ����, ��������������� � ���������� �� ����, ������������ ����� ���������) 
#define F_VHTPER    1                 //!< used to indicate that measured period is valid (actually measured)
#define F_ISSYNC    2                 //!< indicates that synchronization has been completed (missing teeth found)
#define F_STROKE    3                 //!< flag for synchronization with rotation (���� ������������� � ���������)
#define F_USEKNK    4                 //!< flag which indicates using of knock channel (������� ������������� ������ ���������)
#define F_NTSCHA    5                 //!< indicates that it is necessary to set channel
#ifdef DWELL_CONTROL
 #define F_NTSCHB   6                 //!< Indicates that it is necessary to set channel (COMPB) 
#endif
#define F_IGNIEN    7                 //!< Ignition enabled/disabled

//Additional flags (see flags2 variable)
#ifdef DWELL_CONTROL
 #define F_ADDPTK    0                //!< indicates that time between spark and next cog was taken into account
#endif
#ifdef PHASED_IGNITION
 #define F_CAMISS    1                //!< Indicates that system has already obtained event from a cam sensor
#endif
#define F_CALTIM     2                //!< Indicates that time calculation is started before the spark

/** State variables */
typedef struct
{
 uint16_t icr_prev;                   //!< previous value if Input Capture Register (���������� �������� �������� �������)
 volatile uint16_t period_curr;       //!< last measured inter-tooth period (�������� ���������� ��������� ������)
 uint16_t period_prev;                //!< previous value of inter-tooth period (���������� �������� ���������� �������)
 volatile uint16_t cog;               //!< counts teeth starting from missing teeth (2 revolutions), begins from 1 (������� ����� ����� ������, �������� ������� � 1)
 volatile uint8_t cog360;             //!< counts teeth starting from missing teeth (1 revolution).
 uint16_t measure_start_value;        //!< remembers the value of the capture register to measure the half-turn (���������� �������� �������� ������� ��� ��������� ������� �����������)
 uint16_t current_angle;              //!< counts out given advance angle during the passage of each tooth (����������� �������� ��� ��� ����������� ������� ����)
 volatile uint16_t half_turn_period;  //!< stores the last measurement of the passage of teeth n (������ ��������� ��������� ������� ����������� n ������)
 int16_t  advance_angle;              //!< required adv.angle * ANGLE_MULTIPLAYER (��������� ��� * ANGLE_MULTIPLAYER)
 volatile int16_t advance_angle_buffered;//!< buffered value of advance angle (to ensure correct latching)
 uint8_t  ignition_cogs;              //!< number of teeth determining the duration of ignition drive pulse (���-�� ������ ������������ ������������ ��������� ������� ������������)
 uint8_t  starting_mode;              //!< state of state machine processing of teeth at the startup (��������� ��������� �������� ��������� ������ �� �����)
 uint8_t  channel_mode;               //!< determines which channel of the ignition to run at the moment (���������� ����� ����� ��������� ����� ��������� � ������ ������)
 volatile uint8_t cogs_btdc;          //!< number of teeth from missing teeth to TDC of the first cylinder (���-�� ������ �� ����������� �� �.�.� ������� ��������)
 int8_t   knock_wnd_begin_abs;        //!< begin of the phase selection window of detonation in the teeth of wheel, relatively to TDC (������ ���� ������� �������� ��������� � ������ ����� ������������ �.�.�)
 int8_t   knock_wnd_end_abs;          //!< end of the phase selection window of detonation in the teeth of wheel, relatively to TDC (����� ���� ������� �������� ��������� � ������ ����� ������������ �.�.�)
 volatile uint8_t chan_number;        //!< number of ignition channels (���-�� ������� ���������)
 uint32_t frq_calc_dividend;          //!< divident for calculating RPM (������� ��� ������� ������� ��������)
#ifdef DWELL_CONTROL
 volatile uint16_t cr_acc_time;       //!< accumulation time for dwell control (timer's ticks)
 uint8_t  channel_mode_b;             //!< determines which channel of the ignition to start accumulate at the moment (���������� ����� ����� ��������� ����� ����������� ������� � ������ ������)
 uint32_t acc_delay;                  //!< delay between last ignition and next accumulation
 uint16_t tmrval_saved;               //!< value of timer at the moment of each spark
 uint16_t period_saved;               //!< inter-tooth period at the moment of each spark
#endif
 volatile uint8_t chan_mask;          //!< mask used to disable multi-channel mode and use single channel
#ifdef HALL_OUTPUT
 int8_t   hop_offset;                 //!< Hall output: start of pulse in tooth of wheel relatively to TDC
 uint8_t  hop_duration;               //!< Hall output: duration of pulse in tooth of wheel
#endif

 volatile uint8_t wheel_cogs_num;     //!< Number of teeth, including absent (���������� ������, ������� �������������)
 volatile uint8_t wheel_cogs_nump1;   //!< wheel_cogs_num + 1
 volatile uint8_t wheel_cogs_numm1;   //!< wheel_cogs_num - 1
//volatile uint8_t wheel_cogs_numd2;   //!< wheel_cogs_num/2
 volatile uint16_t wheel_cogs_num2;   //!< Number of teeth which corresponds to 720� (2 revolutions)
 volatile uint16_t wheel_cogs_num2p1; //!< wheel_cogs_num2 + 1
 volatile uint8_t miss_cogs_num;      //!< Count of crank wheel's missing teeth (���������� ������������� ������)
 volatile uint8_t wheel_last_cog;     //!< Number of last(present) tooth, numeration begins from 1! (����� ����������(�������������) ����, ��������� ���������� � 1!)
 /**Number of teeth before TDC which determines moment of advance angle latching, start of measurements from sensors,
  * latching of settings into HIP9011 (���-�� ������ �� �.�.� ������������ ������ �������� ���, ����� ��������� ��������,
  * �������� �������� � HIP)
  */
 volatile uint8_t  wheel_latch_btdc;
 volatile uint16_t degrees_per_cog;   //!< Number of degrees which corresponds to the 1 tooth (���������� �������� ������������ �� ���� ��� �����)
 volatile uint16_t cogs_per_chan;     //!< Number of teeth per 1 ignition channel (it is fractional number * 256)
 volatile int16_t start_angle;        //!< Precalculated value of the advance angle at 66� (at least) BTDC
#ifdef STROBOSCOPE
 uint8_t strobe;                      //!< Flag indicates that strobe pulse must be output on pending ignition stroke
#endif
}ckpsstate_t;
 
/**Precalculated data (reference points) and state data for a single channel plug
 * ��������������� ������(������� �����) � ������ ��������� ��� ���������� ������ ���������
 */
typedef struct
{
#ifndef DWELL_CONTROL
 /** Counts out teeth for ignition pulse (����������� ����� �������� ���������) */
 volatile uint8_t ignition_pulse_cogs;
#endif

 /**Address of callback which will be used for settiong of I/O */
 volatile fnptr_t io_callback1;
#ifdef PHASED_IGNITION
 /**Second callback used only in semi-sequential ignition mode */
 volatile fnptr_t io_callback2;
#endif

#ifdef HALL_OUTPUT
 volatile uint16_t hop_begin_cog;      //!< Hall output: tooth number that corresponds to the beginning of pulse  
 volatile uint16_t hop_end_cog;        //!< Hall output: tooth number that corresponds to the end of pulse
#endif

 /** Determines number of tooth (relatively to TDC) at which "latching" of data is performed (���������� ����� ���� (������������ �.�.�.) �� ������� ���������� "������������" ������) */
 volatile uint16_t cogs_latch;
 /** Determines number of tooth at which measurement of rotation period is performed (���������� ����� ���� �� ������� ������������ ��������� ������� �������� ��������� (����� ���. �������)) */
 volatile uint16_t cogs_btdc;
 /** Determines number of tooth at which phase selection window for knock detection is opened (���������� ����� ���� �� ������� ����������� ���� ������� �������� ������� �� (������ ��������������)) */
 volatile uint16_t knock_wnd_begin;
 /** Determines number of tooth at which phase selection window for knock detection is closed (���������� ����� ���� �� ������� ����������� ���� ������� �������� ������� �� (����� ��������������)) */
 volatile uint16_t knock_wnd_end;
}chanstate_t;

ckpsstate_t ckps;                         //!< instance of state variables
chanstate_t chanstate[IGN_CHANNELS_MAX];  //!< instance of array of channel's state variables

/** Arrange flags in the free I/O register (��������� � ��������� �������� �����/������) 
 *  note: may be not effective on other MCUs or even case bugs! Be aware.
 */
#define flags  TWAR  
#define flags2 TWBR

/** Supplement timer/counter 0 up to 16 bits, use R15 (��� ���������� �������/�������� 0 �� 16 ��������, ���������� R15) */
#ifdef __ICCAVR__
 __no_init __regvar uint8_t TCNT0_H@15;
#else //GCC
 uint8_t TCNT0_H __attribute__((section (".noinit")));
#endif

/**Table srtores dividends for calculating of RPM */
#define FRQ_CALC_DIVIDEND(channum) PGM_GET_DWORD(&frq_calc_dividend[channum])
prog_uint32_t frq_calc_dividend[1+IGN_CHANNELS_MAX] = 
 //     1          2          3          4         5         6      7      8
 {0, 30000000L, 15000000L, 10000000L, 7500000L, 6000000L, 5000000L, 4285714L, 3750000L};

void ckps_init_state_variables(void)
{
#ifndef DWELL_CONTROL
 uint8_t i;
 _BEGIN_ATOMIC_BLOCK();
 //Set to value that means to do nothing with outputs
 //(��������� � �������� ���������� ������ �� ������ � ��������) 
 for(i = 0; i < IGN_CHANNELS_MAX; ++i)
  chanstate[i].ignition_pulse_cogs = 255;
#else
 _BEGIN_ATOMIC_BLOCK();
 ckps.cr_acc_time = 0;
 ckps.channel_mode_b = CKPS_CHANNEL_MODENA;
 CLEARBIT(flags, F_NTSCHB);
#endif

 ckps.cog = ckps.cog360 = 0;
 ckps.half_turn_period = 0xFFFF;
 ckps.advance_angle = 0;
 ckps.advance_angle_buffered = 0;
 ckps.starting_mode = 0;
 ckps.channel_mode = CKPS_CHANNEL_MODENA;
#ifdef PHASED_IGNITION
 CLEARBIT(flags2, F_CAMISS);
#endif
 CLEARBIT(flags, F_NTSCHA);
 CLEARBIT(flags, F_STROKE);
 CLEARBIT(flags, F_ISSYNC);
 SETBIT(flags, F_IGNIEN);
 CLEARBIT(flags2, F_CALTIM);

 TCCR0 = 0; //timer is stopped (������������� ������0)
#ifdef STROBOSCOPE
 ckps.strobe = 0;
#endif
 _END_ATOMIC_BLOCK();
}

void ckps_init_state(void)
{
 _BEGIN_ATOMIC_BLOCK();
 ckps_init_state_variables();
 CLEARBIT(flags, F_ERROR);

 //Compare channels do not connected to lines of ports (normal port mode)
 //(������ Compare �� ���������� � ������ ������ (���������� ����� ������))
 TCCR1A = 0;

 //(Noise reduction(���������� ����), rising edge of capture(�������� ����� �������), clock = 250kHz)
 TCCR1B = _BV(ICNC1)|_BV(ICES1)|_BV(CS11)|_BV(CS10);

 //enable input capture and Compare A interrupts of timer 1, also overflow interrupt of timer 0
 //(��������� ���������� �� ������� � ��������� � ������� 1, � ����� �� ������������ ������� 0)
 TIMSK|= _BV(TICIE1)|_BV(TOIE0);
 _END_ATOMIC_BLOCK();
}

void ckps_set_advance_angle(int16_t angle)
{
 _BEGIN_ATOMIC_BLOCK();
 ckps.advance_angle_buffered = angle;
 _END_ATOMIC_BLOCK();
}

void ckps_init_ports(void)
{
 PORTD|= _BV(PD6); // pullup for ICP1 (�������� ��� ICP1)

 //after ignition is on, igniters must not be in the accumulation mode,
 //therefore set low level on their inputs
 //(����� ��������� ��������� ����������� �� ������ ���� � ������ ����������,
 //������� ������������� �� �� ������ ������ �������)
 iocfg_i_ign_out1(IGN_OUTPUTS_INIT_VAL);                //init 1-st ignition channel
 iocfg_i_ign_out2(IGN_OUTPUTS_INIT_VAL);                //init 2-nd ignition channel
 IOCFG_INIT(IOP_IGN_OUT3, IGN_OUTPUTS_INIT_VAL);        //init 3-rd (can be remapped)
 IOCFG_INIT(IOP_IGN_OUT4, IGN_OUTPUTS_INIT_VAL);        //init 4-th (can be remapped)
 IOCFG_INIT(IOP_ADD_IO1, IGN_OUTPUTS_INIT_VAL);         //init 5-th (can be remapped)
 IOCFG_INIT(IOP_ADD_IO2, IGN_OUTPUTS_INIT_VAL);         //init 6-th (can be remapped)

 //init I/O for Hall output if it is enabled
#ifdef HALL_OUTPUT
 IOCFG_INIT(IOP_HALL_OUT, 1);
#endif

 //init I/O for stroboscope
#ifdef STROBOSCOPE
 IOCFG_INIT(IOP_STROBE, 0);
#endif
}

//Instantaneous frequency calculation of crankshaft rotation from the measured period between the engine strokes
//(for example for 4-cylinder, 4-stroke it is 180�) 
//Period measured in the discretes of timer (one discrete = 4us), one minute = 60 seconds, one second has 1,000,000 us.
//������������ ���������� ������� �������� ��������� �� ����������� ������� ����� ������� ��������� 
//(�������� ��� 4-������������, 4-� �������� ��� 180 ��������)
//������ � ��������� ������� (���� �������� = 4���), � ����� ������ 60 ���, � ����� ������� 1000000 ���.
uint16_t ckps_calculate_instant_freq(void)
{
 uint16_t period;
 _DISABLE_INTERRUPT();
 period = ckps.half_turn_period;           //ensure atomic acces to variable (������������ ��������� ������ � ����������)
 _ENABLE_INTERRUPT();

 //if equal to minimum, this means engine is stopped (���� ����� �������, ������ ��������� �����������)
 if (period != 0xFFFF)
  return (ckps.frq_calc_dividend)/(period);
 else
  return 0;
}

void ckps_set_edge_type(uint8_t edge_type)
{
 _BEGIN_ATOMIC_BLOCK();
 if (edge_type)
  TCCR1B|= _BV(ICES1);
 else
  TCCR1B&=~_BV(ICES1);
 _END_ATOMIC_BLOCK();
}

/**
 * Ensures that tooth number will be in the allowed range.
 * Tooth number should not be greater than cogs number * 2 or less than zero
 */
static uint16_t _normalize_tn(int16_t i_tn)
{
 if (i_tn > (int16_t)ckps.wheel_cogs_num2)
  return i_tn - (int16_t)ckps.wheel_cogs_num2;
 if (i_tn < 0)
  return i_tn + ckps.wheel_cogs_num2;
 return i_tn;
}

void ckps_set_cogs_btdc(uint8_t cogs_btdc)
{
 uint8_t _t, i;
 _t=_SAVE_INTERRUPT();
 _DISABLE_INTERRUPT();
 for(i = 0; i < ckps.chan_number; ++i)
 {
  uint16_t tdc = (((uint16_t)cogs_btdc) + ((i * ckps.cogs_per_chan) >> 8));
  chanstate[i].cogs_btdc = _normalize_tn(tdc);
  chanstate[i].cogs_latch = _normalize_tn(tdc - ckps.wheel_latch_btdc);
  chanstate[i].knock_wnd_begin = _normalize_tn(tdc + ckps.knock_wnd_begin_abs);
  chanstate[i].knock_wnd_end = _normalize_tn(tdc + ckps.knock_wnd_end_abs);
#ifdef HALL_OUTPUT
  //update Hall output pulse parameters because they depend on ckps.cogs_btdc parameter
  chanstate[i].hop_begin_cog = _normalize_tn(tdc - ckps.hop_offset);
  chanstate[i].hop_end_cog = _normalize_tn(chanstate[i].hop_begin_cog + ckps.hop_duration);
#endif
 }
 ckps.cogs_btdc = cogs_btdc;
 _RESTORE_INTERRUPT(_t);
}

#ifndef DWELL_CONTROL
void ckps_set_ignition_cogs(uint8_t cogs)
{
 _BEGIN_ATOMIC_BLOCK();
 ckps.ignition_cogs = cogs;
 _END_ATOMIC_BLOCK();
}
#else
void ckps_set_acc_time(uint16_t i_acc_time)
{
 _BEGIN_ATOMIC_BLOCK();
 ckps.cr_acc_time = i_acc_time;
 _END_ATOMIC_BLOCK();
}
#endif

uint8_t ckps_is_error(void)
{
 return CHECKBIT(flags, F_ERROR) > 0;
}

void ckps_reset_error(void)
{
 CLEARBIT(flags, F_ERROR);
}

void ckps_use_knock_channel(uint8_t use_knock_channel)
{
 WRITEBIT(flags, F_USEKNK, use_knock_channel);
}

uint8_t ckps_is_stroke_event_r()
{
 uint8_t result;
 _BEGIN_ATOMIC_BLOCK();
 result = CHECKBIT(flags, F_STROKE) > 0;
 CLEARBIT(flags, F_STROKE);
 _END_ATOMIC_BLOCK();
 return result;
}

uint8_t ckps_get_current_cog(void)
{
 return ckps.cog;
}

uint8_t ckps_is_cog_changed(void)
{
 static uint8_t prev_cog = 0;
 uint8_t value = ckps.cog;
 if (prev_cog != value)
 {
  prev_cog = value;
  return 1;
 }
 return 0;
}

/** Get value of I/O callback by index
 * \param index Index of callback
 */
static fnptr_t get_callback(uint8_t index)
{
 if (0 == index)
  return (fnptr_t)iocfg_s_ign_out1;
 else if (1 == index)
  return (fnptr_t)iocfg_s_ign_out2;
 else
  return IOCFG_CB(index);
}

#ifndef PHASED_IGNITION
/**Tune channels' I/O for semi-sequential ignition mode (wasted spark) */
static void set_channels_ss(void)
{
 uint8_t _t, i = 0, chan = ckps.chan_number / 2;
 for(; i < chan; ++i)
 {
  fnptr_t value = get_callback(i);
  _t=_SAVE_INTERRUPT();
  _DISABLE_INTERRUPT();
  chanstate[i].io_callback1 = value;
  chanstate[i + chan].io_callback1 = value;
  _RESTORE_INTERRUPT(_t);
 }
}

#else

/**Tune channels' I/O for full sequential ignition mode */
static void set_channels_fs(uint8_t fs_mode)
{
 uint8_t _t, i = 0, ch2 = fs_mode ? 0 : ckps.chan_number / 2, iss;
 for(; i < ckps.chan_number; ++i)
 {
  iss = (i + ch2);
  if (iss >= ckps.chan_number)
   iss-=ckps.chan_number;

  _t=_SAVE_INTERRUPT();
  _DISABLE_INTERRUPT();
  chanstate[i].io_callback1 = get_callback(i);
  chanstate[i].io_callback2 = get_callback(iss);
  _RESTORE_INTERRUPT(_t);
 }
}
#endif

void ckps_set_cyl_number(uint8_t i_cyl_number)
{
 _BEGIN_ATOMIC_BLOCK();
 ckps.chan_number = i_cyl_number;
 _END_ATOMIC_BLOCK();

 ckps.frq_calc_dividend = FRQ_CALC_DIVIDEND(i_cyl_number);

 //We have to retune I/O configuration after changing of cylinder number
#ifndef PHASED_IGNITION
 set_channels_ss();  // Tune for semi-sequential mode
#else //phased ignition
 //Tune for full sequential mode if cam sensor works, otherwise tune for semi-sequential mode
 set_channels_fs(cams_is_ready());
#endif

 //TODO: calculations previosly made by ckps_set_cogs_btdc()|ckps_set_knock_window()|ckps_set_hall_pulse() becomes invalid!
 //So, ckps_set_cogs_btdc() must be called again. Do it here or in place where this function called.
}

void ckps_set_knock_window(int16_t begin, int16_t end)
{
 uint8_t _t, i;
 //translate from degrees to teeth (��������� �� �������� � �����)
 ckps.knock_wnd_begin_abs = begin / ckps.degrees_per_cog;
 ckps.knock_wnd_end_abs = end / ckps.degrees_per_cog;

 _t=_SAVE_INTERRUPT();
 _DISABLE_INTERRUPT();
 for(i = 0; i < ckps.chan_number; ++i)
 {
  uint16_t tdc = (((uint16_t)ckps.cogs_btdc) + ((i * ckps.cogs_per_chan) >> 8));
  chanstate[i].knock_wnd_begin = _normalize_tn(tdc + ckps.knock_wnd_begin_abs);
  chanstate[i].knock_wnd_end = _normalize_tn(tdc + ckps.knock_wnd_end_abs);
 }
 _RESTORE_INTERRUPT(_t);
}

void ckps_enable_ignition(uint8_t i_cutoff)
{
 WRITEBIT(flags, F_IGNIEN, i_cutoff);
}

void ckps_set_merge_outs(uint8_t i_merge)
{
 ckps.chan_mask = i_merge ? 0x00 : 0xFF;
}

#ifdef HALL_OUTPUT
void ckps_set_hall_pulse(int8_t i_offset, uint8_t i_duration)
{
 uint8_t _t, i;
 //save values because we will access them from other function
 ckps.hop_offset = i_offset;
 ckps.hop_duration = i_duration;

 _t=_SAVE_INTERRUPT();
 _DISABLE_INTERRUPT();
 for(i = 0; i < ckps.chan_number; ++i)
 {
  uint16_t tdc = (((uint16_t)ckps.cogs_btdc) + ((i * ckps.cogs_per_chan) >> 8));
  chanstate[i].hop_begin_cog = _normalize_tn(tdc - ckps.hop_offset);
  chanstate[i].hop_end_cog = _normalize_tn(chanstate[i].hop_begin_cog + ckps.hop_duration);
 }
 _RESTORE_INTERRUPT(_t);
}
#endif

void ckps_set_cogs_num(uint8_t norm_num, uint8_t miss_num)
{
 div_t dr; uint8_t _t;
 uint16_t cogs_per_chan, degrees_per_cog;

 //precalculate number of cogs per 1 ignition channel, it is fractional number multiplied by 256
 cogs_per_chan = (((uint32_t)(norm_num * 2)) << 8) / ckps.chan_number;

 //precalculate value of degrees per 1 cog, it is fractional number multiplied by ANGLE_MULTIPLAYER
 degrees_per_cog = (((((uint32_t)360) << 8) / norm_num) * ANGLE_MULTIPLAYER) >> 8;

 //precalculate value and round it always to the upper bound,
 //e.g. for 60-2 crank wheel result = 11 (66�), for 36-1 crank wheel result = 7 (70�)
 dr = div(ANGLE_MAGNITUDE(66), degrees_per_cog);

 _t=_SAVE_INTERRUPT();
 _DISABLE_INTERRUPT();
 //calculate number of last cog
 ckps.wheel_last_cog = norm_num - miss_num;
 //set number of teeth (normal and missing)
 ckps.wheel_cogs_num = norm_num;
 ckps.wheel_cogs_nump1 = norm_num + 1;
 ckps.wheel_cogs_numm1 = norm_num - 1;
//ckps.wheel_cogs_numd2 = norm_num >> 1;
 ckps.miss_cogs_num = miss_num;
 ckps.wheel_cogs_num2 = norm_num * 2;
 ckps.wheel_cogs_num2p1 = (norm_num * 2) + 1;
 //set other precalculated values
 ckps.wheel_latch_btdc = dr.quot + (dr.rem > 0);
 ckps.degrees_per_cog = degrees_per_cog;
 ckps.cogs_per_chan = cogs_per_chan;
 ckps.start_angle = ckps.degrees_per_cog * ckps.wheel_latch_btdc;
 _RESTORE_INTERRUPT(_t);
}

/** Turn OFF specified ignition channel
 * \param i_channel number of ignition channel to turn off
 */
INLINE
void turn_off_ignition_channel(uint8_t i_channel)
{
 if (!CHECKBIT(flags, F_IGNIEN))
  return; //ignition disabled
 //Completion of igniter's ignition drive pulse, transfer line of port into a low level - makes 
 //the igniter go to the regime of energy accumulation
 //���������� �������� ������� �����������, ������� ����� ����� � ������ ������� - ����������
 //���������� ������� � ����� ���������� �������
 ((iocfg_pfn_set)chanstate[i_channel].io_callback1)(IGN_OUTPUTS_OFF_VAL);
#ifdef PHASED_IGNITION
 ((iocfg_pfn_set)chanstate[i_channel].io_callback2)(IGN_OUTPUTS_OFF_VAL);
#endif
}

/**Forces ignition spark if corresponding interrupt is pending*/
#ifdef PHASED_IGNITION
#define force_pending_spark() \
 if ((TIFR & _BV(OCF1A)) && (CHECKBIT(flags2, F_CALTIM))\
 { \
  ((iocfg_pfn_set)chanstate[ckps.channel_mode].io_callback1)(IGN_OUTPUTS_ON_VAL);\
  ((iocfg_pfn_set)chanstate[ckps.channel_mode].io_callback2)(IGN_OUTPUTS_ON_VAL);\
 }
#else
#define force_pending_spark() \
 if ((TIFR & _BV(OCF1A)) && (CHECKBIT(flags2, F_CALTIM)))\
  ((iocfg_pfn_set)chanstate[ckps.channel_mode].io_callback1)(IGN_OUTPUTS_ON_VAL);
#endif
 
/**Interrupt handler for Compare/Match channel A of timer T1
 * ������ ���������� �� ���������� ������ � ������� �1
 */
ISR(TIMER1_COMPA_vect)
{
#ifdef COOLINGFAN_PWM
 uint8_t timsk_sv, ucsrb_sv = UCSRB;
#endif

#ifdef DWELL_CONTROL
 ckps.tmrval_saved = TCNT1;
#endif

 TIMSK&= ~_BV(OCIE1A); //disable interrupt (��������� ����������)

 //line of port in the low level, now set it into a high level - makes the igniter to stop 
 //the accumulation of energy and close the transistor (spark)
 //(����� ����� � ������ ������, ������ ��������� � � ������� ������� - ���������� ���������� ����������
 //���������� ������� � ������� ���������� (�����)).
 if (CKPS_CHANNEL_MODENA == ckps.channel_mode)
  return; //none of channels selected (������� ����� �� ������)

#ifdef STROBOSCOPE
 if (1==ckps.strobe)
 {
  IOCFG_SET(IOP_STROBE, 1); //start pulse
  ckps.strobe = 2;          //and set flag to next state
  OCR1A = TCNT1 + 25;       //We will generate 100uS pulse
  TIMSK|= _BV(OCIE1A);      //pulse will be ended in the next interrupt
 }
 else if (2==ckps.strobe)
 {
  IOCFG_SET(IOP_STROBE, 0); //end pulse
  ckps.strobe = 0;          //and reset flag
  return;
 }
#endif

 ((iocfg_pfn_set)chanstate[ckps.channel_mode].io_callback1)(IGN_OUTPUTS_ON_VAL);
#ifdef PHASED_IGNITION
 ((iocfg_pfn_set)chanstate[ckps.channel_mode].io_callback2)(IGN_OUTPUTS_ON_VAL);
#endif

 //-----------------------------------------------------
#ifdef COOLINGFAN_PWM
 timsk_sv = TIMSK;
 //ADCSRA&=~_BV(ADIE);            //??
 UCSRB&=~(_BV(RXCIE)|_BV(UDRIE)); //mask UART interrupts
 TIMSK&= _BV(OCIE2)|_BV(TOIE2);   //mask all timer interrupts except OCF2 and TOV2
 _ENABLE_INTERRUPT();
#endif
 //-----------------------------------------------------

#ifdef DWELL_CONTROL 
 ckps.acc_delay = (((uint32_t)ckps.period_curr) * ckps.cogs_per_chan) >> 8; 
 if (ckps.cr_acc_time > ckps.acc_delay-120)
  ckps.cr_acc_time = ckps.acc_delay-120;  //restrict accumulation time. Dead band = 500us
 ckps.acc_delay-= ckps.cr_acc_time;
 ckps.period_saved = ckps.period_curr; //remember current inter-tooth period

 ckps.channel_mode_b = (ckps.channel_mode < ckps.chan_number-1) ? ckps.channel_mode + 1 : 0 ;
 ckps.channel_mode_b&= ckps.chan_mask;
 CLEARBIT(flags2, F_ADDPTK);
 SETBIT(flags, F_NTSCHB);

 //We remembered value of TCNT1 at the top of of this function. But input capture event
 //may occur when interrupts were already disabled (by hardware) but value of timer is still
 //not saved. Another words, ICR1 must not be less than tmrval_saved.
 if (TIFR & _BV(ICF1))
  ckps.tmrval_saved = ICR1;
#else
 //start counting the duration of pulse in the teeth (�������� ������ ������������ �������� � ������)
 chanstate[ckps.channel_mode].ignition_pulse_cogs = 0;
#endif

 CLEARBIT(flags2, F_CALTIM); //we already output the spark, so calculation of time is finished
 //-----------------------------------------------------
#ifdef COOLINGFAN_PWM
 _DISABLE_INTERRUPT();
 //ADCSRA|=_BV(ADIE);
 UCSRB = ucsrb_sv;
 TIMSK = timsk_sv;
#endif
 //-----------------------------------------------------
}

#ifdef DWELL_CONTROL
/**Interrupt handler for Compare/Match channel B of timer T1. Used for dwell control.
 * ������ ���������� �� ���������� ������ B ������� �1. ������������ ��� ���������� ����������� ������� ��.
 */
ISR(TIMER1_COMPB_vect)
{
 TIMSK&= ~_BV(OCIE1B); //��������� ����������
 //start accumulation
 turn_off_ignition_channel(ckps.channel_mode_b);
 ckps.channel_mode_b = CKPS_CHANNEL_MODENA;
}
#endif

/**Initialization of timer 0 using specified value and start, clock = 250kHz
 * It is assumed that this function called when all interrupts are disabled
 * (������������� ������� 0 ��������� ��������� � ������, clock = 250kHz.
 * �������������� ��� ����� ���� ������� ����� ����������� ��� ����������� �����������)
 * \param value value for load into timer (�������� ��� �������� � ������)
 */
INLINE
void set_timer0(uint16_t value)
{
 TCNT0_H = _AB(value, 1);
 TCNT0 = ~(_AB(value, 0));  //One's complement is faster than 255 - low byte
 TCCR0  = _BV(CS01)|_BV(CS00);
}

/**Helpful function, used at the startup of engine
 * (��������������� �������, ������������ �� ����� �����)
 * \return 1 when synchronization is finished, othrewise 0 (1 ����� ������������� ��������, ����� 0)
 */
static uint8_t sync_at_startup(void)
{
 switch(ckps.starting_mode)
 {
  case 0: //skip certain number of teeth (������� ������������� ���-�� ������)
   CLEARBIT(flags, F_VHTPER);
   if (ckps.cog >= CKPS_ON_START_SKIP_COGS) 
    ckps.starting_mode = 1;
   break;

  case 1: //find out missing teeth (����� �����������)
#ifdef PHASED_IGNITION
   if (ckps.chan_number & 1) //cylinder number is odd?
   {
    cams_detect_edge();
    if (cams_is_event_r())
     SETBIT(flags2, F_CAMISS);
   }
   else //cylinder number is even, cam synchronization will be performed later
    SETBIT(flags2, F_CAMISS);
#endif

   //if missing teeth = 0, then reference will be identified by additional VR sensor (REF_S input)
   if (
#ifdef SECU3T
   ((0==ckps.miss_cogs_num) ? cams_vr_is_event_r() : (ckps.period_curr > CKPS_GAP_BARRIER(ckps.period_prev)))
#else
   (ckps.period_curr > CKPS_GAP_BARRIER(ckps.period_prev))
#endif
#ifdef PHASED_IGNITION
   && CHECKBIT(flags2, F_CAMISS)
#endif
   ) 
   {
    SETBIT(flags, F_ISSYNC);
    ckps.period_curr = ckps.period_prev;  //exclude value of missing teeth's period
    ckps.cog = ckps.cog360 = 1; //first tooth (1-� ���)
    return 1; //finish process of synchronization (����� �������� �������������)
   }
   break;
 }
 ckps.icr_prev = GetICR();
 ckps.period_prev = ckps.period_curr;
 ++ckps.cog;
 return 0; //continue process of synchronization (����������� �������� �������������)
}

/**This procedure called for all teeth (including recovered teeth)
 * ���������. ���������� ��� ���� ������ ����� (������������ � ����������������)
 */
static void process_ckps_cogs(void)
{
 uint8_t i, timsk_sv = TIMSK;

 //-----------------------------------------------------
 //Software PWM is very sensitive even to small delays. So, we need to allow OCF2 and TOV2
 //interrupts occur during processing of this handler.
#ifdef COOLINGFAN_PWM
 //remember current state, mask all not urgent interrupts and enable interrupts
 uint8_t ucsrb_sv = UCSRB;
 //ADCSRA&=~_BV(ADIE);            //??
 UCSRB&=~(_BV(RXCIE)|_BV(UDRIE)); //mask UART interrupts
 TIMSK&= _BV(OCIE2)|_BV(TOIE2);   //mask all timer interrupts except OCF2 and TOV2
 _ENABLE_INTERRUPT();
#endif
 //-----------------------------------------------------
 force_pending_spark();

#ifdef DWELL_CONTROL
 if (ckps.channel_mode_b != CKPS_CHANNEL_MODENA)
 {
  //We must take into account time elapsed between last spark and following tooth.
  //Because ICR1 can not be less than tmrval_saved we are using addition modulo 65535
  //to calculate difference (elapsed time).
  if (!CHECKBIT(flags2, F_ADDPTK))
  {
   ckps.acc_delay-=((uint16_t)(~ckps.tmrval_saved)) + 1 + GetICR();
   SETBIT(flags2, F_ADDPTK);
  }
  //Correct our prediction on each cog 
  ckps.acc_delay+=((int32_t)ckps.period_curr - ckps.period_saved);

  //Do we have to set COMPB ? (We have to set COMPB if less than 2 periods remain)
  if (CHECKBIT(flags, F_NTSCHB) && ckps.acc_delay <= (ckps.period_curr << 1))
  {
   OCR1B = GetICR() + ckps.acc_delay + ((int32_t)ckps.period_curr - ckps.period_saved);
   TIFR = _BV(OCF1B);
   timsk_sv|= _BV(OCIE1B);
   CLEARBIT(flags, F_NTSCHB); // To avoid entering into setup mode (����� �� ����� � ����� ��������� ��� ���)
  }
  ckps.acc_delay-=ckps.period_curr;
 }
#endif

 force_pending_spark();

 for(i = 0; i < ckps.chan_number; ++i)
 {
  if (CHECKBIT(flags, F_USEKNK))
  {
   //start listening a detonation (opening the window)
   //�������� ������� ��������� (�������� ����)
   if (ckps.cog == chanstate[i].knock_wnd_begin)
    knock_set_integration_mode(KNOCK_INTMODE_INT);

   //finish listening a detonation (closing the window) and start the process of measuring integrated value
   //����������� ������� ��������� (�������� ����) � ��������� ������� ��������� ������������ ��������
   if (ckps.cog == chanstate[i].knock_wnd_end)
   {
    knock_set_integration_mode(KNOCK_INTMODE_HOLD);
    adc_begin_measure_knock(_AB(ckps.half_turn_period, 1) < 4);
   }
  }

  //for 66� before TDC (before working stroke) establish new advance angle to be actuated,
  //before this moment value was stored in a temporary buffer.
  //�� 66 �������� �� �.�.� ����� ������� ������ ������������� ����� ��� ��� ����������, ���
  //�� ����� �������� �� ��������� ������.
  if (ckps.cog == chanstate[i].cogs_latch)
  {
   ckps.channel_mode = (i & ckps.chan_mask); //remember number of channel (���������� ����� ������)
   SETBIT(flags, F_NTSCHA);                  //establish an indication that it is need to count advance angle (������������� ������� ����, ��� ����� ����������� ���)
   //start counting of advance angle (�������� ������ ���� ����������)
   ckps.current_angle = ckps.start_angle; // those same 66� (�� ����� 66�)
   ckps.advance_angle = ckps.advance_angle_buffered; //advance angle with all the adjustments (say, 15�)(���������� �� ����� ��������������� (��������, 15�))
   knock_start_settings_latching();//start the process of downloading the settings into the HIP9011 (��������� ������� �������� �������� � HIP)
   adc_begin_measure(_AB(ckps.half_turn_period, 1) < 4);//start the process of measuring analog input values (������ �������� ��������� �������� ���������� ������)
#ifdef STROBOSCOPE
   if (0==i)
    ckps.strobe = 1; //strobe!
#endif   
  }

  force_pending_spark();

  //teeth of end/beginning of the measurement of rotation period - TDC Read and save the measured period,
  //then remember current value of count for the next measurement
  //(����� ����������/������ ��������� �������� ��������  - �.�.�. ���������� � ���������� ����������� �������,
  //����� ����������� �������� �������� �������� ��� ���������� ���������)
  if (ckps.cog==chanstate[i].cogs_btdc) 
  {
   //if it was overflow, then set the maximum possible time
   //���� ���� ������������ �� ������������� ����������� ��������� �����
   if (((ckps.period_curr > 1250) || !CHECKBIT(flags, F_VHTPER)))
    ckps.half_turn_period = 0xFFFF;
   else
    ckps.half_turn_period = (GetICR() - ckps.measure_start_value);

   ckps.measure_start_value = GetICR();
   SETBIT(flags, F_VHTPER);
   SETBIT(flags, F_STROKE); //set the stroke-synchronozation event (������������� ������� �������� �������������)
  }

#ifdef HALL_OUTPUT
  if (ckps.cog == chanstate[i].hop_begin_cog)
   IOCFG_SET(IOP_HALL_OUT, 1);
  if (ckps.cog == chanstate[i].hop_end_cog)
   IOCFG_SET(IOP_HALL_OUT, 0);
#endif
 }

 force_pending_spark();

 //Preparing to start the ignition for the current channel (if the right moment became)
 //���������� � ������� ��������� ��� �������� ������ (���� �������� ������ ������)
 if (CHECKBIT(flags, F_NTSCHA) && ckps.channel_mode!= CKPS_CHANNEL_MODENA)
 {
  uint16_t diff = ckps.current_angle - ckps.advance_angle;
  if (diff <= (ckps.degrees_per_cog << 1))
  {
   //before starting the ignition it is left to count less than 2 teeth. It is necessary to prepare the compare module
   //(�� ������� ��������� �������� ��������� ������ 2-x ������. ���������� ����������� ������ ���������)
   //TODO: replace heavy division by multiplication with magic number. This will reduce up to 40uS !
   if (ckps.period_curr < 128)
    OCR1A = GetICR() + ((diff * (ckps.period_curr)) / ckps.degrees_per_cog) - COMPA_VECT_DELAY;
   else
    OCR1A = GetICR() + (((uint32_t)diff * (ckps.period_curr)) / ckps.degrees_per_cog) - COMPA_VECT_DELAY;
   TIFR = _BV(OCF1A);
   CLEARBIT(flags, F_NTSCHA); // For avoiding to enter into setup mode (����� �� ����� � ����� ��������� ��� ���)
   SETBIT(flags2, F_CALTIM);  // Set indication that we begin to calculate the time
   timsk_sv|= _BV(OCIE1A);    // enable Compare A interrupt (��������� ����������)
  }
 }

#ifndef DWELL_CONTROL
 //finish the ignition trigger pulses for igniter(s) and immediately increase the number of tooth for processed channel
 //����������� �������� ������� �����������(��) � ����� ����������� ����� ���� ��� ������������� ������
 for(i = 0; i < ckps.chan_number; ++i)
 {
  if (chanstate[i].ignition_pulse_cogs == 255)
   continue;

  if (chanstate[i].ignition_pulse_cogs >= ckps.ignition_cogs)
  {
   turn_off_ignition_channel(i);
   chanstate[i].ignition_pulse_cogs = 255; //set indication that channel has finished to work
  }
  else
   ++(chanstate[i].ignition_pulse_cogs);
 }
#endif

 //tooth passed - angle before TDC decriased (e.g 6� per tooth for 60-2).
 //(������ ��� - ���� �� �.�.�. ���������� (�������� 6� �� ��� ��� 60-2)).
 ckps.current_angle-= ckps.degrees_per_cog;
 ++ckps.cog;

#ifdef PHASE_SENSOR
 //search for level's toggle from camshaft sensor on each cog
 cams_detect_edge();
#endif
#ifdef PHASED_IGNITION
 force_pending_spark();
 if (cams_is_event_r())
 {
  //Synchronize. We rely that cam sonsor event (e.g. falling edge) coming before missing teeth
  if (ckps.cog < ckps.wheel_cogs_nump1)
   ckps.cog+= ckps.wheel_cogs_num;

  //Turn on full sequential mode because Cam sensor is OK
  if (!CHECKBIT(flags2, F_CAMISS))
  {
   set_channels_fs(1);
   SETBIT(flags2, F_CAMISS);
  }
 }
 if (cams_is_error())
 {
  //Turn off full sequential mode because cam semsor is not OK
  if (CHECKBIT(flags2, F_CAMISS))
  {
   set_channels_fs(0); //<--valid only for engines with even number of cylinders
   CLEARBIT(flags2, F_CAMISS);
  }
 }
#endif

 //-----------------------------------------------------
#ifdef COOLINGFAN_PWM
 //disable interrupts and restore previous states of masked interrupts
 _DISABLE_INTERRUPT();
 //ADCSRA|=_BV(ADIE);
 UCSRB = ucsrb_sv;
#endif
 TIMSK = timsk_sv;
 //-----------------------------------------------------

 force_pending_spark();
}

/**Input capture interrupt of timer 1 (called at passage of each tooth)
 * ���������� �� ������� ������� 1 (���������� ��� ����������� ���������� ����)
 */
ISR(TIMER1_CAPT_vect)
{
 force_pending_spark();

 ckps.period_curr = GetICR() - ckps.icr_prev;

 //At the start of engine, skipping a certain number of teeth for initializing
 //the memory of previous periods. Then look for missing teeth.
 //��� ������ ���������, ���������� ������������ ���-�� ������ ��� �������������
 //������ ���������� ��������. ����� ���� �����������.
 if (!CHECKBIT(flags, F_ISSYNC))
 {
  if (sync_at_startup())
   goto sync_enter;
  return;
 }

 //if missing teeth = 0, then reference will be identified by additional VR sensor (REF_S input),
 //Otherwise:
 //Each period, check for missing teeth, and if, after discovering of missing teeth
 //count of teeth being found incorrect, then set error flag.
 //(������ ������ ��������� �� �����������, � ���� ����� ����������� �����������
 //��������� ��� ���-�� ������ ������������, �� ������������� ������� ������).
#ifdef SECU3T
 if ((0==ckps.miss_cogs_num) ? cams_vr_is_event_r() : (ckps.period_curr > CKPS_GAP_BARRIER(ckps.period_prev)))
#else
 if (ckps.period_curr > CKPS_GAP_BARRIER(ckps.period_prev))
#endif
 {
  if ((ckps.cog360 != ckps.wheel_cogs_nump1)) //also taking into account recovered teeth (��������� ����� ��������������� �����)
  {
   SETBIT(flags, F_ERROR); //ERROR
   ckps.cog = 1;
   //TODO: maybe we need to turn off full sequential mode
  }
  //Reset 360� tooth counter to the first tooth (1-� ���)
  ckps.cog360 = 1;
  //Also reset 720� tooth counter
  if (ckps.cog == ckps.wheel_cogs_num2p1)
   ckps.cog = 1;
  ckps.period_curr = ckps.period_prev;  //exclude value of missing teeth's period (for missing teeth only)
 }

sync_enter:
 //If the last tooth before missing teeth, we begin the countdown for
 //the restoration of missing teeth, as the initial data using the last
 //value of inter-teeth period.
 //(���� ��������� ��� ����� ������������, �� �������� ������ ������� ���
 //�������������� ������������� ������, � �������� �������� ������ ����������
 //��������� �������� ���������� �������).
 if (ckps.miss_cogs_num && ckps.cog360 == ckps.wheel_last_cog)
  set_timer0(ckps.period_curr);

 //call handler for normal teeth (�������� ���������� ��� ���������� ������)
 process_ckps_cogs();
 ++ckps.cog360;

 ckps.icr_prev = GetICR();
 ckps.period_prev = ckps.period_curr;

 force_pending_spark();

//#ifdef SECU3T
// if ((0==ckps.miss_cogs_num) && ckps.cog360 == ckps.wheel_cogs_numd2)
//  cams_enable_vr_event();
//#endif
}

/**Purpose of this interrupt handler is to supplement timer up to 16 bits and call procedure
 * for processing teeth when set 16 bit timer expires
 * (������ ����� ����������� ��������� ������ �� 16-�� �������� � �������� ���������
 * ��������� ������ �� ��������� �������������� 16-�� ���������� �������). */
ISR(TIMER0_OVF_vect)
{
 if (TCNT0_H!=0)  //Did high byte exhaust (������� ���� �� ��������) ?
 {
  TCNT0 = 0;
  --TCNT0_H;
 }
 else
 {//the countdown is over (������ ������� ����������)
  ICR1 = TCNT1;  //simulate input capture 
  TCCR0 = 0;     //stop timer (������������� ������)

  if (ckps.miss_cogs_num > 1)
  {
   //start timer to recover 60th tooth (��������� ������ ����� ������������ 60-� (���������) ���)
   if (ckps.cog360 == ckps.wheel_cogs_numm1)
    set_timer0(ckps.period_curr);
  }

  //Call handler for missing teeth (�������� ���������� ��� ������������� ������)
  process_ckps_cogs();
  ++ckps.cog360;
 }
}
