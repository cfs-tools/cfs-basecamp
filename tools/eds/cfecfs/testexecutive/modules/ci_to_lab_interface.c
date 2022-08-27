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
 * \file     ci_to_lab_interface.c
 * \ingroup  testexecutive
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation of test executive
 */

#include <stdlib.h>
#include <string.h> /* memset() */
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"
#include "edslib_displaydb.h"
#include "cfe_missionlib_runtime.h"
#include "cfe_missionlib_api.h"
#include "edslib_lua_objects.h"
#include "cfe_missionlib_lua_softwarebus.h"

#include "testexec.h"
#include "to_lab_eds_typedefs.h"

#define TO_LAB_BASE_PORT        1235
#define CI_LAB_BASE_PORT        1234

#define UDP_NET_BUFFER_SIZE     8192


typedef struct
{
    CFE_SB_SoftwareBus_PubSub_Interface_t TargetApp;
    struct sockaddr_in LocalAddr;
    struct sockaddr_in TargetAddr;
    struct timespec InterPacketDelay;
    int SocketId;
    luaL_Buffer TempBuf;
} TestIntf_CI_TO_LAB_Connection_t;

static int TestIntf_CI_TO_LAB_Send(lua_State *lua)
{
    TestIntf_CI_TO_LAB_Connection_t *Conn = luaL_checkudata(lua, 1, "TestIntf_CI_TO_LAB_Connection");
    const char *Data = luaL_checkstring(lua, 2);
    uint32_t DataLength = lua_rawlen(lua, 2);

    printf("%s():\n",__func__);
    EdsLib_Generate_Hexdump(stdout, (const uint8_t*)Data, 0, DataLength);
    printf("\n");

    sendto(Conn->SocketId, Data, DataLength, 0,
            (struct sockaddr *)&Conn->TargetAddr, sizeof(Conn->TargetAddr));

    clock_nanosleep(CLOCK_REALTIME, 0, &Conn->InterPacketDelay, NULL);

    return 0;
}

static int TestIntf_CI_TO_LAB_Wait(lua_State *lua)
{
    TestIntf_CI_TO_LAB_Connection_t *Conn = luaL_checkudata(lua, 1, "TestIntf_CI_TO_LAB_Connection");
    int32_t Timeout = luaL_optinteger(lua, 2, -1);
    fd_set rdfd;
    struct timeval tv;
    struct timeval *ptv;

    if (Timeout >= 0)
    {
        tv.tv_sec = Timeout / 1000;
        tv.tv_usec = 1000 * (Timeout % 1000);
        ptv = &tv;
    }
    else
    {
        ptv = NULL;
    }

    FD_ZERO(&rdfd);
    FD_SET(Conn->SocketId, &rdfd);

    if (select(1 + Conn->SocketId, &rdfd, NULL, NULL, ptv) < 0)
    {
        lua_pushboolean(lua, 0);
    }
    else
    {
        lua_pushboolean(lua, 1);
    }

    return 1;
}

static int TestIntf_CI_TO_LAB_Poll(lua_State *lua)
{
    TestIntf_CI_TO_LAB_Connection_t *Conn = luaL_checkudata(lua, 1, "TestIntf_CI_TO_LAB_Connection");
    ssize_t Length;

    lua_pushstring(lua, "Message");
    luaL_buffinit(lua, &Conn->TempBuf);

    Length = recv(Conn->SocketId,
            luaL_prepbuffsize(&Conn->TempBuf, UDP_NET_BUFFER_SIZE),
            UDP_NET_BUFFER_SIZE, MSG_DONTWAIT);

    if (Length <= 0)
    {
        luaL_pushresultsize(&Conn->TempBuf, 0);
        return 0;
    }

    luaL_pushresultsize(&Conn->TempBuf, Length);
    return 2;
}

static int TestIntf_CI_TO_LAB_CheckSubscribeEvent(lua_State *lua)
{
    const char *AppName;

    lua_settop(lua, 1);
    luaL_checkudata(lua, 1, "EdsLib_Object");

    lua_getfield(lua, 1, "Payload");
    lua_getfield(lua, 2, "PacketID");
    lua_getfield(lua, 3, "AppName");
    lua_call(lua, 0, 1);
    AppName = lua_tostring(lua, -1);
    if (AppName == NULL || strcmp(AppName, "TO_LAB_APP") != 0)
    {
        return 0;
    }


    lua_getfield(lua, 3, "EventID");
    lua_call(lua, 0, 1);

    if (lua_compare(lua, lua_upvalueindex(1), -1, LUA_OPEQ))
    {
        printf("%s(): TRUE\n", __func__);
        lua_settop(lua, 1);
        return 1;
    }

    return 0;
}

static int TestIntf_CI_TO_LAB_DoSubscription(lua_State *lua)
{
    int IsSubscribe;
    luaL_checkudata(lua, 1, "TestIntf_CI_TO_LAB_Connection");

    lua_settop(lua, 3);
    lua_getglobal(lua, "EdsDB");                /* top@ 4 */
    lua_pushcfunction(lua, TestIntf_CI_TO_LAB_Send);  /* top@ 5 */
    lua_pushvalue(lua, 1);                      /* top@ 6 */
    lua_getfield(lua, 4, "Encode");            /* top@ 7 */
    lua_getfield(lua, 4, "NewMessage");        /* top@ 8 */

    lua_getuservalue(lua, 1);                   /* top@ 9 */
    lua_getfield(lua, -1, "Interface");         /* top@ 10 */
    lua_remove(lua, -2);                        /* uservalue, leave interface object @9 */

    /* Push the operation type - this is either "AddPacket" (subscribe) or "RemovePacket" (unsubscribe) */
    lua_pushvalue(lua, lua_upvalueindex(1));    /* top@ 10 */
    IsSubscribe = (strcmp(lua_tostring(lua, -1), "AddPacket") == 0);
    lua_call(lua, 2, 1);                        /* Call EdsDB.NewMessage(intf, command) - result @8 */

    lua_getfield(lua, -1, "Payload");           /* Payload @9 */
    lua_pushinteger(lua, 5);
    lua_setfield(lua, -2, "BufLimit");

    /* Incorporate any user-specified attributes */
    if (lua_istable(lua, 3))
    {
        lua_pushnil(lua);
        while (lua_next(lua, 3))
        {
            lua_pushvalue(lua, -2);
            lua_insert(lua, -2);
            lua_settable(lua, 9);
        }
    }

    /* Set the "MsgId" parameter to be the second argument */
    lua_pushvalue(lua, 2);
    lua_setfield(lua, 9, "MsgId");
    lua_pop(lua, 1);

    lua_call(lua, 1, 1);                        /* Call EdsDB.Encode(msg) - result@ 7 */

    /* Send the subscription/unsubscription message via the normal Send() function */
    lua_call(lua, 2, 0);                        /* Call TestIntf_CI_TO_LAB_Send() - top@ 4 */

    /*
     * On subscribe requests, it should delay until the subscription takes effect.
     * Return a wait table value that specifies to wait for CFE_EVS EVENTMSG
     * further specify a callback function that looks specifically for TO_LAB_APP event 15 (subscribe)
     */
    lua_getuservalue(lua, 1);                               /* top@ 5 */
    lua_newtable(lua);                                      /* top@ 6 */
    lua_newtable(lua);                                      /* top@ 7 */
    lua_getfield(lua, 5, "EventIntf");
    lua_setfield(lua, 7, "Interface");

    /*
     * JPHFIX: the magic numbers here are the event IDs for subscribe/unsubscribe.
     * These should be defined as an enumeration in the EDS to clean this up.
     */
    if (IsSubscribe)
    {
        lua_pushinteger(lua, 15);  /* subscribe event id */
    }
    else
    {
        lua_pushinteger(lua, 16); /* unsubscribe event id */
    }

    lua_pushcclosure(lua, TestIntf_CI_TO_LAB_CheckSubscribeEvent, 1);
    lua_setfield(lua, 7, "Callback");
    lua_rawseti(lua, 6, 1);
    lua_pushinteger(lua, 5000);

    return 2;
}

static int TestIntf_CI_TO_LAB_Destroy(lua_State *lua)
{
    TestIntf_CI_TO_LAB_Connection_t *Conn = luaL_checkudata(lua, 1, "TestIntf_CI_TO_LAB_Connection");

    if (Conn->SocketId >= 0)
    {
        close(Conn->SocketId);
        Conn->SocketId = -1;
    }

    return 0;
}

int TestIntf_CI_TO_LAB_Create(lua_State *lua)
{
    uint16_t TargetNum = luaL_checkinteger(lua, 1);
    uint16_t LocalPort = 0;
    uint16_t TargetPort = 0;
    uint32_t InterPacketDelay = 25;
    const char *LocalIP = NULL;
    const char *TargetIP = NULL;
    TestIntf_CI_TO_LAB_Connection_t *Conn;

    if (lua_isstring(lua, 2))
    {
        TargetIP = lua_tostring(lua, 2);
    }
    else if (lua_istable(lua, 2))
    {
        /* Trim the stack to 2 entries in case extra args were passed */
        lua_settop(lua, 2);

        lua_getfield(lua, 2, "TargetIP");
        TargetIP = lua_tostring(lua, -1);
        lua_getfield(lua, 2, "LocalIP");
        LocalIP = lua_tostring(lua, -1);

        /*
         * Note that these strings cannot be popped yet,
         * they must sit on the stack until we are done with
         * the C strings above
         */

        lua_getfield(lua, 2, "TargetPort");
        if (lua_isnumber(lua, -1))
        {
            TargetPort = lua_tointeger(lua, -1);
        }
        lua_pop(lua, 1);

        lua_getfield(lua, 2, "TargetIP");
        if (lua_isnumber(lua, -1))
        {
            LocalPort = lua_tointeger(lua, -1);
        }
        lua_pop(lua, 1);

        lua_getfield(lua, 2, "InterPacketDelay");
        if (lua_isnumber(lua, -1))
        {
            InterPacketDelay = lua_tointeger(lua, -1);
        }
        lua_pop(lua, 1);
    }

    /*
     * Always adjust the stack to the same level so absolute refs can be used.
     */
    lua_settop(lua, 4);

    if (TargetIP == NULL)
    {
        /* If user did not specify target, assume it is running on localhost */
        TargetIP = "127.0.0.1";
    }

    if (LocalIP == NULL)
    {
        /* If user did not specify target, assume it is running on localhost */
        LocalIP = "127.0.0.1";
    }

    if (TargetPort == 0)
    {
        /* If unspecified, send to the default UDP port that CI_LAB listens on */
        TargetPort = CI_LAB_BASE_PORT + TargetNum - 1;
    }

    if (LocalPort == 0)
    {
        /* If unspecified, bind to the default UDP port that TO_LAB sends to */
        LocalPort = TO_LAB_BASE_PORT + TargetNum - 1;
    }

    /* index 5 -- this is the object that will be returned */
    Conn = lua_newuserdata(lua, sizeof(TestIntf_CI_TO_LAB_Connection_t));
    if (luaL_newmetatable(lua, "TestIntf_CI_TO_LAB_Connection"))
    {
        lua_newtable(lua);

        lua_pushcfunction(lua, TestIntf_CI_TO_LAB_Send);
        lua_setfield(lua, -2, "Send");
        lua_pushcfunction(lua, TestIntf_CI_TO_LAB_Wait);
        lua_setfield(lua, -2, "Wait");
        lua_pushcfunction(lua, TestIntf_CI_TO_LAB_Poll);
        lua_setfield(lua, -2, "Poll");

        lua_pushstring(lua, "AddPacket");
        lua_pushcclosure(lua, TestIntf_CI_TO_LAB_DoSubscription, 1);
        lua_setfield(lua, -2, "Subscribe");
        lua_pushstring(lua, "RemovePacket");
        lua_pushcclosure(lua, TestIntf_CI_TO_LAB_DoSubscription, 1);
        lua_setfield(lua, -2, "Unsubscribe");

        lua_setfield(lua, -2, "__index");

        lua_pushcfunction(lua, TestIntf_CI_TO_LAB_Destroy);
        lua_setfield(lua, -2, "__gc");
    }
    lua_setmetatable(lua, -2);

    memset(Conn, 0, sizeof(*Conn));

    Conn->SocketId = socket(PF_INET, SOCK_DGRAM, 0);
    if (Conn->SocketId < 0)
    {
        return luaL_error(lua, "Error opening socket: %s", strerror(errno));
    }

    Conn->InterPacketDelay.tv_sec = InterPacketDelay / 1000;
    Conn->InterPacketDelay.tv_nsec = 1000000 * (InterPacketDelay % 1000);
    Conn->LocalAddr.sin_family = AF_INET;
    Conn->TargetAddr.sin_family = AF_INET;
    if (inet_pton(AF_INET, TargetIP, &Conn->TargetAddr.sin_addr) <= 0)
    {
        return luaL_argerror(lua, 2, "Invalid target IP address");
    }
    if (inet_pton(AF_INET, LocalIP, &Conn->LocalAddr.sin_addr) <= 0)
    {
        return luaL_argerror(lua, 2, "Invalid local IP address");
    }

    Conn->LocalAddr.sin_port = htons(LocalPort);
    Conn->TargetAddr.sin_port = htons(TargetPort);

    if (bind(Conn->SocketId, (struct sockaddr *)&Conn->LocalAddr, sizeof(Conn->LocalAddr)) < 0)
    {
        return luaL_error(lua, "Failed to bind: %s", strerror(errno));
    }

    lua_pushcfunction(lua, TestIntf_CI_TO_LAB_Send);  /* top@ 6 */
    lua_getglobal(lua, "EdsDB");                /* top@ 7 */
    lua_getfield(lua, -1, "Encode");
    lua_getfield(lua, -2, "NewMessage");
    lua_getfield(lua, -3, "GetInterface");

    lua_pushvalue(lua, 5);                      /* Connection object - top@ 11 */
    lua_replace(lua, 7);                        /* Replace the EdsDB object @7 - top @10 */

    /*
     * Top of Lua stack should currently be arranged as follows:
     * @ 6 - TestIntf_CI_TO_LAB_Send() function
     * @ 7 - Connection object (1st argument to TestIntf_CI_TO_LAB_Send)
     * @ 8 - EdsDB.Encode() function (return value is 2nd argument to TestIntf_CI_TO_LAB_Send)
     * @ 9 - EdsDB.NewMessage() function (return value is 1st argument to EdsDB.Encode)
     * @ 10 - EdsDB.GetInterface() function (return value is argument to EdsDB.NewMessage)
     */

    /*
     * Save the interface object for future use.
     * Create a table to use as the uservalue and set this as the "Interface" field in it.
     * The CFE_EVS "EVENT_MSG" interface is relevant to check for subscription events.
     */
    lua_newtable(lua);                          /* top @11 */
    lua_pushvalue(lua, -2);                     /* GetInterface function - top@ 12 */
    lua_pushstring(lua, "CFE_EVS/Application/EVENT_MSG");   /* top@ 13 */
    lua_call(lua, 1, 1);                        /* Call GetInterface("CFE_EVS/Application/EVENT_MSG") - result@ 12 */
    if (lua_type(lua, -1) != LUA_TUSERDATA)
    {
        return luaL_error(lua, "Unknown interface \'CFE_EVS/Application/EVENT_MSG\'");
    }
    lua_setfield(lua, -2, "EventIntf");         /* Set object as the "EventIntf" field within the table - top@ 11 */
    lua_pushvalue(lua, -2);                     /* GetInterface function - top@ 12 */
    lua_pushstring(lua, "TO_LAB/Application/CMD");          /* top@ 13 */
    lua_call(lua, 1, 1);                        /* Call GetInterface("TO_LAB/Application/CMD") - result@ 12 */
    if (lua_type(lua, -1) != LUA_TUSERDATA)
    {
        return luaL_error(lua, "Unknown interface \'TO_LAB/Application/CMD\'");
    }
    lua_pushvalue(lua, -1);                     /* save a copy of the intf for next call to NewMessage */
    lua_replace(lua, 10);
    lua_setfield(lua, -2, "Interface");         /* Set object as the "Interface" field within the table - top@ 11 */
    lua_setuservalue(lua, 7);                   /* Set the table as the uservalue for the returned object - top@ 10 */

    /*
     * Create an "OutputEnable" message based on the interface
     */
    lua_pushstring(lua, "EnableOutput");        /* top@ 11 */
    lua_call(lua, 2, 1);                        /* Call NewMessage(intf, "OutputEnable") - result@ 9 */

    /* Set the Payload.dest_IP member to be the value specified by LocalIP */
    lua_getfield(lua, -1, "Payload");           /* top@ 10 */
    lua_pushstring(lua, LocalIP);               /* top@ 11 */
    lua_setfield(lua, -2, "dest_IP");           /* top@ 10 */
    lua_pop(lua, 1);                            /* top@ 9  */

    lua_call(lua, 1, 1);                        /* Call Encode(msg) - result@ 8 */
    lua_call(lua, 2, 0);                        /* Call TestIntf_CI_TO_LAB_Send(msg) - no return value - top@ 5 */

    /* Stack top should be back at 5 (the connection object) */

    return 1;
} /* end TestIntf_CI_TO_LAB_Create */
