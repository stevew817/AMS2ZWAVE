#include "readings.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

uint32_t active_power_watt = 0;

char meter_type[16] = {0};
known_list_ids_t meter_type_id = UNKNOWN;
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

list_updated_cb_t list1_updated_cb = NULL;
list_updated_cb_t list2_updated_cb = NULL;
list_updated_cb_t list3_updated_cb = NULL;
list_updated_cb_t metertype_updated_cb = NULL;

#ifdef __cplusplus
}
#endif
