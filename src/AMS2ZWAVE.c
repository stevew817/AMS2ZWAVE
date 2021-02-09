/**
 * @file AMS2ZWAVE.c
 * @brief Device reading electricity meter values from a Norwegian smart meter
 *        and reporting those through Z-Wave.
 *
 * Adapted from the Silicon Labs Z-Wave sample application repository, which is
 * licensed under the Zlib license:
 *   https://github.com/SiliconLabs/z_wave_applications
 *
 * Modifications by Steven Cooreman, github.com/stevew817
 *******************************************************************************
 * Original license:
 *
 * Copyright 2020 Silicon Laboratories Inc. www.silabs.com
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
 *******************************************************************************
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
/* During development of your device, you may add features using command    */
/* classes which are not already included. Remember to include relevant     */
/* classes and utilities and add them in your make file.                    */
/****************************************************************************/

/**************************** Boilerplate includes ****************************/
#include "config_rf.h"
#include <stdbool.h>
#include <stdint.h>
#include "SizeOf.h"
#include "Assert.h"
#include "DebugPrintConfig.h"
#define DEBUGPRINT
#include "DebugPrint.h"
#include "config_app.h"
#include "ZAF_app_version.h"
#include <ZAF_file_ids.h>
#include "nvm3.h"
#include "ZAF_nvm3_app.h"
#include <em_system.h>
#include <ZW_radio_api.h>
#include <ZW_slave_api.h>
#include <ZW_classcmd.h>
#include <ZW_TransportLayer.h>

#include <ZAF_uart_utils.h>
#include <board.h>
#include <ev_man.h>

#include <AppTimer.h>
#include <SwTimer.h>
#include <EventDistributor.h>
#include <ZW_system_startup_api.h>
#include <ZW_application_transport_interface.h>

#include <association_plus.h>
#include <agi.h>

#include <CC_Association.h>
#include <CC_AssociationGroupInfo.h>
#include <CC_DeviceResetLocally.h>
#include <CC_Indicator.h>
#include <CC_ManufacturerSpecific.h>
#include <CC_MultiChanAssociation.h>
#include <CC_PowerLevel.h>
#include <CC_Security.h>
#include <CC_Supervision.h>
#include <CC_Version.h>
#include <CC_ZWavePlusInfo.h>
#include <CC_FirmwareUpdate.h>

#include <ZW_TransportMulticast.h>

#include <zaf_event_helper.h>
#include <zaf_job_helper.h>

#include <ZAF_Common_helper.h>
#include <ZAF_network_learn.h>
#include <ota_util.h>
#include <ZAF_TSE.h>
#include <ota_util.h>
#include <ZAF_CmdPublisher.h>
#include <em_wdog.h>
#include "events.h"

#include <ZAF_network_management.h>

/***************************** AMS2ZWAVE includes *****************************/
#include <string.h>
#include "hanparser.h"
#include "readings.h"
#include "em_usart.h"

#include "CC_Configuration.h"

/*********************** AMS2ZWAVE function prototypes ************************/
void HAN_callback(const han_parser_data_t* decoded_data);
void HAN_serial_rx();
void HAN_setup();
void HAN_loadFromNVM(void);
void HAN_storeToNVM(bool update_meter, bool update_accumulated);
void HAN_resetNVM(void);

void* CC_Meter_prepare_zaf_tse_data(RECEIVE_OPTIONS_TYPE_EX* pRxOpt);
void CC_Meter_update_power(void);
void CC_Meter_update_energy(void);
void CC_Meter_report_unhandled_as_voltage(
    TRANSMIT_OPTIONS_TYPE_SINGLE_EX txOptions,
    void* pData);
void CC_Meter_report_unhandled_as_current(
    TRANSMIT_OPTIONS_TYPE_SINGLE_EX txOptions,
    void* pData);


received_frame_status_t
handleCommandClassMeter(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength);

received_frame_status_t
handleCommandClassConfiguration(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength);
/******************* end AMS2ZWAVE function prototypes ************************/

/****************************************************************************/
/* Application specific button and LED definitions                          */
/****************************************************************************/

// AMS2ZWAVE does not have extra LEDs or buttons besides APP_BUTTON_LEARN_RESET
// and APP_LED_INDICATOR

static EZwaveCommandStatusType unhandledCommandStatus;
static EZwaveReceiveType unhandledPacket;

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Application states. Function AppStateManager(..) includes the state
 * event machine.
 */
typedef enum _STATE_APP_
{
  STATE_APP_STARTUP,
  STATE_APP_IDLE,
  STATE_APP_LEARN_MODE,
  STATE_APP_RESET,
} STATE_APP;

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/**
 * Please see the description of app_node_information_t.
 */
static uint8_t cmdClassListNonSecureNotIncluded[] =
{
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_METER_V5,
  COMMAND_CLASS_CONFIGURATION_V4,
  COMMAND_CLASS_ASSOCIATION_V2,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
  COMMAND_CLASS_TRANSPORT_SERVICE_V2,
  COMMAND_CLASS_VERSION,
  COMMAND_CLASS_MANUFACTURER_SPECIFIC,
  COMMAND_CLASS_DEVICE_RESET_LOCALLY,
  COMMAND_CLASS_INDICATOR,
  COMMAND_CLASS_POWERLEVEL,
  COMMAND_CLASS_SECURITY,
  COMMAND_CLASS_SECURITY_2,
  COMMAND_CLASS_SUPERVISION,
  COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5
};

/**
 * Please see the description of app_node_information_t.
 */
static uint8_t cmdClassListNonSecureIncludedSecure[] =
{
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_TRANSPORT_SERVICE_V2,
  COMMAND_CLASS_SECURITY,
  COMMAND_CLASS_SECURITY_2,
  COMMAND_CLASS_SUPERVISION
};

/**
 * Please see the description of app_node_information_t.
 */
static uint8_t cmdClassListSecure[] =
{
  COMMAND_CLASS_VERSION,
  COMMAND_CLASS_METER_V5,
  COMMAND_CLASS_CONFIGURATION_V4,
  COMMAND_CLASS_ASSOCIATION,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_MANUFACTURER_SPECIFIC,
  COMMAND_CLASS_DEVICE_RESET_LOCALLY,
  COMMAND_CLASS_INDICATOR,
  COMMAND_CLASS_POWERLEVEL,
  COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5
};

/**
 * Structure includes application node information list's and device type.
 */
app_node_information_t m_AppNIF =
{
  cmdClassListNonSecureNotIncluded, sizeof(cmdClassListNonSecureNotIncluded),
  cmdClassListNonSecureIncludedSecure, sizeof(cmdClassListNonSecureIncludedSecure),
  cmdClassListSecure, sizeof(cmdClassListSecure),
  DEVICE_OPTIONS_MASK, {GENERIC_TYPE, SPECIFIC_TYPE}
};


/**
* Set up security keys to request when joining a network.
* These are taken from the config_app.h header file.
*/
static const uint8_t SecureKeysRequested = REQUESTED_SECURITY_KEYS;

static const SAppNodeInfo_t AppNodeInfo =
{
  .DeviceOptionsMask = DEVICE_OPTIONS_MASK,
  .NodeType.generic = GENERIC_TYPE,
  .NodeType.specific = SPECIFIC_TYPE,
  .CommandClasses.UnSecureIncludedCC.iListLength = sizeof_array(cmdClassListNonSecureNotIncluded),
  .CommandClasses.UnSecureIncludedCC.pCommandClasses = cmdClassListNonSecureNotIncluded,
  .CommandClasses.SecureIncludedUnSecureCC.iListLength = sizeof_array(cmdClassListNonSecureIncludedSecure),
  .CommandClasses.SecureIncludedUnSecureCC.pCommandClasses = cmdClassListNonSecureIncludedSecure,
  .CommandClasses.SecureIncludedSecureCC.iListLength = sizeof_array(cmdClassListSecure),
  .CommandClasses.SecureIncludedSecureCC.pCommandClasses = cmdClassListSecure
};

static const SRadioConfig_t RadioConfig =
{
  .iListenBeforeTalkThreshold = ELISTENBEFORETALKTRESHOLD_DEFAULT,
  .iTxPowerLevelMax = APP_MAX_TX_POWER,
  .iTxPowerLevelAdjust = APP_MEASURED_0DBM_TX_POWER,
  .eRegion = APP_FREQ
};

static const SProtocolConfig_t ProtocolConfig = {
  .pVirtualSlaveNodeInfoTable = NULL,
  .pSecureKeysRequested = &SecureKeysRequested,
  .pNodeInfo = &AppNodeInfo,
  .pRadioConfig = &RadioConfig
};

/**
 * Setup AGI lifeline table from config_app.h
 */
CMD_CLASS_GRP  agiTableLifeLine[] = {AGITABLE_LIFELINE_GROUP};

// Removed all references to AGI root device, since we don't associate with any
// device besides lifeline

/**************************************************************************************************
 * Configuration for Z-Wave Plus Info CC
 **************************************************************************************************
 */
static const SCCZWavePlusInfo CCZWavePlusInfo = {
                               .pEndpointIconList = NULL,
                               .roleType = APP_ROLE_TYPE,
                               .nodeType = APP_NODE_TYPE,
                               .installerIconType = APP_ICON_TYPE,
                               .userIconType = APP_USER_ICON_TYPE
};

/**
 * Application state-machine state.
 */
static STATE_APP currentState = STATE_APP_IDLE;

/**
 * Parameter is used to save wakeup reason after ApplicationInit(..)
 */
EResetReason_t g_eResetReason;

static TaskHandle_t g_AppTaskHandle;

#ifdef DEBUGPRINT
static uint8_t m_aDebugPrintBuffer[96];
#endif

// Pointer to AppHandles that is passed as input to ApplicationTask(..)
SApplicationHandles* g_pAppHandles;

// Prioritized events that can wakeup protocol thread.
typedef enum EApplicationEvent
{
  EAPPLICATIONEVENT_TIMER = 0,
  EAPPLICATIONEVENT_ZWRX,
  EAPPLICATIONEVENT_ZWCOMMANDSTATUS,
  EAPPLICATIONEVENT_APP,
  EAPPLICATIONEVENT_SERIALDATARX,   /* Register event for 'USART data available'
                                     * This will trigger the parser, which in
                                     * turn will update device state and trigger
                                     * the appropriate app events on the app
                                     * queue. */
} EApplicationEvent;

static void EventHandlerZwRx(void);
static void EventHandlerZwCommandStatus(void);
static void EventHandlerApp(void);

// Event distributor object
static SEventDistributor g_EventDistributor;

// Event distributor event handler table
static const EventDistributorEventHandler g_aEventHandlerTable[] =
{
  AppTimerNotificationHandler,  // Event 0
  EventHandlerZwRx,
  EventHandlerZwCommandStatus,
  EventHandlerApp,
  HAN_serial_rx,
};

#define APP_EVENT_QUEUE_SIZE 5

/**
 * The following four variables are used for the application event queue.
 */
static SQueueNotifying m_AppEventNotifyingQueue;
static StaticQueue_t m_AppEventQueueObject;
static EVENT_APP eventQueueStorage[APP_EVENT_QUEUE_SIZE];
static QueueHandle_t m_AppEventQueue;

/* True Status Engine (TSE) variables */
static s_CC_indicator_data_t ZAF_TSE_localActuationIdentifyData = {
  .rxOptions = {
    .rxStatus = 0,          /* rxStatus, verified by the TSE for Multicast */
    .securityKey = 0,       /* securityKey, ignored by the TSE */
    .sourceNode = {0,0},    /* sourceNode (nodeId, endpoint), verified against lifeline destinations by the TSE */
    .destNode = {0,0}       /* destNode (nodeId, endpoint), verified by the TSE for local endpoint */
  },
  .indicatorId = 0x50      /* Identify Indicator*/
};

/**
* TSE simulated RX option for local change addressed to the Root Device
* All applications can use this variable when triggering the TSE after
* a local / non Z-Wave initiated change
*/
static RECEIVE_OPTIONS_TYPE_EX zaf_tse_local_actuation = {
    0,      /* rxStatus, verified by the TSE for Multicast */
    0,      /* securityKey, ignored by the TSE */
    {0,0},  /* sourceNode (nodeId, endpoint), verified against lifeline destinations by the TSE */
    {0,0}   /* destNode (nodeId, endpoint), verified by the TSE for local endpoint */
};

static nvm3_Handle_t* pFileSystemApplication;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

// No exported data.

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

void DeviceResetLocallyDone(TRANSMISSION_RESULT * pTransmissionResult);
void DeviceResetLocally(void);
STATE_APP GetAppState(void);
void AppStateManager(EVENT_APP event);
static void ChangeState(STATE_APP newState);

static void ApplicationTask(SApplicationHandles* pAppHandles);

bool LoadConfiguration(void);
void SetDefaultConfiguration(void);

void AppResetNvm(void);

/**
* @brief Called when protocol puts a frame on the ZwRxQueue.
*/
static void EventHandlerZwRx(void)
{
  QueueHandle_t Queue = g_pAppHandles->ZwRxQueue;
  SZwaveReceivePackage RxPackage;

  // Handle incoming replies
  while (xQueueReceive(Queue, (uint8_t*)(&RxPackage), 0) == pdTRUE)
  {
    DPRINTF("Incoming Rx msg %d\r\n", RxPackage.eReceiveType);

    switch (RxPackage.eReceiveType)
    {
      case EZWAVERECEIVETYPE_SINGLE:
      {
        ZAF_CP_CommandPublish(ZAF_getCPHandle(), &RxPackage);
        break;
      }

      case EZWAVERECEIVETYPE_NODE_UPDATE:
      {
        /*ApplicationSlaveUpdate() was called from this place in version prior to SDK7*/
        break;
      }

      case EZWAVERECEIVETYPE_SECURITY_EVENT:
      {
        /*ApplicationSecurityEvent() was used to support CSA, not needed in SDK7*/
        break;
      }

    default:
      {
        unhandledPacket = RxPackage.eReceiveType;
        ZAF_EventHelperEventEnqueue(EVENT_APP_UNHANDLED_PACKET);
        break;
      }
    }
  }

}


/**
* @brief Triggered when protocol puts a message on the ZwCommandStatusQueue.
*/
static void EventHandlerZwCommandStatus(void)
{
  QueueHandle_t Queue = g_pAppHandles->ZwCommandStatusQueue;
  SZwaveCommandStatusPackage Status;

  // Handle incoming replies
  while (xQueueReceive(Queue, (uint8_t*)(&Status), 0) == pdTRUE)
  {
    switch (Status.eStatusType)
    {
      case EZWAVECOMMANDSTATUS_TX:
      {
        SZWaveTransmitStatus* pTxStatus = &Status.Content.TxStatus;
        if (!pTxStatus->bIsTxFrameLegal)
        {
          DPRINT("Auch - not sure what to do\r\n");
        }
        else
        {
          DPRINT("Tx Status received\r\n");
          if (ZAF_TSE_TXCallback == pTxStatus->Handle)
          {
            // TSE do not use the TX result to anything. Hence, we pass NULL.
            ZAF_TSE_TXCallback(NULL);
          }
          else if (pTxStatus->Handle)
          {
            void(*pCallback)(uint8_t txStatus, TX_STATUS_TYPE* extendedTxStatus) = pTxStatus->Handle;
            pCallback(pTxStatus->TxStatus, &pTxStatus->ExtendedTxStatus);
          }
        }

        break;
      }

      case EZWAVECOMMANDSTATUS_GENERATE_RANDOM:
      {
        DPRINT("Generate Random status\r\n");
        break;
      }

      case EZWAVECOMMANDSTATUS_LEARN_MODE_STATUS:
      {
        DPRINTF("Learn status %d\r\n", Status.Content.LearnModeStatus.Status);
        if (ELEARNSTATUS_ASSIGN_COMPLETE == Status.Content.LearnModeStatus.Status)
        {
          // When security S0 or higher is set, remove all settings which happen before secure inclusion
          // calling function SetDefaultConfiguration(). The same function is used when there is an
          // EINCLUSIONSTATE_EXCLUDED.
          if ( (EINCLUSIONSTATE_EXCLUDED == ZAF_GetInclusionState()) ||
                      (SECURITY_KEY_NONE != GetHighestSecureLevel(ZAF_GetSecurityKeys())) )
          {
            SetDefaultConfiguration();
          }
          ZAF_EventHelperEventEnqueue((EVENT_APP) EVENT_SYSTEM_LEARNMODE_FINISHED);
          ZAF_Transport_OnLearnCompleted();
        }
        else if(ELEARNSTATUS_SMART_START_IN_PROGRESS == Status.Content.LearnModeStatus.Status)
        {
          ZAF_EventHelperEventEnqueue(EVENT_APP_SMARTSTART_IN_PROGRESS);
        }
        else if(ELEARNSTATUS_LEARN_IN_PROGRESS == Status.Content.LearnModeStatus.Status)
        {
          ZAF_EventHelperEventEnqueue(EVENT_APP_LEARN_IN_PROGRESS);
        }
        else if(ELEARNSTATUS_LEARN_MODE_COMPLETED_TIMEOUT == Status.Content.LearnModeStatus.Status)
        {
          ZAF_EventHelperEventEnqueue((EVENT_APP) EVENT_SYSTEM_LEARNMODE_FINISHED);
        }
        else if(ELEARNSTATUS_LEARN_MODE_COMPLETED_FAILED == Status.Content.LearnModeStatus.Status)
        {
          //Reformats protocol and application NVM. Then soft reset.
          ZAF_EventHelperEventEnqueue((EVENT_APP) EVENT_SYSTEM_RESET);
        }
        break;
      }

      case EZWAVECOMMANDSTATUS_NETWORK_LEARN_MODE_START:
      {
        DPRINT("Started network learn mode\n");
        break;
      }

      case EZWAVECOMMANDSTATUS_SET_DEFAULT:
      { // Received when protocol is started (not implemented yet), and when SetDefault command is completed
        DPRINTF("Protocol Ready\r\n");
        ZAF_EventHelperEventEnqueue(EVENT_APP_FLUSHMEM_READY);
        break;
      }

      case EZWAVECOMMANDSTATUS_INVALID_TX_REQUEST:
      {
        DPRINTF("ERROR: Invalid TX Request to protocol - %d", Status.Content.InvalidTxRequestStatus.InvalidTxRequest);
        break;
      }

      case EZWAVECOMMANDSTATUS_INVALID_COMMAND:
      {
        DPRINTF("ERROR: Invalid command to protocol - %d", Status.Content.InvalidCommandStatus.InvalidCommand);
        break;
      }

      case EZWAVECOMMANDSTATUS_ZW_SET_MAX_INCL_REQ_INTERVALS:
      {
        // Status response from calling the ZAF_SetMaxInclusionRequestIntervals function
        DPRINTF("SetMaxInclusionRequestIntervals status: %s\r\n",
                 Status.Content.NetworkManagementStatus.statusInfo[0] == true ? "SUCCESS" : "FAIL");

        // Add your application code here...
        break;
      }

      case EZWAVECOMMANDSTATUS_NODE_INFO:
      case EZWAVECOMMANDSTATUS_SET_RF_RECEIVE_MODE:
      case EZWAVECOMMANDSTATUS_IS_NODE_WITHIN_DIRECT_RANGE:
      case EZWAVECOMMANDSTATUS_GET_NEIGHBOR_COUNT:
      case EZWAVECOMMANDSTATUS_ARE_NODES_NEIGHBOURS:
      case EZWAVECOMMANDSTATUS_IS_FAILED_NODE_ID:
      case EZWAVECOMMANDSTATUS_GET_ROUTING_TABLE_LINE:
      case EZWAVECOMMANDSTATUS_SET_ROUTING_INFO:
      case EZWAVECOMMANDSTATUS_STORE_NODE_INFO:
      case EZWAVECOMMANDSTATUS_GET_PRIORITY_ROUTE:
      case EZWAVECOMMANDSTATUS_SET_PRIORITY_ROUTE:
      case EZWAVECOMMANDSTATUS_SET_SLAVE_LEARN_MODE:
      case EZWAVECOMMANDSTATUS_SET_SLAVE_LEARN_MODE_RESULT:
      case EZWAVECOMMANDSTATUS_IS_VIRTUAL_NODE:
      case EZWAVECOMMANDSTATUS_GET_VIRTUAL_NODES:
      case EZWAVECOMMANDSTATUS_GET_CONTROLLER_CAPABILITIES:
      case EZWAVECOMMANDSTATUS_IS_PRIMARY_CTRL:
      case EZWAVECOMMANDSTATUS_NETWORK_MANAGEMENT:
      case EZWAVECOMMANDSTATUS_GET_BACKGROUND_RSSI:
      case EZWAVECOMMANDSTATUS_AES_ECB:
      case EZWAVECOMMANDSTATUS_REMOVE_FAILED_NODE_ID:
      case EZWAVECOMMANDSTATUS_REPLACE_FAILED_NODE_ID:
      case EZWAVECOMMANDSTATUS_NVM_BACKUP_RESTORE:
      case EZWAVECOMMANDSTATUS_PM_SET_POWERDOWN_CALLBACK:
      case EZWAVECOMMANDSTATUS_ZW_GET_INCLUDED_NODES:
      case EZWAVECOMMANDSTATUS_ZW_REQUESTNODENEIGHBORUPDATE:
      case EZWAVECOMMANDSTATUS_ZW_INITIATE_SHUTDOWN:
      case EZWAVECOMMANDSTATUS_ZW_GET_INCLUDED_LR_NODES:
      case EZWAVECOMMANDSTATUS_ZW_GET_LR_CHANNEL:
        unhandledCommandStatus = Status.eStatusType;
        ZAF_EventHelperEventEnqueue(EVENT_APP_UNHANDLED_STATUS);
        break;

      default:
        ASSERT(false);
        break;
    }
  }
}

/**
 *
 */
static void EventHandlerApp(void)
{
//  DPRINT("Got event!\r\n");

  uint8_t event;

  while (xQueueReceive(m_AppEventQueue, (uint8_t*)(&event), 0) == pdTRUE)
  {
    DPRINTF("Event: %d\r\n", event);
    AppStateManager((EVENT_APP)event);
  }
}

/*
 * Initialized all modules related to event queuing, task notifying and job registering.
 */
static void EventQueueInit()
{
  // Initialize Queue Notifier for events in the application.
  m_AppEventQueue = xQueueCreateStatic(
    sizeof_array(eventQueueStorage),
    sizeof(eventQueueStorage[0]),
    (uint8_t*)eventQueueStorage,
    &m_AppEventQueueObject
  );

  /*
   * Registers events with associated data, and notifies
   * the specific task about a pending job!
   */
  QueueNotifyingInit(
      &m_AppEventNotifyingQueue,
      m_AppEventQueue,
      g_AppTaskHandle,
      EAPPLICATIONEVENT_APP);

  /*
   * Wraps the QueueNotifying module for simple event generations!
   */
  ZAF_EventHelperInit(&m_AppEventNotifyingQueue);

  /*
   * Creates an internal queue to store no more than @ref JOB_QUEUE_BUFFER_SIZE jobs.
   * This module has no notification feature!
   */
  ZAF_JobHelperInit();
}

static void printDSK(void)
{
  // address taken from https://www.silabs.com/community/wireless/z-wave/knowledge-base.entry.html/2019/01/04/z-wave_700_how_tor-RQy8
  uint8_t* dskptr = (uint8_t*)0x0FE043CCUL;

  DPRINT("DSK: \n");
  for(size_t i = 0; i < 16; i+=2) {
    uint16_t element = dskptr[i] << 8 | dskptr[i+1];
    DPRINTF("    %05u", element);
    if(i < 14) {
      DPRINT("-\n");
    }
  }
  DPRINT("\n");
}

/**
 * @brief See description for function prototype in ZW_basis_api.h.
 */
ZW_APPLICATION_STATUS
ApplicationInit(EResetReason_t eResetReason)
{
  // NULL - We dont have the Application Task handle yet
  AppTimerInit(EAPPLICATIONEVENT_TIMER, NULL);

  g_eResetReason = eResetReason;

  /* hardware initialization */
  Board_Init();
  BRD420xBoardInit(RadioConfig.eRegion);

#ifdef DEBUGPRINT
  ZAF_UART0_enable(115200, true, true);
  DebugPrintConfig(m_aDebugPrintBuffer, sizeof(m_aDebugPrintBuffer), ZAF_UART0_tx_send);
#endif

  /* Init state machine*/
  currentState = STATE_APP_STARTUP;

  DPRINT("\n\n--------------------------------\n");
  DPRINT("AMS2ZWAVE\n");
  DPRINTF("SDK: %d.%d.%d ZAF: %d.%d.%d.%d [Freq: %d]\n",
          SDK_VERSION_MAJOR,
          SDK_VERSION_MINOR,
          SDK_VERSION_PATCH,
          ZAF_GetAppVersionMajor(),
          ZAF_GetAppVersionMinor(),
          ZAF_GetAppVersionPatchLevel(),
          ZAF_BUILD_NO,
          RadioConfig.eRegion);
  printDSK();
  DPRINT("--------------------------------\n");
  DPRINTF("%s: Toggle learn mode\n", Board_GetButtonLabel(APP_BUTTON_LEARN_RESET));
  DPRINT("      Hold 5 sec: Reset\n");
  DPRINTF("%s: Learn mode + identify + activity\n", Board_GetLedLabel(APP_LED_INDICATOR));
  DPRINT("--------------------------------\n\n");

  DPRINTF("ApplicationInit eResetReason = %d\n", g_eResetReason);

  CC_ZWavePlusInfo_Init(&CCZWavePlusInfo);

  CC_Version_SetApplicationVersionInfo(ZAF_GetAppVersionMajor(),
                                       ZAF_GetAppVersionMinor(),
                                       ZAF_GetAppVersionPatchLevel(),
                                       ZAF_BUILD_NO);

  // AMS2ZWAVE ACTION: Initialize HAN serial port & parser
  // logic is deferred to after the application task is created and scheduled,
  // since a serial interrupt could try to put events on the app queue.

  /* Register task function */
  bool bWasTaskCreated = ZW_ApplicationRegisterTask(
                                                    ApplicationTask,
                                                    EAPPLICATIONEVENT_ZWRX,
                                                    EAPPLICATIONEVENT_ZWCOMMANDSTATUS,
                                                    &ProtocolConfig
                                                    );
  ASSERT(bWasTaskCreated);

  return(APPLICATION_RUNNING);
}

/**
 * A pointer to this function is passed to ZW_ApplicationRegisterTask() making it the FreeRTOS
 * application task.
 */
static void
ApplicationTask(SApplicationHandles* pAppHandles)
{
  // Init
  DPRINT("Enabling watchdog\n");
  WDOGn_Enable(DEFAULT_WDOG, true);

  g_AppTaskHandle = xTaskGetCurrentTaskHandle();
  g_pAppHandles = pAppHandles;
  AppTimerSetReceiverTask(g_AppTaskHandle);

  ZAF_Init(g_AppTaskHandle, pAppHandles, &ProtocolConfig, NULL);

  /*
   * Create an initialize some of the modules regarding queue and event handling and passing.
   * The UserTask that is dependent on modules initialized here, must be able to detect and wait
   * before using these modules. Specially if it has higher priority than this task!
   *
   * Currently, the UserTask is checking whether zaf_event_helper.h module is ready.
   * This module is the last to be initialized!
   */
  EventQueueInit();

  ZAF_EventHelperEventEnqueue(EVENT_APP_INIT);

  Board_EnableButton(APP_BUTTON_LEARN_RESET);
  Board_IndicatorInit(APP_LED_INDICATOR);
  Board_IndicateStatus(BOARD_STATUS_IDLE);

  CommandClassSupervisionInit(
      CC_SUPERVISION_STATUS_UPDATES_NOT_SUPPORTED,
      NULL,
      NULL);

  EventDistributorConfig(
    &g_EventDistributor,
    sizeof_array(g_aEventHandlerTable),
    g_aEventHandlerTable,
    NULL
    );

  // Wait for and process events
  DPRINT("AMS2ZWAVE Event processor Started\r\n");
  uint32_t iMaxTaskSleep = 10;
  for (;;)
  {
    EventDistributorDistribute(&g_EventDistributor, iMaxTaskSleep, 0);
  }
}

/**
 * @brief See description for function prototype in ZW_TransportEndpoint.h.
 */
received_frame_status_t
Transport_ApplicationCommandHandlerEx(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength)
{
  received_frame_status_t frame_status = RECEIVED_FRAME_STATUS_NO_SUPPORT;
  DPRINTF("\r\nTAppH %u\r\n", pCmd->ZW_Common.cmdClass);


  /* Call command class handlers */
  switch (pCmd->ZW_Common.cmdClass)
  {
    case COMMAND_CLASS_VERSION:
      frame_status = handleCommandClassVersion(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_ASSOCIATION_GRP_INFO:
      frame_status = handleCommandClassAssociationGroupInfo( rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_ASSOCIATION:
      frame_status = handleCommandClassAssociation(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_INDICATOR:
      frame_status = handleCommandClassIndicator(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_POWERLEVEL:
      frame_status = handleCommandClassPowerLevel(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_MANUFACTURER_SPECIFIC:
      frame_status = handleCommandClassManufacturerSpecific(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_ZWAVEPLUS_INFO:
      frame_status = handleCommandClassZWavePlusInfo(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2:
      frame_status = handleCommandClassMultiChannelAssociation(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_SUPERVISION:
      frame_status = handleCommandClassSupervision(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_SECURITY:
    case COMMAND_CLASS_SECURITY_2:
      frame_status = handleCommandClassSecurity(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5:
      frame_status = handleCommandClassFWUpdate(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_METER_V5:
      frame_status = handleCommandClassMeter(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_CONFIGURATION_V4:
      frame_status = handleCommandClassConfiguration(rxOpt, pCmd, cmdLength);
      break;
  }
  return frame_status;
}

/**
 * @brief Returns the current state of the application state machine.
 * @return Current state
 */
STATE_APP
GetAppState(void)
{
  return currentState;
}

static void doRemainingInitialization()
{
  /* Load the application settings from NVM3 file system */
  LoadConfiguration();

  // Setup AGI group lists
  AGI_Init();
  CC_AGI_LifeLineGroupSetup(agiTableLifeLine, (sizeof(agiTableLifeLine)/sizeof(CMD_CLASS_GRP)), ENDPOINT_ROOT );

  /*
   * Initialize Event Scheduler.
   */
  Transport_OnApplicationInitSW( &m_AppNIF, NULL);

  /* Enter SmartStart*/
  /* Protocol will commence SmartStart only if the node is NOT already included in the network */
  ZAF_setNetworkLearnMode(E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART, g_eResetReason);

  /* Init state machine*/
  ZAF_EventHelperEventEnqueue(EVENT_EMPTY);
}

/**
 * @brief The core state machine of this sample application.
 * @param event The event that triggered the call of AppStateManager.
 */
void
AppStateManager(EVENT_APP event)
{
  DPRINTF("AppStateManager St: %d, Ev: %d\r\n", currentState, event);

  if ((BTN_EVENT_LONG_PRESS(APP_BUTTON_LEARN_RESET) == (BUTTON_EVENT)event) ||
      (EVENT_SYSTEM_RESET == (EVENT_SYSTEM)event))
  {
    /*Force state change to activate system-reset without taking care of current state.*/
    ChangeState(STATE_APP_RESET);
    /* Send reset notification*/
    DeviceResetLocally();
  }

  switch(currentState)
  {

    case STATE_APP_STARTUP:
      if(EVENT_APP_FLUSHMEM_READY == event)
      {
        AppResetNvm();
      }

      if (EVENT_APP_INIT == event)
      {
        doRemainingInitialization();

        HAN_setup();
        break;
      }
      CC_FirmwareUpdate_Init(NULL, NULL, NULL, true);
      ChangeState(STATE_APP_IDLE);
      break;

    case STATE_APP_IDLE:
      if(EVENT_APP_REFRESH_MMI == event)
      {
        Board_IndicateStatus(BOARD_STATUS_IDLE);
        break;
      }

      if(EVENT_APP_FLUSHMEM_READY == event)
      {
        DPRINT("Flushmem\n");
        AppResetNvm();
        LoadConfiguration();
      }

      if(EVENT_APP_SMARTSTART_IN_PROGRESS == event)
      {
        DPRINT("SS in progress\n");
        ChangeState(STATE_APP_LEARN_MODE);
      }

      /**************************************************************************************
       * Learn/Reset button
       **************************************************************************************
       */
      if (BTN_EVENT_SHORT_PRESS(APP_BUTTON_LEARN_RESET) == (BUTTON_EVENT)event ||
          (EVENT_SYSTEM_LEARNMODE_START == (EVENT_SYSTEM)event))
      {
        if (EINCLUSIONSTATE_EXCLUDED != g_pAppHandles->pNetworkInfo->eInclusionState)
        {
          DPRINT("LEARN_MODE_EXCLUSION\r\n");
          ZAF_setNetworkLearnMode(E_NETWORK_LEARN_MODE_EXCLUSION_NWE, g_eResetReason);
        }
        else
        {
          DPRINT("LEARN_MODE_INCLUSION\r\n");
          ZAF_setNetworkLearnMode(E_NETWORK_LEARN_MODE_INCLUSION, g_eResetReason);
        }
        ChangeState(STATE_APP_LEARN_MODE);
      }

      bool send_power_report = false;
      // ACTION: AMS2ZWAVE list 1 received (2.5s interval)
      if (EVENT_APP_POWER_UPDATE_FAST == event ||
          EVENT_APP_POWER_UPDATE_SLOW == event ||
          EVENT_APP_ENERGY_UPDATE == event) {
        if( CC_ConfigurationData.power_change_for_meter_report > 0 ) {
          uint32_t watt_trigger = CC_ConfigurationData.power_change_for_meter_report * 100;
          if( active_power_watt > last_reported_power_watt + watt_trigger ||
              ((last_reported_power_watt >= watt_trigger) && (active_power_watt < last_reported_power_watt - watt_trigger)) ) {
            send_power_report = true;
          }
        }
      }

      // ACTION: AMS2ZWAVE list 2 received (10s interval)
      if (EVENT_APP_POWER_UPDATE_SLOW == event ||
          EVENT_APP_ENERGY_UPDATE == event) {
        // 'slow' updates still come in every 10 seconds. Considering the low
        // throughput of a z-wave network, reporting every 30s is more than
        // plenty (and maybe still unwanted).
        static uint8_t iterations = 0;
        if(CC_ConfigurationData.amount_of_10s_reports_for_meter_report > 0) {
          iterations++;
          if(iterations >= CC_ConfigurationData.amount_of_10s_reports_for_meter_report) {
              send_power_report = true;
              iterations = 0;
          }
        }
      }

      // ACTION: report on hourly update
      if (EVENT_APP_ENERGY_UPDATE == event) {
        if(CC_ConfigurationData.enable_hourly_report == 1) {
          CC_Meter_update_energy();
        }
      }

      if (send_power_report) {
        CC_Meter_update_power();
      }

      if (EVENT_APP_UNHANDLED_STATUS == event) {
          DPRINTF("Unimplemented command status handler for response %d\r\n", unhandledCommandStatus);
#ifndef NDEBUG
          // Signal unhandled type remotely by sending a report on the currently
          // non-reporting voltage value
          void * pData = CC_Meter_prepare_zaf_tse_data(&zaf_tse_local_actuation);
          ZAF_TSE_Trigger((void *)CC_Meter_report_unhandled_as_voltage, pData, true);
#endif
      }

      if (EVENT_APP_UNHANDLED_PACKET == event) {
          DPRINTF("Failed to handle incoming msg with type %d\n", unhandledPacket);
#ifndef NDEBUG
          // Signal unhandled type remotely by sending a report on the currently
          // non-reporting voltage value
          void * pData = CC_Meter_prepare_zaf_tse_data(&zaf_tse_local_actuation);
          ZAF_TSE_Trigger((void *)CC_Meter_report_unhandled_as_current, pData, true);
#endif
      }
      break;

    case STATE_APP_LEARN_MODE:
      if (EVENT_APP_REFRESH_MMI == event)
      {
        Board_IndicateStatus(BOARD_STATUS_LEARNMODE_ACTIVE);
      }

      if(EVENT_APP_FLUSHMEM_READY == event)
      {
        DPRINT("Learn flushmem\n");
        AppResetNvm();
        LoadConfiguration();
      }

      if (BTN_EVENT_SHORT_PRESS(APP_BUTTON_LEARN_RESET) == (BUTTON_EVENT)event ||
          (EVENT_SYSTEM_LEARNMODE_STOP == (EVENT_SYSTEM)event))
      {
        DPRINT("\r\nLEARN MODE DISABLE\r\n");
        ZAF_setNetworkLearnMode(E_NETWORK_LEARN_MODE_DISABLE, g_eResetReason);

        //Go back to smart start if the node was never included.
        //Protocol will not commence SmartStart if the node is already included in the network.
        ZAF_setNetworkLearnMode(E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART, g_eResetReason);

        Board_IndicateStatus(BOARD_STATUS_IDLE);
        ChangeState(STATE_APP_IDLE);

        /* If we are in a network and the Identify LED state was changed to idle due to learn mode, report it to lifeline */
        CC_Indicator_RefreshIndicatorProperties();
        ZAF_TSE_Trigger((void *)CC_Indicator_report_stx, &ZAF_TSE_localActuationIdentifyData, true);
      }

      if (EVENT_SYSTEM_LEARNMODE_FINISHED == (EVENT_SYSTEM)event)
      {
        //Go back to smart start if the node was excluded.
        //Protocol will not commence SmartStart if the node is already included in the network.
        DPRINT("retrigger SS\n");
        ZAF_setNetworkLearnMode(E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART, g_eResetReason);

        DPRINT("\r\nLEARN MODE FINISH\r\n");
        ChangeState(STATE_APP_IDLE);

        /* If we are in a network and the Identify LED state was changed to idle due to learn mode, report it to lifeline */
        CC_Indicator_RefreshIndicatorProperties();
        ZAF_TSE_Trigger((void *)CC_Indicator_report_stx, &ZAF_TSE_localActuationIdentifyData, true);
      }
      break;

    case STATE_APP_RESET:
      if(EVENT_APP_REFRESH_MMI == event){}

      if(EVENT_APP_FLUSHMEM_READY == event)
      {
        DPRINT("App reset state\n");
        AppResetNvm();
        /* Soft reset */
        Board_ResetHandler();
      }
      break;

    default:
      // Do nothing.
      break;
  }
}

/**
 * @brief Sets the current state to a new, given state.
 * @param newState New state.
 */
static void
ChangeState(STATE_APP newState)
{
  DPRINTF("State changed from %d to %d\r\n", currentState, newState);

  currentState = newState;

  /**< Pre-action on new state is to refresh MMI */
  ZAF_EventHelperEventEnqueue(EVENT_APP_REFRESH_MMI);
}

/**
 * @brief Transmission callback for Device Reset Locally call.
 * @param pTransmissionResult Result of each transmission.
 */
void
DeviceResetLocallyDone(TRANSMISSION_RESULT * pTransmissionResult)
{
  if (TRANSMISSION_RESULT_FINISHED == pTransmissionResult->isFinished)
  {
    /* Reset protocol */
    // Set default command to protocol
    SZwaveCommandPackage CommandPackage;
    CommandPackage.eCommandType = EZWAVECOMMANDTYPE_SET_DEFAULT;

    DPRINT("\nDisabling watchdog during reset\n");
    WDOGn_Enable(DEFAULT_WDOG, false);

    EQueueNotifyingStatus Status = QueueNotifyingSendToBack(g_pAppHandles->pZwCommandQueue,
                                                            (uint8_t*)&CommandPackage,
                                                            500);
    ASSERT(EQUEUENOTIFYING_STATUS_SUCCESS == Status);
  }
}

/**
 * @brief Send reset notification.
 */
void
DeviceResetLocally(void)
{
  AGI_PROFILE lifelineProfile = {
      ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL,
      ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_LIFELINE
  };
  DPRINT("Call locally reset\r\n");

  CC_DeviceResetLocally_notification_tx(&lifelineProfile, DeviceResetLocallyDone);
}

/**
 * @brief See description for function prototype in CC_Version.h.
 */
uint8_t
CC_Version_getNumberOfFirmwareTargets_handler(void)
{
  return 1; /*CHANGE THIS - firmware 0 version*/
}

/**
 * @brief See description for function prototype in CC_Version.h.
 */
void
handleGetFirmwareVersion(
  uint8_t bFirmwareNumber,
  VG_VERSION_REPORT_V2_VG *pVariantgroup)
{
  /*firmware 0 version and sub version*/
  if(bFirmwareNumber == 0)
  {
    pVariantgroup->firmwareVersion = ZAF_GetAppVersionMajor();
    pVariantgroup->firmwareSubVersion = ZAF_GetAppVersionMinor();
  }
  else
  {
    /*Just set it to 0 if firmware n is not present*/
    pVariantgroup->firmwareVersion = 0;
    pVariantgroup->firmwareSubVersion = 0;
  }
}

/**
 * @brief Function resets configuration to default values.
 *
 * Add application specific functions here to initialize configuration values stored in persistent memory.
 * Will be called at any of the following events:
 *  - Network Exclusion
 *  - Network Secure Inclusion (after S2 bootstrapping complete)
 *  - Device Reset Locally
 */
void
SetDefaultConfiguration(void)
{
  AssociationInit(true, pFileSystemApplication);

  CC_Configuration_resetToDefault(pFileSystemApplication);

  // Reset persistent meter values
  HAN_resetNVM();

  loadInitStatusPowerLevel();

  uint32_t appVersion = ZAF_GetAppVersion();
  Ecode_t result = nvm3_writeData(pFileSystemApplication, ZAF_FILE_ID_APP_VERSION, &appVersion, ZAF_FILE_SIZE_APP_VERSION);

  ASSERT(ECODE_NVM3_OK == result);
}

/**
 * @brief This function loads the application settings from file system.
 * If no settings are found, default values are used and saved.
 */
bool
LoadConfiguration(void)
{
  // Init file system
  ApplicationFileSystemInit(&pFileSystemApplication);

  uint32_t appVersion;
  Ecode_t versionFileStatus = nvm3_readData(pFileSystemApplication, ZAF_FILE_ID_APP_VERSION, &appVersion, ZAF_FILE_SIZE_APP_VERSION);

  if (ECODE_NVM3_OK == versionFileStatus)
  {
    if (ZAF_GetAppVersion() != appVersion)
    {
      // Add code for migration of file system to higher version here.
    }

    CC_Configuration_loadFromNVM(pFileSystemApplication);

    /* Initialize association module */
    AssociationInit(false, pFileSystemApplication);

    /* Load NVM variables for HAN meter */
    HAN_loadFromNVM();
    return true;
  }
  else
  {
    DPRINT("Application FileSystem Verify failed\r\n");
    loadInitStatusPowerLevel();

    // Reset the file system if ZAF_FILE_ID_APP_VERSION is missing since this indicates
    // corrupt or missing file system.
    AppResetNvm();

    return false;
  }
}

void AppResetNvm(void)
{
  DPRINT("Resetting application FileSystem to default\r\n");

  ASSERT(0 != pFileSystemApplication); //Assert has been kept for debugging , can be removed from production code. This error can only be caused by some internal flash HW failure

  Ecode_t errCode = nvm3_eraseAll(pFileSystemApplication);
  ASSERT(ECODE_NVM3_OK == errCode); //Assert has been kept for debugging , can be removed from production code. This error can only be caused by some internal flash HW failure

  /* Apparently there is no valid configuration in file system, so load */
  /* default values and save them to file system. */
  SetDefaultConfiguration();
}

// AMS2ZWAVE: stripped all of the scene controller functionality from the sample app

// AMS2ZWAVE: stripped out 'ApplicationData' handling in favor of Configuration CC

uint16_t handleFirmWareIdGet(uint8_t n)
{
  if (n == 0)
  {
    // Firmware 0
    return APP_FIRMWARE_ID;
  }
  // Invalid Firmware number.
  return 0;
}

uint8_t CC_Version_GetHardwareVersion_handler(void)
{
  return 1;
}

void CC_ManufacturerSpecific_ManufacturerSpecificGet_handler(uint16_t * pManufacturerID,
                                                             uint16_t * pProductID)
{
  *pManufacturerID = APP_MANUFACTURER_ID;
  *pProductID = APP_PRODUCT_ID;
}

/*
 * This function will report a serial number in a binary format according to the specification.
 * The serial number is the chip serial number and can be verified using the Simplicity Commander.
 * The choice of reporting can be changed in accordance with the Manufacturer Specific
 * Command Class specification.
 */
void CC_ManufacturerSpecific_DeviceSpecificGet_handler(device_id_type_t * pDeviceIDType,
                                                       device_id_format_t * pDeviceIDDataFormat,
                                                       uint8_t * pDeviceIDDataLength,
                                                       uint8_t * pDeviceIDData)
{
  *pDeviceIDType = DEVICE_ID_TYPE_SERIAL_NUMBER;
  *pDeviceIDDataFormat = DEVICE_ID_FORMAT_BINARY;
  *pDeviceIDDataLength = 8;
  uint64_t uuID = SYSTEM_GetUnique();
  DPRINTF("\r\nuuID: %x", (uint32_t)uuID);
  *(pDeviceIDData + 0) = (uint8_t)(uuID >> 56);
  *(pDeviceIDData + 1) = (uint8_t)(uuID >> 48);
  *(pDeviceIDData + 2) = (uint8_t)(uuID >> 40);
  *(pDeviceIDData + 3) = (uint8_t)(uuID >> 32);
  *(pDeviceIDData + 4) = (uint8_t)(uuID >> 24);
  *(pDeviceIDData + 5) = (uint8_t)(uuID >> 16);
  *(pDeviceIDData + 6) = (uint8_t)(uuID >>  8);
  *(pDeviceIDData + 7) = (uint8_t)(uuID >>  0);
}


/*******************************************************************************
 * AMS2ZWAVE: HAN handling from here on down
 ******************************************************************************/
/* Set up a double-buffered USART RX for receiving HAN frames
 *
 * Concept: At any given point in time, there's an active buffer and a shadow
 * buffer. The USART RX ISR is always pointed at the active buffer, and will
 * 'dumbly' fill the active buffer until it runs out of space, after which point
 * it will simply drop the incoming bytes.
 * This is done both for speed, and to avoid synchronisation constructs between
 * the application and the ISR.
 *
 * The application side gets notified by the ISR when the ISR starts filling an
 * active buffer which previously was empty.
 *
 * The application will then, at its own leisure, get around to handling the
 * notification. At that point, the application swaps the buffers around by
 * pointing the ISR at the other buffer, making the shadow buffer the new
 * (empty) active buffer, and the previously-active buffer the shadow buffer.
 * The application can then parse the contents of the shadow buffer independent
 * of what the ISR is doing.
 *
 * In this way, the distribution between read and write operations is:
 * * From ISR:
 *   - Active buffer data: W
 *   - Active buffer size: R/W
 *   - Shadow buffer data: not accessed
 *   - Shadow buffer size: not accessed
 *   - buffer pointer: R
 * * From application:
 *   - Active buffer data: not accessed
 *   - Active buffer size: not accessed
 *   - Shadow buffer data: R/W
 *   - Shadow buffer size: R/W
 *   - buffer pointer: R/W
 *
 * Meaning no potential for write conflicts between application and ISR, and
 * therefore not required to fiddle with critical sections or IRQ priorities.
 */
#define BUFFERSIZE           512

volatile struct hanBuffer
{
  volatile uint8_t  data[2][BUFFERSIZE];
  volatile size_t   data_size[2];   /* Amount of bytes in the buffer. If the
                                     * application is done reading a buffer,
                                     * this needs to be set to all-FF. */
  volatile size_t   active_buffer;  /* which buffer index is currently active */
} rxBuf = { { {0}, {0} }, { 0xFFFFFFFFUL, 0xFFFFFFFFUL }, 0 };

// The interrupt handler fills the 'active buffer' until it runs out of space
// The application is responsible for swapping the buffers fast enough to
// prevent the buffer filling from dropping bytes.

// Allow HAN input on USART0 (debug USART)
void USART0_RX_IRQHandler(void)
{
  /* Act on RX data valid interrupt */
  while (USART0->STATUS & USART_STATUS_RXDATAV)
  {
    uint8_t active_buffer = rxBuf.active_buffer;
    size_t current_size = rxBuf.data_size[active_buffer];
    uint8_t data_byte = USART_Rx(USART0);

    // If we've been pointed at an empty buffer, start filling it and notify the
    // application there's data to be had.
    if(current_size == 0xFFFFFFFFUL) {
      rxBuf.data[active_buffer][0] = data_byte;
      rxBuf.data_size[active_buffer] = 1;

      xTaskNotifyFromISR(g_AppTaskHandle,
                         1 << EAPPLICATIONEVENT_SERIALDATARX,
                         eSetBits,
                         NULL);
      continue;
    }

    // If we've been pointed at a buffer in progress, fill it up as long as
    // there's room
    if(current_size < sizeof(rxBuf.data[0])) {
      rxBuf.data[active_buffer][current_size] = data_byte;
      rxBuf.data_size[active_buffer] = current_size + 1;
      continue;
    }

    // When ending up down here, it means we've ran out of buffer to write bytes
    // into. Too bad. Flag this condition to the application by setting the size
    // to the buffer size + 1;
    if(current_size == sizeof(rxBuf.data[0])) {
      rxBuf.data_size[active_buffer] = current_size + 1;
      continue;
    }
  }
}

// Allow HAN input on USART1, too
void USART1_RX_IRQHandler(void)
{
  /* Act on RX data valid interrupt */
  while (USART1->STATUS & USART_STATUS_RXDATAV)
  {
    uint8_t active_buffer = rxBuf.active_buffer;
    size_t current_size = rxBuf.data_size[active_buffer];
    uint8_t data_byte = USART_Rx(USART1);

    // If we've been pointed at an empty buffer, start filling it and notify the
    // application there's data to be had.
    if(current_size == 0xFFFFFFFFUL) {
      rxBuf.data[active_buffer][0] = data_byte;
      rxBuf.data_size[active_buffer] = 1;

      xTaskNotifyFromISR(g_AppTaskHandle,
                         1 << EAPPLICATIONEVENT_SERIALDATARX,
                         eSetBits,
                         NULL);
      continue;
    }

    // If we've been pointed at a buffer in progress, fill it up as long as
    // there's room
    if(current_size < sizeof(rxBuf.data[0])) {
      rxBuf.data[active_buffer][current_size] = data_byte;
      rxBuf.data_size[active_buffer] = current_size + 1;
      continue;
    }

    // When ending up down here, it means we've ran out of buffer to write bytes
    // into. Too bad. Flag this condition to the application by setting the size
    // to the buffer size + 1;
    if(current_size == sizeof(rxBuf.data[0])) {
      rxBuf.data_size[active_buffer] = current_size + 1;
      continue;
    }
  }
}

// The system will call this function at its own pace when poked by the ISR.
// Swapping the buffer needs to be done before the incoming data stream can
// manage to fill it up, so how much time you have would be a function of how
// quickly you can parse the data, how big the buffers are, and how fast the
// data is coming in.
void HAN_serial_rx(void)
{
  // Perform a buffer swap before parsing the data
  uint8_t read_buffer = rxBuf.active_buffer;
  rxBuf.active_buffer = (read_buffer + 1) & 0x1UL;

  // Pump all bytes from what was the active buffer
  size_t bytes = rxBuf.data_size[read_buffer];

  //DPRINTF("Pumping %d bytes\n", bytes);
  for(size_t i = 0; i < bytes; i++) {
    han_parser_input_byte(rxBuf.data[read_buffer][i]);
  }

  rxBuf.data_size[read_buffer] = 0xFFFFFFFFUL;
}

// Business logic goes here!

// Receive decoded packet from parser and trigger event
void HAN_callback(const han_parser_data_t* decoded_data) {
  bool is_list2 = false;
  bool is_list3 = false;

  if(decoded_data->has_meter_data) {
    if(memcmp(meter_id, decoded_data->meter_gsin, strlen(decoded_data->meter_gsin) + 1) != 0) {
      // We got attached to a different meter than the one we were previously attached to
      // invalidate persistently stored parameters
      HAN_resetNVM();

      // reset all in-RAM values too
      active_power_watt = 0;
      last_reported_power_watt = 0;

      voltage_l1 = 0;
      voltage_l2 = 0;
      voltage_l3 = 0;

      current_l1 = 0;
      current_l2 = 0;
      current_l3 = 0;

      // Store new meter identity
      memcpy(meter_id, decoded_data->meter_gsin,
        (sizeof(meter_id) < strlen(decoded_data->meter_gsin) + 1 ?
          sizeof(meter_id) :
          strlen(decoded_data->meter_gsin) + 1) );
      memcpy(meter_model, decoded_data->meter_model,
        (sizeof(meter_model) < strlen(decoded_data->meter_model) + 1 ?
          sizeof(meter_model) :
          strlen(decoded_data->meter_gsin) + 1) );

      HAN_storeToNVM(true, false);
    }
  }

  if(decoded_data->has_power_data) {
      active_power_watt = decoded_data->active_power_import;
      DPRINTF("Active power: %d W\n", active_power_watt);
      list1_recv = true;
  }

  if(decoded_data->has_energy_data) {
      total_meter_reading = decoded_data->active_energy_import;
      DPRINTF("Hourly report: accumulated %d Wh\n", total_meter_reading);
      list3_recv = true;
      is_list3 = true;
      HAN_storeToNVM(false, true);
  }

  if(decoded_data->has_line_data) {
      voltage_l1 = decoded_data->voltage_l1;
      voltage_l2 = decoded_data->voltage_l2;
      voltage_l3 = decoded_data->voltage_l3;

      current_l1 = decoded_data->current_l1;
      current_l2 = decoded_data->current_l2;
      current_l3 = decoded_data->current_l3;

      is_3phase = decoded_data->is_3p;

      is_list2 = true;
      list2_recv = true;
  }

  if((is_list3 || is_list2) &&
     currentState != STATE_APP_LEARN_MODE) {
    // Indicate activity using the indicator LED every 10s
    Board_IndicatorControl(200, 800, 1, false);
  }

  if(is_list3) {
      DPRINT("Triggering list3 event\n");
      ZAF_EventHelperEventEnqueue(EVENT_APP_ENERGY_UPDATE);
  } else if(is_list2) {
      DPRINT("Triggering list2 event\n");
      ZAF_EventHelperEventEnqueue(EVENT_APP_POWER_UPDATE_SLOW);
  } else {
      DPRINT("Triggering list1 event\n");
      ZAF_EventHelperEventEnqueue(EVENT_APP_POWER_UPDATE_FAST);
  }
}

void HAN_setup(void)
{
  // Turn on uart1 for HAN input @ 2400 baud

  // It appears there's both 8-N-1 and 8-E-1 formats in use by the various meters
  // on the Norwegian market. To be able to receive data from both of them, we
  // configure the UART to 8-N-1 (which is the default anyway).
  // That way, the UART will receive the first 9 bits (and we'll drop the 9th).
  // 8-N-1 = start | b0 | b1 | b2 | b3 | b4 | b5 | b6 | b7 | stop |
  // 8-E-1 = start | b0 | b1 | b2 | b3 | b4 | b5 | b6 | b7 | epar | stop |
  //
  // The 'extra' parity bit when the meter is sending 8-E-1 will thus either get
  // received correctly (if the parity bit is the same state as the stop bit),
  // or the USART will generate a framing error because of stop bit mismatch.
  // Lucky for us, the EFR32 USART still puts the received bits in the RX FIFO
  // even when a framing error occurs. So we can just keep receiving our data
  // and disregard the FERR flag.
  // The downside of this solution is that we don't check parity on the serial
  // bus level, but to compensate, we have two CRC-16 checks on the HDLC level
  // just above. So we can be sure that no corrupted packet gets through to the
  // parser output.
  ZAF_UART1_enable(2400, false, true);

  // Turn on GPCRC for HAN parser
  CMU_ClockEnable(cmuClock_HFPER, true);
  CMU_ClockEnable(cmuClock_GPCRC, true);

  // https://www.silabs.com/community/wireless/z-wave/knowledge-base.entry.html/2019/04/26/z-wave_700_how_toi-7ckT
  // Additionally: set UART IRQ priority lower than the radio to avoid race conditions
  CMU_ClockEnable(cmuClock_USART0, true);
  USART_IntClear(USART0, _USART_IF_MASK);
  USART_IntEnable(USART0, USART_IF_RXDATAV);
  NVIC_ClearPendingIRQ(USART0_RX_IRQn);
  NVIC_SetPriority(USART0_RX_IRQn, 4);
  NVIC_EnableIRQ(USART0_RX_IRQn);

  CMU_ClockEnable(cmuClock_USART1, true);
  USART_IntClear(USART1, _USART_IF_MASK);
  USART_IntEnable(USART1, USART_IF_RXDATAV);
  NVIC_ClearPendingIRQ(USART1_RX_IRQn);
  NVIC_SetPriority(USART1_RX_IRQn, 4);
  NVIC_EnableIRQ(USART1_RX_IRQn);



  han_parser_set_callback(&HAN_callback);
}


#define FILE_ID_GSIN 0x0010
#define FILE_ID_MODEL 0x0011
#define FILE_ID_ACCUMULATED 0x0020
#define FILE_ID_ACCUMULATED_RESET 0x0021

void HAN_printPersistentData(void) {
  DPRINTF("Meter GSIN: %s\n", meter_id);
  DPRINTF("Meter model: %s\n", meter_model);
  DPRINTF("Last reading: %u.%u kWh\n", total_meter_reading / 1000, total_meter_reading % 1000);
  DPRINTF("ZWave reset value: %u.%u kWh\n", meter_offset / 1000, meter_offset % 1000);
}

void HAN_loadFromNVM(void) {
  // Load persistently saved values from NVM at startup

  // Meter GSIN
  Ecode_t result = nvm3_readData(pFileSystemApplication,
                                 FILE_ID_GSIN, meter_id, sizeof(meter_id));
  if(result != ECODE_NVM3_OK) {
    // Need to reset NVM since something went wrong
    return HAN_resetNVM();
  }

  // Meter model
  result = nvm3_readData(pFileSystemApplication,
                         FILE_ID_MODEL, meter_model, sizeof(meter_model));
  if(result != ECODE_NVM3_OK) {
    // Need to reset NVM since something went wrong
    return HAN_resetNVM();
  }

  // Last stored accumulated value
  result = nvm3_readData(pFileSystemApplication,
                         FILE_ID_ACCUMULATED, &total_meter_reading, sizeof(total_meter_reading));
  if(result != ECODE_NVM3_OK) {
    // Need to reset NVM since something went wrong
    return HAN_resetNVM();
  }

  if(total_meter_reading != 0) {
    list3_recv = true;
  }

  // node-specific reset value
  result = nvm3_readData(pFileSystemApplication,
                         FILE_ID_ACCUMULATED_RESET, &meter_offset, sizeof(meter_offset));
  if(result != ECODE_NVM3_OK) {
    // Need to reset NVM since something went wrong
    return HAN_resetNVM();
  }

  DPRINT("Loaded meter data from NVM:\n");
  HAN_printPersistentData();
  DPRINT("===========================\n");
}

void HAN_storeToNVM(bool update_meter, bool update_accumulated) {
  Ecode_t result = ECODE_NVM3_OK;
  // Store persistently saved values to NVM on update
  if(update_meter) {
    // Meter GSIN
    result = nvm3_writeData(pFileSystemApplication,
                            FILE_ID_GSIN, meter_id, sizeof(meter_id));
    ASSERT(ECODE_NVM3_OK == result);

    // Meter model
    result = nvm3_writeData(pFileSystemApplication,
                            FILE_ID_MODEL, meter_model, sizeof(meter_model));
    ASSERT(ECODE_NVM3_OK == result);
  }

  if(update_accumulated) {
    // Accumulated value
    result = nvm3_writeData(pFileSystemApplication,
                            FILE_ID_ACCUMULATED, &total_meter_reading, sizeof(total_meter_reading));
    ASSERT(ECODE_NVM3_OK == result);

    // node-specific reset value
    result = nvm3_writeData(pFileSystemApplication,
                          FILE_ID_ACCUMULATED_RESET, &meter_offset, sizeof(meter_offset));
    ASSERT(ECODE_NVM3_OK == result);
  }

  DPRINT("Stored meter data to NVM:\n");
  HAN_printPersistentData();
  DPRINT("===========================\n");
}

void HAN_resetNVM(void) {
  memset(meter_id, 0, sizeof(meter_id));
  memset(meter_model, 0, sizeof(meter_model));
  total_meter_reading = 0;
  meter_offset = 0;

  // Invalidate reporting accumulated data
  list2_recv = false;
  list3_recv = false;
  is_3phase = false;

  // Set GSIN to {0}
  Ecode_t result = nvm3_writeData(pFileSystemApplication,
                                  FILE_ID_GSIN, meter_id, sizeof(meter_id));
  ASSERT(ECODE_NVM3_OK == result);

  // Set meter model to {0}
  result = nvm3_writeData(pFileSystemApplication,
                          FILE_ID_MODEL, meter_model, sizeof(meter_model));
  ASSERT(ECODE_NVM3_OK == result);

  // Set accumulated value to 0
  result = nvm3_writeData(pFileSystemApplication,
                          FILE_ID_ACCUMULATED, &total_meter_reading, sizeof(total_meter_reading));
  ASSERT(ECODE_NVM3_OK == result);

  // Set node-specific reset value 0
  result = nvm3_writeData(pFileSystemApplication,
                          FILE_ID_ACCUMULATED_RESET, &meter_offset, sizeof(meter_offset));
  ASSERT(ECODE_NVM3_OK == result);

  DPRINT("Reset meter data in NVM:\n");
  HAN_printPersistentData();
  DPRINT("===========================\n");
}

/*******************************************************************************
 * 'Meter' command class from here on down. The implementation is too closely-
 * tied to the AMS2ZWAVE application to warrant pulling out to a generic CC.
 ******************************************************************************/
/**
 * Struct used to pass operational data to TSE module
 */
typedef struct s_CC_meter_data_t_
{
  RECEIVE_OPTIONS_TYPE_EX rxOptions; /**< rxOptions */
} s_CC_meter_data_t;
s_CC_meter_data_t ZAF_TSE_MeterData;

/**
 * Prepare the data input for the TSE for any Meter CC command based on the pRxOption pointer.
 * @param pRxOpt pointer used to indicate how the frame was received (if any) and what endpoints are affected
*/
void* CC_Meter_prepare_zaf_tse_data(RECEIVE_OPTIONS_TYPE_EX* pRxOpt)
{
  /* Copy the RxOption in the data and return the pointer */
  ZAF_TSE_MeterData.rxOptions = *pRxOpt;
  return &ZAF_TSE_MeterData;
}

#define SCALE_KWH 0x0
#define SCALE_KVAH 0x1
#define SCALE_W 0x2
#define SCALE_V 0x4
#define SCALE_A 0x05

#define RT_DEFAULT 0x0
#define RT_IMPORT 0x1
#define RT_EXPORT 0x2

#define PRECISION_WHOLE 0x0
#define PRECISION_1_DECIMAL 0x1
#define PRECISION_2_DECIMAL 0x2
#define PRECISION_3_DECIMAL 0x3

uint8_t set_meter_report_uint32(ZW_APPLICATION_TX_BUFFER *pTxBuf, uint8_t rate, uint8_t scale, uint8_t precision, uint32_t value)
{
  pTxBuf->ZW_MeterReport4byteV5Frame.cmdClass = COMMAND_CLASS_METER_V5;
  pTxBuf->ZW_MeterReport4byteV5Frame.cmd = METER_REPORT_V5;

  // Encode meter type
  pTxBuf->ZW_MeterReport4byteV5Frame.properties1 = 0x01; //electric meter
  pTxBuf->ZW_MeterReport4byteV5Frame.properties1 |= (rate << 5) & 0x60;
  pTxBuf->ZW_MeterReport4byteV5Frame.properties1 |= (scale << 5) & 0x80;
  pTxBuf->ZW_MeterReport4byteV5Frame.properties2 = (scale & 0x3) << 3;
  pTxBuf->ZW_MeterReport4byteV5Frame.properties2 |= 0x4; //uint32
  pTxBuf->ZW_MeterReport4byteV5Frame.properties2 |= (precision & 0x7) << 5;
  pTxBuf->ZW_MeterReport4byteV5Frame.meterValue1 = (value >> 24) & 0xFF;
  pTxBuf->ZW_MeterReport4byteV5Frame.meterValue2 = (value >> 16) & 0xFF;
  pTxBuf->ZW_MeterReport4byteV5Frame.meterValue3 = (value >>  8) & 0xFF;
  pTxBuf->ZW_MeterReport4byteV5Frame.meterValue4 = value & 0xFF;
  pTxBuf->ZW_MeterReport4byteV5Frame.deltaTime1 = 0;
  pTxBuf->ZW_MeterReport4byteV5Frame.deltaTime2 = 0;

  if(scale > 0x7) {
      pTxBuf->ZW_MeterReport4byteV5Frame.scale2 = scale - 0x7;
      return 11; // 4-byte value, no historical data, extended scale
  }
  return 10; // 4-byte value, no historical data, no extended scale
}

uint8_t set_meter_supported_report_uint32(ZW_APPLICATION_TX_BUFFER *pTxBuf)
{
  pTxBuf->ZW_MeterSupportedReport1byteV5Frame.cmdClass = COMMAND_CLASS_METER_V5;
  pTxBuf->ZW_MeterSupportedReport1byteV5Frame.cmd = METER_SUPPORTED_REPORT_V5;

  // Encode supported meter values
  pTxBuf->ZW_MeterSupportedReport1byteV5Frame.properties1 = 0x01; // electricity meter type
  pTxBuf->ZW_MeterSupportedReport1byteV5Frame.properties1 |= (0x1 << 5); // only supports import energy reporting
  pTxBuf->ZW_MeterSupportedReport1byteV5Frame.properties1 |= (1 << 7); // supports reset command

  pTxBuf->ZW_MeterSupportedReport1byteV5Frame.properties2 = (1 << SCALE_KWH); // support reading KWH
  pTxBuf->ZW_MeterSupportedReport1byteV5Frame.properties2 |= (1 << SCALE_W); // support reading W
  pTxBuf->ZW_MeterSupportedReport1byteV5Frame.properties2 |= (1 << SCALE_V); // support reading V
  pTxBuf->ZW_MeterSupportedReport1byteV5Frame.properties2 |= (1 << SCALE_A); // support reading A

  return 4; // not using extended scales
}

received_frame_status_t
handleCommandClassMeter(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength)
{
  switch (pCmd->ZW_Common.cmd)
  {
    case METER_GET_V5:
      if(false == Check_not_legal_response_job(rxOpt))
      {
        ZAF_TRANSPORT_TX_BUFFER  TxBuf;
        ZW_APPLICATION_TX_BUFFER *pTxBuf = &(TxBuf.appTxBuf);
        memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

        TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
        RxToTxOptions(rxOpt, &pTxOptionsEx);

        // Grab the options from the meter get command
        uint8_t requested_scale = (pCmd->ZW_MeterGetV5Frame.properties1 & 0x38) >> 3;
        if(requested_scale == 0x07) {
            requested_scale = pCmd->ZW_MeterGetV5Frame.scale2;
            return RECEIVED_FRAME_STATUS_NO_SUPPORT;
        }

        uint8_t rate_type = pCmd->ZW_MeterGetV5Frame.properties1 >> 6;
        uint8_t response_size;

        // Get requested value
        switch(requested_scale) {
          case SCALE_KWH:
            if((rate_type == RT_DEFAULT || rate_type == RT_IMPORT)) {
              response_size = set_meter_report_uint32(pTxBuf, rate_type, requested_scale, 3, total_meter_reading - meter_offset);
            } else {
              return RECEIVED_FRAME_STATUS_NO_SUPPORT;
            }
            break;
          case SCALE_W:
            if(rate_type == RT_DEFAULT || rate_type == RT_IMPORT) {
              response_size = set_meter_report_uint32(pTxBuf, rate_type, requested_scale, 0, active_power_watt);
            } else {
              return RECEIVED_FRAME_STATUS_NO_SUPPORT;
            }
            break;
          case SCALE_A:
            if(rate_type == RT_DEFAULT || rate_type == RT_IMPORT) {
              response_size = set_meter_report_uint32(pTxBuf, rate_type, requested_scale, 3, current_l1);
            } else {
              return RECEIVED_FRAME_STATUS_NO_SUPPORT;
            }
            break;
          case SCALE_V:
            if(rate_type == RT_DEFAULT || rate_type == RT_IMPORT) {
              response_size = set_meter_report_uint32(pTxBuf, rate_type, requested_scale, 0, voltage_l1);
            } else {
              return RECEIVED_FRAME_STATUS_NO_SUPPORT;
            }
            break;
          default:
            return RECEIVED_FRAME_STATUS_NO_SUPPORT;
        }

        if(EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendResponseEP(
            (uint8_t *)pTxBuf,
            response_size,
            pTxOptionsEx,
            NULL))
        {
          /*Job failed */
          ;
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;

    case METER_SUPPORTED_GET_V5:
      if(false == Check_not_legal_response_job(rxOpt)) {
        ZAF_TRANSPORT_TX_BUFFER  TxBuf;
        ZW_APPLICATION_TX_BUFFER *pTxBuf = &(TxBuf.appTxBuf);
        memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

        TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
        RxToTxOptions(rxOpt, &pTxOptionsEx);

        uint8_t response_size = set_meter_supported_report_uint32(pTxBuf);

        if(EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendResponseEP(
            (uint8_t *)pTxBuf,
            response_size,
            pTxOptionsEx,
            NULL))
        {
          /*Job failed */
          ;
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;
    case METER_RESET_V5:
      if(false == Check_not_legal_response_job(rxOpt)) {
        meter_offset = total_meter_reading;
        HAN_storeToNVM(false, true);
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}

REGISTER_CC(COMMAND_CLASS_METER, METER_VERSION_V5, handleCommandClassMeter);

void CC_Meter_report_power(
    TRANSMIT_OPTIONS_TYPE_SINGLE_EX txOptions,
    s_CC_meter_data_t* pData)
{
  DPRINTF("* %s() *\n"
      "\ttxOpt.src = %d\n"
      "\ttxOpt.options %#02x\n"
      "\ttxOpt.secOptions %#02x\n",
      __func__, txOptions.sourceEndpoint, txOptions.txOptions, txOptions.txSecOptions);

  /* Prepare payload for report */
  ZAF_TRANSPORT_TX_BUFFER  TxBuf;
  ZW_APPLICATION_TX_BUFFER *pTxBuf = &(TxBuf.appTxBuf);
  memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

  uint8_t response_size = set_meter_report_uint32(pTxBuf, RT_IMPORT, SCALE_W, 0, active_power_watt);

  if (EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendRequestEP((uint8_t *)pTxBuf,
                                                                response_size,
                                                                &txOptions,
                                                                ZAF_TSE_TXCallback))
  {
    //sending request failed
    DPRINTF("%s(): Transport_SendRequestEP() failed. \n", __func__);
  }
}

void CC_Meter_report_energy(
    TRANSMIT_OPTIONS_TYPE_SINGLE_EX txOptions,
    s_CC_meter_data_t* pData)
{
  DPRINTF("* %s() *\n"
      "\ttxOpt.src = %d\n"
      "\ttxOpt.options %#02x\n"
      "\ttxOpt.secOptions %#02x\n",
      __func__, txOptions.sourceEndpoint, txOptions.txOptions, txOptions.txSecOptions);

  /* Prepare payload for report */
  ZAF_TRANSPORT_TX_BUFFER  TxBuf;
  ZW_APPLICATION_TX_BUFFER *pTxBuf = &(TxBuf.appTxBuf);
  memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

  uint8_t response_size = set_meter_report_uint32(pTxBuf, RT_IMPORT, SCALE_KWH, 3, total_meter_reading - meter_offset);

  if (EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendRequestEP((uint8_t *)pTxBuf,
                                                                response_size,
                                                                &txOptions,
                                                                ZAF_TSE_TXCallback))
  {
    //sending request failed
    DPRINTF("%s(): Transport_SendRequestEP() failed. \n", __func__);
  }
}

void CC_Meter_report_unhandled_as_voltage(
    TRANSMIT_OPTIONS_TYPE_SINGLE_EX txOptions,
    void* pData)
{
  DPRINTF("* %s() *\n"
      "\ttxOpt.src = %d\n"
      "\ttxOpt.options %#02x\n"
      "\ttxOpt.secOptions %#02x\n",
      __func__, txOptions.sourceEndpoint, txOptions.txOptions, txOptions.txSecOptions);

  /* Prepare payload for report */
  ZAF_TRANSPORT_TX_BUFFER  TxBuf;
  ZW_APPLICATION_TX_BUFFER *pTxBuf = &(TxBuf.appTxBuf);
  memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

  uint8_t response_size = set_meter_report_uint32(pTxBuf, RT_IMPORT, SCALE_V, 0, unhandledCommandStatus);

  if (EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendRequestEP((uint8_t *)pTxBuf,
                                                                response_size,
                                                                &txOptions,
                                                                ZAF_TSE_TXCallback))
  {
    //sending request failed
    DPRINTF("%s(): Transport_SendRequestEP() failed. \n", __func__);
  }
}

void CC_Meter_report_unhandled_as_current(
    TRANSMIT_OPTIONS_TYPE_SINGLE_EX txOptions,
    void* pData)
{
  DPRINTF("* %s() *\n"
      "\ttxOpt.src = %d\n"
      "\ttxOpt.options %#02x\n"
      "\ttxOpt.secOptions %#02x\n",
      __func__, txOptions.sourceEndpoint, txOptions.txOptions, txOptions.txSecOptions);

  /* Prepare payload for report */
  ZAF_TRANSPORT_TX_BUFFER  TxBuf;
  ZW_APPLICATION_TX_BUFFER *pTxBuf = &(TxBuf.appTxBuf);
  memset((uint8_t*)pTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );

  uint8_t response_size = set_meter_report_uint32(pTxBuf, RT_IMPORT, SCALE_A, 0, unhandledPacket);

  if (EQUEUENOTIFYING_STATUS_SUCCESS != Transport_SendRequestEP((uint8_t *)pTxBuf,
                                                                response_size,
                                                                &txOptions,
                                                                ZAF_TSE_TXCallback))
  {
    //sending request failed
    DPRINTF("%s(): Transport_SendRequestEP() failed. \n", __func__);
  }
}

void CC_Meter_update_power(void)
{
  /* Update the lifeline destinations when the Binary Switch state has changed */
  void * pData = CC_Meter_prepare_zaf_tse_data(&zaf_tse_local_actuation);
  ZAF_TSE_Trigger((void *)CC_Meter_report_power, pData, true);
  last_reported_power_watt = active_power_watt;
}

void CC_Meter_update_energy(void)
{
  /* Update the lifeline destinations when the Binary Switch state has changed */
  void * pData = CC_Meter_prepare_zaf_tse_data(&zaf_tse_local_actuation);
  ZAF_TSE_Trigger((void *)CC_Meter_report_energy, pData, true);
}

/*******************************************************************************
 * TODO: add support for querying meter GSIN over Z-Wave. This would require
 * support for the much more complicated 'meter table' CC, though. And since it
 * is a much more recent CC, OpenZWave doesn't support it yet, so I can't really
 * be bothered to implement it here either.
 ******************************************************************************/
