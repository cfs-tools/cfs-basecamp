/************************************************************************
 * NASA Docket No. GSC-18,719-1, and identified as “core Flight System: Bootes”
 *
 * Copyright (c) 2020 United States Government as represented by the
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
 * @file
 *   Sample CFS library
 */

/*************************************************************************
** Includes
*************************************************************************/
#include <assert.h>
#include "scriptengine_internal.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <osapi.h>
#include <cfe.h>

#include <edslib_init.h>
#include <edslib_binding_objects.h>
#include <edslib_lua_objects.h>

#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"
#include "cfe_missionlib_lua_softwarebus.h"

/* HACK - this is passed direct to Lua, so it must be a native path, not an OSAL path */
#define SCRIPTENGINE_TESTFILE       "./cf/testscript.lua"
#define SCRIPTENGINE_ERRHANDLER_IDX 1

SCRIPTENGINE_Global_t SCRIPTENGINE_Global;

typedef struct SCRIPTENGINE_PipeWrapper
{
    CFE_SB_PipeId_t PipeId;
} SCRIPTENGINE_PipeWrapper_t;

static int SCRIPTENGINE_ErrorHandler(lua_State *lua)
{
    lua_getglobal(lua, "debug");
    lua_getfield(lua, -1, "traceback");
    lua_remove(lua, -2);
    lua_pushnil(lua);
    lua_pushinteger(lua, 3);
    lua_call(lua, 2, 1);
    OS_printf("LUA Error: %s\n%s\n", lua_tostring(lua, 1), lua_tostring(lua, 2));
    return 0;
}

static void *SCRIPTENGINE_NewEdsObjectBase(lua_State *lua, EdsLib_Id_t EdsId, size_t AllocSize)
{
    EdsLib_Binding_DescriptorObject_t *DescObj;

    /* JPHFIX: This chunk of code should be part of the bindings */
    DescObj = lua_newuserdata(lua, AllocSize);
    EdsLib_Binding_InitDescriptor(DescObj, &EDS_DATABASE, EdsId);
    luaL_getmetatable(lua, "EdsLib_Object");
    lua_setmetatable(lua, -2);

    return DescObj;
}

static EdsLib_Binding_DescriptorObject_t *
SCRIPTENGINE_NewEdsObjectWrapper(lua_State *lua, EdsLib_Id_t EdsId, void *DataPtr)
{
    struct ContentWrapper
    {
        EdsLib_Binding_DescriptorObject_t DescObj;
        EdsLib_Binding_Buffer_Content_t   BufObj;
    } *ObjectUserData;

    ObjectUserData = SCRIPTENGINE_NewEdsObjectBase(lua, EdsId, sizeof(struct ContentWrapper));

    EdsLib_Binding_InitUnmanagedBuffer(&ObjectUserData->BufObj, DataPtr, ObjectUserData->DescObj.Length);
    EdsLib_Binding_SetDescBuffer(&ObjectUserData->DescObj, &ObjectUserData->BufObj);

    return &ObjectUserData->DescObj;
}

static EdsLib_Binding_DescriptorObject_t *
SCRIPTENGINE_NewEdsObjectWithContent(lua_State *lua, EdsLib_Id_t EdsId, size_t MaxContentSize)
{
    struct ContentWrapper
    {
        EdsLib_Binding_DescriptorObject_t DescObj;
        EdsLib_Binding_Buffer_Content_t   BufObj;
        EdsLib_GenericValueUnion_t        ContentStart;
    } *ObjectUserData;

    ObjectUserData = SCRIPTENGINE_NewEdsObjectBase(lua,
                                                   EdsId,
                                                   sizeof(struct ContentWrapper) + MaxContentSize
                                                       - sizeof(EdsLib_GenericValueUnion_t));

    EdsLib_Binding_InitUnmanagedBuffer(&ObjectUserData->BufObj, &ObjectUserData->ContentStart, MaxContentSize);
    EdsLib_Binding_SetDescBuffer(&ObjectUserData->DescObj, &ObjectUserData->BufObj);

    return &ObjectUserData->DescObj;
}

static int SCRIPTENGINE_SendMsg(lua_State *lua)
{
    void  *ObjPtr;
    size_t ObjSize;

    EdsLib_LuaBinding_GetNativeObject(lua, 1, &ObjPtr, &ObjSize);

    OS_printf("SCRIPTENGINE_SendMsg\n");
    EdsLib_Generate_Hexdump(stdout, ObjPtr, 0, ObjSize);

    CFE_SB_TransmitMsg(ObjPtr, false);

    return 0;
}

static int SCRIPTENGINE_CreatePipe(lua_State *lua)
{
    lua_Integer      Depth    = luaL_checkinteger(lua, 1);
    const char      *PipeName = luaL_checkstring(lua, 2);
    CFE_SB_PipeId_t *PipeObj;

    PipeObj = lua_newuserdata(lua, sizeof(*PipeObj));

    /*
     * Set metatable for the new object
     */
    luaL_getmetatable(lua, "CFE_SB_PipeId");
    lua_setmetatable(lua, -2);

    CFE_SB_CreatePipe(PipeObj, Depth, PipeName);

    return 1;
}

static int SCRIPTENGINE_DeletePipe(lua_State *lua)
{
    CFE_SB_PipeId_t *PipeObj = luaL_testudata(lua, 1, "CFE_SB_PipeId");

    if (PipeObj != NULL)
    {
        CFE_SB_DeletePipe(*PipeObj);
    }

    return 0;
}

static int SCRIPTENGINE_WaitFor(lua_State *lua)
{
    CFE_MissionLib_Lua_Interface_Userdata_t *IntfObj = luaL_checkudata(lua, 1, "CFE_MissionLib_Lua_Interface");
    CFE_SB_PipeId_t                         *PipeObj = luaL_checkudata(lua, 2, "CFE_SB_PipeId");
    lua_Integer                              timeout = luaL_optinteger(lua, 3, 1000);
    EdsInterface_CFE_SB_SoftwareBus_PubSub_t PubSub;
    CFE_SB_Buffer_t                         *BufPtr;
    CFE_MSG_Size_t                           MsgSize;
    EdsLib_Binding_DescriptorObject_t       *ObjectUserData;
    EdsLib_DataTypeDB_DerivativeObjectInfo_t DerivObjInfo;
    int32                                    RcvStatus;
    void                                    *LuaObj;
    int                                      nret;

    nret   = 0;
    BufPtr = NULL;

    /* Map the Intf to a MsgID */
    CFE_MissionLib_Lua_MapPubSubParams(&PubSub, IntfObj);

    /* If this succeeds, then this must also unsubscribe later */
    RcvStatus = CFE_SB_Subscribe(PubSub.MsgId, *PipeObj);
    if (RcvStatus == CFE_SUCCESS)
    {
        RcvStatus = CFE_SB_ReceiveBuffer(&BufPtr, *PipeObj, timeout);
        CFE_SB_Unsubscribe(PubSub.MsgId, *PipeObj);
    }

    /*
     * Here it gets tricky - CFE_SB_ReceiveBuffer() is outputting a reference
     * to a shared struct, and that needs to be returned to the SB pool, it
     * cannot be held indefinitely here.  So this needs to make a copy to
     * pass back to the Lua environment, at least for now.
     *
     * It would be preferable to make a wrapper around the buffer, and return
     * the buffer when it was eventually garbage collected, but CFE SB currently
     * lacks the required API to do this.
     */
    if (RcvStatus == CFE_SUCCESS)
    {
        CFE_MSG_GetSize(&BufPtr->Msg, &MsgSize);
        if (EdsLib_DataTypeDB_IdentifyBuffer(&EDS_DATABASE, IntfObj->IndicationBaseArg, BufPtr, &DerivObjInfo)
            != EDSLIB_SUCCESS)
        {
            /* This is OK and may mean that the object simply isn't derived; use the original type */
            DerivObjInfo.EdsId = IntfObj->IndicationBaseArg;
        }

        ObjectUserData = SCRIPTENGINE_NewEdsObjectWithContent(lua, DerivObjInfo.EdsId, MsgSize);
        LuaObj         = EdsLib_Binding_GetNativeObject(ObjectUserData);

        if (LuaObj != NULL)
        {
            /* Populate the object with the same data */
            memcpy(LuaObj, &BufPtr->Msg, MsgSize);
            ++nret;
        }

        /*
         * Now release the reference by calling to CFE_SB_ReceiveBuffer() on the same pipe.
         * Problem is, this will also return the next message if the pipe is not empty. So
         * this needs to be done in a loop until it is empty.  But that will likely be the
         * case on the first call since nothing is supposed to be subscribed right now.
         */
        while (CFE_SB_ReceiveBuffer(&BufPtr, *PipeObj, CFE_SB_POLL) == CFE_SUCCESS)
        {
            /* do nothing, just repeat */
            (void)0;
        }
    }

    return nret;
}

static int SCRIPTENGINE_PipeIdToString(lua_State *lua)
{
    CFE_SB_PipeId_t *PipeObj = luaL_checkudata(lua, 1, "CFE_SB_PipeId");

    lua_pushfstring(lua, "CFE_SB_PipeId: %d", (int)CFE_RESOURCEID_TO_ULONG(*PipeObj));

    return 1;
}

static void SCRIPTENGINE_Setup(lua_State *lua)
{
    luaL_openlibs(lua);

    EdsLib_Lua_Attach(lua, &EDS_DATABASE);
    CFE_MissionLib_Lua_SoftwareBus_Attach(lua, &CFE_SOFTWAREBUS_INTERFACE);
    lua_setglobal(lua, "EdsDB");

    /*
     * Create a metatable for EDS objects (userdata blobs)
     * This also has the hook to call our routine when old objects are collected
     */
    if (luaL_newmetatable(lua, "CFE_SB_PipeId"))
    {
        lua_pushstring(lua, "__tostring");
        lua_pushcfunction(lua, SCRIPTENGINE_PipeIdToString);
        lua_rawset(lua, -3);
        lua_pushstring(lua, "__gc");
        lua_pushcfunction(lua, SCRIPTENGINE_DeletePipe);
        lua_rawset(lua, -3);
    }
    lua_pop(lua, 1);

    lua_newtable(lua);
    lua_pushstring(lua, "SendMsg");
    lua_pushcfunction(lua, SCRIPTENGINE_SendMsg);
    lua_settable(lua, -3);

    lua_pushstring(lua, "CreatePipe");
    lua_pushcfunction(lua, SCRIPTENGINE_CreatePipe);
    lua_settable(lua, -3);

    lua_pushstring(lua, "WaitFor");
    lua_pushcfunction(lua, SCRIPTENGINE_WaitFor);
    lua_settable(lua, -3);

    lua_setglobal(lua, "CFE");

    /* Stack index 1 will be the error handler function (used for protected calls) */
    /* This can just sit there at the bottom of the stack, useful for when pcall() is used later */
    lua_pushcfunction(lua, SCRIPTENGINE_ErrorHandler);
    assert(SCRIPTENGINE_ERRHANDLER_IDX == lua_gettop(lua));
}

static int32 SCRIPTENGINE_DoLoad(lua_State *lua, const char *Filename)
{
    int   stack_top;
    int32 status;

    stack_top = lua_gettop(lua);

    if (luaL_loadfile(lua, Filename) != LUA_OK)
    {
        CFE_ES_WriteToSysLog("SCRIPTENGINE: Cannot load Lua file: %s: %s\n", Filename, lua_tostring(lua, -1));
        status = CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    else
    {
        if (lua_pcall(lua, 0, 0, SCRIPTENGINE_ERRHANDLER_IDX) != LUA_OK)
        {
            CFE_ES_WriteToSysLog("SCRIPTENGINE: Failed to execute: %s\n", Filename);
            status = CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
        }
        else
        {
            status = CFE_SUCCESS;
        }
    }

    /* Always return the stack to where it was */
    lua_settop(lua, stack_top);

    return status;
}

static int32 SCRIPTENGINE_DoCall(lua_State *lua, const char *Function, void *ArgData, EdsLib_Id_t ArgEdsId)
{
    int                                stack_top;
    int                                nargs;
    int32                              status;
    EdsLib_Binding_DescriptorObject_t *ObjectUserData;

    nargs          = 0;
    ObjectUserData = NULL;
    stack_top      = lua_gettop(lua);

    lua_getglobal(lua, Function);

    if (ArgData != NULL)
    {
        ObjectUserData = SCRIPTENGINE_NewEdsObjectWrapper(lua, ArgEdsId, ArgData);
        ++nargs;
    }

    if (lua_pcall(lua, nargs, 0, SCRIPTENGINE_ERRHANDLER_IDX) != LUA_OK)
    {
        CFE_ES_WriteToSysLog("SCRIPTENGINE: Failed to execute Lua function: %s\n", Function);
        status = CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }
    else
    {
        status = CFE_SUCCESS;
    }

    /*
     * Detach the buffer, if it was used.  Although the local buf was not dynamic,
     * in case the caller stored the object somewhere, this will prevent future access
     * (it is owned by the caller and going out of scope, so it can't be referenced anymore)
     */
    if (ObjectUserData != NULL)
    {
        EdsLib_Binding_SetDescBuffer(ObjectUserData, NULL);
    }

    /* Always return the stack to where it was */
    lua_settop(lua, stack_top);

    return status;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Library Initialization Routine                                  */
/* cFE requires that a library have an initialization routine      */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 SCRIPTENGINE_Init(void)
{
    EdsLib_Initialize();

    /* Create a Lua state */
    SCRIPTENGINE_Global.lua = luaL_newstate();
    SCRIPTENGINE_Setup(SCRIPTENGINE_Global.lua);

    SCRIPTENGINE_LoadFile(SCRIPTENGINE_TESTFILE);

    CFE_ES_WriteToSysLog("SCRIPTENGINE: Initialized\n");

    return CFE_SUCCESS;
}

int32 SCRIPTENGINE_LoadFile(const char *Filename)
{
    return SCRIPTENGINE_DoLoad(SCRIPTENGINE_Global.lua, Filename);
}

int32 SCRIPTENGINE_CallFunctionVoid(const char *FunctionName)
{
    return SCRIPTENGINE_DoCall(SCRIPTENGINE_Global.lua, FunctionName, NULL, EDSLIB_ID_INVALID);
}

int32 SCRIPTENGINE_CallFunctionArg(const char *FunctionName, void *ArgData, uint16 AppIdx, uint16 FormatIdx)
{
    return SCRIPTENGINE_DoCall(SCRIPTENGINE_Global.lua, FunctionName, ArgData, EDSLIB_MAKE_ID(AppIdx, FormatIdx));
}