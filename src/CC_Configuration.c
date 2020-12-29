/***************************************************************************//**
 * @file CC_Configuration.c
 * @brief This file contains the local configuration parameters and
 *        Configuration CC implementation.
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
 ******************************************************************************/

/*******************************************************************************
 * The current implementation adheres to Configuration CC v4, but lacks support
 * for bulk commands. This is fine from the standard's perspective (SDS13781) as
 * long as we advertise that we don't have support for it, and generate the
 * requisite response should we receive such a non-supported command.
 *
 * Todo:
 * * Add support for declaring a parameter as changing node capabilities
 *   (will require callback into application on change)
 * * Proper support of signed values
 ******************************************************************************/
#include "CC_Configuration.h"
#include "ZW_TransportEndpoint.h"
#include "DebugPrint.h"
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
  SIGNED = 0,
  UNSIGNED = 1,
  ENUMERATED = 2,
  BITFIELD = 3,
} parameter_format_t;

typedef union {
  uint8_t u8;
  uint16_t u16;
  uint32_t u32;
  int8_t i8;
  int16_t i16;
  int32_t i32;
} parameter_value_t;

#define PARAM_VALUE_U8(x) { .u8 = x }
#define PARAM_VALUE_U16(x) { .u16 = x }
#define PARAM_VALUE_U32(x) { .u32 = x }

#define PARAM_VALUE_I8(x) { .i8 = x }
#define PARAM_VALUE_I16(x) { .i16 = x }
#define PARAM_VALUE_I32(x) { .i32 = x }

typedef struct {
  const char* buffer;
  const size_t length;
} parameter_string_t;

#define PARAM_DESC_STR(x) { x, sizeof(x) - 1}

typedef struct {
  const uint8_t param_nbr;  // Configuration parameter number
  const uint8_t param_size; // Amount of bytes in parameter
  void* param;              // Memory location where parameter resides
  const parameter_string_t name;   // User-visible parameter name
  const parameter_string_t info;   // User-visible parameter info
  const parameter_value_t param_default;
  const parameter_value_t param_min;
  const parameter_value_t param_max;
  const parameter_format_t format;
  const bool read_only;
  const bool is_advanced;
} param_desc_t;

// Runtime storage object
SConfigurationData CC_ConfigurationData;

/**************************** CUSTOMISE HERE **********************************/
static const param_desc_t parameter_table[] = {
    {
        .param_nbr = 1,
        .param_size = sizeof(CC_ConfigurationData.amount_of_10s_reports_for_meter_report),
        .param = &CC_ConfigurationData.amount_of_10s_reports_for_meter_report,
        .name = PARAM_DESC_STR("Time interval for reporting power consumption"),
        .info = PARAM_DESC_STR("How often to report power consumption to lifeline group, in 10s intervals. 0 = no time-based power reporting."),
        .param_default = PARAM_VALUE_U8(3),
        .param_min = PARAM_VALUE_U8(0),
        .param_max = PARAM_VALUE_U8(255),
        .format = UNSIGNED,
        .read_only = false,
        .is_advanced = false,
    },
    {
        .param_nbr = 2,
        .param_size = sizeof(CC_ConfigurationData.power_change_for_meter_report),
        .param = &CC_ConfigurationData.power_change_for_meter_report,
        .name = PARAM_DESC_STR("Change-based power consumption reporting"),
        .info = PARAM_DESC_STR("When to report power consumption to lifeline group, based on change in 100W increments since the last report. 0 = no change-based power reporting."),
        .param_default = PARAM_VALUE_U8(5),
        .param_min = PARAM_VALUE_U8(0),
        .param_max = PARAM_VALUE_U8(255),
        .format = UNSIGNED,
        .read_only = false,
        .is_advanced = false,
    },
    {
        .param_nbr = 3,
        .param_size = sizeof(CC_ConfigurationData.turn_off_hourly_report),
        .param = &CC_ConfigurationData.turn_off_hourly_report,
        .name = PARAM_DESC_STR("Enable accumulated energy consumption reporting"),
        .info = PARAM_DESC_STR("Whether to enable sending hourly accumulated energy consumption reports to lifeline group. 0 = enabled, 1 = disabled."),
        .param_default = PARAM_VALUE_U8(0),
        .param_min = PARAM_VALUE_U8(0),
        .param_max = PARAM_VALUE_U8(1),
        .format = BITFIELD,
        .read_only = false,
        .is_advanced = false,
    },
};
/*************************** END CUSTOMISATION ********************************/

// What to report for name and info on an undefined parameter number
static const char undefined_param[] = "Unassigned parameter";
// todo: figure out the actual max size...
static const size_t max_chars_per_report = 40;

/*******************************************************************************
 * Callback logic for only queueing one fragment of a string report at a time.
 ******************************************************************************/
typedef struct {
  bool in_progress;                             // is the content of this struct valid?
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX pkg_options;  // transmit options for sending the next packet
  uint8_t command;                              // command ID to send the next packet with
  uint8_t param_nbr;                            // parameter number to send the next packet with
  uint8_t num_packets_remaining;                // remaining number of packets to send
  const char* string;                           // base pointer to string being sent
  size_t string_length;                         // total size of string being sent
  size_t offset;                                // index in string for sending the next packet
} string_package_in_progress_t;

static string_package_in_progress_t string_package_in_progress = {
  .in_progress = false,
};

static void string_package_progress_cb( uint8_t status )
{
  ZAF_TRANSPORT_TX_BUFFER  TxBuf;
  ZW_APPLICATION_TX_BUFFER *pTxBuf = &(TxBuf.appTxBuf);
  memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

  // Name and info reports look identical besides their command ID.
  pTxBuf->ZW_ConfigurationNameReport1byteV4Frame.cmdClass =
      COMMAND_CLASS_CONFIGURATION;
  pTxBuf->ZW_ConfigurationNameReport1byteV4Frame.cmd =
      string_package_in_progress.command;
  pTxBuf->ZW_ConfigurationNameReport1byteV4Frame.parameterNumber1 =
      0;
  pTxBuf->ZW_ConfigurationNameReport1byteV4Frame.parameterNumber2 =
      string_package_in_progress.param_nbr;
  pTxBuf->ZW_ConfigurationNameReport1byteV4Frame.reportsToFollow =
      string_package_in_progress.num_packets_remaining;

  size_t report_size;
  if( string_package_in_progress.num_packets_remaining > 0 ) {
    report_size = max_chars_per_report;
  } else {
    report_size = string_package_in_progress.string_length -
                  string_package_in_progress.offset;
  }

  memcpy(&pTxBuf->ZW_ConfigurationNameReport1byteV4Frame.name1,
         &string_package_in_progress.string[string_package_in_progress.offset],
         report_size);

  // Add packet overhead to reported string length
  report_size += offsetof(ZW_CONFIGURATION_NAME_REPORT_1BYTE_V4_FRAME, name1);

  if( string_package_in_progress.num_packets_remaining > 0 ) {
    if( EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendResponseEP(
          (uint8_t *)pTxBuf,
          report_size,
          &string_package_in_progress.pkg_options,
          string_package_progress_cb) )
    {
      // Failed to queue the packet somehow, meaning we won't get a
      // new callback. Give up on this transmission.
      string_package_in_progress.in_progress = false;
    }

    string_package_in_progress.num_packets_remaining--;
    string_package_in_progress.offset += report_size - 5;

  } else {
    if( EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendResponseEP(
          (uint8_t *)pTxBuf,
          report_size,
          &string_package_in_progress.pkg_options,
          NULL) )
    {
      ;
    }

    // Full string has been transmitted
    string_package_in_progress.in_progress = false;
  }
}


/*******************************************************************************
 * NVM storage handling for configuration data
 ******************************************************************************/
static nvm3_Handle_t* lastLoadedFilesystem;

// Load all configuration parameters from storage
void CC_Configuration_loadFromNVM( nvm3_Handle_t* pFileSystemApplication )
{
  if( pFileSystemApplication != NULL )
    lastLoadedFilesystem = pFileSystemApplication;
  else
    pFileSystemApplication = lastLoadedFilesystem;

  Ecode_t errCode = nvm3_readData(pFileSystemApplication,
                                  FILE_ID_CONFIGURATIONDATA,
                                  &CC_ConfigurationData,
                                  sizeof(CC_ConfigurationData));

  // Reset to default if the data got corrupted or not migrated properly
  if( errCode == ECODE_NVM3_ERR_KEY_NOT_FOUND ) {
    DPRINT("Error: configuration parameter object does not exist\n");
    CC_Configuration_resetToDefault(pFileSystemApplication);
  } else if( errCode == ECODE_NVM3_ERR_READ_DATA_SIZE ) {
/**************************** CUSTOMISE HERE **********************************/
    // Object exists, but its size does not match the currently compiled size.
    // This is most likely due to adding/removing parameters between firmware
    // versions. You should handle this in your upgrade logic, or inline here.
    DPRINT("Error: configuration parameter object size mismatch\n");
    CC_Configuration_resetToDefault(pFileSystemApplication);
/*************************** END CUSTOMISATION ********************************/
  } else {
    // Assert has been kept for debugging , can be removed from production code.
    // This error hard to occur when a corresponding write is successful
    // This error can only be caused by some internal flash HW failure
    ASSERT(ECODE_NVM3_OK == errCode);
  }
}

// Save all configuration parameters to storage
void CC_Configuration_saveToNVM( nvm3_Handle_t* pFileSystemApplication )
{
  if( pFileSystemApplication != NULL )
    lastLoadedFilesystem = pFileSystemApplication;
  else
    pFileSystemApplication = lastLoadedFilesystem;

  Ecode_t errCode = nvm3_writeData(pFileSystemApplication,
                                   FILE_ID_CONFIGURATIONDATA,
                                   &CC_ConfigurationData,
                                   sizeof(CC_ConfigurationData));
  // Assert has been kept for debugging , can be removed from production code.
  // This error can only be caused by some internal flash HW failure
  ASSERT(ECODE_NVM3_OK == errCode);
}

// Reset all configuration parameters to their default values
void CC_Configuration_resetToDefault( nvm3_Handle_t* pFileSystemApplication )
{
  if( pFileSystemApplication != NULL )
    lastLoadedFilesystem = pFileSystemApplication;
  else
    pFileSystemApplication = lastLoadedFilesystem;

  // Scroll through parameter table to load default values into struct
  for( size_t i = 0;
       i < sizeof(parameter_table) / sizeof(parameter_table[0]);
       i++ ) {
    switch( parameter_table[i].param_size ) {
      case sizeof(uint8_t):
        *((uint8_t*)(parameter_table[i].param)) =
          parameter_table[i].param_default.u8;
        break;
      case sizeof(uint16_t):
        *((uint16_t*)(parameter_table[i].param)) =
          parameter_table[i].param_default.u16;
        break;
      case sizeof(uint32_t):
        *((uint32_t*)(parameter_table[i].param)) =
          parameter_table[i].param_default.u32;
        break;
      default:
        // Don't know what to do with this type...
        break;
    }
  }

  // Save it
  CC_Configuration_saveToNVM(pFileSystemApplication);
}

/*******************************************************************************
 * Command handler to attach in \ref Transport_ApplicationCommandHandlerEx
 ******************************************************************************/
received_frame_status_t handleCommandClassConfiguration(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength )
{
  switch (pCmd->ZW_Common.cmd)
  {
    case CONFIGURATION_GET_V4:
      if( false == Check_not_legal_response_job(rxOpt) ) {
        ZAF_TRANSPORT_TX_BUFFER  TxBuf;
        ZW_APPLICATION_TX_BUFFER *pTxBuf = &(TxBuf.appTxBuf);
        memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

        TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
        RxToTxOptions(rxOpt, &pTxOptionsEx);

        uint8_t param_nbr = pCmd->ZW_ConfigurationGetV4Frame.parameterNumber;
        const param_desc_t* param_descr = NULL;
        for( size_t i = 0;
             i < sizeof(parameter_table) / sizeof(parameter_table[0]);
             i++ ) {
          if( parameter_table[i].param_nbr == param_nbr ) {
            param_descr = &parameter_table[i];
            break;
          }
        }

        if( param_descr == NULL ) {
          // SDS13781: When a 'get' command is issued with an invalid parameter
          // number, return the value of the first available parameter
          param_descr = &parameter_table[0];
          param_nbr = param_descr->param_nbr;
        }

        pTxBuf->ZW_ConfigurationReport1byteV4Frame.cmdClass = COMMAND_CLASS_CONFIGURATION;
        pTxBuf->ZW_ConfigurationReport1byteV4Frame.cmd = CONFIGURATION_REPORT;
        pTxBuf->ZW_ConfigurationReport1byteV4Frame.parameterNumber = param_nbr;
        pTxBuf->ZW_ConfigurationReport1byteV4Frame.level = param_descr->param_size;

        size_t frame_size;
        uint32_t value;
        switch( param_descr->param_size ) {
          case 1:
            value = *((uint8_t*)(param_descr->param));
            pTxBuf->ZW_ConfigurationReport1byteV4Frame.configurationValue1 = value & 0xFF;
            frame_size = sizeof(pTxBuf->ZW_ConfigurationReport1byteV4Frame);
            break;
          case 2:
            value = *((uint16_t*)(param_descr->param));
            pTxBuf->ZW_ConfigurationReport2byteV4Frame.configurationValue1 = (value >> 8) & 0xFF;
            pTxBuf->ZW_ConfigurationReport2byteV4Frame.configurationValue2 = (value & 0xFF);
            frame_size = sizeof(pTxBuf->ZW_ConfigurationReport2byteV4Frame);
            break;
          case 4:
            value = *((uint32_t*)(param_descr->param));
            pTxBuf->ZW_ConfigurationReport4byteV4Frame.configurationValue1 = (value >> 24) & 0xFF;
            pTxBuf->ZW_ConfigurationReport4byteV4Frame.configurationValue2 = (value >> 16) & 0xFF;
            pTxBuf->ZW_ConfigurationReport4byteV4Frame.configurationValue3 = (value >> 8) & 0xFF;
            pTxBuf->ZW_ConfigurationReport4byteV4Frame.configurationValue4 = value & 0xFF;
            frame_size = sizeof(pTxBuf->ZW_ConfigurationReport4byteV4Frame);
            break;
          default:
            return RECEIVED_FRAME_STATUS_FAIL;
        }

        if( EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendResponseEP(
              (uint8_t *)pTxBuf,
              frame_size,
              pTxOptionsEx,
              NULL) )
        {
          /*Job failed */
          ;
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;

    case CONFIGURATION_SET_V4:
      if( false == Check_not_legal_response_job(rxOpt) ) {
        // Command does not require sending back a report, we just need to apply
        // the new value.
        uint8_t param_nbr = pCmd->ZW_ConfigurationSet1byteV4Frame.parameterNumber;
        uint8_t size = pCmd->ZW_ConfigurationSet1byteV4Frame.level & 0x7;
        uint8_t set_default = pCmd->ZW_ConfigurationSet1byteV4Frame.level >> 7;

        const param_desc_t* param_descr = NULL;
        for( size_t i = 0;
             i < sizeof(parameter_table) / sizeof(parameter_table[0]);
             i++ ) {
          if( parameter_table[i].param_nbr == param_nbr &&
              parameter_table[i].param_size == size ) {
            param_descr = &parameter_table[i];
            break;
          }
        }

        // Ignore unknown parameters or non-matching size
        if( param_descr == NULL ) {
          return RECEIVED_FRAME_STATUS_NO_SUPPORT;
        }

        // Ignore read-only parameters
        if( param_descr->read_only ) {
            return RECEIVED_FRAME_STATUS_NO_SUPPORT;
        }

        uint32_t value;
        switch( param_descr->param_size ) {
          case 1:
            value = pCmd->ZW_ConfigurationSet1byteV4Frame.configurationValue1;
            if( set_default ) {
                value = param_descr->param_default.u8;
            }
            *((uint8_t*)param_descr->param) = value;
            break;
          case 2:
            value = pCmd->ZW_ConfigurationSet2byteV4Frame.configurationValue1 << 8;
            value |= pCmd->ZW_ConfigurationSet2byteV4Frame.configurationValue2;
            if( set_default ) {
                value = param_descr->param_default.u16;
            }
            *((uint16_t*)param_descr->param) = value;
            break;
          case 4:
            value = pCmd->ZW_ConfigurationSet4byteV4Frame.configurationValue1 << 24;
            value |= pCmd->ZW_ConfigurationSet4byteV4Frame.configurationValue2 << 16;
            value |= pCmd->ZW_ConfigurationSet4byteV4Frame.configurationValue3 << 8;
            value |= pCmd->ZW_ConfigurationSet4byteV4Frame.configurationValue4;

            if( set_default ) {
                value = param_descr->param_default.u32;
            }
            *((uint32_t*)param_descr->param) = value;
            break;
          default:
            return RECEIVED_FRAME_STATUS_FAIL;
        }

        // Write out application data
        CC_Configuration_saveToNVM(NULL);

        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;

    case CONFIGURATION_NAME_GET_V4:
      if( false == Check_not_legal_response_job(rxOpt) ) {
        ZAF_TRANSPORT_TX_BUFFER  TxBuf;
        ZW_APPLICATION_TX_BUFFER *pTxBuf = &(TxBuf.appTxBuf);
        memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

        TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
        RxToTxOptions(rxOpt, &pTxOptionsEx);

        // No support for parameters > 255 yet
        if( pCmd->ZW_ConfigurationNameGetV4Frame.parameterNumber1 != 0 ) {
          return RECEIVED_FRAME_STATUS_NO_SUPPORT;
        }

        uint8_t param_nbr = pCmd->ZW_ConfigurationNameGetV4Frame.parameterNumber2;
        const param_desc_t* param_descr = NULL;
        for( size_t i = 0;
             i < sizeof(parameter_table) / sizeof(parameter_table[0]);
             i++ ) {
          if( parameter_table[i].param_nbr == param_nbr ) {
            param_descr = &parameter_table[i];
            break;
          }
        }

        const char* param_info;
        size_t param_info_length;

        if( param_descr == NULL ) {
          param_info = undefined_param;
          param_info_length = sizeof(undefined_param) - 1;
        } else {
          param_info = param_descr->name.buffer;
          param_info_length = param_descr->name.length;
        }

        pTxBuf->ZW_ConfigurationNameReport1byteV4Frame.cmdClass = COMMAND_CLASS_CONFIGURATION;
        pTxBuf->ZW_ConfigurationNameReport1byteV4Frame.cmd = CONFIGURATION_NAME_REPORT_V4;
        pTxBuf->ZW_ConfigurationNameReport1byteV4Frame.parameterNumber1 = 0;
        pTxBuf->ZW_ConfigurationNameReport1byteV4Frame.parameterNumber2 = param_nbr;

        pTxBuf->ZW_ConfigurationNameReport1byteV4Frame.reportsToFollow =
          param_info_length / max_chars_per_report;

        if( param_info_length * max_chars_per_report < param_info_length ) {
          pTxBuf->ZW_ConfigurationNameReport1byteV4Frame.reportsToFollow++;
        }

        if( param_info_length > max_chars_per_report ) {
          if( string_package_in_progress.in_progress ) {
            return RECEIVED_FRAME_STATUS_FAIL;
          }

          memcpy(&pTxBuf->ZW_ConfigurationNameReport1byteV4Frame.name1,
                 param_info,
                 max_chars_per_report);

          // Set up string package callback
          string_package_in_progress.in_progress = true;
          memcpy(&string_package_in_progress.pkg_options,
                 pTxOptionsEx,
                 sizeof(TRANSMIT_OPTIONS_TYPE_SINGLE_EX));
          string_package_in_progress.command = CONFIGURATION_NAME_REPORT_V4;
          string_package_in_progress.param_nbr = param_nbr;
          string_package_in_progress.num_packets_remaining = pTxBuf->ZW_ConfigurationNameReport1byteV4Frame.reportsToFollow - 1;
          string_package_in_progress.string = param_info;
          string_package_in_progress.string_length = param_info_length;
          string_package_in_progress.offset = max_chars_per_report;

          if( EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendResponseEP(
                (uint8_t *)pTxBuf,
                max_chars_per_report + 5,
                pTxOptionsEx,
                string_package_progress_cb) )
          {
            /*Job failed */
            ;
          }
        } else {
          memcpy(&pTxBuf->ZW_ConfigurationNameReport1byteV4Frame.name1, param_info, param_info_length);

          if( EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendResponseEP(
                (uint8_t *)pTxBuf,
                param_info_length + 5,
                pTxOptionsEx,
                NULL) )
          {
            /*Job failed */
            ;
          }
        }

        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;

    case CONFIGURATION_INFO_GET_V4:
      if( false == Check_not_legal_response_job(rxOpt) ) {
        ZAF_TRANSPORT_TX_BUFFER  TxBuf;
        ZW_APPLICATION_TX_BUFFER *pTxBuf = &(TxBuf.appTxBuf);
        memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

        TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
        RxToTxOptions(rxOpt, &pTxOptionsEx);

        // No support for parameters > 255 yet
        if( pCmd->ZW_ConfigurationInfoGetV4Frame.parameterNumber1 != 0 ) {
          return RECEIVED_FRAME_STATUS_NO_SUPPORT;
        }

        uint8_t param_nbr = pCmd->ZW_ConfigurationInfoGetV4Frame.parameterNumber2;
        const param_desc_t* param_descr = NULL;
        for( size_t i = 0;
             i < sizeof(parameter_table) / sizeof(parameter_table[0]);
             i++ ) {
          if( parameter_table[i].param_nbr == param_nbr ) {
            param_descr = &parameter_table[i];
            break;
          }
        }

        const char* param_info;
        size_t param_info_length;

        if( param_descr == NULL ) {
          param_info = undefined_param;
          param_info_length = sizeof(undefined_param) - 1;
        } else {
          param_info = param_descr->info.buffer;
          param_info_length = param_descr->info.length;
        }

        pTxBuf->ZW_ConfigurationInfoReport1byteV4Frame.cmdClass = COMMAND_CLASS_CONFIGURATION;
        pTxBuf->ZW_ConfigurationInfoReport1byteV4Frame.cmd = CONFIGURATION_INFO_REPORT_V4;
        pTxBuf->ZW_ConfigurationInfoReport1byteV4Frame.parameterNumber1 = 0;
        pTxBuf->ZW_ConfigurationInfoReport1byteV4Frame.parameterNumber2 = param_nbr;

        pTxBuf->ZW_ConfigurationInfoReport1byteV4Frame.reportsToFollow =
          param_info_length / max_chars_per_report;

        if( param_info_length * max_chars_per_report < param_info_length ) {
          pTxBuf->ZW_ConfigurationInfoReport1byteV4Frame.reportsToFollow++;
        }

        if( param_info_length > max_chars_per_report ) {
          if( string_package_in_progress.in_progress ) {
            return RECEIVED_FRAME_STATUS_FAIL;
          }

          memcpy(&pTxBuf->ZW_ConfigurationInfoReport1byteV4Frame.info1, param_info, max_chars_per_report);

          // Set up string package callback
          string_package_in_progress.in_progress = true;
          memcpy(&string_package_in_progress.pkg_options,
                 pTxOptionsEx,
                 sizeof(TRANSMIT_OPTIONS_TYPE_SINGLE_EX));
          string_package_in_progress.command = CONFIGURATION_INFO_REPORT_V4;
          string_package_in_progress.param_nbr = param_nbr;
          string_package_in_progress.num_packets_remaining = pTxBuf->ZW_ConfigurationInfoReport1byteV4Frame.reportsToFollow - 1;
          string_package_in_progress.string = param_info;
          string_package_in_progress.string_length = param_info_length;
          string_package_in_progress.offset = max_chars_per_report;

          if( EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendResponseEP(
                (uint8_t *)pTxBuf,
                max_chars_per_report + 5,
                pTxOptionsEx,
                string_package_progress_cb) )
          {
            /*Job failed */
            ;
          }
        } else {
          memcpy(&pTxBuf->ZW_ConfigurationInfoReport1byteV4Frame.info1, param_info, param_info_length);

          if( EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendResponseEP(
                (uint8_t *)pTxBuf,
                param_info_length + 5,
                pTxOptionsEx,
                NULL) )
          {
            /*Job failed */
            ;
          }
        }

        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;

    case CONFIGURATION_PROPERTIES_GET_V4:
      if( false == Check_not_legal_response_job(rxOpt) ) {
        ZAF_TRANSPORT_TX_BUFFER  TxBuf;
        ZW_APPLICATION_TX_BUFFER *pTxBuf = &(TxBuf.appTxBuf);
        memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

        TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
        RxToTxOptions(rxOpt, &pTxOptionsEx);

        // No support for parameters > 255 yet
        if( pCmd->ZW_ConfigurationPropertiesGetV4Frame.parameterNumber1 != 0 ) {
          return RECEIVED_FRAME_STATUS_NO_SUPPORT;
        }

        uint8_t param_nbr = pCmd->ZW_ConfigurationPropertiesGetV4Frame.parameterNumber2;
        uint8_t next_param_nbr;
        const param_desc_t* param_descr = NULL;
        for( size_t i = 0;
             i < sizeof(parameter_table) / sizeof(parameter_table[0]);
             i++ ) {
          if( parameter_table[i].param_nbr == param_nbr ) {
            param_descr = &parameter_table[i];
            if( i < (sizeof(parameter_table) / sizeof(parameter_table[0]) - 1) ) {
              next_param_nbr = parameter_table[i+1].param_nbr;
            } else {
              // Last parameter
              next_param_nbr = 0;
            }
            break;
          }
        }

        pTxBuf->ZW_ConfigurationPropertiesReport1byteV4Frame.cmdClass = COMMAND_CLASS_CONFIGURATION;
        pTxBuf->ZW_ConfigurationPropertiesReport1byteV4Frame.cmd = CONFIGURATION_PROPERTIES_REPORT_V4;
        pTxBuf->ZW_ConfigurationPropertiesReport1byteV4Frame.parameterNumber1 = 0;
        pTxBuf->ZW_ConfigurationPropertiesReport1byteV4Frame.parameterNumber2 = param_nbr;

        // Handle undefined parameter case first
        if( param_descr == NULL ) {
          pTxBuf->ZW_ConfigurationPropertiesReport1byteV4Frame.properties1 = 0;
          pTxBuf->ZW_ConfigurationPropertiesReport1byteV4Frame.minValue1 = 0; //actually next parameter number 1 due to declaring size = 0
          pTxBuf->ZW_ConfigurationPropertiesReport1byteV4Frame.maxValue1 = parameter_table[0].param_nbr; //actually next parameter number 2 due to declaring size = 0
          pTxBuf->ZW_ConfigurationPropertiesReport1byteV4Frame.defaultValue1 = 0x2; // Actually properties2 due to declaring size = 0, set no bulk support bit

          if( EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendResponseEP(
                (uint8_t *)pTxBuf,
                8,
                pTxOptionsEx,
                NULL) )
          {
            /*Job failed */
            ;
          }
          return RECEIVED_FRAME_STATUS_SUCCESS;
        }

        size_t response_size;

        // Todo: add support for declaring parameters which alter capabilities
        pTxBuf->ZW_ConfigurationPropertiesReport1byteV4Frame.properties1 =
          (param_descr->param_size & 0x7) |
          ((param_descr->format & 0x7) << 3) |
          (param_descr->read_only ? (1 << 6) : 0);

        switch( param_descr->param_size ) {
          case 1:
            pTxBuf->ZW_ConfigurationPropertiesReport1byteV4Frame.minValue1 = param_descr->param_min.u8;
            pTxBuf->ZW_ConfigurationPropertiesReport1byteV4Frame.maxValue1 = param_descr->param_max.u8;
            pTxBuf->ZW_ConfigurationPropertiesReport1byteV4Frame.defaultValue1 = param_descr->param_default.u8;

            pTxBuf->ZW_ConfigurationPropertiesReport1byteV4Frame.nextParameterNumber1 = 0;
            pTxBuf->ZW_ConfigurationPropertiesReport1byteV4Frame.nextParameterNumber2 = next_param_nbr;
            pTxBuf->ZW_ConfigurationPropertiesReport1byteV4Frame.properties2 = 0x2; // Set no bulk support bit
            if(param_descr->is_advanced)
              pTxBuf->ZW_ConfigurationPropertiesReport1byteV4Frame.properties2 |= 0x1;
            response_size = sizeof(pTxBuf->ZW_ConfigurationPropertiesReport1byteV4Frame);
            break;
          case 2:
            pTxBuf->ZW_ConfigurationPropertiesReport2byteV4Frame.minValue1 = param_descr->param_min.u16 >> 8;
            pTxBuf->ZW_ConfigurationPropertiesReport2byteV4Frame.minValue2 = param_descr->param_min.u16 & 0xFF;

            pTxBuf->ZW_ConfigurationPropertiesReport2byteV4Frame.maxValue1 = param_descr->param_max.u16 >> 8;
            pTxBuf->ZW_ConfigurationPropertiesReport2byteV4Frame.maxValue2 = param_descr->param_max.u16 & 0xFF;

            pTxBuf->ZW_ConfigurationPropertiesReport2byteV4Frame.defaultValue1 = param_descr->param_default.u16 >> 8;
            pTxBuf->ZW_ConfigurationPropertiesReport2byteV4Frame.defaultValue2 = param_descr->param_default.u16 & 0xFF;

            pTxBuf->ZW_ConfigurationPropertiesReport2byteV4Frame.nextParameterNumber1 = 0;
            pTxBuf->ZW_ConfigurationPropertiesReport2byteV4Frame.nextParameterNumber2 = next_param_nbr;
            pTxBuf->ZW_ConfigurationPropertiesReport2byteV4Frame.properties2 = 0x2; // Set no bulk support bit
            if(param_descr->is_advanced)
              pTxBuf->ZW_ConfigurationPropertiesReport2byteV4Frame.properties2 |= 0x1;
            response_size = sizeof(pTxBuf->ZW_ConfigurationPropertiesReport2byteV4Frame);
            break;
          case 4:
            pTxBuf->ZW_ConfigurationPropertiesReport4byteV4Frame.minValue1 = (param_descr->param_min.u32 >> 24) & 0xFF;
            pTxBuf->ZW_ConfigurationPropertiesReport4byteV4Frame.minValue2 = (param_descr->param_min.u32 >> 16) & 0xFF;
            pTxBuf->ZW_ConfigurationPropertiesReport4byteV4Frame.minValue3 = (param_descr->param_min.u32 >> 8) & 0xFF;
            pTxBuf->ZW_ConfigurationPropertiesReport4byteV4Frame.minValue4 = (param_descr->param_min.u32 >> 0) & 0xFF;

            pTxBuf->ZW_ConfigurationPropertiesReport4byteV4Frame.maxValue1 = (param_descr->param_max.u32 >> 24) & 0xFF;
            pTxBuf->ZW_ConfigurationPropertiesReport4byteV4Frame.maxValue2 = (param_descr->param_max.u32 >> 16) & 0xFF;
            pTxBuf->ZW_ConfigurationPropertiesReport4byteV4Frame.maxValue3 = (param_descr->param_max.u32 >> 8) & 0xFF;
            pTxBuf->ZW_ConfigurationPropertiesReport4byteV4Frame.maxValue4 = (param_descr->param_max.u32 >> 0) & 0xFF;

            pTxBuf->ZW_ConfigurationPropertiesReport4byteV4Frame.defaultValue1 = (param_descr->param_default.u32 >> 24) & 0xFF;
            pTxBuf->ZW_ConfigurationPropertiesReport4byteV4Frame.defaultValue2 = (param_descr->param_default.u32 >> 16) & 0xFF;
            pTxBuf->ZW_ConfigurationPropertiesReport4byteV4Frame.defaultValue3 = (param_descr->param_default.u32 >> 8) & 0xFF;
            pTxBuf->ZW_ConfigurationPropertiesReport4byteV4Frame.defaultValue4 = (param_descr->param_default.u32 >> 0) & 0xFF;

            pTxBuf->ZW_ConfigurationPropertiesReport4byteV4Frame.nextParameterNumber1 = 0;
            pTxBuf->ZW_ConfigurationPropertiesReport4byteV4Frame.nextParameterNumber2 = next_param_nbr;
            pTxBuf->ZW_ConfigurationPropertiesReport4byteV4Frame.properties2 = 0x2; // Set no bulk support bit
            if(param_descr->is_advanced)
              pTxBuf->ZW_ConfigurationPropertiesReport4byteV4Frame.properties2 |= 0x1;
            response_size = sizeof(pTxBuf->ZW_ConfigurationPropertiesReport4byteV4Frame);
            break;
          default:
            return RECEIVED_FRAME_STATUS_FAIL;
        }

        if( EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendResponseEP(
              (uint8_t *)pTxBuf,
              response_size,
              pTxOptionsEx,
              NULL) )
        {
          /*Job failed */
          ;
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;

    case CONFIGURATION_DEFAULT_RESET_V4:
      if( false == Check_not_legal_response_job(rxOpt) )
      {
        // Command does not require sending back a report, we just need to reset
        // our settings locally.
        CC_Configuration_resetToDefault(NULL);
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;

    case CONFIGURATION_BULK_GET_V4:
    case CONFIGURATION_BULK_SET_V4:
      // SDS13781: we may elect to ignore bulk configuration commands, given we
      // issue a 'rejected request' command to the originating node, and set the
      // No Bulk support bit in the properties report.
      if( false == Check_not_legal_response_job(rxOpt) ) {
        ZAF_TRANSPORT_TX_BUFFER  TxBuf;
        ZW_APPLICATION_TX_BUFFER *pTxBuf = &(TxBuf.appTxBuf);
        memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

        TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
        RxToTxOptions(rxOpt, &pTxOptionsEx);

        pTxBuf->ZW_ApplicationRejectedRequestFrame.cmdClass = COMMAND_CLASS_APPLICATION_STATUS;
        pTxBuf->ZW_ApplicationRejectedRequestFrame.cmd = APPLICATION_REJECTED_REQUEST;
        pTxBuf->ZW_ApplicationRejectedRequestFrame.status = 0;

        if( EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendResponseEP(
              (uint8_t *)pTxBuf,
              sizeof(pTxBuf->ZW_ApplicationRejectedRequestFrame),
              pTxOptionsEx,
              NULL) )
          {
            /*Job failed */
            ;
          }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;
    default:
      break;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}

REGISTER_CC(COMMAND_CLASS_CONFIGURATION, CONFIGURATION_VERSION_V4, handleCommandClassConfiguration);

#ifdef __cplusplus
}
#endif
