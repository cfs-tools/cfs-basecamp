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
        Provide JSON base class.
"""

from abc import ABC, abstractmethod
import sys
import time
import os
import json
import re

from datetime import datetime


###############################################################################

class JsonFile():
    """
    Abstract base class to manage JSON files used to store state in a consistent
    manner. This is designed for Basecamp as opposed to general utility so it's up
    to the user to protect against errors. JSON key constants should all be used
    within the Json classes to localize impacts due to JSON key changes.
    """
    def __init__(self, json_file):

        self.file = json_file
        f = open(json_file)
        self.json = json.load(f)
        f.close()
        
    def title(self):
        return self.json['title']

    def objective(self):
        return self.json['objective']

    def description(self):
        return self.json['description']

    def reset(self):
        """
        Reset only applies when a JSON file is being used to maintain state
        across executions.
        """
        self.json['time-stamp'] = ""
        self.reset_child()
        self.write_file()

    @abstractmethod
    def reset_child(self):
        raise NotImplementedError
            
    def short_description(self):
        return self.json['short-descr']

    def update(self):
        self.json['time-stamp'] = datetime.now().strftime("%m/%d/%Y-%H:%M:%S")
        self.write_file()
    
    def write_file(self):
        with open(self.file, "w") as outfile:
            json.dump(self.json, outfile, indent=2)
    

###############################################################################

class JsonTblTopicMap():
    """
    TODO: Generalize from hard coded KIT_TO JSON packet-array structure 
    KTI_SCH and KIT_TO use a Topic ID mapping scheme in their JSON tables
    that allow their table parsing code to use a generic table field
    name like "topic-id" that is mapped to a specifc Topic ID in it's
    topic ID mapping JSON object.
    
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
    # JSON keywords
    JSON_TOPIC_ID_MAP     = 'topic-id-map'
    JSON_TOPIC_ID_MAP_END = 'end'
    JSON_TOPIC_ID_PREFIX  = 'topic-id-'
    JSON_TOPIC_NAME       = 'name'
    
    JSON_USER_TOPIC_PREFIX  = 'USER_'
    JSON_USER_TOPIC_NAME    = 'USER_TOPICID'
    JSON_USER_TOPIC_KEYWORD = 'topic-id-'
    
    def __init__(self, json_file):
        self.file = json_file
        self.load_json(json_file)
        
    def load_json(self, json_file):
        f = open(json_file)
        self.json = json.load(f)
        f.close()
        self.topic_map = self.json[JsonTblTopicMap.JSON_TOPIC_ID_MAP]

    def spare_topics(self):
        spare_topics = [topic for topic in self.topic_map if JsonTblTopicMap.JSON_USER_TOPIC_PREFIX in topic]
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
        last_name_val = ""
        with open(self.file) as f:
            for line in f:
                if not replaced_list:
                    if ':' in line:
                        keyword     = line.split(':')
                        keyword_str = keyword[0].strip().strip('\"')
                        keyword_val = keyword[1].strip().strip('\",\n')
                        # Skip topics already in JSON file                        
                        if keyword_str in new_topics:
                            print('Found existing keyword: ',keyword_str)
                            del new_topics[new_topics.index(keyword_str)]
                            if len(new_topics) == 0:
                               replaced_list = True
                        elif  keyword_str.startswith(JsonTblTopicMap.JSON_USER_TOPIC_PREFIX):
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
            for obj in json_text["packet-array"]:
                for tid in replaced_tids:
                    if tid in obj["packet"]:
                        obj["packet"][JsonTblTopicMap.JSON_TOPIC_NAME] = replaced_tids[tid]
                        replaced_tid = True
                        
            if replaced_tid:
                instantiated_text = json.dumps(json_text,indent=3)
            
            with open(self.file, 'w') as f:
                f.write(instantiated_text)
            
        self.load_json(self.file)
        return True
        
    def restore_spare_topics(self, remove_topics):
        """
        The JSON dictionary doesn't maintain positional order when dictionary entries
        are replaced so the JSON file is directly changed.
        
        File is written only if the entire new_topic list is replaced
        """
        if len(remove_topics) == 0:
           return False
        
        # Copy used for removing topic names in array
        remove_topic_names = remove_topics.copy() 

        removed_list = False
        instantiated_text = ""
        with open(self.file) as f:
            for line in f:
                if not removed_list:
                    if ':' in line:
                        keyword     = line.split(':')
                        keyword_str = keyword[0].strip().strip('"')           
                        if keyword_str in remove_topics:
                            remove_idx = remove_topics.index(keyword_str)
                            print(f'Keyword: {keyword_str} at remove_list index {remove_idx}')
                            spare_id = re.search(r'\d+', keyword[1]).group()
                            print(f'spare_id: {spare_id}')
                            spare_key = f'USER_{spare_id}_TOPICID'
                            line = f'      "{spare_key}":{keyword[1]}'
                            print('newline: ', line)
                            del remove_topics[remove_idx]
                            if len(remove_topics) == 0:
                               removed_list = True
                else:
                    if ':' in line:
                        keyword = line.split(':')
                        keyword_str = keyword[0].strip().strip('\"')
                        keyword_val = keyword[1].strip().strip('\",\n')                
                        if JsonTblTopicMap.JSON_TOPIC_NAME in keyword_str:
                           for topic_name in remove_topic_names:
                               if topic_name in keyword_val:
                                   print('orgline: ', line)
                                   line = f'           "{keyword_str}":"{JsonTblTopicMap.JSON_USER_TOPIC_NAME}",\n'
                                   print('newline: ', line)
                
                instantiated_text += line
        
        if removed_list:
            with open(self.file, 'w') as f:
                f.write(instantiated_text)
       
        self.load_json(self.file)
        return True


###############################################################################

if __name__ == '__main__':

    json_topic_map = JsonTblTopicMap('../../../cfe-eds-framework/basecamp_defs/cpu1_kit_to_pkt_tbl.json')
    json_topic_map.replace_spare_topics(['replace-1', 'replace-2'])
    json_topic_map.restore_spare_topics(['replace-1', 'replace-2'])
    
    
