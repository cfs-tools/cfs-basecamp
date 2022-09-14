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
        Provide Electronic Data Sheet support
"""

import sys
import os
import time
import configparser
import xml.dom.minidom
from datetime import datetime

if __name__ == '__main__':
    from utils  import compress_abs_path
else:
    from .utils import compress_abs_path


# cFE Topic ID EDS keywords

CFE_TOPICIDS_FILE = 'cfe-topicids.xml'

EDS_TAG_PACKAGE  = 'Package'
EDS_TAG_TOPIC_ID = 'Define'

EDS_ATTR_TOPIC_ID_NAME  = 'name'
EDS_ATTR_TOPIC_ID_DESCR = 'shortDescription'

SPARE_CMD_TOPIC_KEYWORD = 'SPARE_CMD_TOPICID_'
SPARE_TLM_TOPIC_KEYWORD = 'SPARE_TLM_TOPICID_'

# App Spec EDS keywords

EDS_TAG_INTERFACE            = 'Interface'
EDS_TAG_REQ_INTERFACE        = 'RequiredInterfaceSet'
EDS_ATTR_INTERFACE_NAME      = 'name'
EDS_ATTR_INTERFACE_TYPE      = 'type'
EDS_ATTR_INTERFACE_CMD_VAL   = 'CFE_SB/Telecommand'
EDS_ATTR_INTERFACE_TLM_VAL   = 'CFE_SB/Telemetry'

EDS_TAG_IMPLEMENTATION       = 'Implementation'
EDS_TAG_VARIABLE_SET         = 'VariableSet'
EDS_TAG_VARIABLE             = 'Variable'
EDS_ATTR_VARIABLE_NAME       = 'name'
EDS_ATTR_VARIABLE_INIT_VAL   = 'initialValue'

EDS_TAG_PARAM_MAP_SET        = 'ParameterMapSet'
EDS_TAG_PARAM_MAP            = 'ParameterMap'
EDS_ATTR_PARAM_MAP_INTERFACE = 'interface'
EDS_ATTR_PARAM_MAP_PARAM     = 'parameter'
EDS_ATTR_PARAM_MAP_TOPIC_ID  = 'TopicId'
EDS_ATTR_PARAM_MAP_VAR_REF   = 'variableRef'

###############################################################################

class CfeTopicIds():
    """
    Process the cfe-topicids.xml file
    """
    def __init__(self, topicid_path_file):

        self.topicid_path_file = topicid_path_file
        self.dom = xml.dom.minidom.parse(topicid_path_file)
        self.spare_cmd_topics = self.get_spare_topics(SPARE_CMD_TOPIC_KEYWORD)
        self.spare_tlm_topics = self.get_spare_topics(SPARE_TLM_TOPIC_KEYWORD)

    def spare_cmd_topic_cnt(self):
        return len(self.spare_cmd_topics)
        
    def spare_tlm_topic_cnt(self):
        return len(self.spare_tlm_topics)
        
    def get_topic_id_names(self):
        topic_id_names = []
        topic_elements = self.dom.getElementsByTagName(EDS_TAG_TOPIC_ID)
        for topic in topic_elements:
            topic_id_names.append(topic.getAttribute(EDS_ATTR_TOPIC_ID_NAME))
        return topic_id_names

    def get_spare_topics(self, spare_keyword):
        spare_topics = []
        topic_elements = self.dom.getElementsByTagName(EDS_TAG_TOPIC_ID)
        for topic in topic_elements:
            topic_id_name = topic.getAttribute(EDS_ATTR_TOPIC_ID_NAME)
            if spare_keyword in topic_id_name:
                spare_topics.append(topic)
        return spare_topics
              
    def replace_spare_cmd_topic(self, cmd_topic_id_name):
        topic_id_names = self.get_topic_id_names()
        if cmd_topic_id_name not in topic_id_names:
            if len(self.spare_cmd_topics) > 0:
                self.spare_cmd_topics[0].setAttribute(EDS_ATTR_TOPIC_ID_NAME, cmd_topic_id_name)
                del self.spare_cmd_topics[0]
        else:
            print(cmd_topic_id_name, ' already exists in the topic ID file')
                
    def restore_spare_cmd_topic(self, cmd_topic_id_name):
        """
        By convention the spare topic ID slots store the name of the spare ID in
        the short description attribute. 
        """
        topic_elements = self.dom.getElementsByTagName(EDS_TAG_TOPIC_ID)
        for topic in topic_elements:
            topic_id_name = topic.getAttribute(EDS_ATTR_TOPIC_ID_NAME)
            if cmd_topic_id_name in topic_id_name:
                descr = topic.getAttribute(EDS_ATTR_TOPIC_ID_DESCR)
                if len(descr) > 0:
                    topic.setAttribute(EDS_ATTR_TOPIC_ID_NAME, descr)
                    self.spare_cmd_topics = self.get_spare_topics(SPARE_CMD_TOPIC_KEYWORD)
                break

    def replace_spare_tlm_topic(self, tlm_topic_id_name):
        topic_id_names = self.get_topic_id_names()
        if tlm_topic_id_name not in topic_id_names:
            if len(self.spare_tlm_topics) > 0:
                self.spare_tlm_topics[0].setAttribute(EDS_ATTR_TOPIC_ID_NAME, tlm_topic_id_name)
                del self.spare_tlm_topics[0]
        else:
            print(tlm_topic_id_name, ' already exists in the topic ID file')
            
    def restore_spare_tlm_topic(self, tlm_topic_id_name):
        """
        By convention the spare topic ID slots store the name of the spare ID in
        the short description attribute. 
        """
        topic_elements = self.dom.getElementsByTagName(EDS_TAG_TOPIC_ID)
        for topic in topic_elements:
            topic_id_name = topic.getAttribute(EDS_ATTR_TOPIC_ID_NAME)
            if tlm_topic_id_name in topic_id_name:
                descr = topic.getAttribute(EDS_ATTR_TOPIC_ID_DESCR)
                if len(descr) > 0:
                    topic.setAttribute(EDS_ATTR_TOPIC_ID_NAME, descr)
                    self.spare_tlm_topics = self.get_spare_topics(SPARE_TLM_TOPIC_KEYWORD)
                break

    def prettier_xml(self, pretty_xml):
        """
        This cleans up minidom's toprettyxml(). It does not try to replace the
        pretty print, just remove some of the extra lines with spaces/tabs
        """
        prettier_xml = ''
        temp_str = ''
        analysis_state = 'pass_thru'
        for pretty_char in pretty_xml:
           if analysis_state == 'pass_thru':
               if pretty_char == '\n':
                   temp_str = pretty_char
                   analysis_state = 'first_newline'
               else:
                   prettier_xml += pretty_char
           elif analysis_state == 'first_newline':
               if pretty_char in (' ','\t'):
                   temp_str += pretty_char
               elif pretty_char == '\n':
                   temp_str += pretty_char
                   analysis_state = 'second_newline'
               else:
                   temp_str += pretty_char
                   prettier_xml += temp_str
                   temp_str = ''
                   analysis_state = 'pass_thru'
           elif analysis_state == 'second_newline':
               if pretty_char in (' ','\t'):
                   temp_str += pretty_char
               elif pretty_char == '\n':
                   temp_str = '\n'
                   analysis_state = 'first_newline'                   
               else:
                   temp_str += pretty_char
                   prettier_xml += temp_str
                   temp_str = ''
                   analysis_state = 'pass_thru'
        return prettier_xml
 
    def write_doc_to_file(self):
        pretty_xml   = self.dom.toprettyxml()
        prettier_xml = self.prettier_xml(pretty_xml) 
        with open(self.topicid_path_file, "w") as f:
            f.write(prettier_xml)

###############################################################################

class AppEds():
    """
    Process an app's EDS file
    """
    def __init__(self, app_spec_path_file):
        self.app_spec_path_file = app_spec_path_file
        self.doc = xml.dom.minidom.parse(app_spec_path_file)
        self.req_cmd_interface = self.get_req_interface(EDS_ATTR_INTERFACE_CMD_VAL)
        self.req_tlm_interface = self.get_req_interface(EDS_ATTR_INTERFACE_TLM_VAL)
        self.interface_to_topic_id = self.get_interface_to_topic_id()
        #print(str(self.req_cmd_interface))
        #print(str(self.req_tlm_interface))

    def cmd_topics(self):
        cmd_topics = [self.interface_to_topic_id[cmd_if] for cmd_if in self.req_cmd_interface]
        return cmd_topics
               
    def tlm_topics(self):
        tlm_topics = [self.interface_to_topic_id[tlm_if] for tlm_if in self.req_tlm_interface]
        return tlm_topics
               
    def get_req_interface(self, interface_type):
        req_interface_list = []
        req_interface = self.doc.getElementsByTagName(EDS_TAG_REQ_INTERFACE)
        req_interfaces = req_interface[0].getElementsByTagName(EDS_TAG_INTERFACE)
        for req_interface in req_interfaces:
            req_interface_name = req_interface.getAttribute(EDS_ATTR_INTERFACE_NAME)
            req_interface_type = req_interface.getAttribute(EDS_ATTR_INTERFACE_TYPE)
            if interface_type in req_interface_type:
                req_interface_list.append(req_interface_name)
        return req_interface_list
        
    def get_interface_to_topic_id(self):
        var_ref_to_interface = {}
        interface_to_topic_id = {}
        implementation = self.doc.getElementsByTagName(EDS_TAG_IMPLEMENTATION)
        
        param_map_set = implementation[0].getElementsByTagName(EDS_TAG_PARAM_MAP_SET)
        param_map = param_map_set[0].getElementsByTagName(EDS_TAG_PARAM_MAP)
        for param in param_map:
            interface = param.getAttribute(EDS_ATTR_PARAM_MAP_INTERFACE)
            parameter = param.getAttribute(EDS_ATTR_PARAM_MAP_PARAM)
            var_ref   = param.getAttribute(EDS_ATTR_PARAM_MAP_VAR_REF)
            if EDS_ATTR_PARAM_MAP_TOPIC_ID in parameter:
                var_ref_to_interface[var_ref] = interface

        variable_set = implementation[0].getElementsByTagName(EDS_TAG_VARIABLE_SET)
        variables = variable_set[0].getElementsByTagName(EDS_TAG_VARIABLE)
        for variable in variables:
            name = variable.getAttribute(EDS_ATTR_VARIABLE_NAME)
            if name in var_ref_to_interface:
                initial_value = variable.getAttribute(EDS_ATTR_VARIABLE_INIT_VAL)
                initial_value = initial_value.split('/')[1].replace('}','')
                interface_to_topic_id[var_ref_to_interface[name]] = initial_value
        
        print(str(interface_to_topic_id))
        return interface_to_topic_id

###############################################################################

if __name__ == '__main__':

    config = configparser.ConfigParser()
    config.read('../basecamp.ini')

    # basecamp.ini assumes cwd of basecamp 
    basecamp_defs_path = config.get('PATHS','BASECAMP_DEFS_PATH')
    eds_defs_path = compress_abs_path(os.path.join(os.getcwd(),'..', basecamp_defs_path, 'eds')) 
    cfe_topic_ids_path_file = os.path.join(eds_defs_path, CFE_TOPICIDS_FILE) 
    print ("cfe_topic_ids_path_file: ", cfe_topic_ids_path_file)
    basecamp_apps_path = config.get('PATHS','BASECAMP_APPS_PATH')
    file_mgr_eds_path = compress_abs_path(os.path.join(os.getcwd(),'..', basecamp_apps_path, 'file_mgr', 'eds')) 
    file_mgr_eds_path_file = os.path.join(file_mgr_eds_path, 'file_mgr.xml') 
    print ("file_mgr EDS spec: ", file_mgr_eds_path_file,'\n')

    cfe_topic_ids = CfeTopicIds(cfe_topic_ids_path_file)
    file_mgr_eds  = AppEds(file_mgr_eds_path_file)
   
    for cmd in file_mgr_eds.cmd_topics():
        cfe_topic_ids.replace_spare_cmd_topic(cmd)
        print(cmd)
        
    for tlm in file_mgr_eds.tlm_topics():
        cfe_topic_ids.replace_spare_tlm_topic(tlm)
        print(tlm)

    cfe_topic_ids.write_doc_to_file()

    #print('TOPIC LIST:\n',cfe_topic_ids.get_topic_id_names(),'\n')
    print('SPARE CMD TOPIC LIST LEN: '+str(len(cfe_topic_ids.spare_cmd_topics)))
    print('SPARE TLM TOPIC LIST LEN: '+str(len(cfe_topic_ids.spare_tlm_topics)))
    #cfe_topic_ids.restore_spare_cmd_topic(test_cmd_topic_id_name)
    #cfe_topic_ids.restore_spare_tlm_topic(test_tlm_topic_id_name)
    

