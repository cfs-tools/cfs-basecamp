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
 * \file     eds2cfetbl.c
 * \ingroup  cfecfs
 * \author   joseph.p.hickey@nasa.gov
 *
 * Build-time utility to write table files from C source files using edslib
 * Replacement for the old "elf2cfetbl" utility
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <expat.h>
#include <getopt.h>
#include <fcntl.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "scripts/eds_tbltool_filedef.h"

#include "edslib_init.h"
#include "edslib_global.h"
#include "edslib_datatypedb.h"
#include "edslib_displaydb.h"
#include "edslib_intfdb.h"
#include "edslib_binding_objects.h"

#include "cfe_tbl_eds_defines.h"
#include "cfe_tbl_filedef.h"
#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"
#include "cfe_tbl_eds_datatypes.h"
#include "cfe_fs_eds_datatypes.h"
#include "cfe_sb_eds_datatypes.h"
#include "edslib_lua_objects.h"
#include "cfe_missionlib_lua_softwarebus.h"
#include "cfe_missionlib_runtime.h"

/* an identifying marker that will be put at the beginning of every "info" (non error/warning) line */
#define EDSTABLETOOL_INFO "--> "

/* an identifying marker that will be put at the beginning of every EDSTABLETOOL_WARNING "line */
#define EDSTABLETOOL_WARNING " **WARNING** "

/* an identifying marker that will be put at the beginning of every "error" line */
#define EDSTABLETOOL_ERROR " **ERROR** "

const char LUA_APPLIST_KEY = 0;

/* This structure captures the metadata from importing an object from C */
typedef struct EdsTableTool_CObj
{
    /* this should capture the original CFE_TBL_FileDef object */
    CFE_TBL_FileDef_t FileDefInfo;

    /* all the remaining items are filled in as info is determined */
    size_t TotalObjectSize;
    size_t ElementSize;

    void *ObjPtr;

    char CTypeName[64];
    char EdsTypeName[64];

    EdsDataType_BASE_TYPES_ApiName_t RuntimeAppName;
    EdsDataType_BASE_TYPES_ApiName_t TableName;

    EdsLib_Id_t IntfDataTypeEdsId;
    EdsLib_Id_t CTypeEdsId;
    EdsLib_Id_t SelectedTypeEdsId;

} EdsTableTool_CObj_t;

typedef struct EdsTableTool_Global
{
    /* The Instance number of the current target (in cases where it is instance specific) */
    uint16_t ProcessorId;

    /* Whether to allow "passthrough" encoding for objects that do not have EDS */
    bool AllowPassthrough;

    /* temporary holding buffer for binary objects imported from C table files */
    EdsTableTool_CObj_t CObj;

} EdsTableTool_Global_t;

EdsTableTool_Global_t EdsTableTool_Global;

/* The table load command is a fixed definition, does not need a lookup */
const EdsLib_Id_t EDS_TABLE_LOAD_CMD_ID = EDSLIB_INTF_ID(EDS_INDEX(CFE_TBL), EdsCommand_CFE_TBL_Table_load_DECLARATION);

/*----------------------------------------------------------------
 *
 * Map CMD topic ID to MsgID
 * This calls the same impl that CFE FSW uses
 *
 *-----------------------------------------------------------------*/
EdsDataType_CFE_SB_MsgIdValue_t CFE_SB_CmdTopicIdToMsgId(uint16_t TopicId, uint16_t InstanceNum)
{
    const EdsComponent_CFE_SB_Listener_t Params = {
        { .InstanceNumber = InstanceNum, .TopicId = TopicId }
    };
    EdsInterface_CFE_SB_SoftwareBus_PubSub_t Output;

    CFE_MissionLib_MapListenerComponent(&Output, &Params);

    return Output.MsgId.Value;
}

/*----------------------------------------------------------------
 *
 * Map TLM topic ID to MsgID
 * This calls the same impl that CFE FSW uses
 *
 *-----------------------------------------------------------------*/
EdsDataType_CFE_SB_MsgIdValue_t CFE_SB_TlmTopicIdToMsgId(uint16_t TopicId, uint16_t InstanceNum)
{
    const EdsComponent_CFE_SB_Publisher_t Params = {
        { .InstanceNumber = InstanceNum, .TopicId = TopicId }
    };
    EdsInterface_CFE_SB_SoftwareBus_PubSub_t Output;

    CFE_MissionLib_MapPublisherComponent(&Output, &Params);

    return Output.MsgId.Value;
}

/*----------------------------------------------------------------
 *
 * Return the current processor ID
 * This reflects the CPU for which the table is getting built
 *
 *-----------------------------------------------------------------*/
uint16_t EdsTableTool_GetProcessorId(void)
{
    return EdsTableTool_Global.ProcessorId;
}

/*----------------------------------------------------------------
 *
 * Exports a C-defined object to the global instance
 *
 * This just saves the object info, including the data associated with it,
 * into the global object so it can be dealt with later.  It has to be copied
 * to the global because when this is called the object exists only on the stack
 * in the caller context.
 *
 *-----------------------------------------------------------------*/
void EdsTableTool_DoExport(const void *filedefptr,
                           const void *objptr,
                           size_t      objsize,
                           const char *typename,
                           size_t typesize)
{
    EdsTableTool_CObj_t *CObj = &EdsTableTool_Global.CObj;

    memset(CObj, 0, sizeof(*CObj));

    /* Save the original file def info */
    memcpy(&CObj->FileDefInfo, filedefptr, sizeof(CFE_TBL_FileDef_t));

    /*
     * The size specified in FILEDEF should concur with the size of the actual object.
     * If the CFE_TBL_FILEDEF macro was used, then this should always be true because the macro
     * uses sizeof() on the object just like the wrapper does.  However some older definitions
     * will create the CFE_TBL_FileDef_t object directly, and this could be different
     */
    if (CObj->FileDefInfo.ObjectSize != objsize)
    {
        fprintf(stderr,
                EDSTABLETOOL_WARNING "Size from FileDef (%lu) differs from actual object (%lu)\n",
                (unsigned long)CObj->FileDefInfo.ObjectSize,
                (unsigned long)objsize);
    }

    CObj->ObjPtr = malloc(objsize);
    if (CObj->ObjPtr == NULL)
    {
        fprintf(stderr, EDSTABLETOOL_ERROR "malloc(): %s\n", strerror(errno));
        abort();
    }

    memcpy(CObj->ObjPtr, objptr, objsize);

    strncpy(CObj->CTypeName, typename, sizeof(CObj->CTypeName) - 1);

    CObj->TotalObjectSize = objsize;
    CObj->ElementSize     = typesize;
}

/*----------------------------------------------------------------
 *
 * Preloads a "C" table definition for backward compatibilty
 * The result of this is used to pre-fill the object buffer so scripts
 * can still execute on this and change the output.  If no script is
 * executed, then this will be the final result.
 *
 * This will populate the "CObj" member of the global
 *
 *-----------------------------------------------------------------*/
void EdsTableTool_LoadCTemplateFile(const char *Filename)
{
    void       *dlhandle;
    void       *fptr;
    const char *tmpstr;
    void (*DynamicGeneratorFuncPtr)(void);
    char ModuleTempName[64];

    printf(EDSTABLETOOL_INFO "Initializing table data from: %s\n", Filename);

    dlerror();
    dlhandle = dlopen(Filename, RTLD_LAZY | RTLD_LOCAL);
    tmpstr   = dlerror();
    if (tmpstr != NULL || dlhandle == NULL)
    {
        if (tmpstr == NULL)
        {
            tmpstr = "[Unknown Error]";
        }
        fprintf(stderr, EDSTABLETOOL_ERROR "dlopen(): %s\n", tmpstr);
        exit(EXIT_FAILURE);
    }

    /* The generated wrapper always has the same function name */
    fptr   = dlsym(dlhandle, "EdsTableTool_GENERATOR");
    tmpstr = dlerror();
    if (fptr == NULL || tmpstr != NULL)
    {
        if (tmpstr == NULL)
        {
            tmpstr = "[Object pointer is NULL]";
        }
        fprintf(stderr, EDSTABLETOOL_ERROR "dlsym('%s'): %s\n", ModuleTempName, tmpstr);
        exit(EXIT_FAILURE);
    }

    *((void **)&DynamicGeneratorFuncPtr) = fptr;

    /*
     * If a dynamic generator function was defined, then it must be called now before dlclose().
     * This populates the object in memory (can still be post-processed in Lua too)
     */
    if (DynamicGeneratorFuncPtr != NULL)
    {
        DynamicGeneratorFuncPtr();

        printf(EDSTABLETOOL_INFO "Got object from C domain, shape=%lux%lu, total size=%lu\n",
               (unsigned long)EdsTableTool_Global.CObj.TotalObjectSize / EdsTableTool_Global.CObj.ElementSize,
               (unsigned long)EdsTableTool_Global.CObj.ElementSize,
               (unsigned long)EdsTableTool_Global.CObj.TotalObjectSize);
    }

    dlclose(dlhandle);
}

/*----------------------------------------------------------------
 *
 * Determine which EDS data type to use for the table encoding.
 *
 * The tool gets type info from two places, the original C declaration and by
 * looking up the table interface in the EDS Database.  Ideally they should match
 * and then there is no ambiguity.  However for a variety of historical reasons,
 * the C data type may not exactly match the table interface type, but they must
 * be at least compatible.  This attempts to determine a common point of compatibility,
 * via base type and/or containment relationships.
 *
 *-----------------------------------------------------------------*/
EdsLib_Id_t EdsTableTool_FindCommonType(EdsLib_Id_t Type1, EdsLib_Id_t Type2)
{
    EdsLib_DataTypeDB_TypeInfo_t   TypeInfo;
    EdsLib_DataTypeDB_EntityInfo_t MemberInfo;
    EdsLib_Id_t                    ResultEdsId;
    int32_t                        EdsStatus;

    /* Find some common point between the two types */
    ResultEdsId = Type1;
    while (!EdsLib_Is_Similar(ResultEdsId, Type2))
    {
        EdsStatus = EdsLib_DataTypeDB_GetTypeInfo(&EDS_DATABASE, ResultEdsId, &TypeInfo);
        if (EdsStatus != EDSLIB_SUCCESS)
        {
            ResultEdsId = EDSLIB_ID_INVALID;
            break;
        }

        /* one possibility is that the type is a base type of the other type */
        EdsStatus = EdsLib_DataTypeDB_BaseCheck(&EDS_DATABASE, ResultEdsId, Type2);
        if (EdsStatus == EDSLIB_SUCCESS)
        {
            /* In this case use the derived type */
            ResultEdsId = Type2;
            break;
        }

        /* another possibility is that the table intf type contains/wraps the C data type */
        /* Check the first member type - if its a container, this is the base type,
         * or if it is an array, this is the array data type */
        EdsStatus = EdsLib_DataTypeDB_GetMemberByIndex(&EDS_DATABASE, ResultEdsId, 0, &MemberInfo);
        if (EdsStatus != EDSLIB_SUCCESS)
        {
            ResultEdsId = EDSLIB_ID_INVALID;
            break;
        }

        /* Repeat check using the member type */
        ResultEdsId = MemberInfo.EdsId;
    }

    return ResultEdsId;
}

/*----------------------------------------------------------------
 *
 * Identifies the object obtained from the C file
 *
 * This will utilize the "CObj" member of the global, which should
 * have been populated by EdsTableTool_LoadCTemplateFile.  It looks
 * up the type(s) in the EDS DB and attempts to correlate something
 * to the objects imported from C.
 *
 *-----------------------------------------------------------------*/
void EdsTableTool_IdentifyCObj(void)
{
    EdsTableTool_CObj_t *CObj = &EdsTableTool_Global.CObj;
    const char          *tmpstr;
    char                *TempPtr;
    size_t               tmplen;
    char                 TypeNameBuff[64];
    EdsLib_Id_t          TblIntfEdsId;
    EdsLib_Id_t          AppComponentEdsId;
    uint8_t              PkgIdx;
    int32_t              EdsStatus;

    /*
     * The TableName in the FileDef object is supposed to be of the form "AppName.TableName" -
     * first split this into its constituent parts.
     *
     * Note, however, that the "AppName." part refers to the runtime app name, which often has
     * a suffix of some sort (e.g. _APP).  It should be related to the EDS app name though,
     * if naming conventions are followed.  (and if conventions are not followed, this tool
     * is not likely to work).
     */
    tmpstr = strchr(CObj->FileDefInfo.TableName, '.');
    if (tmpstr == NULL)
    {
        tmplen = 0;
        tmpstr = CObj->FileDefInfo.TableName;
    }
    else
    {
        tmplen = tmpstr - CObj->FileDefInfo.TableName;
        ++tmpstr;
    }

    if (tmplen >= sizeof(CObj->RuntimeAppName))
    {
        tmplen = sizeof(CObj->RuntimeAppName) - 1;
    }

    memcpy(CObj->RuntimeAppName, CObj->FileDefInfo.TableName, tmplen);
    CObj->RuntimeAppName[tmplen] = 0;

    strncpy(CObj->TableName, tmpstr, sizeof(CObj->TableName) - 1);
    CObj->TableName[sizeof(CObj->TableName) - 1] = 0;

    printf(EDSTABLETOOL_INFO "Identifying C Object: AppName=\'%s\', TableName=\'%s\', CType=\'%s\'\n",
           CObj->RuntimeAppName,
           CObj->TableName,
           CObj->CTypeName);

    /* Try to determine the EdsId of the C data type.  This should exist, but in some cases
     * the user has defined a custom type only for the table definition (such as a union).  This
     * may be OK as long as the custom type is compatible, but this tool cannot verify that */
    tmpstr = CObj->CTypeName;
    if (strncmp(CObj->RuntimeAppName, tmpstr, tmplen) == 0 && ispunct((unsigned char)tmpstr[tmplen]))
    {
        tmpstr += tmplen + 1;
    }
    snprintf(TypeNameBuff, sizeof(TypeNameBuff), "%s/%s", CObj->RuntimeAppName, tmpstr);

    /* The C typedef should have a "_t" suffix - trim this off for EDS type names */
    tmplen = strlen(TypeNameBuff);
    if (tmplen > 2 && strcmp(&TypeNameBuff[tmplen - 2], "_t") == 0)
    {
        TypeNameBuff[tmplen - 2] = 0;
    }

    CObj->CTypeEdsId = EdsLib_DisplayDB_LookupTypeName(&EDS_DATABASE, TypeNameBuff);
    if (EdsLib_Is_Valid(CObj->CTypeEdsId))
    {
        printf(EDSTABLETOOL_INFO "Found EDS type definition \'%s\' for C datatype \'%s\'\n",
               TypeNameBuff,
               CObj->CTypeName);
    }

    /* Now find an EDS interface definition and data type that matches */
    AppComponentEdsId = EDSLIB_ID_INVALID;
    TblIntfEdsId      = EDSLIB_ID_INVALID;

    /* Note that tables will not be loadable by TBL services unless there is an exact match.  This
     * previously made extra assumptions about the possibility of an _APP suffix and other differences,
     * but the FSW is just looking for an exact match, so using other matching schemes is only going
     * to defer failure to load time rather than build time */
    EdsStatus = EdsLib_FindPackageIdxByName(&EDS_DATABASE, CObj->RuntimeAppName, &PkgIdx);
    if (EdsStatus == EDSLIB_SUCCESS)
    {
        /* Primary Data type lookup method: Find an interface in the Application section */
        EdsStatus = EdsLib_IntfDB_FindComponentByLocalName(&EDS_DATABASE, PkgIdx, "Application", &AppComponentEdsId);
        if (EdsStatus == EDSLIB_SUCCESS)
        {
            EdsStatus = EdsLib_IntfDB_FindComponentInterfaceByLocalName(&EDS_DATABASE,
                                                                        AppComponentEdsId,
                                                                        CObj->TableName,
                                                                        &TblIntfEdsId);
        }

        /* Fallback - if no exact match, check for a numeric suffix.  Some apps (such as MD)
         * have multiple instances of the same table and they attach a numeric suffix to the name */
        if (EdsStatus != EDSLIB_SUCCESS)
        {
            TempPtr = CObj->TableName + strlen(CObj->TableName);
            while (TempPtr > CObj->TableName)
            {
                --TempPtr;
                if (!isdigit((int)(*TempPtr)))
                {
                    break;
                }

                *TempPtr = 0;
            }
            EdsStatus = EdsLib_IntfDB_FindComponentInterfaceByLocalName(&EDS_DATABASE,
                                                                        AppComponentEdsId,
                                                                        CObj->TableName,
                                                                        &TblIntfEdsId);
        }

        if (EdsStatus == EDSLIB_SUCCESS)
        {
            /* Determine the argument data type for "load" command */
            EdsStatus = EdsLib_IntfDB_FindAllArgumentTypes(&EDS_DATABASE,
                                                           EDS_TABLE_LOAD_CMD_ID,
                                                           TblIntfEdsId,
                                                           &CObj->IntfDataTypeEdsId,
                                                           1);
        }

        /*
         * If we only got one of the two types (C Datatype or Intf Datatype) then use that.
         *  (the table may not load correctly but that is a problem elsewhere)
         *
         * If we got both, and they are equal, then there is also no problem.
         * However if we get both an they are different, we have to correlate them somehow.
         * The most likely situation is that the C table is an array, and there is possibly
         * a separate container around that array.
         */
        if (!EdsLib_Is_Valid(CObj->IntfDataTypeEdsId))
        {
            /* Intf type not known - just use the C type */
            CObj->SelectedTypeEdsId = CObj->CTypeEdsId;
        }
        else if (!EdsLib_Is_Valid(CObj->CTypeEdsId))
        {
            /* C type not known - just use the intf type */
            CObj->SelectedTypeEdsId = CObj->IntfDataTypeEdsId;
        }
        else
        {
            /* see if there is some type that is common between them */
            CObj->SelectedTypeEdsId = EdsTableTool_FindCommonType(CObj->IntfDataTypeEdsId, CObj->CTypeEdsId);
            if (!EdsLib_Is_Valid(CObj->SelectedTypeEdsId))
            {
                /* try again but reversed */
                CObj->SelectedTypeEdsId = EdsTableTool_FindCommonType(CObj->CTypeEdsId, CObj->IntfDataTypeEdsId);
            }
        }
    }
    else
    {
        CObj->SelectedTypeEdsId = EDSLIB_ID_INVALID;
    }

    TypeNameBuff[0] = 0;
    EdsLib_DisplayDB_GetTypeName(&EDS_DATABASE, CObj->SelectedTypeEdsId, TypeNameBuff, sizeof(TypeNameBuff));

    /* sanity check that the type name is valid - it might have returned undefined string */
    if (EdsLib_Is_Valid(EdsLib_DisplayDB_LookupTypeName(&EDS_DATABASE, TypeNameBuff)))
    {
        printf(EDSTABLETOOL_INFO "Selected EDS type definition \'%s\' for table \'%s\'\n",
               TypeNameBuff,
               CObj->FileDefInfo.TableName);

        strncpy(CObj->EdsTypeName, TypeNameBuff, sizeof(CObj->EdsTypeName) - 1);
        CObj->EdsTypeName[sizeof(CObj->EdsTypeName) - 1] = 0;
    }
    else if (EdsTableTool_Global.AllowPassthrough)
    {
        printf(EDSTABLETOOL_INFO "Allowing passthrough encoding for table \'%s\'\n", CObj->FileDefInfo.TableName);
        CObj->EdsTypeName[0] = 0;
    }
    else
    {
        fprintf(stderr,
                EDSTABLETOOL_ERROR "Could not map table \'%s\' to an EDS-defined data type\n",
                CObj->FileDefInfo.TableName);
        exit(EXIT_FAILURE);
    }
}

/*----------------------------------------------------------------
 *
 * Exports a C-defined object to the Lua environment
 *
 * This expects a lua_State object as the abstract argument,
 * and a table should be the topmost entry in the stack.  The
 * object(s) from C will be wrapped in a Lua object and appended
 * to the table.
 *
 *-----------------------------------------------------------------*/
void EdsTableTool_AppendObjsFromCObj(lua_State *lua, EdsTableTool_CObj_t *CObj)
{
    EdsLib_Binding_DescriptorObject_t  *ObjectUserData;
    EdsLib_DataTypeDB_DerivedTypeInfo_t DerivInfo;
    int32_t                             EdsStatus;
    const uint8_t                      *sptr;
    void                               *tptr;
    size_t                              tsz;
    int                                 tbl_pos;
    size_t                              bytes_remain;
    size_t                              bytes_per_elem;

    /* There should be a table at the top of the initial stack */
    tbl_pos = lua_gettop(lua);

    /* now do a sanity check on the size */
    EdsStatus = EdsLib_DataTypeDB_GetDerivedInfo(&EDS_DATABASE, CObj->SelectedTypeEdsId, &DerivInfo);
    if (EdsStatus != EDSLIB_SUCCESS)
    {
        /* Just directly use the C object data */
        lua_pushlstring(lua, CObj->ObjPtr, CObj->TotalObjectSize);
        lua_rawseti(lua, tbl_pos, 1);

        bytes_per_elem = 0;
        bytes_remain   = 0;
        sptr           = NULL;
    }
    else
    {
        bytes_remain = CObj->TotalObjectSize;
        sptr         = CObj->ObjPtr;

        if (CObj->ElementSize < CObj->TotalObjectSize && DerivInfo.MaxSize.Bytes <= CObj->ElementSize)
        {
            /* there are multiple elements, each one gets encoded individually */
            bytes_per_elem = CObj->ElementSize;
        }
        else
        {
            /* The entire object gets encoded in one go */
            bytes_per_elem = CObj->TotalObjectSize;
        }
    }

    lua_getglobal(lua, "EdsDB");
    while (bytes_remain > 0)
    {
        /* Call "New_Object" for the object type */
        lua_getfield(lua, -1, "NewObject");
        lua_pushstring(lua, CObj->EdsTypeName);
        if (lua_pcall(lua, 1, 1, 1) != LUA_OK)
        {
            /* note - error handler already displayed error message */
            fprintf(stderr, EDSTABLETOOL_ERROR "Failed to execute: EdsDB.NewObject(%s)\n", CObj->EdsTypeName);
            exit(EXIT_FAILURE);
        }

        ObjectUserData = luaL_testudata(lua, -1, "EdsLib_Object");

        /* Fill the Lua object with the data from the C world */
        tsz = EdsLib_Binding_GetBufferMaxSize(ObjectUserData);
        EdsLib_LuaBinding_GetNativeObject(lua, -1, &tptr, NULL);

        /* this should not happen because the size was checked earlier */
        if (tsz < bytes_per_elem)
        {
            fprintf(stderr,
                    EDSTABLETOOL_ERROR "Object of %lu bytes cannot be copied into buffer of %lu bytes (datatype=%s)\n",
                    (unsigned long)bytes_per_elem,
                    (unsigned long)tsz,
                    CObj->EdsTypeName);
            exit(EXIT_FAILURE);
        }

        memcpy(tptr, sptr, bytes_per_elem);
        sptr         += bytes_per_elem;
        bytes_remain -= bytes_per_elem;

        /* Append to the table at the top of the initial stack */
        lua_rawseti(lua, tbl_pos, 1 + lua_rawlen(lua, tbl_pos));
    }

    /* Reset the stack to the same point as it was initially */
    lua_settop(lua, tbl_pos);
}

/*----------------------------------------------------------------
 *
 * Processes the object obtained from the C file
 *
 * This will utilize the "CObj" member of the global, which should
 * have been populated by EdsTableTool_LoadCTemplateFile
 *
 *-----------------------------------------------------------------*/
void EdsTableTool_CreateLuaObjFromCObj(lua_State *lua)
{
    EdsTableTool_CObj_t *CObj = &EdsTableTool_Global.CObj;
    int                  obj_idx;

    lua_getglobal(lua, CObj->RuntimeAppName);
    if (lua_type(lua, -1) != LUA_TTABLE)
    {
        lua_pop(lua, 1);
        lua_newtable(lua);
        lua_pushvalue(lua, -1);
        lua_setglobal(lua, CObj->RuntimeAppName);
    }

    lua_newtable(lua);
    obj_idx = lua_gettop(lua);

    lua_pushstring(lua, CObj->FileDefInfo.TableName);
    lua_setfield(lua, obj_idx, "TableName"); /* Set "TableName" in Table object */

    lua_pushstring(lua, CObj->FileDefInfo.Description);
    lua_setfield(lua, obj_idx, "Description"); /* Set "Description" in Table object */

    lua_pushstring(lua, CObj->FileDefInfo.TgtFilename);
    lua_setfield(lua, obj_idx, "TgtFilename"); /* Set "TgtFilename" in Table object */

    /*
     * If a C object is loaded, make EDS objects out of it
     */
    if (CObj->ObjPtr != NULL)
    {
        /* The C object could be an array of entries, so make a table to hold them */
        lua_newtable(lua);

        EdsTableTool_AppendObjsFromCObj(lua, CObj);

        printf(EDSTABLETOOL_INFO "Got %lu object(s) from C domain\n", (unsigned long)lua_rawlen(lua, -1));

        /*
         * If only one object was obtained, then just put that object directly in the parent,
         * there is no need for the table wrapper.  (this is the normal/typical case when the
         * table is fully defined in EDS and C uses an EDS type).
         */
        if (lua_rawlen(lua, -1) == 1)
        {
            lua_rawgeti(lua, -1, 1);
        }
        lua_setfield(lua, obj_idx, "Content"); /* Set "Content" in Table object */
        lua_settop(lua, obj_idx);
    }

    /* Append the Applist table to hold the loaded table template objects */
    lua_rawgetp(lua, LUA_REGISTRYINDEX, &LUA_APPLIST_KEY);
    lua_pushvalue(lua, obj_idx);
    lua_rawseti(lua, -2, 1 + lua_rawlen(lua, -2));

    lua_settop(lua, obj_idx);
    lua_setfield(lua, -2, CObj->TableName); /* Set entry in App table */
    lua_pop(lua, 1);

    printf(EDSTABLETOOL_INFO "Stored in global context as %s.%s \n", CObj->RuntimeAppName, CObj->TableName);

    /* now that the object is wrapped in a Lua binding, the original is no longer needed */
    /* this permits another file to be loaded */
    free(CObj->ObjPtr);
    memset(&EdsTableTool_Global.CObj, 0, sizeof(EdsTableTool_Global.CObj));

    lua_settop(lua, obj_idx - 2);
}

/*----------------------------------------------------------------
 *
 * Loads and executes a lua script
 *
 * The lua script can perform any arbitrary processing of the data objects
 *
 *-----------------------------------------------------------------*/
void EdsTableTool_LoadLuaFile(lua_State *lua, const char *Filename)
{
    printf(EDSTABLETOOL_INFO "Executing LUA script: %s\n", Filename);

    if (luaL_loadfile(lua, Filename) != LUA_OK)
    {
        fprintf(stderr, EDSTABLETOOL_ERROR "Cannot load Lua file: %s: %s\n", Filename, lua_tostring(lua, -1));
        exit(EXIT_FAILURE);
    }

    if (lua_pcall(lua, 0, 0, 1) != LUA_OK)
    {
        /* note - error handler already displayed error message */
        fprintf(stderr, EDSTABLETOOL_ERROR "Failed to execute: %s\n", Filename);
        exit(EXIT_FAILURE);
    }
}

/*----------------------------------------------------------------
 *
 * Encodes the singular Lua object at the top of the stack
 *
 * Input stack should have a single native object at the top (-1)
 * That object will be removed and replaced with the encoded representation
 *
 *-----------------------------------------------------------------*/
void EdsTableTool_PushEncodedSingleObject(lua_State *lua)
{
    if (luaL_testudata(lua, -1, "EdsLib_Object"))
    {
        lua_getglobal(lua, "EdsDB");
        lua_getfield(lua, -1, "Encode");
        lua_remove(lua, -2); /* remove the EdsDB global */
        lua_pushvalue(lua, -2);
        lua_call(lua, 1, 1);
    }
    else
    {
        luaL_checkstring(lua, -1);
    }
}

/*----------------------------------------------------------------
 *
 * Encodes the Lua table object at the top of the stack
 *
 * Input stack should have a Lua table containing an array/list of native objects
 * That object will be removed, each element will be individually encoded by
 * invoking EdsTableTool_PushEncodedSingleObject, then all the resulting binary
 * blobs will be concatenated together into a single blob.
 *
 * The final result will be put on the stack, replacing the original table.
 *
 *-----------------------------------------------------------------*/
void EdsTableTool_PushEncodedMultiObject(lua_State *lua)
{
    size_t num_tbl;
    size_t num_enc;
    int    obj_idx;

    obj_idx = lua_gettop(lua);

    luaL_checktype(lua, -1, LUA_TTABLE);

    num_tbl = lua_rawlen(lua, obj_idx);

    num_enc = 0;
    while (num_enc < num_tbl)
    {
        ++num_enc;
        lua_rawgeti(lua, obj_idx, num_enc); /* Get the EdsLib_Object table entry */
        EdsTableTool_PushEncodedSingleObject(lua);
        lua_remove(lua, -2); /* remove the EdsLib_Object table entry from stack */
    }

    lua_concat(lua, num_enc);
}

/*----------------------------------------------------------------
 *
 * Opens a file for output
 *
 * This is just a wrapper around fopen() that applies the OUTPUT_DIR
 *
 * The "filename_pos" should refer to a stack position containing a
 * relative file name
 *
 * If the "OUTPUT_DIR" global is set, it will create a full path using
 * the specified directory combined with the file name.  If the
 * "OUTPUT_DIR" is not set, then the full path will be based on
 * the current directory (.)
 *
 * In both cases the file is opened for writing ("w" mode) and the
 * file pointer is returned.
 *
 *-----------------------------------------------------------------*/
FILE *EdsTableTool_OpenOutputFile(lua_State *lua, int filename_pos)
{
    FILE       *OutputFile = NULL;
    int         start_top  = lua_gettop(lua);
    const char *OutputName = luaL_checkstring(lua, filename_pos);
    char        OutputFullNameBuffer[256];
    const char *OutputPath;

    if (OutputName != NULL)
    {
        lua_getglobal(lua, "OUTPUT_DIR");
        if (lua_isnoneornil(lua, -1))
        {
            OutputPath = ".";
        }
        else
        {
            OutputPath = lua_tostring(lua, -1);
        }

        snprintf(OutputFullNameBuffer, sizeof(OutputFullNameBuffer), "%s/%s", OutputPath, OutputName);
        OutputFile = fopen(OutputFullNameBuffer, "w");
    }

    lua_settop(lua, start_top);

    return OutputFile;
}

/*----------------------------------------------------------------
 *
 * Writes an encoded object to a file (generic)
 *
 * The stack should contain a single native (unencoded) object at the top
 * That object will be encoded to binary, and written to the output file.
 *
 * There is no additional headers or framing added to the object; the file
 * will contain only the binary data from the object and nothing else.
 *
 * The filename is retrieved from absolute stack position 1.
 *
 *-----------------------------------------------------------------*/
int EdsTableTool_WriteGenericFile(lua_State *lua)
{
    FILE *OutputFile;

    OutputFile = EdsTableTool_OpenOutputFile(lua, 1);
    if (OutputFile == NULL)
    {
        return luaL_error(lua, "%s: %s", lua_tostring(lua, 1), strerror(errno));
    }

    if (lua_type(lua, -1) == LUA_TTABLE)
    {
        EdsTableTool_PushEncodedMultiObject(lua);
    }
    else
    {
        EdsTableTool_PushEncodedSingleObject(lua);
    }

    fwrite(lua_tostring(lua, -1), lua_rawlen(lua, -1), 1, OutputFile);
    fclose(OutputFile);
    lua_pop(lua, 1);

    return 0;
}

/*----------------------------------------------------------------
 *
 * Writes an encoded object to a file (CFE table)
 *
 * The stack should contain a native (unencoded) object or table at the top
 * That object will be encoded to binary, and written to the output file.
 *
 * This function writes a CFE_FS header and a CFE_TBL header to the file,
 * then writes the encoded data.  The resulting file is intended to be
 * compatible with the CFE Table load.
 *
 * The filename is retrieved from absolute stack position 1.
 *
 *-----------------------------------------------------------------*/
int EdsTableTool_WriteCfeTableFile(lua_State *lua)
{
    union
    {
        EdsDataType_CFE_FS_Header_t    FileHeader;
        EdsDataType_CFE_TBL_File_Hdr_t TblHeader;
    } Buffer;
    EdsPackedBuffer_CFE_FS_Header_t    PackedFileHeader;
    EdsPackedBuffer_CFE_TBL_File_Hdr_t PackedTblHeader;
    uint32_t                           TblHeaderBlockSize;
    uint32_t                           FileHeaderBlockSize;
    EdsLib_Id_t                        PackedEdsId;
    FILE                              *OutputFile;
    const char                        *OutputName;
    EdsLib_DataTypeDB_TypeInfo_t       BlockInfo;
    const void                        *content_ptr;
    size_t                             content_sz;

    OutputFile = NULL;
    OutputName = luaL_checkstring(lua, 1);

    if (lua_type(lua, -1) == LUA_TTABLE)
    {
        EdsTableTool_PushEncodedMultiObject(lua);
    }
    else
    {
        EdsTableTool_PushEncodedSingleObject(lua);
    }

    if (lua_isnoneornil(lua, -1))
    {
        return luaL_error(lua, "%s: Failed to encode object", OutputName);
    }

    memset(&Buffer, 0, sizeof(Buffer));
    Buffer.TblHeader.NumBytes = lua_rawlen(lua, -1);
    strncpy(Buffer.TblHeader.TableName, luaL_checkstring(lua, 3), sizeof(Buffer.TblHeader.TableName));

    PackedEdsId = EDSLIB_MAKE_ID(EDS_INDEX(CFE_TBL), EdsContainer_CFE_TBL_File_Hdr_DATADICTIONARY);
    EdsLib_DataTypeDB_PackCompleteObject(&EDS_DATABASE,
                                         &PackedEdsId,
                                         PackedTblHeader,
                                         &Buffer.TblHeader,
                                         sizeof(PackedTblHeader) * 8,
                                         sizeof(Buffer.TblHeader));
    EdsLib_DataTypeDB_GetTypeInfo(&EDS_DATABASE, PackedEdsId, &BlockInfo);
    TblHeaderBlockSize = (BlockInfo.Size.Bits + 7) / 8;

    memset(&Buffer, 0, sizeof(Buffer));
    Buffer.FileHeader.ContentType = CFE_FS_FILE_CONTENT_ID;
    Buffer.FileHeader.SubType     = luaL_checkinteger(lua, lua_upvalueindex(1));
    Buffer.FileHeader.Length      = TblHeaderBlockSize + lua_rawlen(lua, -1);
    snprintf(Buffer.FileHeader.Description, sizeof(Buffer.FileHeader.Description), "%s", luaL_checkstring(lua, 2));

    PackedEdsId = EDSLIB_MAKE_ID(EDS_INDEX(CFE_FS), EdsContainer_CFE_FS_Header_DATADICTIONARY);
    EdsLib_DataTypeDB_PackCompleteObject(&EDS_DATABASE,
                                         &PackedEdsId,
                                         PackedFileHeader,
                                         &Buffer,
                                         sizeof(PackedFileHeader) * 8,
                                         sizeof(Buffer));
    EdsLib_DataTypeDB_GetTypeInfo(&EDS_DATABASE, PackedEdsId, &BlockInfo);
    FileHeaderBlockSize = (BlockInfo.Size.Bits + 7) / 8;

    OutputFile = EdsTableTool_OpenOutputFile(lua, 1);
    if (OutputFile == NULL)
    {
        return luaL_error(lua, "%s: %s", OutputName, strerror(errno));
    }

    fwrite(PackedFileHeader, FileHeaderBlockSize, 1, OutputFile);
    fwrite(PackedTblHeader, TblHeaderBlockSize, 1, OutputFile);
    content_ptr = lua_tostring(lua, -1);
    content_sz  = lua_rawlen(lua, -1);
    if (content_ptr != NULL && content_sz > 0)
    {
        fwrite(content_ptr, content_sz, 1, OutputFile);
    }
    else
    {
        fprintf(stderr, EDSTABLETOOL_WARNING "No content produced\n");
    }

    fclose(OutputFile);
    printf(EDSTABLETOOL_INFO "Wrote File: %s\n", OutputName);
    lua_pop(lua, 1);

    return 0;
}

/*----------------------------------------------------------------
 *
 * Gets the template object metadata from the C domain
 *
 * Retrieves the lua table associated with the specified table name (arg 1)
 *
 * This is the meta object that was created by EdsTableTool_CreateLuaObjFromCObj()
 * and is used when saving the file associated with this template object.
 *
 *-----------------------------------------------------------------*/
static int EdsTableTool_GetCObjectMetaData(lua_State *lua)
{
    int nret = 0;
    int tpos;
    int tidx;

    /* This registry entry is a list of objects created from .so files */
    /* See EdsTableTool_CreateLuaObjFromCObj() for how it is assembled */
    lua_rawgetp(lua, LUA_REGISTRYINDEX, &LUA_APPLIST_KEY);
    tidx = lua_gettop(lua);
    tpos = lua_rawlen(lua, tidx);
    while (tpos > 0)
    {
        lua_rawgeti(lua, tidx, tpos);
        if (lua_type(lua, -1) == LUA_TTABLE)
        {
            lua_getfield(lua, -1, "TableName");
            if (lua_compare(lua, -1, 1, LUA_OPEQ))
            {
                /* found a match, return it */
                lua_settop(lua, tidx + 1);
                nret = 1;
                break;
            }
        }

        lua_settop(lua, tidx);
        --tpos;
    }

    return nret;
}

/*----------------------------------------------------------------
 *
 * Handles errors in processing the objects from Lua
 *
 * Reports an error to the console, along with a Lua backtrace
 *
 *-----------------------------------------------------------------*/
static int EdsTableTool_ErrorHandler(lua_State *lua)
{
    lua_getglobal(lua, "debug");
    lua_getfield(lua, -1, "traceback");
    lua_remove(lua, -2);
    lua_pushnil(lua);
    lua_pushinteger(lua, 3);
    lua_call(lua, 2, 1);
    fprintf(stderr, EDSTABLETOOL_ERROR "%s\n%s\n", lua_tostring(lua, 1), lua_tostring(lua, 2));
    return 0;
}

/*----------------------------------------------------------------
 *
 * Main entry point to the tool
 *
 *-----------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    int        arg;
    size_t     slen;
    lua_State *lua;

    EdsLib_Initialize();
    memset(&EdsTableTool_Global, 0, sizeof(EdsTableTool_Global));

    /* Create a Lua state and register all of our local test functions */
    lua = luaL_newstate();
    luaL_openlibs(lua);

    /* Stack index 1 will be the error handler function (used for protected calls) */
    lua_pushcfunction(lua, EdsTableTool_ErrorHandler);

    /* Create an Applist table to hold the loaded table template objects */
    lua_newtable(lua);
    lua_rawsetp(lua, LUA_REGISTRYINDEX, &LUA_APPLIST_KEY);

    while ((arg = getopt(argc, argv, "e:p")) != -1)
    {
        switch (arg)
        {
            case 'p':
                EdsTableTool_Global.AllowPassthrough = true;
                break;

            case 'e':
                if (luaL_loadstring(lua, optarg) != LUA_OK)
                {
                    fprintf(stderr,
                            EDSTABLETOOL_ERROR "Invalid command line expression: %s\n",
                            luaL_tolstring(lua, -1, NULL));
                    lua_pop(lua, 2);
                    return EXIT_FAILURE;
                }

                if (lua_pcall(lua, 0, 0, 1) != LUA_OK)
                {
                    fprintf(stderr,
                            EDSTABLETOOL_ERROR "Cannot evaluate command line expression: %s\n",
                            luaL_tolstring(lua, -1, NULL));
                    lua_pop(lua, 2);
                    return EXIT_FAILURE;
                }
                break;

            case '?':
                if (isprint(optopt))
                {
                    fprintf(stderr, EDSTABLETOOL_ERROR "Unknown option `-%c'.\n", optopt);
                }
                else
                {
                    fprintf(stderr, EDSTABLETOOL_ERROR "Unknown option character `\\x%x'.\n", optopt);
                }
                return EXIT_FAILURE;
                break;

            default:
                return EXIT_FAILURE;
                break;
        }
    }

    /* All additional args are input files.  If none, this should be considered an error */
    if (optind >= argc)
    {
        fprintf(stderr, EDSTABLETOOL_ERROR "No input files specified on command line.\n");
        return EXIT_FAILURE;
    }

    /* The CPUNAME and CPUNUMBER variables should both be defined.
     * If only one was provided then look up the other value in EDS */
    lua_getglobal(lua, "CPUNAME");
    lua_getglobal(lua, "CPUNUMBER");
    if (lua_isstring(lua, -2) && lua_isnil(lua, -1))
    {
        uint16_t InstanceNum;

        InstanceNum = CFE_MissionLib_GetInstanceNumber(&CFE_SOFTWAREBUS_INTERFACE, lua_tostring(lua, -2));
        if (InstanceNum > 0)
        {
            lua_pushinteger(lua, InstanceNum);
            lua_setglobal(lua, "CPUNUMBER");
        }
    }
    else if (lua_isnumber(lua, -1) && lua_isnil(lua, -2))
    {
        const char *NameStr;

        NameStr = CFE_MissionLib_GetInstanceNameNonNull(&CFE_SOFTWAREBUS_INTERFACE, lua_tointeger(lua, -1));
        if (NameStr != NULL)
        {
            lua_pushstring(lua, NameStr);
            lua_setglobal(lua, "CPUNAME");
        }
    }
    lua_pop(lua, 2);

    lua_pushinteger(lua, CFE_FS_SubType_TBL_IMG);
    lua_pushcclosure(lua, EdsTableTool_WriteCfeTableFile, 1);
    lua_setglobal(lua, "Write_CFE_LoadFile");

    lua_pushcfunction(lua, EdsTableTool_WriteGenericFile);
    lua_setglobal(lua, "Write_GenericFile");

    lua_pushcfunction(lua, EdsTableTool_GetCObjectMetaData);
    lua_setglobal(lua, "Get_CObjectMetaData");

    EdsLib_Lua_Attach(lua, &EDS_DATABASE);
    CFE_MissionLib_Lua_SoftwareBus_Attach(lua, &CFE_SOFTWAREBUS_INTERFACE);
    lua_setglobal(lua, "EdsDB");

    lua_getglobal(lua, "CPUNUMBER");
    EdsTableTool_Global.ProcessorId = lua_tointeger(lua, -1);
    lua_pop(lua, 1);

    for (arg = optind; arg < argc; arg++)
    {
        /* assume the argument is a file to execute/load */
        slen = strlen(argv[arg]);
        if (slen >= 3 && strcmp(argv[arg] + slen - 3, ".so") == 0)
        {
            /* Handling a C table object (.so file) is a multi-step process */
            /* the first step is to populate the "CObj" member in the global */
            EdsTableTool_LoadCTemplateFile(argv[arg]);

            /* next step is to identify that object */
            EdsTableTool_IdentifyCObj();

            /* now create Lua object(s) from those C objects */
            /* note: future lua scripts may act on these objects */
            EdsTableTool_CreateLuaObjFromCObj(lua);
        }
        else if (slen >= 4 && strcmp(argv[arg] + slen - 4, ".lua") == 0)
        {
            EdsTableTool_LoadLuaFile(lua, argv[arg]);
        }
        else
        {
            fprintf(stderr, EDSTABLETOOL_ERROR "Unable to handle file argument: %s\n", argv[arg]);
            return EXIT_FAILURE;
        }
    }

    lua_rawgetp(lua, LUA_REGISTRYINDEX, &LUA_APPLIST_KEY);
    lua_pushnil(lua);
    while (lua_next(lua, -2))
    {
        luaL_checktype(lua, -1, LUA_TTABLE);
        lua_getglobal(lua, "Write_CFE_LoadFile");
        assert(lua_type(lua, -1) == LUA_TFUNCTION);
        lua_pushstring(lua, "TgtFilename");
        lua_gettable(lua, -3);
        lua_pushstring(lua, "Description");
        lua_gettable(lua, -4);
        lua_pushstring(lua, "TableName");
        lua_gettable(lua, -5);
        lua_pushstring(lua, "Content");
        lua_gettable(lua, -6);
        assert(lua_type(lua, -5) == LUA_TFUNCTION);
        if (lua_pcall(lua, 4, 0, 1) != LUA_OK)
        {
            /* note - more descriptive error message already printed by error handler function */
            fprintf(stderr, EDSTABLETOOL_ERROR "Failed to write CFE table file from template\n");
            return (EXIT_FAILURE);
        }

        lua_pop(lua, 1);
    }
    lua_pop(lua, 1); /* Applist Global */

    return EXIT_SUCCESS;
}
