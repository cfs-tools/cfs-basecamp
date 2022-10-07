#!/usr/bin/env python
"""
    Copyright 2022 bitValence, Inc.
    All Rights Reserved.
    This program is free software; you can modify and/or redistribute it
    under the terms of the GNU Affero General Public License
    as published by the Free Software Foundation; version 3 with
    attribution addendums as found in the LICENSE.txt.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.
    
    Purpose:
      Define MQTT constants shared by Basecamp GUI target control
      and remote target ops
        
    Notes:    
      1. Test strings that can be pasted into hivemq browser MQTT broker 
            {"target": "noop"}
            {"seq-cnt": 1, "cmd-cnt": 2, "event": "This is a test", "cfs-exe": true, "cfs-apps": "a,b,c", "py-exe": true, "py-apps": "x,y,z" }
        
"""

"""
MQTT topic structure:
    /basecamp/<target type>/<id>/[cmd|tlm]
 
    <target>  - User defined target type, e.g. 'rpi' for Raspberry Pi
    <id>      - User defined unique identifier within the target type scope
    [cmd|tlm] - Identifes the topic as either a command or a telemetry message
""" 
 
MQTT_TOPIC_ROOT = 'basecamp'
MQTT_TOPIC_CMD  = 'cmd'
MQTT_TOPIC_TLM  = 'tlm'


"""
JSON payload command message:
    { 
      "[cfs|python|target]": "[CMD_CFS|CMD_PYTHON|CMD_TARGET]",
      "parameter": ""
    }
      
    - One command per payload 
    - Commands for each subsystem are listed below 
    - Parameter is a string whose content is command specific
"""

JSON_CMD_SUBSYSTEM_CFS    = 'cfs'
JSON_CMD_SUBSYSTEM_PYTHON = 'python'
JSON_CMD_SUBSYSTEM_TARGET = 'target'

JSON_CMD_CFS_ENA_TLM      = 'ena-tlm'
JSON_CMD_CFS_START        = 'start'
JSON_CMD_CFS_STOP         = 'stop'

JSON_CMD_PYTHON_LIST_APPS = 'list-apps'
JSON_CMD_PYTHON_START     = 'start'
JSON_CMD_PYTHON_STOP      = 'stop'

JSON_CMD_TARGET_NOOP      = 'noop'
JSON_CMD_TARGET_REBOOT    = 'reboot'
JSON_CMD_TARGET_SHUTDOWN  = 'shutdown'

JSON_CMD_PARAMETER        = 'parameter'


"""
JSON telemetry message constants:
{
    "seq-cnt":  integer
    "cmd-cnt":  integer
    "event"  :  string
    
    "cfs-exe":  boolean  # Is cFS executing?
    "cfs-apps": string   # Comma separated cFS user apps

    "py-exe":   boolean  # Is a python app executing?
    "py-apps":  string   # Comma separated list of python apps
                         # An asterick indicates the app is executing
}
"""

JSON_TLM_SEQ_CNT  = 'seq-cnt'
JSON_TLM_CMD_CNT  = 'cmd-cnt'
JSON_TLM_EVENT    = 'event'

JSON_TLM_CFS_EXE  = 'cfs-exe'
JSON_TLM_CFS_APPS = 'cfs-apps'

JSON_TLM_PY_EXE   = 'py-exe'
JSON_TLM_PY_APPS  = 'py-apps'

JSON_VAL_NONE = 'None'
