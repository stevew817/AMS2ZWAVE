/***************************************************************************//**
 * @file hanparser_platform.h
 * @brief This file describes what the HAN parser needs from the platform
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

#ifndef AMS_HANPARSER_PLATFORM_H_
#define AMS_HANPARSER_PLATFORM_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Check CRC-16/X-25 checksum of a byte array
// start at the pointed-to byte, read 'bytes' to calculate checksum, check sum
// at the 2 bytes right after 'bytes'.
// Return true if checksum is valid
bool han_parser_check_crc16_x25(uint8_t* start, size_t bytes);

void han_parser_debug_print(const char* msg);
void han_parser_debug_printf(const char* msg, ...);

#ifdef __cplusplus
}
#endif

#endif /* AMS_HANPARSER_PLATFORM_H_ */
