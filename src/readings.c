/***************************************************************************//**
 * @file readings.h
 * @brief This file contains the HAN meter readout values, updated by main.c
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
#include "readings.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

uint32_t active_power_watt = 0;

char meter_type[16] = {0};
char meter_id[20] = {0};
char meter_model[20] = {0};

// values for voltage and current are in 1/10ths of their unit
uint32_t voltage_l1 = 0;
uint32_t voltage_l2 = 0;
uint32_t voltage_l3 = 0;
int32_t current_l1 = 0;
int32_t current_l2 = 0;
int32_t current_l3 = 0;

uint32_t total_meter_reading = 0;
uint32_t meter_offset = 0;
uint64_t last_total_reading_timestamp = 0;

// list1 received = active power valid
bool list1_recv = false;
// list2 received = meter ident, voltage and current valid
bool list2_recv = false;
bool is_3phase = false;
// list3 received = total meter reading and time/date valid
bool list3_recv = false;

#ifdef __cplusplus
}
#endif
