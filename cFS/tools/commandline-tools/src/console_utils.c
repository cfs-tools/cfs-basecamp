/************************************************************************
 * NASA Docket No. GSC-19,200-1, and identified as "cFS Draco"
 *
 * Copyright (c) 2023 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/**
 * \file     cmdUtil.c
 * \ingroup  cfecfs
 * \author   joseph.p.hickey@nasa.gov
 *
 ** cmdUtil -- A CCSDS Command utility. This program will build a CCSDS Command packet
 **               with variable parameters and send it on a UDP network socket.
 **               this program is primarily used to command a cFE flight software system.
 */

/*
** System includes
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <ctype.h>

#include "console_utils.h"

ConsoleUtils_LogLevel_t ConsoleUtils_Verbosity;

/*----------------------------------------------------------------
 *
 * Application Helper function
 * Writes a console log message, depending on log level/verbosity
 *
 *-----------------------------------------------------------------*/
void ConsoleUtils_Log(ConsoleUtils_LogLevel_t lev, const char *spec, ...)
{
    va_list vl;

    if (lev <= ConsoleUtils_Verbosity)
    {
        va_start(vl, spec);
        vprintf(spec, vl);
        va_end(vl);
    }
}

/*----------------------------------------------------------------
 *
 * Application Helper function
 * Writes a hexdump to the console
 *
 *-----------------------------------------------------------------*/
void ConsoleUtils_Hexdump(const void *Buf, size_t Sz)
{
    const uint8_t *p = Buf;
    char           hexdump_line[64];
    char           chardump_line[20];
    int            c;
    size_t         i;
    size_t         j;

    i = 0;
    while (i < Sz)
    {
        for (j = 0; j < 16 && i < Sz; ++j)
        {
            c = p[i];

            snprintf(&hexdump_line[j * 3], 4, " %02x", c);
            if (!isprint(c))
            {
                c = '.';
            }
            snprintf(&chardump_line[j], 2, "%c", c);

            ++i;
        }
        printf("0x%03x:%-50s%-20s\n", (unsigned int)(i - j), hexdump_line, chardump_line);
    }
}
