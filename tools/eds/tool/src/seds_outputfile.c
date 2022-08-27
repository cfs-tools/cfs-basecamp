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
 * \file     seds_outputfile.c
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Output file module implementation.
 *
 * This component is a generic file writer capable of generating output files that
 * implement the style and syntax of the language being generated.  This mainly
 * includes an abstraction for:
 *  - File name conventions
 *  - Comment formats (delimeter characters which vary)
 *  - Grouping rules
 *  - Indentation rules
 */


#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "seds_global.h"
#include "seds_user_message.h"
#include "seds_outputfile.h"

static const char SEDS_CDECL_OUTPUT_DEFAULT_LINE_ENDING[]  = "\n";

/**
 * Output file record which maps to a Lua userdata filehandle object
 */
typedef struct
{
    FILE *outfp;
    const char *static_line_ending;
    const char *comment_start;
    const char *comment_docstart;
    const char *comment_multiline;
    const char *comment_end;
    long content_start;
    long content_end;
    seds_integer_t indent_depth;
    char output_file_name[256];
    char output_buffer[512];
    char cheader_guard_string[128];
    char user_line_ending[4];
} seds_output_file_t;

/*******************************************************************************/
/*                      Internal / static Helper Functions                     */
/*                  (these are not referenced outside this unit)               */
/*******************************************************************************/


/* ------------------------------------------------------------------- */
/**
 * Write all pending output to the file.
 *
 * This affects the _locally_ buffered output, not output that might be
 * pending in the operating system buffers.  The intent is to make sure
 * the local buffers are empty to prepare for new data.
 */
void seds_output_file_flush(seds_output_file_t *pfile)
{
    if (pfile->outfp != NULL && pfile->output_buffer[0] != 0)
    {
        if (pfile->indent_depth > 0)
        {
            fprintf(pfile->outfp,"%*s", (int)pfile->indent_depth * 3, "");
        }

        fprintf(pfile->outfp, "%s%s%s", pfile->output_buffer, pfile->user_line_ending, pfile->static_line_ending);
        pfile->output_buffer[0] = 0;
    }
    pfile->user_line_ending[0] = 0;
}


/* ------------------------------------------------------------------- */
/**
 * Add whitespace into the output file.
 *
 * This improves the readability of generated files, making them look more
 * like hand written files.
 */
void seds_output_file_add_whitespace(seds_output_file_t *pfile, seds_integer_t nlines)
{
    seds_output_file_flush(pfile);

    if (pfile->outfp != NULL)
    {
        while (nlines > 0)
        {
            fprintf(pfile->outfp,"%s",pfile->static_line_ending);
            --nlines;
        }
    }
}


/* ------------------------------------------------------------------- */
/**
 * Modify the line ending of the previously written line
 *
 * This is intended for cases where generating a comma-separated list of items.
 * This typically requires all _except_ the last entry to be followed by a comma, in
 * order to be grammatically correct.  This tedious as it generally involves "peeking"
 * ahead to determine if the current line is the last line or not, before writing the
 * output.
 *
 * To make this easier, this output library allows the output to be written without
 * peeking ahead.  It is held in a temporary buffer before being written to the output file.
 * If it is determined that it is _not_ the final entry in a list, this function can
 * be used to attach the required punctuation to the already-written line, before it gets
 * sent on.
 *
 * This function has no effect if the output has been flushed (i.e. no previous line is
 * in the buffer).  This can be taken advantage of by intentionally flushing the buffer
 * before starting a list output loop, eliminating the need for any conditionals.
 */
void seds_output_file_append_previous(seds_output_file_t *pfile, const char *str)
{
    strncpy(pfile->user_line_ending, str, sizeof(pfile->user_line_ending) - 1);
    pfile->user_line_ending[sizeof(pfile->user_line_ending) - 1] = 0;
}


/* ------------------------------------------------------------------- */
/**
 * Writes a the line into the output file
 *
 * This accepts a printf-style format string and appends the line to the output.
 */
void seds_output_file_write_line(seds_output_file_t *pfile, const char *format, ...)
{
    va_list va;
    int outsz;

    /* Output any current buffer contents to the actual file */
    seds_output_file_flush(pfile);

    /* Populate the buffer from the user data */
    va_start(va, format);
    outsz = vsnprintf(pfile->output_buffer, sizeof(pfile->output_buffer), format, va);
    va_end(va);
    if (outsz >= sizeof(pfile->output_buffer))
    {
        outsz = sizeof(pfile->output_buffer) - 1;
    }

    /* trim trailing whitespace (including any newlines, which will be replaced with the preferred newline sequence) */
    while(outsz > 0 && isspace((int)pfile->output_buffer[outsz - 1]))
    {
        --outsz;
    }
    pfile->output_buffer[outsz] = 0;
}


/* ------------------------------------------------------------------- */
/**
 * Open an indented output group
 *
 * The supplied string is directly written to the output file, and
 * the indent of future lines will be increased by one level.
 *
 * In C style languages this would be used for the "open brace" where
 * it is common to indent all contents between the braces for readability.
 */
void seds_output_file_start_indent_group(seds_output_file_t *pfile, const char *string)
{
    seds_output_file_flush(pfile);

    if (string != NULL && *string != 0)
    {
        seds_output_file_write_line(pfile, "%s", string);
        seds_output_file_flush(pfile);
    }

    ++pfile->indent_depth;
}


/* ------------------------------------------------------------------- */
/**
 * Close an indented output group
 *
 * The indent of the output lines is decreased by one level, and the
 * supplied string is directly written to the output file to close the group.
 *
 * In C style languages this would be used for the "close brace" where
 * it is common to indent all contents between the braces for readability.
 */
void seds_output_file_end_indent_group(seds_output_file_t *pfile, const char *string)
{
    seds_output_file_flush(pfile);

    --pfile->indent_depth;

    if (string != NULL && *string != 0)
    {
        seds_output_file_write_line(pfile, "%s", string);
        /* Do not flush yet, in case append needs to be done */
    }
}


/* ------------------------------------------------------------------- */
/**
 * Add a banner style comment to an output file
 *
 * This creates a large banner surrounded by whitespace, using language-specific
 * comment delimiters.  It is intended to act as a marker between sections of the output file
 * and increase readability.
 */
void seds_output_file_section_marker(seds_output_file_t *pfile, const char *section_name)
{
    int padlen;

    seds_output_file_flush(pfile);

    padlen = (72 - strlen(section_name)) / 2;
    seds_output_file_add_whitespace(pfile, 2);
    if (pfile->comment_multiline)
    {
        if (pfile->comment_start)
        {
            seds_output_file_write_line(pfile, "%s", pfile->comment_start);
        }

        seds_output_file_write_line(pfile, "%s ******************************************************************************",
                pfile->comment_multiline);
        seds_output_file_write_line(pfile, "%s ** %*s%*s **",
                pfile->comment_multiline,72 - padlen, section_name, padlen, "");
        seds_output_file_write_line(pfile, "%s ******************************************************************************",
                pfile->comment_multiline);

        if (pfile->comment_end)
        {
            seds_output_file_write_line(pfile, "%s", pfile->comment_end);
        }
    }
    seds_output_file_add_whitespace(pfile, 1);
}


/* ------------------------------------------------------------------- */
/**
 * Add a documentation style comment to an output file
 *
 * This adds a comment framed by language-specific comment delimiters to describe a
 * particular item in the file.  The intent is to be compatible with the "doxygen"
 * tool for C code, but other languages could be implemented as well.
 */
void seds_output_file_write_doxytags(seds_output_file_t *pfile, const char *shortdesc, const char *longdesc, seds_boolean_t delim)
{
    char linebuffer[120];
    const char *end;
    seds_boolean_t shortdesc_present;
    size_t sz;

    seds_output_file_flush(pfile);

    if (shortdesc != NULL || longdesc != NULL)
    {
        if (delim && pfile->comment_docstart != NULL)
        {
            /* add opening delimiter, if indicated */
            seds_output_file_add_whitespace(pfile, 1);
            seds_output_file_write_line(pfile, "%s",
                    pfile->comment_docstart);
        }

        if (shortdesc != NULL && *shortdesc != 0 && pfile->comment_multiline != NULL)
        {
            seds_output_file_write_line(pfile, "%s@brief %s",
                    pfile->comment_multiline, shortdesc);
            shortdesc_present = true;
        }
        else
        {
            shortdesc_present = false;
        }

        if (longdesc != NULL && *longdesc != 0 && pfile->comment_multiline != NULL)
        {
            /* if both long and short descriptions are present, add a line between them */
            if (shortdesc_present)
            {
                seds_output_file_write_line(pfile, "%s",
                        pfile->comment_multiline);
            }

            sz = 0;
            end = longdesc;
            while (*end != 0)
            {
                /* Skip leading spaces (but not newlines) */
                while (*end == ' ')
                {
                    ++end;
                }
                while (1)
                {
                    if (sz >= (sizeof(linebuffer) - 1))
                    {
                        /* Back up to a whitespace or punctuation character, if possible */
                        while (sz > (sizeof(linebuffer) / 2) && !ispunct(linebuffer[sz-1]) && !isspace(linebuffer[sz-1]))
                        {
                            --end;
                            --sz;
                        }
                        break;
                    }
                    if (*end == 0)
                    {
                        break;
                    }
                    if (*end == '\n')
                    {
                        ++end;
                        break;
                    }
                    linebuffer[sz] = *end;
                    ++sz;
                    ++end;
                }
                /* Trim trailing whitespace */
                while (sz > 0 && isspace(linebuffer[sz-1]))
                {
                    --sz;
                }
                linebuffer[sz] = 0;
                seds_output_file_write_line(pfile,"%s%s%s",
                        pfile->comment_multiline, linebuffer, pfile->static_line_ending);
                sz = 0;
            }
        }

        if (delim && pfile->comment_end != NULL)
        {
            /* add closing delimiter, if indicated */
            seds_output_file_write_line(pfile,"%s",
                    pfile->comment_end);
        }
    }

    seds_output_file_flush(pfile);
}


/* ------------------------------------------------------------------- */
/**
 * Compare a generated output file with a previously-existing copy of the same file.
 *
 * Each time the tool executes, all files are completely regenerated.  This can be problematic
 * for timestamp-based makefiles, since timestamps will always change regardless of whether
 * the content changed.  In the majority of cases, most content will stay the same, so updating
 * timestamps can create a significant number of unnecessary rebuilds.
 *
 * This avoids that pitfall by actually comparing the generated content with an existing
 * file of the same name.  If the content is identical, the new copy is unlinked and the old
 * copy is not touched, preserving its timestamp.  If the new data is different in any way,
 * it is renamed over the old copy, updating its timestamp.
 *
 * The only aspect of this which is slightly non-trivial is that the diff must ignore the
 * header, which itself contains a timestamp of when the file was generated, which will always
 * be different.
 */
static seds_boolean_t seds_verify_final_file(seds_output_file_t *pfile)
{
    FILE *refp;
    char refline[sizeof(pfile->output_buffer)];
    size_t linelen;
    size_t endlen;
    long ref_start;
    long ref_end;
    seds_boolean_t result_match;


    /*
     * Now re-read the existing file, if any, to see if it is substantively identical
     * i.e. consider it identical if only the initial header comments differ.
     */
    refp = fopen(pfile->output_file_name, "r");

    /*
     * If it failed to open, assume the file doesn't exist or is no good.
     */
    if (refp == NULL)
    {
        return false;
    }

    if (fseek(pfile->outfp, pfile->content_start, SEEK_SET) < 0)
    {
        return false;
    }

    result_match = true;
    endlen = strlen(pfile->static_line_ending);

    if (pfile->comment_multiline == NULL)
    {
        /* no initial comment block -- content starts immediately */
        ref_start = 0;
    }
    else
    {
        /* read the reference file until the first blank line is encountered */
        ref_start = -1;
    }

    while (fgets(refline, sizeof(refline), refp) != NULL)
    {
        linelen = strlen(refline);

        if (ref_start >= 0)
        {
            /* content should be identical.  Note that comparing 1+linelen chars
             * includes the terminating NUL char, so if one line is longer than the
             * other one then this check will fail. */
            if (fgets(pfile->output_buffer, sizeof(pfile->output_buffer), pfile->outfp) == NULL ||
                    memcmp(pfile->output_buffer, refline, 1 + linelen) != 0)
            {
                result_match = false;
                break;
            }
        }
        else if (linelen < endlen ||
                    memcmp(&refline[linelen - endlen], pfile->static_line_ending, endlen) != 0)
        {
            result_match = false;
            break;
        }
        else if (linelen == endlen)
        {
            /* line contains ONLY the static end-of-line character(s),
             * consider this the start of the actual substantive content */
            ref_start = ftell(refp);
        }
    }

    if (result_match)
    {
        ref_end = ftell(refp);
        result_match = ((ref_end - ref_start) == (pfile->content_end - pfile->content_start));
    }

    return result_match;
}


/* ------------------------------------------------------------------- */
/**
 * Close an output file.
 *
 * This flushes any output, closes the file, and performs all post-generation file checks.
 */
void seds_output_file_close(seds_output_file_t *pfile)
{
    char namebuf[sizeof(pfile->output_file_name) + 4];
    seds_boolean_t result_match;

    if (pfile->outfp != NULL)
    {
        /*
         * If the cheader_guard_string is set to something, then close the preprocessor directive
         */
        if (pfile->cheader_guard_string[0] != 0)
        {
            seds_output_file_add_whitespace(pfile, 1);
            seds_output_file_write_line(pfile, "#endif    %s _%s_ %s",
                    pfile->comment_start, pfile->cheader_guard_string, pfile->comment_end);
            pfile->cheader_guard_string[0] = 0;
        }

        seds_output_file_section_marker(pfile, "END OF FILE");
        seds_output_file_flush(pfile);
        fflush(pfile->outfp);
        pfile->content_end = ftell(pfile->outfp);
        result_match = seds_verify_final_file(pfile);
        fclose(pfile->outfp);
        pfile->outfp = NULL;

        snprintf(namebuf,sizeof(namebuf),"%s.tmp",pfile->output_file_name);
        if (result_match)
        {
            /*
             * result file was identical, so remove the new one, keep only the old one
             * NOTE - it is done this way to preserve timestamps and avoids rebuilding
             * dependent code unnecessarily.  If we updated unconditionally, then the
             * timestamps would needlessly change and all dependent code would be rebuilt.
             */
            remove(namebuf);
        }
        else
        {
            /* result file was different, so rename the new one overwriting the old one */
            rename(namebuf,pfile->output_file_name);
        }
    }
}


/* ------------------------------------------------------------------- */
/**
 * Open an output file.
 *
 * This creates an output file with the given name, in the given subdirectory.
 *
 * It actually opens a temporary file with a different name first.  Once the file
 * is closed, it will replace the existing copy, making the change atomic.
 */
void seds_output_file_open(seds_output_file_t *pfile, const char *basedir, const char *subdir, const char *destfile, const char *sourcefile)
{
    char namebuf[sizeof(pfile->output_file_name) + 4];
    time_t nowtime;

    seds_output_file_close(pfile);

    if (mkdir(basedir, 0755) < 0 && errno != EEXIST)
    {
        SEDS_REPORT_ERRNO(FATAL, basedir);
    }

    snprintf(namebuf,sizeof(namebuf),"%s/%s",basedir,subdir);
    if (mkdir(namebuf, 0755) < 0 && errno != EEXIST)
    {
        SEDS_REPORT_ERRNO(FATAL, namebuf);
    }

    snprintf(pfile->output_file_name,sizeof(pfile->output_file_name),"%s/%s/%s",basedir,subdir,destfile);
    snprintf(namebuf,sizeof(namebuf),"%s.tmp",pfile->output_file_name);
    pfile->outfp = fopen(namebuf, "w+");
    SEDS_ASSERT_ERRNO(pfile->outfp != NULL, namebuf);

    if (pfile->comment_multiline != NULL)
    {
        if (pfile->comment_start)
        {
            seds_output_file_write_line(pfile, "%s", pfile->comment_start);
        }

        seds_output_file_write_line(pfile, "%s NOTE -- THIS IS A GENERATED FILE -- DO NOT EDIT",
                pfile->comment_multiline);
        seds_output_file_write_line(pfile, "%s", pfile->comment_multiline);
        if (sourcefile != NULL && *sourcefile != 0)
        {
            seds_output_file_write_line(pfile, "%s %16s: %s", pfile->comment_multiline, "Generated From", sourcefile);
        }
        seds_output_file_write_line(pfile, "%s %16s: %s", pfile->comment_multiline, "Output File", destfile);
        nowtime = time(NULL);
        seds_output_file_write_line(pfile, "%s %16s: %s", pfile->comment_multiline, "Generation Time", ctime(&nowtime)); /* note - ctime adds '\n' */

        if (pfile->comment_end)
        {
            seds_output_file_write_line(pfile, "%s", pfile->comment_end);
        }

        /*
         * IMPORTANT: always add 1 line of whitespace.  This acts as a key
         * that indicates the END of the file header block comment, regardless
         * of the specific formatting of the comment.
         */
        seds_output_file_add_whitespace(pfile, 1);
    }

    pfile->content_start = ftell(pfile->outfp);

    /*
     * If the cheader_guard_string is set to something, then generate the preprocessor directive
     */
    if (pfile->cheader_guard_string[0] != 0)
    {
        seds_output_file_write_line(pfile,"#ifndef _%s_", pfile->cheader_guard_string);
        seds_output_file_write_line(pfile,"#define _%s_", pfile->cheader_guard_string);
        seds_output_file_add_whitespace(pfile, 1);
    }
}

/* ------------------------------------------------------------------- */
/**
 * Lua callable version of the file write function
 */
static int seds_lua_output_file_write(lua_State *lua)
{
    seds_output_file_write_line(luaL_checkudata(lua, 1, "seds_output_file"), "%s", luaL_checkstring(lua, 2));
    return 0;
}

/* ------------------------------------------------------------------- */
/**
 * Lua callable version of the flush function
 */
static int seds_lua_output_file_flush(lua_State *lua)
{
    seds_output_file_flush(luaL_checkudata(lua, 1, "seds_output_file"));
    return 0;
}

/* ------------------------------------------------------------------- */
/**
 * Lua callable version of the add_whitespace function
 */
static int seds_lua_output_file_add_whitespace(lua_State *lua)
{
    seds_output_file_add_whitespace(luaL_checkudata(lua, 1, "seds_output_file"), luaL_optinteger(lua, 2, 1));
    return 0;
}

/* ------------------------------------------------------------------- */
/**
 * Lua callable version of the start_group function
 */
static int seds_lua_output_file_start_group(lua_State *lua)
{
    seds_output_file_start_indent_group(luaL_checkudata(lua, 1, "seds_output_file"), luaL_optstring(lua, 2, ""));
    return 0;
}

/* ------------------------------------------------------------------- */
/**
 * Lua callable version of the end_group function
 */
static int seds_lua_output_file_end_group(lua_State *lua)
{
    seds_output_file_end_indent_group(luaL_checkudata(lua, 1, "seds_output_file"), luaL_optstring(lua, 2, ""));
    return 0;
}

/* ------------------------------------------------------------------- */
/**
 * Lua callable version of the append_previous function
 */
static int seds_lua_output_file_append_previous(lua_State *lua)
{
    seds_output_file_append_previous(luaL_checkudata(lua, 1, "seds_output_file"), luaL_checkstring(lua, 2));
    return 0;
}

/* ------------------------------------------------------------------- */
/**
 * Lua callable version of the file section_marker function
 */
static int seds_lua_output_file_section_marker(lua_State *lua)
{
    seds_output_file_section_marker(luaL_checkudata(lua, 1, "seds_output_file"), luaL_checkstring(lua, 2));
    return 0;
}

/* ------------------------------------------------------------------- */
/**
 * Lua callable version of the write_doxytags function
 */
static int seds_lua_output_documentation(lua_State *lua)
{
    seds_output_file_write_doxytags(luaL_checkudata(lua, 1, "seds_output_file"), luaL_optstring(lua, 2, NULL), luaL_optstring(lua, 3, NULL), true);
    return 0;
}

/* ------------------------------------------------------------------- */
/**
 * Lua callable version of the file close function
 */
static int seds_lua_output_file_close(lua_State *lua)
{
    seds_output_file_close(luaL_checkudata(lua, 1, "seds_output_file"));
    return 0;
}

/* ------------------------------------------------------------------- */
/**
 * Lua property get method for file objects.  This primarily gets the various
 * methods that are implemented for file objects.  If no method exists,
 * then it looks up the key in an internal key/value store.
 *
 * Expected Stack args:
 *  1: output file object
 *  2: property key
 */
static int seds_lua_output_file_get_method(lua_State *lua)
{
    luaL_checkudata(lua, 1, "seds_output_file");
    if (lua_type(lua, 2) == LUA_TSTRING)
    {
        const char *keyname = lua_tostring(lua, 2);
        if (strcmp(keyname, "write") == 0)
        {
            lua_pushcfunction(lua, seds_lua_output_file_write);
        }
        else if (strcmp(keyname, "flush") == 0)
        {
            lua_pushcfunction(lua, seds_lua_output_file_flush);
        }
        else if (strcmp(keyname, "add_whitespace") == 0)
        {
            lua_pushcfunction(lua, seds_lua_output_file_add_whitespace);
        }
        else if (strcmp(keyname, "start_group") == 0)
        {
            lua_pushcfunction(lua, seds_lua_output_file_start_group);
        }
        else if (strcmp(keyname, "end_group") == 0)
        {
            lua_pushcfunction(lua, seds_lua_output_file_end_group);
        }
        else if (strcmp(keyname, "append_previous") == 0)
        {
            lua_pushcfunction(lua, seds_lua_output_file_append_previous);
        }
        else if (strcmp(keyname, "section_marker") == 0)
        {
            lua_pushcfunction(lua, seds_lua_output_file_section_marker);
        }
        else if (strcmp(keyname, "add_documentation") == 0)
        {
            lua_pushcfunction(lua, seds_lua_output_documentation);
        }
    }

    if (lua_gettop(lua) == 2)
    {
        lua_getuservalue(lua, 1);
        lua_pushvalue(lua, 2);
        lua_rawget(lua, -2);
    }

    return 1;
}

/* ------------------------------------------------------------------- */
/**
 * Lua property set method for file objects.
 *
 * This allows users to store arbitrary key/value pairs
 * inside file objects, to keep track of local state as necessary.
 * Its use is optional.
 *
 * Expected Stack args:
 *  1: output file object
 *  2: property key
 *  3: property value
 */
static int seds_lua_output_file_set_method(lua_State *lua)
{
    luaL_checkudata(lua, 1, "seds_output_file");
    lua_getuservalue(lua, 1);
    lua_pushvalue(lua, 2);
    lua_pushvalue(lua, 3);
    lua_rawset(lua, -3);
    return 0;
}

/* ------------------------------------------------------------------- */
/**
 * Lua callable file open function
 *
 * Unlike the other methods, this is attached in the SEDS global object.
 * It produces file objects which in turn use the other methods.
 *
 * Expected Input Stack:
 *  1: string - file name
 *  2: string - source file (optional)
 *  3: file type - optional
 *
 * The source file, if present, is included in a banner comment at the
 * top of the file to indicate which file it was generated from.  The
 * file type, if specified, will override the default logic of using the
 * file extension.
 */
static int seds_lua_output_file_open(lua_State *lua)
{
    const char *destfile = luaL_checkstring(lua, 1);
    const char *sourcefile = luaL_optstring(lua, 2, NULL);
    const char *outlang = luaL_optstring(lua, 3, NULL);
    const char *subdir = ".";
    seds_output_file_t *pfile = lua_newuserdata(lua, sizeof(seds_output_file_t));
    int stack_top = lua_gettop(lua);

    memset(pfile, 0, sizeof(*pfile));
    if (luaL_newmetatable(lua, "seds_output_file"))
    {
        lua_pushstring(lua, "__index");
        lua_pushcfunction(lua, seds_lua_output_file_get_method);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__newindex");
        lua_pushcfunction(lua, seds_lua_output_file_set_method);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__gc");
        lua_pushcfunction(lua, seds_lua_output_file_close);
        lua_rawset(lua, -3);
    }
    lua_setmetatable(lua, -2);

    lua_newtable(lua);
    lua_setuservalue(lua, -2);

    /*
     * Set default line ending mode
     */
    pfile->static_line_ending = SEDS_CDECL_OUTPUT_DEFAULT_LINE_ENDING;

    /*
     * If the output language was not specified,
     * try to deduce from the filename.
     */
    if (outlang == NULL)
    {
        outlang = strrchr(destfile, '.');
        if (outlang == NULL)
        {
            outlang = "c";
        }
        else
        {
            ++outlang;
        }
    }

    /*
     * If the output language is C/C++,
     * set up for C-style comment delimiters
     * this also sets up for a preprocessor guard macro around the whole file
     * (only if the filename ends in .h)
     */
    if (strcasecmp(outlang, "c") == 0 ||
            strcasecmp(outlang, "h") == 0 ||
            strcasecmp(outlang, "cpp") == 0 ||
            strcasecmp(outlang, "hpp") == 0)
    {
        size_t sz = strlen(destfile);
        const char *inp = destfile + sz;
        char *outp;

        while (sz > 0)
        {
            --sz;
            --inp;
            if (*inp == '.')
            {
                ++inp;
                break;
            }
        }

        if (inp != destfile && tolower((int)*inp) == 'h')
        {
            /* Must be a header file - put into "inc" subdir */
            subdir = "inc";

            /* Set up for the guard macro */
            lua_getglobal(lua, "SEDS");
            if (lua_type(lua, -1) == LUA_TTABLE)
            {
                lua_getfield(lua, -1, "get_define");
                if (lua_type(lua, -1) == LUA_TFUNCTION)
                {
                    lua_pushstring(lua, "MISSION_NAME");
                    lua_call(lua, 1, 1);
                    if (lua_type(lua, -1) == LUA_TSTRING)
                    {
                        lua_pushstring(lua, "_");
                        lua_concat(lua, 2);
                        strncpy(pfile->cheader_guard_string, lua_tostring(lua, -1), sizeof(pfile->cheader_guard_string) - 1);
                        pfile->cheader_guard_string[sizeof(pfile->cheader_guard_string) - 1] = 0;
                    }
                }
                lua_pop(lua, 1);
            }
            lua_pop(lua, 1);

            inp = destfile;
            outp = pfile->cheader_guard_string;
            sz = strlen(pfile->cheader_guard_string);
            outp += sz;
            while (sz < (sizeof(pfile->cheader_guard_string) - 1) && *inp != 0)
            {
                if (isalnum((int)*inp))
                {
                    *outp = toupper((int)*inp);
                }
                else
                {
                    *outp = '_';
                }
                ++outp;
                ++inp;
                ++sz;
            }
            *outp = 0;
        }
        else
        {
            /* Must be a source file - put into "src" subdir */
            subdir = "src";
        }

        /*
         * Set up C-style comment blocks
         */
        pfile->comment_start = "/* ";
        pfile->comment_docstart = "/**";
        pfile->comment_multiline = " * ";
        pfile->comment_end = " */";
    }
    else if (strcasecmp(outlang, "make") == 0 ||
            strcasecmp(outlang, "mk") == 0 ||
            strcasecmp(outlang, "shell") == 0 ||
            strcasecmp(outlang, "sh") == 0 ||
            strcasecmp(outlang, "perl") == 0 ||
            strcasecmp(outlang, "pl") == 0 ||
            strcasecmp(outlang, "python") == 0 ||
            strcasecmp(outlang, "py") == 0)
    {
        /*
         * Set up shell-style comment blocks
         * This style is applicable to several interpreted languages,
         * including makefiles, perl and python
         */
        pfile->comment_start = "# ";
        pfile->comment_docstart = pfile->comment_start;
        pfile->comment_multiline = pfile->comment_start;
        pfile->comment_end = pfile->comment_start;
    }
    else if (strcasecmp(outlang, "lua") == 0)
    {
        /*
         * Set up lua-style comment blocks
         */
        pfile->comment_start = "-- ";
        pfile->comment_docstart = "---";
        pfile->comment_multiline = pfile->comment_start;
        pfile->comment_end = pfile->comment_start;
    }
    else if (strcasecmp(outlang, "xml") == 0)
    {
        /*
         * Set up xml-style comment blocks
         */
        pfile->comment_start = "<!-- ";
        pfile->comment_docstart = NULL;
        pfile->comment_multiline = NULL;
        pfile->comment_end = "  -->";
    }

    lua_rawgetp(lua, LUA_REGISTRYINDEX, &sedstool.GLOBAL_SYMBOL_TABLE_KEY);
    lua_getfield(lua, -1, "MISSION_BINARY_DIR");

    seds_output_file_open(pfile, lua_tostring(lua, -1), subdir, destfile, sourcefile);

    lua_settop(lua, stack_top);

    return 1;
}

/*******************************************************************************/
/*                      Externally-Called Functions                            */
/*      (referenced outside this unit and prototyped in a separate header)     */
/*******************************************************************************/


void seds_outputfile_register_globals(lua_State *lua)
{
    luaL_checktype(lua, -1, LUA_TTABLE);

    lua_pushcfunction(lua, seds_lua_output_file_open);
    lua_setfield(lua, -2, "output_open");
    lua_pushcfunction(lua, seds_lua_output_file_close);
    lua_setfield(lua, -2, "output_close");
}

