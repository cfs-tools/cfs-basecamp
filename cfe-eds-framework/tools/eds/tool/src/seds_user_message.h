/*
 * LEW-19710-1, CCSDS SOIS Electronic Data Sheet Implementation
 * 
 * Copyright (c) 2020 United States Government as represented by
 * the Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *    http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/**
 * \file     seds_user_message.h
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation of the user message output module.
 * For module documentation see seds_user_message.c
 */

#ifndef _SEDS_USER_MESSAGE_H_
#define _SEDS_USER_MESSAGE_H_

#include "seds_global.h"

/*
 * MACROS TO SIMPLIFY ERROR REPORTING
 */

/**
 * Assert that some condition is true, otherwise trigger a "fatal" error and abort
 *
 * Useful for conditions where there is no valid/useful runtime recovery, e.g. if
 * a call to malloc() returns NULL, no sense in going on.
 */
#define SEDS_ASSERT(condition,message)  { if (!(condition)) seds_user_message_preformat(SEDS_USER_MESSAGE_FATAL, __FILE__, __LINE__, "Assertion \'" #condition "\' failed", message); }

/**
 * Report a message based on the host system "errno" value.
 * The severity of the message is user-defined.
 */
#define SEDS_REPORT_ERRNO(mt,x)         { seds_user_message_errno(SEDS_USER_MESSAGE_ ## mt, __FILE__, __LINE__, x); }

/**
 * Assert that a condition is true, trigger a "fatal" error and abort if not.
 * The reported message to the user will be based on the host system "errno" value.
 */
#define SEDS_ASSERT_ERRNO(cond,x)       { if (!(cond)) SEDS_REPORT_ERRNO(FATAL, "Assertion \'" #cond "\' failed: " #x); }



/**
 * Classification of various types of user messages.
 *
 * A message of the "FATAL" type will cause the tool to abort.
 * The tool will continue after an "ERROR" but will report an
 * error exit status to the OS when it does complete.  A warning
 * will show a warning message but not otherwise cause a failure.
 */
typedef enum
{
    SEDS_USER_MESSAGE_DEBUG = 0,
    SEDS_USER_MESSAGE_INFO,
    SEDS_USER_MESSAGE_WARNING,
    SEDS_USER_MESSAGE_ERROR,
    SEDS_USER_MESSAGE_FATAL,
    SEDS_USER_MESSAGE_MAX
} seds_user_message_t;

/**
 * Send any pre-formatted message to the user via stderr
 *
 * Context information is attached to the message, including the file, line, and additional details
 * if provided. The severity of the message is user-defined, and this will also call "abort()" if
 * the severity is FATAL.
 *
 * @param msgtype the type of message to display (FATAL will abort the tool)
 * @param file the source file for which the error is related (may be NULL)
 * @param line the line number of the source file (0 for no line number)
 * @param message1 the complete error message to display
 * @param message2 additional details or context info about the error (may be NULL if no added details)
 */
void seds_user_message_preformat(seds_user_message_t msgtype, const char *file, unsigned long line, const char *message1, const char *message2);

/**
 * Send any message to the user, using a printf-style format specification
 *
 * A complete error message is generated from the format string and arguments, and
 * passed to seds_user_message_impl.
 *
 * @sa seds_user_message_impl
 * @param msgtype passed through to seds_user_message_impl
 * @param file passed through to seds_user_message_impl
 * @param line passed through to seds_user_message_impl
 * @param format printf-style format string
 */
void seds_user_message_printf(seds_user_message_t msgtype, const char *file, unsigned long line, const char *format, ...) SEDS_PRINTF(4,5);

/**
 * Send an "errno" message to the user
 *
 * A message is formulated based on the current value of the global "errno" variable and
 * passed to seds_user_message_impl.  This provides a method to report errors reported
 * by the system libraries up to the user in a consistent way.
 *
 * @param msgtype passed through to seds_user_message_impl
 * @param file passed through to seds_user_message_impl
 * @param line passed through to seds_user_message_impl
 * @param message additional context information
 *
 */
void seds_user_message_errno(seds_user_message_t msgtype, const char *file, unsigned long line, const char *message);


/**
 * Return the number of times a message of the given type was generated
 *
 * @param msgtype the message type
 * @return the number of times a message of that type was generated
 */
seds_integer_t seds_user_message_get_count(seds_user_message_t msgtype);

/**
 * Registers all user message output functions in the Lua state
 *
 * The top of the stack should be a table object.  All Lua-callable
 * functions in this module will be added into the table.
 */
void seds_user_message_register_globals(lua_State *lua);


#endif  /* _SEDS_USER_MESSAGE_H_ */

