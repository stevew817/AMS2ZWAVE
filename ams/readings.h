/*
 * readings.h
 *
 *  Created on: Dec 21, 2020
 *      Author: steve
 */

#ifndef AMS_READINGS_H_
#define AMS_READINGS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "ams/known_lists.h"

extern uint32_t active_power_watt;

extern char meter_type[16];
extern known_list_ids_t meter_type_id;
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

typedef void (*list_updated_cb_t)(void);
extern list_updated_cb_t list1_updated_cb;
extern list_updated_cb_t list2_updated_cb;
extern list_updated_cb_t list3_updated_cb;
extern list_updated_cb_t metertype_updated_cb;

#ifdef __cplusplus
}
#endif

#endif /* AMS_READINGS_H_ */
