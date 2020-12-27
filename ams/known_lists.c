/***************************************************************************//**
 * @file known_lists.c
 * @brief This file contains the HAN parser's known messages description
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

#include "known_lists.h"

#ifndef DISABLE_AIDON
const ams_known_list_t aidon_list1 = {
  // Aidon list1
  4,
  {
      {ACTIVE_POWER_IMPORT, 2, 0}
  }
};

const ams_known_list_t aidon_list2_it = {
  // Aidon list2 for 3phase IT
  /* 0202 0906 0101000281ff 0a0b 4149444f4e5f5630303031 -> list ID
   * 0202 0906 0000600100ff 0a10 3733XXXXXXXXXXXXXXXXXXXXXXX13130 -> meter ID (ASCII)
   * 0202 0906 0000600107ff 0a04 36353235 -> meter type (ASCII)
   * 0203 0906 0100010700ff 06 00000e90 0202 0f00 161b -> active power import, kW 4.3
   * 0203 0906 0100020700ff 06 00000000 0202 0f00 161b -> active power export, kW 4.3
   * 0203 0906 0100030700ff 06 0000001c 0202 0f00 161d -> reactive power import, kW 4.3
   * 0203 0906 0100040700ff 06 00000000 0202 0f00 161d -> reactive power export, kW 4.3
   * 0203 0906 01001f0700ff 10 0091     0202 0fff 1621 -> current l1
   * 0203 0906 0100470700ff 10 0090     0202 0fff 1621 -> current l3
   * 0203 0906 0100200700ff 12 0932     0202 0fff 1623 -> voltage l1
   * 0203 0906 0100340700ff 12 091e     0202 0fff 1623 -> voltage l2
   * 0203 0906 0100480700ff 12 0933     0202 0fff 1623 -> voltage l3
   */
  42,
  {
    {METER_ID, 4, 0},
    {METER_TYPE, 6, 0},
    {ACTIVE_POWER_IMPORT, 8, 0},
    {ACTIVE_POWER_EXPORT, 12, 0},
    {REACTIVE_POWER_IMPORT, 16, 0},
    {REACTIVE_POWER_EXPORT, 20, 0},
    {CURRENT_L1, 24, 2},
    {CURRENT_L3, 28, 2},
    {VOLTAGE_L1, 32, -1},
    {VOLTAGE_L2, 36, -1},
    {VOLTAGE_L3, 40, -1},
    {END_OF_LIST, 0, 0}
  }
};

const ams_known_list_t aidon_list2_tn = {
  // Aidon list2 for 3phase TN
  46,
  {
    {METER_ID, 4, 0},
    {METER_TYPE, 6, 0},
    {ACTIVE_POWER_IMPORT, 8, 0},
    {ACTIVE_POWER_EXPORT, 12, 0},
    {REACTIVE_POWER_IMPORT, 16, 0},
    {REACTIVE_POWER_EXPORT, 20, 0},
    {CURRENT_L1, 24, 2},
    {CURRENT_L2, 28, 2},
    {CURRENT_L3, 32, 2},
    {VOLTAGE_L1, 36, -1},
    {VOLTAGE_L2, 40, -1},
    {VOLTAGE_L3, 44, -1},
    {END_OF_LIST, 0, 0}
  }
};

const ams_known_list_t aidon_list2_1p = {
  // Aidon list2 for 1phase
  30,
  {
    {METER_ID, 4, 0},
    {METER_TYPE, 6, 0},
    {ACTIVE_POWER_IMPORT, 8, 0},
    {ACTIVE_POWER_EXPORT, 12, 0},
    {REACTIVE_POWER_IMPORT, 16, 0},
    {REACTIVE_POWER_EXPORT, 20, 0},
    {CURRENT_L1, 24, 2},
    {VOLTAGE_L1, 28, -1},
    {END_OF_LIST, 0, 0}
  }
};

const ams_known_list_t aidon_list3_it = {
  // Aidon list3 for 3phase IT
  /* 0202 0906 0101000281ff 0a0b 4149444f4e5f5630303031 -> list ID
   * 0202 0906 0000600100ff 0a10 3733XXXXXXXXXXXXXXXXXXXXXXX13130 -> meter ID (ASCII)
   * 0202 0906 0000600107ff 0a04 36353235 -> meter type (ASCII)
   * 0203 0906 0100010700ff 06 00000e90 0202 0f00 161b -> active power import
   * 0203 0906 0100020700ff 06 00000000 0202 0f00 161b -> active power export
   * 0203 0906 0100030700ff 06 0000001c 0202 0f00 161d -> reactive power import
   * 0203 0906 0100040700ff 06 00000000 0202 0f00 161d -> reactive power export
   * 0203 0906 01001f0700ff 10 0091     0202 0fff 1621 -> current l1
   * 0203 0906 0100470700ff 10 0090     0202 0fff 1621 -> current l3
   * 0203 0906 0100200700ff 12 0932     0202 0fff 1623 -> voltage l1
   * 0203 0906 0100340700ff 12 091e     0202 0fff 1623 -> voltage l2
   * 0203 0906 0100480700ff 12 0933     0202 0fff 1623 -> voltage l3
   * 0202 0906 0000010000ff 090c 07e3 06 0c 03 17 00 00 ff 003c 00 -> timestamp
   * 0203 0906 0100010800ff 06 0021684d 0202 0f01 161e -> cumulative import
   * 0203 0906 0100020800ff 06 00000000 0202 0f01 161e -> cumulative export
   * 0203 0906 0100030800ff 06 00008251 0202 0f01 1620 -> cumulative reactive import
   * 0203 0906 0100040800ff 06 00011ba5 0202 0f01 1620 -> cumulative reactive export
   */
  60,
  {
    {METER_ID, 4, 0},
    {METER_TYPE, 6, 0},
    {ACTIVE_POWER_IMPORT, 8, 0},
    {ACTIVE_POWER_EXPORT, 12, 0},
    {REACTIVE_POWER_IMPORT, 16, 0},
    {REACTIVE_POWER_EXPORT, 20, 0},
    {CURRENT_L1, 24, 2},
    {CURRENT_L3, 28, 2},
    {VOLTAGE_L1, 32, -1},
    {VOLTAGE_L2, 36, -1},
    {VOLTAGE_L3, 40, -1},
    {DATE_TIME, 44, 0},
    {ACTIVE_ENERGY_IMPORT, 46, 1},
    {ACTIVE_ENERGY_EXPORT, 50, 1},
    {REACTIVE_ENERGY_IMPORT, 54, 1},
    {REACTIVE_ENERGY_EXPORT, 58, 1},
    {END_OF_LIST, 0, 0}
  }
};

const ams_known_list_t aidon_list3_tn = {
  // Aidon list3 for 3phase TN
  64,
  {
    {METER_ID, 4, 0},
    {METER_TYPE, 6, 0},
    {ACTIVE_POWER_IMPORT, 8, 0},
    {ACTIVE_POWER_EXPORT, 12, 0},
    {REACTIVE_POWER_IMPORT, 16, 0},
    {REACTIVE_POWER_EXPORT, 20, 0},
    {CURRENT_L1, 24, 2},
    {CURRENT_L2, 28, 2},
    {CURRENT_L3, 32, 2},
    {VOLTAGE_L1, 36, -1},
    {VOLTAGE_L2, 40, -1},
    {VOLTAGE_L3, 44, -1},
    {DATE_TIME, 48, 0},
    {ACTIVE_ENERGY_IMPORT, 50, 1},
    {ACTIVE_ENERGY_EXPORT, 54, 1},
    {REACTIVE_ENERGY_IMPORT, 58, 1},
    {REACTIVE_ENERGY_EXPORT, 62, 1},
    {END_OF_LIST, 0, 0}
  }
};

const ams_known_list_t aidon_list3_1p = {
  //Aidon list3 for 1phase
  48,
  {
    {METER_ID, 4, 0},
    {METER_TYPE, 6, 0},
    {ACTIVE_POWER_IMPORT, 8, 0},
    {ACTIVE_POWER_EXPORT, 12, 0},
    {REACTIVE_POWER_IMPORT, 16, 0},
    {REACTIVE_POWER_EXPORT, 20, 0},
    {CURRENT_L1, 24, 2},
    {VOLTAGE_L1, 28, -1},
    {DATE_TIME, 32, 0},
    {ACTIVE_ENERGY_IMPORT, 34, 1},
    {ACTIVE_ENERGY_EXPORT, 38, 1},
    {REACTIVE_ENERGY_IMPORT, 42, 1},
    {REACTIVE_ENERGY_EXPORT, 46, 1},
    {END_OF_LIST, 0}
  }
};

const ams_known_list_ids_mapping_t aidon_v0001_mapping = {
  AIDON_V0001,
  sizeof("AIDON_V0001") - 1,
  "AIDON_V0001",
  {
    &aidon_list1,
    &aidon_list2_it,
    &aidon_list2_tn,
    &aidon_list2_1p,
    &aidon_list3_it,
    &aidon_list3_tn,
    &aidon_list3_1p,
    NULL
  }
};
#endif //DISABLE_AIDON

#ifndef DISABLE_KAMSTRUP
const ams_known_list_t kamstrup_list2_3p = {
  // Kamstrup list2 for 3phase TN
  /* 0A0E 4B616D73747275705F5630303031 -> list ID
   * 0906 0101000005FF 0A10 35373036353637303030303030303030 -> meter GSIN
   * 0906 0101600101FF 0A12 303030303030303030303030303030303030 -> meter model
   * 0906 0101010700FF 0600000000 -> active power import, kW 4.3
   * 0906 0101020700FF 0600000000 -> active power export, kW 4.3
   * 0906 0101030700FF 0600000000 -> reactive power import, kVAr 4.3
   * 0906 0101040700FF 0600000000 -> reactive power export, kVAr 4.3
   * 0906 01011F0700FF 0600000000 -> 1sec RMS current L1, A 3.2
   * 0906 0101330700FF 0600000000 -> 1sec RMS current L2, A 3.2
   * 0906 0101470700FF 0600000000 -> 1sec RMS current L3, A 3.2
   * 0906 0101200700FF 120000 -> 1sec RMS voltage L1, V 3.0
   * 0906 0101340700FF 120000 -> 1sec RMS voltage L2, V 3.0
   * 0906 0101480700FF 120000 -> 1sec RMS voltage L3, V 3.0
   */
  25,
  {
    {METER_ID, 3, 0},
    {METER_TYPE, 5, 0},
    {ACTIVE_POWER_IMPORT, 7, 0},
    {ACTIVE_POWER_EXPORT, 9, 0},
    {REACTIVE_POWER_IMPORT, 11, 0},
    {REACTIVE_POWER_EXPORT, 13, 0},
    {CURRENT_L1, 15, 1},
    {CURRENT_L2, 17, 1},
    {CURRENT_L3, 19, 1},
    {VOLTAGE_L1, 21, 0},
    {VOLTAGE_L2, 23, 0},
    {VOLTAGE_L3, 25, 0},
    {END_OF_LIST, 0, 0}
  }
};

const ams_known_list_t kamstrup_list2_3p_it = {
  // Kamstrup list2 for 3phase IT
  /* 0A0E 4B616D73747275705F5630303031 -> list ID
   * 0906 0101000005FF 0A10 35373036353637303030303030303030 -> meter GSIN
   * 0906 0101600101FF 0A12 303030303030303030303030303030303030 -> meter model
   * 0906 0101010700FF 0600000000 -> active power import, kW 4.3
   * 0906 0101020700FF 0600000000 -> active power export, kW 4.3
   * 0906 0101030700FF 0600000000 -> reactive power import, kVAr 4.3
   * 0906 0101040700FF 0600000000 -> reactive power export, kVAr 4.3
   * 0906 01011F0700FF 0600000000 -> 1sec RMS current L1, A 3.2
   * 0906 0101470700FF 0600000000 -> 1sec RMS current L3, A 3.2
   * 0906 0101200700FF 120000 -> 1sec RMS voltage L1, V 3.0
   * 0906 0101340700FF 120000 -> 1sec RMS voltage L2, V 3.0
   * 0906 0101480700FF 120000 -> 1sec RMS voltage L3, V 3.0
   */
  23,
  {
    {METER_ID, 3, 0},
    {METER_TYPE, 5, 0},
    {ACTIVE_POWER_IMPORT, 7, 0},
    {ACTIVE_POWER_EXPORT, 9, 0},
    {REACTIVE_POWER_IMPORT, 11, 0},
    {REACTIVE_POWER_EXPORT, 13, 0},
    {CURRENT_L1, 15, 1},
    {CURRENT_L3, 17, 1},
    {VOLTAGE_L1, 19, 0},
    {VOLTAGE_L2, 21, 0},
    {VOLTAGE_L3, 23, 0},
    {END_OF_LIST, 0, 0}
  }
};

const ams_known_list_t kamstrup_list2_1p = {
  // Kamstrup list2 for 1phase
  17,
  {
    {METER_ID, 3, 0},
    {METER_TYPE, 5, 0},
    {ACTIVE_POWER_IMPORT, 7, 0},
    {ACTIVE_POWER_EXPORT, 9, 0},
    {REACTIVE_POWER_IMPORT, 11, 0},
    {REACTIVE_POWER_EXPORT, 13, 0},
    {CURRENT_L1, 15, 1},
    {VOLTAGE_L1, 17, 0},
    {END_OF_LIST, 0, 0}
  }
};

const ams_known_list_t kamstrup_list3_3p = {
  // Kamstrup list2 for 3phase TN
  /* 0A0E 4B616D73747275705F5630303031 -> list ID
   * 0906 0101000005FF 0A10 35373036353637303030303030303030 -> meter GSIN
   * 0906 0101600101FF 0A12 303030303030303030303030303030303030 -> meter model
   * 0906 0101010700FF 0600000000 -> active power import, kW 4.3
   * 0906 0101020700FF 0600000000 -> active power export, kW 4.3
   * 0906 0101030700FF 0600000000 -> reactive power import, kVAr 4.3
   * 0906 0101040700FF 0600000000 -> reactive power export, kVAr 4.3
   * 0906 01011F0700FF 0600000000 -> 1sec RMS current L1, A 3.2
   * 0906 0101330700FF 0600000000 -> 1sec RMS current L2, A 3.2
   * 0906 0101470700FF 0600000000 -> 1sec RMS current L3, A 3.2
   * 0906 0101200700FF 120000 -> 1sec RMS voltage L1, V 3.0
   * 0906 0101340700FF 120000 -> 1sec RMS voltage L2, V 3.0
   * 0906 0101480700FF 120000 -> 1sec RMS voltage L3, V 3.0
   * 0906 0001010000FF 090C 07E1081003100005FF800000 -> timestamp
   * 0906 0101010800FF 0600000000 -> active energy import, kWh 7.2
   * 0906 0101020800FF 0600000000 -> active energy export, kWh 7.2
   * 0906 0101030800FF 0600000000 -> reactive energy import, kWh 7.2
   * 0906 0101040800FF 0600000000 -> reactive energy export, kWh 7.2
   */
  35,
  {
    {METER_ID, 3, 0},
    {METER_TYPE, 5, 0},
    {ACTIVE_POWER_IMPORT, 7, 0},
    {ACTIVE_POWER_EXPORT, 9, 0},
    {REACTIVE_POWER_IMPORT, 11, 0},
    {REACTIVE_POWER_EXPORT, 13, 0},
    {CURRENT_L1, 15, 1},
    {CURRENT_L2, 17, 1},
    {CURRENT_L3, 19, 1},
    {VOLTAGE_L1, 21, 0},
    {VOLTAGE_L2, 23, 0},
    {VOLTAGE_L3, 25, 0},
    {DATE_TIME, 27, 0},
    {ACTIVE_ENERGY_IMPORT, 29, 1},
    {ACTIVE_ENERGY_EXPORT, 31, 1},
    {REACTIVE_ENERGY_IMPORT, 33, 1},
    {REACTIVE_ENERGY_EXPORT, 35, 1},
    {END_OF_LIST, 0, 0}
  }
};

const ams_known_list_t kamstrup_list3_3p_it = {
  // Kamstrup list3 for 3phase IT
  /* 0A0E 4B616D73747275705F5630303031 -> list ID
   * 0906 0101000005FF 0A10 35373036353637303030303030303030 -> meter GSIN
   * 0906 0101600101FF 0A12 303030303030303030303030303030303030 -> meter model
   * 0906 0101010700FF 0600000000 -> active power import, kW 4.3
   * 0906 0101020700FF 0600000000 -> active power export, kW 4.3
   * 0906 0101030700FF 0600000000 -> reactive power import, kVAr 4.3
   * 0906 0101040700FF 0600000000 -> reactive power export, kVAr 4.3
   * 0906 01011F0700FF 0600000000 -> 1sec RMS current L1, A 3.2
   * 0906 0101330700FF 0600000000 -> 1sec RMS current L2, A 3.2
   * 0906 0101470700FF 0600000000 -> 1sec RMS current L3, A 3.2
   * 0906 0101200700FF 120000 -> 1sec RMS voltage L1, V 3.0
   * 0906 0101340700FF 120000 -> 1sec RMS voltage L2, V 3.0
   * 0906 0101480700FF 120000 -> 1sec RMS voltage L3, V 3.0
   * 0906 0001010000FF 090C 07E1081003100005FF800000 -> timestamp
   * 0906 0101010800FF 0600000000 -> active energy import, kWh 7.2
   * 0906 0101020800FF 0600000000 -> active energy export, kWh 7.2
   * 0906 0101030800FF 0600000000 -> reactive energy import, kWh 7.2
   * 0906 0101040800FF 0600000000 -> reactive energy export, kWh 7.2
   */
  33,
  {
    {METER_ID, 3, 0},
    {METER_TYPE, 5, 0},
    {ACTIVE_POWER_IMPORT, 7, 0},
    {ACTIVE_POWER_EXPORT, 9, 0},
    {REACTIVE_POWER_IMPORT, 11, 0},
    {REACTIVE_POWER_EXPORT, 13, 0},
    {CURRENT_L1, 15, 1},
    {CURRENT_L3, 17, 1},
    {VOLTAGE_L1, 19, 0},
    {VOLTAGE_L2, 21, 0},
    {VOLTAGE_L3, 23, 0},
    {DATE_TIME, 25, 0},
    {ACTIVE_ENERGY_IMPORT, 27, 1},
    {ACTIVE_ENERGY_EXPORT, 29, 1},
    {REACTIVE_ENERGY_IMPORT, 31, 1},
    {REACTIVE_ENERGY_EXPORT, 33, 1},
    {END_OF_LIST, 0, 0}
  }
};

const ams_known_list_t kamstrup_list3_1p = {
  // Kamstrup list3 for 1phase
  27,
  {
    {METER_ID, 3, 0},
    {METER_TYPE, 5, 0},
    {ACTIVE_POWER_IMPORT, 7, 0},
    {ACTIVE_POWER_EXPORT, 9, 0},
    {REACTIVE_POWER_IMPORT, 11, 0},
    {REACTIVE_POWER_EXPORT, 13, 0},
    {CURRENT_L1, 15, 1},
    {VOLTAGE_L1, 17, 0},
    {DATE_TIME, 19, 0},
    {ACTIVE_ENERGY_IMPORT, 21, 1},
    {ACTIVE_ENERGY_EXPORT, 23, 1},
    {REACTIVE_ENERGY_IMPORT, 25, 1},
    {REACTIVE_ENERGY_EXPORT, 27, 1},
    {END_OF_LIST, 0, 0}
  }
};

const ams_known_list_ids_mapping_t kamstrup_v0001_mapping = {
  Kamstrup_V0001,
  sizeof("Kamstrup_V0001") - 1,
  "Kamstrup_V0001",
  {
    &kamstrup_list2_1p,
    &kamstrup_list2_3p,
    &kamstrup_list2_3p_it,
    &kamstrup_list3_1p,
    &kamstrup_list3_3p,
    &kamstrup_list3_3p_it,
    NULL
  }
};
#endif //DISABLE_KAMSTRUP

#ifndef DISABLE_KAIFA
const ams_known_list_t kaifa_list1 = {
  // Kaifa list1
  /* 0201
   *   06000002FA
   */
  1,
  {
    {ACTIVE_POWER_IMPORT, 1, 0},
    {END_OF_LIST, 0, 0}
  }
};

const ams_known_list_t kaifa_list2_3p = {
  // Kaifa list2 for 3phase
  /* 020D
   *   0907 4B464D5F303031 -> list ID
   *   0910 36393730363331343031373533393835 -> meter GSIN
   *   0908 4D41333034483345 -> meter model
   *   06000002FF -> active power import, Watt
   *   0600000000 -> active power export, Watt
   *   0600000000 -> reactive power import, VAr
   *   060000008B -> reactive power export, VAr
   *   0600000821 -> Current L1, A 4.3
   *   0600000798 -> Current L2 (zero for IT-wired meter with current transformer measurement), A 4.3
   *   0600000AD2 -> Current L3, A 4.3
   *   060000095D -> Voltage L1, V 6.1
   *   0600000000 -> Voltage L2 (always zero for IT-wired meters), V 6.1
   *   0600000966 -> Voltage L3, V 6.1
   */
  13,
  {
    {METER_ID, 2, 0},
    {METER_TYPE, 3, 0},
    {ACTIVE_POWER_IMPORT, 4, 0},
    {ACTIVE_POWER_EXPORT, 5, 0},
    {REACTIVE_POWER_IMPORT, 6, 0},
    {REACTIVE_POWER_EXPORT, 7, 0},
    {CURRENT_L1, 8, 0},
    {CURRENT_L2, 9, 0},
    {CURRENT_L3, 10, 0},
    {VOLTAGE_L1, 11, -1},
    {VOLTAGE_L2, 12, -1},
    {VOLTAGE_L3, 13, -1},
    {END_OF_LIST, 0, 0}
  }
};

const ams_known_list_t kaifa_list2_1p = {
  // Kaifa list2 for 1phase
  9,
  {
    {METER_ID, 2, 0},
    {METER_TYPE, 3, 0},
    {ACTIVE_POWER_IMPORT, 4, 0},
    {ACTIVE_POWER_EXPORT, 5, 0},
    {REACTIVE_POWER_IMPORT, 6, 0},
    {REACTIVE_POWER_EXPORT, 7, 0},
    {CURRENT_L1, 8, 0},
    {VOLTAGE_L1, 9, -1},
    {END_OF_LIST, 0, 0}
  }
};

const ams_known_list_t kaifa_list3_3p = {
  // Kaifa list3 for 3phase
  /* 090C 07 E1 09 0D 02 00 00 0A FF 80 00 00
   * 0212
   *  0907 4B 46 4D 5F 30 30 31
   *  0910 36 39 37 30 36 33 31 34 30 31 37 35 33 39 38 35
   *  0908 4D 41 33 30 34 48 33 45
   *  06 000011B5
   *  06 00000000
   *  06 00000000
   *  06 00000080
   *  06 00003848
   *  06 00003C76
   *  06 0000145B
   *  06 0000094D
   *  06 00000000
   *  06 0000095A
   *  090C 07 E1 09 0D 02 00 00 0A FF 80 00 00
   *  06 0001B857
   *  06 00000000
   *  06 00000065
   *  06 000033A4 */
  18,
  {
    {METER_ID, 2, 0},
    {METER_TYPE, 3, 0},
    {ACTIVE_POWER_IMPORT, 4, 0},
    {ACTIVE_POWER_EXPORT, 5, 0},
    {REACTIVE_POWER_IMPORT, 6, 0},
    {REACTIVE_POWER_EXPORT, 7, 0},
    {CURRENT_L1, 8, 0},
    {CURRENT_L2, 9, 0},
    {CURRENT_L3, 10, 0},
    {VOLTAGE_L1, 11, -1},
    {VOLTAGE_L2, 12, -1},
    {VOLTAGE_L3, 13, -1},
    {DATE_TIME, 14, 0},
    {ACTIVE_ENERGY_IMPORT, 15, 0},
    {ACTIVE_ENERGY_EXPORT, 16, 0},
    {REACTIVE_ENERGY_IMPORT, 17, 0},
    {REACTIVE_ENERGY_EXPORT, 18, 0},
    {END_OF_LIST, 0, 0}
  }
};

const ams_known_list_t kaifa_list3_1p = {
  // Kaifa list3 for 1phase
  14,
  {
    {METER_ID, 2, 0},
    {METER_TYPE, 3, 0},
    {ACTIVE_POWER_IMPORT, 4, 0},
    {ACTIVE_POWER_EXPORT, 5, 0},
    {REACTIVE_POWER_IMPORT, 6, 0},
    {REACTIVE_POWER_EXPORT, 7, 0},
    {CURRENT_L1, 8, 0},
    {VOLTAGE_L1, 9, -1},
    {DATE_TIME, 10, 0},
    {ACTIVE_ENERGY_IMPORT, 11, 0},
    {ACTIVE_ENERGY_EXPORT, 12, 0},
    {REACTIVE_ENERGY_IMPORT, 13, 0},
    {REACTIVE_ENERGY_EXPORT, 14, 0},
    {END_OF_LIST, 0, 0}
  }
};

const ams_known_list_ids_mapping_t kaifa_v0001_mapping = {
  KFM_001,
  sizeof("KFM_001") - 1,
  "KFM_001",
  {
    &kaifa_list1,
    &kaifa_list2_1p,
    &kaifa_list2_3p,
    &kaifa_list3_1p,
    &kaifa_list3_3p,
    NULL
  }
};
#endif //DISABLE_KAIFA

const ams_known_list_ids_mapping_t* ams_known_list_ids_mapping[] = {
#ifndef DISABLE_AIDON
  &aidon_v0001_mapping,
#endif // DISABLE_AIDON
#ifndef DISABLE_KAMSTRUP
  &kamstrup_v0001_mapping,
#endif // DISABLE_KAMSTRUP
#ifndef DISABLE_KAIFA
  &kaifa_v0001_mapping,
#endif // DISABLE_KAIFA
  // add more mappings here when added to ams_known_list_ids_t
  NULL
};
