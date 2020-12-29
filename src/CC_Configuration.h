/***************************************************************************//**
 * @file CC_Configuration.h
 * @brief This file describes the local configuration parameter and CC interface
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
#ifndef CC_CONFIGURATION_H_
#define CC_CONFIGURATION_H_

#include "ZAF_types.h"
#include "nvm3.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**************************** CUSTOMISE HERE **********************************/
// Declare your configuration parameters' runtime storage object
typedef struct {
  uint8_t amount_of_10s_reports_for_meter_report;
  uint8_t power_change_for_meter_report;
  uint8_t turn_off_hourly_report;
} SConfigurationData;
/*************************** END CUSTOMISATION ********************************/

#define FILE_ID_CONFIGURATIONDATA (0x0001)

// Access runtime value through CC_ConfigurationData object
extern SConfigurationData CC_ConfigurationData;

// Load all configuration parameters from storage
void CC_Configuration_loadFromNVM( nvm3_Handle_t* pFileSystemApplication );

// Save all configuration parameters to storage
void CC_Configuration_saveToNVM( nvm3_Handle_t* pFileSystemApplication );

// Reset all configuration parameters to their default values
void CC_Configuration_resetToDefault( nvm3_Handle_t* pFileSystemApplication );

// Command handler to attach in \ref Transport_ApplicationCommandHandlerEx
received_frame_status_t handleCommandClassConfiguration(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength );

#ifdef __cplusplus
}
#endif

#endif /* CC_CONFIGURATION_H_ */
