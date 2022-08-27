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
 * \file     edslib_full_test.c
 * \ingroup  edslib
 * \author   joseph.p.hickey@nasa.gov
 *
 * Unit testing of the "full" (name-based) EDS API
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utassert.h"

#include <stdint.h>
#include <cfe_mission_cfg.h>
#include <UTM_eds_index.h>

#include "edslib_displaydb.h"
#include "edslib_id.h"
#include "UTHDR_msgdefs.h"
#include "UT1_msgdefs.h"

extern EdsLib_DataDictionary_t *UTM_EDS_DATADICT_APPTBL[UTM_EDS_MAX_INDEX];
extern EdsLib_NameDictionary_t *UTM_EDS_NAMEDICT_APPTBL[UTM_EDS_MAX_INDEX];
extern EdsLib_SymbolDictionary_t UTM_EDS_SYMBOL_TABLE;
extern EdsLib_DatabaseObject_t GD_BASIC;

union
{
    UT1_Cmd_t As_Cmd;
    UT1_Tlm1_t As_Tlm1;
    uint8_t As_Bytes[1024];
} Buffer;

EdsLib_DatabaseObject_t GD_FULL =
{
        .AppTableSize = UTM_EDS_MAX_INDEX,
        .AppDataTable = UTM_EDS_DATADICT_APPTBL,
        .AppNameTable = UTM_EDS_NAMEDICT_APPTBL,
        .SymbolTable = &UTM_EDS_SYMBOL_TABLE
};

const char *FullFormats_EXPECTED[] =
{
        "Formats:int8:T=1/O=0/H=0",
        "Formats:uint8:T=2/O=0/H=0",
        "Formats:int16:T=1/O=0/H=0",
        "Formats:uint16:T=2/O=0/H=0",
        "Formats:int32:T=1/O=0/H=0",
        "Formats:uint32:T=2/O=0/H=0",
        "Formats:int64:T=1/O=0/H=0",
        "Formats:uint64:T=2/O=0/H=0",
        "Formats:memaddr:T=2/O=0/H=0",
        "Formats:float:T=3/O=0/H=0",
        "Formats:double:T=3/O=0/H=0",
        "Formats:UTHDR/BasePacket:T=7/O=0/H=2",
        "Formats:UTHDR/UtCmd:T=7/O=0/H=2",
        "Formats:UTHDR/UtTlm:T=7/O=0/H=2",
        "Formats:UTHDR/UtPriHdr:T=5/O=0/H=2",
        "Formats:UTHDR/UtCmdSecHdr:T=5/O=0/H=2",
        "Formats:UTHDR/UtTlmSecHdr:T=5/O=0/H=2",
        "Formats:UT1/TestCMD:T=7/O=0/H=2",
        "Formats:UT1/TestTLM:T=7/O=0/H=2",
        "Formats:UT1/Cmd1:T=7/O=0/H=2",
        "Formats:UT1/Cmd2:T=7/O=0/H=2",
        "Formats:UT1/CMDVAL:T=2/O=0/H=4",
        "Formats:UT1/char@10:T=4/O=0/H=3",
        "Formats:UT1/uint16@4:T=6/O=0/H=0",
        "Formats:UT1/uint8@250:T=6/O=0/H=0",
        "Formats:UT1/bitmask@20:T=6/O=0/H=0",
        "Formats:UT1/bitmask@40:T=6/O=0/H=0",
        "Formats:UT1/bitmask@10:T=6/O=0/H=0",
        "Formats:UT1/BasicDataTypes:T=5/O=0/H=2",
        "Formats:UT1/BasicDataTypes@5:T=6/O=0/H=0",
        "Formats:UT1/Cmd:T=5/O=0/H=2",
        "Formats:UT1/Tlm1:T=5/O=0/H=2",
        "Formats:UT1/Table1:T=5/O=0/H=2",
        NULL
};

const char *Symbols_EXPECTED[] =
{
        "Symbols:BASE_NUMBER=14",
        "Symbols:BASE_TYPE=0",
        "Symbols:NUMBER_OF_WIDGETS=14",
        "Symbols:SIGNED_INTEGER_ENCODING=0",
        "Symbols:UTHDR_UT_CMD_MID_PER_CPU=30",
        "Symbols:UTHDR_UT_PORTABLE_CMD_CPU_MSG_BASE=30",
        "Symbols:UTHDR_UT_PORTABLE_TLM_CPU_MSG_BASE=60",
        "Symbols:UTHDR_UT_TLM_MID_PER_CPU=30",
        NULL
};

const char *TlmPayloadShallow_EXPECTED[] =
{
        "TlmPayloadShallow:{NONAME}:T=7/O=0/H=2",
        "TlmPayloadShallow:Payload:T=5/O=8/H=2",
        NULL
};

const char *TlmPayloadDeep_EXPECTED[] =
{
        "TlmPayloadDeep:UtLength:T=2/O=0/H=0",
        "TlmPayloadDeep:UtStreamId:T=2/O=2/H=0",
        "TlmPayloadDeep:SecHdr.UtCommandId:T=2/O=4/H=0",
        "TlmPayloadDeep:SecHdr.CmdExtraInfo:T=2/O=6/H=0",
        "TlmPayloadDeep:Payload.Commands[0]:T=2/O=8/H=0",
        "TlmPayloadDeep:Payload.Commands[1]:T=2/O=9/H=0",
        "TlmPayloadDeep:Payload.Commands[2]:T=2/O=10/H=0",
        "TlmPayloadDeep:Payload.Commands[3]:T=2/O=11/H=0",
        NULL
};

const char *CmdPayloadShallow_EXPECTED[] =
{
        "CmdPayloadShallow:{NONAME}:T=7/O=0/H=2",
        "CmdPayloadShallow:Payload:T=5/O=8/H=2",
        NULL
};

const char *CmdPayloadDeep_EXPECTED[] =
{
        "CmdPayloadDeep:UtLength:T=2/O=0/H=0",
        "CmdPayloadDeep:UtStreamId:T=2/O=2/H=0",
        "CmdPayloadDeep:SecHdr.UtCommandId:T=2/O=4/H=0",
        "CmdPayloadDeep:SecHdr.CmdExtraInfo:T=2/O=6/H=0",
        "CmdPayloadDeep:Payload.Commands[0]:T=2/O=8/H=0",
        "CmdPayloadDeep:Payload.Commands[1]:T=2/O=9/H=0",
        "CmdPayloadDeep:Payload.Commands[2]:T=2/O=10/H=0",
        "CmdPayloadDeep:Payload.Commands[3]:T=2/O=11/H=0",
        NULL
};

const char *BasicDataTypes_EXPECTED[] =
{
        "BasicDataTypes:cmdval:T=2/O=0/H=4",
        "BasicDataTypes:b1:T=1/O=1/H=0",
        "BasicDataTypes:bl1:T=2/O=2/H=0",
        "BasicDataTypes:w1:T=2/O=4/H=0",
        "BasicDataTypes:w2:T=1/O=6/H=0",
        "BasicDataTypes:dw1:T=2/O=8/H=0",
        "BasicDataTypes:dw2:T=1/O=12/H=0",
        "BasicDataTypes:f1:T=3/O=16/H=0",
        "BasicDataTypes:df1:T=3/O=24/H=0",
        "BasicDataTypes:str:T=4/O=32/H=3",
};

typedef struct
{
    const char *TestName;
    const char **CurrentLine;
} WalkTestState_t;

static void FullTestCallback(void *Arg, const EdsLib_EntityDescriptor_t *Param)
{
    char OutputBuffer[128];
    const char *TempPtr;
    WalkTestState_t *TestState = Arg;

    if (Param->FullName == NULL)
    {
        TempPtr = "{NONAME}";
    }
    else
    {
        TempPtr = Param->FullName;
    }

    snprintf(OutputBuffer, sizeof(OutputBuffer), "%s:%s:T=%u/O=%u/H=%u",
            TestState->TestName,
            TempPtr,
            (unsigned int)Param->Basic.TypeInfo.ElemType,
            (unsigned int)Param->Basic.ComponentInfo.AbsOffset,
            (unsigned int)Param->Disp.DisplayHint);

    TempPtr = *TestState->CurrentLine;
    if (TempPtr == NULL)
    {
        TempPtr = "{END}";
    }
    else
    {
        ++TestState->CurrentLine;
    }

    UtAssert_True(strcmp(TempPtr, OutputBuffer) == 0, "%s", OutputBuffer);
}

static EdsLib_Iterator_Rc_t FormatCallback(EdsLib_Iterator_CbType_t CbType, void *Arg, EdsLib_Basic_MemberInfo_t *WalkState)
{
    EdsLib_EntityDescriptor_t Desc;

    Desc.Basic = *WalkState;
    EdsLib_Get_DisplayHint(&GD_FULL, WalkState->ComponentInfo.StructureId, &Desc.Disp);
    Desc.FullName = EdsLib_Get_DescriptiveName(&GD_FULL, WalkState->ComponentInfo.StructureId);
    FullTestCallback(Arg, &Desc);

    return EDSLIB_ITERATOR_RC_CONTINUE;
}

static void TestSymbolCallback(void *Arg, const char *SymbolName, int32_t SymbolValue)
{
    WalkTestState_t *TestState = Arg;
    const char *TempPtr;
    char OutputBuffer[128];

    snprintf(OutputBuffer, sizeof(OutputBuffer), "%s:%s=%ld",
            TestState->TestName,
            SymbolName,
            (long)SymbolValue);

    TempPtr = *TestState->CurrentLine;
    if (TempPtr == NULL)
    {
        TempPtr = "{END}";
    }
    else
    {
        ++TestState->CurrentLine;
    }

    UtAssert_True(strcmp(TempPtr, OutputBuffer) == 0, "%s", OutputBuffer);
}

void EdsLib_Full_Test(void)
{
    EdsLib_Id_t MsgId;
    char OutputBuffer[128];
    EdsLib_EntityDescriptor_t LocateDesc;
    uint16_t i;
    const char *StringResult;
    int32_t TestResult;
    WalkTestState_t WalkTestState;

    WalkTestState.CurrentLine = FullFormats_EXPECTED;
    WalkTestState.TestName = "Formats";
    EdsLib_Iterator_Formats(&GD_BASIC, FormatCallback, &WalkTestState);

    WalkTestState.CurrentLine = Symbols_EXPECTED;
    WalkTestState.TestName = "Symbols";
    EdsLib_Iterator_Symbol_Namespace(&GD_FULL, NULL, TestSymbolCallback, &WalkTestState);

    for (i=0; i < sizeof(Buffer); ++i)
    {
        Buffer.As_Bytes[i] = 0xAA ^ (i & 0xFF);
    }
    //EdsLib_Generate_Hexdump(Logfile, Buffer.As_Bytes, 16, sizeof(UT1_Tlm1_Payload_t));
    //EdsLib_Generate_Hexdump(Logfile, Buffer.As_Bytes, 4, sizeof(UT1_Cmd_Payload_t));

    MsgId = EdsLib_DisplayDB_LookupTypeName(&GD_FULL, "2:UTHDR/UtTlm");
    i = EdsLib_Get_CpuNumber(MsgId);
    UtAssert_True(i == 2, "EdsLib_Get_CpuNumber(%08x) (%u) == 2",
            (unsigned int)MsgId, (unsigned int)i);
    i = EdsLib_Get_AppIdx(MsgId);
    UtAssert_True(i == UTM_EDS_UTHDR_INDEX, "EdsLib_Get_AppIdx(%08x) (%u) == %u",
            (unsigned int)MsgId, (unsigned int)i, (unsigned int)UTM_EDS_UTHDR_INDEX);
    i = EdsLib_Get_FormatIdx(MsgId);
    UtAssert_True(i == UTHDR_UtTlm_Intf_t_DATADICTIONARY, "EdsLib_Get_FormatIndex(%08x) (%u) == %u",
            (unsigned int)MsgId, (unsigned int)i, (unsigned int)UTHDR_UtTlm_Intf_t_DATADICTIONARY);

    MsgId = EdsLib_DisplayDB_LookupTypeName(&GD_FULL, "4:UT1/Cmd1");
    i = EdsLib_Get_CpuNumber(MsgId);
    UtAssert_True(i == 4, "EdsLib_Get_CpuNumber(%08x) (%u) == 4",
            (unsigned int)MsgId, (unsigned int)i);
    i = EdsLib_Get_AppIdx(MsgId);
    UtAssert_True(i == UTM_EDS_UT1_INDEX, "EdsLib_Get_AppIdx(%08x) (%u) == %u",
            (unsigned int)MsgId, (unsigned int)i, (unsigned int)UTM_EDS_UT1_INDEX);
    i = EdsLib_Get_FormatIdx(MsgId);
    UtAssert_True(i == UT1_Cmd1_Intf_t_DATADICTIONARY, "EdsLib_Get_FormatIndex(%08x) (%u) == %u",
            (unsigned int)MsgId, (unsigned int)i, (unsigned int)UT1_Cmd1_Intf_t_DATADICTIONARY);

    WalkTestState.CurrentLine = CmdPayloadShallow_EXPECTED;
    WalkTestState.TestName = "CmdPayloadShallow";
    EdsLib_DisplayDB_IterateMembers(&GD_FULL, MsgId, FullTestCallback, &WalkTestState);
    WalkTestState.CurrentLine = CmdPayloadDeep_EXPECTED;
    WalkTestState.TestName = "CmdPayloadDeep";
    EdsLib_Iterator_Payload_FullNames(&GD_FULL, MsgId, FullTestCallback, &WalkTestState);

    /* FIXME: coverage */
    //EdsLib_MsgId_To_StreamId(&GD_FULL, MsgId);

    if (EdsLib_Endian_NeedsSwap(EDSLIB_BYTE_ORDER_BIG_ENDIAN, EDSLIB_BYTE_ORDER_NATIVE))
    {
        EdsLib_SwapInPlace(&GD_FULL, MsgId, Buffer.As_Bytes, 0);
    }

    /* For sanity we have to replace the floating points with reasonable values */
    Buffer.As_Tlm1.BasicDataTypes1.f1 = 0.1;
    Buffer.As_Tlm1.BasicDataTypes1.df1 = 0.3;
    for (i=0; i < 5; ++i)
    {
        Buffer.As_Tlm1.BasicDataTypes2[i].f1 = i + 1.1;
        Buffer.As_Tlm1.BasicDataTypes2[i].df1 = i + 1.3;
    }

    StringResult = EdsLib_DisplayDB_GetEdsName(&GD_BASIC, MsgId);
    UtAssert_True(strcmp(StringResult,"UNDEFINED") == 0, "EdsLib_DisplayDB_GetEdsName(GD_BASIC, %x) (%s) == UNDEFINED",
            (unsigned int)MsgId, StringResult);
    StringResult = EdsLib_Get_DescriptiveName(&GD_BASIC, MsgId);
    UtAssert_True(strcmp(StringResult,"UNDEFINED") == 0,"EdsLib_Get_DescriptiveName(GD_BASIC, %x) (%s) == UNDEFINED",
            (unsigned int)MsgId, StringResult);
    StringResult = EdsLib_DisplayDB_GetEdsName(&GD_FULL, MsgId);
    UtAssert_True(strcmp(StringResult,"UT1") == 0,"EdsLib_DisplayDB_GetEdsName(GD_FULL, %x) (%s) == UT1",
            (unsigned int)MsgId, StringResult);
    StringResult = EdsLib_Get_DescriptiveName(&GD_FULL, MsgId);
    UtAssert_True(strcmp(StringResult,"UT1/Cmd1") == 0,"EdsLib_Get_DescriptiveName(GD_FULL, %x) (%s) == UT1/Cmd1",
            (unsigned int)MsgId, StringResult);

    WalkTestState.CurrentLine = TlmPayloadShallow_EXPECTED;
    WalkTestState.TestName = "TlmPayloadShallow";
    EdsLib_DisplayDB_IterateMembers(&GD_FULL, MsgId, FullTestCallback, &WalkTestState);
    WalkTestState.CurrentLine = TlmPayloadDeep_EXPECTED;
    WalkTestState.TestName = "TlmPayloadDeep";
    EdsLib_Iterator_Payload_FullNames(&GD_FULL, MsgId, FullTestCallback, &WalkTestState);

    WalkTestState.CurrentLine = BasicDataTypes_EXPECTED;
    WalkTestState.TestName = "BasicDataTypes";
    MsgId = EDSLIB_MAKE_ID(UTM_EDS_UT1_INDEX, UT1_BasicDataTypes_t_DATADICTIONARY);
    EdsLib_Iterator_Payload_FullNames(&GD_FULL, MsgId, FullTestCallback, &WalkTestState);

    memset(&LocateDesc, 0, sizeof(LocateDesc));
    LocateDesc.FullName = "BasicDataTypes2[3].df1";
    MsgId = EDSLIB_MAKE_ID(UTM_EDS_UT1_INDEX, UT1_Tlm1_t_DATADICTIONARY);

    TestResult = EdsLib_DisplayDB_LocateSubEntity(&GD_FULL, MsgId, LocateDesc.FullName, &LocateDesc.Basic.ComponentInfo);
    UtAssert_True(TestResult == 0, "EdsLib_Locate_Parameter(GD_FULL, %x) (%d) == 0",
            (unsigned int)MsgId, (int)TestResult);
    UtAssert_True(LocateDesc.Basic.ComponentInfo.AbsOffset  == 232, "LocateDesc.Basic.AbsOffset (%u) == 232",
            (unsigned int)LocateDesc.Basic.ComponentInfo.AbsOffset);
    TestResult = EdsLib_DataTypeDB_GetTypeInfo(&GD_FULL, LocateDesc.Basic.ComponentInfo.StructureId, &LocateDesc.Basic.TypeInfo);
    UtAssert_True(TestResult == 0, "EdsLib_DataTypeDB_GetTypeInfo(GD_FULL, %x) (%d) == 0",
            (unsigned int)LocateDesc.Basic.ComponentInfo.StructureId, (int)TestResult);
    UtAssert_True(LocateDesc.Basic.TypeInfo.Size.Bytes  == 8, "LocateDesc.Basic.TypeInfo.ByteSize (%u) == 8",
            (unsigned int)LocateDesc.Basic.TypeInfo.Size.Bytes);
    UtAssert_True(LocateDesc.Basic.TypeInfo.ElemType  == EDSLIB_BASICTYPE_FLOAT, "LocateDesc.Basic.TypeInfo.ElemType (%u) == EDSLIB_ELEMTYPE_IEEE_FLOAT",
            (unsigned int)LocateDesc.Basic.TypeInfo.ElemType);

    TestResult = EdsLib_DisplayDB_GetIndexByName(&GD_FULL, MsgId, LocateDesc.FullName, &i);
    UtAssert_True(TestResult == 0, "EdsLib_Lookup_SubMemberByName(GD_FULL, %x) (%d) == 0",
            (unsigned int)MsgId, (int)TestResult);
    UtAssert_True(i == 4, "SubIndex (%u) == 4", (unsigned int)i);

    memset(&LocateDesc, 0, sizeof(LocateDesc));

    TestResult = EdsLib_DisplayDB_LocateSubEntity(&GD_FULL, MsgId, "BasicDataTypes2{badsyntax}", &LocateDesc.Basic.ComponentInfo);
    UtAssert_True(TestResult != 0, "EdsLib_Locate_Parameter(GD_FULL, %x) (%d) != 0",
            (unsigned int)MsgId, (int)TestResult);
    TestResult = EdsLib_DisplayDB_LocateSubEntity(&GD_FULL, MsgId, "BasicDataTypes2[4].nonexisting", &LocateDesc.Basic.ComponentInfo);
    UtAssert_True(TestResult != 0, "EdsLib_Locate_Parameter(GD_FULL, %x) (%d) != 0",
            (unsigned int)MsgId, (int)TestResult);
    TestResult = EdsLib_DisplayDB_GetIndexByName(&GD_FULL, MsgId, "nonexisting", &i);
    UtAssert_True(TestResult != 0, "EdsLib_Lookup_SubMemberByName(GD_FULL, %x) (%d) != 0",
            (unsigned int)MsgId, (int)TestResult);
    TestResult = EdsLib_DisplayDB_GetIndexByName(&GD_FULL, MsgId, "bitmask16", &i);
    UtAssert_True(TestResult == 0, "EdsLib_Lookup_SubMemberByName(GD_FULL, %x) (%d) == 0",
            (unsigned int)MsgId, (int)TestResult);
    UtAssert_True(i  == 2, "SubIndex (%u) == 2", (unsigned int)i);

    EdsLib_Set_FormatIdx(&MsgId, 100);
    StringResult = EdsLib_Get_DescriptiveName(&GD_FULL, MsgId);
    UtAssert_True(strcmp(StringResult,"UNDEFINED")  == 0, "EdsLib_Get_DescriptiveName(GD_FULL, %x) (%s) == UNDEFINED",
            (unsigned int)MsgId, StringResult);
    MsgId = EDSLIB_MAKE_ID(10,0);
    StringResult = EdsLib_DisplayDB_GetEdsName(&GD_FULL, MsgId);
    UtAssert_True(strcmp(StringResult,"UNDEFINED")  == 0, "EdsLib_DisplayDB_GetEdsName(GD_FULL, %x) (%s) == UNDEFINED",
            (unsigned int)MsgId, StringResult);
    MsgId = EDSLIB_MAKE_ID(UTM_EDS_UTHDR_INDEX, 100);
    StringResult = EdsLib_Get_DescriptiveName(&GD_FULL, MsgId);
    UtAssert_True(strcmp(StringResult,"UNDEFINED")  == 0, "EdsLib_Get_DescriptiveName(GD_FULL, %x) (%s) == UNDEFINED",
            (unsigned int)MsgId, StringResult);

    MsgId = EdsLib_DisplayDB_LookupTypeName(&GD_FULL, "INVALID");
    UtAssert_True(MsgId == EDSLIB_ID_INVALID, "MsgId (%x) == EDSLIB_ID_INVALID", (unsigned int)MsgId);
    MsgId = EdsLib_DisplayDB_LookupTypeName(&GD_FULL, "UT1:INVALID");
    UtAssert_True(MsgId == EDSLIB_ID_INVALID, "MsgId (%x) == EDSLIB_ID_INVALID", (unsigned int)MsgId);
    MsgId = EdsLib_DisplayDB_LookupTypeName(&GD_FULL, "UT1/BasicDataTypes");
    i = EdsLib_Get_CpuNumber(MsgId);
    UtAssert_True(i == 0, "EdsLib_Get_CpuNumber(%x) (%u) == 0", (unsigned int)MsgId, (unsigned int)i);
    i = EdsLib_Get_AppIdx(MsgId);
    UtAssert_True(i == UTM_EDS_UT1_INDEX, "EdsLib_Get_AppIdx(%x) (%u) == UTM_EDS_UT1_INDEX", (unsigned int)MsgId, (unsigned int)i);
    i = EdsLib_Get_FormatIdx(MsgId);
    UtAssert_True(i == UT1_BasicDataTypes_t_DATADICTIONARY, "EdsLib_Get_FormatIdx(%x) (%u) == UT1_BasicDataTypes_t_DATADICTIONARY", (unsigned int)MsgId, (unsigned int)i);
    StringResult = EdsLib_Get_DescriptiveName(&GD_FULL, MsgId);
    UtAssert_True(strcmp(StringResult,"UT1/BasicDataTypes") == 0, "EdsLib_Get_DescriptiveName(GD_FULL, %x) (%s) == UT1/BasicDataTypes",
            (unsigned int)MsgId, StringResult);

    MsgId = EdsLib_DisplayDB_LookupTypeName(&GD_FULL, "INVALID");
    UtAssert_True(MsgId == EDSLIB_ID_INVALID, "MsgId (%x) == EDSLIB_ID_INVALID", (unsigned int)MsgId);
    MsgId = EdsLib_DisplayDB_LookupTypeName(&GD_FULL, "1:UT1/Cmd1");
    i = EdsLib_Get_CpuNumber(MsgId);
    UtAssert_True(i == 1, "EdsLib_Get_CpuNumber(%x) (%u) == 1", (unsigned int)MsgId, (unsigned int)i);
    i = EdsLib_Get_AppIdx(MsgId);
    UtAssert_True(i == UTM_EDS_UT1_INDEX, "EdsLib_Get_AppIdx(%x) (%u) == UTM_EDS_UT1_INDEX", (unsigned int)MsgId, (unsigned int)i);
    i = EdsLib_Get_FormatIdx(MsgId);
    UtAssert_True(i == UT1_Cmd1_Intf_t_DATADICTIONARY, "EdsLib_Get_FormatIdx(%x) (%u) == UT1_Cmd1_Intf_t_DATADICTIONARY",
            (unsigned int)MsgId, (unsigned int)i);
    MsgId = EdsLib_DisplayDB_LookupTypeName(&GD_FULL, "2:UT1/Tlm1");
    i = EdsLib_Get_CpuNumber(MsgId);
    UtAssert_True(i == 2, "EdsLib_Get_CpuNumber(%x) (%u) == 2", (unsigned int)MsgId, (unsigned int)i);
    i = EdsLib_Get_AppIdx(MsgId);
    UtAssert_True(i == UTM_EDS_UT1_INDEX, "EdsLib_Get_AppIdx(%x) (%u) == UTM_EDS_UT1_INDEX", (unsigned int)MsgId, (unsigned int)i);
    i = EdsLib_Get_FormatIdx(MsgId);
    UtAssert_True(i == UT1_Tlm1_t_DATADICTIONARY, "EdsLib_Get_FormatIdx(%x) (%u) == UT1_Tlm1_t_DATADICTIONARY", (unsigned int)MsgId, (unsigned int)i);
    MsgId = EdsLib_DisplayDB_LookupTypeName(&GD_FULL, "3:UT1/TestCMD");
    i = EdsLib_Get_CpuNumber(MsgId);
    UtAssert_True(i == 3, "EdsLib_Get_CpuNumber(%x) (%u) == 3", (unsigned int)MsgId, (unsigned int)i);
    i = EdsLib_Get_AppIdx(MsgId);
    UtAssert_True(i == UTM_EDS_UT1_INDEX, "EdsLib_Get_AppIdx(%x) (%u) == UTM_EDS_UT1_INDEX", (unsigned int)MsgId, (unsigned int)i);
    i = EdsLib_Get_FormatIdx(MsgId);
    UtAssert_True(i == UT1_TestCMD_Intf_t_DATADICTIONARY, "EdsLib_Get_FormatIdx(%x) (%u) == UT1_TestCMD_Intf_t_DATADICTIONARY",
            (unsigned int)MsgId, (unsigned int)i);
    EdsLib_GlobalMessageId_ToString(&GD_FULL, OutputBuffer, sizeof(OutputBuffer), MsgId);
    UtAssert_True(strcmp(OutputBuffer, "3:UT1/TestCMD") == 0, "EdsLib_GlobalMessageId_ToString(..., %x) (%s) == 3:UT1/TestCMD",
            (unsigned int)MsgId, OutputBuffer);
    //EdsLib_MsgId_To_StreamId(&GD_FULL, MsgId); /* FIXME: coverage */
}

void EdsLib_StringConv_Test(void)
{
    EdsLib_NumberBuffer_t ValBuf;
    EdsLib_DisplayHint_t Conv;
    int32_t TestResult;
    char TestBuffer[128];

    /* Perform some to/from string conversions - must hit every type */
    /* A conv that has been "zeroed out" should be safe */
    memset(&Conv, 0, sizeof(Conv));
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 1, "INVALID", EDSLIB_BASICTYPE_NONE, NULL);
    UtAssert_True(TestResult != 0, "EdsLib_Scalar_FromString(..., INVALID) (%d) != 0", (int)TestResult);

    /* Test conversion of strings to signed ints of various sizes */
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 1, "-10", EDSLIB_BASICTYPE_SIGNED_INT, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_FromString(i8) (%d) == 0", (int)TestResult);
    UtAssert_True(ValBuf.i8  == -10, "ValBuf.i8 (%d) == -10", (int)ValBuf.i8);
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 2, "-15743", EDSLIB_BASICTYPE_SIGNED_INT, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_FromString(i16) (%d) == 0", (int)TestResult);
    UtAssert_True(ValBuf.i16  == -15743, "ValBuf.i16 (%d) == -15743", (int)ValBuf.i16);
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 4, "-1045203485", EDSLIB_BASICTYPE_SIGNED_INT, NULL);
    UtAssert_True(TestResult  == 0, "EdsLib_Scalar_FromString(i32) (%d) == 0", (int)TestResult);
    UtAssert_True(ValBuf.i32  == -1045203485, "ValBuf.i32 (%d) == -1045203485", (int)ValBuf.i32);
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 8, "-5968483949505", EDSLIB_BASICTYPE_SIGNED_INT, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_FromString(i64) (%d) == 0", (int)TestResult);
    /* NOTE: On 32 bit machines the assertion here should still work but the message may be wrong */
    UtAssert_True(ValBuf.i64  == -5968483949505, "ValBuf.i64 (%ld) == -5968483949505", (long)ValBuf.i64);
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 0, "xxx", EDSLIB_BASICTYPE_SIGNED_INT, NULL);
    UtAssert_True(TestResult != 0, "EdsLib_Scalar_FromString(non-integer) (%d) != 0", (int)TestResult);
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 10, "123", EDSLIB_BASICTYPE_SIGNED_INT, NULL);
    UtAssert_True(TestResult != 0, "EdsLib_Scalar_FromString(bad size) (%d) != 0", (int)TestResult);

    /* Test conversion of signed ints of various sizes to strings */
    ValBuf.i8 = -11;
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 1, EDSLIB_BASICTYPE_SIGNED_INT, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_ToString(i8) (%d) == 0", (int)TestResult);
    UtAssert_True(strcmp(TestBuffer, "-11") == 0, "OutputBuffer(%s) == -11", TestBuffer);
    ValBuf.i16 = -15744;
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 2, EDSLIB_BASICTYPE_SIGNED_INT, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_FromString(i16) (%d) == 0", (int)TestResult);
    UtAssert_True(strcmp(TestBuffer, "-15744") == 0, "OutputBuffer(%s) == -15744", TestBuffer);
    ValBuf.i32 = -1045203486;
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 4, EDSLIB_BASICTYPE_SIGNED_INT, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_ToString(i32) (%d) == 0", (int)TestResult);
    UtAssert_True(strcmp(TestBuffer, "-1045203486") == 0, "OutputBuffer(%s) == -1045203486", TestBuffer);
    ValBuf.i64 = -5968483949506;
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 8, EDSLIB_BASICTYPE_SIGNED_INT, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_ToString(i64) (%d) == 0", (int)TestResult);
    UtAssert_True(strcmp(TestBuffer, "-5968483949506") == 0, "OutputBuffer(%s) == -5968483949506", TestBuffer);
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 13, EDSLIB_BASICTYPE_SIGNED_INT, NULL);
    UtAssert_True(TestResult != 0, "EdsLib_Scalar_ToString(bad_size) (%d) != 0", (int)TestResult);

    /* Test conversion of strings to unsigned ints of various sizes */
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 1, "250", EDSLIB_BASICTYPE_UNSIGNED_INT, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_FromString(u8) (%d) == 0", (int)TestResult);
    UtAssert_True(ValBuf.u8  == 250, "ValBuf.u8 (%u) == 250", (unsigned int)ValBuf.u8 );
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 2, "40321", EDSLIB_BASICTYPE_UNSIGNED_INT, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_FromString(u16) (%d) == 0", (int)TestResult);
    UtAssert_True(ValBuf.u16  == 40321, "ValBuf.u16 (%u) == 40321", (unsigned int)ValBuf.u16);
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 4, "3194304358", EDSLIB_BASICTYPE_UNSIGNED_INT, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_FromString(u32) (%d) == 0)",(int)TestResult);
    UtAssert_True(ValBuf.u32  == 3194304358, "ValBuf.u32 (%lu) == 3194304358", (unsigned long)ValBuf.u32);
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 8, "59486574499385", EDSLIB_BASICTYPE_UNSIGNED_INT, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_FromString(u64) (%d) == 0",(int)TestResult);
    /* NOTE: On 32 bit machines the assertion here should still work but the message may be wrong */
    UtAssert_True(ValBuf.u64  == 59486574499385, "ValBuf.u64 (%lu) == 59486574499385", (unsigned long)ValBuf.u64);
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 0, "xxx", EDSLIB_BASICTYPE_UNSIGNED_INT, NULL);
    UtAssert_True(TestResult != 0, "EdsLib_Scalar_FromString(non-integer) (%d) != 0", (int)TestResult);
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 3, "123", EDSLIB_BASICTYPE_UNSIGNED_INT, NULL);
    UtAssert_True(TestResult != 0, "EdsLib_Scalar_FromString(bad size) (%d) != 0", (int)TestResult);

    /* Test conversion of unsigned ints of various sizes to strings */
    ValBuf.u8 = 215;
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 1, EDSLIB_BASICTYPE_UNSIGNED_INT, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_ToString(u8) (%d) == 0", (int)TestResult);
    UtAssert_True(strcmp(TestBuffer, "215") == 0, "OutputBuffer(%s) == 215", TestBuffer);
    ValBuf.u16 = 35645;
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 2, EDSLIB_BASICTYPE_UNSIGNED_INT, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_ToString(u16) (%d) == 0", (int)TestResult);
    UtAssert_True(strcmp(TestBuffer, "35645") == 0, "OutputBuffer(%s) == 35645", TestBuffer);
    ValBuf.u32 = 3194304394;
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 4, EDSLIB_BASICTYPE_UNSIGNED_INT, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_ToString(u32) (%d) == 0", (int)TestResult);
    UtAssert_True(strcmp(TestBuffer, "3194304394") == 0, "OutputBuffer(%s) == 3194304394", TestBuffer);
    ValBuf.u64 = 59486574499193;
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 8, EDSLIB_BASICTYPE_UNSIGNED_INT, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_ToString(u64) (%d) == 0", (int)TestResult);
    UtAssert_True(strcmp(TestBuffer, "59486574499193") == 0, "OutputBuffer(%s) == 59486574499193", TestBuffer);
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 3, EDSLIB_BASICTYPE_UNSIGNED_INT, NULL);
    UtAssert_True(TestResult != 0, "EdsLib_Scalar_ToString(bad_size) (%d) != 0", (int)TestResult);

    /*
     * Test conversion of strings to floats and doubles
     * NOTE: specific values were chosen such that they are exactly representable in IEEE754 number space.
     * (it must pass an exact equality check)
     */
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 4, "2.5", EDSLIB_BASICTYPE_FLOAT, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_FromString(f32) (%d) == 0", (int)TestResult);
    UtAssert_True(ValBuf.f32 == 2.5, "ValBuf.flt (%3.1f) == 2.5", (float)ValBuf.f32);
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 8, "-3.5", EDSLIB_BASICTYPE_FLOAT, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_FromString(f64) (%d) == 0", (int)TestResult);
    UtAssert_True(ValBuf.f64 == -3.5, "ValBuf.dbl (%3.1lf) == -3.5", (double)ValBuf.f64);
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 0, "xxx", EDSLIB_BASICTYPE_FLOAT, NULL);
    UtAssert_True(TestResult != 0, "EdsLib_Scalar_FromString(non-number) (%d) != 0", (int)TestResult);

    /* Test conversion of floats and doubles to strings */
    ValBuf.f32 = -6.25;
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 4, EDSLIB_BASICTYPE_FLOAT, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_ToString(f32) (%d) == 0", (int)TestResult);
    UtAssert_True(strcmp(TestBuffer, "-6.25") == 0, "OutputBuffer(%s) == -6.25", TestBuffer);
    ValBuf.f64 = 17.75;
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 8, EDSLIB_BASICTYPE_FLOAT, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_ToString(f64) (%d) == 0", (int)TestResult);
    UtAssert_True(strcmp(TestBuffer, "17.75") == 0, "OutputBuffer(%s) == 17.75", TestBuffer);

    /* Test conversion of other element types (non-numeric entities) */
    /* A "raw binary" from string should be rejected for now (base64 encoding is a possible way to accept this) */
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 6, "blahblahblah", EDSLIB_BASICTYPE_BINARY, NULL);
    UtAssert_True(TestResult != 0, "EdsLib_Scalar_FromString(binary) (%d) != 0", (int)TestResult);
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 4, EDSLIB_BASICTYPE_BINARY, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_ToString(binary) (%d) == 0", (int)TestResult);
    UtAssert_True(strcmp(TestBuffer, "<data>") == 0, "OutputBuffer(%s) == <data>", TestBuffer);

    /* Containers/Interfaces/Arrays cannot be produced directly from a string right now */
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 6, "blahblahblah", EDSLIB_BASICTYPE_CONTAINER, NULL);
    UtAssert_True(TestResult != 0, "EdsLib_Scalar_FromString(container) (%d) != 0", (int)TestResult);
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 6, EDSLIB_BASICTYPE_CONTAINER, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_ToString(container) (%d) == 0", (int)TestResult);
    UtAssert_True(strcmp(TestBuffer, "<container>") == 0, "OutputBuffer(%s) == <container>", TestBuffer);
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 6, "blahblahblah", EDSLIB_BASICTYPE_ARRAY, NULL);
    UtAssert_True(TestResult != 0, "EdsLib_Scalar_FromString(array) (%d) != 0", (int)TestResult);
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 6, EDSLIB_BASICTYPE_ARRAY, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_ToString(array) (%d) == 0", (int)TestResult);
    UtAssert_True(strcmp(TestBuffer, "<array>") == 0, "OutputBuffer(%s) == <array>", TestBuffer);
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 6, "blahblahblah", EDSLIB_BASICTYPE_INTERFACE, NULL);
    UtAssert_True(TestResult != 0, "EdsLib_Scalar_FromString(interface) (%d) != 0", (int)TestResult);
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 6, EDSLIB_BASICTYPE_INTERFACE, NULL);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_ToString(interface) (%d) == 0", (int)TestResult);
    UtAssert_True(strcmp(TestBuffer, "<interface>") == 0, "OutputBuffer(%s) == <interface>", TestBuffer);

    /* Test conversions using "display hints" */

    /*
     * The "fixed name" display hint will force a specific name for the item
     * It is only relevant for output (EdsLib_Scalar_ToString).
     */
    Conv.DisplayHint = EDSLIB_DISPLAYHINT_FIXEDNAME;
    Conv.HintArg = "MyFixedName";
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 4, EDSLIB_BASICTYPE_BINARY, &Conv);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_ToString(fixedname) (%d) == 0", (int)TestResult);
    UtAssert_True(strcmp(TestBuffer, "MyFixedName") == 0, "OutputBuffer(%s) == MyFixedName", TestBuffer);

    /*
     * The "char string" display hint will interpret the binary data as a character string
     * note that null termination is _not_ required for strings within containers, but should
     * be truncated to the right size
     */
    Conv.DisplayHint = EDSLIB_DISPLAYHINT_CHAR_STRING;
    Conv.HintArg = NULL;
    memset(ValBuf.chrs, 'y', 7);
    ValBuf.chrs[7] = 0;
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 4, "12345678", EDSLIB_BASICTYPE_BINARY, &Conv);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_FromString(string) (%d) == 0", (int)TestResult);
    UtAssert_True(memcmp(ValBuf.chrs, "1234yyy\0", 8) == 0, "chrs(%s) == 1234yyy\\0", ValBuf.chrs);
    memset(ValBuf.chrs, 'v', 7);
    ValBuf.chrs[7] = 0;
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 6, "1234", EDSLIB_BASICTYPE_BINARY, &Conv);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_FromString(string) (%d) == 0", (int)TestResult);
    UtAssert_True(memcmp(ValBuf.chrs, "1234\0\0v\0", 8) == 0, "chrs(%s) == 1234\\0\\0v\\0", ValBuf.chrs);
    memset(ValBuf.chrs, 'z', 7);
    ValBuf.chrs[7] = 0;
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 5, EDSLIB_BASICTYPE_BINARY, &Conv);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_ToString(string) (%d) == 0", (int)TestResult);
    UtAssert_True(strcmp(TestBuffer, "zzzzz") == 0, "TestBuffer(%s) == zzzzz", TestBuffer);
    memcpy(ValBuf.chrs, "ab\0cd\033ef", 8);
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 8, EDSLIB_BASICTYPE_BINARY, &Conv);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_ToString(string) (%d) == 0", (int)TestResult);
    UtAssert_True(strcmp(TestBuffer, "abcd.ef") == 0, "TestBuffer(%s) == abcd.ef", TestBuffer);

    /*
     * Memory Addresses are unsigned ints, but should always be hex
     * Note: Actual use of this feature is discouraged but supported for now
     */
    Conv.DisplayHint = EDSLIB_DISPLAYHINT_ADDRESS;
    Conv.HintArg = NULL;
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 4, "1234", EDSLIB_BASICTYPE_UNSIGNED_INT, &Conv);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_FromString(address) (%d) == 0", (int)TestResult);
    UtAssert_True(ValBuf.u32  == 0x1234, "ValBuf.u32 (0x%lx) == 0x1234", (unsigned long)ValBuf.u32);
    ValBuf.u32 = 1234;
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 4, EDSLIB_BASICTYPE_UNSIGNED_INT, &Conv);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_ToString(address) (%d) == 0", (int)TestResult);
    UtAssert_True(strcmp(TestBuffer, "0x000004d2") == 0, "OutputBuffer(%s) == 0x000004d2", TestBuffer);

    /*
     * Enumerations use a lookup table to determine a user-friendly name for the value
     */
    EdsLib_Get_DisplayHint(&GD_FULL, EDSLIB_MAKE_ID(UTM_EDS_UT1_INDEX,UT1_CMDVAL_Enum_t_DATADICTIONARY), &Conv);
    UtAssert_Simple(Conv.DisplayHint == EDSLIB_DISPLAYHINT_ENUM_SYMTABLE);
    UtAssert_Simple(Conv.HintArg != NULL);
    TestResult = EdsLib_Scalar_FromString(ValBuf.bytes, 4, "C144", EDSLIB_BASICTYPE_UNSIGNED_INT, &Conv);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_FromString(enum) (%d) == 0", (int)TestResult);
    UtAssert_True(ValBuf.u32  == 144, "ValBuf.u32 (%lu) == 144", (unsigned long)ValBuf.u32);
    ValBuf.i16 = 160;
    TestResult = EdsLib_Scalar_ToString(TestBuffer, sizeof(TestBuffer), ValBuf.bytes, 2, EDSLIB_BASICTYPE_UNSIGNED_INT, &Conv);
    UtAssert_True(TestResult == 0, "EdsLib_Scalar_ToString(enum) (%d) == 0", (int)TestResult);
    UtAssert_True(strcmp(TestBuffer, "C160") == 0, "OutputBuffer(%s) == C160", TestBuffer);

}

