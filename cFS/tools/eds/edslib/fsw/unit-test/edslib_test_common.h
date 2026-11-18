#ifndef EDSLIB_TEST_H
#define EDSLIB_TEST_H

#include "edslib_id.h"
#include "edslib_api_types.h"
#include "edslib_global.h"

#include "ut_eds_master_index.h"
#include "uthdr_eds_datatypes.h"
#include "utapp_eds_datatypes.h"

extern const EdsLib_Id_t UT_EDS_UINT32_ID;
extern const EdsLib_Id_t UT_EDS_INT8_ID;
extern const EdsLib_Id_t UT_EDS_UINT8_ID;
extern const EdsLib_Id_t UT_EDS_ENUM_ID;
extern const EdsLib_Id_t UT_EDS_CMD1_ID;
extern const EdsLib_Id_t UT_EDS_CMDHDR_ID;
extern const EdsLib_Id_t UT_EDS_COMPONENT_ID;
extern const EdsLib_Id_t UT_EDS_CMDDECL_ID;
extern const EdsLib_Id_t UT_EDS_CMDINTF_ID;
extern const EdsLib_Id_t UT_EDS_RECVCMD_ID;

extern const struct EdsLib_App_DataTypeDB EDS_PACKAGE_UTAPP_DATATYPE_DB;
extern const struct EdsLib_App_DataTypeDB EDS_PACKAGE_UTHDR_DATATYPE_DB;
extern const struct EdsLib_App_DataTypeDB EDS_PACKAGE_CCSDS_SOIS_SEDS_DATATYPE_DB;

extern const EdsLib_DatabaseObject_t UT_DATABASE;

#endif
