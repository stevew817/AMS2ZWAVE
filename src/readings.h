/***************************************************************************//**
 * @file readings.h
 * @brief This file describes the HAN meter readout values, updated by hanparser.c
 * @author github.com/stevew817
 *
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *******************************************************************************/

#ifndef AMS_READINGS_H_
#define AMS_READINGS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

extern uint32_t active_power_watt;

extern char meter_type[16];
extern char meter_id[20];
extern char meter_model[20];
extern uint32_t voltage_l1;
extern uint32_t voltage_l2;
extern uint32_t voltage_l3;
extern int32_t current_l1;
extern int32_t current_l2;
extern int32_t current_l3;

extern uint32_t total_meter_reading;
extern uint32_t meter_offset;
extern uint64_t last_total_reading_timestamp;

// list1 received = active power valid
extern bool list1_recv;
// list2 received = meter ident, voltage and current valid
extern bool list2_recv;
extern bool is_3phase;
// list3 received = total meter reading and time/date valid
extern bool list3_recv;

#ifdef __cplusplus
}
#endif

#endif /* AMS_READINGS_H_ */
