/**
 * @file events.h
 *
 * Definitions of events for AMS2ZWAVE.
 *
 * Adapted from the Silicon Labs Z-Wave sample application repository, which is
 * licensed under the Zlib license:
 *   https://github.com/SiliconLabs/z_wave_applications
 *
 * Modifications by Steven Cooreman, github.com/stevew817
 *******************************************************************************
 * Original license:
 *
 * copyright 2019 Silicon Laboratories Inc.
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
#ifndef APPS_AMS2ZWAVE_EVENTS_H_
#define APPS_AMS2ZWAVE_EVENTS_H_

/**
 * Defines events for the application.
 *
 * These events are not referred to anywhere else than in the application. Hence, they can be
 * altered to suit the application flow.
 *
 * The events are located in a separate file to make it possible to include them in other
 * application files. An example could be a peripheral driver that enqueues an event when something
 * specific happens.
 */
typedef enum EVENT_APP_AMS2ZWAVE
{
  EVENT_EMPTY = DEFINE_EVENT_APP_NBR,
  EVENT_APP_INIT,
  EVENT_APP_LEARN_MODE_FINISH,
  EVENT_APP_REFRESH_MMI,
  EVENT_APP_FLUSHMEM_READY,
  EVENT_APP_NEXT_EVENT_JOB,
  EVENT_APP_FINISH_EVENT_JOB,
  EVENT_APP_TOGGLE_LEARN_MODE,
  EVENT_APP_SMARTSTART_IN_PROGRESS,
  EVENT_APP_LEARN_IN_PROGRESS,
  EVENT_APP_SERIAL_RECV,       // fires when a packet becomes available
  EVENT_APP_POWER_UPDATE_FAST, // fires each 2.5s when a meter is connected
  EVENT_APP_POWER_UPDATE_SLOW, // fires each 10s when a meter is connected
  EVENT_APP_ENERGY_UPDATE      // fires each 3600s when a meter is connected
}
EVENT_APP;

#endif /* APPS_AMS2ZWAVE_EVENTS_H_ */
