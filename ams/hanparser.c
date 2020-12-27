/***************************************************************************//**
 * @file hanparser.c
 * @brief This file contains the HAN parser
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

#include "hanparser.h"
#include "readings.h"
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "ams/hanparser_platform.h"
#include "ams/known_lists.h"

#define DPRINT  han_parser_debug_print
#define DPRINTF han_parser_debug_printf

/**** Trying to include some documentation
 *
 * There are three types of meters currently on the Norwegian market:
 * * Aidon
 * * Kamstrup
 * * Kaifa
 *
 * Each of those claims to adhere to the Norwegian HAN specification, but all
 * three have interpreted the binary format differently.
 *
 * Aidon:
 * * Includes OBIS codes in the streamed messages
 * * Includes format/scalar information for each register value
 * * Encapsulates each OBIS entry in list/struct
 * * Does not have time/date in header
 *
 * Kamstrup:
 * * Includes OBIS codes in the streamed messages
 * * Does not include format/scalar information
 * * Does not encapsulate (=> list is flat)
 * * Issues a combined list1/list2 at 10 second intervals
 * * Has time/date in extended header
 *
 * Kaifa:
 * * Does not include OBIS codes
 * * Declares COSEM hex strings even though the content is ascii
 * * Does not encapsulate (=> list is flat)
 * * Has time/date in extended header
 *
 * So, what can we parse through autodetection?
 * -> wait for the first list2, since that contains the list information
 * -> Search for a known list format in list2 if the meter type is unknown
 */

#ifdef __cplusplus
extern "C"
{
#endif

#define HDLC_FLAG 0x7E

static uint8_t input_buffer[1024];
static size_t input_buffer_pos = 0;
static size_t hdlc_length = sizeof(input_buffer);
static size_t dst_addr_size = 0;
static size_t src_addr_size = 0;
static size_t hcs_offset = 0;

void reset_parser( char* msg )
{
  input_buffer_pos = 0;
  hdlc_length = sizeof(input_buffer);
  dst_addr_size = 0;
  src_addr_size = 0;
  hcs_offset = 0;

  if(msg != NULL) {
      DPRINT("Parser reset with msg: ");
      DPRINT(msg);
      DPRINT("\n");
  } else {
      DPRINT("Parser reset \n");
  }
}

bool find_array_in_array(const uint8_t* haystack, size_t haystack_size, const uint8_t* needle, size_t needle_size)
{
  size_t haypos, needlepos;
  if(needle_size > haystack_size)
    return false;

  for(haypos = 0; haypos < haystack_size - needle_size; haypos++) {
      for(needlepos = 0; needlepos < needle_size; needlepos++) {
          if(haystack[haypos + needlepos] != needle[needlepos])
            break;
      }
      if(needlepos == needle_size) {
          return true;
      }
  }
  return false;
}

#define EAT_BYTES(x) do {if(x > bytes) {reset_parser("COSEM parser error\n"); return false;} bytes -= x; start += x;} while(0)
#define PRINT_LEVEL do { for(size_t level_indent = 0; level_indent < current_level * 2; level_indent++) DPRINT(" "); } while(0)

bool get_element_by_sequence_number(uint8_t* start, size_t bytes, size_t seqnum, uint8_t** item_ptr, size_t* item_size)
{
  size_t item_counter = 1;
  while(bytes > 0) {
    switch(start[0]) {
      case 0x01:
        EAT_BYTES(2);
        break;
      case 0x02:
        EAT_BYTES(2);
        break;
      case 0x09:
        if(item_counter == seqnum) {
            *item_ptr = &start[2];
            *item_size = start[1];
            return true;
        }
        item_counter++;
        EAT_BYTES(2 + start[1]);
        break;
      case 0x0a:
        if(item_counter == seqnum) {
            *item_ptr = &start[2];
            *item_size = start[1];
            return true;
        }
        item_counter++;
        EAT_BYTES(2 + start[1]);
        break;
      case 0x10:
        if(item_counter == seqnum) {
            *item_ptr = &start[1];
            *item_size = 2;
            return true;
        }
        item_counter++;
        EAT_BYTES(3);
        break;
      case 0x12:
        if(item_counter == seqnum) {
            *item_ptr = &start[1];
            *item_size = 2;
            return true;
        }
        item_counter++;
        EAT_BYTES(3);
        break;
      case 0x06:
        if(item_counter == seqnum) {
            *item_ptr = &start[1];
            *item_size = 4;
            return true;
        }
        item_counter++;
        EAT_BYTES(5);
        break;
      case 0x0F:
        if(item_counter == seqnum) {
            *item_ptr = &start[1];
            *item_size = 1;
            return true;
        }
        item_counter++;
        EAT_BYTES(2);
        break;
      case 0x16:
        if(item_counter == seqnum) {
            *item_ptr = &start[1];
            *item_size = 1;
            return true;
        }
        item_counter++;
        EAT_BYTES(2);
        break;
      default:
        reset_parser("Unknown COSEM\n");
        return false;
    }
  }
  return false;
}

bool parse_cosem(uint8_t* array, size_t array_bytes)
{
  size_t items_in_level[10] = {0};
  size_t current_level = 0;
  size_t item_counter = 0;
  uint8_t* start = array;
  size_t bytes = array_bytes;

  // remainder is COSEM payload
  DPRINTF("COSEM payload (%d bytes):\n ", bytes);
  for(size_t i = 0; i < bytes; i++) {
      DPRINTF(" %02x", start[i]);
  }
  DPRINT("\n");

  if(start[0] == 0x09 && start[1] == 0x0C) {
      EAT_BYTES(1);
  }
  DPRINTF("Extended header size: %d\n ", start[0]);
  if(start[0] > 0) {
    for(size_t i = 0; i < start[0]; i++) {
        DPRINTF(" %02x", start[1+i]);
    }
  }
  DPRINT("\n");
  item_counter++;
  EAT_BYTES(1 + start[0]);
  // throw away header information
  array = start;
  array_bytes = bytes;

  DPRINT("=============================================\n");
  // parser setup: for each started element, handle its content, then increase
  // 'start' and decrease 'bytes' with the amount of bytes read
  while(bytes > 0) {
      bool level_increased = false;
      switch(start[0]) {
        case 0x01:
          PRINT_LEVEL;
          DPRINTF("Start of array with %d items:\n", start[1]);
          items_in_level[++current_level] = start[1];
          level_increased = true;
          EAT_BYTES(2);
          break;
        case 0x02:
          PRINT_LEVEL;
          DPRINTF("Start of struct with %d items:\n", start[1]);
          items_in_level[++current_level] = start[1];
          level_increased = true;
          EAT_BYTES(2);
          break;
        case 0x09:
          PRINT_LEVEL;
          DPRINTF("(%d) hex-string: ", item_counter);
          for(size_t i = 0; i < start[1]; i++) {
              DPRINTF(" %02x", start[2+i]);
          }
          DPRINT("\n");
          item_counter++;
          EAT_BYTES(2 + start[1]);
          break;
        case 0x0a:
          PRINT_LEVEL;
          DPRINTF("(%d) ascii-string: ", item_counter);
          for(size_t i = 0; i < start[1]; i++) {
              DPRINTF("%c", start[2+i]);
          }
          DPRINT("\n");
          item_counter++;
          EAT_BYTES(2 + start[1]);
          break;
        case 0x10:
          PRINT_LEVEL;
          DPRINTF("(%d) int16: %d\n", item_counter, (int16_t)(start[1] << 8 | start[2]));
          item_counter++;
          EAT_BYTES(3);
          break;
        case 0x12:
          PRINT_LEVEL;
          DPRINTF("(%d) uint16: %u\n", item_counter, (uint16_t)((start[1] << 8) | start[2]));
          item_counter++;
          EAT_BYTES(3);
          break;
        case 0x06:
          PRINT_LEVEL;
          DPRINTF("(%d) uint32: %u\n", item_counter, (uint32_t)((start[1] << 24) | (start[2] << 16) | (start[3] << 8) | start[4]));
          item_counter++;
          EAT_BYTES(5);
          break;
        case 0x0F:
          PRINT_LEVEL;
          DPRINTF("(%d) int8: %d\n", item_counter, (int8_t)(start[1]));
          item_counter++;
          EAT_BYTES(2);
          break;
        case 0x16:
          PRINT_LEVEL;
          DPRINTF("(%d) enum: %u\n", item_counter, start[1]);
          item_counter++;
          EAT_BYTES(2);
          break;
        default:
          reset_parser("Unknown COSEM\n");
          return false;
      }
      if(!level_increased && current_level > 0) {
          while(current_level > 0 && --items_in_level[current_level] == 0) {
              current_level--;
          }
      }
  }
  DPRINT("=============================================\n");

  DPRINTF("Total items: %d\n", --item_counter);

  const ams_known_list_ids_mapping_t* detected_mapping = NULL;

  if(meter_type_id != UNKNOWN && item_counter > 4) {
      // have a meter type from before, check it is consistent
      if(!find_array_in_array(array, array_bytes, (const uint8_t*)meter_type, strlen(meter_type))) {
          memset(meter_type, 0, sizeof(meter_type));
          meter_type_id = UNKNOWN;
          reset_parser("Invalid list identifier, resetting meter type\n");
          return false;
      } else {
        size_t mapping_it = 0;
        while(ams_known_list_ids_mapping[mapping_it] != NULL) {
          if(ams_known_list_ids_mapping[mapping_it]->list_id_version == meter_type_id) {
            detected_mapping = ams_known_list_ids_mapping[mapping_it];
            break;
          }
        }

        if(detected_mapping == NULL) {
          meter_type_id = UNKNOWN;
          reset_parser("Unknown meter type, resetting meter type\n");
          return false;
        }
      }
  } else {
    // try to auto-detect meter type
    if(item_counter > 4) {
        size_t mapping_it = 0;
        while(ams_known_list_ids_mapping[mapping_it] != NULL) {
            if(find_array_in_array(array, array_bytes, (const uint8_t*)ams_known_list_ids_mapping[mapping_it]->list_id, ams_known_list_ids_mapping[mapping_it]->list_id_size)) {
                DPRINTF("Found meter type %s\n", ams_known_list_ids_mapping[mapping_it]->list_id);

                // TODO: anything else to reset upon meter type detection?
                meter_type_id = ams_known_list_ids_mapping[mapping_it]->list_id_version;
                memcpy(meter_type, ams_known_list_ids_mapping[mapping_it]->list_id, ams_known_list_ids_mapping[mapping_it]->list_id_size);
                meter_type[ams_known_list_ids_mapping[mapping_it]->list_id_size] = 0;
                break;
            }
            mapping_it++;
        }
        if(meter_type_id == UNKNOWN) {
            reset_parser("Could not auto-detect meter type\n");
            return false;
        }
    }
  }

  uint8_t* temp_item_ptr;
  size_t temp_item_size;
  bool item_found = false;

  if(item_counter == 1) {
      // single item message, assume it is reactive import power
      item_found = get_element_by_sequence_number(array, array_bytes, 1, &temp_item_ptr, &temp_item_size);
      if(item_found && temp_item_size == 4) {
          active_power_watt = (temp_item_ptr[0] << 24) | (temp_item_ptr[1] << 16) | (temp_item_ptr[2] << 8) | (temp_item_ptr[3] << 0);
          DPRINTF("Active power: %u watt\n", active_power_watt);
          list1_recv = true;
          if(list1_updated_cb != NULL) {
              list1_updated_cb();
          }
          return true;
      }
      return false;
  } else if(item_counter == 4) {
      // assume it's an Aidon single item message with reactive import power
      item_found = get_element_by_sequence_number(array, array_bytes, 2, &temp_item_ptr, &temp_item_size);
      if(item_found && temp_item_size == 4) {
          active_power_watt = (temp_item_ptr[0] << 24) | (temp_item_ptr[1] << 16) | (temp_item_ptr[2] << 8) | (temp_item_ptr[3] << 0);
          DPRINTF("Active power: %u watt\n", active_power_watt);
          list1_recv = true;
          if(list1_updated_cb != NULL) {
              list1_updated_cb();
          }
          return true;
      }
      return false;
  } else if(item_counter > 4) {
      // find a defined list based in meter type
      size_t list_it = 0;
      const ams_known_list_t* detected_list = NULL;
      while(detected_mapping->known_message_formats[list_it] != NULL) {
          if(detected_mapping->known_message_formats[list_it]->total_num_cosem_elements == item_counter) {
              DPRINT("Found matching list\n");
              detected_list = detected_mapping->known_message_formats[list_it];
              break;
          }
          list_it++;
      }

      if(detected_list == NULL) {
          reset_parser("Could not find matching list\n");
          return false;
      }

      // Go through all the mappings
      bool have_accumulated_consumption = false;

      size_t mapping_it = 0;
      while(detected_list->mappings[mapping_it].element != END_OF_LIST) {
          item_found = get_element_by_sequence_number(array, array_bytes, detected_list->mappings[mapping_it].cosem_element_offset, &temp_item_ptr, &temp_item_size);
          if(!item_found) {
              reset_parser("Could not find item from predefined list :/\n");
              return false;
          }

          switch(detected_list->mappings[mapping_it].element) {
            case ACTIVE_POWER_IMPORT:
              active_power_watt = (temp_item_ptr[0] << 24) | (temp_item_ptr[1] << 16) | (temp_item_ptr[2] << 8) | (temp_item_ptr[3] << 0);
              list1_recv = true;
              DPRINTF("Active power: %u watt\n", active_power_watt);
              break;
            case METER_ID:
              //TODO: trigger something on change of meter ID?
              memcpy(meter_id, temp_item_ptr, temp_item_size);
              memset(&meter_id[temp_item_size], 0, sizeof(meter_id) - temp_item_size);
              DPRINT("Meter ID: ");
              for(size_t i = 0; i < temp_item_size; i++) {
                  DPRINTF("%c", meter_id[i]);
              }
              DPRINT("\n");
              list2_recv = true;
              break;
            case METER_TYPE:
              memcpy(meter_model, temp_item_ptr, temp_item_size);
              memset(&meter_model[temp_item_size], 0, sizeof(meter_model) - temp_item_size);
              DPRINT("Meter model: ");
              for(size_t i = 0; i < temp_item_size; i++) {
                  DPRINTF("%c", meter_model[i]);
              }
              DPRINT("\n");
              list2_recv = true;
              break;
            case CURRENT_L1:
              current_l1 = ((int32_t)((int8_t)temp_item_ptr[0]) << 8) | (temp_item_ptr[1] << 0);
              if(detected_list->mappings[mapping_it].exponent == -1) {
                  current_l1 /= 10;
              }
              DPRINTF("Current L1 %d A\n", current_l1);
              list2_recv = true;
              break;
            case CURRENT_L2:
              current_l2 = ((int32_t)((int8_t)temp_item_ptr[0]) << 8) | (temp_item_ptr[1] << 0);
              if(detected_list->mappings[mapping_it].exponent == -1) {
                  current_l2 /= 10;
              }
              DPRINTF("Current L2 %d A\n", current_l2);
              list2_recv = true;
              break;
            case CURRENT_L3:
              current_l3 = ((int32_t)((int8_t)temp_item_ptr[0]) << 8) | (temp_item_ptr[1] << 0);
              if(detected_list->mappings[mapping_it].exponent == -1) {
                  current_l3 /= 10;
              }
              DPRINTF("Current L3 %d A\n", current_l3);
              list2_recv = true;
              break;
            case VOLTAGE_L1:
              voltage_l1 = (temp_item_ptr[0] << 8) | (temp_item_ptr[1] << 0);
              if(detected_list->mappings[mapping_it].exponent == -1) {
                  voltage_l1 /= 10;
              }
              DPRINTF("Voltage L1 %d V\n", voltage_l1);
              list2_recv = true;
              break;
            case VOLTAGE_L2:
              voltage_l2 = (temp_item_ptr[0] << 8) | (temp_item_ptr[1] << 0);
              if(detected_list->mappings[mapping_it].exponent == -1) {
                  voltage_l2 /= 10;
              }
              DPRINTF("Voltage L2 %d V\n", voltage_l2);
              list2_recv = true;
              break;
            case VOLTAGE_L3:
              voltage_l3 = (temp_item_ptr[0] << 8) | (temp_item_ptr[1] << 0);
              if(detected_list->mappings[mapping_it].exponent == -1) {
                  voltage_l3 /= 10;
              }
              DPRINTF("Voltage L3 %d V\n", voltage_l3);
              list2_recv = true;
              break;
              break;
            case ACTIVE_ENERGY_IMPORT:
              have_accumulated_consumption = true;
              total_meter_reading = (temp_item_ptr[0] << 24) | (temp_item_ptr[1] << 16) | (temp_item_ptr[2] << 8) | (temp_item_ptr[3] << 0);
              if(detected_list->mappings[mapping_it].exponent == 1) {
                  total_meter_reading *= 10;
              }
              DPRINTF("Total meter reading %d Wh\n", total_meter_reading);
              list3_recv = true;
              break;
            default:
              break;
          }
          mapping_it++;
      }

      if(have_accumulated_consumption && list3_updated_cb != NULL) {
          list3_updated_cb();
      } else if(!have_accumulated_consumption && list2_updated_cb != NULL) {
          list2_updated_cb();
      }
  }

  return false;
}

void parse_msdu(uint8_t* start, size_t bytes)
{
  if(bytes < 8)
    return reset_parser("Too little data in the MSDU\n");

  DPRINT("Parsing MSDU\n");

  DPRINTF("llc_dst_svc_ap: %02x\n", start[0]);
  DPRINTF("llc_src_svc_ap: %02x\n", start[1]);
  DPRINTF("llc_control: %02x\n", start[2]);

  DPRINTF("apdu_tag: %02x\n", start[3]);
  DPRINT("apdu_invoke_id_and_priority: ");
  for(size_t i = 0; i < 4; i++) {
      DPRINTF(" %02x", start[4+i]);
  }
  DPRINT("\n");

  parse_cosem(&start[8], bytes - 8);
}

void input_byte(uint8_t byte)
{
  // go through the processing loop...

  // if we don't have any input, only the flag byte is valid
  if(input_buffer_pos == 0 && byte != HDLC_FLAG) {
      return;
  }

  // if we detect multiple subsequent 0x7E it may be a stop-and-start case
  if(input_buffer_pos == 1 && byte == HDLC_FLAG) {
      return;
  }

  // if we have input, add the input byte to the buffer
  if(input_buffer_pos < sizeof(input_buffer)) {
      input_buffer[input_buffer_pos] = byte;
      input_buffer_pos++;
  } else {
      // overflowed, reset and wait for next flag
      input_buffer_pos = 0;
      return reset_parser("buffer overflowed");
  }

  // what happens during processing depends on how far along we are in the message
  // pos == 3 -> received flag (0) and length (1 and 2)
  if(input_buffer_pos == 3) {
      // check format type == 3 and no segmentation
      if((input_buffer[1] & 0xF8) != 0xA0)
        return reset_parser("wrong frame type");
      // try to parse length
      hdlc_length = ((input_buffer[1] & 0x07) << 8) | input_buffer[2];

      // make sure length is sane
      if(hdlc_length > sizeof(input_buffer) - 2)
        return reset_parser("length would overflow buffer");

      // start parsing dst addr
      dst_addr_size = 1;
  }
  // parse dst address until uneven
  else if(input_buffer_pos == 3 + dst_addr_size && src_addr_size == 0) {
      // is address even? extend until 4
      if((byte & 0x1) == 0) {
          switch(dst_addr_size) {
            case 1:
              dst_addr_size = 2;
              break;
            case 2:
              dst_addr_size = 4;
              break;
            default:
              return reset_parser("DST address not valid\n");
          }
      } else {
          // start looking for source address
          src_addr_size = 1;
      }
  }
  // parse src address until uneven
  else if(input_buffer_pos == 3 + dst_addr_size + src_addr_size && hcs_offset == 0) {
      // is address even? extend until 4
      if((byte & 0x1) == 0) {
          switch(src_addr_size) {
            case 1:
              src_addr_size = 2;
              break;
            case 2:
              src_addr_size = 4;
              break;
            default:
              return reset_parser("SRC address not valid\n");
          }
      } else {
          // found HCS offset
          hcs_offset = 1 + 2 + dst_addr_size + src_addr_size + 1;
      }
  }
  // Check HCS as it comes in
  else if(input_buffer_pos == hcs_offset + 2 + 1) {
      if(!han_parser_check_crc16_x25(&input_buffer[1], 2 + dst_addr_size + src_addr_size + 1))
          return reset_parser("invalid HCS CRC");
      else
        DPRINT("HCS checks out\n");
  }
  // Check CRC and parse packet on close
  else if(input_buffer_pos == hdlc_length + 2) {
      // we have the full package plus the two flag bytes
      // check final flag byte
      if(input_buffer[1 + hdlc_length] != HDLC_FLAG)
        return reset_parser("no flag after declared packet length");

      // check FCS
      if(!han_parser_check_crc16_x25(&input_buffer[1], hdlc_length - 2)) {
          //return reset_parser("invalid FCS CRC");
      }

      // parse contents
      parse_msdu(&input_buffer[hcs_offset + 2], hdlc_length - 2 - 1 - 2 - dst_addr_size - src_addr_size - 2);
      reset_parser(NULL);
      input_buffer_pos = 1; // allow restarted communication (shared flag for end and start)
  }
}

#ifdef __cplusplus
}
#endif
