/**
 * @file config_rf.h
 *
 * RF Configuration file for AMS2ZWAVE.
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
#ifndef _CONFIG_RF_H_
#define _CONFIG_RF_H_

// The maximum allowed Tx power in deci dBm
#define APP_MAX_TX_POWER      0

// The deci dBm output measured at a PA setting of 0dBm (raw value 24)
#define APP_MEASURED_0DBM_TX_POWER 33

#endif /* _CONFIG_RF_H_ */
