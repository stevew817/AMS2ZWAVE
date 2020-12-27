/***************************************************************************//**
 * @file known_lists.h
 * @brief This file contains the HAN parser 'known list' data types
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
 * mapping data elements to item numbers inside the COSEM structure of a specific list.
 */

/**
 * List of all known data potentially sent in a HAN message
 */
typedef enum {
  END_OF_LIST,            /** Marker to signify end of data mapping list */
  ACTIVE_POWER_IMPORT,    /** Active power in import direction, unit Watt, unsigned integer */
  ACTIVE_POWER_EXPORT,    /** Active power in export direction, unit Watt, unsigned integer */
  REACTIVE_POWER_IMPORT,  /** Reactive power in import direction, unit Watt, unsigned integer */
  REACTIVE_POWER_EXPORT,  /** Reactive power in export direction, unit Watt, unsigned integer */
  METER_ID,               /** Meter ID (GSIN), ASCII */
  METER_TYPE,             /** Meter type (model number), ASCII */
  CURRENT_L1,             /** Current supplied on phase 1, unit milli-Ampere, signed integer */
  CURRENT_L2,             /** Current supplied on phase 2, unit milli-Ampere, signed integer (three-phase meter only) */
  CURRENT_L3,             /** Current supplied on phase 3, unit milli-Ampere, signed integer (three-phase meter only) */
  VOLTAGE_L1,             /** Voltage supplied on phase 1, unit Volt, unsigned integer */
  VOLTAGE_L2,             /** Voltage supplied on phase 2, unit Volt, unsigned integer (three-phase meter only) */
  VOLTAGE_L3,             /** Voltage supplied on phase 3, unit Volt, unsigned integer (three-phase meter only) */
  ACTIVE_ENERGY_IMPORT,   /** Accumulated energy in import direction over meter's lifetime (= 'meter reading'), unit Wh, unsigned integer */
  ACTIVE_ENERGY_EXPORT,   /** Accumulated energy in export direction over meter's lifetime (= 'meter reading'), unit Wh, unsigned integer */
  REACTIVE_ENERGY_IMPORT, /** Accumulated reactive energy in import direction over meter's lifetime, unit VArh, unsigned integer */
  REACTIVE_ENERGY_EXPORT, /** Accumulated reactive energy in export direction over meter's lifetime, unit VArh, unsigned integer */
  DATE_TIME,              /** Timestamp for when the report was generated. COSEM date-time serialized format (12 bytes) */
} ams_data_element_t;

/**
 * Mapping object to locate a datapoint in a HAN message
 */
typedef struct {
  const ams_data_element_t element;     /** Data field mapped */
  const uint8_t cosem_element_offset;   /** Offset ('n-th entry') in the flattened cosem list wherer the data field is */
  const int8_t exponent;                /** Exponent (datapoint in our scale = raw value * 10 ^ exponent) */
} ams_data_element_mapping_t;

typedef enum {
#ifndef DISABLE_AIDON
  AIDON_V0001,
#endif // DISABLE_AIDON
#ifndef DISABLE_KAMSTRUP
  Kamstrup_V0001,
#endif // DISABLE_KAMSTRUP
#ifndef DISABLE_KAIFA
  KFM_001,
#endif // DISABLE_KAIFA
  // Insert more known IDs here, and add mapping to known_list_ids_mapping
  UNKNOWN /** End marker for mappings list */
} ams_known_list_ids_t;

/**
 * Description of a known message, corresponding to a known list ID.
 */
typedef struct {
  const size_t total_num_cosem_elements;        /** Amount of COSEM elements in this known message (flattened, so don't count list or array) */
  const ams_data_element_mapping_t mappings[];  /** List of known data elements that can be extracted from this COSEM, ending in \ref END_OF_LIST */
} ams_known_list_t;

/**
 * Description of a known list ID, mapping our enum to the actual ASCII list ID, as well as a list of known messages for that list ID.
 */
typedef struct {
  const ams_known_list_ids_t list_id_version;
  const size_t list_id_size;                        /** number of bytes in \ref list_id */
  const char list_id[16];                           /** ASCII value of list ID in COSEM message that \ref list_id_version corresponds to */
  const ams_known_list_t* known_message_formats[];  /** List of message formats known for this list ID, ending with a NULL pointer */
} ams_known_list_ids_mapping_t;

// The actual list mappings are inside known_lists.c
/** list of known list IDs, ending in an entry with list ID 'UNKNOWN' */
extern const ams_known_list_ids_mapping_t* ams_known_list_ids_mapping[];

#ifdef __cplusplus
}
#endif

#endif /* AMS_KNOWN_LISTS_H_ */
