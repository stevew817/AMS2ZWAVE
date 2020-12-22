/**
 * @file config_rf.h
 *
 * Configuration file for AMS2ZWAVE.
 *
 * Adapted from the Silicon Labs Z-Wave sample application repository, which is
 * licensed under the Zlib license:
 *   https://github.com/SiliconLabs/z_wave_applications
 *
 * Modifications by Steven Cooreman, github.com/stevew817
 *******************************************************************************
 * Original license:
 *
 * copyright 2018 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
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
#ifndef _CONFIG_APP_H_
#define _CONFIG_APP_H_

#include <ZW_product_id_enum.h>
#include <CC_ManufacturerSpecific.h>

/****************************************************************************
 *
 * Application version, which is generated during release of SDK.
 * The application developer can freely alter the version numbers
 * according to his needs.
 *
 ****************************************************************************/
#define APP_VERSION ZAF_VERSION_MAJOR
#define APP_REVISION ZAF_VERSION_MINOR
#define APP_PATCH ZAF_VERSION_PATCH

/****************************************************************************
 *
 * Defines device generic and specific types
 *
 ****************************************************************************/
//@ [GENERIC_TYPE_ID]
#define GENERIC_TYPE GENERIC_TYPE_METER
#define SPECIFIC_TYPE SPECIFIC_TYPE_NOT_USED
//@ [GENERIC_TYPE_ID]

/**
 * See ZW_basis_api.h for ApplicationNodeInformation field deviceOptionMask
 */
//@ [DEVICE_OPTIONS_MASK_ID]
#define DEVICE_OPTIONS_MASK   APPLICATION_NODEINFO_LISTENING
//@ [DEVICE_OPTIONS_MASK_ID]

/****************************************************************************
 *
 * Defines used to initialize the Z-Wave Plus Info Command Class.
 *
 ****************************************************************************/
//@ [APP_TYPE_ID]
#define APP_ROLE_TYPE ZWAVEPLUS_INFO_REPORT_ROLE_TYPE_SLAVE_ALWAYS_ON
#define APP_NODE_TYPE ZWAVEPLUS_INFO_REPORT_NODE_TYPE_ZWAVEPLUS_NODE
#define APP_ICON_TYPE ICON_TYPE_GENERIC_WHOLE_HOME_METER_SIMPLE
#define APP_USER_ICON_TYPE ICON_TYPE_GENERIC_WHOLE_HOME_METER_SIMPLE
//@ [APP_TYPE_ID]

/****************************************************************************
 *
 * Defines used to initialize the Manufacturer Specific Command Class.
 *
 ****************************************************************************/
//NOTE: These values are not actually assigned to this product. Should you
//      choose to run this firmware in your local Z-Wave network, you should
//      be aware that there will be no interoperability guarantees.
//      The AMS2ZWAVE code is not certified.
#define APP_MANUFACTURER_ID     MFG_ID_NOT_DEFINED
#define APP_PRODUCT_TYPE_ID     0xCA
#define APP_PRODUCT_ID          0xFE

#define APP_FIRMWARE_ID         APP_PRODUCT_ID | (APP_PRODUCT_TYPE_ID << 8)

/****************************************************************************
 *
 * Defines used to initialize the Association Group Information (AGI)
 * Command Class.
 *
 ****************************************************************************/
#define NUMBER_OF_ENDPOINTS         0
#define MAX_ASSOCIATION_GROUPS      1
#define MAX_ASSOCIATION_IN_GROUP    5

/*
 * File identifiers for application file system
 * Range: 0x00000 - 0x0FFFF
 */
#define FILE_ID_APPLICATIONDATA (0x00000)

//@ [AGI_TABLE_ID]
#define AGITABLE_LIFELINE_GROUP \
 {COMMAND_CLASS_DEVICE_RESET_LOCALLY, DEVICE_RESET_LOCALLY_NOTIFICATION}, \
 {COMMAND_CLASS_METER_V5, METER_REPORT_V5},\
 {COMMAND_CLASS_INDICATOR, INDICATOR_REPORT_V3}


#define  AGITABLE_ROOTDEVICE_GROUPS NULL
//@ [AGI_TABLE_ID]

/**
 * Security keys
 */
//@ [REQUESTED_SECURITY_KEYS_ID]
#define REQUESTED_SECURITY_KEYS (SECURITY_KEY_S0_BIT | SECURITY_KEY_S2_UNAUTHENTICATED_BIT | SECURITY_KEY_S2_AUTHENTICATED_BIT)
//@ [REQUESTED_SECURITY_KEYS_ID]

/**
 * Setup the Z-Wave frequency
 *
 * The definition is only set if it's not already set to make it possible to pass the frequency to
 * the compiler by command line or in the Studio project.
 */
#ifndef APP_FREQ
#define APP_FREQ REGION_DEFAULT
#endif

#endif /* _CONFIG_APP_H_ */

