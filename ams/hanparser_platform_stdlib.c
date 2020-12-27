/***************************************************************************//**
 * @file hanparser_platform_zwave.c
 * @brief This file implements hanparser_platform.h against the C stdlib
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
#include "hanparser_platform.h"

// In order to redirect a variadic printf-style debug print function, we need to
// include support for variadic functions and an implementation of vsnprintf.
#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

// CRC source: https://people.cs.umu.se/isak/snippets/crc-16.c
#define POLY 0x8408

// Check CRC-16/X-25 checksum of a byte array
// start at the pointed-to byte, read 'bytes' to calculate checksum, check sum
// at the 2 bytes right after 'bytes'.
// Return true if checksum is valid
bool han_parser_check_crc16_x25(uint8_t* start, size_t bytes)
{
  unsigned int i, data, crc = 0xffff;
  size_t length = bytes;
  uint8_t* data_p = start;

  if (length == 0)
    goto check;

  do
  {
    for (i=0, data=(unsigned int)0xff & *data_p++;
         i < 8;
         i++, data >>= 1)
    {
      if ((crc & 0x0001) ^ (data & 0x0001))
        crc = (crc >> 1) ^ POLY;
      else  crc >>= 1;
    }
  } while (--length);

  crc = ~crc;
  data = crc;
  crc = (crc << 8) | (data >> 8 & 0xff);

check:
  if(start[bytes] != ((crc >> 8) & 0xFF) ||
     start[bytes + 1] != (crc & 0xFF)) {
    return false;
  }
  return true;
}

void han_parser_debug_print(const char* msg)
{
#ifdef DEBUGPRINT
  printf(msg);
#endif
}

void han_parser_debug_printf(const char* fmt, ...)
{
#ifdef DEBUGPRINT
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  printf(buf);
#endif
}

#ifdef __cplusplus
}
#endif
