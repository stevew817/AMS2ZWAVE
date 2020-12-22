/*
 * known_lists.h
 *
 *  Created on: Dec 22, 2020
 *      Author: steve
 */

#ifndef AMS_KNOWN_LISTS_H_
#define AMS_KNOWN_LISTS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stddef.h>

/*
 * Known lists is a mapping based on list identifier (e.g. 'AIDON_V0001')
 * mapping data elements to item numbers inside the COSEM structure.
 */
typedef enum {
  END_OF_LIST,
  ACTIVE_POWER_IMPORT,
  METER_ID,
  METER_TYPE,
  CURRENT_L1,
  CURRENT_L2,
  CURRENT_L3,
  VOLTAGE_L1,
  VOLTAGE_L2,
  VOLTAGE_L3,
  TOTAL_DELIVERED_ENERGY
} data_element_t;

typedef struct {
  const data_element_t element;
  const uint8_t cosem_element_offset;
  const int8_t exponent;
} data_element_mapping_t;

typedef enum {
  AIDON_V0001,
  // Insert more known IDs here, and add mapping to known_list_ids_mapping
  UNKNOWN
} known_list_ids_t;

typedef struct {
  const known_list_ids_t list_id_version;
  const size_t total_num_cosem_elements;
  const data_element_mapping_t mappings[];
} known_list_t;

typedef struct {
  const known_list_ids_t list_id_version;
  const size_t list_id_size;
  const char list_id[];
} known_list_ids_mapping_t;

extern const known_list_ids_mapping_t* known_list_ids_mapping[];
extern const known_list_t* list_db[];

#ifdef __cplusplus
}
#endif
#endif /* AMS_KNOWN_LISTS_H_ */
