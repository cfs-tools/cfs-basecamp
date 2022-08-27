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
 * \file     seds_tree_node.h
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation of the tree node module.
 * For module description see seds_tree_node.c
 */

#ifndef _SEDS_TREE_NODE_H_
#define _SEDS_TREE_NODE_H_

#include "seds_global.h"
#include "seds_checksum.h"

/**
 * Node types used in DOM tree nodes
 *
 * @note  The order of tags in this enum matters --
 * macros are defined for ranges to determine the basic class of
 * an element.  The ranges are designated by the "_FIRST/_LAST" entries.
 *
 * See macros below for the range checking options
 */
typedef enum
{
    /*
     * Any "unknown" nodetype should everything within it.
     * This can be used to discard any element type which is
     * unsupported.
     */

   SEDS_NODETYPE_UNKNOWN,                 /**< Reserved value - for nodes that are ignored */
   SEDS_NODETYPE_ROOT,                    /**< Root node of entire heirarchy */
   SEDS_NODETYPE_CCSDS_STANDARD_FIRST,    /**< Reserved index marker - not used */

   /* Basic EDS node types */
   SEDS_NODETYPE_DATASHEET,               /**< Opening tag for SEDS datasheet */
   SEDS_NODETYPE_PACKAGEFILE,             /**< Opening tag for SEDS packagefile */
   SEDS_NODETYPE_PACKAGE,                 /**< Package definition */
   SEDS_NODETYPE_LONG_DESCRIPTION,        /**< Long description (documentation) node */
   SEDS_NODETYPE_SEMANTICS,               /**< Extra usage semantics that are beyond EDS scope (TBD) */

   /*
    * SET and LIST EDS nodes: these are structural from the xml file and only exist
    * to contain other nodes of a specific type
    */
   SEDS_NODETYPE_SET_NODE_FIRST,          /**< Reserved index marker - not used */
   SEDS_NODETYPE_DATA_TYPE_SET,           /**< DataTypeSet node */
   SEDS_NODETYPE_BASE_INTERFACE_SET,
   SEDS_NODETYPE_GENERIC_TYPE_SET,
   SEDS_NODETYPE_PARAMETER_SET,
   SEDS_NODETYPE_COMMAND_SET,
   SEDS_NODETYPE_GENERIC_TYPE_MAP_SET,
   SEDS_NODETYPE_CONSTRAINT_SET,
   SEDS_NODETYPE_DECLARED_INTERFACE_SET,
   SEDS_NODETYPE_PROVIDED_INTERFACE_SET,
   SEDS_NODETYPE_REQUIRED_INTERFACE_SET,
   SEDS_NODETYPE_COMPONENT_SET,
   SEDS_NODETYPE_ALTERNATE_SET,
   SEDS_NODETYPE_PARAMETER_MAP_SET,
   SEDS_NODETYPE_VARIABLE_SET,
   SEDS_NODETYPE_ACTIVITY_SET,
   SEDS_NODETYPE_METADATA_VALUE_SET,
   SEDS_NODETYPE_NOMINAL_RANGE_SET,
   SEDS_NODETYPE_PARAMETER_ACTIVITY_MAP_SET,
   SEDS_NODETYPE_SAFE_RANGE_SET,
   SEDS_NODETYPE_STATE_MACHINE_SET,
   SEDS_NODETYPE_SET_NODE_LAST,         /**< Reserved index marker - not used */

   SEDS_NODETYPE_COMPONENT,
   SEDS_NODETYPE_COMMAND,
   SEDS_NODETYPE_IMPLEMENTATION,
   SEDS_NODETYPE_ALTERNATE,
   SEDS_NODETYPE_PARAMETER,
   SEDS_NODETYPE_ARGUMENT,
   SEDS_NODETYPE_GENERIC_TYPE_MAP,
   SEDS_NODETYPE_PARAMETER_MAP,
   SEDS_NODETYPE_VARIABLE,

   SEDS_NODETYPE_CONTAINER_ENTRY_LIST,    /**< EntryList node (container types) */
   SEDS_NODETYPE_CONTAINER_TRAILER_ENTRY_LIST,  /**< TrailerEntryList node (container types) */
   SEDS_NODETYPE_DIMENSION_LIST,          /**< DimensionList node (array types) */
   SEDS_NODETYPE_ENUMERATION_LIST,        /**< EnumerationList node (enumerated types) */

   /* Data type definition EDS nodes */
   SEDS_NODETYPE_SCALAR_DATATYPE_FIRST,   /**< Reserved index marker - not used */
   SEDS_NODETYPE_INTEGER_DATATYPE,        /**< Integer Data type */
   SEDS_NODETYPE_FLOAT_DATATYPE,          /**< Floating Point Data type */
   SEDS_NODETYPE_ENUMERATION_DATATYPE,    /**< Enumerated Data type */
   SEDS_NODETYPE_BINARY_DATATYPE,         /**< Binary Blob Data type */
   SEDS_NODETYPE_STRING_DATATYPE,         /**< String Data type */
   SEDS_NODETYPE_BOOLEAN_DATATYPE,        /**< Boolean Data type */
   SEDS_NODETYPE_SUBRANGE_DATATYPE,       /**< SubRange Data type */
   SEDS_NODETYPE_SCALAR_DATATYPE_LAST,    /**< Reserved index marker - not used */

   SEDS_NODETYPE_COMPOUND_DATATYPE_FIRST, /**< Reserved index marker - not used */
   SEDS_NODETYPE_ARRAY_DATATYPE,          /**< Array Data type */
   SEDS_NODETYPE_CONTAINER_DATATYPE,      /**< Container Data type */
   SEDS_NODETYPE_COMPOUND_DATATYPE_LAST,  /**< Reserved index marker - not used */

   SEDS_NODETYPE_DYNAMIC_DATATYPE_FIRST,  /**< Reserved index marker - not used */
   SEDS_NODETYPE_GENERIC_TYPE,            /**< Generic Type (deferred definition) */
   SEDS_NODETYPE_DYNAMIC_DATATYPE_LAST,   /**< Reserved index marker - not used */

   SEDS_NODETYPE_INTERFACE_FIRST,         /**< Reserved index marker - not used */
   SEDS_NODETYPE_INTERFACE,
   SEDS_NODETYPE_DECLARED_INTERFACE,
   SEDS_NODETYPE_PROVIDED_INTERFACE,
   SEDS_NODETYPE_REQUIRED_INTERFACE,
   SEDS_NODETYPE_BASE_INTERFACE,
   SEDS_NODETYPE_INTERFACE_LAST,          /**< Reserved index marker - not used */

   SEDS_NODETYPE_ENUMERATION_ENTRY,             /**< Enumeration label node (enumerated types) */
   SEDS_NODETYPE_CONTAINER_ENTRY,               /**< Entry node (container types) */
   SEDS_NODETYPE_CONTAINER_FIXED_VALUE_ENTRY,    /**< Entry node (container types) */
   SEDS_NODETYPE_CONTAINER_PADDING_ENTRY,       /**< Entry node (container types) */
   SEDS_NODETYPE_CONTAINER_LIST_ENTRY,          /**< Entry node (container types) */
   SEDS_NODETYPE_CONTAINER_LENGTH_ENTRY,        /**< Entry node (container types) */
   SEDS_NODETYPE_CONTAINER_ERROR_CONTROL_ENTRY, /**< Entry node (container types) */

   /* Contstraint nodes */
   SEDS_NODETYPE_CONSTRAINT_FIRST,        /**< Reserved index marker - not used */
   SEDS_NODETYPE_CONSTRAINT,
   SEDS_NODETYPE_TYPE_CONSTRAINT,
   SEDS_NODETYPE_RANGE_CONSTRAINT,
   SEDS_NODETYPE_VALUE_CONSTRAINT,
   SEDS_NODETYPE_CONSTRAINT_LAST,         /**< Reserved index marker - not used */

   /* Encoding specifiers */
   SEDS_NODETYPE_ENCODING_FIRST,          /**< Reserved index marker - not used */
   SEDS_NODETYPE_INTEGER_DATA_ENCODING,
   SEDS_NODETYPE_FLOAT_DATA_ENCODING,
   SEDS_NODETYPE_STRING_DATA_ENCODING,
   SEDS_NODETYPE_ENCODING_LAST,           /**< Reserved index marker - not used */

   /* Range specifiers */
   SEDS_NODETYPE_RANGE_FIRST,             /**< Reserved index marker - not used */
   SEDS_NODETYPE_MINMAX_RANGE,
   SEDS_NODETYPE_PRECISION_RANGE,
   SEDS_NODETYPE_ENUMERATED_RANGE,
   SEDS_NODETYPE_RANGE_LAST,              /**< Reserved index marker - not used */

   SEDS_NODETYPE_RANGE,
   SEDS_NODETYPE_VALID_RANGE,

   /* Dimension specifiers */
   SEDS_NODETYPE_DIMENSION,               /**< Dimension node (array types) */
   SEDS_NODETYPE_ARRAY_DIMENSIONS,

   /* Calibration specifiers */
   SEDS_NODETYPE_SPLINE_CALIBRATOR,       /**< Spline calibrator */
   SEDS_NODETYPE_POLYNOMIAL_CALIBRATOR,   /**< Polynomial calibrator */
   SEDS_NODETYPE_SPLINE_POINT,            /**< Spline point */
   SEDS_NODETYPE_POLYNOMIAL_TERM,         /**< Polynomial term */

   /* Other SEDS XML elements */
   /* (These are acceptable per SEDS schema but may or may not be utilized) */
   SEDS_NODETYPE_ACTIVITY,
   SEDS_NODETYPE_ANDED_CONDITIONS,
   SEDS_NODETYPE_ARGUMENT_VALUE,
   SEDS_NODETYPE_ARRAY_DATA_TYPE,
   SEDS_NODETYPE_ASSIGNMENT,
   SEDS_NODETYPE_BODY,
   SEDS_NODETYPE_BOOLEAN_DATA_ENCODING,
   SEDS_NODETYPE_CALIBRATION,
   SEDS_NODETYPE_CALL,
   SEDS_NODETYPE_CATEGORY,
   SEDS_NODETYPE_COMPARISON_OPERATOR,
   SEDS_NODETYPE_CONDITIONAL,
   SEDS_NODETYPE_CONDITION,
   SEDS_NODETYPE_DATE_VALUE,
   SEDS_NODETYPE_DEVICE,
   SEDS_NODETYPE_DO,
   SEDS_NODETYPE_END_AT,
   SEDS_NODETYPE_ENTRY_STATE,
   SEDS_NODETYPE_EXIT_STATE,
   SEDS_NODETYPE_FIRST_OPERAND,
   SEDS_NODETYPE_FLOAT_VALUE,
   SEDS_NODETYPE_GET_ACTIVITY,
   SEDS_NODETYPE_GUARD,
   SEDS_NODETYPE_INTEGER_VALUE,
   SEDS_NODETYPE_ITERATION,
   SEDS_NODETYPE_LABEL,
   SEDS_NODETYPE_MATH_OPERATION,
   SEDS_NODETYPE_METADATA,
   SEDS_NODETYPE_ON_COMMAND_PRIMITIVE,
   SEDS_NODETYPE_ON_CONDITION_FALSE,
   SEDS_NODETYPE_ON_CONDITION_TRUE,
   SEDS_NODETYPE_ON_ENTRY,
   SEDS_NODETYPE_ON_EXIT,
   SEDS_NODETYPE_ON_PARAMETER_PRIMITIVE,
   SEDS_NODETYPE_ON_TIMER,
   SEDS_NODETYPE_OPERATOR,
   SEDS_NODETYPE_ORED_CONDITIONS,
   SEDS_NODETYPE_OVER_ARRAY,
   SEDS_NODETYPE_PARAMETER_ACTIVITY_MAP,
   SEDS_NODETYPE_PROVIDED,
   SEDS_NODETYPE_REQUIRED,
   SEDS_NODETYPE_SECOND_OPERAND,
   SEDS_NODETYPE_SEMANTICS_TERM,
   SEDS_NODETYPE_SEND_COMMAND_PRIMITIVE,
   SEDS_NODETYPE_SEND_PARAMETER_PRIMITIVE,
   SEDS_NODETYPE_SET_ACTIVITY_ONLY,
   SEDS_NODETYPE_SET_ACTIVITY,
   SEDS_NODETYPE_START_AT,
   SEDS_NODETYPE_STATE_MACHINE,
   SEDS_NODETYPE_STATE,
   SEDS_NODETYPE_STEP,
   SEDS_NODETYPE_STRING_VALUE,
   SEDS_NODETYPE_TRANSITION,
   SEDS_NODETYPE_TYPE_CONDITION,
   SEDS_NODETYPE_TYPE_OPERAND,
   SEDS_NODETYPE_VALUE,
   SEDS_NODETYPE_VARIABLE_REF,

   SEDS_NODETYPE_CCSDS_STANDARD_LAST,     /**< Reserved index marker - not used */

   /*
    * ----------------------------------------------------------------------------------
    * Additional node types specific to this NASA cFS toolchain implementation
    *
    * These provide extra necessary information that the CCSDS SEDS specification currently does not cover.
    * These elements would generally be stripped out of XML files that were intended to be CCSDS SEDS-compliant,
    * and/or instantiated via supplemental XML files that are specific to this tool chain.
    * ----------------------------------------------------------------------------------
    */

   SEDS_NODETYPE_XINCLUDE_PASSTHRU,       /**< For Xinclude elements - ignored by this tool, suppresses warnings */
   SEDS_NODETYPE_DESCRIPTION_PASSTHRU,    /**< Long description (documentation) continuation/pass-through node */
   SEDS_NODETYPE_DESIGN_PARAMETERS,       /**< Opening tag for SEDS design-time parameter sheet (supplements the data sheet)  */
   SEDS_NODETYPE_DEFINE,                  /**< Equate text name to value during pre-processing */
   SEDS_NODETYPE_INSTANCE_RULE_SET,
   SEDS_NODETYPE_INSTANCE_RULE,
   SEDS_NODETYPE_INTERFACE_MAP_SET,
   SEDS_NODETYPE_INTERFACE_MAP,
   SEDS_NODETYPE_PARAMETER_VALUE,

   SEDS_NODETYPE_MAX
} seds_nodetype_t;

#define SEDSTAG_IN_RANGE(typ,range)      (((uint32_t)(typ)) > SEDS_NODETYPE_##range##_FIRST && ((uint32_t)(typ)) < SEDS_NODETYPE_##range##_LAST)
#define SEDSTAG_IS_SCALAR_DATATYPE(t)    SEDSTAG_IN_RANGE(t, SCALAR_DATATYPE)
#define SEDSTAG_IS_COMPOUND_DATATYPE(t)  SEDSTAG_IN_RANGE(t, COMPOUND_DATATYPE)
#define SEDSTAG_IS_DYNAMIC_DATATYPE(t)   SEDSTAG_IN_RANGE(t, DYNAMIC_DATATYPE)
#define SEDSTAG_IS_NORMAL_DATATYPE(t)    (SEDSTAG_IS_SCALAR_DATATYPE(t) || SEDSTAG_IS_COMPOUND_DATATYPE(t))
#define SEDSTAG_IS_ANY_DATATYPE(t)       (SEDSTAG_IS_SCALAR_DATATYPE(t) || SEDSTAG_IS_COMPOUND_DATATYPE(t) || SEDSTAG_IS_DYNAMIC_DATATYPE(t))
#define SEDSTAG_IS_CONSTRAINT(t)         SEDSTAG_IN_RANGE(t, CONSTRAINT)
#define SEDSTAG_IS_ENCODING(t)           SEDSTAG_IN_RANGE(t, ENCODING)
#define SEDSTAG_IS_RANGE(t)              SEDSTAG_IN_RANGE(t, RANGE)
#define SEDSTAG_IS_INTERFACE(t)          SEDSTAG_IN_RANGE(t, INTERFACE)
#define SEDSTAG_IS_CCSDS_STANDARD(t)     SEDSTAG_IN_RANGE(t, CCSDS_STANDARD)
#define SEDSTAG_IS_VALID(t)              ((t) != SEDS_NODETYPE_UNKNOWN && ((uint32_t)(t)) < SEDS_NODETYPE_MAX)

/**
 * Basic DOM Node object
 *
 * In the native C object this only indicates the type of node.  All other metadata
 * about the node is stored in the associated Lua userdata.  It is implemented this
 * way primarily to take advantage of Lua metatable-based type checking i.e.
 * luaL_checkudata() that is typical of userdata objects.
 */
typedef struct
{
    seds_nodetype_t node_type;
}  seds_node_t;

/**
 * Push a new node onto the Lua stack.
 *
 * A new "seds_node" userdata object is created and the metadata is set accordingly,
 * and the object is pushed onto the Lua stack.
 */
void seds_tree_node_push_new_object(lua_State *lua, seds_nodetype_t node_type);

/**
 * Obtain the type of a node object on the Lua stack
 *
 * The Lua stack at the indicated position should contain a seds_node userdata object.
 * This retrieves the type of the object (the only metadata stored in the C struct) and
 * returns it.
 */
seds_nodetype_t seds_tree_node_get_type(lua_State *lua, int pos);

/**
 * Register the tree functions which are called from Lua
 * These are added to a table which is at the top of the stack
 */
void seds_tree_node_register_globals(lua_State *lua);


#endif  /* _SEDS_TREE_NODE_H_ */

