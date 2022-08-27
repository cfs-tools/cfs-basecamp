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
 * \file     seds_xmlparser.c
 * \ingroup  tool
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implements the functions to read the contents of the SEDS XML data sheet into a
 * tree structure in memory (DOM tree).  The dependencies between the nodes in the final
 * tree structure can then be resolved, and the final data object model can be used
 * to write the output files.
 *
 * This implements the portion that reads the XML files using expat.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <errno.h>
#include <expat.h>
#include <ctype.h>
#include <stdarg.h>
#include <math.h>
#include <sys/stat.h>

#include "seds_global.h"
#include "seds_user_message.h"
#include "seds_xmlparser.h"
#include "seds_tree_node.h"

/*
 * Wrapper macro for XML character literals
 *
 * As long as XML_Char type == char type, then this is a pass through
 * If XML_Char is something different, this macro should provide a conversion
 */
#define XML_CHAR_C(x)           x


/*
 * Define the functions used for basic string ops on XML strings.
 * Allows override with custom / wide-char versions if needed.
 *
 * Default to C library implementations of these.
 *
 * (Use strcasecmp for case insensitivity during most operations)
 */
#define XML_strcmp              strcasecmp
#define XML_tolower             tolower

/**
 * Structure to map between character based XML tags and
 * enumerated tag ids used in the remainder of code
 */
typedef struct
{
    const XML_Char *tag_name;
    seds_nodetype_t tag_id;
} seds_stringmap_t;

/*
 * Mapping table for start tags - comes from the SOIS EDS blue book
 * Each defined XML tag should have an entry in this table and an associated enum label for that tag
 */
static const seds_stringmap_t XML_SEDS_STARTTAG_MAP[] =
{

        /* Element types defined by SEDS and supported by this toolchain */
        { .tag_name = XML_CHAR_C("ActivitySet"),            .tag_id = SEDS_NODETYPE_ACTIVITY_SET },
        { .tag_name = XML_CHAR_C("Activity"),               .tag_id = SEDS_NODETYPE_ACTIVITY },
        { .tag_name = XML_CHAR_C("AlternateSet"),           .tag_id = SEDS_NODETYPE_ALTERNATE_SET           },
        { .tag_name = XML_CHAR_C("Alternate"),              .tag_id = SEDS_NODETYPE_ALTERNATE               },
        { .tag_name = XML_CHAR_C("ANDedConditions"),        .tag_id = SEDS_NODETYPE_ANDED_CONDITIONS        },
        { .tag_name = XML_CHAR_C("Argument"),               .tag_id = SEDS_NODETYPE_ARGUMENT                },
        { .tag_name = XML_CHAR_C("ArgumentValue"),          .tag_id = SEDS_NODETYPE_ARGUMENT_VALUE          },
        { .tag_name = XML_CHAR_C("ArrayDataType"),          .tag_id = SEDS_NODETYPE_ARRAY_DATATYPE          },
        { .tag_name = XML_CHAR_C("ArrayDimensions"),        .tag_id = SEDS_NODETYPE_ARRAY_DIMENSIONS        },
        { .tag_name = XML_CHAR_C("Assignment"),             .tag_id = SEDS_NODETYPE_ASSIGNMENT              },
        { .tag_name = XML_CHAR_C("BaseInterfaceSet"),       .tag_id = SEDS_NODETYPE_BASE_INTERFACE_SET      },
        { .tag_name = XML_CHAR_C("BinaryDataType"),         .tag_id = SEDS_NODETYPE_BINARY_DATATYPE         },
        { .tag_name = XML_CHAR_C("Body"),                   .tag_id = SEDS_NODETYPE_BODY                    },
        { .tag_name = XML_CHAR_C("BooleanDataEncoding"),    .tag_id = SEDS_NODETYPE_BOOLEAN_DATA_ENCODING   },
        { .tag_name = XML_CHAR_C("BooleanDataType"),        .tag_id = SEDS_NODETYPE_BOOLEAN_DATATYPE        },
        { .tag_name = XML_CHAR_C("Calibration"),            .tag_id = SEDS_NODETYPE_CALIBRATION             },
        { .tag_name = XML_CHAR_C("Call"),                   .tag_id = SEDS_NODETYPE_CALL                    },
        { .tag_name = XML_CHAR_C("Category"),               .tag_id = SEDS_NODETYPE_CATEGORY                },
        { .tag_name = XML_CHAR_C("CommandSet"),             .tag_id = SEDS_NODETYPE_COMMAND_SET             },
        { .tag_name = XML_CHAR_C("Command"),                .tag_id = SEDS_NODETYPE_COMMAND                 },
        { .tag_name = XML_CHAR_C("ComparisonOperator"),     .tag_id = SEDS_NODETYPE_COMPARISON_OPERATOR     },
        { .tag_name = XML_CHAR_C("ComponentSet"),           .tag_id = SEDS_NODETYPE_COMPONENT_SET           },
        { .tag_name = XML_CHAR_C("Component"),              .tag_id = SEDS_NODETYPE_COMPONENT               },
        { .tag_name = XML_CHAR_C("Conditional"),            .tag_id = SEDS_NODETYPE_CONDITIONAL             },
        { .tag_name = XML_CHAR_C("Condition"),              .tag_id = SEDS_NODETYPE_CONDITION               },
        { .tag_name = XML_CHAR_C("ConstraintSet"),          .tag_id = SEDS_NODETYPE_CONSTRAINT_SET          },
        { .tag_name = XML_CHAR_C("ContainerDataType"),      .tag_id = SEDS_NODETYPE_CONTAINER_DATATYPE      },
        { .tag_name = XML_CHAR_C("DataSheet"),              .tag_id = SEDS_NODETYPE_DATASHEET               },
        { .tag_name = XML_CHAR_C("DataTypeSet"),            .tag_id = SEDS_NODETYPE_DATA_TYPE_SET           },
        { .tag_name = XML_CHAR_C("DateValue"),              .tag_id = SEDS_NODETYPE_DATE_VALUE              },
        { .tag_name = XML_CHAR_C("DeclaredInterfaceSet"),   .tag_id = SEDS_NODETYPE_DECLARED_INTERFACE_SET  },
        { .tag_name = XML_CHAR_C("Device"),                 .tag_id = SEDS_NODETYPE_DEVICE                  },
        { .tag_name = XML_CHAR_C("DimensionList"),          .tag_id = SEDS_NODETYPE_DIMENSION_LIST          },
        { .tag_name = XML_CHAR_C("Dimension"),              .tag_id = SEDS_NODETYPE_DIMENSION               },
        { .tag_name = XML_CHAR_C("Do"),                     .tag_id = SEDS_NODETYPE_DO                      },
        { .tag_name = XML_CHAR_C("EndAt"),                  .tag_id = SEDS_NODETYPE_END_AT                  },
        { .tag_name = XML_CHAR_C("EntryList"),              .tag_id = SEDS_NODETYPE_CONTAINER_ENTRY_LIST    },
        { .tag_name = XML_CHAR_C("EntryState"),             .tag_id = SEDS_NODETYPE_ENTRY_STATE             },
        { .tag_name = XML_CHAR_C("Entry"),                  .tag_id = SEDS_NODETYPE_CONTAINER_ENTRY         },
        { .tag_name = XML_CHAR_C("EnumeratedDataType"),     .tag_id = SEDS_NODETYPE_ENUMERATION_DATATYPE    },
        { .tag_name = XML_CHAR_C("EnumeratedRange"),        .tag_id = SEDS_NODETYPE_ENUMERATED_RANGE        },
        { .tag_name = XML_CHAR_C("EnumerationList"),        .tag_id = SEDS_NODETYPE_ENUMERATION_LIST        },
        { .tag_name = XML_CHAR_C("Enumeration"),            .tag_id = SEDS_NODETYPE_ENUMERATION_ENTRY       },
        { .tag_name = XML_CHAR_C("ErrorControlEntry"),      .tag_id = SEDS_NODETYPE_CONTAINER_ERROR_CONTROL_ENTRY   },
        { .tag_name = XML_CHAR_C("ExitState"),              .tag_id = SEDS_NODETYPE_EXIT_STATE              },
        { .tag_name = XML_CHAR_C("FirstOperand"),           .tag_id = SEDS_NODETYPE_FIRST_OPERAND           },
        { .tag_name = XML_CHAR_C("FixedValueEntry"),        .tag_id = SEDS_NODETYPE_CONTAINER_FIXED_VALUE_ENTRY      },
        { .tag_name = XML_CHAR_C("FloatDataEncoding"),      .tag_id = SEDS_NODETYPE_FLOAT_DATA_ENCODING     },
        { .tag_name = XML_CHAR_C("FloatDataType"),          .tag_id = SEDS_NODETYPE_FLOAT_DATATYPE          },
        { .tag_name = XML_CHAR_C("FloatValue"),             .tag_id = SEDS_NODETYPE_FLOAT_VALUE             },
        { .tag_name = XML_CHAR_C("GenericTypeMapSet"),      .tag_id = SEDS_NODETYPE_GENERIC_TYPE_MAP_SET    },
        { .tag_name = XML_CHAR_C("GenericTypeMap"),         .tag_id = SEDS_NODETYPE_GENERIC_TYPE_MAP        },
        { .tag_name = XML_CHAR_C("GenericTypeSet"),         .tag_id = SEDS_NODETYPE_GENERIC_TYPE_SET        },
        { .tag_name = XML_CHAR_C("GenericType"),            .tag_id = SEDS_NODETYPE_GENERIC_TYPE            },
        { .tag_name = XML_CHAR_C("GetActivity"),            .tag_id = SEDS_NODETYPE_GET_ACTIVITY            },
        { .tag_name = XML_CHAR_C("Guard"),                  .tag_id = SEDS_NODETYPE_GUARD                   },
        { .tag_name = XML_CHAR_C("Implementation"),         .tag_id = SEDS_NODETYPE_IMPLEMENTATION          },
        { .tag_name = XML_CHAR_C("IntegerDataEncoding"),    .tag_id = SEDS_NODETYPE_INTEGER_DATA_ENCODING   },
        { .tag_name = XML_CHAR_C("IntegerDataType"),        .tag_id = SEDS_NODETYPE_INTEGER_DATATYPE        },
        { .tag_name = XML_CHAR_C("IntegerValue"),           .tag_id = SEDS_NODETYPE_INTEGER_VALUE           },
        { .tag_name = XML_CHAR_C("Interface"),              .tag_id = SEDS_NODETYPE_INTERFACE               },
        { .tag_name = XML_CHAR_C("Iteration"),              .tag_id = SEDS_NODETYPE_ITERATION               },
        { .tag_name = XML_CHAR_C("Label"),                  .tag_id = SEDS_NODETYPE_LABEL                   },
        { .tag_name = XML_CHAR_C("LengthEntry"),            .tag_id = SEDS_NODETYPE_CONTAINER_LENGTH_ENTRY  },
        { .tag_name = XML_CHAR_C("ListEntry"),              .tag_id = SEDS_NODETYPE_CONTAINER_LIST_ENTRY    },
        { .tag_name = XML_CHAR_C("LongDescription"),        .tag_id = SEDS_NODETYPE_LONG_DESCRIPTION        },
        { .tag_name = XML_CHAR_C("MathOperation"),          .tag_id = SEDS_NODETYPE_MATH_OPERATION          },
        { .tag_name = XML_CHAR_C("Metadata"),               .tag_id = SEDS_NODETYPE_METADATA                },
        { .tag_name = XML_CHAR_C("MetadataValueSet"),       .tag_id = SEDS_NODETYPE_METADATA_VALUE_SET      },
        { .tag_name = XML_CHAR_C("MinMaxRange"),            .tag_id = SEDS_NODETYPE_MINMAX_RANGE            },
        { .tag_name = XML_CHAR_C("NominalRangeSet"),        .tag_id = SEDS_NODETYPE_NOMINAL_RANGE_SET       },
        { .tag_name = XML_CHAR_C("OnCommandPrimitive"),     .tag_id = SEDS_NODETYPE_ON_COMMAND_PRIMITIVE    },
        { .tag_name = XML_CHAR_C("OnConditionFalse"),       .tag_id = SEDS_NODETYPE_ON_CONDITION_FALSE      },
        { .tag_name = XML_CHAR_C("OnConditionTrue"),        .tag_id = SEDS_NODETYPE_ON_CONDITION_TRUE       },
        { .tag_name = XML_CHAR_C("OnEntry"),                .tag_id = SEDS_NODETYPE_ON_ENTRY                },
        { .tag_name = XML_CHAR_C("OnExit"),                 .tag_id = SEDS_NODETYPE_ON_EXIT                 },
        { .tag_name = XML_CHAR_C("OnParameterPrimitive"),   .tag_id = SEDS_NODETYPE_ON_PARAMETER_PRIMITIVE  },
        { .tag_name = XML_CHAR_C("OnTimer"),                .tag_id = SEDS_NODETYPE_ON_TIMER                },
        { .tag_name = XML_CHAR_C("Operator"),               .tag_id = SEDS_NODETYPE_OPERATOR                },
        { .tag_name = XML_CHAR_C("ORedConditions"),         .tag_id = SEDS_NODETYPE_ORED_CONDITIONS         },
        { .tag_name = XML_CHAR_C("OverArray"),              .tag_id = SEDS_NODETYPE_OVER_ARRAY              },
        { .tag_name = XML_CHAR_C("PackageFile"),            .tag_id = SEDS_NODETYPE_PACKAGEFILE             },
        { .tag_name = XML_CHAR_C("Package"),                .tag_id = SEDS_NODETYPE_PACKAGE                 },
        { .tag_name = XML_CHAR_C("PaddingEntry"),           .tag_id = SEDS_NODETYPE_CONTAINER_PADDING_ENTRY },
        { .tag_name = XML_CHAR_C("ParameterActivityMapSet"),.tag_id = SEDS_NODETYPE_PARAMETER_ACTIVITY_MAP_SET },
        { .tag_name = XML_CHAR_C("ParameterActivityMap"),   .tag_id = SEDS_NODETYPE_PARAMETER_ACTIVITY_MAP  },
        { .tag_name = XML_CHAR_C("ParameterMapSet"),        .tag_id = SEDS_NODETYPE_PARAMETER_MAP_SET       },
        { .tag_name = XML_CHAR_C("ParameterMap"),           .tag_id = SEDS_NODETYPE_PARAMETER_MAP           },
        { .tag_name = XML_CHAR_C("ParameterSet"),           .tag_id = SEDS_NODETYPE_PARAMETER_SET           },
        { .tag_name = XML_CHAR_C("Parameter"),              .tag_id = SEDS_NODETYPE_PARAMETER               },
        { .tag_name = XML_CHAR_C("PolynomialCalibrator"),   .tag_id = SEDS_NODETYPE_POLYNOMIAL_CALIBRATOR   },
        { .tag_name = XML_CHAR_C("PrecisionRange"),         .tag_id = SEDS_NODETYPE_PRECISION_RANGE         },
        { .tag_name = XML_CHAR_C("ProvidedInterfaceSet"),   .tag_id = SEDS_NODETYPE_PROVIDED_INTERFACE_SET  },
        { .tag_name = XML_CHAR_C("Provided"),               .tag_id = SEDS_NODETYPE_PROVIDED                },
        { .tag_name = XML_CHAR_C("RangeConstraint"),        .tag_id = SEDS_NODETYPE_RANGE_CONSTRAINT        },
        { .tag_name = XML_CHAR_C("Range"),                  .tag_id = SEDS_NODETYPE_RANGE                   },
        { .tag_name = XML_CHAR_C("RequiredInterfaceSet"),   .tag_id = SEDS_NODETYPE_REQUIRED_INTERFACE_SET  },
        { .tag_name = XML_CHAR_C("Required"),               .tag_id = SEDS_NODETYPE_REQUIRED                },
        { .tag_name = XML_CHAR_C("SafeRangeSet"),           .tag_id = SEDS_NODETYPE_SAFE_RANGE_SET          },
        { .tag_name = XML_CHAR_C("SecondOperand"),          .tag_id = SEDS_NODETYPE_SECOND_OPERAND          },
        { .tag_name = XML_CHAR_C("Semantics"),              .tag_id = SEDS_NODETYPE_SEMANTICS               },
        { .tag_name = XML_CHAR_C("SemanticsTerm"),          .tag_id = SEDS_NODETYPE_SEMANTICS_TERM          },
        { .tag_name = XML_CHAR_C("SendCommandPrimitive"),   .tag_id = SEDS_NODETYPE_SEND_COMMAND_PRIMITIVE  },
        { .tag_name = XML_CHAR_C("SendParameterPrimitive"), .tag_id = SEDS_NODETYPE_SEND_PARAMETER_PRIMITIVE},
        { .tag_name = XML_CHAR_C("SetActivityOnly"),        .tag_id = SEDS_NODETYPE_SET_ACTIVITY_ONLY       },
        { .tag_name = XML_CHAR_C("SetActivity"),            .tag_id = SEDS_NODETYPE_SET_ACTIVITY            },
        { .tag_name = XML_CHAR_C("SplineCalibrator"),       .tag_id = SEDS_NODETYPE_SPLINE_CALIBRATOR       },
        { .tag_name = XML_CHAR_C("SplinePoint"),            .tag_id = SEDS_NODETYPE_SPLINE_POINT            },
        { .tag_name = XML_CHAR_C("StartAt"),                .tag_id = SEDS_NODETYPE_START_AT                },
        { .tag_name = XML_CHAR_C("StateMachineSet"),        .tag_id = SEDS_NODETYPE_STATE_MACHINE_SET       },
        { .tag_name = XML_CHAR_C("StateMachine"),           .tag_id = SEDS_NODETYPE_STATE_MACHINE           },
        { .tag_name = XML_CHAR_C("State"),                  .tag_id = SEDS_NODETYPE_STATE                   },
        { .tag_name = XML_CHAR_C("Step"),                   .tag_id = SEDS_NODETYPE_STEP                    },
        { .tag_name = XML_CHAR_C("StringDataEncoding"),     .tag_id = SEDS_NODETYPE_STRING_DATA_ENCODING    },
        { .tag_name = XML_CHAR_C("StringDataType"),         .tag_id = SEDS_NODETYPE_STRING_DATATYPE         },
        { .tag_name = XML_CHAR_C("StringValue"),            .tag_id = SEDS_NODETYPE_STRING_VALUE            },
        { .tag_name = XML_CHAR_C("SubRangeDataType"),       .tag_id = SEDS_NODETYPE_SUBRANGE_DATATYPE       },
        { .tag_name = XML_CHAR_C("Term"),                   .tag_id = SEDS_NODETYPE_POLYNOMIAL_TERM         },
        { .tag_name = XML_CHAR_C("TrailerEntryList"),       .tag_id = SEDS_NODETYPE_CONTAINER_TRAILER_ENTRY_LIST    },
        { .tag_name = XML_CHAR_C("Transition"),             .tag_id = SEDS_NODETYPE_TRANSITION              },
        { .tag_name = XML_CHAR_C("TypeCondition"),          .tag_id = SEDS_NODETYPE_TYPE_CONDITION          },
        { .tag_name = XML_CHAR_C("TypeConstraint"),         .tag_id = SEDS_NODETYPE_TYPE_CONSTRAINT         },
        { .tag_name = XML_CHAR_C("TypeOperand"),            .tag_id = SEDS_NODETYPE_TYPE_OPERAND            },
        { .tag_name = XML_CHAR_C("ValidRange"),             .tag_id = SEDS_NODETYPE_VALID_RANGE              },
        { .tag_name = XML_CHAR_C("ValueConstraint"),        .tag_id = SEDS_NODETYPE_VALUE_CONSTRAINT        },
        { .tag_name = XML_CHAR_C("Value"),                  .tag_id = SEDS_NODETYPE_VALUE                   },
        { .tag_name = XML_CHAR_C("VariableRef"),            .tag_id = SEDS_NODETYPE_VARIABLE_REF            },
        { .tag_name = XML_CHAR_C("VariableSet"),            .tag_id = SEDS_NODETYPE_VARIABLE_SET            },
        { .tag_name = XML_CHAR_C("Variable"),               .tag_id = SEDS_NODETYPE_VARIABLE                },

        /* catch instances of xi:include, these are ignored by this tool but do not generate warnings */
        { .tag_name = XML_CHAR_C("xi:include"),             .tag_id = SEDS_NODETYPE_XINCLUDE_PASSTHRU       },

        /* Extra Element types _not_ defined by SEDS that this toolchain supports (SEDS supplement) */
        { .tag_name = XML_CHAR_C("DesignParameters"),       .tag_id = SEDS_NODETYPE_DESIGN_PARAMETERS       },
        { .tag_name = XML_CHAR_C("Define"),                 .tag_id = SEDS_NODETYPE_DEFINE                  },
        { .tag_name = XML_CHAR_C("InstanceRuleSet"),        .tag_id = SEDS_NODETYPE_INSTANCE_RULE_SET       },
        { .tag_name = XML_CHAR_C("InstanceRule"),           .tag_id = SEDS_NODETYPE_INSTANCE_RULE           },
        { .tag_name = XML_CHAR_C("InterfaceMapSet"),        .tag_id = SEDS_NODETYPE_INTERFACE_MAP_SET       },
        { .tag_name = XML_CHAR_C("InterfaceMap"),           .tag_id = SEDS_NODETYPE_INTERFACE_MAP           },
        { .tag_name = XML_CHAR_C("ParameterValue"),         .tag_id = SEDS_NODETYPE_PARAMETER_VALUE         },
        { .tag_name = XML_CHAR_C("DeclaredInterface"),      .tag_id = SEDS_NODETYPE_DECLARED_INTERFACE      },
        { .tag_name = XML_CHAR_C("ProvidedInterface"),      .tag_id = SEDS_NODETYPE_PROVIDED_INTERFACE      },
        { .tag_name = XML_CHAR_C("RequiredInterface"),      .tag_id = SEDS_NODETYPE_REQUIRED_INTERFACE      },

        /* Keep NULL tag last - absolute end of list marker */
        { .tag_name = NULL,                                 .tag_id = SEDS_NODETYPE_UNKNOWN                 }
};

/**
 * Local XML parser object.
 *
 * Contains a reference to the expat instance as well as the current source file.
 */
typedef struct
{
    XML_Parser xmlp;
    FILE *fp;
} seds_parser_t;


/*******************************************************************************/
/*                      Internal / static Helper Functions                     */
/*                  (these are not referenced outside this unit)               */
/*******************************************************************************/


/**
 * Helper function to identify an element in an XML file.
 *
 * Sometimes node identification is context-dependent, and therefore the parent nodetype
 * is required for complete identification.
 *
 * @returns the associated enumerated nodetype
 */
static seds_nodetype_t seds_xmlparser_identify_element(seds_nodetype_t parent_node_type, const XML_Char *str)
{
    const seds_stringmap_t *mapptr;
    seds_nodetype_t node_type;

    /* if the parent was any "passthru" type, then consider the child to also
     * be that same type.  This can be used to ignore entire subtrees, for
     * unimplemented features  */
    if (parent_node_type == SEDS_NODETYPE_UNKNOWN ||
            parent_node_type == SEDS_NODETYPE_XINCLUDE_PASSTHRU ||
            parent_node_type == SEDS_NODETYPE_DESCRIPTION_PASSTHRU)
    {
        node_type = parent_node_type;
    }
    else if (parent_node_type == SEDS_NODETYPE_LONG_DESCRIPTION)
    {
        /*
         * Once a long description has started, treat everything inside
         * as a continuation of the long description
         */
        node_type = SEDS_NODETYPE_DESCRIPTION_PASSTHRU;
    }
    else if (SEDSTAG_IS_VALID(parent_node_type))
    {
        mapptr = XML_SEDS_STARTTAG_MAP;
        while (mapptr->tag_name != NULL && mapptr->tag_id != SEDS_NODETYPE_UNKNOWN)
        {
            if (XML_strcmp(mapptr->tag_name, str) == 0)
            {
                break;
            }
            ++mapptr;
        }
        node_type = mapptr->tag_id;
    }
    else
    {
        node_type = SEDS_NODETYPE_UNKNOWN;
    }

    if (node_type == SEDS_NODETYPE_INTERFACE)
    {
        /*
         * For interface tags, this is actually one of three possible types.  They are all
         * "Interface" in the schema, but can mean three different things depending on the
         * context.  This can be a declaration, provider, or requirer reference depending
         * on the parent element type.  It is important to differentiate these early as
         * there are different sets of required attributes and the attribute interpretation
         * varies.
         */
        switch(parent_node_type)
        {
        case SEDS_NODETYPE_DECLARED_INTERFACE_SET:
            node_type = SEDS_NODETYPE_DECLARED_INTERFACE;
            break;
        case SEDS_NODETYPE_REQUIRED_INTERFACE_SET:
            node_type = SEDS_NODETYPE_REQUIRED_INTERFACE;
            break;
        case SEDS_NODETYPE_PROVIDED_INTERFACE_SET:
            node_type = SEDS_NODETYPE_PROVIDED_INTERFACE;
            break;
        case SEDS_NODETYPE_BASE_INTERFACE_SET:
            node_type = SEDS_NODETYPE_BASE_INTERFACE;
            break;
        default:
            break;
        }
    }

    return node_type;
}

/* ------------------------------------------------------------------- */
/**
 * Helper function to generate a message to the user.
 *
 * This used to generate messages to the user console regarding XML parsing
 * warnings, errors, or other information.  This attempts to add contextual
 * information to indicate the source of the error or warning.
 *
 * The message should be a Lua string at the top of the Lua stack, and the
 * top-1 entry on the Lua stack should be the node on which the message
 * is based (for context info).
 *
 * @param lua the Lua stack
 * @param msgtype the type of message to generate (error or warning, typically)
 */
static void seds_xmlparser_message(lua_State *lua, seds_user_message_t msgtype)
{
    int top_start = lua_gettop(lua);        /* top of stack should be the user supplied message */
    const char *element;
    const char *file;
    unsigned long line;

    element = NULL;
    file = NULL;
    line = 0;

    if (luaL_checkudata(lua, top_start - 1, "seds_node"))
    {
        lua_getfield(lua, top_start - 1, "xml_filename");
        lua_getfield(lua, top_start - 1, "xml_linenum");
        lua_getfield(lua, top_start - 1, "xml_element");

        file = lua_tostring(lua, -3);
        line = lua_tointeger(lua, -2);
        element = lua_tostring(lua, -1);
    }

    seds_user_message_preformat(msgtype, file, line, luaL_tolstring(lua, top_start, NULL), element);

    lua_settop(lua, top_start - 1);
}

/* ------------------------------------------------------------------- */
/**
 * Helper function to "recreate" a chunk of XML code
 *
 * Lua callable function which generates an XML element string based on an
 * element name, attribute set, and arbitrary character data.
 *
 * This is used to create "resolved" versions of the XML tags with the
 * placeholders replaced with real values for attributes.
 *
 * @param lua Lua stack
 *
 * Expected Lua stack arguments:
 *  1: XML element name (string)
 *  2: XML attribute set (table)
 *  3: XML xml_cdata (string)
 *
 * Returns a Lua string with the above converted into XML
 */
static int seds_xmlparser_recreate_xml(lua_State *lua)
{
    luaL_Buffer buf;
    luaL_buffinit(lua, &buf);
    luaL_addchar(&buf, '<');
    luaL_addlstring(&buf, lua_tostring(lua, 1), lua_rawlen(lua, 1));

    lua_pushnil(lua);
    while (lua_next(lua, 2))
    {
        if (lua_type(lua, -2) == LUA_TSTRING)
        {
            luaL_addchar(&buf, ' ');
            luaL_addlstring(&buf, lua_tostring(lua, -2), lua_rawlen(lua, -2));
            luaL_addstring(&buf, "=\"");
            luaL_addlstring(&buf, lua_tostring(lua, -1), lua_rawlen(lua, -1));
            luaL_addchar(&buf, '"');
        }
        lua_pop(lua, 1);
    }

    if (lua_rawlen(lua, 3) == 0)
    {
        /* empty element */
        luaL_addstring(&buf, "/>");
    }
    else
    {
        luaL_addchar(&buf, '>');
        luaL_addlstring(&buf, lua_tostring(lua, 3), lua_rawlen(lua, 3));
        luaL_addstring(&buf, "</");
        luaL_addlstring(&buf, lua_tostring(lua, 1), lua_rawlen(lua, 1));
        luaL_addchar(&buf, '>');
    }

    luaL_pushresult(&buf);
    return 1;
}

/* ------------------------------------------------------------------- */
/**
 * Helper function for handling of flattened NAMESPACE elements
 *
 * The official SEDS examples do not explicitly nest "<namespace>" elements, but
 * do allow nesting to be specified implicitly using namespace separators within
 * a single namespace element.  for instance, they do this:
 *
 *   <namespace name="CCSDS/SOIS/SEDS"> ... </namespace>
 *
 * rather than:
 *
 *   <namespace name="CCSDS">
 *   <namespace name="SOIS">
 *   <namespace name="EDS">
 *    ...
 *   </namespace>
 *   </namespace>
 *   </namespace>
 *
 * The post-processing later on works better when each namespace has a dedicated
 * node in the tree.  So in case the first (implicit nesting) syntax is used this
 * should be treated equivalently to the second syntax when building the in-memory
 * tree.
 *
 * This code will detect if the the namespace contains separator chars ('/') and if
 * so, it will break up the single node into multiple nodes with parent connections.
 *
 * Expected Lua input stack:
 *  1: Namespace node to break up.
 *
 * Returns a modified namespace node following the hierarchy
 */
static int seds_xmlparser_breakup_namespace(lua_State *lua)
{
    int stack_top = lua_gettop(lua);
    const char *nspace;
    const char *sep;

    while(1)
    {
        if (luaL_checkudata(lua, 1, "seds_node") == NULL)
        {
            break;
        }
        lua_getfield(lua, 1, "name");
        if (lua_type(lua, stack_top + 1) != LUA_TSTRING)
        {
            break;
        }

        nspace = lua_tostring(lua, stack_top + 1);
        sep = strrchr(nspace, '/');
        if (sep == NULL)
        {
            break;
        }

        /*
         * Set the name of the original node to be only the
         * remainder of the string past the final slash (/) character
         */
        lua_pushstring(lua, sep + 1);
        lua_setfield(lua, stack_top, "name");

        /*
         * Create a new namespace node to act as the parent
         * Duplicate the xml_filename / xml_linenum fields
         * Set the name to be the string prior to the final slash (/) character
         */
        seds_tree_node_push_new_object(lua, SEDS_NODETYPE_PACKAGE);
        lua_getfield(lua, stack_top, "xml_filename");
        lua_setfield(lua, stack_top + 2, "xml_filename");
        lua_getfield(lua, stack_top, "xml_linenum");
        lua_setfield(lua, stack_top + 2, "xml_linenum");
        lua_pushlstring(lua, nspace, sep - nspace);
        lua_setfield(lua, stack_top + 2, "name");

        /*
         * Set the original node as a subnode of the newly-created node
         */
        lua_newtable(lua);
        lua_pushvalue(lua, 1);
        lua_rawseti(lua, stack_top + 3, 1);
        lua_setfield(lua, stack_top + 2, "subnodes");

        /*
         * Set the newly-created node as the parent of the original node
         */
        lua_pushvalue(lua, stack_top + 2);
        lua_setfield(lua, 1, "parent");

        lua_replace(lua, 1);
        lua_settop(lua, 1);
    }

    lua_settop(lua, 1);
    return 1;
}

/* ------------------------------------------------------------------- */
/**
 * Helper function for handling XML start tags
 *
 * This is an Expat-compatible callback that is called by the Expat parser when it
 * encounters an XML start tag.  The prototype and arguments are specified by Expat.
 *
 * @param data user object, which should be the Lua stack
 * @param el Element name
 * @param xattr XML attributes
 *
 * A new DOM tree node is created to reflect this XML element, and will be pushed
 * onto the Lua stack.
 */
static void seds_xmlparser_starttag(void *data, const XML_Char *el, const XML_Char **xattr)
{
    lua_State *lua = data;
    seds_nodetype_t node_type;
    const char *name_attr;
    char attrib_string[64];
    size_t len;
    int top_start;
    const XML_Char **iattr;

    top_start = lua_gettop(lua);

    luaL_checkstack(lua, 4, __func__);

    /*
     * Node should be at the top of the stack, representing the parent element data
     */
    luaL_argcheck(lua, luaL_checkudata(lua, top_start, "seds_node") != NULL, top_start, "seds_node expected");

    /*
     * Create a new node to store the XML element attributes and associated data
     * Stack Position => top + 1
     */
    node_type = seds_xmlparser_identify_element(seds_tree_node_get_type(lua, -1), el);
    seds_tree_node_push_new_object(lua, node_type);

    /*
     * Store file and line number of the source XML in case an error
     * occurs during validation, this info helps to create useful messages.
     *
     * This is stored unconditionally for ALL elements encountered
     */
    lua_pushvalue(lua, 2);
    luaL_checktype(lua, top_start + 2, LUA_TSTRING);
    lua_setfield(lua, top_start + 1, "xml_filename");
    lua_getfield(lua, 1, "xml_linenum");
    lua_setfield(lua, top_start + 1, "xml_linenum");
    lua_pushstring(lua, el);
    lua_setfield(lua, top_start + 1, "xml_element");

    if (node_type == SEDS_NODETYPE_ENUMERATION_ENTRY)
    {
        name_attr = "label";
    }
    else
    {
        name_attr = "name";
    }

    if (xattr[0] != NULL && xattr[1] != NULL)
    {
        iattr = xattr;
        lua_newtable(lua);                                      /* top+2: current node attribute table */
        lua_newtable(lua);                                      /* top+3: current node attribute table */
        while (iattr[0] != NULL && iattr[1] != NULL)
        {
            lua_pushstring(lua, iattr[0]);
            lua_pushstring(lua, iattr[1]);
            /*
             * Make a local copy of the attribute name,
             * converting all characters to lowercase
             */
            for (len = 0; iattr[0][len] != 0 && len < (sizeof(attrib_string) - 1); ++len)
            {
                attrib_string[len] = XML_tolower(iattr[0][len]);
            }
            attrib_string[len] = 0;
            if (strcmp(attrib_string, name_attr) == 0)
            {
                lua_setfield(lua, top_start + 1, "name");
            }
            else
            {
                lua_setfield(lua, top_start + 2, attrib_string);
            }
            /* save the original attribute name in case the XML needs to be recreated */
            lua_setfield(lua, top_start + 3, attrib_string);
            iattr += 2;
        }
        lua_setfield(lua, top_start + 1, "xml_attrname");
        lua_setfield(lua, top_start + 1, "xml_attrs");
    }

    /*
     * In case the node type wasn't recognized, generate a warning
     * that the node will be dropped
     * (Note that this also causes any other XML tags beneath it to be dropped)
     */
    if (!SEDSTAG_IS_VALID(node_type))
    {
        lua_pushstring(lua, "dropped element");
        seds_xmlparser_message(lua, SEDS_USER_MESSAGE_WARNING);
    }

    lua_settop(lua, top_start + 1);                         /* top  : current node data table */
}

/* ------------------------------------------------------------------- */
/**
 * Helper function for handling XML end tags
 *
 * This is an Expat-compatible callback that is called by the Expat parser when it
 * encounters an XML end tag.  The prototype and arguments are specified by Expat.
 *
 * @param data user object, which should be the Lua stack
 * @param el Element name
 *
 * A the DOM tree node at the top of the Lua stack popped off, any necessary finalization
 * procedure is done, and then it is attached to its parent node (the node now at the
 * top of the stack).
 */
static void seds_xmlparser_endtag(void *data, const XML_Char *el)
{
    lua_State *lua = data;
    const char *parent_append_field;
    int parent_append_type;
    int top_start;

    top_start = lua_gettop(lua);
    luaL_argcheck(lua, luaL_checkudata(lua, top_start - 1, "seds_node") != NULL, top_start - 1, "seds_node expected");

    switch(seds_tree_node_get_type(lua, top_start))
    {
    case SEDS_NODETYPE_LONG_DESCRIPTION:
        /*
         * concatenate with the "longdescription" of the parent
         */
        parent_append_field = "longdescription";
        parent_append_type = LUA_TSTRING;
        lua_getfield(lua, top_start, "xml_cdata");
        lua_remove(lua, top_start);
        break;

    case SEDS_NODETYPE_DESCRIPTION_PASSTHRU:
        /*
         * Re-create an XML tag from the contents,
         * concatenate with the "xml_cdata" of the parent
         */
        parent_append_field = "xml_cdata";
        parent_append_type = LUA_TSTRING;

        lua_pushcfunction(lua, seds_xmlparser_recreate_xml);
        lua_getfield(lua, top_start, "xml_element");
        lua_getfield(lua, top_start, "xml_attrs");
        lua_getfield(lua, top_start, "xml_cdata");
        lua_call(lua, 3, 1);

        lua_remove(lua, top_start);
        break;

    case SEDS_NODETYPE_PACKAGE:
        /*
         * Namespaces needs special handing in case they have
         * implicitly nested namespaces within them.
         *
         */
        lua_pushcfunction(lua, seds_xmlparser_breakup_namespace);
        lua_insert(lua, top_start);
        lua_call(lua, 1, 1);

        parent_append_field = "subnodes";
        parent_append_type = LUA_TTABLE;
        break;

    case SEDS_NODETYPE_UNKNOWN:
    case SEDS_NODETYPE_XINCLUDE_PASSTHRU:
        /* discard the node, do not append it */
        parent_append_field = NULL;
        parent_append_type = LUA_TNONE;
        break;

    default:
        /*
         * Typical case -- append the current node to the subnodes table of the parent
         */
        parent_append_field = "subnodes";
        parent_append_type = LUA_TTABLE;
        break;
    }

    if (parent_append_field != NULL)
    {
        if (parent_append_type == LUA_TTABLE)
        {
            lua_pushvalue(lua, top_start - 1);
            lua_setfield(lua, top_start, "parent");

            lua_getfield(lua, 1, "id_lookup");
            if (!lua_istable(lua, -1))
            {
                lua_pop(lua, 1);
                lua_newtable(lua);
            }
            lua_pushvalue(lua, top_start);
            lua_rawseti(lua, -2,  1 + lua_rawlen(lua, -2));

            lua_pushinteger(lua, lua_rawlen(lua, -1));
            lua_setfield(lua, top_start, "id");
            lua_setfield(lua, 1, "id_lookup");
        }

        lua_getfield(lua, top_start - 1, parent_append_field);

        if (lua_type(lua, -1) != parent_append_type)
        {
            lua_pop(lua, 1);
            switch(parent_append_type)
            {
            case LUA_TTABLE:
                lua_newtable(lua);
                break;
            case LUA_TSTRING:
                lua_pushstring(lua, "");
                break;
            default:
                lua_pushnil(lua);
                break;
            }
        }

        lua_insert(lua, top_start);

        switch(parent_append_type)
        {
        case LUA_TTABLE:
            lua_rawseti(lua, top_start, 1 + lua_rawlen(lua, top_start));
            break;
        case LUA_TSTRING:
            if (lua_isstring(lua, -1))
            {
                lua_concat(lua, 2);
            }
            else
            {
                lua_pop(lua, 1);
            }
            break;
        default:
            break;
        }

        lua_setfield(lua, top_start - 1, parent_append_field);
    }

    lua_settop(lua, top_start - 1);
}

/* ------------------------------------------------------------------- */
/**
 * Helper function for handling XML character data
 *
 * This is an Expat-compatible callback that is called by the Expat parser when it
 * encounters arbitrary character data within the XML file.  The prototype and arguments
 * are specified by Expat.
 *
 * @param data user object, which should be the Lua stack
 * @param s Character data
 * @param len Length of character data
 *
 * This function attaches the character data to the node at the top of the Lua
 * stack in the "cdata" property, for future use.
 */
static void seds_xmlparser_cdata(void *data, const XML_Char *s, int len)
{
    lua_State *lua = data;
    int top_start = lua_gettop(lua);


    /*
     * At the top of the LUA stack should be a table
     * Append this data to the "xml_cdata" field of that table
     *
     * (If the top element is not a table, then this xml_cdata is discarded)
     */
    if (len > 0)
    {
        luaL_argcheck(lua, luaL_checkudata(lua, top_start, "seds_node") != NULL, top_start, "seds_node expected");
        lua_getfield(lua, top_start, "xml_cdata");
        if (lua_isnil(lua, -1))
        {
            /* First xml_cdata -- pop the nil and push the xml_cdata string */
            lua_pop(lua, 1);
            lua_pushlstring(lua, s, len);
        }
        else
        {
            /* Subsequent xml_cdata -- concatenate xml_cdata strings together */
            lua_pushlstring(lua, s, len);
            lua_concat(lua, 2);
        }
        lua_setfield(lua, top_start, "xml_cdata");
    }

    /*
     * Ensure that this function is always stack-neutral
     */
    lua_settop(lua, top_start);
}

/* ------------------------------------------------------------------- */
/**
 * Lua-callable Helper function to reset the XML parser.
 *
 * This closes any open file and frees the Expat instance, if it exists.
 * Expected Stack args:
 *    1: parser object
 *
 * No return value.
 */
static int seds_xmlparser_reset(lua_State *lua)
{
    seds_parser_t *pself = luaL_checkudata(lua, 1, "seds_parser");

    if (pself != NULL)
    {
        if (pself->fp != NULL)
        {
            fclose(pself->fp);
            pself->fp = NULL;
        }
        if (pself->xmlp != NULL)
        {
            XML_ParserFree(pself->xmlp);
            pself->xmlp = NULL;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------- */
/**
 * Lua-callable Helper function to get metadata from an XML parser object
 *
 * Expected Stack args:
 *  1: parser object
 *  2: property name
 *
 * Any properties may be added to a parser object at any time during parsing.
 * The special property "xml_linenum" will always return the current line number
 * of the XML parser (from the original XML source).
 */
static int seds_xmlparser_get_property(lua_State *lua)
{
    /*
     */
    seds_parser_t *pself = luaL_checkudata(lua, 1, "seds_parser");
    const char *prop = NULL;

    /*
     * note - lua_tostring() possibly converts to a string,
     * so do not call it unless it already is a string
     */
    if (lua_type(lua, 2) == LUA_TSTRING)
    {
        prop = lua_tostring(lua, 2);
    }

    if (prop != NULL && strcmp(prop, "xml_linenum") == 0)
    {
        if (pself->xmlp == NULL)
        {
            lua_pushnil(lua);
        }
        else
        {
            lua_pushinteger(lua, XML_GetCurrentLineNumber(pself->xmlp));
        }
    }
    else
    {
        /*
         * Get it from the user value table
         */
        lua_getuservalue(lua, 1);
        lua_pushvalue(lua, 2);
        lua_rawget(lua, -2);
        lua_remove(lua, -2);
    }

    return 1;
}

/* ------------------------------------------------------------------- */
/**
 * Lua-callable Helper function to set metadata in an XML parser object
 *
 * Expected Stack args:
 *  1: parser object
 *  2: property name
 *  3: property value
 *
 * Any properties may be added to a parser object at any time during parsing.
 * The special property "xml_linenum" is read-only and throws an error if
 * attempting to set it.
 */
static int seds_xmlparser_set_property(lua_State *lua)
{
    const char *prop = NULL;

    /*
     * note - lua_tostring() possibly converts to a string,
     * so do not call it unless it already is a string
     */
    if (lua_type(lua, 2) == LUA_TSTRING)
    {
        prop = lua_tostring(lua, 2);
    }

    if (prop != NULL && strcmp(prop, "xml_linenum") == 0)
    {
        /* not settable */
        luaL_error(lua, "read-only property: %s", prop);
    }
    else
    {
        /*
         * Set it in the user value table
         * Stack pos = 4
         */
        lua_getuservalue(lua, 1);
        lua_pushvalue(lua, 2);
        lua_pushvalue(lua, 3);
        lua_rawset(lua, 4);
    }

    return 0;
}

/*******************************************************************************/
/*                      Externally-Called Functions                            */
/*      (referenced outside this unit and prototyped in a separate header)     */
/*******************************************************************************/

/*
 * ------------------------------------------------------
 * External API function - see full details in prototype.
 * ------------------------------------------------------
 */
int seds_xmlparser_readfile(lua_State *lua)
{
    seds_parser_t *pself = luaL_checkudata(lua, 1, "seds_parser");
    void *xmlbuf;
    size_t len;
    int file_flag;


    luaL_argcheck(lua, pself != NULL, 1, "seds_parser expected");

    /*
     * Create a new XML parser every time
     *
     * Although expat has a "XML_ParserReset" function, there doesn't seem to be documentation
     * as to whether it can handle an error return (longjmp) and still reset sanely.
     *
     * The safest thing is to just totally re-create it each time we parse a file
     */
    if (pself->xmlp != NULL)
    {
        XML_ParserFree(pself->xmlp);
    }
    pself->xmlp = XML_ParserCreate(NULL);
    if (pself->xmlp == NULL)
    {
        lua_pushfstring(lua, "XML_ParserCreate(): %s", strerror(errno));
        return lua_error(lua);
    }

    /*
     * If there is an open file from a previous call, close it first
     */
    if (pself->fp != NULL)
    {
        fclose(pself->fp);
    }
    pself->fp = fopen(lua_tostring(lua, 2), "r");    /* arg2: xml_filename */
    if (!pself->fp)
    {
        lua_pushfstring(lua, "%s: %s", lua_tostring(lua, 2), strerror(errno));
        return lua_error(lua);
    }

    XML_SetElementHandler(pself->xmlp, seds_xmlparser_starttag, seds_xmlparser_endtag);
    XML_SetCharacterDataHandler(pself->xmlp, seds_xmlparser_cdata);

    XML_SetUserData(pself->xmlp, lua);

    /*
     * Get the documents table from the parser object,
     * which is the root node that all other parsed elements will attach to
     */
    lua_getfield(lua, 1, "documents");

    do
    {
        xmlbuf = XML_GetBuffer(pself->xmlp, 16384);
        if (xmlbuf == NULL)
        {
            lua_pushfstring(lua, "%s: XML Buffering Error: %s",
                    lua_tostring(lua, 2),
                    XML_ErrorString(XML_GetErrorCode(pself->xmlp)));
            return lua_error(lua);
        }

        len = fread(xmlbuf, 1, 16384, pself->fp);
        file_flag = ferror(pself->fp);
        if (file_flag != 0)
        {
            lua_pushfstring(lua, "%s: %s", lua_tostring(lua, 2), strerror(file_flag));
            return lua_error(lua);
        }

        file_flag = feof(pself->fp);
        if (!XML_ParseBuffer(pself->xmlp, len, file_flag))
        {
            lua_pushfstring(lua, "%s:%d: XML Parsing Error: %s",
                    lua_tostring(lua, 2),
                    XML_GetCurrentLineNumber(pself->xmlp),
                    XML_ErrorString(XML_GetErrorCode(pself->xmlp)));
            return lua_error(lua);
        }
    }
    while (!file_flag);

    XML_SetUserData(pself->xmlp, NULL);

    if (pself->xmlp != NULL)
    {
        XML_ParserFree(pself->xmlp);
        pself->xmlp = NULL;
    }
    if (pself->fp != NULL)
    {
        fclose(pself->fp);
        pself->fp = NULL;
    }

    lua_setfield(lua, 1, "documents");

    return 0;
}

/*
 * ------------------------------------------------------
 * External API function - see full details in prototype.
 * ------------------------------------------------------
 */
int seds_xmlparser_create(lua_State *lua)
{
    seds_parser_t *pself;

    pself = (seds_parser_t *)lua_newuserdata(lua, sizeof(*pself));
    memset(pself, 0, sizeof(*pself));
    if (luaL_newmetatable(lua, "seds_parser"))
    {
        /*
         * Attach a garbage collector function.
         * This way, when the above object goes out of scope,
         * the file can be closed an the XML parser object can be freed.
         *
         * This method works even in error conditions where the normal exit path
         * is not followed (i.e. if someone called lua_error)
         */
        lua_pushstring(lua, "__gc");
        lua_pushcfunction(lua, seds_xmlparser_reset);
        lua_rawset(lua, -3);

        lua_pushstring(lua, "__index");
        lua_pushcfunction(lua, seds_xmlparser_get_property);
        lua_rawset(lua, -3);

        lua_pushstring(lua, "__newindex");
        lua_pushcfunction(lua, seds_xmlparser_set_property);
        lua_rawset(lua, -3);
    }
    lua_setmetatable(lua, -2);

    /* Create a storage table for parse messages and result nodes */
    lua_newtable(lua);

    lua_pushstring(lua, "documents");
    seds_tree_node_push_new_object(lua, SEDS_NODETYPE_ROOT);
    lua_rawset(lua, -3);

    lua_setuservalue(lua, -2);

    return 1;
}

/*
 * ------------------------------------------------------
 * External API function - see full details in prototype.
 * ------------------------------------------------------
 */
int seds_xmlparser_finish(lua_State *lua)
{
    seds_parser_t *pself = luaL_checkudata(lua, 1, "seds_parser");
    luaL_argcheck(lua, pself != NULL, 1, "seds_parser expected");
    luaL_checkudata(lua, 1, "seds_parser");
    lua_getfield(lua, 1, "documents");
    lua_getfield(lua, 2, "subnodes");
    if (!lua_istable(lua, 3))
    {
        return luaL_error(lua, "No XML datasheets parsed");
    }

    seds_user_message_printf(SEDS_USER_MESSAGE_DEBUG, __FILE__, __LINE__, "%s(): ROOT=> %d", __func__, (int) lua_rawlen(lua, 3));
    lua_pushnil(lua);
    while (lua_next(lua, 3))
    {
        luaL_tolstring(lua, -2, NULL);
        luaL_tolstring(lua, -2, NULL);
        seds_user_message_printf(SEDS_USER_MESSAGE_DEBUG, __FILE__, __LINE__, "%s(): %s => %s", __func__,
                lua_tostring(lua, -2), lua_tostring(lua, -1));
        lua_pop(lua, 3);
    }
    lua_pop(lua, 1);
    return 1;
}

