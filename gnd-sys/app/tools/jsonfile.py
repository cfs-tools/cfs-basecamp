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
      Provide common JSON file processing functionality that can be extended
      by specific JSON file use cases
      
    Notes:
      1. One design goal is to provide a functional API that doesn't couple
         the JSON object names to end clients. For example, clients should
         call JsonFile::title() rather than JsonFile::get('title') so the
         JSON object names are not coupled with the client.
"""

from abc import ABC, abstractmethod
import sys
sys.path.append('..')
import os
import time
import json
import re
import shutil
from pathlib import Path
from datetime import datetime
from typing import Any, Dict, Optional

import FreeSimpleGUI as sg

from tools.bcpaths import BcPath, BcPathMgr, CfsPath, CfsPathMgr

    
###############################################################################

class JsonFileError(Exception):
    """Base exception for all JSON file operations"""
    pass

class JsonFileMissingError(JsonFileError):
    """Raised when a required JSON file is missing."""
    pass

class JsonFileFormatError(JsonFileError):
    """Raised when a JSON file contains invalid syntax or fails validation."""
    pass

###############################################################################

class JsonFile():
    """
    Abstract base class to manage JSON files used to store state in a consistent
    manner. This is designed for Basecamp as opposed to general utility so it's up
    to the user to protect against errors. JSON key constants should all be used
    within the Json classes to localize impacts due to JSON key changes.
    """
    def __init__(self, json_file):
        self.path_filename = Path(json_file)
        self.json : Dict[str, Any] = {}
        
        if not self.path_filename.exists():
            raise JsonFileMissingError(f'Required file not found: {self.path_filename}')
       
        try:
            with open(self.path_filename, 'r', encoding='utf-8') as f:
                self.json = json.load(f)
            f.close()
        except json.JSONDecodeError as e:
            raise JsonFileFormatError(f'Malformed JSON syntax in {self.path_filename}: {e.msg} (line {e.lineno})') from e
        except PermissionError as e:
            raise JJsonFileError(f'Permission denied reading {self.path_filename}') from e

    def get(self, key: str, optional=False, default: Optional[Any] = None) -> Any:
        """
        Safely retrieves a value from the loaded data which means if a key is missing
        None will be returned and an exception is not thrown.
        This function should only be used by this class and it's child classes. See 
        file prologue notes for rationale. 
        """
        value = self.json.get(key, default)
        if value is None and not optional:
            raise JsonFileFormatError(f'Missing JSON key "{key}" in {self.path_filename}')
        return value
 
    def set(self, key: str, value: Any):
        if key in self.json:
            self.json[key] = value
        else:
            raise JsonFileFormatError(f'Missing JSON key "{key}" in {self.path_filename}')

    def title(self):
        return self.get('title')

    def objective(self):
        return self.get('objective')

    def description(self):
        return self.get('description')

    def reset(self):
        """
        Reset only applies when a JSON file is being used to maintain state
        across executions.
        """
        self.set('time-stamp', "")
        self.reset_child()
        self.write_file()

    @abstractmethod
    def reset_child(self):
        raise NotImplementedError
            
    def short_description(self):
        return self.get('short-descr')

    def update(self):
        self.set('time-stamp', datetime.now().strftime("%m/%d/%Y-%H:%M:%S"))
        self.write_file()
    
    def write_file(self):
        with open(self.path_filename, "w") as outfile:
            json.dump(self.json, outfile, indent=2)
    
    def write_time_stamp(self, time_stamp):
        if 'time-stamp' in self.json:
            self.json['time-stamp'] = time_stamp


###############################################################################

class JsonSupportFiles():
    """
    Project and Proxy App specs can include a support file object that
    identifies files to be copied into gnd-sys directories. The support
    file fields are defined as follows:    
    
        "support-files":
        { 
            "cmd-sender":  "file1,file2",
            "file-server": "",
            "scripts":     "",
            "templates":   "",
            "tutorials":   ""
        }
        
        The "" strings contain a list of comma separated filenames. 
    """
    
    # Map JSON objects to Basecamp paths
    PATHS = {
        'cmd-sender':  BcPath.CMD_SENDER,
        'file-server': BcPath.FLT_FILE_SERVER,
        'scripts':     BcPath.SCRIPTS,
        'templates':   BcPath.TEMPLATES,
        'tutorials':   BcPath.TUTORIALS
    }

    def __init__(self, client_spec_path, client_spec_support_files):
        """
        client_spec_path - Absolute path of the client's spec file where support files reside
        client_spec_support_files - Dictionary containing the JSON spec's "support-files"
        """
        self.bc_path_mgr = BcPathMgr()
        self.client_spec_path = client_spec_path
        self.client_spec_support_files = client_spec_support_files
  
    def path_file_list(self, path_key):
        """
        All support file objects use the same syntax of a string of comma
        separated filenames 
        """
        path_file_list = []

        if len(self.client_spec_support_files) > 0:
            try:
                support_file_string = self.client_spec_support_files[path_key]
                # Check greater than 0 length in case a couple of spaces
                if len(support_file_string) > 3:
                    path_file_list = support_file_string.split(',')
            except:
                pass
        
        return path_file_list

 
    def copy_files(self):
        """
        Copy files from client's directory to their respective ground system
        directory. Assumes all support files are in the client's base directory. 
        """        

        for path_key in self.PATHS:
            file_list = self.path_file_list(path_key)

            if file_list != None:
                bc_path = self.PATHS[path_key]
                for file in file_list:
                    src_pathfile = os.path.join(self.client_spec_path, file)
                    dst_pathfile = self.bc_path_mgr.get_file(bc_path, file)
                    os.makedirs(os.path.dirname(dst_pathfile), exist_ok=True)
                    copy_file = True
                    if os.path.exists(dst_pathfile):
                        overwrite = sg.popup_yes_no(f'{dst_pathfile} exists.\nDo you want to overwrite it?',  title='Support File Installation')
                        copy_file = (overwrite == 'Yes')
                    if copy_file:
                        shutil.copyfile(src_pathfile, dst_pathfile)
    
    """
    This was in protarget's ProjectTemplate class, but unused. Wanted to preserve logic
    in case a need arises and JsonSupportFiles is where it should live
    
    def delete_copy_support_files(self):
        # Assumes all support files are in the project's base directory. 

        gnd_sys_app_path = compress_abs_path(os.path.join(self.path, '../../app'))
        ##print(f'gnd_sys_app_path: {gnd_sys_app_path}')
        config = configparser.ConfigParser()
        config.read(os.path.join(gnd_sys_app_path,'basecamp.ini'))

        support_dict = {
            'cmd-sender':  'CMD_SENDER_PATH',
            'file-server': 'FLT_SERVER_PATH',
            'scripts':     'SCRIPT_PATH',
            'templates':   'APP_TEMPLATES_PATH',
            'tutorials':   'TUTORIALS_PATH'
            }

        for support_file in support_dict:           
            file_list = self.json.get_support_file(support_file)
            if file_list != None:
                config_param = support_dict[support_file]
                rel_gnd_path = config.get('PATHS',config_param)           
                for file in file_list:
                    src_pathfile = os.path.join(self.path, file)
                    dst_pathfile = os.path.join(gnd_sys_app_path, rel_gnd_path, file)
                    os.makedirs(os.path.dirname(dst_pathfile), exist_ok=True)
                    copy_file = True
                    if os.path.exists(dst_pathfile):
                        overwrite = sg.popup_yes_no(f'{dst_pathfile} exists.\nDo you want to overwrite it?',  title='Create Project')
                        copy_file = (overwrite == 'Yes')
                    if copy_file:
                        shutil.copyfile(src_pathfile, dst_pathfile)
    """
                        
###############################################################################

class JsonTblTopicMap():
    """
    KIT_SCH and KIT_TO use a Topic ID mapping scheme in their JSON tables
    that allow their table parsing code to use a generic table field
    name like "topic-id" that is mapped to a specifc Topic ID in it's
    topic ID mapping JSON object.
    
    The replace_spare_topics() and restore_spare_topics() methods support both
    KIT_SCH and KIT_TO. 
    
    "topic-id-map": { 
       "start": true,
       "ES_HK_TLM_TOPICID"       : "topic-id-0",
       ...
       "USER_70_TOPICID"   : "topic-id-70",
       "USER_71_TOPICID"   : "topic-id-71",
       ...
      "end": true
    },
    
    A packet table entry
      {"packet": {
         "name": "USER_TOPICID",
         "topic-id-83": 0,
         "topic-id": 0,
         "priority": 0,
         "reliability": 0,
         "buf-limit": 4,
         "filter": { "type": 2, "X": 1, "N": 1, "O": 0}
      }},

    This class allows uers to replace spare entries with topic ID names
    and restore spare IDs.
    """
    
    # App IDs used to distinguish which mapping table is being modified
    
    APP_KIT_SCH = 1
    APP_KIT_TO  = 2
    
    USRMSG_UNUSED_PERIOD_ID = 99
    
    # JSON keywords
    JSON_TOPIC_ID_MAP     = 'topic-id-map'
    JSON_TOPIC_ID_MAP_END = 'end'
    JSON_TOPIC_ID_PREFIX  = 'topic-id-'
    JSON_TOPIC_NAME       = 'name'
    
    JSON_USER_TOPIC_PREFIX   = 'USER_'
    JSON_USER_TOPIC_NAME     = 'USER_TOPICID'
    
    JSON_ARRAY_TOPIC_KEYWORD  = 'topic-id'
    JSON_ARRAY_PERIOD_KEYWORD = 'period-id'
    
    def __init__(self, json_file, array_name, array_element):
        self.file = json_file
        self.load_json(json_file)
        self.array_name    = array_name
        self.array_element = array_element
        
    def load_json(self, json_file):
        f = open(json_file)
        self.json = json.load(f)
        f.close()
        self.topic_map = self.json[self.JSON_TOPIC_ID_MAP]

    def spare_topics(self):
        spare_topics = [topic for topic in self.topic_map if self.JSON_USER_TOPIC_PREFIX in topic]
        return spare_topics
        
    def replace_spare_topics(self, new_topics):
        """
        The JSON dictionary doesn't maintain positional order when dictionary entries
        are replaced so the JSON file is directly changed for the topic-id-map
        updates. After the topic-id-map is updated the file contents are read into a
        JSON dictionary to replace the spare topic names in the message array.
        
        File is written only if the entire new_topic list is replaced
        Assumes
        - Spare topic IDs are at the end of the topic map object
        - Topic map object is before the array object using the mapped topic IDs
        - JSON file uses the JSON constants defined at the app level 
        - JSON_TOPIC_NAME object is followed by JSON_TOPIC_ID_PREFIX object 
        """
        spare_topics = self.spare_topics()
        if len(new_topics) == 0 or len(new_topics) > len(spare_topics):
           return False
        
        replaced_tids = {}  # Topic IDs replaced, created as each TID replaced 
        replaced_list = False
        instantiated_text = ""
        with open(self.file) as f:
            for line in f:
                if not replaced_list:
                    if ':' in line:
                        keyword     = line.split(':')
                        keyword_str = keyword[0].strip().strip('\"')
                        keyword_val = keyword[1].strip().strip('\",\n')
                        # Skip topics already in JSON file                        
                        if keyword_str in new_topics:
                            #~print('Found existing keyword: ',keyword_str)
                            del new_topics[new_topics.index(keyword_str)]
                            if len(new_topics) == 0:
                               replaced_list = True
                        elif  keyword_str.startswith(self.JSON_USER_TOPIC_PREFIX):
                            # Logic has a hole in it if new_topic[0] occurs later in the JSON file
                            line = f'      "{new_topics[0]}": "{keyword_val}",\n'
                            replaced_tids[keyword_val] = new_topics[0]   # Save topic names for later substitution
                            del new_topics[0]
                            if len(new_topics) == 0:
                               replaced_list = True

                instantiated_text += line
        
        if replaced_list:
            replaced_tid = False
            json_text = json.loads(instantiated_text)
            for obj in json_text[self.array_name]:
                for tid in replaced_tids:
                    if tid in obj[self.array_element]:
                        obj[self.array_element][self.JSON_TOPIC_NAME] = replaced_tids[tid]
                        replaced_tid = True
                        
            if replaced_tid:
                instantiated_text = json.dumps(json_text,indent=3)
            
            with open(self.file, 'w') as f:
                f.write(instantiated_text)
            
        self.load_json(self.file)
        return True
        
    def restore_spare_topics(self, remove_topics, app):
        """
        This function is used to retore KIT_TO's packet table and KIT_SCH's user
        message table.
        
        The JSON dictionary doesn't maintain positional order when dictionary entries
        are replaced so the JSON file is directly changed.
        
        File is written only if the entire new_topic list is replaced.
        """
        if len(remove_topics) == 0:
           return False
        
        array_kw_indent = '            '
        
        # Copy used for removing topic names in array
        remove_topic_names = remove_topics.copy() 

        removed_list    = False
        restoring_entry = False
        restored_kw_cnt = 0
        instantiated_text = ""
        with open(self.file) as f:
            for line in f:
                if not removed_list:
                    if ':' in line:
                        keyword     = line.split(':')
                        keyword_str = keyword[0].strip().strip('"')           
                        if keyword_str in remove_topics:
                            remove_idx = remove_topics.index(keyword_str)
                            #~print(f'Keyword: {keyword_str} at remove_list index {remove_idx}')
                            spare_id = re.search(r'\d+', keyword[1]).group()
                            #~print(f'spare_id: {spare_id}')
                            spare_key = f'USER_{spare_id}_TOPICID'
                            line = f'      "{spare_key}":{keyword[1]}'
                            #~print('newline: ', line)
                            del remove_topics[remove_idx]
                            if len(remove_topics) == 0:
                                removed_list = True
                else:
                                            
                    if ':' in line:                    
                        keyword = line.split(':')
                        keyword_str = keyword[0].strip().strip('\"')
                        keyword_val = keyword[1].strip().strip('\",\n')                
                        if restoring_entry:
                            """
                            This logic assumes the entry orders is
                                message -> topic-id(two entries) -> period-id
                            
                            """
                            if self.JSON_ARRAY_TOPIC_KEYWORD in keyword_str:
                                line = f'{array_kw_indent}"{keyword_str}": 0,\n'
                                restored_kw_cnt += 1
                            
                            if app == self.APP_KIT_TO:
                                if restored_kw_cnt == 2:
                                    restoring_entry = False
                                    restored_kw_cnt = 0
                            else:
                                if self.JSON_ARRAY_PERIOD_KEYWORD in keyword_str:
                                    #~print("Restoring period-id")
                                    line = f'{array_kw_indent}"{keyword_str}": {self.USRMSG_UNUSED_PERIOD_ID}\n'
                                    restoring_entry = False
                                    restored_kw_cnt = 0
                        
                        elif self.JSON_TOPIC_NAME in keyword_str:
                           for topic_name in remove_topic_names:
                               if topic_name in keyword_val:
                                   #~print('orgline: ', line)
                                   line = f'{array_kw_indent}"{keyword_str}":"{self.JSON_USER_TOPIC_NAME}",\n'
                                   #~print('newline: ', line)
                                   restoring_entry = True
                                   restored_kw_cnt = 0
                
                instantiated_text += line
        #~print(instantiated_text)
        if removed_list:
            with open(self.file, 'w') as f:
                f.write(instantiated_text)
       
        self.load_json(self.file)
        return True

    def update_usrmsg_periods(self, new_topic_dict):
        """
        This method is only used to update KIT_SCH's User Message table and 
        must be called after replace_spare_topics() has been called. The
        new_topic_dict parameter should have keys that are the same as the
        list passed to replace_spare_topics().
        
        Logic assumes there are no JSON syntax errors, "name" comes before
        JSON_ARRAY_PERIOD_KEYWORD and JSON_ARRAY_PERIOD_KEYWORD is last.
        "message": {
            "name": "USER_TOPICID",
            "id": 8,
            "topic-id-8": 0,
            "topic-id": 0,
            "period-id": 99
        }
        """
        #~print(f'update_usrmsg_periods(new_topic_dict): {new_topic_dict}')
        number_of_topics = len(new_topic_dict)
        topic_count = 0
        instantiated_text = ""
        
        period_id = self.USRMSG_UNUSED_PERIOD_ID
        with open(self.file) as f:
            for line in f:
                if topic_count < number_of_topics:
                    if ':' in line:
                        keyword     = line.split(':')
                        keyword_str = keyword[0].strip().strip('\"')
                        keyword_val = keyword[1].strip().strip('\",\n')
                        if keyword_str == self.JSON_TOPIC_NAME and keyword_val in new_topic_dict:
                            period_id = new_topic_dict[keyword_val]
                        elif period_id != self.USRMSG_UNUSED_PERIOD_ID and keyword_str == self.JSON_ARRAY_PERIOD_KEYWORD:
                            line = f'            "{self.JSON_ARRAY_PERIOD_KEYWORD}": {period_id}\n' # Assumes period-id is the last object in message (no comma at end)
                            topic_count += 1
                            period_id == self.USRMSG_UNUSED_PERIOD_ID
                instantiated_text += line
                
        with open(self.file, 'w') as f:
            f.write(instantiated_text)
            
        self.load_json(self.file)
        
        return True


###############################################################################

if __name__ == '__main__':

    cfs_path_mgr = CfsPathMgr()
    json_topic_file = cfs_path_mgr.get_file(CfsPath.CFS_TARGET_DEFS, f'{self.cfs_path_mgr.get_cpu_name()}_{BcPathConst.KIT_TO_PKT_TBL}')
    print(f'json_topic_file" {json_topic_file}')
    
    #TODO: json_topic_map = JsonTblTopicMap(json_topic_file)
    #json_topic_map.replace_spare_topics(['replace-1', 'replace-2'])
    #json_topic_map.restore_spare_topics(['replace-1', 'replace-2'])
    
    
