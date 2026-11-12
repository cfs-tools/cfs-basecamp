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
       Define Basecamp application filenames and provide path
       management
        
    Notes:
       1. Some design goals include encapsulation of the knowledge
          of basecamp's directory structure and abstraction of the
          structure so clients can query for a path by name. For 
          example, query for the ground scripts directory.          
       
"""

import os
from pathlib import Path
from enum import Enum
import configparser

def compress_abs_path(path):
    """
    Remove '../' entries from a path
    """
    path_array = path.split(os.path.sep)
    path_array.pop(0)   # Abs paths start with separator so first split array is ''

    new_path_array = []
    for p in path_array:
        if p == '..':
            new_path_array.pop()
        else:
            new_path_array.append(p)
    
    new_path = os.path.sep
    for p in new_path_array:
        new_path += p + os.path.sep
    
    return new_path


###############################################################################

class BcPathConst:
    """
    Define Basecamp path and filename constants. These hould not change and
    are considered part of the application therefore they are defined here
    and not in a configuration file.
    
    These can't be defined in the classes that they are related to because it
    prevents them from being in class definitons. For example, if BcPathMgr
    defines BASECAMP_INI_FILE then it can't be used in get_file() definition
    """
    
    ### Build & Target Info ###
    
    DEFAULT_CPU_NAME  = 'cpu1'  # Default CPU name used to prefix files in app repositories 
    CMAKE_INSERT_TAG  = '!BASECAMP-INSERT!'  # Tag TARGET_CMAKE_FILE lines that can be modified
    
    ### File Info ###
    
    BASECAMP_INI_FILE   = 'basecamp.ini'
    TUTORIAL_JSON_FILE  = 'tutorial.json'

    TARGET_CMAKE_FILE = 'targets.cmake'
    CFE_STARTUP_SCR   = 'cfe_es_startup.scr'
    CFE_TOPIC_ID_FILE = 'cfe-topicids.xml'

    SH_BUILD_CFS_TOPICIDS = 'build_cfs_topicids.sh'
    SH_MAKE_INSTALL_CFS   = 'make_install.sh'
    SH_STOP_CFS           = 'stop_cfs.sh'
    SH_START_CFS          = 'start_cfs.sh'
    SH_SUDO_START_CFS     = 'sudo_start_cfs.sh'    
    
    KIT_SCH_USRMSG_TBL  = 'kit_sch_usrmsgtbl.json'
    KIT_TO_PKT_TBL      = 'kit_to_pkt_tbl.json'
    
    ### Path Info ###

    CMAKE_DEFS_PATH_STR = 'basecamp_defs'
    DOCS_PATH_STR       = 'docs'

    CFS_STARTUP_PATH    = '/cf'   # This is short for compact flash
    
    CPU_TYPE   = '<cpu>'     # CfsPathMgr path template parameter
    BUILD_TYPE = '<build>'   # CfsPathMgr path template parameter

###############################################################################

class BcPath(Enum):
    GUI_APP         =  1
    CFS_INTERFACE   =  2
    REMOTE_OPS      =  3
    APP_TOOLS       =  4   # GUI app tools vs gnd-sys tools defined below
    CMD_SENDER      =  5
    FLT_FILE_SERVER =  6
    LOGS            =  7
    PERF_MONITOR    =  8
    PROJECTS        =  9
    SCRIPTS         = 10
    TEMPLATES       = 11
    TOOLS           = 12
    TUTORIALS       = 13
    USR_APPS        = 14
    USR_SCRIPTS     = 15
    
     
###############################################################################

class BcPathMgr:
    """
    All python scripts that use this facility are run within basecamp's gnd-sys
    directory structure. Therefore the current working directory is somewhere
    in the path: gnd-sys/app/[cfsinterface | remoteops | tools]
        The usr base dir is assumed to be at same dir level as basecamp root dir
    """
    
    # Relative paths from basecamp execution directory
    RELATIVE_PATH = {
        BcPath.GUI_APP.name         : ".",
        BcPath.CFS_INTERFACE.name   : "cfsinterface",
        BcPath.REMOTE_OPS.name      : "remoteops",
        BcPath.APP_TOOLS.name       : "tools",
        BcPath.CMD_SENDER.name      : "../cmd-sender",
        BcPath.FLT_FILE_SERVER.name : "../flt-file-server",
        BcPath.LOGS.name            : "../logs",
        BcPath.PERF_MONITOR.name    : "../perf-monitor",
        BcPath.PROJECTS.name        : "../projects",
        BcPath.SCRIPTS.name         : "../scripts",
        BcPath.TEMPLATES.name       : "../templates",
        BcPath.TOOLS.name           : "../tools",
        BcPath.TUTORIALS.name       : "../tutorials",
        BcPath.USR_APPS.name        : "../../usr/apps",
        BcPath.USR_SCRIPTS.name     : "../../usr/scripts"
    } 
 
 
    def __init__(self):
        self.current_dir  = Path.cwd()       
        cwd_path_parts = self.current_dir.parts
        
        # Goal is to find GUI executable directory. 'app' may not
        # not be unique so use 'gnd-sys'  
        target_dir = 'gnd-sys'
        if target_dir in cwd_path_parts:
            # +1 to target directory is the 'app' index of ../gnd-sys/app
            # Slice up to AND including the target (+1 handles inclusion)
            # The * unpacks the tuple segments back into a Path object
            bc_exe_dir_index = cwd_path_parts.index(target_dir) + 1
            self.gui_exe_path = Path(*cwd_path_parts[:bc_exe_dir_index + 1])
        self.create_abs_paths()
        
    def create_abs_paths(self):
        self.absolute_path = {}
        for key, value in self.RELATIVE_PATH.items():
            self.absolute_path[key] = Path(compress_abs_path(str(self.gui_exe_path / value)))

    def get(self, path: BcPath = BcPath.GUI_APP) -> str:
        """
        get() defaults to None if key not found, no exception thrown.
        """
        return self.absolute_path.get(path.name)

    def get_doc(self, path: str = BcPath.GUI_APP) -> str:
        """
        This function allows the BcPath enumerations to be used to retrieve
        document paths. The following paths are supported:
            BcPath.GUI_APP  - Technical docs 
            BcPath.PROJECTS - Project docs
        """
        doc_path = None
        if path == BcPath.GUI_APP:
            doc_path = Path(self.get(path) / '../../' / BcPathConst.DOCS_PATH_STR)
            doc_path = compress_abs_path(str(doc_path))             
        elif path == BcPath.PROJECTS:
            doc_path = Path(self.get(path) / BcPathConst.DOCS_PATH_STR)
            doc_path = compress_abs_path(str(doc_path))
        return doc_path
     
    def get_file(self, path: BcPath = BcPath.GUI_APP, filename: str = BcPathConst.BASECAMP_INI_FILE) -> str:
        return str(self.get(path) / Path(filename))

    def get_basecamp_ini_file(self):
        return str(self.gui_exe_path / BcPathConst.BASECAMP_INI_FILE)

    def get_shell_script(self, shell_script: str) -> str:
        """
        The shell_script parameter must be one of the filenames defined as
        BcPathConst.SH_*. This function does not verify this constraint 
        
        Clients don't need to know where shell scripts reside 
        """
        return self.get_file(BcPath.GUI_APP, shell_script)


###############################################################################

class CfsPath(Enum):
    BC_APPS         = 1
    CFS             = 2
    CFS_TARGET_DEFS = 3
    CFS_APPS        = 4
    CFS_EXE         = 5
    CFS_CF          = 6
    
    
###############################################################################

class CfsPathMgr():
    """
    The BcPathMgr.*_TYPE parameters are replaced with 
    BcPathConst.BASECAMP_INI_FILE configurations.
    """
    
    # Relative paths from basecamp execution directory
    RELATIVE_PATH = {
        CfsPath.BC_APPS.name         : "../../apps",
        CfsPath.CFS.name             : "../../cFS",
        CfsPath.CFS_TARGET_DEFS.name : f"../../cFS/{BcPathConst.CMAKE_DEFS_PATH_STR}",
        CfsPath.CFS_APPS.name        : "../../cFS/apps",
        CfsPath.CFS_EXE.name         : f"../../cFS/{BcPathConst.BUILD_TYPE}/exe/{BcPathConst.CPU_TYPE}",
        CfsPath.CFS_CF.name          : f"../../cFS/{BcPathConst.BUILD_TYPE}/exe/{BcPathConst.CPU_TYPE}/cf"
    }     
    
    def __init__(self):
        self.bc_path_mgr = BcPathMgr()
        self.gui_exe_path = self.bc_path_mgr.get(BcPath.GUI_APP)
        
        config_file = str(Path(self.gui_exe_path / BcPathConst.BASECAMP_INI_FILE))
        self.config = configparser.ConfigParser()
        self.config.read(config_file)

        self.CPU_NAME   = self.config.get('CFS_TARGET','CPU_NAME').lower()     
        self.BUILD_TYPE = self.config.get('CFS_TARGET','BUILD_TYPE')      
        
        self.create_abs_paths()

    def create_abs_paths(self):
        
        self.absolute_path = {}
        for key, value in CfsPathMgr.RELATIVE_PATH.items():
            abs_path = compress_abs_path(str(self.gui_exe_path / value))
            abs_path = abs_path.replace(BcPathConst.BUILD_TYPE, self.BUILD_TYPE).replace(BcPathConst.CPU_TYPE, self.CPU_NAME)
            self.absolute_path[key] = Path(abs_path)

    def get(self, path: CfsPath = CfsPath.CFS_TARGET_DEFS) -> str:
        """
        get() defaults to None if key not found, no exception thrown.
        """
        return self.absolute_path.get(path.name)

    def get_file(self, path: CfsPath = CfsPath.CFS_TARGET_DEFS, filename: str = BcPathConst.TARGET_CMAKE_FILE) -> str:
        return str(self.get(path) / Path(filename))

    def get_exe_filename(self) -> str:
        return 'core-' + self.CPU_NAME

    def get_cpu_name(self) -> str:
        return self.CPU_NAME

###############################################################################

if __name__ == '__main__':

    print('\n*** BcPath ***\n')
    bc_path_mgr = BcPathMgr()
    for path in BcPath:
        print(f'bc_path_mgr.get({path}): {bc_path_mgr.get(path)}')
    print(f'bc_path_mgr.get_doc({BcPath.GUI_APP}):  {bc_path_mgr.get_doc(BcPath.GUI_APP)}')
    print(f'bc_path_mgr.get_doc({BcPath.PROJECTS}): {bc_path_mgr.get_doc(BcPath.PROJECTS)}')
    print(f'bc_path_mgr.get_doc({BcPath.TOOLS}):    {bc_path_mgr.get_doc(BcPath.TOOLS)}')
    print(f'get_file(): {bc_path_mgr.get_file(BcPath.GUI_APP,BcPathConst.BASECAMP_INI_FILE)}')
    print(f'get_basecamp_ini_file(): {bc_path_mgr.get_basecamp_ini_file()}')
    
    print('\n*** CfsPath ***\n')
    cfs_path = CfsPathMgr()
    for path in CfsPath:
        print(f'cfs_path.get({path}): {cfs_path.get(path)}')    

