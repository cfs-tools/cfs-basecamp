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
 * \file     remote_interface.c
 * \ingroup  testexecutive
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation of test executive
 */

#include <stdlib.h>
#include <string.h> /* memset() */
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "testexec.h"

#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"
#include "edslib_displaydb.h"
#include "cfe_missionlib_runtime.h"
#include "cfe_missionlib_api.h"
#include "edslib_lua_objects.h"
#include "cfe_missionlib_lua_softwarebus.h"

/* Size to use for ring buffer - keep as power of 2 for masking */
#define REMOTE_BUFFER_SIZE      0x8000
#define REMOTE_BUFFER_MASK      0x7FFF
#define SEND_BUFFER_SIZE        0x4000

typedef struct
{
    pid_t pid;
    int InFd;
    int OutFd;
    char SendBuffer[SEND_BUFFER_SIZE];
    char RecvBuffer[REMOTE_BUFFER_SIZE];
    size_t RecvBufWrPos;
    size_t RecvBufChkPos;
    size_t RecvBufRdPos;

} TestIntf_RemoteConnection_t;

static void TestIntf_Poll_Subprocess(TestIntf_RemoteConnection_t *Conn)
{
    if (Conn->pid > 0)
    {
        if (waitpid(Conn->pid, NULL, WNOHANG) == Conn->pid)
        {
            Conn->pid = -1;

            if (Conn->InFd >= 0)
            {
                close(Conn->InFd);
                Conn->InFd = -1;
            }

            if (Conn->OutFd >= 0)
            {
                close(Conn->OutFd);
                Conn->OutFd = -1;
            }
        }
    }
}

static int TestIntf_Remote_ObjToString(lua_State *lua)
{
    switch(lua_type(lua, 1))
    {
    case LUA_TSTRING:
    {
        /*
         * Note this string could contain embedded special chars (including nulls)
         * as well as newlines and other binary data which will throw off the parser.
         * To be safe all strings will be base64 encoded.  It will increase the length
         * by 4/3 but it will be guaranteed to get through intact.
         */
        const uint8_t *Data = (const uint8_t *)lua_tostring(lua, 1);
        uint32_t DataLength = 8 * lua_rawlen(lua, 1);
        uint32_t EncodedLength = (DataLength + 32) / 6;
        char *Block;
        luaL_Buffer Buf;

        luaL_buffinit(lua, &Buf);
        luaL_addstring(&Buf, "Base64(");
        lua_pushinteger(lua, DataLength);
        luaL_addvalue(&Buf);
        luaL_addstring(&Buf, ",\"");
        Block = luaL_prepbuffsize(&Buf, EncodedLength);
        EdsLib_DisplayDB_Base64Encode(Block, EncodedLength, Data, DataLength);
        luaL_addsize(&Buf, strlen(Block));
        luaL_addstring(&Buf, "\")");
        luaL_pushresult(&Buf);
        break;
    }
    case LUA_TNUMBER:
    case LUA_TBOOLEAN:
    {
        luaL_tolstring(lua, 1, NULL);
        break;
    }
    case LUA_TTABLE:
    {
        lua_pushstring(lua, "{");
        lua_pushnil(lua);
        while(lua_next(lua, 1))
        {
            if (lua_checkstack(lua, 3) && lua_type(lua, -2) == LUA_TSTRING)
            {
                lua_pushcfunction(lua, TestIntf_Remote_ObjToString);
                lua_insert(lua, -2);
                lua_call(lua, 1, 1);
                lua_pushstring(lua, "=");
                lua_insert(lua, -2);
                lua_pushstring(lua, ",");
                lua_pushvalue(lua, -4);
            }
        }
        if (lua_gettop(lua) > 2)
        {
            lua_pop(lua, 1);
        }
        lua_pushstring(lua, "}");
        lua_concat(lua, lua_gettop(lua) - 1);
        break;
    }
    default:
        lua_pushstring(lua, "nil");
        break;
    }

    return 1;
}

static int TestIntf_Remote_DoAction(lua_State *lua)
{
    TestIntf_RemoteConnection_t *Conn = luaL_checkudata(lua, 1, "TestIntf_RemoteConnection");
    char *BufPtr;
    size_t BufLen;
    ssize_t wrsz;
    int narg;
    int idx;

    TestIntf_Poll_Subprocess(Conn);
    if (Conn->OutFd < 0)
    {
        return luaL_error(lua, "Remote subprocess not running");
    }

    BufLen = snprintf(Conn->SendBuffer, sizeof(Conn->SendBuffer), "DoAction(\"%s\"",
            lua_tostring(lua, lua_upvalueindex(1)));

    if (BufLen < sizeof(Conn->SendBuffer))
    {
        BufPtr = &Conn->SendBuffer[BufLen + 1];
    }
    else
    {
        BufPtr = NULL;
    }

    narg = lua_gettop(lua);
    for (idx = 2; idx <= narg && BufPtr != NULL; ++idx)
    {
        lua_pushcfunction(lua, TestIntf_Remote_ObjToString);
        lua_pushvalue(lua, idx);
        lua_call(lua, 1, 1);

        /*
         * note that memccpy() returns a pointer to the byte after the null char,
         * so we need to back it up by one to overwrite the null and append the string.
         * for subsequent args we need to replace this with a comma separator.
         */
        *(BufPtr - 1) = ',';
        BufLen = BufPtr - Conn->SendBuffer;
        BufPtr = memccpy(BufPtr, lua_tostring(lua, -1), 0, sizeof(Conn->SendBuffer) - BufLen);
    }

    if (BufPtr != NULL)
    {
        --BufPtr;
        BufLen = BufPtr - Conn->SendBuffer;
        BufPtr = memccpy(BufPtr, ")\n", 0, sizeof(Conn->SendBuffer) - BufLen);
    }

    if (BufPtr == NULL)
    {
        return luaL_error(lua, "Remote request too long");
    }

    BufLen = BufPtr - Conn->SendBuffer;
    BufPtr = Conn->SendBuffer;
    while (BufLen > 0)
    {
        wrsz = write(Conn->OutFd, BufPtr, BufLen);
        if (wrsz == 0)
        {
            lua_pushstring(lua, "zero length write");
            break;
        }
        if (wrsz > 0)
        {
            BufPtr += wrsz;
            BufLen -= wrsz;
        }
        else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            lua_pushstring(lua, strerror(errno));
            break;
        }
    }

    if (BufLen > 0)
    {
        if (lua_gettop(lua) == narg)
        {
            lua_pushstring(lua, "incomplete write");
        }
        return lua_error(lua);
    }


    return 0;
}

const char * TestIntf_Remote_RecvBuffReader(lua_State *lua, void *Data, size_t *ChunkSize)
{
    TestIntf_RemoteConnection_t *Conn = Data;
    const char *BufPtr;

    if (Conn->RecvBufRdPos == Conn->RecvBufChkPos)
    {
        BufPtr = NULL;
        *ChunkSize = 0;
    }
    else
    {
        size_t StartPos = Conn->RecvBufRdPos & REMOTE_BUFFER_MASK;
        size_t EndPos = Conn->RecvBufChkPos & REMOTE_BUFFER_MASK;
        BufPtr = &Conn->RecvBuffer[StartPos];
        if (StartPos < EndPos)
        {
            /* The whole chunk may be returned */
            *ChunkSize = EndPos - StartPos;
        }
        else
        {
            *ChunkSize = sizeof(Conn->RecvBuffer) - StartPos;
        }
        Conn->RecvBufRdPos += *ChunkSize;
    }

    return BufPtr;
}

static int TestIntf_Remote_ExecuteAction(lua_State *lua)
{
    int tbl = lua_upvalueindex(1);

    while(lua_gettop(lua) > 0)
    {
        printf("%s(): set %s\n", __func__, lua_typename(lua, lua_type(lua, -1)));
        lua_rawseti(lua, tbl, 1 + lua_rawlen(lua, tbl));
    }

    return 0;
}

static int TestIntf_Remote_Base64(lua_State *lua)
{
    uint32_t DataLengthBits = luaL_checkinteger(lua, 1);
    const char *EncodedStr = luaL_checkstring(lua, 2);
    uint32_t DataLengthBytes = (DataLengthBits + 7) / 8;
    luaL_Buffer Buf;

    luaL_buffinit(lua, &Buf);
    EdsLib_DisplayDB_Base64Decode((uint8_t*)luaL_prepbuffsize(&Buf, DataLengthBytes),
            DataLengthBits, EncodedStr);
    luaL_addsize(&Buf, DataLengthBytes);
    luaL_pushresult(&Buf);
    return 1;
}

static int TestIntf_Remote_Wait(lua_State *lua)
{
    TestIntf_RemoteConnection_t *Conn = luaL_checkudata(lua, 1, "TestIntf_RemoteConnection");
    int32_t Timeout = luaL_optinteger(lua, 2, -1);
    struct timeval stime;
    struct timeval *stptr;
    fd_set rdfd;

    printf("%s()\n", __func__);

    TestIntf_Poll_Subprocess(Conn);
    if (Conn->InFd < 0)
    {
        lua_pushboolean(lua, 0);
    }
    else
    {
        if (Timeout >= 0)
        {
            stime.tv_sec = Timeout / 1000;
            stime.tv_usec = 1000 * (Timeout % 1000);
            stptr = &stime;
        }
        else
        {
            stptr = NULL;
        }

        FD_ZERO(&rdfd);
        FD_SET(Conn->InFd, &rdfd);

        if (select(1 + Conn->InFd, &rdfd, NULL, NULL, stptr) < 0)
        {
            if (errno != EINTR)
            {
                /* error reading the filehandle */
                fprintf(stderr,"%s(): select: %s\n", __func__, strerror(errno));
            }
            lua_pushboolean(lua, 0);
        }
        else
        {
            lua_pushboolean(lua, 1);
        }
    }

    return 1;
}

static int TestIntf_Remote_Poll(lua_State *lua)
{
    TestIntf_RemoteConnection_t *Conn = luaL_checkudata(lua, 1, "TestIntf_RemoteConnection");
    int idx;
    size_t TempPos;
    ssize_t rdsz;

    lua_settop(lua, 1);
    lua_newtable(lua);                                     /* idx 2 - action table (initially empty) */

    /*
     * First poll that the subprocess is still alive.
     * If it has ended, we can close the file handles.
     */
    TestIntf_Poll_Subprocess(Conn);

    while (1)
    {
        /*
         * check for anything interesting already in the local buffer
         * before reading more from the pipe.
         */
        while (Conn->RecvBufChkPos != Conn->RecvBufWrPos)
        {
            TempPos = Conn->RecvBufChkPos & REMOTE_BUFFER_MASK;
            ++Conn->RecvBufChkPos;

            if (Conn->RecvBuffer[TempPos] == '\n')
            {
                /*
                 * Check that the buffer is not overrun -- if so the data is all garbage
                 * and must be dropped.
                 */
                if ((Conn->RecvBufWrPos - Conn->RecvBufRdPos) < sizeof(Conn->RecvBuffer))
                {
                    if (lua_gettop(lua) == 2)
                    {
                        /* Save the global registry and replace it with a temporary one */
                        lua_rawgeti(lua, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS); /* idx 3 - saved global enivornment */
                        lua_newtable(lua);                                     /* idx 4 - temp global state */
                        lua_pushcfunction(lua, TestIntf_Remote_Base64);
                        lua_setfield(lua, -2, "Base64");
                        lua_pushvalue(lua, 2);
                        lua_pushcclosure(lua, TestIntf_Remote_ExecuteAction, 1);
                        lua_setfield(lua, -2, "DoAction");
                        lua_rawseti(lua, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS); /* Set as temp/protected global environment */
                    }

                    if (lua_load(lua, TestIntf_Remote_RecvBuffReader, Conn, "in", "t") != LUA_OK)
                    {
                        /* In this case Lua should have pushed an error message */
                        fprintf(stderr, "%s(): Load error: %s\n", __func__, luaL_tolstring(lua, -1, NULL));
                        lua_pop(lua, 2);
                    }
                    else if (lua_pcall(lua, 0, 0, 0)  != LUA_OK)
                    {
                        fprintf(stderr, "%s(): %s\n", __func__, luaL_tolstring(lua, -1, NULL));
                        lua_pop(lua, 2);
                    }
                }

                /*
                 * Normally this would have consumed all data in the buffer.
                 * If not that means data is being dropped and it is probably worth a note.
                 */
                if (Conn->RecvBufRdPos != Conn->RecvBufChkPos)
                {
                    fprintf(stderr, "%s(): dropped %zu bytes\n", __func__, Conn->RecvBufChkPos - Conn->RecvBufRdPos);
                    Conn->RecvBufRdPos = Conn->RecvBufChkPos;
                }

                if (lua_rawlen(lua, 2) > 0)
                {
                    /* Action(s) got added to the table, so exit the loop now */
                    break;
                }
            }
        }

        if (lua_rawlen(lua, 2) > 0)
        {
            /* Action(s) got added to the table, so exit the loop now */
            break;
        }

        if (Conn->InFd < 0)
        {
            /* No valid filehandle to read, nothing more to do */
            break;
        }

        /*
         * Read additional data.
         * Do not read more than 25% of the buffer in one shot -
         * this makes it less likely to overrun in case the remote side
         * starts spewing data, as we can consume some data before reading more.
         *
         * The filehandle should be set to nonblocking (O_NONBLOCK flag)
         * so that there is no risk of getting stuck here.
         */
        TempPos = Conn->RecvBufWrPos & REMOTE_BUFFER_MASK;
        rdsz = sizeof(Conn->RecvBuffer) - TempPos;
        if (rdsz > (sizeof(Conn->RecvBuffer) / 4))
        {
            rdsz = (sizeof(Conn->RecvBuffer) / 4);
        }
        fprintf(stderr, "%s() about to read %ld\n", __func__, (long)rdsz);
        rdsz = read(Conn->InFd, &Conn->RecvBuffer[TempPos], rdsz);
        fprintf(stderr, "%s() read got %ld\n", __func__, (long)rdsz);

        if (rdsz == 0)
        {
            /* this is indicative of an EOF condition.
             * for a pipe this means the subprocess must have ended. */
            break;
        }

        if (rdsz > 0)
        {
            Conn->RecvBufWrPos += rdsz;
        }
        else
        {
            if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
            {
                fprintf(stderr,"%s(): read: %s\n", __func__, strerror(errno));
            }
            break;
        }
    }

    if (lua_gettop(lua) == 3)
    {
        /* Restore the original LUA_RIDX_GLOBALS environment */
        lua_rawseti(lua, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
    }

    for (idx = lua_rawlen(lua, 2); idx > 0; --idx)
    {
        lua_rawgeti(lua, 2, idx);
        printf("%s(): get %d %s\n", __func__, lua_gettop(lua), lua_typename(lua, lua_type(lua, -1)));
    }

    return lua_gettop(lua) - 2;
}

static int TestIntf_Remote_Close(lua_State *lua)
{
    TestIntf_RemoteConnection_t *Conn = luaL_checkudata(lua, 1, "TestIntf_RemoteConnection");
    uint32_t Timeout;
    struct pollfd pfd;
    nfds_t nfds;

    printf("%s(): START\n", __func__);
    /*
     * Close our side of the output pipe.  The subprocess should see
     * this as an EOF on its STDIN stream which (depending on the
     * implementation) may trigger a shutdown/exit procedure.
     */
    if (Conn->OutFd >= 0)
    {
        close(Conn->OutFd);
        Conn->OutFd = -1;
    }

    TestIntf_Poll_Subprocess(Conn);

    /*
     * When the subprocess exits, it should close its stdout stream,
     * which may appear on this side as a broken pipe signal or a closure of
     * the remote side of the pipe.
     *
     * By using select() on the file descriptor for this side of the pipe we
     * can delay until that happens.
     *
     * This needs to be time-limited to protect against the possibility of a
     * rogue subprocess that ignores the TERM signal and refuses to exit.
     */
    Timeout = 100;

    /*
     * If the child process is running then send a TERM signal
     */
    while (Conn->pid > 0)
    {
        if (waitpid(Conn->pid, NULL, WNOHANG) == Conn->pid)
        {
            /* process has exited, status gotten */
            Conn->pid = -1;
            break;
        }

        if (Timeout == 0)
        {
            if (lua_toboolean(lua, lua_upvalueindex(1)))
            {
                /*
                 * did our best but no status was collected --
                 * in garbage collection mode we are losing this reference anyway.
                 *
                 * This will cause the InStream to be closed if it is still open.
                 */
                Conn->pid = -1;
            }
            break;
        }

        /*
         * subprocess is not exiting on its own ...
         *
         * First try sending SIGTERM to do graceful shutdown
         * Then do subsequently more aggressive signals to get it to exit
         *
         * Note that some tools only respond to certain signals --
         * e.g. 'nc' in listen mode when nothing is connected only seems to respond to SIGINT
         *
         * If that doesn't work and we are in garbage collection mode (upvalue 1)
         * then finally send a SIGKILL which cannot be ignored
         * (this should take care of a subprocess which is hung)
         */
        if (Timeout == 90)
        {
            fprintf(stderr, "Sending SIGTERM to child task\n");
            kill(Conn->pid, SIGTERM);
        }
        else if (Timeout == 70)
        {
            fprintf(stderr, "Sending SIGQUIT to child task\n");
            kill(Conn->pid, SIGQUIT);
        }
        else if (Timeout == 50)
        {
            fprintf(stderr, "Sending SIGINT to child task\n");
            kill(Conn->pid, SIGINT);
        }
        else if (Timeout == 30 && lua_toboolean(lua, lua_upvalueindex(1)))
        {
            fprintf(stderr, "Sending SIGKILL to child task\n");
            kill(Conn->pid, SIGKILL);
        }
        else
        {
            /*
             * Wait up to 20ms for something to happen.
             * If the input pipe is not yet closed then call poll() on it.
             * Otherwise use poll() as a way to sleep for a bit.
             */
            memset(&pfd, 0, sizeof(pfd));
            if (Conn->InFd >= 0)
            {
                nfds = 1;
                pfd.fd = Conn->InFd;
                pfd.events = POLLIN;
            }
            else
            {
                nfds = 0;
            }

            if (poll(&pfd, nfds, 20) > 0)
            {
                /*
                 * in case there was actual data available, read it
                 * all data at this point is discarded.
                 */
                if (read(Conn->InFd, Conn->RecvBuffer, sizeof(Conn->RecvBuffer)) == 0)
                {
                    /*
                     * zero length read is eof indicator.
                     * This means the remote end of the pipe is closed.
                     * At this point just poll for task to exit.
                     */
                    close(Conn->InFd);
                    Conn->InFd = -1;
                }
            }
        }

        --Timeout;
    }

    /*
     * Ensure that the input read pipe is also closed.
     */
    if (Conn->InFd >= 0 && Conn->pid <= 0)
    {
        close(Conn->InFd);
        Conn->InFd = -1;
    }

    return 0;
}

int TestIntf_Remote_Create(lua_State *lua)
{
    const char *ShellCmd = luaL_checkstring(lua, 2);
    TestIntf_RemoteConnection_t *Conn;
    int rdpipe[2];
    int wrpipe[2];
    int status;

    Conn = lua_newuserdata(lua, sizeof(TestIntf_RemoteConnection_t));
    if (luaL_newmetatable(lua, "TestIntf_RemoteConnection"))
    {
        lua_newtable(lua);

        lua_pushstring(lua, "Message");
        lua_pushcfunction(lua, TestIntf_Remote_DoAction);
        lua_setfield(lua, -2, "Send");

        lua_pushcfunction(lua, TestIntf_Remote_Poll);
        lua_setfield(lua, -2, "Poll");

        lua_pushcfunction(lua, TestIntf_Remote_Wait);
        lua_setfield(lua, -2, "Wait");

        lua_pushboolean(lua, 0);
        lua_pushcclosure(lua, TestIntf_Remote_Close, 1);
        lua_setfield(lua, -2, "Close");

        lua_pushstring(lua, "Subscribe");
        lua_pushcclosure(lua, TestIntf_Remote_DoAction, 1);
        lua_setfield(lua, -2, "Subscribe");

        lua_pushstring(lua, "Unsubscribe");
        lua_pushcclosure(lua, TestIntf_Remote_DoAction, 1);
        lua_setfield(lua, -2, "Unsubscribe");

        lua_setfield(lua, -2, "__index");

        lua_pushboolean(lua, 1);
        lua_pushcclosure(lua, TestIntf_Remote_Close, 1);
        lua_setfield(lua, -2, "__gc");
    }
    lua_setmetatable(lua, -2);

    memset(Conn, 0, sizeof(*Conn));
    Conn->InFd = -1;
    Conn->OutFd = -1;
    Conn->pid = -1;

    status = pipe(rdpipe);
    if (status < 0)
    {
        return luaL_error(lua, "pipe(rd): %s", strerror(errno));
    }

    status = pipe(wrpipe);
    if (status < 0)
    {
        close(rdpipe[1]);
        close(rdpipe[0]);
        return luaL_error(lua, "pipe(wr): %s", strerror(errno));
    }

    Conn->pid = fork();
    if (Conn->pid < 0)
    {
        /* Some error occurred */
        close(rdpipe[1]);
        close(rdpipe[0]);
        close(wrpipe[1]);
        close(wrpipe[0]);
        return luaL_error(lua, "fork(): %s", strerror(errno));
    }

    if (Conn->pid == 0)
    {
        /* This is the child process */
        close(rdpipe[0]); /* read side */
        close(wrpipe[1]); /* write side */

        dup2(wrpipe[0], STDIN_FILENO);
        close(wrpipe[0]);
        dup2(rdpipe[1], STDOUT_FILENO);
        close(rdpipe[1]);

        /* Treat the entire string as a command line to pass to the shell for interpretation */
        fprintf(stderr, "running: %s\n", ShellCmd);
        status = execl("/bin/sh", "sh", "-c", ShellCmd, NULL);
        if (status < 0)
        {
            /*
             * Error occurred -- cannot throw a Lua error anymore since this
             * is executing in a child process.
             */
            fprintf(stderr, "execl(): %s\n", strerror(errno));
        }

        /* In case e.g. exec did not work, then exit with a failure status code */
        exit(EXIT_FAILURE);
    }

    /*
     * This is the parent process
     * Set up the InStream / OutStream FILE objects from the file descriptors.
     */
    close(rdpipe[1]);   /* write side */
    close(wrpipe[0]);   /* read side */
    Conn->InFd = rdpipe[0];
    Conn->OutFd = wrpipe[1];

    status = fcntl(Conn->InFd, F_GETFL);
    status |= O_NONBLOCK;
    fcntl(Conn->InFd, F_SETFL, status);

    return 1;
} /* end TestIntf_Remote_Create */

