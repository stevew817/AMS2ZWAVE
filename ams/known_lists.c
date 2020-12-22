/*
 * known_lists.c
 *
 *  Created on: Dec 22, 2020
 *      Author: steve
 */

#include "known_lists.h"

const known_list_ids_mapping_t aidon_v0001_mapping = {
    AIDON_V0001,
    sizeof("AIDON_V0001") - 1,
    "AIDON_V0001"
};

const known_list_ids_mapping_t* known_list_ids_mapping[] = {
    &aidon_v0001_mapping,
    // add more mappings here when added to known_list_ids_t
    NULL
};



const known_list_t aidon_list1 = {
    // Aidon list1
    AIDON_V0001,
    4,
    {
        {ACTIVE_POWER_IMPORT, 2, 0}
    }
};

const known_list_t aidon_list2_it = {
    // Aidon list2 for 3phase IT
    /*      0202 0906 0101000281ff 0a0b 4149444f4e5f5630303031 -> list ID
            0202 0906 0000600100ff 0a10 3733XXXXXXXXXXXXXXXXXXXXXXX13130 -> meter ID (ASCII)
            0202 0906 0000600107ff 0a04 36353235 -> meter type (ASCII)
            0203 0906 0100010700ff 06 00000e90 0202 0f00 161b -> active power import
            0203 0906 0100020700ff 06 00000000 0202 0f00 161b -> active power export
            0203 0906 0100030700ff 06 0000001c 0202 0f00 161d -> reactive power import
            0203 0906 0100040700ff 06 00000000 0202 0f00 161d -> reactive power export
            0203 0906 01001f0700ff 10 0091     0202 0fff 1621 -> current l1
            0203 0906 0100470700ff 10 0090     0202 0fff 1621 -> current l3
            0203 0906 0100200700ff 12 0932     0202 0fff 1623 -> voltage l1
            0203 0906 0100340700ff 12 091e     0202 0fff 1623 -> voltage l2
            0203 0906 0100480700ff 12 0933     0202 0fff 1623 -> voltage l3
    */
    AIDON_V0001,
    42,
    {
      {METER_ID, 4, 0},
      {METER_TYPE, 6, 0},
      {ACTIVE_POWER_IMPORT, 8, 0},
      {CURRENT_L1, 24, -1},
      {CURRENT_L3, 28, -1},
      {VOLTAGE_L1, 32, -1},
      {VOLTAGE_L2, 36, -1},
      {VOLTAGE_L3, 40, -1},
      {END_OF_LIST, 0, 0}
    }
};

const known_list_t aidon_list2_tn = {
    // Aidon list2 for 3phase TN
    AIDON_V0001,
    46,
    {
      {METER_ID, 4, 0},
      {METER_TYPE, 6, 0},
      {ACTIVE_POWER_IMPORT, 8, 0},
      {CURRENT_L1, 24, -1},
      {CURRENT_L2, 28, -1},
      {CURRENT_L3, 32, -1},
      {VOLTAGE_L1, 36, -1},
      {VOLTAGE_L2, 40, -1},
      {VOLTAGE_L3, 44, -1},
      {END_OF_LIST, 0, 0}
    }
};

const known_list_t aidon_list2_1p = {
    // Aidon list2 for 1phase
    AIDON_V0001,
    30,
    {
      {METER_ID, 4, 0},
      {METER_TYPE, 6, 0},
      {ACTIVE_POWER_IMPORT, 8, 0},
      {CURRENT_L1, 24, -1},
      {VOLTAGE_L1, 28, -1},
      {END_OF_LIST, 0, 0}
    }
};

const known_list_t aidon_list3_it = {
    // Aidon list3 for 3phase IT
    /*      0202 0906 0101000281ff 0a0b 4149444f4e5f5630303031 -> list ID
            0202 0906 0000600100ff 0a10 3733XXXXXXXXXXXXXXXXXXXXXXX13130 -> meter ID (ASCII)
            0202 0906 0000600107ff 0a04 36353235 -> meter type (ASCII)
            0203 0906 0100010700ff 06 00000e90 0202 0f00 161b -> active power import
            0203 0906 0100020700ff 06 00000000 0202 0f00 161b -> active power export
            0203 0906 0100030700ff 06 0000001c 0202 0f00 161d -> reactive power import
            0203 0906 0100040700ff 06 00000000 0202 0f00 161d -> reactive power export
            0203 0906 01001f0700ff 10 0091     0202 0fff 1621 -> current l1
            0203 0906 0100470700ff 10 0090     0202 0fff 1621 -> current l3
            0203 0906 0100200700ff 12 0932     0202 0fff 1623 -> voltage l1
            0203 0906 0100340700ff 12 091e     0202 0fff 1623 -> voltage l2
            0203 0906 0100480700ff 12 0933     0202 0fff 1623 -> voltage l3
            0202 0906 0000010000ff 090c 07e3 06 0c 03 17 00 00 ff 003c 00 -> timestamp
            0203 0906 0100010800ff 06 0021684d 0202 0f01 161e -> cumulative import
            0203 0906 0100020800ff 06 00000000 0202 0f01 161e -> cumulative export
            0203 0906 0100030800ff 06 00008251 0202 0f01 1620 -> cumulative reactive import
            0203 0906 0100040800ff 06 00011ba5 0202 0f01 1620 -> cumulative reactive export
    */
    AIDON_V0001,
    60,
    {
      {METER_ID, 4, 0},
      {METER_TYPE, 6, 0},
      {ACTIVE_POWER_IMPORT, 8, 0},
      {CURRENT_L1, 24, -1},
      {CURRENT_L3, 28, -1},
      {VOLTAGE_L1, 32, -1},
      {VOLTAGE_L2, 36, -1},
      {VOLTAGE_L3, 40, -1},
      {TOTAL_DELIVERED_ENERGY, 46, 1},
      {END_OF_LIST, 0, 0}
    }
};

const known_list_t aidon_list3_tn = {
    // Aidon list3 for 3phase TN
    AIDON_V0001,
    64,
    {
      {METER_ID, 4, 0},
      {METER_TYPE, 6, 0},
      {ACTIVE_POWER_IMPORT, 8, 0},
      {CURRENT_L1, 24, -1},
      {CURRENT_L2, 28, -1},
      {CURRENT_L3, 32, -1},
      {VOLTAGE_L1, 36, -1},
      {VOLTAGE_L2, 40, -1},
      {VOLTAGE_L3, 44, -1},
      {TOTAL_DELIVERED_ENERGY, 50, 1},
      {END_OF_LIST, 0, 0}
    }
};

const known_list_t aidon_list3_1p = {
    //Aidon list3 for 1phase
    AIDON_V0001,
    48,
    {
      {METER_ID, 4, 0},
      {METER_TYPE, 6, 0},
      {ACTIVE_POWER_IMPORT, 8, 0},
      {CURRENT_L1, 24, -1},
      {VOLTAGE_L1, 28, -1},
      {TOTAL_DELIVERED_ENERGY, 34, 1},
      {END_OF_LIST, 0}
    }
};

const known_list_t* list_db[] = {
    &aidon_list1,
    &aidon_list2_it,
    &aidon_list2_tn,
    &aidon_list2_1p,
    &aidon_list3_it,
    &aidon_list3_tn,
    &aidon_list3_1p,
    NULL
};
