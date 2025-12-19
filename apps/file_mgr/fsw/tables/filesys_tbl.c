/*
** Purpose: Define the File Manager app's file system table
**
** Notes:
**   1. This is a refactor of NASA's File Manager (FM) app. The refactor includes
**      adaptation to the app C framework and prootyping the usage of an app 
**      init JSON file. The idea is to rethink which configuration paarameters
**      should be compile time and which should be runtime. 
**
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
FILE_MGR_FileSysTbl_t FILESYS_Tbl =
{
  {
    {                                           /* - 0 - */
        FILE_MGR_FileSysTblEntryState_ENABLED,  /* Entry state (enabled, disabled, unused) */
        "/ram",                                 /* File system name (logical mount point) */
    },
    {                                           /* - 1 - */
        FILE_MGR_FileSysTblEntryState_DISABLED, /* Entry state (enabled, disabled, unused) */
        "/boot",                                /* File system name (logical mount point) */
    },
    {                                           /* - 2 - */
        FILE_MGR_FileSysTblEntryState_DISABLED, /* Entry state (enabled, disabled, unused) */
        "/alt",                                 /* File system name (logical mount point) */
    },
    {                                           /* - 3 - */
        FILE_MGR_FileSysTblEntryState_UNUSED,   /* Entry state (enabled, disabled, unused) */
        "",                                     /* File system name (logical mount point) */
    }
  }
};

/*
** cFE Table header
*/

CFE_TBL_FILEDEF(FILESYS_Tbl, FILE_MGR.FileSysTbl, File system volumes, filesys_tbl.tbl)
