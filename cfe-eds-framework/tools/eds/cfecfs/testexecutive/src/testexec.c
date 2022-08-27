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
 * \file     testexec.c
 * \ingroup  testexecutive
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation of test executive
 */

#include <stdlib.h>
#include <string.h> /* memset() */

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "osapi.h"
#include "utbsp.h"
#include "uttest.h"
#include "utassert.h"

#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"
#include "edslib_displaydb.h"
#include "cfe_missionlib_runtime.h"
#include "cfe_missionlib_api.h"
#include "edslib_lua_objects.h"
#include "cfe_missionlib_lua_softwarebus.h"

#include "testexec.h"

static lua_State *BaseState;
static int BaseStackTop;

typedef struct
{
    int32 FileId;
    char DataBuffer[252];
} TestExec_OsalLoadState_t;


/*
**  volume table.
*/
OS_VolumeInfo_t OS_VolumeTable [OS_MAX_FILE_SYSTEMS] =
{
        /*
         ** The following entry is a "pre-mounted" path to a non-volatile device
         ** This is intended to contain the functional test scripts
         */
        { "ftest",   "./ftest", FS_BASED, FALSE, FALSE,  TRUE, "FT", "/ftest",   512  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  },
        { "unused",   "unused", FS_BASED,  TRUE,  TRUE, FALSE,  " ",      " ",     0  }
};


static const char *TestExec_Lua_FileReader(lua_State *lua, void *data, size_t *size)
{
    TestExec_OsalLoadState_t *LoadState = data;
    int32 Status;

    Status = OS_read(LoadState->FileId, LoadState->DataBuffer, sizeof (LoadState->DataBuffer));
    if (Status < 0)
    {
        *size = 0;
        return NULL;
    }

    *size = Status;
    return LoadState->DataBuffer;
}

static int TestExec_Lua_DoAssert(lua_State *lua)
{
    /* stack:
     *  @1 = condition  (boolean)
     *  @2 = message    (string)
     */
    lua_Debug ar;
    const char *filename;

    if (lua_getstack(lua, 1, &ar))
    {
        lua_getinfo(lua, "Sl", &ar);
    }
    else
    {
        memset(&ar, 0, sizeof(ar));
    }

    if (ar.source != NULL && *ar.source == '@')
    {
        filename = ar.source + 1;
    }
    else
    {
        filename = NULL;
        ar.currentline = -1;
    }

    UtAssertEx(lua_toboolean(lua, 1), UtAssert_GetContext(),
            filename, ar.currentline, "%s", luaL_tolstring(lua, 2, NULL));

    return 0;
}

static int TestExec_Lua_ErrorHandler(lua_State *lua)
{
    /* Start stack: 1 = user string */
    lua_Debug ar;
    const char *file;
    uint32 line;

    file = NULL;
    line = 0;
    if (lua_getstack(lua, 1, &ar) && lua_getinfo(lua, "Sl", &ar))
    {
        file = ar.source;
        if (file != NULL && *file == '@')
        {
            ++file;
        }
        line = ar.currentline;
    }

    UtAssertEx(FALSE, UTASSERT_CASETYPE_FAILURE, file, line, "%s", lua_tostring(lua, 1));

    lua_getglobal(lua, "debug");
    lua_getfield(lua, -1, "traceback");
    lua_pushnil(lua);
    lua_pushinteger(lua, 3);
    lua_call(lua, 2, 1);
    UT_BSP_DoText(UTASSERT_CASETYPE_DEBUG, lua_tostring(lua, -1));
    return 0;
}


static void TestExec_LoadScript (void)
{
    const char *SegmentName = UtAssert_GetSegmentName();
    TestExec_OsalLoadState_t LoadState;
    int32 Status;

    Status = OS_open(SegmentName, OS_READ_ONLY, 0);
    if (Status < 0)
    {
        os_err_name_t ErrName;
        OS_GetErrorName(Status, &ErrName);
        lua_pushfstring(BaseState, "OS_open(%s): %s", SegmentName, ErrName);
    }
    else
    {
        LoadState.FileId = Status;
        lua_load(BaseState, TestExec_Lua_FileReader, &LoadState, SegmentName, NULL);
        OS_close(LoadState.FileId);
    }

    if (lua_type(BaseState, -1) == LUA_TSTRING)
    {
        UtAssert_Failed("%s", lua_tostring(BaseState, -1));
    }
    else
    {
        UtAssert_True(lua_isfunction(BaseState, -1), "Load test script: %s", SegmentName);
    }

}

static void TestExec_RunScript (void)
{
    if (lua_type(BaseState, -1) == LUA_TFUNCTION)
    {
        lua_pcall(BaseState, 0, 0, BaseStackTop);
    }
}

static void TestExec_ResetStack (void)
{
    lua_settop(BaseState, BaseStackTop);
}

void OS_Application_Shutdown(void)
{
    lua_close(BaseState);
}

void OS_Application_Startup(void)
{
    int32 i;
    const char *OptString;

    if (OS_API_Init() != OS_SUCCESS)
    {
        UtAssert_Abort("OS_API_Init() failed");
    }

    atexit(OS_Application_Shutdown);

    /*
     * Create a new Lua state and load the standard libraries
     */
    BaseState = luaL_newstate();
    luaL_openlibs(BaseState);

    EdsLib_Lua_Attach(BaseState, &EDS_DATABASE);
    CFE_MissionLib_Lua_SoftwareBus_Attach(BaseState, &CFE_SOFTWAREBUS_INTERFACE);
    lua_setglobal(BaseState, "EdsDB");

    lua_pushcfunction(BaseState, TestIntf_GetFactory);
    lua_call(BaseState, 0, 1);
    lua_setglobal(BaseState, "NewConnection");

    lua_pushinteger(BaseState, UTASSERT_CASETYPE_FAILURE);
    lua_pushcclosure(BaseState, TestExec_Lua_DoAssert, 1);
    lua_setglobal(BaseState, "Assert");

    lua_pushcfunction(BaseState, TestExec_Lua_ErrorHandler);
    BaseStackTop = lua_gettop(BaseState);

    for (i=0; i < UT_BSP_GetTotalOptions(); ++i)
    {
        OptString = UT_BSP_GetOptionString(i);
        if (OptString != NULL)
        {

            /* Check if the option has an assignment operator.
             * If so, pass it directly to Lua for interpretation
             * (it can e.g. set a global)
             */
            if (strchr(OptString, '=') != NULL)
            {
                luaL_loadstring(BaseState, OptString);
                lua_pcall(BaseState, 0, 0, 1);
            }
            else
            {
                UtTest_Add(TestExec_RunScript, TestExec_LoadScript, TestExec_ResetStack, OptString);
            }
        }

    }

#ifdef JPHFIX_CLIENT
   OS_SocketAddrFromString(&SocketAddress, "127.0.0.1");
   Status = OS_SocketConnect(SocketID, &SocketAddress, OS_PEND);
   if (Status != OS_SUCCESS)
   {
       fprintf(stderr,"OS_SocketConnect() failed: %d\n", (int)Status);
       return;
   }

   while (1)
   {
       Status = OS_StreamRead(SocketID, DataBuf, sizeof(DataBuf)-1, OS_PEND);
       if (Status < OS_SUCCESS)
       {
           fprintf(stderr,"OS_StreamRead() failed: %d\n", (int)Status);
           break;
       }

       if (Status == 0)
       {
           printf("OS_StreamRead(): disconnect\n");
           break;
       }

       DataBuf[Status] = 0;
       printf("OS_StreamRead(): %s\n", DataBuf);
   }
#endif


} /* end OS_Application Startup */

