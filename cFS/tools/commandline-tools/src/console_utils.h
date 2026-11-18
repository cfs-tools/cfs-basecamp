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
#ifndef CONSOLE_UTILS_H
#define CONSOLE_UTILS_H

#include <stddef.h>

typedef enum
{
    ConsoleUtils_LogLevel_ERROR,
    ConsoleUtils_LogLevel_INFO,
    ConsoleUtils_LogLevel_DEBUG
} ConsoleUtils_LogLevel_t;

void ConsoleUtils_Log(ConsoleUtils_LogLevel_t lev, const char *spec, ...);
void ConsoleUtils_Hexdump(const void *Buf, size_t Sz);

#define CONSOLEUTILS_DEBUG(...) ConsoleUtils_Log(ConsoleUtils_LogLevel_DEBUG, __VA_ARGS__)
#define CONSOLEUTILS_INFO(...)  ConsoleUtils_Log(ConsoleUtils_LogLevel_INFO, __VA_ARGS__)
#define CONSOLEUTILS_ERROR(...) ConsoleUtils_Log(ConsoleUtils_LogLevel_ERROR, __VA_ARGS__)

extern ConsoleUtils_LogLevel_t ConsoleUtils_Verbosity;

#endif
