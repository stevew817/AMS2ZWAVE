/***************************************************************************//**
 * @file hanparser_platform_zwave.c
 * @brief This file implements hanparser_platform.h against the Z-Wave SDK on EFR32
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
#include "ams/hanparser_platform.h"

// Calculate CRC using dedicated hardware
#include "em_gpcrc.h"

// Use Z-Wave SDK's debug print functions
#include "DebugPrint.h"

// In order to redirect a variadic printf-style debug print function, we need to
// include support for variadic functions and an implementation of vsnprintf.
#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Check CRC-16/X-25 checksum of a byte array
// start at the pointed-to byte, read 'bytes' to calculate checksum, check sum
// at the 2 bytes right after 'bytes'.
// Return true if checksum is valid
bool han_parser_check_crc16_x25(uint8_t* start, size_t bytes)
{
  const GPCRC_Init_TypeDef crc_init = {
    0x1021UL,   /* CRC32 Polynomial value. */
    0xFFFFUL,   /* Initialization value. */
    false,      /* Byte order is normal. */
    false,      /* Bit order is not reversed on output. */
    true,       /* Enable byte mode. */
    false,      /* Disable automatic initialization on data read. */
    true,       /* Enable GPCRC. */
  };

  GPCRC_Init(GPCRC, &crc_init);
  GPCRC_Start(GPCRC);

  for(size_t i = 0; i < bytes; i++) {
    GPCRC_InputU8(GPCRC, start[i]);
  }

  // CRC-16/X-25 reads inverted
  uint32_t result = ~GPCRC_DataRead(GPCRC);
  if(start[bytes] != (result & 0xFF) ||
     start[bytes + 1] != ((result >> 8) & 0xFF)) {
    return false;
  }
  return true;
}

void han_parser_debug_print(const char* msg)
{
#ifdef DEBUGPRINT
    DPRINT(msg);
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

    DPRINT(buf);
#endif
}

#ifdef __cplusplus
}
#endif
