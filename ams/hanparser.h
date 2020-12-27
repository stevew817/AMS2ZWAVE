/***************************************************************************//**
 * @file hanparser.h
 * @brief This file describes the HAN parser interface
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

#ifndef AMS_HANPARSER_H_
#define AMS_HANPARSER_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    const char* meter_gsin;         /** Null-terminated meter GSIN, ASCII */
    const char* meter_model;        /** Null-terminated meter model, ASCII */

    uint32_t active_power_import;   /** Active power in import direction, Watt */
    uint32_t active_power_export;   /** Active power in export direction, Watt */
    uint32_t reactive_power_import; /** Reactive power in import direction, VAr */
    uint32_t reactive_power_export; /** Reactive power in export direction, VAr */

    uint32_t voltage_l1;            /** Line/phase voltage for L1, Volt */
    uint32_t voltage_l2;            /** Line/phase voltage for L2, Volt (3-phase only) */
    uint32_t voltage_l3;            /** Line/phase voltage for L3, Volt (3-phase only) */

    int32_t current_l1;             /** Current flowing through L1, milli-Ampere */
    int32_t current_l2;             /** Current flowing through L2, milli-Ampere */
    int32_t current_l3;             /** Current flowing through L3, milli-Ampere */

    uint32_t active_energy_import;  /** Accumulated active energy in import direction, Wh */
    uint32_t active_energy_export;  /** Accumulated active energy in export direction, Wh */
    uint32_t reactive_energy_import;/** Accumulated reactive energy in import direction, VArh */
    uint32_t reactive_energy_export;/** Accumulated reactive energy in export direction, VArh */

    bool is_3p;             /** If true: values for three phases are present, else only l1 */
    bool has_power_data;    /** If true: value for active_power_import is valid */
    bool has_meter_data;    /** If true: values for meter_* are valid */
    bool has_line_data;     /** If true: values for (re)active_power_*, voltage_lx and current_lx are valid */
    bool has_energy_data;   /** If true: values for (re)active_energy_* are valid */
} han_parser_data_t;

/**
 * Callback function type.
 * decoded_data points to data extracted from the successfully decoded message.
 * When the callback function returns, the data structure is no longer considered valid.
 */
typedef void (*han_parser_message_decoded_cb_t)(const han_parser_data_t* decoded_data);

/**
 * Parse data received on HAN interface, one byte at a time.
 */
void han_parser_input_byte(uint8_t byte);

/**
 * Set callback for successfully parsed packets.
 */
void han_parser_set_callback(han_parser_message_decoded_cb_t func);

#ifdef __cplusplus
}
#endif

#endif /* AMS_HANPARSER_H_ */
