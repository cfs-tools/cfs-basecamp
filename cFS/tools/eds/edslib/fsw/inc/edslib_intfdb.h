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
 * \file     edslib_intfdb.h
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Defines a runtime API to obtain information about
 * the interfaces defined within EDS EDS.
 */

#ifndef EDSLIB_INTFDB_H
#define EDSLIB_INTFDB_H

/*
 * The EDS library uses the fixed-width types from the C99 stdint.h header file,
 * rather than the OSAL types.  This allows use in applications that are not
 * based on OSAL.
 */
#include <stdint.h>
#include <stddef.h>

#include <edslib_id.h>
#include <edslib_api_types.h>

/******************************
 * MACROS
 ******************************/

/******************************
 * TYPEDEFS
 ******************************/

typedef struct EdsLib_IntfDB_DatabaseObject EdsLib_IntfDB_DatabaseObject_t;

struct EdsLib_IntfDB_InterfaceInfo
{
    const char *InterfaceName;
    EdsLib_Id_t IntfTypeEdsId;
    EdsLib_Id_t ParentCompEdsId;
    uint16_t    GenericTypeMapCount;
};

typedef struct EdsLib_IntfDB_InterfaceInfo EdsLib_IntfDB_InterfaceInfo_t;

struct EdsLib_IntfDB_ComponentInfo
{
    const char *ComponentName;
    uint16_t    RequiredIntfCount;
};

typedef struct EdsLib_IntfDB_ComponentInfo EdsLib_IntfDB_ComponentInfo_t;

struct EdsLib_IntfDB_CommandInfo
{
    const char *CommandName;
    EdsLib_Id_t ParentDeclIntfId;
    uint16_t    ArgumentCount;
};

typedef struct EdsLib_IntfDB_CommandInfo EdsLib_IntfDB_CommandInfo_t;

/******************************
 * API CALLS
 ******************************/

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Initialize the Interface DB
     *
     * Normally applications should not invoke this directly, as the library is normally initialized via
     * the combined EdsLib_Initialize() function. The subsystem-specific init is declared here for testing
     * purposes.
     */
    void EdsLib_IntfDB_Initialize(void);

    /**
     * Find an EDS component by name
     *
     * This looks up by the non-qualified / bare name.  The containing package must already be known.
     *
     * @param[in]  GD the active EdsLib runtime database object
     * @param[in]  CompName the name of the component to find, in the form PACKAGE/COMPONENT
     * @param[out] IdBuffer output buffer for the Eds ID, if successful
     * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
     */
    int32_t EdsLib_IntfDB_FindComponentByLocalName(const EdsLib_DatabaseObject_t *GD,
                                                   uint16_t                       AppIdx,
                                                   const char                    *CompName,
                                                   EdsLib_Id_t                   *IdBuffer);

    /**
     * Find an EDS declared interface by name
     *
     * This looks up by the non-qualified / bare name.  The containing package must already be known.
     *
     * @param[in]  GD the active EdsLib runtime database object
     * @param[in]  AppIdx the application index, either a fixed value or looked up via EdsLib_FindPackageIdxByName()
     * @param[in]  IntfName the name of the interface to find
     * @param[out] IdBuffer output buffer for the Eds ID, if successful
     * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
     */
    int32_t EdsLib_IntfDB_FindDeclaredInterfaceByLocalName(const EdsLib_DatabaseObject_t *GD,
                                                           uint16_t                       AppIdx,
                                                           const char                    *IntfName,
                                                           EdsLib_Id_t                   *IdBuffer);

    /**
     * Find an EDS declared interface by name
     *
     * This version expects a fully qualified SEDS name in the form of PACKAGE/INTERFACE
     *
     * @param[in]  GD the active EdsLib runtime database object
     * @param[in]  IntfName the interface name to find
     * @param[out] IdBuffer output buffer for the Eds ID, if successful
     * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
     */
    int32_t EdsLib_IntfDB_FindDeclaredInterfaceByFullName(const EdsLib_DatabaseObject_t *GD,
                                                          const char                    *IntfName,
                                                          EdsLib_Id_t                   *IdBuffer);

    /**
     * Find an EDS component (utilized) interface by its localized name
     *
     * This specifically refers to a "provided" or "required" interface in a component declaration, where
     * a declared interface facilitates bridging components together.
     *
     * As these interface names are localized to the component they are associated with, one must first find the
     * component, and then find the interface on that component.
     *
     * @param[in]  GD the active EdsLib runtime database object
     * @param[in]  ComponentId the component identifier, from e.g. EdsLib_IntfDB_FindComponentByLocalName()
     * @param[in]  IntfName the interface name to find
     * @param[out] IdBuffer output buffer for the Eds ID, if successful
     * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
     */
    int32_t EdsLib_IntfDB_FindComponentInterfaceByLocalName(const EdsLib_DatabaseObject_t *GD,
                                                            EdsLib_Id_t                    ComponentId,
                                                            const char                    *IntfName,
                                                            EdsLib_Id_t                   *IdBuffer);

    /**
     * Find an EDS component (utilized) interface by fully qualified name
     *
     * This specifically refers to a "provided" or "required" interface in a component declaration, where
     * a declared interface facilitates bridging components together.
     *
     * This version expects a fully qualified SEDS name in the form of PACKAGE/COMPONENT/INTERFACE
     *
     * @param[in]  GD the active EdsLib runtime database object
     * @param[in]  IntfName the interface name to find
     * @param[out] IdBuffer output buffer for the Eds ID, if successful
     * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
     */
    int32_t EdsLib_IntfDB_FindComponentInterfaceByFullName(const EdsLib_DatabaseObject_t *GD,
                                                           const char                    *IntfName,
                                                           EdsLib_Id_t                   *IdBuffer);

    /**
     * Find an EDS command by name
     *
     * Commands are declared within an interface declaration, such as "send" and "receive" for message
     * interfaces, or "load" and "save" for file interfaces, and so forth.
     *
     * As command names are localized to the interface they are associated with, one must first find the
     * declared interface, and then find a command on that interface.
     *
     * @param[in]  GD the active EdsLib runtime database object
     * @param[in]  IntfId the declared interface identifier, from e.g. EdsLib_IntfDB_FindDeclaredInterfaceByLocalName()
     * @param[in]  CmdName the command name to find
     * @param[out] IdBuffer output buffer for the Eds ID, if successful
     * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
     */
    int32_t EdsLib_IntfDB_FindCommandByLocalName(const EdsLib_DatabaseObject_t *GD,
                                                 EdsLib_Id_t                    IntfId,
                                                 const char                    *CmdName,
                                                 EdsLib_Id_t                   *IdBuffer);

    /**
     * Gets the EdsId for all EDS-defined commands within an interface
     *
     * Commands are declared within an interface declaration, such as "send" and "receive" for message
     * interfaces, or "load" and "save" for file interfaces, and so forth.
     *
     * This function gets a list of all commands within an interface and returns the set via the output
     * Id buffer array.
     *
     * @param[in]  GD the active EdsLib runtime database object
     * @param[in]  IntfId the declared interface identifier, from e.g. EdsLib_IntfDB_FindDeclaredInterfaceByLocalName()
     * @param[out] IdBuffer output buffers for the Eds ID, if successful
     * @param[in]  NumIdBufs the number of buffers in the IdBuffer array
     * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
     */
    int32_t EdsLib_IntfDB_FindAllCommands(const EdsLib_DatabaseObject_t *GD,
                                          EdsLib_Id_t                    IntfId,
                                          EdsLib_Id_t                   *IdBuffer,
                                          size_t                         NumIdBufs);

    /**
     * Map all command arguments to their actual data type
     *
     * This combines the information from the command definition (within the declared interface) with
     * information from the component definition to determine the actual data type for a command argument.
     *
     * Note that command arguments are often defined in a generic type, and that generic type is mapped
     * to a real type when the interface is realized within a component.  This function finds that mapping
     * from a generic type to a real data type.
     *
     * As commands can have multiple arguments, the output buffer should be an array that is sized
     * appropriately for the number of arguments on the given command.
     *
     * The output IDs from this routine are data type identifiers and may be used with the DataTypeDB
     * functions.
     *
     * @param[in]  GD the active EdsLib runtime database object
     * @param[in]  CmdEdsId the command identifier, from e.g. EdsLib_IntfDB_FindCommandByLocalName()
     * @param[in]  CompIntfEdsId the interface identifier, from e.g. EdsLib_IntfDB_FindComponentInterfaceByLocalName()
     * @param[out] IdBuffer output buffers for the Eds ID, if successful
     * @param[in]  NumIdBufs the number of buffers in the IdBuffer array
     * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
     */
    int32_t EdsLib_IntfDB_FindAllArgumentTypes(const EdsLib_DatabaseObject_t *GD,
                                               EdsLib_Id_t                    CmdEdsId,
                                               EdsLib_Id_t                    CompIntfEdsId,
                                               EdsLib_Id_t                   *IdBuffer,
                                               size_t                         NumIdBufs);

    /**
     * Get details on an EDS declared interface
     *
     * @param[in]  GD the active EdsLib runtime database object
     * @param[in]  IntfId the interface identifier, from e.g. EdsLib_IntfDB_FindDeclaredInterfaceByLocalName()
     * @param[out] IntfInfoBuffer output buffer for the information, if successful
     * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
     */
    int32_t EdsLib_IntfDB_GetComponentInterfaceInfo(const EdsLib_DatabaseObject_t *GD,
                                                    EdsLib_Id_t                    IntfId,
                                                    EdsLib_IntfDB_InterfaceInfo_t *IntfInfoBuffer);

    /**
     * Get details on an EDS component
     *
     * @param[in]  GD the active EdsLib runtime database object
     * @param[in]  ComponentId the component identifier
     * @param[out] CompInfoBuffer output buffer for the component information, if successful
     * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
     */
    int32_t EdsLib_IntfDB_GetComponentInfo(const EdsLib_DatabaseObject_t *GD,
                                           EdsLib_Id_t                    ComponentId,
                                           EdsLib_IntfDB_ComponentInfo_t *CompInfoBuffer);

    /**
     * Get full name of an EDS entity
     *
     * This assembles a fully-qualified name starting with a package name, in the
     * form of PACKAGE/COMPONENT/INTERFACE
     *
     * @param[in]  GD the active EdsLib runtime database object
     * @param[in]  EdsId the entity identifier
     * @param[out] BufferPtr output buffer for the component name, if successful
     * @param[in]  BufferLen length of the output buffer
     * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
     */
    int32_t
    EdsLib_IntfDB_GetFullName(const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, char *BufferPtr, size_t BufferLen);

    /**
     * Get details on an EDS command
     *
     * @param[in]  GD the active EdsLib runtime database object
     * @param[in]  CommandId the command identifier
     * @param[out] CommandInfoBuffer output buffer for the component information, if successful
     * @return EDSLIB_SUCCESS if successful, error code if unsuccessful
     */
    int32_t EdsLib_IntfDB_GetCommandInfo(const EdsLib_DatabaseObject_t *GD,
                                         EdsLib_Id_t                    CommandId,
                                         EdsLib_IntfDB_CommandInfo_t   *CommandInfoBuffer);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* EDSLIB_INTFDB_H */
