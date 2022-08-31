/*
** Purpose: Define the File Manager app's file system table
**
** Notes:
**   1. This is a refactor of NASA's File Manager (FM) app. The refactor includes
**      adaptation to the OSK app framework and prootyping the usage of an app 
**      init JSON file. The idea is to rethink whcih configuration paarameters
**      should be compile time and which should be runtime. 
**
** References:
**   1. OpenSatKit Object-based Application Developer's Guide.
**   2. cFS Application Developer's Guide.
**
**   Written by David McComas, licensed under the Apache License, Version 2.0
**   (the "License"); you may not use this file except in compliance with the
**   License. You may obtain a copy of the License at
**
**      http://www.apache.org/licenses/LICENSE-2.0
**
**   Unless required by applicable law or agreed to in writing, software
**   distributed under the License is distributed on an "AS IS" BASIS,
**   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**   See the License for the specific language governing permissions and
**   limitations under the License.
*/


#include "cfe.h"
#include "cfe_tbl_filedef.h"
#include "app_cfg.h"
#include "filesys.h"

/*
** Table header
*/


/*
** FM file system free space table data
**
** -- table entries must be enabled or disabled or unused
**
** -- enabled table entries may be disabled by command
** -- disabled table entries may be enabled by command
** -- unused table entries may not be modified by command
**
** -- enabled or disabled entries must have a valid file system name
**
** -- the file system name for unused entries is ignored
*/
FILESYS_TblData_t FILESYS_Tbl =
{
  {
    {                                   /* - 0 - */
        FILESYS_TBL_ENTRY_ENABLED,      /* Entry state (enabled, disabled, unused) */
        "/ram",                         /* File system name (logical mount point) */
    },
    {                                   /* - 1 - */
        FILESYS_TBL_ENTRY_DISABLED,     /* Entry state (enabled, disabled, unused) */
        "/boot",                        /* File system name (logical mount point) */
    },
    {                                   /* - 2 - */
        FILESYS_TBL_ENTRY_DISABLED,     /* Entry state (enabled, disabled, unused) */
        "/alt",                         /* File system name (logical mount point) */
    },
    {                                   /* - 3 - */
        FILESYS_TBL_ENTRY_UNUSED,       /* Entry state (enabled, disabled, unused) */
        "",                             /* File system name (logical mount point) */
    },
    {                                   /* - 4 - */
        FILESYS_TBL_ENTRY_UNUSED,       /* Entry state (enabled, disabled, unused) */
        "",                             /* File system name (logical mount point) */
    },
    {                                   /* - 5 - */
        FILESYS_TBL_ENTRY_UNUSED,       /* Entry state (enabled, disabled, unused) */
        "",                             /* File system name (logical mount point) */
    },
    {                                   /* - 6 - */
        FILESYS_TBL_ENTRY_UNUSED,       /* Entry state (enabled, disabled, unused) */
        "",                             /* File system name (logical mount point) */
    },
    {                                   /* - 7 - */
        FILESYS_TBL_ENTRY_UNUSED,       /* Entry state (enabled, disabled, unused) */
        "",                             /* File system name (logical mount point) */
    },
  },
};

/*
** cFE Table header
*/
CFE_TBL_FILEDEF(FILESYS_Tbl, FILE_MGR.FileSysTbl, File system volumes, filesys_tbl.tbl)

/*
static CFE_TBL_FileDef_t CFE_TBL_FileDef __attribute__((__used__)) =
{
    "FILESYS_Tbl", FILEMGR_APP_CFE_NAME "." FILEMGR_TBL_CFE_NAME,
    "File System Free Space", "filesys_tbl.tbl", sizeof(FILESYS_TblData_t)
};
*/
/* TODO - Macro chokes on FILESYS_* macro lengths 
#define FILESYS_TBL_NAME  (FILEMGR_INI_APP_NAME "." FILEMGR_INI_TBL_CFE_NAME)
CFE_TBL_FILEDEF(FILESYS_Tbl, FILEMGR_TBL_NAME, FILEMGR_INI_TBL_DEF_DESC, FILEMGR_INI_TBL_FILENAME)
*/
