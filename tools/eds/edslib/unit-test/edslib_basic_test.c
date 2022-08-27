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
 * \file     edslib_basic_test.c
 * \ingroup  edslib
 * \author   joseph.p.hickey@nasa.gov
 * 
 * Basic testing of the "data type" EDS structures
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utassert.h"

#include <stdint.h>
#include <cfe_mission_cfg.h>
#include <UTM_eds_index.h>

#include "edslib_datatypedb.h"
#include "edslib_id.h"
#include "UTHDR_msgdefs.h"
#include "UT1_msgdefs.h"

extern EdsLib_DataDictionary_t UT1_DATATYPE_DB;
extern EdsLib_DataDictionary_t UTHDR_DATATYPE_DB;
extern EdsLib_DataDictionary_t ut_base_types_DATATYPE_DB;

EdsLib_DataDictionary_t *UTM_EDS_LOCAL_APPTBL[UTM_EDS_MAX_INDEX] = { NULL };
EdsLib_DatabaseObject_t GD_BASIC =
{
      .AppTableSize = UTM_EDS_MAX_INDEX,
      .AppDataTable = UTM_EDS_LOCAL_APPTBL,
      .AppNameTable = NULL,
      .SymbolTable = NULL
};

typedef struct
{
    UTHDR_UtPriHdr_t Pri;
    UTHDR_UtTlmSecHdr_t Tlm;
} TestTlmHdr_t;

typedef struct
{
    UTHDR_UtPriHdr_t Pri;
    UTHDR_UtCmdSecHdr_t Cmd;
} TestCmdHdr_t;

typedef struct
{
   TestTlmHdr_t Hdr;
   UT1_Tlm1_t Payload;
} TestTlm_t;

typedef struct
{
   TestCmdHdr_t Hdr;
   UT1_Cmd_t Payload;
} TestCmd_t;

union
{
   uint8_t Bytes[1];
   TestCmd_t Cmd;
   TestTlm_t Tlm;
} Buffer;

typedef struct
{
    bool DoDescend;
    bool DoStop;
    const char *TestName;
    const char **CurrentLine;
} WalkTestState_t;

const char *BasicFormats_EXPECTED[] =
{
        "BasicFormats:1:I=1/T=1/O=0/S=1/N=0",
        "BasicFormats:1:I=2/T=2/O=0/S=1/N=0",
        "BasicFormats:1:I=3/T=1/O=0/S=2/N=0",
        "BasicFormats:1:I=4/T=2/O=0/S=2/N=0",
        "BasicFormats:1:I=5/T=1/O=0/S=4/N=0",
        "BasicFormats:1:I=6/T=2/O=0/S=4/N=0",
        "BasicFormats:1:I=7/T=1/O=0/S=8/N=0",
        "BasicFormats:1:I=8/T=2/O=0/S=8/N=0",
        "BasicFormats:1:I=9/T=2/O=0/S=8/N=0",
        "BasicFormats:1:I=10/T=3/O=0/S=4/N=0",
        "BasicFormats:1:I=11/T=3/O=0/S=8/N=0",
        "BasicFormats:1:I=12/T=7/O=0/S=4/N=1",
        "BasicFormats:1:I=13/T=7/O=0/S=8/N=2",
        "BasicFormats:1:I=14/T=7/O=0/S=8/N=2",
        "BasicFormats:1:I=15/T=5/O=0/S=4/N=2",
        "BasicFormats:1:I=16/T=5/O=0/S=4/N=2",
        "BasicFormats:1:I=17/T=5/O=0/S=4/N=2",
        "BasicFormats:1:I=18/T=7/O=0/S=8/N=1",
        "BasicFormats:1:I=19/T=7/O=0/S=312/N=2",
        "BasicFormats:1:I=20/T=7/O=0/S=12/N=2",
        "BasicFormats:1:I=21/T=7/O=0/S=12/N=2",
        "BasicFormats:1:I=22/T=2/O=0/S=1/N=0",
        "BasicFormats:1:I=23/T=4/O=0/S=10/N=0",
        "BasicFormats:1:I=24/T=6/O=0/S=4/N=4",
        "BasicFormats:1:I=25/T=6/O=0/S=250/N=250",
        "BasicFormats:1:I=26/T=6/O=0/S=3/N=3",
        "BasicFormats:1:I=27/T=6/O=0/S=6/N=3",
        "BasicFormats:1:I=28/T=6/O=0/S=4/N=1",
        "BasicFormats:1:I=29/T=5/O=0/S=48/N=10",
        "BasicFormats:1:I=30/T=6/O=0/S=240/N=5",
        "BasicFormats:1:I=31/T=5/O=0/S=4/N=1",
        "BasicFormats:1:I=32/T=5/O=0/S=304/N=5",
        "BasicFormats:1:I=33/T=5/O=0/S=250/N=1",
        NULL
};

const char *PayloadShallow_EXPECTED[] =
{
        "PayloadShallow:2:I=0/T=7/O=0/S=312/N=2",
        "PayloadShallow:1:I=0/T=7/O=0/S=8/N=2",
        "PayloadShallow:1:I=1/T=5/O=8/S=304/N=5",
        "PayloadShallow:3:I=0/T=7/O=0/S=312/N=2",
        NULL
};

const char *PayloadDeep_EXPECTED[] =
{
        "PayloadDeep:2:I=0/T=7/O=0/S=312/N=2",
        "PayloadDeep:1:I=0/T=7/O=0/S=8/N=2",
        "PayloadDeep:2:I=1/T=7/O=0/S=8/N=2",
        "PayloadDeep:1:I=0/T=7/O=0/S=4/N=1",
        "PayloadDeep:2:I=1/T=7/O=0/S=4/N=1",
        "PayloadDeep:1:I=0/T=5/O=0/S=4/N=2",
        "PayloadDeep:2:I=1/T=5/O=0/S=4/N=2",
        "PayloadDeep:1:I=0/T=2/O=0/S=2/N=0",
        "PayloadDeep:1:I=1/T=2/O=2/S=2/N=0",
        "PayloadDeep:3:I=1/T=5/O=0/S=4/N=2",
        "PayloadDeep:3:I=1/T=7/O=0/S=4/N=1",
        "PayloadDeep:1:I=1/T=5/O=4/S=4/N=2",
        "PayloadDeep:2:I=2/T=5/O=4/S=4/N=2",
        "PayloadDeep:1:I=0/T=2/O=4/S=2/N=0",
        "PayloadDeep:1:I=1/T=2/O=6/S=2/N=0",
        "PayloadDeep:3:I=2/T=5/O=4/S=4/N=2",
        "PayloadDeep:3:I=1/T=7/O=0/S=8/N=2",
        "PayloadDeep:1:I=1/T=5/O=8/S=304/N=5",
        "PayloadDeep:2:I=2/T=5/O=8/S=304/N=5",
        "PayloadDeep:1:I=0/T=5/O=8/S=48/N=10",
        "PayloadDeep:2:I=1/T=5/O=8/S=48/N=10",
        "PayloadDeep:1:I=0/T=2/O=8/S=1/N=0",
        "PayloadDeep:1:I=1/T=1/O=9/S=1/N=0",
        "PayloadDeep:1:I=2/T=2/O=10/S=1/N=0",
        "PayloadDeep:1:I=3/T=2/O=12/S=2/N=0",
        "PayloadDeep:1:I=4/T=1/O=14/S=2/N=0",
        "PayloadDeep:1:I=5/T=2/O=16/S=4/N=0",
        "PayloadDeep:1:I=6/T=1/O=20/S=4/N=0",
        "PayloadDeep:1:I=7/T=3/O=24/S=4/N=0",
        "PayloadDeep:1:I=8/T=3/O=32/S=8/N=0",
        "PayloadDeep:1:I=9/T=4/O=40/S=10/N=0",
        "PayloadDeep:3:I=1/T=5/O=8/S=48/N=10",
        "PayloadDeep:1:I=1/T=6/O=56/S=3/N=3",
        "PayloadDeep:2:I=2/T=6/O=56/S=3/N=3",
        "PayloadDeep:1:I=0/T=2/O=56/S=1/N=0",
        "PayloadDeep:1:I=1/T=2/O=57/S=1/N=0",
        "PayloadDeep:1:I=2/T=2/O=58/S=1/N=0",
        "PayloadDeep:3:I=2/T=6/O=56/S=3/N=3",
        "PayloadDeep:1:I=2/T=6/O=60/S=6/N=3",
        "PayloadDeep:2:I=3/T=6/O=60/S=6/N=3",
        "PayloadDeep:1:I=0/T=2/O=60/S=2/N=0",
        "PayloadDeep:1:I=1/T=2/O=62/S=2/N=0",
        "PayloadDeep:1:I=2/T=2/O=64/S=2/N=0",
        "PayloadDeep:3:I=3/T=6/O=60/S=6/N=3",
        "PayloadDeep:1:I=3/T=6/O=68/S=4/N=1",
        "PayloadDeep:2:I=4/T=6/O=68/S=4/N=1",
        "PayloadDeep:1:I=0/T=2/O=68/S=4/N=0",
        "PayloadDeep:3:I=4/T=6/O=68/S=4/N=1",
        "PayloadDeep:1:I=4/T=6/O=72/S=240/N=5",
        "PayloadDeep:2:I=5/T=6/O=72/S=240/N=5",
        "PayloadDeep:1:I=0/T=5/O=72/S=48/N=10",
        "PayloadDeep:2:I=1/T=5/O=72/S=48/N=10",
        "PayloadDeep:1:I=0/T=2/O=72/S=1/N=0",
        "PayloadDeep:1:I=1/T=1/O=73/S=1/N=0",
        "PayloadDeep:1:I=2/T=2/O=74/S=1/N=0",
        "PayloadDeep:1:I=3/T=2/O=76/S=2/N=0",
        "PayloadDeep:1:I=4/T=1/O=78/S=2/N=0",
        "PayloadDeep:1:I=5/T=2/O=80/S=4/N=0",
        "PayloadDeep:1:I=6/T=1/O=84/S=4/N=0",
        "PayloadDeep:1:I=7/T=3/O=88/S=4/N=0",
        "PayloadDeep:1:I=8/T=3/O=96/S=8/N=0",
        "PayloadDeep:1:I=9/T=4/O=104/S=10/N=0",
        "PayloadDeep:3:I=1/T=5/O=72/S=48/N=10",
        "PayloadDeep:1:I=1/T=5/O=120/S=48/N=10",
        "PayloadDeep:2:I=2/T=5/O=120/S=48/N=10",
        "PayloadDeep:1:I=0/T=2/O=120/S=1/N=0",
        "PayloadDeep:1:I=1/T=1/O=121/S=1/N=0",
        "PayloadDeep:1:I=2/T=2/O=122/S=1/N=0",
        "PayloadDeep:1:I=3/T=2/O=124/S=2/N=0",
        "PayloadDeep:1:I=4/T=1/O=126/S=2/N=0",
        "PayloadDeep:1:I=5/T=2/O=128/S=4/N=0",
        "PayloadDeep:1:I=6/T=1/O=132/S=4/N=0",
        "PayloadDeep:1:I=7/T=3/O=136/S=4/N=0",
        "PayloadDeep:1:I=8/T=3/O=144/S=8/N=0",
        "PayloadDeep:1:I=9/T=4/O=152/S=10/N=0",
        "PayloadDeep:3:I=2/T=5/O=120/S=48/N=10",
        "PayloadDeep:1:I=2/T=5/O=168/S=48/N=10",
        "PayloadDeep:2:I=3/T=5/O=168/S=48/N=10",
        "PayloadDeep:1:I=0/T=2/O=168/S=1/N=0",
        "PayloadDeep:1:I=1/T=1/O=169/S=1/N=0",
        "PayloadDeep:1:I=2/T=2/O=170/S=1/N=0",
        "PayloadDeep:1:I=3/T=2/O=172/S=2/N=0",
        "PayloadDeep:1:I=4/T=1/O=174/S=2/N=0",
        "PayloadDeep:1:I=5/T=2/O=176/S=4/N=0",
        "PayloadDeep:1:I=6/T=1/O=180/S=4/N=0",
        "PayloadDeep:1:I=7/T=3/O=184/S=4/N=0",
        "PayloadDeep:1:I=8/T=3/O=192/S=8/N=0",
        "PayloadDeep:1:I=9/T=4/O=200/S=10/N=0",
        "PayloadDeep:3:I=3/T=5/O=168/S=48/N=10",
        "PayloadDeep:1:I=3/T=5/O=216/S=48/N=10",
        "PayloadDeep:2:I=4/T=5/O=216/S=48/N=10",
        "PayloadDeep:1:I=0/T=2/O=216/S=1/N=0",
        "PayloadDeep:1:I=1/T=1/O=217/S=1/N=0",
        "PayloadDeep:1:I=2/T=2/O=218/S=1/N=0",
        "PayloadDeep:1:I=3/T=2/O=220/S=2/N=0",
        "PayloadDeep:1:I=4/T=1/O=222/S=2/N=0",
        "PayloadDeep:1:I=5/T=2/O=224/S=4/N=0",
        "PayloadDeep:1:I=6/T=1/O=228/S=4/N=0",
        "PayloadDeep:1:I=7/T=3/O=232/S=4/N=0",
        "PayloadDeep:1:I=8/T=3/O=240/S=8/N=0",
        "PayloadDeep:1:I=9/T=4/O=248/S=10/N=0",
        "PayloadDeep:3:I=4/T=5/O=216/S=48/N=10",
        "PayloadDeep:1:I=4/T=5/O=264/S=48/N=10",
        "PayloadDeep:2:I=5/T=5/O=264/S=48/N=10",
        "PayloadDeep:1:I=0/T=2/O=264/S=1/N=0",
        "PayloadDeep:1:I=1/T=1/O=265/S=1/N=0",
        "PayloadDeep:1:I=2/T=2/O=266/S=1/N=0",
        "PayloadDeep:1:I=3/T=2/O=268/S=2/N=0",
        "PayloadDeep:1:I=4/T=1/O=270/S=2/N=0",
        "PayloadDeep:1:I=5/T=2/O=272/S=4/N=0",
        "PayloadDeep:1:I=6/T=1/O=276/S=4/N=0",
        "PayloadDeep:1:I=7/T=3/O=280/S=4/N=0",
        "PayloadDeep:1:I=8/T=3/O=288/S=8/N=0",
        "PayloadDeep:1:I=9/T=4/O=296/S=10/N=0",
        "PayloadDeep:3:I=5/T=5/O=264/S=48/N=10",
        "PayloadDeep:3:I=5/T=6/O=72/S=240/N=5",
        "PayloadDeep:3:I=2/T=5/O=8/S=304/N=5",
        "PayloadDeep:3:I=0/T=7/O=0/S=312/N=2",
        NULL
};

const char *PayloadStop_EXPECTED[] =
{
        "PayloadStop:2:I=0/T=7/O=0/S=312/N=2",
        NULL
};

const char *Invalid_EXPECTED[] =
{
        "INVALID",
        NULL
};

static EdsLib_Iterator_Rc_t BasicCheckCallback(EdsLib_Iterator_CbType_t CbType, void *Arg, EdsLib_Basic_MemberInfo_t *WalkState)
{
    char OutputBuffer[128];
    const char *TempPtr;
    WalkTestState_t *TestState = Arg;

    snprintf(OutputBuffer, sizeof(OutputBuffer), "%s:%u:I=%u/T=%u/O=%u/S=%u/N=%u",
            TestState->TestName,
            (unsigned int)CbType,
            (unsigned int)WalkState->ComponentInfo.CurrIndex,
            (unsigned int)WalkState->TypeInfo.ElemType,
            (unsigned int)WalkState->ComponentInfo.AbsOffset,
            (unsigned int)WalkState->TypeInfo.Size.Bytes,
            (unsigned int)WalkState->TypeInfo.NumSubElements);

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

    if (TestState->DoStop)
    {
        return EDSLIB_ITERATOR_RC_STOP;
    }

    if (TestState->DoDescend && CbType == EDSLIB_ITERATOR_CBTYPE_MEMBER)
    {
       return EDSLIB_ITERATOR_RC_DESCEND;
    }

    return EDSLIB_ITERATOR_RC_CONTINUE;
}

void EdsLib_Basic_Test(void)
{
    EdsLib_Id_t MsgId;
    int32_t rc;
    uint8_t *BufPtr;
    uint16_t i;
    uint16_t Orig16;
    uint32_t Orig32;
    WalkTestState_t WalkTestState;
    EdsLib_DataTypeDB_TypeInfo_t TypeInfo;
    EdsLib_DataTypeDB_EntityInfo_t CompInfo;

    rc = EdsLib_DataTypeDB_Register(&GD_BASIC, &UT1_DATATYPE_DB, &i);
    UtAssert_Simple(rc == EDSLIB_SUCCESS);
    UtAssert_Simple(i == UTM_EDS_UT1_INDEX);
    rc = EdsLib_DataTypeDB_Register(&GD_BASIC, &UTHDR_DATATYPE_DB, &i);
    UtAssert_Simple(rc == EDSLIB_SUCCESS);
    UtAssert_Simple(i == UTM_EDS_UTHDR_INDEX);
    rc = EdsLib_DataTypeDB_Register(&GD_BASIC, &ut_base_types_DATATYPE_DB, &i);
    UtAssert_Simple(rc == EDSLIB_SUCCESS);
    UtAssert_Simple(i == UTM_EDS_ut_base_types_INDEX);

    memset(&WalkTestState, 0, sizeof(WalkTestState));
    WalkTestState.TestName = "BasicFormats";
    WalkTestState.CurrentLine = BasicFormats_EXPECTED;
    EdsLib_Iterator_Formats(&GD_BASIC, BasicCheckCallback, &WalkTestState);
    UtAssert_Simple(*WalkTestState.CurrentLine == NULL);

    UtAssert_Simple(EdsLib_Endian_NeedsSwap(EDSLIB_BYTE_ORDER_BIG_ENDIAN, EDSLIB_BYTE_ORDER_LITTLE_ENDIAN));
    UtAssert_Simple(EdsLib_Endian_NeedsSwap(EDSLIB_BYTE_ORDER_NATIVE, EDSLIB_BYTE_ORDER_NON_NATIVE));
    if (EdsLib_Endian_NeedsSwap(EDSLIB_BYTE_ORDER_NATIVE, EDSLIB_BYTE_ORDER_BIG_ENDIAN) == 0)
    {
        UtAssert_Simple(EdsLib_Endian_NeedsSwap(EDSLIB_BYTE_ORDER_NATIVE, EDSLIB_BYTE_ORDER_LITTLE_ENDIAN));
    }
    else
    {
        UtAssert_Simple(!EdsLib_Endian_NeedsSwap(EDSLIB_BYTE_ORDER_NATIVE, EDSLIB_BYTE_ORDER_LITTLE_ENDIAN));
    }

    BufPtr = Buffer.Bytes;
    for (i=0; i < sizeof(Buffer); ++i)
    {
        BufPtr[i] = i & 0xFF;
    }
    Orig32 = Buffer.Tlm.Payload.bitmask32[0];
    Orig16 = Buffer.Tlm.Hdr.Pri.UtLength;

    EdsLib_SwapInPlace(&GD_BASIC, EDSLIB_MAKE_ID(UTM_EDS_UTHDR_INDEX, UTHDR_BasePacket_Intf_t_DATADICTIONARY), Buffer.Bytes, 0);
    UtAssert_True(Orig16 != Buffer.Tlm.Hdr.Pri.UtLength, "Orig16 (%x) != Buffer.Tlm.Hdr.Pri.UtLength(%x)",
            (unsigned int)Orig16 , (unsigned int)Buffer.Tlm.Hdr.Pri.UtLength);
    UtAssert_True(Orig32 == Buffer.Tlm.Payload.bitmask32[0], "Orig32 (%x) == Buffer.Tlm.Payload.bitmask32[0](%x)",
            (unsigned int)Orig32 , (unsigned int)Buffer.Tlm.Payload.bitmask32[0]);
    EdsLib_ByteSwap((uint8_t*)&Orig16, sizeof(Orig16));
    UtAssert_True(Orig16 == Buffer.Tlm.Hdr.Pri.UtLength, "Orig16 (%x) == Buffer.Tlm.Hdr.Pri.UtLength(%x)",
            (unsigned int)Orig16 , (unsigned int)Buffer.Tlm.Hdr.Pri.UtLength);

    MsgId = EdsLib_Lookup_DerivedType(&GD_BASIC, EDSLIB_MAKE_ID(UTM_EDS_UTHDR_INDEX, UTHDR_BasePacket_Intf_t_DATADICTIONARY),
            0, UT1_TestTLM_MSG);
    Orig16 = Buffer.Tlm.Hdr.Tlm.TlmExtraInfo1;
    EdsLib_SwapInPlace(&GD_BASIC, MsgId, Buffer.Bytes, sizeof(UTHDR_UtPriHdr_t));
    UtAssert_True(Orig16 != Buffer.Tlm.Hdr.Tlm.TlmExtraInfo1, "Orig16 (%x) != Buffer.Tlm.Hdr.Tlm.TlmExtraInfo1(%x)",
            (unsigned int)Orig16 , (unsigned int)Buffer.Tlm.Hdr.Tlm.TlmExtraInfo1);
    UtAssert_True(Orig32 != Buffer.Tlm.Payload.bitmask32[0], "Orig32 (%x) != Buffer.Tlm.Payload.bitmask32[0](%x)",
            (unsigned int)Orig32 , (unsigned int)Buffer.Tlm.Payload.bitmask32[0]);
    EdsLib_ByteSwap((uint8_t*)&Orig16, sizeof(Orig16));
    EdsLib_ByteSwap((uint8_t*)&Orig32, sizeof(Orig32));
    UtAssert_True(Orig16 == Buffer.Tlm.Hdr.Tlm.TlmExtraInfo1, "Orig16 (%x) == Buffer.Tlm.Hdr.Tlm.TlmExtraInfo1(%x)",
            (unsigned int)Orig16 , (unsigned int)Buffer.Tlm.Hdr.Tlm.TlmExtraInfo1);
    UtAssert_True(Orig32 == Buffer.Tlm.Payload.bitmask32[0], "Orig32 (%x) == Buffer.Tlm.Payload.bitmask32[0](%x)",
            (unsigned int)Orig32 , (unsigned int)Buffer.Tlm.Payload.bitmask32[0]);

    for (i=0; i < sizeof(Buffer); ++i)
    {
        BufPtr[i] = i & 0xFF;
    }

    Orig16 = Buffer.Cmd.Hdr.Cmd.UtCommandId;
    MsgId = EdsLib_Lookup_DerivedType(&GD_BASIC, EDSLIB_MAKE_ID(UTM_EDS_UTHDR_INDEX, UTHDR_BasePacket_Intf_t_DATADICTIONARY),
            0, UT1_TestTLM_MSG);
    EdsLib_SwapInPlace(&GD_BASIC, MsgId, Buffer.Bytes, 0);
    UtAssert_True(Orig16 != Buffer.Cmd.Hdr.Cmd.UtCommandId, "Orig16 (%x) != Buffer.Cmd.Hdr.Cmd.UtCommandId(%x)",
            (unsigned int)Orig16 , (unsigned int)Buffer.Cmd.Hdr.Cmd.UtCommandId);
    EdsLib_ByteSwap((uint8_t*)&Orig16, sizeof(Orig16));
    UtAssert_True(Orig16 == Buffer.Cmd.Hdr.Cmd.UtCommandId, "Orig16 (%x) == Buffer.Cmd.Hdr.Cmd.UtCommandId(%x)",
            (unsigned int)Orig16 , (unsigned int)Buffer.Cmd.Hdr.Cmd.UtCommandId);

    EdsLib_SwapInPlace(&GD_BASIC, EDSLIB_ID_INVALID, Buffer.Bytes, 0);
    UtAssert_True(Orig16 == Buffer.Cmd.Hdr.Cmd.UtCommandId, "Orig16 (%x) == Buffer.Cmd.Hdr.Cmd.UtCommandId(%x)",
            (unsigned int)Orig16 , (unsigned int)Buffer.Cmd.Hdr.Cmd.UtCommandId);

#ifdef JPHFIX /* Auth levels not currently implemented in SOIS XML schema */
    TestResultU8 = EdsLib_Lookup_AuthLevel(&GD_BASIC, MsgId);
    UtAssert_True(TestResultU8 == 200, "EdsLib_Lookup_AuthLevel(GD, %x) (%u) == 200",
            (unsigned int)MsgId, (unsigned int)TestResultU8);
    EdsLib_Set_CommandCode(&MsgId, UT1_Cmd2_CC);
    TestResultU8 = EdsLib_Lookup_AuthLevel(&GD_BASIC, MsgId);
    UtAssert_True(TestResultU8 == 100, "EdsLib_Lookup_AuthLevel(GD, %x) (%u) == 100",
            (unsigned int)MsgId, (unsigned int)TestResultU8);
#endif

#ifdef JPHFIX
    MsgId = EDSLIB_MAKE_ID(UTM_EDS_UT1_INDEX, UT1_TestCMD_Intf_t_DATADICTIONARY);
    EdsLib_Set_CpuNumber(&MsgId, 3);
    StreamId = EdsLib_MsgId_To_StreamId(&GD_BASIC, MsgId);
    UtAssert_True(StreamId != 0, "StreamId (%x) != 0", (unsigned int)StreamId);
    Orig32 = MsgId;
    MsgId = EdsLib_MsgId_From_StreamId(&GD_BASIC, StreamId);
    TestResultU16 = EdsLib_Get_CpuNumber(MsgId);
    UtAssert_True(TestResultU16 == 3, "EdsLib_Get_CpuNumber(%x) (%u) == 3",
            (unsigned int)MsgId, (unsigned int)TestResultU16);
    TestResultU16 = EdsLib_Get_AppIdx(MsgId);
    TestCompareU16 = EdsLib_Get_AppIdx(Orig32);
    UtAssert_True(TestResultU16 == TestCompareU16, "EdsLib_Get_AppIdx(%x) (%u) == EdsLib_Get_AppIdx(%x) (%u)",
            (unsigned int)MsgId, (unsigned int)TestResultU16,
            (unsigned int)Orig32, (unsigned int)TestCompareU16);
    TestResultU16 = EdsLib_Get_FormatIdx(MsgId);
    TestCompareU16 = EdsLib_Get_FormatIdx(Orig32);
    UtAssert_True(TestResultU16 == TestCompareU16, "EdsLib_Get_FormatIndex(%x) (%u) == EdsLib_Get_FormatIndex(%x) (%u)",
            (unsigned int)MsgId, (unsigned int)TestResultU16,
            (unsigned int)Orig32, (unsigned int)TestCompareU16);
    MsgId = EdsLib_Lookup_DerivedType(&GD_BASIC, GD_BASIC.PrimaryHeaderMsgId, offsetof(UTHDR_UtPriHdr_t, UtStreamId), UT1_TestTLM_MSG);
    TestResultU8 =  EdsLib_Basic_Validate_MessageSize(&GD_BASIC, MsgId, sizeof(TestTlm_t));
    UtAssert_True(TestResultU8, "EdsLib_Basic_Validate_MessageSize(GD, %x, %u)",
            (unsigned int)MsgId, (unsigned int)sizeof(TestTlm_t));
    TestResultU8 =  EdsLib_Basic_Validate_MessageSize(&GD_BASIC, MsgId, sizeof(TestTlm_t) - 3);
    UtAssert_True(!TestResultU8, "!EdsLib_Basic_Validate_MessageSize(GD, %x, %u)",
            (unsigned int)MsgId, (unsigned int)(sizeof(TestTlm_t) - 3));
    EdsLib_Set_CpuNumber(&MsgId, 4);
    StreamId = EdsLib_MsgId_To_StreamId(&GD_BASIC, MsgId);
    UtAssert_True(StreamId != 0, "StreamId (%x) != 0", (unsigned int)StreamId );
    Orig32 = MsgId;
    MsgId = EdsLib_MsgId_From_StreamId(&GD_BASIC, StreamId);
    UtAssert_True(MsgId == Orig32, "MsgId (%x) == Orig32(%x)", (unsigned int)MsgId , (unsigned int)Orig32);
#endif

    MsgId = EDSLIB_MAKE_ID(UTM_EDS_UT1_INDEX, UT1_Table1_t_DATADICTIONARY);
#ifdef JPHFIX /* Auth levels not currently implemented in SOIS XML schema */
    TestResultU8 =  EdsLib_Lookup_AuthLevel(&GD_BASIC, MsgId);
    UtAssert_True(TestResultU8 == 0, "EdsLib_Lookup_AuthLevel(GD, %x) (%u) == 0",
            (unsigned int)MsgId, (unsigned int)TestResultU8);
#endif
    rc = EdsLib_DataTypeDB_GetTypeInfo(&GD_BASIC, MsgId, &TypeInfo);
    UtAssert_Simple(rc == EDSLIB_SUCCESS);
    UtAssert_True(TypeInfo.Size.Bytes == sizeof(UT1_Table1_t), "EdsLib_DataTypeDB_GetTypeInfo(GD, %x) (size %u) == sizeof(UT1_Table1_t)(%u)",
            (unsigned int)MsgId, (unsigned int)TypeInfo.Size.Bytes, (unsigned int)sizeof(UT1_Table1_t));
    EdsLib_Set_AppIdx(&MsgId, 30);
    rc = EdsLib_DataTypeDB_GetTypeInfo(&GD_BASIC, MsgId, &TypeInfo);
    UtAssert_True(rc != EDSLIB_SUCCESS, "EdsLib_DataTypeDB_GetTypeInfo(GD, %x) (%u) != EDSLIB_SUCCESS",
            (unsigned int)MsgId, (unsigned int)rc);
    rc = EdsLib_DataTypeDB_GetTypeInfo(NULL, MsgId, &TypeInfo);
    UtAssert_True(rc != EDSLIB_SUCCESS, "EdsLib_DataTypeDB_GetTypeInfo(NULL, %x) (%u) != EDSLIB_SUCCESS",
            (unsigned int)MsgId, (unsigned int)rc);
    MsgId = EDSLIB_MAKE_ID(UTM_EDS_UT1_INDEX, 1000);
    rc = EdsLib_DataTypeDB_GetTypeInfo(NULL, MsgId, &TypeInfo);
    UtAssert_True(rc != EDSLIB_SUCCESS, "EdsLib_DataTypeDB_GetTypeInfo(GD, %x) (%u) != EDSLIB_SUCCESS",
            (unsigned int)MsgId, (unsigned int)rc);

    /* Test calls for dispatching messages (determining payload subtypes given the primary interface type) */
    MsgId = EDSLIB_MAKE_ID(UTM_EDS_UT1_INDEX, UT1_TestTLM_Intf_t_DATADICTIONARY);
    rc = EdsLib_Lookup_SubMemberByOffset(&GD_BASIC, MsgId, sizeof(UTHDR_UtTlm_Intf_t), &CompInfo);
    UtAssert_Simple(rc == EDSLIB_SUCCESS);
    UtAssert_True(CompInfo.CurrIndex == 1, "EdsLib_Lookup_SubMemberByOffset(%x) (%u) == 1",
            (unsigned int)MsgId, (unsigned int)CompInfo.CurrIndex);
    i = EdsLib_Get_AppIdx(CompInfo.StructureId);
    UtAssert_True(i == UTM_EDS_UT1_INDEX, "EdsLib_Get_AppIdx(%x) (%u) == UTM_EDS_UT1_INDEX",
            (unsigned int)CompInfo.StructureId, (unsigned int)i);
    i = EdsLib_Get_FormatIdx(CompInfo.StructureId);
    UtAssert_True(i == UT1_Tlm1_t_DATADICTIONARY, "EdsLib_Get_FormatIdx(%x) (%u) == UT1_Tlm1_t_DATADICTIONARY",
            (unsigned int)CompInfo.StructureId, (unsigned int)i);

    MsgId = EDSLIB_MAKE_ID(UTM_EDS_UT1_INDEX, UT1_BasicDataTypes_5_Array_t_DATADICTIONARY);
    rc = EdsLib_Lookup_SubMemberByOffset(&GD_BASIC, MsgId, 1 + (2 * sizeof(UT1_BasicDataTypes_t)), &CompInfo);
    UtAssert_Simple(rc == EDSLIB_SUCCESS);
    UtAssert_True(CompInfo.CurrIndex == 2, "EdsLib_Lookup_SubMemberByOffset(%x) (%u) == 2",
            (unsigned int)MsgId, (unsigned int)CompInfo.CurrIndex);
    i = EdsLib_Get_AppIdx(CompInfo.StructureId);
    UtAssert_True(i == UTM_EDS_UT1_INDEX, "EdsLib_Get_AppIdx(%x) (%u) == UTM_EDS_UT1_INDEX",
            (unsigned int)CompInfo.StructureId, (unsigned int)i);
    i = EdsLib_Get_FormatIdx(CompInfo.StructureId);
    UtAssert_True(i == UT1_BasicDataTypes_t_DATADICTIONARY, "EdsLib_Get_FormatIdx(%x) (%u) == UT1_BasicDataTypes_t_DATADICTIONARY",
            (unsigned int)CompInfo.StructureId, (unsigned int)i);

    MsgId = EDSLIB_MAKE_ID(UTM_EDS_UT1_INDEX, UT1_BasicDataTypes_t_DATADICTIONARY);
    rc = EdsLib_Lookup_SubMemberByOffset(&GD_BASIC, MsgId, 5, &CompInfo);
    UtAssert_Simple(rc == EDSLIB_SUCCESS);
    UtAssert_True(CompInfo.CurrIndex == 3, "EdsLib_Lookup_SubMemberByOffset(%x) (%u) == 3",
            (unsigned int)MsgId, (unsigned int)CompInfo.CurrIndex);
    i = EdsLib_Get_AppIdx(CompInfo.StructureId);
    UtAssert_True(i == UTM_EDS_ut_base_types_INDEX, "EdsLib_Get_AppIdx(%x) (%u) == UTM_EDS_ut_base_types_INDEX",
            (unsigned int)CompInfo.StructureId, (unsigned int)i);
    i = EdsLib_Get_FormatIdx(CompInfo.StructureId);
    UtAssert_True(i == uint16_Atom_t_DATADICTIONARY, "EdsLib_Get_FormatIdx(%x) (%u) == uint16_Atom_t_DATADICTIONARY",
            (unsigned int)CompInfo.StructureId, (unsigned int)i);

    MsgId = EDSLIB_MAKE_ID(UTM_EDS_UT1_INDEX, UT1_Cmd1_Intf_t_DATADICTIONARY);
    rc = EdsLib_Lookup_SubMemberByIndex(&GD_BASIC, MsgId, 1, &CompInfo);
    UtAssert_Simple(rc == EDSLIB_SUCCESS);
    UtAssert_True(CompInfo.AbsOffset == sizeof(UTHDR_UtCmd_Intf_t), "EdsLib_Lookup_SubMemberByIndex(%x) (%u) == %u",
            (unsigned int)MsgId, (unsigned int)CompInfo.AbsOffset, (unsigned int)sizeof(UTHDR_UtCmd_Intf_t));
    i = EdsLib_Get_AppIdx(CompInfo.StructureId);
    UtAssert_True(i == UTM_EDS_UT1_INDEX, "EdsLib_Get_AppIdx(%x) (%u) == UTM_EDS_UT1_INDEX",
            (unsigned int)CompInfo.StructureId, (unsigned int)i);
    i = EdsLib_Get_FormatIdx(CompInfo.StructureId);
    UtAssert_True(i == UT1_Cmd_t_DATADICTIONARY, "EdsLib_Get_FormatIdx(%x) (%u) == UT1_Cmd_t_DATADICTIONARY",
            (unsigned int)CompInfo.StructureId, (unsigned int)i);


    MsgId = EDSLIB_MAKE_ID(UTM_EDS_UT1_INDEX, UT1_BasicDataTypes_5_Array_t_DATADICTIONARY);
    rc = EdsLib_Lookup_SubMemberByIndex(&GD_BASIC, MsgId, 4, &CompInfo);
    UtAssert_Simple(rc == EDSLIB_SUCCESS);
    UtAssert_True(CompInfo.AbsOffset == 4 * sizeof(UT1_BasicDataTypes_t), "EdsLib_Lookup_SubMemberByOffset(%x) (%u) == %u",
            (unsigned int)MsgId, (unsigned int)CompInfo.AbsOffset, 4 * (unsigned int)sizeof(UT1_BasicDataTypes_t));
    i = EdsLib_Get_AppIdx(CompInfo.StructureId);
    UtAssert_True(i == UTM_EDS_UT1_INDEX, "EdsLib_Get_AppIdx(%x) (%u) == UTM_EDS_UT1_INDEX",
            (unsigned int)CompInfo.StructureId, (unsigned int)i);
    i = EdsLib_Get_FormatIdx(CompInfo.StructureId);
    UtAssert_True(i == UT1_BasicDataTypes_t_DATADICTIONARY, "EdsLib_Get_FormatIdx(%x) (%u) == UT1_BasicDataTypes_t_DATADICTIONARY",
            (unsigned int)CompInfo.StructureId, (unsigned int)i);

    MsgId = EDSLIB_MAKE_ID(UTM_EDS_UT1_INDEX, UT1_BasicDataTypes_t_DATADICTIONARY);
    rc = EdsLib_DataTypeDB_GetTypeInfo(&GD_BASIC, MsgId, &TypeInfo);
    UtAssert_Simple(rc == EDSLIB_SUCCESS);
    UtAssert_True(TypeInfo.ElemType == EDSLIB_BASICTYPE_CONTAINER, "ElemType (%u) == EDSLIB_BASICTYPE_CONTAINER", (unsigned int)TypeInfo.ElemType);
    UtAssert_True(TypeInfo.NumSubElements == 10, "NumSubElements (%u) == 10", (unsigned int)TypeInfo.NumSubElements);
    UtAssert_True(TypeInfo.Size.Bytes == sizeof(UT1_BasicDataTypes_t), "ByteSize (%u) == %u",
            (unsigned int)TypeInfo.Size.Bytes, (unsigned int)sizeof(UT1_BasicDataTypes_t));

    MsgId = EDSLIB_MAKE_ID(UTM_EDS_UT1_INDEX, UT1_BasicDataTypes_t_DATADICTIONARY);
    rc = EdsLib_Lookup_SubMemberByIndex(&GD_BASIC, MsgId, 6, &CompInfo);
    UtAssert_Simple(rc == EDSLIB_SUCCESS);
    UtAssert_True(i == 12, "EdsLib_Lookup_SubMemberByIndex(%x) (%u) == 1",
            (unsigned int)MsgId, (unsigned int)i);
    i = EdsLib_Get_AppIdx(CompInfo.StructureId);
    UtAssert_True(i == UTM_EDS_ut_base_types_INDEX, "EdsLib_Get_AppIdx(%x) (%u) == UTM_EDS_ut_base_types_INDEX",
            (unsigned int)CompInfo.StructureId, (unsigned int)i);
    i = EdsLib_Get_FormatIdx(CompInfo.StructureId);
    UtAssert_True(i == int32_Atom_t_DATADICTIONARY, "EdsLib_Get_FormatIdx(%x) (%u) == int32_Atom_t_DATADICTIONARY",
            (unsigned int)CompInfo.StructureId, (unsigned int)i);

    MsgId = EDSLIB_MAKE_ID(UTM_EDS_ut_base_types_INDEX, int32_Atom_t_DATADICTIONARY);
    rc = EdsLib_DataTypeDB_GetTypeInfo(&GD_BASIC, MsgId, &TypeInfo);
    UtAssert_Simple(rc == EDSLIB_SUCCESS);
    UtAssert_True(TypeInfo.ElemType == EDSLIB_BASICTYPE_SIGNED_INT, "ElemType (%u) == EDSLIB_BASICTYPE_SIGNED_INT", (unsigned int)TypeInfo.ElemType);
    UtAssert_True(TypeInfo.NumSubElements == 0, "NumSubElements (%u) == 0", (unsigned int)TypeInfo.NumSubElements);
    UtAssert_True(TypeInfo.Size.Bytes == 4, "ByteSize (%u) == 4", (unsigned int)TypeInfo.Size.Bytes);

    /* Test the dictionary walk function -- not worth doing this unless previous tests are working */

    WalkTestState.TestName = "InvalidShallow";
    WalkTestState.CurrentLine = Invalid_EXPECTED;
    EdsLib_Basic_Iterator_Dictionary(&GD_BASIC, EDSLIB_ID_INVALID, BasicCheckCallback, &WalkTestState);
    UtAssert_Simple(WalkTestState.CurrentLine == Invalid_EXPECTED);

    MsgId = EdsLib_Lookup_DerivedType(&GD_BASIC, EDSLIB_MAKE_ID(UTM_EDS_UTHDR_INDEX, UTHDR_BasePacket_Intf_t_DATADICTIONARY),
            0, UT1_TestTLM_MSG);
    UtAssert_Simple(MsgId != 0);
    memset(&WalkTestState, 0, sizeof(WalkTestState));
    WalkTestState.TestName = "PayloadShallow";
    WalkTestState.CurrentLine = PayloadShallow_EXPECTED;
    EdsLib_Basic_Iterator_Dictionary(&GD_BASIC, MsgId, BasicCheckCallback, &WalkTestState);
    UtAssert_Simple(*WalkTestState.CurrentLine == NULL);
    WalkTestState.TestName = "PayloadDeep";
    WalkTestState.DoDescend = true;
    WalkTestState.CurrentLine = PayloadDeep_EXPECTED;
    EdsLib_Basic_Iterator_Dictionary(&GD_BASIC, MsgId, BasicCheckCallback, &WalkTestState);
    UtAssert_Simple(*WalkTestState.CurrentLine == NULL);
    WalkTestState.TestName = "PayloadStop";
    WalkTestState.DoStop = true;
    WalkTestState.CurrentLine = PayloadStop_EXPECTED;
    EdsLib_Basic_Iterator_Dictionary(&GD_BASIC, MsgId, BasicCheckCallback, &WalkTestState);
    UtAssert_Simple(*WalkTestState.CurrentLine == NULL);


    /* Test the unregistration calls */

    rc = EdsLib_Basic_AppDict_Unregister(&GD_BASIC, UTM_EDS_UT1_INDEX);
    UtAssert_Simple(rc == EDSLIB_SUCCESS);

    MsgId = EDSLIB_MAKE_ID(UTM_EDS_UT1_INDEX, UT1_BasicDataTypes_t_DATADICTIONARY);
    rc = EdsLib_DataTypeDB_GetTypeInfo(&GD_BASIC, MsgId, &TypeInfo);
    UtAssert_Simple(rc != EDSLIB_SUCCESS);
    UtAssert_Simple(TypeInfo.ElemType == EDSLIB_BASICTYPE_NONE);
    MsgId = EDSLIB_MAKE_ID(UTM_EDS_UTHDR_INDEX, UTHDR_UtPriHdr_t_DATADICTIONARY);
    rc = EdsLib_DataTypeDB_GetTypeInfo(&GD_BASIC, MsgId, &TypeInfo);
    UtAssert_Simple(rc == EDSLIB_SUCCESS);
    UtAssert_Simple(TypeInfo.ElemType == EDSLIB_BASICTYPE_CONTAINER);
    MsgId = EDSLIB_MAKE_ID(UTM_EDS_ut_base_types_INDEX, int32_Atom_t_DATADICTIONARY);
    rc = EdsLib_DataTypeDB_GetTypeInfo(&GD_BASIC, MsgId, &TypeInfo);
    UtAssert_Simple(rc == EDSLIB_SUCCESS);
    UtAssert_Simple(TypeInfo.ElemType == EDSLIB_BASICTYPE_SIGNED_INT);

    rc = EdsLib_Basic_AppDict_Unregister(&GD_BASIC, UTM_EDS_UTHDR_INDEX);
    UtAssert_Simple(rc == EDSLIB_SUCCESS);

    MsgId = EDSLIB_MAKE_ID(UTM_EDS_UTHDR_INDEX, UTHDR_UtPriHdr_t_DATADICTIONARY);
    rc = EdsLib_DataTypeDB_GetTypeInfo(&GD_BASIC, MsgId, &TypeInfo);
    UtAssert_Simple(rc != EDSLIB_SUCCESS);
    UtAssert_Simple(TypeInfo.ElemType == EDSLIB_BASICTYPE_NONE);
    MsgId = EDSLIB_MAKE_ID(UTM_EDS_ut_base_types_INDEX, int32_Atom_t_DATADICTIONARY);
    rc = EdsLib_DataTypeDB_GetTypeInfo(&GD_BASIC, MsgId, &TypeInfo);
    UtAssert_Simple(rc == EDSLIB_SUCCESS);
    UtAssert_Simple(TypeInfo.ElemType == EDSLIB_BASICTYPE_SIGNED_INT);

}
