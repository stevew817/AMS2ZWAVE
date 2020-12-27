# AMS (Norwegian HAN) message interpreter in C

This folder contains an ANSI C-language parser for messages originating from the 'HAN' port of a Norwegian smart electricity meter.

# Usage
An implementer is expected to implement the functions defined in [`hanparser_platform.h`](hanparser_platform.h).

There's only three, and a reference implementation against stdlib is provided in [`hanparser_platform_stdlib.c`](hanparser_platform_stdlib.c)
* `han_parser_check_crc16_x25`: calculate a CRC-16/X-25 of a byte array and verify it against the provided CRC.
* `han_parser_debug_print`: print function targeting debug output
* `han_parser_debug_printf`: printf function targeting debug output

Getting the decoded values is done through setting a callback function using `han_parser_set_callback`. The callback gets called upon a
successful message decode, and the values struct passed to the callback function contains the decoded values.

To run this parser, set the feed each input byte to `han_parser_input_byte` as it comes in.

# License
SPDX-License-Identifier: Zlib
Author: Steven Cooreman, github.com/stevew817
