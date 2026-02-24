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
        Update JSON initialization files with topic IDs defined in
        the EDS topic ID XML file. 
        
    Notes:    
      1. The EDS topic ID definitions must follow coding conventions of
         adding an offset to a base topic value. See create_topic_dictionary()
         for assumptions.       
"""

import sys
import time
import os
import configparser
import json
import xml.dom.minidom
from datetime import datetime
import logging
logger = logging.getLogger(__name__)


###############################################################################

class CfeTopicIds():

    VERBOSE = False  # Output print messages

    # EDS Element & Attribute keywords
    EDS_TOPIC_ID_ELEMENT = 'Define'
    EDS_TOPIC_ID_NAME    = 'name'
    EDS_TOPIC_ID_VALUE   = 'value'
    EDS_CMD_TOPIC_ID_MISSION_REF = '${CFE_MISSION/TELECOMMAND_BASE_TOPICID}'
    EDS_TLM_TOPIC_ID_MISSION_REF = '${CFE_MISSION/TELEMETRY_BASE_TOPICID}'
    
    # JSON keywords
    JSON_TOPIC_ID_MAP     = 'topic-id-map'
    JSON_TOPIC_ID_MAP_END = 'end'
    JSON_TOPIC_ID_PREFIX  = 'topic-id-'
    
    def __init__(self, config_file):
        """
        """

        self.config = configparser.ConfigParser()
        self.config.read(config_file)
        
        self.cmd_topic_base = self.config.getint('EDS','CMD_TOPIC_BASE')
        self.tlm_topic_base = self.config.getint('EDS','TLM_TOPIC_BASE')
        self.topic_ids = {}
        
        logging.basicConfig(filename=self.config.get('APP','LOG_FILE'), filemode='w', level=logging.DEBUG)
        logging.info("***** cFE Topic IDs %s *****\n" % datetime.now().strftime("%m/%d/%y-%H:%M"))
                
    def debug_msg(self, message, verbose=VERBOSE):
        if verbose:
            print(message)
        
    def create_topic_dictionary(self):
        """
        Look for definitions with 
        <Define name="ES_CMD_TOPICID"     value="${CFE_MISSION/TELECOMMAND_BASE_TOPICID} + 0"/>
        <Define name="ES_HK_TLM_TOPICID"  value="${CFE_MISSION/TELEMETRY_BASE_TOPICID}   + 0"/>
        """
        eds_dom = xml.dom.minidom.parse(self.config.get('EDS','TOPIC_ID_FILE_NAME'))
        definitions = eds_dom.getElementsByTagName(self.EDS_TOPIC_ID_ELEMENT)
        for d in definitions:
            name  = d.getAttribute(self.EDS_TOPIC_ID_NAME)
            value = d.getAttribute(self.EDS_TOPIC_ID_VALUE)
            if '+' in value:
                val = value.split('+')
                topic_type   = val[0].strip()
                topic_offset = val[1].strip()
                if topic_type == self.EDS_CMD_TOPIC_ID_MISSION_REF:
                    if topic_offset.isnumeric():
                         self.topic_ids[name] = self.cmd_topic_base + int(topic_offset)
                elif topic_type == self.EDS_TLM_TOPIC_ID_MISSION_REF:
                    if topic_offset.isnumeric():
                        self.topic_ids[name] = self.tlm_topic_base + int(topic_offset)
                    
        self.debug_msg(str(self.topic_ids))
                    
    def update_json_topicids(self):
        dir_list   = os.listdir(os.getcwd())
        json_files = [filename for filename in dir_list if ".json" in filename]
        for file in json_files:
            self.debug_msg('\n*** Processing File ' + file)
            self.update_json_file(file)
       
    def update_json_file(self, filename):
        """
        There are two options for topic ID definitions:
        1. The app knows the name of the topic ID so it specifies it in it's
           JSON table. The topic ID must be defined on a single JSON line
           (spaces are ignored):
               "MQTT_GW_CMD_TOPICID" : 9999,
        2. The app doesn't know the topic ID names so a mapping must be 
           defined. The Kit_SCH app message table serves as an example. 
           "topic-id-map": { 
               "start": true,
               "ES_SEND_HK_TOPICID"   : "topic-id-0",      
               "end": true
           }
 
        """
        topic_id_map = False
        topic_id_map_start = False
        topic_id_pair = False
        topic_id_lookup = {}
        file_modified = False
        topic_id_updates = []
        instantiated_text = ""
        with open(filename) as f:
            for line in f:
                if ':' in line:
                    keyword = line.split(':')
                    keyword_str = keyword[0].strip().strip('"')
                    self.debug_msg('Keyword: ' + keyword_str)
                    if not topic_id_map_start:
                        topic_id_map_start = (keyword_str == self.JSON_TOPIC_ID_MAP)
                    if topic_id_map_start:
                        self.debug_msg("Map start")
                        if keyword_str == self.JSON_TOPIC_ID_MAP_END:
                            self.debug_msg("***MAP END***")
                            topic_id_map = True
                            topic_id_map_start = False
                            self.debug_msg(str(topic_id_lookup))
                        elif keyword_str in self.topic_ids:
                            topic_id_str = keyword[1].strip().strip(' ",')
                            self.debug_msg('Adding ' + topic_id_str)
                            topic_id_lookup[topic_id_str] = [keyword_str, self.topic_ids[keyword_str]]
                    else:
                        line_suffix = ',\n' if ',' in line else '\n' 
                        if topic_id_map:
                            self.debug_msg('>>> Mapping in progress <<<')
                            if topic_id_pair:
                                line = '%s: %s,\n' % (keyword[0], topic_id_value) # Value preserved from previous iteration                           
                                topic_id_pair = False
                            else:
                                if keyword_str in topic_id_lookup:
                                    topic_id_name  = topic_id_lookup[keyword_str][0]
                                    topic_id_value = topic_id_lookup[keyword_str][1]
                                    line = '%s: %s' % (keyword[0], topic_id_value) + line_suffix
                                    file_modified = True
                                    topic_id_updates.append('    %s : %d (mapped to %s)' % (keyword_str, topic_id_value, topic_id_name))
                                    self.debug_msg(f'Modified mapped line: {line.lstrip(' ').rstrip('\n')}', True)
                                    self.debug_msg(f'Modified mapped line: topic_id_name: {topic_id_name}, topic_id_value: {topic_id_value}\n', True)
                                    topic_id_pair = True
                        else:
                            if keyword_str in self.topic_ids:
                                line = '%s: %d' % (keyword[0], self.topic_ids[keyword_str]) + line_suffix
                                file_modified = True
                                topic_id_updates.append('    %s : %d' % (keyword_str, self.topic_ids[keyword_str]))
                                self.debug_msg(f'Modified unmapped line: {line.lstrip(' ')}',True)
                instantiated_text += line
        
        if file_modified:
            with open(filename, 'w') as f:
                f.write(instantiated_text)
            logging.info(filename+':')
            for update in topic_id_updates:
                logging.info(update)
            logging.info(' ')
            
            
    def execute(self):

        #try:
            self.create_topic_dictionary()
            self.update_json_topicids()
            logger.info('Successfully updated JSON topic IDs')
        #except:
        #    logger.error('Error updating JSON topic IDs')
            


###############################################################################

if __name__ == '__main__':
    """
    If no arguments then the working dir is basecamp_defs 
    If an argument is passed then it's the relative path of basecamp_defs from
    the current working directory
    """
    saved_dir = os.getcwd()
    if len(sys.argv) > 1:
        cwd = os.path.join(os.getcwd(), sys.argv[1]) 
        os.chdir(cwd)

    cfe_topic_ids = CfeTopicIds('cfe_topicids.ini')
    cfe_topic_ids.execute()

    os.chdir(saved_dir)
        

