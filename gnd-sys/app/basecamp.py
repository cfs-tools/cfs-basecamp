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
      Provide the main application for cFS Basecamp

    Notes:
      1. Assumes the exact same app name is used for
          - git repo,  FSW object, app directory 
      
"""
import os
import sys

#sys.path.append(r'../../cfe-eds-framework/build/exe/lib/python')
if 'LD_LIBRARY_PATH' not in os.environ:
    print("LD_LIBRARY_PATH not defined. Run setvars.sh to corrrect the problem")
    sys.exit(1)
    """
    This code fails due to permission errors. It's also dangerous because it attempts to restart
    the python interpreter with a new environment. That's why it's attempted immediately after the
    os and sys imports which define the minimal context to attempt the restart 
    os.environ['LD_LIBRARY_PATH'] = '../../cfe-eds-framework/build/exe/lib'
    try:
        os.execv(sys.argv[0], sys.argv)
    except:
        print("Auto define LD_LIBRARY_PATH failed")
        sys.exit(1)
    """
import inspect
import importlib
import ctypes
import time
import random
import socket
import configparser
import operator
import subprocess
import threading
import queue
import json
import signal
import webbrowser
import io
import shutil
from contextlib import redirect_stdout
from datetime import datetime
from enum import Enum

import logging
from logging.config import fileConfig
fileConfig('logging.ini')
logger = logging.getLogger(__name__)

import paho.mqtt.client as mqtt
import PySimpleGUI as sg

import EdsLib
import CFE_MissionLib
from cfsinterface import CmdTlmRouter, TargetControl
from cfsinterface import Cfe, EdsMission
from cfsinterface import TelecommandInterface, TelecommandScript
from cfsinterface import TelemetryMessage, TelemetryObserver, TelemetryQueueServer
from tools import CreateApp, ManageTutorials, crc_32c, datagram_to_str, compress_abs_path, TextEditor
from tools import AppStore, ManageUsrApps, AppSpec, CfeTopicIds, JsonTblTopicMap, PdfViewer, ManageCodeTutorials

# Shell script names should not change and are considered part of the application
# Therefore they can be defined here and not in a configuration file

DEFAULT_TARGET_NAME = 'cpu1'

SH_BUILD_CFS_TOPICIDS = './build_cfs_topicids.sh'
SH_MAKE_INSTALL_CFS   = './make_install.sh'
SH_STOP_CFS       = './stop_cfs.sh'
SH_START_CFS      = './start_cfs.sh'
SH_SUDO_START_CFS = './sudo_start_cfs.sh'

CFS_DEFS_FOLDER = 'basecamp_defs'

INSERT_KEYWORD = '!BASECAMP-INSERT!'


###############################################################################

class TelecommandGui(TelecommandInterface):
    """
    GUI to manage a user selecting and sending a single telecommand 
    """
    
    def __init__(self, mission, target, cmd_router_queue):
        super().__init__(mission, target, cmd_router_queue)

        """
        eds_mission -
        eds_target  - String containing the identical target name used in the EDS  
        host        - String containing the socket address, e.g. '127.0.0.1:1234'
        port        - Integer containing socket pManageUsrAppsort
        """
        self.NULL_CMD_STR = self.eds_mission.NULL_CMD_STR
        self.NULL_TLM_STR = self.eds_mission.NULL_TLM_STR
       
        self.UNDEFINED_CMD_LIST = [self.NULL_CMD_STR]

        self.PAYLOAD_ROWS, self.PAYLOAD_COLS, self.PAYLOAD_HEADINGS = 8, 3, ('Parameter Name','Type','Value',)
        self.PAYLOAD_INPUT_START = 2 # First row of input payloads (see SendCmd() payload_layout comment)
        self.PAYLOAD_TEXT_INPUT  = 'text'
        self.PAYLOAD_COMBO_INPUT = 'combo'
        
        self.PAYLOAD_NAME_IDX  = 0
        self.PAYLOAD_TYPE_IDX  = 1
        self.PAYLOAD_VALUE_IDX = 2
        self.PAYLOAD_INPUT_IDX = 3

        self.payload_struct = None        # Payload structure for the current command. None if no command selected
        self.payload_gui_entries = {}    
        """
        payload_gui_entries manages displaying and retrieving data from the GUI. The following methods
        methods manage the dictionary 
           1. create_payload_gui_entries()  - Creates initial dictionary from EDS information. gui_value & gui_value_key are null
           2. display_payload_gui_entries() - Sets gui_value_key as it builds the command's payload screen
           3. load_payload_entry_value()    - Called when a command is being built and sent. Uses gui_value_key to retrieve user input
        
        Example payload_gui_entries for FILE_MGR/SendDirListTlm_Payload:
        'DirName': 
        {
            'eds_entry': EdsLib.DatabaseEntry(self.mission_name,'BASE_TYPES/PathName'),
            'eds_name': 'Payload.DirName', 'gui_type': 'BASE_TYPES/PathName',
            'gui_value': [self.NULL_CMD_STR],
            'gui_input': 'text',
            'gui_value_key': '--'
         },
         'DirListOffset':
         {
            'eds_entry': EdsLib.DatabaseEntry(self.mission_name,'BASE_TYPES/uint16'),
            'eds_name': 'Payload.DirListOffset',
            'gui_type': 'BASE_TYPES/uint16',
            'gui_value': ['--'],
            'gui_input': 'text',
            'gui_value_key': '--'
         },
         'IncludeSizeTime':
         {
            'eds_entry': EdsLib.DatabaseEntry(self.mission_name,'FILE_MGR/BooleanUint16'),
            'eds_name': 'Payload.IncludeSizeTime',
            'gui_type': 'FILE_MGR/BooleanUint16',
            'gui_value': ['FALSE', 'TRUE'],
            'gui_input': 'combo',
            'gui_value_key': '--'}}
        """
        
        self.sg_values = None
        
    def create_payload_gui_entries(self, payload_struct):
        """
        Create a list of a command's payload entries from the EDS

        Inputs:
        payload_struct - The payload structure output from mission_db.GetPayload()
        """
        return_str = ""        
        if isinstance(payload_struct, dict):
            return_str = 'Recursively extracting dictionary'
            for item in list(payload_struct.keys()):
                return_str = self.create_payload_gui_entries(payload_struct[item])
        elif isinstance(payload_struct, list):
            return_str = 'Recursively extracting list'
            for item in payload_struct:
                return_str = create_payload_gui_entries(item)
        elif isinstance(payload_struct, tuple):
            logger.debug(f'TUPLEDATA: {str(payload_struct)}')
            return_str = "Extracting tuple entry: " + str(payload_struct)
            eds_name  = payload_struct[0]
            gui_name  = self.remove_eds_payload_name_prefix(eds_name)
            logger.debug(f'gui_name = {str(gui_name)}')
            eds_entry = payload_struct[1] 
            eds_obj   = eds_entry()
            logger.debug(f'eds_entry:\n {str(type(eds_entry))}')
            logger.debug(str(dir(eds_entry)))
            logger.debug(f'eds_obj:\n {str(type(eds_obj))}')
            logger.debug(str(dir(eds_obj)))
            eds_obj_list = str(type(eds_obj)).split(',')
            logger.debug(str(eds_obj_list))
            gui_type  = eds_obj_list[1].replace("'","").replace(")","")
            gui_input = self.PAYLOAD_TEXT_INPUT
            if payload_struct[2] == 'entry':
                gui_value = [self.NULL_CMD_STR]
            elif payload_struct[2] == 'enum':
                gui_input = self.PAYLOAD_COMBO_INPUT
                gui_value = []
                for enum_value in list(payload_struct[3].keys()):
                    gui_value.append(enum_value)
            else:
                return_str = f'Error extracting entries from payload structure tuple: {str(payload_struct)}'
            logger.debug(gui_type)
            self.payload_gui_entries[gui_name] = {'eds_entry': eds_entry, 'eds_name': eds_name,       
                                                  'gui_type': gui_type,   'gui_value': gui_value, 
                                                  'gui_input': gui_input, 'gui_value_key': self.NULL_CMD_STR}
            
        else:
            return_str = f'Error extracting entries from unkown payload structure instance type: {str(payload_struct)}'
        
        logger.debug(f'return_str: {return_str}')
        return return_str


    def display_payload_gui_entries(self):
        """
        See SendCmd() payload_layout definition comment for initial payload display
        When there are no payload parameters (zero length) hide all rows except the first parameter.
        """
        for row in range(self.PAYLOAD_ROWS):
            self.window[f'-PAYLOAD_{row}_NAME-'].update(visible=False)
            self.window[f'-PAYLOAD_{row}_TYPE-'].update(visible=False)
            self.window[f'-PAYLOAD_{row}_VALUE-'].update(visible=False, value=self.UNDEFINED_CMD_LIST[0])

        enum_row  = 0
        entry_row = self.PAYLOAD_INPUT_START
        row = 0

        if len(self.payload_gui_entries) > 0:
            for payload_gui_name in self.payload_gui_entries.keys():
                logger.debug(f'payload_gui_name = {payload_gui_name}')
                if self.payload_gui_entries[payload_gui_name]['gui_input'] == self.PAYLOAD_TEXT_INPUT:
                    row = entry_row
                    entry_row += 1                
                    self.window[f'-PAYLOAD_{row}_VALUE-'].update(visible=True, value=self.payload_gui_entries[payload_gui_name]['gui_value'][0])
                    
                else:
                    row = enum_row
                    enum_row += 1                
                    payload_enum_list = self.payload_gui_entries[payload_gui_name]['gui_value']
                    self.window[f'-PAYLOAD_{row}_VALUE-'].update(visible=True,value=payload_enum_list[0], values=payload_enum_list)
                
                self.payload_gui_entries[payload_gui_name]['gui_value_key'] = f'-PAYLOAD_{row}_VALUE-'
                self.window[f'-PAYLOAD_{row}_NAME-'].update(visible=True,value=payload_gui_name)
                self.window[f'-PAYLOAD_{row}_TYPE-'].update(visible=True,value=self.payload_gui_entries[payload_gui_name]['gui_type'])

        else:
            self.window["-PAYLOAD_0_NAME-"].update(visible=True, value='No Parameters')


    def load_payload_entry_value(self, payload_name, payload_eds_entry, payload_type, payload_list):
        """
        Virtual function used by base Telesommand class set_payload_values() to retrieve values
        from a derived class source: GUI or command line
        """
        logger.debug('load_payload_entry_value() - Entry')
        logger.debug(f'payload_name={payload_name}, payload_eds_entry={payload_eds_entry}, payload_type={payload_type}, payload_list={payload_list}')
        logger.debug(f'self.payload_gui_entries = {str(self.payload_gui_entries)}')
        logger.debug(f'self.sg_values = {str(self.sg_values)}')
        #todo: Add type check error reporting
        value_key = self.payload_gui_entries[self.remove_eds_payload_name_prefix(payload_name)]['gui_value_key']
        logger.debug(f'@@@@value_key = {value_key}')
        value = self.sg_values[value_key]
        return value
        
    def execute(self, topic_name, return_cmd=False):
        """
        The initial design always sent the command. New use cases required this function to
        return the cmd_obj rather than send it so the 'return_cmd ' parameter was added. If
        'return_cmd' is True then 'cmd_sent' should be interpretted as the command would have
        been sent therefore the command is valid.        
        """
        cmd_sent   = False
        cmd_text   = 'No command selected'
        cmd_status = f'Send {topic_name} command aborted'
        cmd_obj    = None

        topic_list = list(self.get_topics().keys())
        logger.debug("topic_list = " + str(topic_list))

        self.command_list = list(self.get_topic_commands(topic_name).keys())
        
        # This GUI is designed for application command topics. The ini file has a non-standard configuration that allows all
        # topics to be sent from the GUI. Topics like 'send HK req' do not have subcommands so this alerts the users 
        if (len(self.command_list) == 1):
            sg.Popup('This is a topic-only command do not select a command/payload', title=topic_name, keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
        
        # Create a layout with more than enough input and combo boxes. Then hide what's not needed for a particular command 
        # The top rows below self.PAYLOAD_INPUT_START are used for enumerated types with combo boxes and the remaining rows
        # are input boxes
        #todo: Had some GUI alignment issues with hiding. See https://github.com/PySimpleGUI/PySimpleGUI/issues/1154
        #todo: Other payload ideas: Create a new send command window. Search command list and tailor max input/combo to specific topic
        row_font = 'Courier 12'
        row_title_font = 'Courier 12 bold'
        row_label_size = (20,1)
        row_input_size = (20,1)
        self.payload_layout = [[sg.Text(heading, font=row_title_font, size=row_label_size) for i, heading in enumerate(self.PAYLOAD_HEADINGS)]]
        for row in range(self.PAYLOAD_ROWS):
            if row < self.PAYLOAD_INPUT_START:
                self.payload_layout += [[sg.pin(sg.Text('Name', font=row_font, size=row_label_size, key="-PAYLOAD_%d_NAME-"%row, visible=False))] + [sg.pin(sg.Text('Type', font=row_font, size=row_label_size, key="-PAYLOAD_%d_TYPE-"%row, visible=False))] + [sg.pin(sg.Combo((self.UNDEFINED_CMD_LIST), font=row_font, size=row_input_size, enable_events=True, key="-PAYLOAD_%d_VALUE-"%row, default_value=self.UNDEFINED_CMD_LIST[0], visible=False))]]
            else:
                self.payload_layout += [[sg.pin(sg.Text('Name', font=row_font, size=row_label_size, key="-PAYLOAD_%d_NAME-"%row, visible=False))] + [sg.pin(sg.Text('Type', font=row_font, size=row_label_size, key="-PAYLOAD_%d_TYPE-"%row, visible=False))] + [sg.pin(sg.Input(self.UNDEFINED_CMD_LIST[0], font=row_font, size=row_input_size, enable_events=True, key="-PAYLOAD_%d_VALUE-"%row, visible=False))]]

            
        #todo: [sg.Text('Topic', size=(10,1)),  sg.Combo((topic_list),   size=(40,1), enable_events=True, key='-TOPIC-',   default_value=topic_list[0])], 
        self.layout = [
                      [sg.Text('Command',size=(10,1)), sg.Combo((self.command_list), size=(40,1), enable_events=True, key='-COMMAND-', default_value=self.command_list[0])],
                      [sg.Text(' ')],
                      [sg.Col(self.payload_layout, size=(650, 250), scrollable=True, element_justification='l', expand_x=True, expand_y=True)],
                      [sg.Text(' ')],
                      [sg.Button('Send', enable_events=True, key='-SEND_CMD-',pad=((0,10),(1,1))), sg.Exit()]
                      ]
                 
        self.window = sg.Window(f'Send {topic_name} Telecommand' , self.layout, element_padding=(1,1), default_element_size=(20,1)) #TODO - default_element_size=(14,1),  return_keyboard_events=True
                
        while True:
        
            self.event, self.values = self.window.read(timeout=100)
            logger.debug(f'Command Window Read()\nEvent: {self.event}\nValues: {self.values}')

            self.sg_values = self.values

            if self.event in (sg.WIN_CLOSED, 'Exit') or self.event is None:       
                break
            
            logger.debug(f'Matching event {self.event}')
            cmd_name   = self.values['-COMMAND-']
            logger.debug(f'Topic: {topic_name}, Command: {cmd_name}')
            
            if self.event == '-COMMAND-':
                if cmd_name != self.eds_mission.COMMAND_TITLE_KEY:
                    
                    cmd_id, cmd_text = self.get_cmd_id(cmd_name)
                    
                    if cmd_id !=0:
                       
                        cmd_valid, cmd_entry, cmd_obj = self.get_cmd_entry(topic_name, cmd_name)
                        cmd_has_payload, cmd_payload_item = self.get_cmd_entry_payload(cmd_entry)
                        logger.debug(f'self.cmd_entry = {str(cmd_entry)}')
                        logger.debug(f'self.cmd_obj = {str(cmd_obj)}')
                        logger.debug(f'cmd_payload_item = {str(cmd_payload_item)}')
    
                        self.payload_struct = None 
                        self.payload_gui_entries = None
                        
                        self.payload_gui_entries = []
                        if cmd_has_payload:
            
                            payload_entry = self.eds_mission.get_database_named_entry(cmd_payload_item[2])
                            payload = payload_entry()
                            self.payload_struct = self.get_payload_struct(payload_entry, payload, 'Payload')
                            logger.debug(f'payload_entry = {str(payload_entry)}')
                            logger.debug(f'payload = {str(payload)}')
                            self.payload_gui_entries = {}
                            status_str = self.create_payload_gui_entries(self.payload_struct)
                            logger.debug(f'status_str = {status_str}')
                            logger.debug(f'self.payload_gui_entries: {str(self.payload_gui_entries)}')
                            if len(self.payload_gui_entries) > 0:
                                self.display_payload_gui_entries()
                            else:
                                cmd_text = f'Error extracting payload parameters from {str(self.payload_struct)}'
                        else:
                            self.display_payload_gui_entries()
                
            if self.event == '-SEND_CMD-':

                if topic_name == self.eds_mission.TOPIC_CMD_TITLE_KEY:
                    cmd_text  = 'Please select a topic before sending a command'
                    break
                    
                if (cmd_name == self.eds_mission.COMMAND_TITLE_KEY and len(self.command_list) > 1):
                    cmd_text  = 'Please select a command before sending a command'
                    break
                
                topic_id, topic_text = self.get_topic_id(topic_name)
                
                cmd_valid, cmd_entry, cmd_obj = self.get_cmd_entry(topic_name, cmd_name)

                if cmd_valid == True:
    
                    logger.debug(f'self.cmd_entry = {str(cmd_entry)}')
                    logger.debug(f'self.cmd_obj = {str(cmd_obj)}')

                    self.set_cmd_hdr(topic_id, cmd_obj)

                    cmd_has_payload, cmd_payload_item = self.get_cmd_entry_payload(cmd_entry)
                    logger.debug(f'cmd_payload_item = {str(cmd_payload_item)}')
                    
                    send_command = True
                    if cmd_has_payload:
            
                        try:
                            # Use the information from the database entry iterator to get a payload Entry and object
                            logger.debug(f'cmd_payload_item[1] = {str(cmd_payload_item[1])}')
                            logger.debug(f'cmd_payload_item[2] = {str(cmd_payload_item[2])}')
                            #todo: payload_entry = self.eds_mission.lib_db.DatabaseEntry(cmd_payload_item[1], cmd_payload_item[2])
                            payload_entry = self.eds_mission.get_database_named_entry(cmd_payload_item[2])
                            payload = payload_entry()
                            logger.debug(f'payload_entry = {str(payload_entry)}')
                            logger.debug(f'payload = {str(payload)}')

                            #payload = EdsLib.DatabaseEntry(self.EDS_MISSION_NAME,'FILE_MGR/SendDirListTlm_Payload')({'DirName': '', 'DirListOffset': 0, 'IncludeSizeTime': 'FALSE'})
                            #todo: Check if None? payload_struct = self.get_payload_struct(payload_entry, payload, 'Payload')
                            eds_payload = self.set_payload_values(self.payload_struct)
                            payload = payload_entry(eds_payload)                   
                            cmd_obj['Payload'] = payload
    
                        except:
                           send_command = False
                           cmd_status = f'{topic_name}/{cmd_name} command not sent. Error loading parameters from command window.'
                    
                    if send_command:
                        if return_cmd:
                            cmd_sent   = True
                            cmd_text   = 'Send command returned command object'
                            cmd_status = f'{topic_name}/{cmd_name} command object created'
                        else:
                            (cmd_sent, cmd_text) = self.send_command(cmd_obj)
                            if cmd_sent:
                                cmd_status = f'Sent {topic_name}/{cmd_name} command'
                    
                else:
                    popup_text = f'Error retrieving command {cmd_name} using topic ID {topic_id}' 
                    sg.popup(popup_text, title='Send Command Error', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)            


                # Keep GUI active if a command error occurs to allow user to fix and resend or cancel
                if cmd_sent:
                    break
                    
        self.window.close()

        return (cmd_sent, cmd_text, cmd_status, cmd_obj)


###############################################################################

class BasecampTelemetryMonitor(TelemetryObserver):
    """
    callback_functions
       [app_name] : {packet: [item list]} 
    
    """

    def __init__(self, tlm_server: TelemetryQueueServer, tlm_monitors, tlm_callback, event_queue):
        super().__init__(tlm_server)

        self.tlm_monitors = tlm_monitors
        self.tlm_callback = tlm_callback
        self.event_queue  = event_queue
        
        self.sys_apps = ['CFE_ES', 'CFE_EVS', 'CFE_SB', 'CFE_TBL', 'CFE_TIME', 'APP_C_DEMO' 'FILE_MGR' 'FILE_XFER']
        
        for msg in self.tlm_server.tlm_messages:
            tlm_msg = self.tlm_server.tlm_messages[msg]
            if tlm_msg.app_name in self.sys_apps:
                self.tlm_server.add_msg_observer(tlm_msg, self)        
                logger.info(f'Basecamp telemetry adding observer for {tlm_msg.app_name}: {tlm_msg.msg_name}')
        

    def update(self, tlm_msg: TelemetryMessage) -> None:
        """
        Receive telemetry updates
        """
        #todo: Determine best tlm identification method: if int(tlm_msg.app_id) == int(self.cfe_es_hk.app_id):
        try:
            if tlm_msg.app_name in self.tlm_monitors:
                self.tlm_callback(tlm_msg.app_name, tlm_msg.msg_name, 'Seconds', str(tlm_msg.sec_hdr().Seconds))

            elif tlm_msg.app_name == 'CFE_EVS':
                if tlm_msg.msg_name == 'LONG_EVENT_MSG':
                    payload = tlm_msg.payload()
                    pkt_id = payload.PacketID
                    event_text = f'FSW Event at {str(tlm_msg.sec_hdr().Seconds)}: {pkt_id.AppName}, {pkt_id.EventType} - {payload.Message}'
                    self.event_queue.put_nowait(event_text)
                    """        
                    LongEventTlm.Payload.PacketID.AppName                        = CFE_TIME
                    LongEventTlm.Payload.PacketID.EventID                        = 20
                    LongEventTlm.Payload.PacketID.EventType                      = 2
                    LongEventTlm.Payload.PacketID.SpacecraftID                   = 66
                    LongEventTlm.Payload.PacketID.ProcessorID                    = 1
                    LongEventTlm.Payload.Message  
                    """
        except Exception as e:
            logger.error(f'Telemetry update exception\n{str(e)}')


###############################################################################

class ManageCfs():
    """
    Manage the display for configuring, building and running the cFS.
    app_abs_path is the python application path, not cFS apps
    #TODO - Define path and file constants
    """
    def __init__(self, basecamp_abs_path, cfs_abs_base_path, usr_app_rel_path, main_window, cfs_target):
        self.basecamp_abs_path      = basecamp_abs_path
        self.cfs_abs_base_path      = cfs_abs_base_path
        self.cfs_abs_defs_path      = os.path.join(self.cfs_abs_base_path, 'basecamp_defs')
        self.basecamp_tools_path    = os.path.join(basecamp_abs_path, 'tools')
        self.usr_app_path           = compress_abs_path(os.path.join(basecamp_abs_path, usr_app_rel_path))
        self.main_window            = main_window
        self.cfs_target             = cfs_target
        self.startup_scr_filename   = cfs_target + '_' + 'cfe_es_startup.scr'
        self.startup_scr_file       = os.path.join(self.cfs_abs_defs_path, self.startup_scr_filename)
        self.targets_cmake_filename = 'targets.cmake'
        self.targets_cmake_file     = os.path.join(self.cfs_abs_defs_path, self.targets_cmake_filename)
        self.cfe_topic_id_filename  = 'cfe-topicids.xml'
        self.cfe_topic_id_file      = os.path.join(self.cfs_abs_defs_path, 'eds', self.cfe_topic_id_filename)
        self.kit_to_tbl_filename    = cfs_target + '_' + 'kit_to_pkt_tbl.json'
        self.kit_to_tbl_file        = os.path.join(self.cfs_abs_defs_path, self.kit_to_tbl_filename)
        self.cmake_app_list         = cfs_target + '_APPLIST'
        self.cmake_file_list        = cfs_target + '_FILELIST'
        self.build_subprocess       = None
        self.selected_app           = None
        self.usr_app_spec           = None
        
        self.b_size  = (4,1)
        self.b_pad   = ((0,2),(2,2))
        self.b_font  = ('Arial bold', 11)
        self.b_color = 'black on LightSkyBlue3'
        self.t_size  = (2,1)
        self.t_font  = ('Arial', 12)
        self.step_font  = ('Arial bold', 14)

    def select_usr_app_gui(self, app_name_list, action_text):
        """
        Select an app to be integrated. The action text should be lower case.
        """
        self.selected_app = None
        
        layout = [
                  [sg.Text(f'Select app to {action_text} from the dropdown iist and click <Submit>\n', font=self.b_font)],
                  [sg.Combo(app_name_list, pad=self.b_pad, font=self.b_font, enable_events=True, key="-USR_APP-", default_value=app_name_list[0]),
                   sg.Button('Submit', button_color=('SpringGreen4'), pad=self.b_pad, key='-SUBMIT-'),
                   sg.Button('Cancel', button_color=('gray'), pad=self.b_pad, key='-CANCEL-')]
                 ]      

        window = sg.Window('Select User App', layout, resizable=True, modal=True)
        
        while True:
        
            event, values = window.read(timeout=200)
        
            if event in (sg.WIN_CLOSED, '-CANCEL-') or event is None:
                break
                
            elif event == '-SUBMIT-':
                self.selected_app = values['-USR_APP-']
                break
        
        window.close()       
              
    
    def add_usr_app_gui(self, usr_app_list):
        """
        Provide steps for the user to integrate an app. Allow the user to add
        multiple apps before moving onto the build step. 
        The steps have some degree of independence in case the user doesn't do
        things in order which means some processing may be repeated. For example
        the table files are recomputed for the edit targets.cmake step and the
        copy files to cFS '_defs' steps. 
        """
        #TODO - Use a loop to construct the layout

        layout = [
                  [sg.Text("Perform the following steps to add one or more apps. For step 1, choose 'Auto' to automatically\nperform all of the steps or 'Man' to manually perform each step. Libraries MUST be added prior\nto the apps that depend upon it.\n", font=self.t_font)],
                  
                  [sg.Text('1. Add app to the cFS build configuration', font=self.step_font, pad=self.b_pad)],
                  [sg.Text('', size=self.b_size), sg.Combo(usr_app_list, pad=self.b_pad, font=self.b_font, enable_events=True, key="-USR_APP-", default_value=usr_app_list[0]),
                   sg.Text('Select an app from the dropdown list', font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Button('Auto', size=self.b_size, button_color=('SpringGreen4'), font=self.b_font, pad=self.b_pad, enable_events=True, key='-1_AUTO-'),
                   sg.Text('Automatically perform all steps or ...', font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Button('Man',  size=self.b_size, button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-1A_MAN-'),
                   sg.Text('Copy table files to %s' % CFS_DEFS_FOLDER, font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Button('Man',  size=self.b_size, button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-1B_MAN-'),
                   sg.Text("Update targets.cmake's %s and %s" % (self.cmake_app_list, self.cmake_file_list), font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Button('Man',  size=self.b_size, button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-1C_MAN-'),
                   sg.Text('Update cpu1_cfe_es_startup.scr', font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Button('Man',  size=self.b_size, button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-1D_MAN-'),
                   sg.Text('Update EDS cfe-topicids.xml', font=self.t_font)], 
                  [sg.Text('', size=self.b_size), sg.Button('Man',  size=self.b_size, button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-1E_MAN-'),
                   sg.Text('Update telemetry output app table', font=self.t_font)],
                  
                  [sg.Text('2. Build new cFS target', font=self.step_font, pad=self.b_pad)],
                  [sg.Text('', size=self.b_size), sg.Button('Build', size=self.b_size, button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-2_AUTO-')],
                  
                  [sg.Text('3. Stop the cFS if it is running', font=self.step_font, pad=self.b_pad)],
                  [sg.Text('', size=self.b_size), sg.Text('Close this window and click <Stop cFS> from the main window or ...', font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Text('Open a terminal window & kill the cFS process or ...', font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Text('Submit [sudo] password and click <Submit>', font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Button('Submit', size=(6,1), button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-3_AUTO-'),
                   sg.InputText(password_char='*', size=(15,1), font=self.t_font, pad=self.b_pad, key='-PASSWORD-')],

                  [sg.Text('4. Exit and restart Basecamp', font=self.step_font, pad=self.b_pad)],
                  [sg.Text('', size=self.b_size), sg.Button('Restart', size=(6,1), button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-4_AUTO-')],
                 ]
        # sg.Button('Exit', enable_events=True, key='-EXIT-')
        window = sg.Window(f'Add User Apps', layout, resizable=True, finalize=True) # modal=True)
        
        restart_main_window = False
        while True:
        
            self.event, self.values = window.read(timeout=200)
        
            if self.event in (sg.WIN_CLOSED, 'Exit', '-EXIT-') or self.event is None:
                break

            ## Step 1 - Update cFS build configuration with selected app
            
            elif self.event == '-1_AUTO-': # Autonomously perform step 1
                """
                Errors are reported in a popup by each function. The success string is an aggregate of each successful return
                that will be reported in a single popup. 
                A boolean return value of True from each function indicates there weren't any errors, it doesn't mean a paricular
                update was performed, because the update may not be required.
                """ 
                self.selected_app = self.values['-USR_APP-']
                self.usr_app_spec = self.manage_usr_apps.get_app_spec(self.selected_app)
                auto_popup_text = f"{self.selected_app.upper()} was successfully added to Basecamp's cFS target:\n\n"
                display_auto_popup = False 
                copy_tables_passed, copy_tables_text = self.copy_app_tables(auto_copy=True)
                if copy_tables_passed:
                    auto_popup_text += f'1. {copy_tables_text}\n\n' 
                    update_cmake_passed, update_cmake_text = self.update_targets_cmake(auto_update=True)
                    if update_cmake_passed:
                        auto_popup_text += f'2. {update_cmake_text}\n\n'
                        update_startup_passed, update_startup_text = self.update_startup_scr(auto_update=True)
                        if update_startup_passed:
                            auto_popup_text += f'3. {update_startup_text}\n\n'
                            if self.usr_app_spec.has_topic_ids():
                                update_topics_passed, update_topics_text = self.update_topic_ids()
                                auto_popup_text += f'4. {update_topics_text}\n\n'
                                display_auto_popup = update_topics_passed
                            else:
                                auto_popup_text += f"4. Topic IDs not updated since it's a library\n\n"
                                display_auto_popup = True
                if display_auto_popup:
                    sg.popup(auto_popup_text, title=f'Update {self.startup_scr_filename}', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
                                
                
            elif self.event == '-1A_MAN-':
                self.copy_app_tables(auto_copy=False)  # Copy table files from app dir to cFS '_defs' file
            elif self.event == '-1B_MAN-':
                self.update_targets_cmake(auto_update=False)
            elif self.event == '-1C_MAN-':
                self.update_startup_scr(auto_update=False)
            elif self.event == '-1D_MAN-':
                popup_text = f"After this dialogue, {self.cfe_topic_id_filename} will open in an editor.\n Replace spare topic IDs with the app's topic ID names"
                sg.popup(popup_text, title=f'Update {self.cfe_topic_id_filename}', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
                self.text_editor = sg.execute_py_file('texteditor.py', parms=self.cfe_topic_id_file, cwd=self.basecamp_tools_path)
            elif self.event == '-1E_MAN-':
                popup_text = f"After this dialogue, {self.kit_to_tbl_filename} will open in an editor.\n Replace spare topic IDs with the app's topic ID names"
                sg.popup(popup_text, title=f'Update {self.kit_to_tbl_filename}', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
                self.text_editor = sg.execute_py_file('texteditor.py', parms=self.kit_to_tbl_file, cwd=self.basecamp_tools_path)
                
            ## Step 2 - Build the cFS

            elif self.event == '-2_AUTO-': # Build the cFS
                build_cfs_sh = os.path.join(self.basecamp_abs_path, SH_BUILD_CFS_TOPICIDS)
                self.build_subprocess = subprocess.Popen(f'{build_cfs_sh} {self.cfs_abs_base_path}',
                                        stdout=subprocess.PIPE, shell=True, bufsize=1, universal_newlines=True)
                if self.build_subprocess is not None:
                    self.cfs_stdout = CfsStdout(self.build_subprocess, self.main_window)
                    self.cfs_stdout.start()
            
            elif self.event == '-2_MAN-': # Build the cFS
                popup_text = f"Open a terminal window, change directory to {self.cfs_abs_base_path} and build the cFS. See '{SH_BUILD_CFS_TOPICIDS}' for guidance"
                sg.popup(popup_text, title='Manually Stop the cFS', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)   

            ## Step 3 - Stop the cFS prior to modifying or adding an app
            
            elif self.event == '-3_AUTO-': # Stop the cFS prior to modifying or adding an app
                """
                #todo: 
                #1 - Current window blocked this from working.
                self.main_window['-STOP_CFS-'].click()
                #2 - Couldn't get blocking and tlm threading exceptions worked out
                layout = [[sg.Text("Enter [sudo] password", size=(20,1)), sg.InputText(password_char='*')],
                          [sg.Button("Submit"), sg.Button("Cancel")]]
                window = sg.Window("Stop cFS", layout) #, modal=True)
                event,values = window.read()
                window.close()
                password = values[0]
                status = subprocess.run(SH_STOP_CFS, shell=True, cwd=self.basecamp_abs_path, input=password.encode())
                """
                #popup_text = 'After you close this popup. Enter your sudo password in the terminal where you started cfs-basecamp'
                #sg.popup(popup_text, title='Automatically Stop the cFS', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=True)
                password = self.values['-PASSWORD-']
                if password is not None:
                    status = subprocess.run(SH_STOP_CFS, shell=True, cwd=self.basecamp_abs_path, input=password.encode())
                    print('status type = %s'%str(type(status.returncode)))
                    if status.returncode == 0:
                        popup_text = f"'{SH_STOP_CFS}' successfully executed with return status {status.returncode}"
                        self.main_window['-STOP_CFS-'].click()
                    else:
                        popup_text = f"'{SH_STOP_CFS}' returned with error status {status.returncode}"
                    sg.popup(popup_text, title='Stop the cFS', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
                else:
                    popup_text = 'No attempt to stop the cFS, since no password supplied'
                    sg.popup(popup_text, title='Stop the cFS', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
                            
            elif self.event == '-3_MAN-': # Stop the cFS prior to modifying or adding an app
                popup_text = f"Open a terminal window and kill any running cFS processes. See '{SH_STOP_CFS}' for guidance" 
                sg.popup(popup_text, title='Stop the cFS', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)

            ## Step 4 - Restart Basecamp

            elif self.event == '-4_AUTO-': # Reload cFS python EDS definitions                
                sg.popup(f'Basecamp will be closed after this dialogue.\nYou must restart Basecamp to use the new cFS target',
                         title='Reload cFS EDS definitions', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
                restart_main_window = True
                break
                
        window.close()       
        if restart_main_window:
            self.main_window['-RESTART-'].click()

    def remove_usr_app_gui(self):

        no_yes = ['No', 'Yes']
        layout = [
                  [sg.Text(f'This will remove {self.usr_app_spec.app_name.upper()} from the cFS target', font=self.t_font)],
                  [sg.Text('Do you want to remove the source files from usr/apps? ', font=self.t_font),
                  sg.Combo(no_yes, enable_events=True, key='-DELETE_FILES-', default_value=no_yes[0], pad=((0,5),(5,5)))], 
                  [sg.Text('', font=self.t_font)],
                  [sg.Button('Remove App', button_color=('SpringGreen4')), sg.Cancel(button_color=('gray'))]
                 ]
        
        window = sg.Window(f'Remove {self.usr_app_spec.app_name.upper()}', layout, resizable=True, finalize=True)
        while True: # Event Loop
            self.event, self.values = window.read()
            if self.event in (sg.WIN_CLOSED, 'Cancel') or self.event is None:       
                break
            if self.event == 'Remove App':
                self.remove_app_tables()
                self.restore_targets_cmake()
                self.restore_startup_scr()
                if self.usr_app_spec.has_topic_ids():
                    self.restore_topic_ids()                
                if self.values['-DELETE_FILES-'] == 'Yes':
                    self.remove_app_src_files()
                sg.popup(f'Successfully removed {self.selected_app.upper()}', title='Remove App', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
                break

        window.close()
        
    def execute(self, action):
        """
        This design has evolved and may need refactoring. Originally select_usr_app_gui()
        was used for both add and remove because only one add could be managed at a time.
        Add app now allows multiple apps to be selected before adding them. Remove only
        operates on a single app.        
        """
        self.manage_usr_apps = ManageUsrApps(self.usr_app_path)
        self.cfs_app_specs = self.manage_usr_apps.get_app_specs()
        if len(self.cfs_app_specs) > 0:
            usr_app_list = list(self.cfs_app_specs.keys())
            if action == 'Add':
                self.add_usr_app_gui(usr_app_list)
            elif action == 'Remove':
                self.select_usr_app_gui(usr_app_list, action.lower())
                if self.selected_app is not None:
                    self.usr_app_spec = self.manage_usr_apps.get_app_spec(self.selected_app)
                    self.remove_usr_app_gui()
        else:
            sg.popup('Your usr/apps directory is empty', title=f'{action} App', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)

    def get_app_table_list(self):
        app_cmake_files = self.usr_app_spec.get_targets_cmake_files()
        table_list_str = ""
        table_list = []
        for table in app_cmake_files['tables']:
            table_list_str += "%s, " % table
            table_list.append(table)
        return table_list
                
    def copy_app_tables(self, auto_copy):
        """
        An app's JSON spec table filename should not have a target prefix. The default table filename in an
        app's tables directory should have a default target name. 
        There may be extra table files in an apps table directory so only copy the tables that are defined
        in the JSON app spec
        """
        copy_passed = True
        popup_text = 'Undefined'
        table_list = self.get_app_table_list()
        if len(table_list) == 0:
            popup_text = "No tables copied since it's a library"
        else:
            app_table_path = os.path.join(self.usr_app_path, self.selected_app, 'fsw', 'tables')
            if auto_copy:
                target_equals_default = (DEFAULT_TARGET_NAME == self.cfs_target)
                try:
                    src=''   # Init for exception
                    dst=''
                    target_prefix = DEFAULT_TARGET_NAME+'_'
                    for table in os.listdir(app_table_path):
                        src_table = table.replace(target_prefix,'')
                        if src_table in table_list:
                            src = os.path.join(app_table_path, table)
                            #print('##src: ' + src)
                            if target_equals_default:
                                dst_table = table
                            else:
                                dst_table = self.cfs_target + '_' + src_table
                            dst = os.path.join(self.cfs_abs_defs_path, dst_table)
                            #print('##dst: ' + dst)
                            shutil.copyfile(src, dst)
                    popup_text = f"Copied table files '{table_list}'\n\nFROM {app_table_path}\n\nTO {self.cfs_abs_defs_path}\n"
                except IOError:
                    popup_text = f'Error copying table file\nFROM\n  {src}\nTO\n  {dst}\n'
            else:
                popup_text = f"Copy table files '{table_list}'\n\nFROM {app_table_path}\n\nTO {self.cfs_abs_defs_path}\n"
                sg.popup(popup_text, title='Copy table files', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
        return copy_passed, popup_text
        
    def remove_app_tables(self):
        table_list = self.get_app_table_list()
        for table in table_list:
            try:
                table_file = os.path.join(self.cfs_abs_defs_path, f'{self.cfs_target}_{table}')
                os.remove(table_file)
                logger.info(f'Successfully removed {table_file} from {self.cfs_abs_defs_path}')
            except Exception as e:
                logger.error(f'Attempt to remove {table_file} raised exception: {repr(e)} ')
                
    def update_targets_cmake(self, auto_update):
        """
        The following two variables list need to be updated:
           SET(cpu1_APPLIST app1 app2 app3) #!BASECAMP-INSERT!
           SET(cpu1_FILELIST file1 file2) #!BASECAMP-INSERT!
        This logic assumes there is only one uncommented APPLIST and
        FILELIST line that needs to be updated. 
        """
        update_passed = True
        popup_text = "Undefined"
        app_cmake_files = self.usr_app_spec.get_targets_cmake_files()
        table_list_str = ''
        table_list = app_cmake_files['tables']
        if len(table_list) > 0:
            table_list_str = f'and {str(table_list)}'
        if auto_update:
            file_modified = False
            instantiated_text = ''
            with open(self.targets_cmake_file) as f:
                for line in f:
                    (line_modified, newline) = self.update_targets_cmake_line(app_cmake_files, line)
                    instantiated_text += newline
                    if line_modified:
                        file_modified = True
            
            if file_modified:
                with open(self.targets_cmake_file, 'w') as f:
                    f.write(instantiated_text)
                popup_text = f"Updated targets_cmake with {app_cmake_files['obj-file']} {table_list_str}"
            else:
                popup_text = f"Preserved targets_cmake, it already contains {app_cmake_files['obj-file']} {table_list_str}"
            #todo: Remove? sg.popup(popup_text, title=f'Update {self.targets_cmake_filename}', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
        else:
            sg.clipboard_set(app_cmake_files['obj-file'] + ',' + str(app_cmake_files['tables']))
            popup_text = f"After this dialogue, {self.targets_cmake_filename} will open in an editor. Paste\n  {app_cmake_files['obj-file']}\ninto\n  {self.cmake_app_list}\n\nPaste filenames with spaces\n  {app_cmake_files['tables']}\ninto\n  {self.cmake_file_list}"
            sg.popup(popup_text, title=f'Update {self.targets_cmake_filename}', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
            self.text_editor = sg.execute_py_file('texteditor.py', parms=self.targets_cmake_file, cwd=self.basecamp_tools_path)
        return update_passed, popup_text
        
    def update_targets_cmake_line(self, app_cmake_files, line):
        line_modified = False
        if INSERT_KEYWORD in line:
            if self.cmake_app_list in line:
                # The string search logic looks for an exact match and allows
                # the name being searched to exist in an end-of-line comment
                if not line.strip().startswith('#'):  # Non-commented line
                    i = line.find(')')
                    if not app_cmake_files['obj-file'] in line[:i].split(' '):
                        line = line[:i] + ' ' + app_cmake_files['obj-file'] + line[i:]     
                        line_modified = True
                        print('app_list_new: ' + line)
            elif self.cmake_file_list in line:
                if not line.strip().startswith('#'):  # Non-commented line
                    for table in app_cmake_files['tables']:
                        i = line.find(')')
                        if not table in line[:i].split(' '):
                            line = line[:i] + ' ' + table + line[i:]     
                            line_modified = True
                            print('file_list_new: ' + line)
        return (line_modified, line)
        
    def restore_targets_cmake(self):
        app_cmake_files = self.usr_app_spec.get_targets_cmake_files()
        obj_file   = app_cmake_files['obj-file'] 
        table_list = self.get_app_table_list()
        
        file_modified = False
        instantiated_text = ''
        with open(self.targets_cmake_file) as f:
            for line in f:
                if INSERT_KEYWORD in line:
                    if self.cmake_app_list in line:
                        if obj_file in line:
                            line = line.replace(obj_file,"")
                            file_modified = True
                            logger.info(f'Removed {obj_file} from {self.targets_cmake_file}')
                    elif self.cmake_file_list in line:
                        for table in table_list:
                            if table in line:
                                line = line.replace(table,"")
                                file_modified = True
                                logger.info(f'Removed {table} from {self.targets_cmake_file}')
                instantiated_text += line        
        if file_modified:
            with open(self.targets_cmake_file, 'w') as f:
                f.write(instantiated_text)

    def update_startup_scr(self, auto_update):
        update_passed = True
        startup_script_entry = self.usr_app_spec.get_startup_scr_entry()
        if auto_update:
            original_entry = ""
            check_for_entry = True
            file_modified = False
            instantiated_text = ""
            with open(self.startup_scr_file) as f:
                for line in f:
                    if check_for_entry:
                        if self.selected_app in line.split(','):
                            check_for_entry = False
                            original_entry = line
                        if INSERT_KEYWORD in line:
                            line = startup_script_entry+'\n'+line
                            check_for_entry = False
                            file_modified = True
                    instantiated_text += line               
            if file_modified:
                with open(self.startup_scr_file, 'w') as f:
                    f.write(instantiated_text)
                popup_text = f'Added {self.selected_app} to startup script entry'
            else:
                popup_text = f'Preserved startup script, it already contains {self.selected_app}'
            #todo: Remove? sg.popup(popup_text, title=f'Update {self.startup_scr_filename}', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
        else:
            sg.clipboard_set(startup_script_entry)
            popup_text = f"After this dialogue, {self.startup_scr_filename} will open in an editor.\nPaste the following entry from the clipboard:\n\n'{startup_script_entry}'\n"
            sg.popup(popup_text, title=f'Update {self.startup_scr_filename}', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
            self.text_editor = sg.execute_py_file('texteditor.py', parms=self.startup_scr_file, cwd=self.basecamp_tools_path)
        return update_passed, popup_text
    
    def restore_startup_scr(self):
        """
        Search for the app's entry. Any field with app's name can be used as
        a keyword for the search
        """
        startup_script_entry = self.usr_app_spec.get_startup_scr_entry()
        keyword = startup_script_entry.split(',')[2]
        check_for_entry = True
        file_modified   = False
        instantiated_text = ""
        with open(self.startup_scr_file) as f:
            for line in f:
                if check_for_entry:
                    if keyword in line:
                        line = ''
                        check_for_entry = False
                        file_modified   = True
                    if INSERT_KEYWORD in line:
                        check_for_entry = False
                instantiated_text += line               
        if file_modified:
            with open(self.startup_scr_file, 'w') as f:
                f.write(instantiated_text)

    def update_topic_ids(self):
        """
        cfe-topicids.xml and kit_to_pkt_tbl.json are updated together because
        they both use the telemetry topic IDs. Also with the current 'spare'
        ID substitution method, this code makes sure they both have enough
        spares before either is updated.
        """
        update_passed = False
        popup_text = 'Undefined'
        cmd_topics = self.usr_app_spec.get_cmd_topics()
        tlm_topics = self.usr_app_spec.get_tlm_topics()
        cfe_topic_ids  = CfeTopicIds(self.cfe_topic_id_file)
        kit_to_pkt_tbl = JsonTblTopicMap(self.kit_to_tbl_file)
        
        if len(cmd_topics) > cfe_topic_ids.spare_cmd_topic_cnt():
            popup_text = f'Error acquiring command topic IDs. {len(cmd_topics)} needed, only {cfe_topic_ids.spare_cmd_topic_cnt()} available.'
            sg.popup(popup_text, title='Update Topic IDs Error', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
        elif len(tlm_topics) > cfe_topic_ids.spare_tlm_topic_cnt():
            popup_text = f'Error acquiring cFE telemetry topic IDs. {len(tlm_topics)} needed, only {cfe_topic_ids.spare_tlm_topic_cnt()} available.'
            sg.popup(popup_text, title='Update Topic IDs Error', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
        elif len(tlm_topics) > len(kit_to_pkt_tbl.spare_topics()):
            popup_text = f'Error acquiring KIT_TO telemetry topic IDs. {len(tlm_topics)} needed, only {len(kit_to_pkt_tbl.spare_topics())} available.'
            sg.popup(popup_text, title='Update Topic IDs Error', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
        else:  
            for cmd in cmd_topics:
                cfe_topic_ids.replace_spare_cmd_topic(cmd)
                print(cmd)
            for tlm in tlm_topics:
                cfe_topic_ids.replace_spare_tlm_topic(tlm)
                print(tlm)
            cfe_topic_ids.write_doc_to_file()
            kit_to_pkt_tbl.replace_spare_topics(tlm_topics)
            popup_text = f'Updated topic IDs in {self.cfe_topic_id_file} and {self.kit_to_tbl_file}'
            update_passed = True
            #todo: Remove? sg.popup(popup_text, title='Update Topic IDs', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
        return update_passed, popup_text 
        
    def restore_topic_ids(self):
        cmd_topics = self.usr_app_spec.get_cmd_topics()
        tlm_topics = self.usr_app_spec.get_tlm_topics()
        cfe_topic_ids  = CfeTopicIds(self.cfe_topic_id_file)
        kit_to_pkt_tbl = JsonTblTopicMap(self.kit_to_tbl_file)
        for cmd in cmd_topics:
            cfe_topic_ids.restore_spare_cmd_topic(cmd)
        for tlm in tlm_topics:
            cfe_topic_ids.restore_spare_tlm_topic(tlm)
        cfe_topic_ids.write_doc_to_file()
        kit_to_pkt_tbl.restore_spare_topics(tlm_topics)


    def remove_app_src_files(self):
        app_path = os.path.join(self.usr_app_path, self.selected_app)
        try:
           shutil.rmtree(app_path)
           logger.info(f'Successfully removed {app_path}')
        except Exception as e:
           logger.error(f'Attempt to remove {app_path} raised exception: {repr(e)} ')
           
 
###############################################################################
  
class CfsStdout(threading.Thread):
    """
    """
    def __init__(self, cfs_subprocess, window):
        threading.Thread.__init__(self)
        self.cfs_subprocess = cfs_subprocess
        self.window = window
        self.cfs_subprocess_log = ""
        self.daemon = True
        
    def run(self):
        """
        This function is invoked after a cFS process is started and it's design depends on how Popen is
        configured when the cFS process is started. I've tried lots of different designs to make this 
        non-blocking and easay to terminate. It assumes the the Popen parameters bufsize=1 and
        universal_newlines=True (text output). A binary stdout would need line.decode('utf-8'). Some loop
        design options:
            for line in io.TextIOWrapper(self.cfs_subprocess.stdout, encoding="utf-8"):
                self.cfs_subprocess_log += line
            while True:
                line = self.cfs_subprocess.stdout.readline()
                if not line:
                    break
                self.cfs_subprocess_log += line

            for line in iter(self.cfs_subprocess.stdout.readline, ''):
                print(">>Line: " + line)
                self.cfs_subprocess_log += line

        Reading stdout is a blocking function. The current design does not let the process get killed and I
        think it's because the read function is always active. I put the try block there becuase I'd like to
        add an exception mechanism to allow the thread to be terminated. Subprocess communiate with a timeout
        i not an option because the child process is terminated if a timeout occurs. I tried the psuedo terminal
        module as an intermediator between the cFS process and stdout thinking it may be non-blocking but
        it still blocked. 
        
        """
 
        try:
            logger.info("Starting cFS terminal window stdout display")
            for line in iter(self.cfs_subprocess.stdout.readline, ''):
                #print(">>Line: " + line)
                self.cfs_subprocess_log += line
                self.window["-CFS_PROCESS_TEXT-"].update(self.cfs_subprocess_log)
                self.window["-CFS_PROCESS_TEXT-"].set_vscroll_position(1.0)  # Scroll to bottom (most recent entry)
        except Exception as e:
            logger.error("Starting cFS terminal window stdout display exception\n" + str(e))
            
    def get_id(self):
 
        # returns id of the respective thread
        if hasattr(self, '_thread_id'):
            return self._thread_id
        for id, thread in threading._active.items():
            if thread is self:
                return id
  
    def terminate(self):
        """
        Terminate the thread by rasing an exception
        """
        logger.info("Raising CfsStdout exception to terminate thread")
        thread_id = self.get_id()
        res = ctypes.pythonapi.PyThreadState_SetAsyncExc(thread_id,
              ctypes.py_object(SystemExit))
        if res > 1:
            ctypes.pythonapi.PyThreadState_SetAsyncExc(thread_id, 0)
            print('Exception raise failure')

###############################################################################

class CfsMqttCmdClient():

    def __init__(self, broker_addr, client_base_name, cfs_cmd_topic, send_event):
        self.broker_addr  = broker_addr
        self.client_name  = f'{client_base_name}-{str(random.randint(0,10000))}'
        self.cmd_topic    = cfs_cmd_topic
        self.send_event   = send_event
        
        self.connected = False
        self.client = None
        
    def connect(self):
        self.connected = False
        try:
            self.client = mqtt.Client(self.client_name)
            self.client.on_connect = self.on_connect   # Callback function for successful connection
            self.client.connect(self.broker_addr)
            self.connected = True
        except Exception as e:
            self.send_event(f'cFS MQTT command client connection error for {self.broker_addr}:{self.broker_port}')
            self.client = None
        return self.connected
    
    def disconnect(self):
        if self.connected:
           self.client.disconnect()
           self.client = None
        self.connected = False

    def on_connect(self, client, userdata, flags, rc):
        """
        """
        print('@@TODO: GOT HERE not happening@@')
        self.send_event(f'cFS MQTT command client {self.client_name} connected with result code {rc}')
        self.client.subscribe(self.cmd_topic)
        
    def send_cmd(self, cmd_text):
        """
        """
        status, msg_id = self.client.publish(self.cmd_topic, cmd_text)
        return (status == 0)


###############################################################################

class App():

    GUI_NO_IMAGE_TXT  = '--None Selected--'
    GUI_NON_APP_STR   = '--'  # Dropdown menu substring used to determine whether selection is a valid app name
    GUI_APP_TITLE_STR = '-- App --'
    GUI_APP_SEPARATOR = '---------------'
    
    CFS_CMD_DEST = Enum('cFSCmdDest', ['UDP', 'MQTT'])
    CFS_TLM_SRC  = {'LOCAL': 'Local', 'REMOTE': 'Remote'}
    
    def __init__(self, ini_file):

        self.path = os.getcwd()
        self.cfs_interface_dir = os.path.join(self.path, "cfsinterface")
        self.config = configparser.ConfigParser()
        self.config.read(ini_file)

        self.APP_VERSION = self.config.get('APP','VERSION')

        self.EDS_MISSION_NAME    = self.config.get('CFS_TARGET','MISSION_EDS_NAME')
        self.EDS_CFS_TARGET_NAME = self.config.get('CFS_TARGET','CPU_EDS_NAME')
        self.SUDO_START_CFS      = self.config.getboolean('CFS_TARGET','SUDO_START_CFS')

        self.CFS_IP_ADDR     = self.config.get('NETWORK','CFS_IP_ADDR')
        self.CFS_CMD_PORT    = self.config.getint('NETWORK','CFS_CMD_PORT')
        self.CFS_IP_DEST_STR = f'{self.CFS_IP_ADDR}:{self.CFS_CMD_PORT}'
        
        self.CFS_MQTT_BROKER_ADDR = self.config.get('NETWORK','MQTT_BROKER_ADDR')
        self.CFS_MQTT_BROKER_PORT = self.config.get('NETWORK','MQTT_BROKER_PORT')
        self.CFS_MQTT_CMD_TOPIC   = self.config.get('NETWORK','MQTT_CFS_CMD_TOPIC')
        self.CFS_MQTT_DEST_STR    = f'{self.CFS_MQTT_BROKER_ADDR}:{self.CFS_MQTT_BROKER_PORT}/{self.CFS_MQTT_CMD_TOPIC}'
        
        self.GND_IP_ADDR      = self.config.get('NETWORK','GND_IP_ADDR')
        self.GND_TLM_PORT     = self.config.getint('NETWORK','GND_TLM_PORT')
        self.GND_TLM_TIMEOUT  = float(self.config.getint('NETWORK','GND_TLM_TIMEOUT'))/1000.0
        self.ROUTER_CTRL_PORT = self.config.getint('NETWORK','CMD_TLM_ROUTER_CTRL_PORT')
        
        self.GUI_CMD_PAYLOAD_TABLE_ROWS = self.config.getint('GUI','CMD_PAYLOAD_TABLE_ROWS')

        self.docs_path  = compress_abs_path(os.path.join(self.path, "../../docs"))
        self.tools_path = os.path.join(self.path, "tools")
        
        self.cfs_exe_rel_path   = 'build/exe/' + self.EDS_CFS_TARGET_NAME.lower()
        self.cfs_exe_file       = 'core-' + self.EDS_CFS_TARGET_NAME.lower()
        self.cfs_abs_base_path  = compress_abs_path(os.path.join(self.path, self.config.get('CFS_TARGET','BASE_PATH')))
        self.cfs_subprocess     = None
        self.cfs_subprocess_log = ""
        self.cfs_stdout         = None
        self.cfs_cmd_dest       = self.CFS_CMD_DEST.UDP
        
        self.cfe_time_event_filter = False  #todo: Retaining the state here doesn't work if user starts and stops the cFS and doesn't restart Basecamp
        self.cfs_build_subprocess  = None
        
        self.event_log   = ""        
        self.event_queue = queue.Queue()
        self.window = None
        
        self.cfe_app_list = ['CFE_ES', 'CFE_EVS', 'CFE_SB', 'CFE_TBL', 'CFE_TIME']
        self.app_cmd_list = []  # Non-cFE apps
        self.app_tlm_list = []  # Non-cFE apps
        self.usr_app_list = [self.GUI_APP_TITLE_STR]  # Non-cFE apps
        self.all_app_list = []  # Combined user and cFE app list

        self.manage_tutorials = ManageTutorials(self.config.get('PATHS', 'TUTORIALS_PATH'))
        self.create_app       = CreateApp(self.config.get('PATHS', 'APP_TEMPLATES_PATH'),
                                          self.config.get('PATHS', 'USR_APP_PATH'))
        self.manage_code_tutorials = ManageCodeTutorials(self.config.get('PATHS', 'USR_APP_PATH'))
        self.cfs_mqtt_cmd_client = CfsMqttCmdClient(self.CFS_MQTT_BROKER_ADDR, self.config.get('NETWORK','MQTT_CLIENT_NAME'),
                                                    self.CFS_MQTT_CMD_TOPIC, self.display_event)
        
        self.file_browser   = None
        self.script_runner  = None
        self.pdf_viewer     = None
        self.tutorial       = None
        self.target_control = None
        self.tlm_plot       = None
        self.tlm_screen     = None

        #todo: Add robust telmeetry screen port number management
        # tlm_screen_port is used a starting port number and each telemetry
        # screen is open with a new port number. This strategy assumes 
        # basecamp is run for short periods of time with only a few telemetry
        # screens.
        self.tlm_screen_port = self.config.getint('NETWORK', 'TLM_SCREEN_TLM_PORT')

    def update_event_history_str(self, new_event_text):
        time = datetime.now().strftime("%H:%M:%S")
        event_str = time + " - " + new_event_text + "\n"        
        self.event_log += event_str
     
    def display_event(self, new_event_text):
        self.update_event_history_str(new_event_text)
        self.window["-EVENT_TEXT-"].update(self.event_log)

    def display_tlm_monitor(self, app_name, tlm_msg, tlm_item, tlm_text):
        #TODO: print("Received [%s, %s, %s] %s" % (app_name, tlm_msg, tlm_item, tlm_text))
        self.window["-CFS_TIME-"].update(tlm_text)

    def send_cfs_mqtt_cmd(self, cmd_obj):
        """
        Assumes a valid cmd_obj
        """
        cmd_packed = self.telecommand_script.eds_mission.get_packed_obj(cmd_obj)
        cmd_text   = cmd_packed.hex()
        cmd_name   = self.telecommand_script.get_cmd_obj_name(cmd_obj)
        if self.cfs_mqtt_cmd_client.send_cmd(cmd_text):
            self.display_event(f'Sent cFS command {cmd_name} to MQTT broker')
        else:
            self.display_event(f'Error sending cFS command {cmd_name} to MQTT broker')

    def send_cfs_cmd(self, app_name, cmd_name, cmd_payload):
        if self.cfs_cmd_dest == self.CFS_CMD_DEST.UDP:
            (cmd_sent, cmd_text, cmd_status) = self.telecommand_script.send_cfs_cmd(app_name, cmd_name, cmd_payload)
            self.display_event(cmd_status)
            #TODO: Provide config switch? self.display_event(cmd_text)
        elif self.cfs_cmd_dest == self.CFS_CMD_DEST.MQTT:
            if self.cfs_mqtt_cmd_client.connected:
                (cmd_valid, cmd_status, cmd_obj) = self.telecommand_script.get_cfs_cmd_obj(app_name, cmd_name, cmd_payload)
                if cmd_valid:
                    self.send_cfs_mqtt_cmd(cmd_obj)
                else:
                    self.display_event(cmd_status)
            else:
                self.display_event(f'Failed to send {app_name}/{cmd_name}. Configured for MQTT commanding, but MQTT client is disconnected.')
            
            
    def enable_telemetry(self):
        """
        The user must enable telemetry every time the cFS is started and most if not all users want
        the time fly wheel event disabled as well so it is also done here
        """
        self.send_cfs_cmd('KIT_TO', 'EnableOutput', {'DestIp': self.GND_IP_ADDR})
        # Disable flywheel events. Assume new cFS instance running so set time_event_filter to false 
        self.cfe_time_event_filter = False 
        time.sleep(0.5)
        self.disable_flywheel_event()

    def disable_flywheel_event(self):
        # CFE_TIME does not configure an event filter so the first time through add an event filter to CFE_TIME
        # Set and add filter commands have identical parameters
        if self.cfe_time_event_filter:
            evs_cmd = 'SetFilterCmd'
        else:
            evs_cmd = 'AddEventFilterCmd'
            self.cfe_time_event_filter = True
                        
        self.send_cfs_cmd('CFE_EVS', evs_cmd,  {'AppName': 'CFE_TIME', 'EventID': Cfe.CFE_TIME_FLY_ON_EID, 'Mask': Cfe.CFE_EVS_FIRST_ONE_STOP})
        time.sleep(0.5)
        self.send_cfs_cmd('CFE_EVS', evs_cmd,  {'AppName': 'CFE_TIME', 'EventID': Cfe.CFE_TIME_FLY_OFF_EID, 'Mask': Cfe.CFE_EVS_FIRST_ONE_STOP})

    def ComingSoonPopup(self, feature_str):
        sg.popup(feature_str, title='Coming soon...', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
 
    def shutdown(self):
        """
        Close routers after close threads that depend on the routers
        """
        logger.info("Starting app shutdown sequence")
        if self.cfs_subprocess is not None:
            logger.info("Killing cFS Process")
            os.killpg(os.getpgid(self.cfs_subprocess.pid), signal.SIGTERM)  # Send the signal to all the process groups
        self.cmd_tlm_router.shutdown()
        self.tlm_server.shutdown()
        time.sleep(self.GND_TLM_TIMEOUT)
        self.window.close()
        logger.info("Completed app shutdown sequence")

    def cmd_topic_list(self):
        cmd_topics = [EdsMission.TOPIC_CMD_TITLE_KEY]
        cmd_topic_list = list(self.telecommand_gui.get_topics().keys())
        all_cmd_topics = self.config.getboolean('GUI','CMD_TOPICS_ALL')
        for topic in cmd_topic_list:
            if EdsMission.APP_CMD_TOPIC_SUFFIX in topic:
                cmd_topics.append(topic)
            else:
                if all_cmd_topics:
                    cmd_topics.append(topic)            
        logger.debug("cmd_topics = " + str(cmd_topics))
        return cmd_topics
        
    def create_app_cmd_list(self, cmd_topics):
        """
        Populate self.app_cmd_list with the app names defined in cmd_topics. Assumes the app name 
         
        """
        for topic in cmd_topics:
            app_name = topic.split('/')[0]
            if app_name not in self.cfe_app_list and app_name not in self.app_cmd_list:
                self.app_cmd_list.append(app_name)
        self.usr_app_list = self.usr_app_list + self.app_cmd_list[1:]
        self.all_app_list = self.usr_app_list + [self.GUI_APP_SEPARATOR] + self.cfe_app_list
        
    def create_app_tlm_list(self, tlm_topics):
        """
        Populate self.app_tlm_list with the app names defined in tlm_topics. Assumes the app name 
         
        """
        for topic in tlm_topics:
            app_name = topic.split('/')[0]
            if app_name not in self.cfe_app_list and app_name not in self.app_tlm_list:
                self.app_tlm_list.append(app_name)

    def view_pdf_doc(self, doc_filename):
        pdf_filename = f'{self.docs_path}{doc_filename}'
        print(f'path_filename: {pdf_filename}')
        self.pdf_viewer = sg.execute_py_file("pdfviewer.py", parms=pdf_filename, cwd=self.tools_path)
                                     
    def launch_tlmplot(self):
                
        # 1. Get user app selection & create telemetry topic dictionary 
        tlm_plot_cmd_parms = ""
        tlm_dict = {}
        app_window_layout = [[sg.Text("")],
                             [sg.Text("Select App"), sg.Combo((self.app_tlm_list), size=(20,1), key='-APP_NAME-', default_value=self.app_tlm_list[0])],
                             [sg.Text("")],
                             [sg.Button('Submit', button_color=('SpringGreen4'), enable_events=True, key='-SUBMIT-', pad=(10,1)),
                             sg.Cancel(button_color=('gray'))]]

        app_window = sg.Window('Plot Data', app_window_layout, finalize=True)           
        while True:  # Event Loop
            app_win_event, app_win_values = app_window.read(timeout=200)
            if app_win_event in (sg.WIN_CLOSED, 'Cancel'):
                break
            elif app_win_event == '-SUBMIT-':
                app_name = app_win_values['-APP_NAME-']
                if app_name != EdsMission.TOPIC_TLM_TITLE_KEY:
                    tlm_plot_cmd_parms += app_name
                    for topic in self.tlm_server.get_topics():
                        if app_name in topic:
                            tlm_dict[topic] = self.tlm_server.eds_mission.get_topic_payload(topic)
                    break
                else:
                    sg.popup("Please select a telemetry topic from the drop down menu", title='Plot Configuration', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
        app_window.close()
        if (len(tlm_dict) == 0):
            return
            
        # 2 - Create tree data
        topic_tree = sg.TreeData()
        for topic in tlm_dict:
            topic_tree.insert("", topic, topic, [])
            for tlm_element in tlm_dict[topic]:
                tlm_element = str(tlm_element[0])
                topic_tree.insert(topic, tlm_element, tlm_element, [])
        
        # 3 - Get user data selection
        #todo: Fix tree column headers. Currently first column is blank
        min_value = 0
        max_value = 100
        plot_data_layout = [[sg.Text('Select an integer telemetry point to be plotted and the data range\n')],
                           [sg.Tree(data=topic_tree, headings=['Topic'], auto_size_columns=True,
                           select_mode=sg.TABLE_SELECT_MODE_EXTENDED, num_rows=20, col0_width=40, key='-TOPIC-',
                           show_expanded=False, enable_events=True, expand_x=True, expand_y=True),],
                           [sg.Text('Plot Min Value:  '), sg.Input(str(min_value), size=(4, 1), font='Any 12', justification='r', key='-MIN-'),
                            sg.Column([[sg.Button('▲', size=(1, 1), font='Any 7', border_width=0, button_color=(sg.theme_text_color(), sg.theme_background_color()), key='-MIN_UP-')],
                            [sg.Button('▼', size=(1, 1), font='Any 7', border_width=0, button_color=(sg.theme_text_color(), sg.theme_background_color()), key='-MIN_DOWN-')]]),
                            sg.Text('Plot Max Value: '), sg.Input(str(max_value), size=(4, 1), font='Any 12', justification='r', key='-MAX-'),
                            sg.Column([[sg.Button('▲', size=(1, 1), font='Any 7', border_width=0, button_color=(sg.theme_text_color(), sg.theme_background_color()), key='-MAX_UP-')],
                            [sg.Button('▼', size=(1, 1), font='Any 7', border_width=0, button_color=(sg.theme_text_color(), sg.theme_background_color()), key='-MAX_DOWN-')]])],
                           [sg.Button('Ok'), sg.Button('Cancel')]]

        plot_data_window = sg.Window('Plot Data', plot_data_layout, resizable=True, finalize=True)
        while True:  # Event Loop
            plot_data_event, plot_data_values = plot_data_window.read(timeout=1000)
            if plot_data_event in (sg.WIN_CLOSED, 'Cancel'):
                break
            elif plot_data_event == 'Ok':
                tlm_topic = 'None'
                tlm_payload = None
                if len(plot_data_values['-TOPIC-']) > 0:
                    tlm_element = plot_data_values['-TOPIC-'][0]
                    for topic in tlm_dict:
                        if tlm_element in tlm_dict[topic].keys():
                            """
                            Find and add payload name to topic string because the payload string is needed by tlmplot
                            EDS naming assumptions:
                             - topic name begins with app name and components separated by '/'
                             - topic type contains the topic's payload variable name and single quotes
                            """
                            app_name   = topic.split('/')[0]
                            topic_type = str(type(tlm_dict[topic]))
                            type_list  = topic_type.split("'")
                            index = [i for i, s in enumerate(type_list) if app_name in s]
                            if len(index) > 0:
                                tlm_payload = type_list[index[0]].split('/')[-1]
                            else:
                                sg.popup("Error retrieving payload name from topic %s type %s" % (topic, topic_type), title='Plot Configuration', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
                                tlm_plot_cmd_parms = ""
                            tlm_topic = topic
                    if tlm_payload is None:
                        tlm_plot_cmd_parms = ""
                    else:
                        tlm_plot_cmd_parms += ' ' + tlm_topic + ' ' + tlm_payload + ' ' + tlm_element + ' ' + plot_data_values['-MIN-'] + ' ' + plot_data_values['-MAX-'] 
                    break
                else:
                    sg.popup("You must select a telemetry element from a telemetry topic's payload", title='Plot Configuration', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
            elif plot_data_event == '-MIN_UP-':
                min_value += 1
                plot_data_window["-MIN-"].update(str(min_value))
            elif plot_data_event == '-MIN_DOWN-':
                min_value -= 1
                plot_data_window["-MIN-"].update(str(min_value))
            elif plot_data_event == '-MAX_UP-':
                max_value += 1
                plot_data_window["-MAX-"].update(str(max_value))
            elif plot_data_event == '-MAX_DOWN-':
                max_value -= 1
                plot_data_window["-MAX-"].update(str(max_value))
            #todo print(plot_data_event, plot_data_values)
        plot_data_window.close()
        print('tlm_plot_cmd_parms = ' + tlm_plot_cmd_parms)

        #todo: Add check if tlm_plots been run before or is there logic in router?
        if (len(tlm_plot_cmd_parms)>0):
            self.cmd_tlm_router.add_gnd_tlm_dest(self.config.getint('NETWORK','TLM_PLOT_TLM_PORT'))                
            self.tlm_plot = sg.execute_py_file("tlmplot.py", parms=tlm_plot_cmd_parms, cwd=self.cfs_interface_dir)

    def create_window(self, sys_target_str, sys_comm_str):
        """
        Create the main window. Non-class variables are used so it can be refreshed, PySimpleGui
        layouts can't be shared.
        """
        sg.theme('LightGreen')
        sg.set_options(element_padding=(0, 0))
    
        tutorial_titles = []
        if len(self.manage_code_tutorials.tutorial_titles) > 0:
            tutorial_titles += self.manage_tutorials.tutorial_titles + ['---'] + self.manage_code_tutorials.tutorial_titles
        else:
            tutorial_titles += self.manage_tutorials.tutorial_titles
        
        menu_def = [
                       ['System', ['Options', 'About', 'Exit']],
                       ['Developer', ['Create App', 'Download App', 'Add App', 'Remove App', '---', 'Run Perf Monitor']], #todo: 'Certify App' 
                       ['Operator', ['Browse Files', 'Run Script', 'Plot Data', '---', 'Control Remote Target', 'Configure Command Destination', 'Configure Telemetry Source']],
                       ['Documents', ['cFS Overview', 'cFE Overview', 'App Dev Guide', 'Remote Ops Guide']],
                       ['Tutorials', tutorial_titles]
                   ]

        self.common_cmds = ['-- Common Commands--', 'Enable Telemetry', 'Reset Time', 'Noop/Reset App', 'Restart App', 'Configure Event Types', 'Reset Event Filter', 'Ena/Dis Flywheel', 'Set Tlm Source', 'cFE Version']


        # Events can't be posted until after first window.read() so initialization string is format here and used as the default string
        
        self.update_event_history_str(sys_target_str)
        self.update_event_history_str(sys_comm_str)
            
        cmd_topics = self.cmd_topic_list()
        tlm_topics = list(self.tlm_server.get_topics().keys())
        logger.debug("tlm_topics = " + str(tlm_topics))
        self.create_app_cmd_list(cmd_topics)
        self.create_app_tlm_list(tlm_topics)
        
        pri_hdr_font = ('Arial bold',14)
        sec_hdr_font = ('Arial',12)
        log_font = ('Courier',12)
        layout = [
                     [sg.Menu(menu_def, tearoff=False, pad=(50, 50))],
                     [sg.Frame('cFS',
                     [[
                      sg.Button('Build New', enable_events=True, key='-CREATE_CFS-', border_width=0, pad=((5,5),(5,5)), tooltip='Issue make distclean before make topicids'),
                      sg.Button('Build',     enable_events=True, key='-BUILD_CFS-',  border_width=0, pad=((5,5),(5,5)), tooltip='Only issue make install'),
                      sg.Button('Start',     enable_events=True, key='-START_CFS-',  button_color=('SpringGreen4'), border_width=0, pad=((5,5),(5,5))),
                      sg.Button('Stop',      enable_events=True, key='-STOP_CFS-',   button_color=('IndianRed3'),   border_width=0, pad=((5,5),(5,5))),
                      ]]),
                      sg.Text('Mission:', font=pri_hdr_font),
                      sg.Text(self.EDS_MISSION_NAME, font=sec_hdr_font, text_color='blue'),
                      sg.Text('Target:', font=pri_hdr_font, pad=(10,1)),
                      sg.Text(self.telecommand_gui.target_name, font=sec_hdr_font, text_color='blue'),
                      sg.Text('Image', font=pri_hdr_font, pad=(10,1)),
                      sg.Text(self.GUI_NO_IMAGE_TXT, key='-CFS_IMAGE-', font=sec_hdr_font, text_color='blue')],
                     [sg.Frame('', [[sg.Button('Ena Tlm', enable_events=True, key='-ENA_TLM-', pad=((10,5),(12,12))),
                      sg.Button('Files...', enable_events=True, key='-FILE_BROWSER-', pad=((5,5),(12,12))),
                      sg.Text('Quick Cmd:', font=sec_hdr_font, pad=((0,0),(12,12))),
                      sg.Combo(self.common_cmds, enable_events=True, key="-COMMON_CMD-", default_value=self.common_cmds[0], pad=((0,5),(12,12))),
                      sg.Text('Send Cmd:', font=sec_hdr_font, pad=((5,0),(12,12))),
                      sg.Combo(cmd_topics, enable_events=True, key="-CMD_TOPICS-", default_value=cmd_topics[0], pad=((0,5),(12,12))),
                      sg.Text('View Tlm:', font=sec_hdr_font, pad=((5,0),(12,12))),
                      sg.Combo(tlm_topics, enable_events=True, key="-TLM_TOPICS-", default_value=tlm_topics[0], pad=((0,5),(12,12))),]], pad=((0,0),(15,15)))],
                     [sg.Text('cFS Target Process Window', font=pri_hdr_font, pad=(4,1)), 
                      sg.Text('  ', font=sec_hdr_font, pad=(4,1)),
                      sg.Text('Telecommand:', font=sec_hdr_font, pad=(4,1)), sg.Text(self.CFS_IP_DEST_STR, key='-CFS_CMD_DEST-', font=sec_hdr_font, text_color='blue'), 
                      sg.Text('  ', font=sec_hdr_font, pad=(4,1)),
                      sg.Text('Telemetry:', font=sec_hdr_font, pad=(4,1)), sg.Text(self.CFS_TLM_SRC['LOCAL'], key='-CFS_TLM_SRC-', font=sec_hdr_font, text_color='blue'), 
                      sg.Text('  ', font=sec_hdr_font, pad=(4,1)),
                      sg.Text('Time:', font=sec_hdr_font, pad=(4,1)), sg.Text(EdsMission.NULL_TLM_STR, key='-CFS_TIME-', font=sec_hdr_font, text_color='blue'), 
                      sg.Button('Restart', enable_events=True, key='-RESTART-', visible=False)],
                     #[sg.Output(font=log_font, size=(125, 10))],
                     [sg.MLine(default_text=self.cfs_subprocess_log, font=log_font, enable_events=True, size=(135, 15), key='-CFS_PROCESS_TEXT-')],
                     [sg.Text('Ground Events', font=pri_hdr_font), sg.Button('Clear', enable_events=True, key='-CLEAR_EVENTS-', pad=(5,1))],
                     [sg.MLine(default_text=self.event_log, font=log_font, enable_events=True, size=(135, 15), key='-EVENT_TEXT-')]
                 ]

        #sg.Button('Send Cmd', enable_events=True, key='-SEND_CMD-', pad=(10,1)),
        #sg.Button('View Tlm', enable_events=True, key='-VIEW_TLM-', pad=(10,1)),
        window = sg.Window(f'cFS Basecamp - v{self.APP_VERSION}', layout, auto_size_text=True, finalize=True)
        return window
  
    def reload_eds_libs(self):
        #importlib.invalidate_caches()
        self.telecommand_gui.eds_mission.reload_libs()
        self.telecommand_script.eds_mission.reload_libs()
        self.tlm_server.eds_mission.reload_libs()
       
       
    def execute(self):
    
        sys_target_str = f"Basecamp version {self.APP_VERSION} initialized with mission '{self.EDS_MISSION_NAME}', target '{self.EDS_CFS_TARGET_NAME}' on {datetime.now().strftime('%m/%d/%Y')} at {datetime.now().strftime('%H:%M:%S')}"
        sys_comm_str = f'Basecamp target host {self.CFS_IP_ADDR}, command port {self.CFS_CMD_PORT}, telemetry port {self.GND_TLM_PORT}'
    
        logger.info(sys_target_str)
        logger.info(sys_comm_str)
        
        self.tlm_monitors = {'CFE_ES': {'HK_TLM': ['Seconds']}, 'FILE_MGR': {'DIR_LIST_TLM': ['Seconds']}}
        
        try:
            # Command & Telemetry Router
                             
            self.cmd_tlm_router = CmdTlmRouter(self.CFS_IP_ADDR, self.CFS_CMD_PORT, 
                                  self.GND_IP_ADDR, self.ROUTER_CTRL_PORT, self.GND_TLM_PORT, self.GND_TLM_TIMEOUT)
            self.cfs_cmd_output_queue = self.cmd_tlm_router.get_cfs_cmd_queue()
            self.cfs_cmd_input_queue  = self.cmd_tlm_router.get_cfs_cmd_source_queue()

        except Exception as e:
            logger.error(f'Error creating command-telemetry router\n{str(e)}')
            sys.exit(2)
            
        try:
            # Command Objects    
             
            self.telecommand_gui    = TelecommandGui(self.EDS_MISSION_NAME, self.EDS_CFS_TARGET_NAME, self.cfs_cmd_output_queue)
            self.telecommand_script = TelecommandScript(self.EDS_MISSION_NAME, self.EDS_CFS_TARGET_NAME, self.cfs_cmd_output_queue)
             
            # Telemetry Objects
             
            self.tlm_server  = TelemetryQueueServer(self.EDS_MISSION_NAME, self.EDS_CFS_TARGET_NAME, self.cmd_tlm_router.get_gnd_tlm_queue())
            self.tlm_monitor = BasecampTelemetryMonitor(self.tlm_server, self.tlm_monitors, self.display_tlm_monitor, self.event_queue)
            self.tlm_server.execute()      
            self.cmd_tlm_router.start()
             
            logger.info("Successfully created application objects")
        
        except Exception as e:
            logger.error(f'Error creating command-telemetry objects/server\n{str(e)}')
            sys.exit(2)

        self.window = self.create_window(sys_target_str, sys_comm_str)
        # --- Loop taking in user input --- #
        restart = False
        while True:
    
            self.event, self.values = self.window.read(timeout=50)
            logger.debug("App Window Read()\nEvent: %s\nValues: %s" % (self.event, self.values))

            if self.event in (sg.WIN_CLOSED, 'Exit', '-RESTART-') or self.event is None:
                restart = (self.event == '-RESTART-')
                break
            
            ######################################
            ##### Autonomous System Behavior #####
            ######################################
            
            if not self.event_queue.empty():
                new_event_text = self.event_queue.get_nowait()
                self.display_event(new_event_text)
            
            # Route commands from remote processes. for now, always accept commands
            if not self.cfs_cmd_input_queue.empty():
                datagram = self.cfs_cmd_input_queue.get()[0]
                self.cfs_cmd_output_queue.put(datagram)
                self.display_event("Sent remote process command: " + datagram_to_str(datagram))
                logger.debug("Sent remote process command: " + datagram_to_str(datagram))

            #######################
            ##### MENU EVENTS #####
            #######################

            ### SYSTEM ###

            if self.event == 'Options':
                self.ComingSoonPopup("Configure Basecamp system options")
            
            elif self.event == 'About':
                about_msg = ('Basecamp provides a cFS application framework,\n'
                             'build/runtime tools, and a lightweight GUI that\n'
                             'simplify creating, integrating, testing, and\n'
                             'deploying cFS applications.\n\n'
                             'Version.......{}'.format(self.APP_VERSION))
                sg.popup(about_msg,
                         title='About Basecamp', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
       
            ### CFS DEVELOPER ###

            elif self.event == 'Create App':
                self.create_app.execute()

            elif self.event == 'Download App':
                repo_exclusions = self.config.get('CFS_TARGET','CFS_APPS').split(',')
                print(repo_exclusions)
                print(self.config.get('PATHS','USR_APP_PATH'))
                app_store = AppStore(self.config.get('APP','APP_STORE_URL'), self.config.get('PATHS','USR_APP_PATH'),repo_exclusions)
                app_store.execute()
 
            elif self.event in ('Add App','Remove App'):
                manage_cfs = ManageCfs(self.path, self.cfs_abs_base_path, self.config.get('PATHS', 'USR_APP_PATH'), self.window, self.EDS_CFS_TARGET_NAME)
                manage_cfs.execute(self.event.split(' ')[0])

            if self.event == 'Certify App':
                self.ComingSoonPopup("Certify your app to an OpenSatKit app repo")

            elif self.event == 'Run Perf Monitor':
                subprocess.Popen("java -jar ../perf-monitor/CPM.jar",shell=True)  #TODO - Use ini file path definition

                                                  
            ### OPERATOR ###

            elif self.event == '-ENA_TLM-':
                self.enable_telemetry()

            elif self.event == 'Run Script':
                self.cmd_tlm_router.add_cfs_cmd_source(self.config.getint('NETWORK','SCRIPT_RUNNER_CMD_PORT'))
                self.cmd_tlm_router.add_gnd_tlm_dest(self.config.getint('NETWORK','SCRIPT_RUNNER_TLM_PORT'))
                self.script_runner = sg.execute_py_file("scriptrunner.py", cwd=self.cfs_interface_dir)

            elif self.event == 'Browse Files' or self.event == '-FILE_BROWSER-':
                self.cmd_tlm_router.add_cfs_cmd_source(self.config.getint('NETWORK','FILE_BROWSER_CMD_PORT'))
                self.cmd_tlm_router.add_gnd_tlm_dest(self.config.getint('NETWORK','FILE_BROWSER_TLM_PORT'))
                self.file_browser = sg.execute_py_file("filebrowser.py", cwd=self.cfs_interface_dir)

            elif self.event == 'Plot Data':
                self.launch_tlmplot()
                
            elif self.event == 'Control Remote Target':
                self.cmd_tlm_router.add_cfs_cmd_source(self.config.getint('NETWORK','TARGET_CONTROL_CMD_PORT'))
                tools_dir = os.path.join(self.path, "cfsinterface")
                self.target_control = sg.execute_py_file("targetcontrol.py", cwd=tools_dir)

            elif self.event == 'Configure Command Destination':
                pop_win = sg.Window('Configure Command Destination',
                                    [[sg.Text("")],
                                     [sg.Text("UDP:",  size=(6,1)), sg.Text("Send commands to cFS IP address/port defined in basecamp.ini")],
                                     [sg.Text("MQTT:", size=(6,1)), sg.Text("Send commands to MQTT broker/topic defined in basecamp.ini")],
                                     [sg.Text("")],
                                     [sg.Button('UDP',  button_color=('SpringGreen4'), enable_events=True, key='-UDP-',  pad=(10,1)),
                                      sg.Button('MQTT', button_color=('SpringGreen4'), enable_events=True, key='-MQTT-', pad=(10,1)), 
                                      sg.Cancel(button_color=('gray'), pad=(10,1))]])
            
                while True:  # Event Loop
                    pop_event, pop_values = pop_win.read(timeout=200)
                    if pop_event in (sg.WIN_CLOSED, 'Cancel'):
                        break
                    if pop_event == '-UDP-':
                        self.cfs_cmd_dest = self.CFS_CMD_DEST.UDP
                        self.display_event('cFS command destination set to UDP')
                        self.window["-CFS_CMD_DEST-"].update(self.CFS_IP_DEST_STR)
                        break
                    elif pop_event == '-MQTT-':
                        if self.cfs_mqtt_cmd_client.connect():
                            self.cfs_cmd_dest = self.CFS_CMD_DEST.MQTT
                            self.window["-CFS_CMD_DEST-"].update(f'{self.CFS_MQTT_DEST_STR}')
                            self.display_event(f'cFS command destination set to MQTT: {self.CFS_MQTT_DEST_STR}')
                        break
                        
                pop_win.close()
                
            elif self.event == 'Configure Telemetry Source':
                pop_win = sg.Window('Configure Telemetry Source',
                                    [[sg.Text("")],
                                     [sg.Text("LOCAL:",  size=(6,1)), sg.Text("KIT_TO configured to send telemetry from the local target")],
                                     [sg.Text("REMOTE:", size=(6,1)), sg.Text("KIT_TO configured to send telemetry from a remote target. Requires MQTT_GW")],
                                     [sg.Text("")],
                                     [sg.Button('Local',  button_color=('SpringGreen4'), enable_events=True, key='-LOCAL-',  pad=(10,1)),
                                      sg.Button('Remote', button_color=('SpringGreen4'), enable_events=True, key='-REMOTE-', pad=(10,1)), 
                                      sg.Cancel(button_color=('gray'), pad=(10,1))]])
            
                while True:  # Event Loop
                    pop_event, pop_values = pop_win.read(timeout=200)
                    if pop_event in (sg.WIN_CLOSED, 'Cancel'):
                        break
                    if pop_event in ('-LOCAL-','-REMOTE-'):
                        tlm_src = pop_event.strip('-')
                        self.send_cfs_cmd('KIT_TO', 'SetTlmSource',  {'Source': tlm_src})
                        self.display_event(f'cFS telemetry source set to {tlm_src}')
                        self.window["-CFS_TLM_SRC-"].update(tlm_src.title())
                        break
                        
                pop_win.close()

            ### DOCUMENTS ###
                """ 
                # Permision Denied, not portable
                subprocess.Popen([path_filename],shell=True) 
                subprocess.call(["xdg-open", path_filename]) 
                # Browser launch issues & wonky for enbvironments like WSL
                prefix = 'file://wsl.localhost/Ubuntu'
                prefix = 'file:/' #TODO - Put in ini file?
                try:
                    displayed = webbrowser.open(path_filename, new=1, autoraise=True)
                    print(f'After webbrowser open {displayed}')
                except Exception as e:
                    displayed = False;
                    logger.error(f'Exception opening {path_filename} in browser\n{str(e)}')
                    print(f'Exception opening {path_filename} in browser\n{str(e)}')
                if not displayed:
                    text = sg.popup_get_text(f'Failed to display PDF document. Paste the following path/file (already added to clipboard) into your browser.',
                                             title='Document', default_text= path_filename, size=(90, 5), keep_on_top=True, grab_anywhere=True, modal=False)
                    sg.clipboard_set(text)
                """
            
            elif self.event == 'cFS Overview':
                self.view_pdf_doc('basecamp-cfs-overview.pdf')
            elif self.event == 'cFE Overview':
                self.view_pdf_doc('basecamp-cfs-framework.pdf')
            elif self.event == 'App Dev Guide':
                self.view_pdf_doc('basecamp-app-dev.pdf')
            elif self.event == 'Remote Ops Guide':
                self.view_pdf_doc('basecamp-remote-ops.pdf')

                
            ### TUTORIALS ###
                   
            elif self.event in self.manage_tutorials.tutorial_titles:
                tutorial_dir = self.manage_tutorials.tutorial_lookup[self.event].path
                self.tutorial = sg.execute_py_file("tutorial.py", parms=tutorial_dir, cwd=self.tools_path)
                
            elif self.event in self.manage_code_tutorials.tutorial_titles:
                tutorial_dir = self.manage_code_tutorials.tutorial_lookup[self.event].path
                print (f'tutorial_dir = {tutorial_dir}')
                self.tutorial = sg.execute_py_file("appcodetutorial.py", parms=tutorial_dir, cwd=self.tools_path)

            #################################
            ##### TOP ROW BUTTON EVENTS #####
            #################################
 
            elif self.event == '-CREATE_CFS-':
            
                if self.cfs_subprocess is None:
                    build_cfs_sh = os.path.join(self.path, SH_BUILD_CFS_TOPICIDS)
                    self.cfs_build_subprocess = subprocess.Popen('%s %s' % (build_cfs_sh, self.cfs_abs_base_path),
                                                       stdout=subprocess.PIPE, shell=True, bufsize=1, universal_newlines=True)
                    if self.cfs_build_subprocess is not None:
                        self.cfs_stdout = CfsStdout(self.cfs_build_subprocess, self.window)
                        self.cfs_stdout.start()
                else:
                    sg.popup("A cFS image is currently running. You must stop the current image prior to building a new image.", title='Build cFS', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
                
            elif self.event == '-BUILD_CFS-':
            
                if self.cfs_subprocess is None:
                    build_cfs_sh = os.path.join(self.path, SH_MAKE_INSTALL_CFS)
                    self.cfs_build_subprocess = subprocess.Popen('%s %s' % (build_cfs_sh, self.cfs_abs_base_path),
                                                       stdout=subprocess.PIPE, shell=True, bufsize=1, universal_newlines=True)
                    if self.cfs_build_subprocess is not None:
                        self.cfs_stdout = CfsStdout(self.cfs_build_subprocess, self.window)
                        self.cfs_stdout.start()
                else:
                    sg.popup("A cFS image is currently running. You must stop the current image prior to building a new image.", title='Build cFS', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)

            elif self.event == '-START_CFS-':
                """
                
                """
                cfs_abs_exe_path = os.path.join(self.cfs_abs_base_path, self.cfs_exe_rel_path) 
                if self.SUDO_START_CFS:
                   start_sh  = os.path.join(self.path, SH_SUDO_START_CFS)
                   password  = self.config.get('APP','PASSWORD')
                   popen_str = f'{start_sh} {cfs_abs_exe_path} {self.cfs_exe_file} {password}'
                else:
                   start_sh  = os.path.join(self.path, SH_START_CFS)
                   popen_str = f'{start_sh} {cfs_abs_exe_path} {self.cfs_exe_file}'
                #self.cfs_subprocess = subprocess.Popen('%s %s %s' % (start_cfs_sh, cfs_abs_exe_path, self.cfs_exe_file), shell=True)
                #self.cfs_subprocess = subprocess.Popen('%s %s %s' % (start_cfs_sh, cfs_abs_exe_path, self.cfs_exe_file),
                #                                       stdout=self.cfs_pty_slave, stderr=self.cfs_pty_slave, close_fds=True,
                #                                       shell=True) #, bufsize=1, universal_newlines=True)                
                print(f'popen_str: {popen_str}')
                self.cfs_subprocess = subprocess.Popen(popen_str, stdout=subprocess.PIPE, shell=True, 
                                                       bufsize=1, universal_newlines=True,
                                                       preexec_fn = lambda : (os.setsid(), os.nice(10)))
                time.sleep(2.0)
                if self.cfs_subprocess is not None:
                    self.window["-CFS_IMAGE-"].update(os.path.join(cfs_abs_exe_path, self.cfs_exe_file))
                    time.sleep(1.0)
                    self.enable_telemetry()
                    self.cfs_stdout = CfsStdout(self.cfs_subprocess, self.window)
                    self.cfs_stdout.start()
                    
                """ 
                #todo: Kill current thread if running
                #todo: history_setting_filename doesn't seem to do anything. My goal is to save cFS image locations across app invocations 
                self.cfs_exe_str = sg.popup_get_file('Please select your cFS executable image', title='cFS Executable Dialog',
                                       default_path = self.cfs_exe_path, history = True, history_setting_filename = 'cfs_exe.log')
                
                if self.cfs_exe_str != None:
                    print("self.cfs_exe_str = " + self.cfs_exe_str)
                    cfs_dir = self.cfs_exe_str[0:self.cfs_exe_str.rfind("/")]
                    print("cfs_dir = " + cfs_dir)
                    self.sg_window_cfs_image.update(self.cfs_exe_str)
                    self.cfs_popen = sg.execute_command_subprocess(self.cfs_exe_str, cwd=cfs_dir)
                """
            elif self.event == '-STOP_CFS-':
                if self.cfs_subprocess is not None:
                    logger.info("Killing cFS Process")
                    os.killpg(os.getpgid(self.cfs_subprocess.pid), signal.SIGTERM)  # Send the signal to all the process groups
                    self.cfs_subprocess = None
                    self.window["-CFS_IMAGE-"].update(self.GUI_NO_IMAGE_TXT)
                    self.window["-CFS_TIME-"].update(EdsMission.NULL_TLM_STR)
                else:
                    self.window["-CFS_IMAGE-"].update(self.GUI_NO_IMAGE_TXT)
                    self.window["-CFS_TIME-"].update(EdsMission.NULL_TLM_STR)
                    """
                    History of 
                    #1
                    if self.cfs_stdout is not None:
                        self.cfs_stdout.terminate()  # I tried to join() afterwards and it hangs
                    subprocess.Popen(SH_STOP_CFS, shell=True)
                    #2                  
                    if hasattr(signal, 'CTRL_C_EVENT'):
                        self.cfs_subprocess.send_signal(signal.CTRL_C_EVENT)
                        #os.kill(self.cfs_subprocess.pid, signal.CTRL_C_EVENT)
                    else:
                        self.cfs_subprocess.send_signal(signal.SIGINT)
                        #pgid = os.getpgid(self.cfs_popen.pid)
                        #if pgid == 1:
                        #    os.kill(self.cfs_popen.pid, signal.SIGINT)
                        #else:
                        #    os.killpg(os.getpgid(self.cfs_popen.pid), signal.SIGINT) 
                        #os.kill(self.cfs_popen.pid(), signal.SIGINT)
                    #3
                    self.cfs_subprocess.kill()
                    #4
                    if self.cfs_subprocess.poll() is not None:
                        logger.info("Killing cFS after subprocess poll")
                        if self.cfs_stdout is not None:
                            self.cfs_stdout.terminate()  # I tried to join() afterwards and it hangs
                        subprocess.Popen(SH_STOP_CFS, shell=True)
                        sg.popup("cFS failed to terminate.\nUse another terminal to kill the process.", title='Warning', grab_anywhere=True, modal=False)
                    else:
                        self.cfs_subprocess = None
                    #5 - Trying to confirm process was actually stopped. The exception wasn't raised. I tried a delay but that prevented some events from being displayed
                    try:
                         print("Trying PID")
                         os.getpgid(self.cfs_subprocess.pid)
                     except ProcessLookupError:
                         print("PID exception")
                         self.cfs_subprocess = None
                         self.window["-CFS_IMAGE-"].update(self.GUI_NO_IMAGE_TXT)
                         self.window["-CFS_TIME-"].update(EdsMission.NULL_TLM_STR)
                    """
        
            elif self.event == '-COMMON_CMD-':
                cfs_config_cmd = self.values['-COMMON_CMD-']
                
                if cfs_config_cmd == self.common_cmds[1]: #### Enable Telemetry ####
                    self.enable_telemetry()

                elif cfs_config_cmd == self.common_cmds[2]: #### Reset Time ####
                    self.send_cfs_cmd('CFE_TIME', 'SetMETCmd', {'Seconds': 0,'MicroSeconds': 0 })
                    time.sleep(0.5)
                    self.send_cfs_cmd('CFE_TIME', 'SetTimeCmd', {'Seconds': 0,'MicroSeconds': 0 })
            
                elif cfs_config_cmd == self.common_cmds[3]: #### Noop/Reset App ####
                    pop_win = sg.Window('Noop-Reset Application',
                                        [[sg.Text("")],
                                         [sg.Text("Select App"), sg.Combo((self.all_app_list), size=(20,1), key='-APP_NAME-', default_value=self.all_app_list[0])],
                                         [sg.Text("")],
                                         [sg.Button('Noop',  button_color=('SpringGreen4'), enable_events=True, key='-NOOP-',  pad=(5,1), tooltip="Send a 'No Operation' command"),
                                          sg.Button('Reset', button_color=('SpringGreen4'), enable_events=True, key='-RESET-', pad=(5,1), tooltip="Send a 'Reset App' command"),
                                          sg.Cancel(button_color=('gray'), pad=(5,1))]])

                    while True:  # Event Loop
                        pop_event, pop_values = pop_win.read(timeout=200)
                        if pop_event in (sg.WIN_CLOSED, 'Cancel'):
                            break
                        if pop_event in ('-NOOP-', '-RESET-'):
                            app_name = pop_values['-APP_NAME-']
                            if self.GUI_NON_APP_STR in app_name:
                                sg.popup(f'{app_name} is not a valid app name. Please select an app from the dropdown list', title='Noop-Reset Application', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
                            else:
                                if app_name in self.cfe_app_list + ['CI_LAB']:  #todo: Remove CI_LAB or update to use KIT_CI that follow app_c_fw standards
                                    if pop_event == '-NOOP-':
                                        self.send_cfs_cmd(app_name, 'NoopCmd', {})
                                    elif pop_event == '-RESET-':
                                        self.send_cfs_cmd(app_name, 'ResetCountersCmd', {})
                                else:
                                    if pop_event == '-NOOP-':
                                        self.send_cfs_cmd(app_name, 'Noop', {})
                                    elif pop_event == '-RESET-':
                                        self.send_cfs_cmd(app_name, 'Reset', {})
                                break        
                                        
                    pop_win.close()
                    
                elif cfs_config_cmd == self.common_cmds[4]: #### Restart App  ####
                    pop_win = sg.Window('Restart Application',
                                        [[sg.Text("")],
                                         [sg.Text("Select App"), sg.Combo((self.usr_app_list), size=(20,1), key='-APP_NAME-', default_value=self.usr_app_list[0])],
                                         [sg.Text("")],
                                         [sg.Button('Restart', button_color=('SpringGreen4'), enable_events=True, key='-RESTART-', pad=(10,1)),
                                          sg.Cancel(button_color=('gray'))]])
                
                    while True:  # Event Loop
                        pop_event, pop_values = pop_win.read(timeout=200)
                        if pop_event in (sg.WIN_CLOSED, 'Cancel'):
                            break
                        if pop_event == '-RESTART-':
                            app_name = pop_values['-APP_NAME-']
                            if self.GUI_NON_APP_STR in app_name:
                                sg.popup(f'{app_name} is not a valid app name. Please select an app from the dropdown list', title='Restart Application', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
                            else:
                                self.send_cfs_cmd('CFE_ES', 'RestartAppCmd',  {'Application': app_name})
                                break        
                    pop_win.close()

                elif cfs_config_cmd == self.common_cmds[5]: #### Configure App Event Types ####
                    pop_win = sg.Window('Configure App Event Types',
                                        [[sg.Text("")],
                                         [sg.Text("Select App"), sg.Combo((self.all_app_list), size=(20,1), key='-APP_NAME-', default_value=self.all_app_list[0])],
                                         [sg.Text("")],
                                         [sg.Checkbox('Debug', key='-DEBUG-', default=False), sg.Checkbox('Information', key='-INFO-', default=True),
                                          sg.Checkbox('Error', key='-ERROR-', default=True),  sg.Checkbox('Critical', key='-CRITICAL-', default=True)], 
                                         [sg.Text("")],
                                         [sg.Button('Enable', button_color=('SpringGreen4'), enable_events=True, key='-ENABLE-', pad=(10,1)),
                                          sg.Button('Disable', button_color=('red4'), enable_events=True, key='-DISABLE-', pad=(10,1)), 
                                          sg.Cancel(button_color=('gray'), pad=(10,1))]])
                
                    while True:  # Event Loop
                        pop_event, pop_values = pop_win.read(timeout=200)
                        if pop_event in (sg.WIN_CLOSED, 'Cancel'):
                            break
                        if pop_event in ('-ENABLE-', '-DISABLE-'):
                            app_name = pop_values['-APP_NAME-'] 
                            if self.GUI_NON_APP_STR in app_name:
                                sg.popup(f'{app_name} is not a valid app name. Please select an app from the dropdown list', title='Configure App Event Types', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
                            else:
                                bit_mask = 0
                                bit_mask = (bit_mask | (Cfe.EVS_DEBUG_MASK    if pop_values['-DEBUG-']    else 0))
                                bit_mask = (bit_mask | (Cfe.EVS_INFO_MASK     if pop_values['-INFO-']     else 0))
                                bit_mask = (bit_mask | (Cfe.EVS_ERROR_MASK    if pop_values['-ERROR-']    else 0))
                                bit_mask = (bit_mask | (Cfe.EVS_CRITICAL_MASK if pop_values['-CRITICAL-'] else 0))

                                if pop_event == '-ENABLE-':
                                    self.send_cfs_cmd('CFE_EVS', 'EnableAppEventTypeCmd',  {'AppName': app_name, 'BitMask': bit_mask})
                                if pop_event == '-DISABLE-':
                                    self.send_cfs_cmd('CFE_EVS', 'DisableAppEventTypeCmd',  {'AppName': app_name, 'BitMask': bit_mask})
                                break        

                    pop_win.close()
                
                elif cfs_config_cmd == self.common_cmds[6]: #### Reset Event Filter ####
                    pop_win = sg.Window('Reset App Event Filter',
                                        [[sg.Text("")],
                                         [sg.Text("Select App"), sg.Combo((self.all_app_list), size=(20,1), key='-APP_NAME-', default_value=self.all_app_list[0], pad=(5,1))],
                                         [sg.Text("")],
                                         [sg.Text("Event ID"), sg.Input('0', size=(8, 1), font='Any 12', key='-EID-', pad=(5,1))],
                                         [sg.Text("")],
                                         [sg.Button('Reset All', button_color=('SpringGreen4'), enable_events=True, key='-RESET_ALL-', pad=(10,1)),
                                          sg.Button('Reset EID', button_color=('SpringGreen4'), enable_events=True, key='-RESET_EID-', pad=(10,1)), 
                                          sg.Cancel(button_color=('gray'), pad=(10,1))]])
                
                    while True:  # Event Loop
                        pop_event, pop_values = pop_win.read(timeout=200)
                        if pop_event in (sg.WIN_CLOSED, 'Cancel'):
                            break
                        if pop_event in ('-RESET_ALL-', '-RESET_EID-'):
                            app_name = pop_values['-APP_NAME-'] 
                            if self.GUI_NON_APP_STR in app_name:
                                sg.popup(f'{app_name} is not a valid app name. Please select an app from the dropdown list', title='Reset App Event Filter', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
                            else:
                                if pop_event == '-RESET_ALL-':
                                    self.send_cfs_cmd('CFE_EVS', 'ResetAllFiltersCmd',  {'AppName': app_name})
                                    break
                                if pop_event == '-RESET_EID-':
                                    event_id = pop_values['-EID-']
                                    if event_id.isdigit():
                                        self.send_cfs_cmd('CFE_EVS', 'ResetFilterCmd',  {'AppName': app_name, 'EventID': event_id})
                                        break
                                    else:
                                        sg.popup(f'{event_id} is not an integer. Please enter a valid event message ID', title='Reset App Event Filter', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)                                    
                                        

                    pop_win.close()

                elif cfs_config_cmd == self.common_cmds[7]: #### Ena/Dis Flywheel ####
            
                    pop_text = "cFE TIME outputs an event when it starts/stops flywheel mode\nthat occurs when time can't synch to the 1Hz pulse. Use the\nbuttons to enable/disable the flywheel event messages..."
                    pop_win = sg.Window('Flywheel Message Configuration',
                                        [[sg.Text(pop_text)],
                                        [sg.Text("")],
                                        [sg.Button('Enable',  button_color=('green'), enable_events=True, key='-FLYWHEEL_ENABLE-',  pad=(10,1)),
                                         sg.Button('Disable', button_color=('red'),   enable_events=True, key='-FLYWHEEL_DISABLE-', pad=(10,1)), 
                                         sg.Cancel(button_color=('gray'))]])

                    while True:  # Event Loop
                        pop_event, pop_values = pop_win.read(timeout=200)
                        if pop_event in (sg.WIN_CLOSED, 'Cancel'):
                            break
                        elif pop_event == '-FLYWHEEL_ENABLE-':
                            self.send_cfs_cmd('CFE_EVS', 'SetFilterCmd',  {'AppName': 'CFE_TIME','EventID': Cfe.CFE_TIME_FLY_ON_EID, 'Mask': Cfe.CFE_EVS_NO_FILTER})
                            time.sleep(0.5)
                            self.send_cfs_cmd('CFE_EVS', 'SetFilterCmd',  {'AppName': 'CFE_TIME','EventID': Cfe.CFE_TIME_FLY_OFF_EID, 'Mask': Cfe.CFE_EVS_NO_FILTER})
                            break
                        if pop_event == '-FLYWHEEL_DISABLE-':
                            self.disable_flywheel_event()
                            break
                    pop_win.close()

                elif cfs_config_cmd == self.common_cmds[8]: #### Set KIT_TO Telemetry source ####

                    pop_text = "Remote telemetry should only be selected\nif an app like MQTT_GW is installed that\nsupports routing remote telemetry to KIT_TO..."

                    pop_win = sg.Window('Telemetry Source Configuration',
                                        [[sg.Text(pop_text)],
                                        [sg.Text("")],
                                        [sg.Button('Local',  button_color=('green'), enable_events=True, key='-LOCAL_SRC-',  pad=(10,1)),
                                         sg.Button('Remote', button_color=('green'), enable_events=True, key='-REMOTE_SRC-', pad=(10,1)), 
                                         sg.Cancel(button_color=('gray'))]])
                
                    while True:  # Event Loop
                        pop_event, pop_values = pop_win.read(timeout=200)
                        if pop_event in (sg.WIN_CLOSED, 'Cancel'):
                            break
                        elif pop_event == '-LOCAL_SRC-':
                            self.send_cfs_cmd('KIT_TO', 'SetTlmSource',  {'Source': 'LOCAL'})
                            break
                        if pop_event == '-REMOTE_SRC-':
                            #self.cmd_tlm_router.set_cfs_ip_addr('192.168.0.3')
                            self.send_cfs_cmd('KIT_TO', 'SetTlmSource',  {'Source': 'REMOTE'})
                            break
                    pop_win.close()
      
            
                elif cfs_config_cmd == self.common_cmds[9]: #### cFE Version (CFE ES Noop) ####
                    self.send_cfs_cmd('CFE_ES', 'NoopCmd', {})
            
                   
            elif self.event == '-CMD_TOPICS-':
                """
                This is very similar to self.send_cfs_cmd() but not exact so a common function was not created.   
                TODO: Create a command string for event window. Raw text may be an option so people can capture commands
                """
                cmd_topic = self.values['-CMD_TOPICS-']
                if cmd_topic != EdsMission.TOPIC_CMD_TITLE_KEY:
                    if self.cfs_cmd_dest == self.CFS_CMD_DEST.UDP:
                        (cmd_sent, cmd_text, cmd_status, cmd_obj) = self.telecommand_gui.execute(cmd_topic)
                        self.display_event(cmd_status)
                        #TODO: Add config switch? self.display_event(cmd_text)
                    elif self.cfs_cmd_dest == self.CFS_CMD_DEST.MQTT:
                        if self.cfs_mqtt_cmd_client.connected:
                            (cmd_valid, cmd_text, cmd_status, cmd_obj) = self.telecommand_gui.execute(cmd_topic, return_cmd=True)
                            if cmd_valid:
                                self.send_cfs_mqtt_cmd(cmd_obj)
                            else:
                                self.display_event(cmd_status) # cmd_status will describe success and failure cases
                        else:
                            self.display_event(f'Failed to send {app_name}:{cmd_name}. Configured for MQTT commanding, but MQTT client is disconnected.')
                else:
                    sg.popup('Please select a command topic from the dropdown list', title='Command Topic', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
            
            elif self.event == '-TLM_TOPICS-':
                tlm_topic = self.values['-TLM_TOPICS-']
                if tlm_topic != EdsMission.TOPIC_TLM_TITLE_KEY:                    
                    app_name = self.tlm_server.get_app_name_from_topic(tlm_topic)
                    tlm_screen_cmd_parms = f'{self.tlm_screen_port} {app_name} {tlm_topic}'
                    self.cmd_tlm_router.add_gnd_tlm_dest(self.tlm_screen_port)
                    self.tlm_screen = sg.execute_py_file("tlmscreen.py", parms=tlm_screen_cmd_parms, cwd=self.cfs_interface_dir)
                    self.display_event(f'Created telemetry screen for {tlm_topic} on port {self.tlm_screen_port}')
                    self.tlm_screen_port += 1
                else:
                    sg.popup('Please select a telemetry topic from the dropdown list', title='Telemetry Topic', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
                
            elif self.event == '-CLEAR_EVENTS-':
                self.event_log = ""
                self.display_event('Cleared event display')

        self.shutdown()

        return restart
        
if __name__ == '__main__':

    image_grey1 = b'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/GMzLdsAADsHaVRYdFhNTDpjb20uYWRvYmUueG1wAAAAAAA8P3hwYWNrZXQgYmVnaW49Iu+7vyIgaWQ9Ilc1TTBNcENlaGlIenJlU3pOVGN6a2M5ZCI/Pgo8eDp4bXBtZXRhIHhtbG5zOng9ImFkb2JlOm5zOm1ldGEvIiB4OnhtcHRrPSJBZG9iZSBYTVAgQ29yZSA1LjYtYzE0NSA3OS4xNjIzMTksIDIwMTgvMDIvMTUtMjA6Mjk6NDMgICAgICAgICI+CiAgIDxyZGY6UkRGIHhtbG5zOnJkZj0iaHR0cDovL3d3dy53My5vcmcvMTk5OS8wMi8yMi1yZGYtc3ludGF4LW5zIyI+CiAgICAgIDxyZGY6RGVzY3JpcHRpb24gcmRmOmFib3V0PSIiCiAgICAgICAgICAgIHhtbG5zOnhtcD0iaHR0cDovL25zLmFkb2JlLmNvbS94YXAvMS4wLyIKICAgICAgICAgICAgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iCiAgICAgICAgICAgIHhtbG5zOnN0RXZ0PSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VFdmVudCMiCiAgICAgICAgICAgIHhtbG5zOnBob3Rvc2hvcD0iaHR0cDovL25zLmFkb2JlLmNvbS9waG90b3Nob3AvMS4wLyIKICAgICAgICAgICAgeG1sbnM6ZGM9Imh0dHA6Ly9wdXJsLm9yZy9kYy9lbGVtZW50cy8xLjEvIgogICAgICAgICAgICB4bWxuczp0aWZmPSJodHRwOi8vbnMuYWRvYmUuY29tL3RpZmYvMS4wLyIKICAgICAgICAgICAgeG1sbnM6ZXhpZj0iaHR0cDovL25zLmFkb2JlLmNvbS9leGlmLzEuMC8iPgogICAgICAgICA8eG1wOkNyZWF0b3JUb29sPkFkb2JlIFBob3Rvc2hvcCBFbGVtZW50cyAxNy4wIChXaW5kb3dzKTwveG1wOkNyZWF0b3JUb29sPgogICAgICAgICA8eG1wOkNyZWF0ZURhdGU+MjAyMC0xMC0wM1QxMToyOTozMi0wNDowMDwveG1wOkNyZWF0ZURhdGU+CiAgICAgICAgIDx4bXA6TWV0YWRhdGFEYXRlPjIwMjAtMTAtMDNUMTE6Mjk6MzItMDQ6MDA8L3htcDpNZXRhZGF0YURhdGU+CiAgICAgICAgIDx4bXA6TW9kaWZ5RGF0ZT4yMDIwLTEwLTAzVDExOjI5OjMyLTA0OjAwPC94bXA6TW9kaWZ5RGF0ZT4KICAgICAgICAgPHhtcE1NOkluc3RhbmNlSUQ+eG1wLmlpZDo2Y2Q5MjZlZS0xYWE3LTBlNDEtYTI2ZS04MmMwMGYyN2E2Nzg8L3htcE1NOkluc3RhbmNlSUQ+CiAgICAgICAgIDx4bXBNTTpEb2N1bWVudElEPmFkb2JlOmRvY2lkOnBob3Rvc2hvcDozMzlhMjcxYS0wNThkLTExZWItOTQ3ZC04N2E5Njc3OWZkYzU8L3htcE1NOkRvY3VtZW50SUQ+CiAgICAgICAgIDx4bXBNTTpPcmlnaW5hbERvY3VtZW50SUQ+eG1wLmRpZDpjZDY3N2JmMi02YjVjLWU4NDgtYTI0OC1kOGRkNGNkZTBkMzM8L3htcE1NOk9yaWdpbmFsRG9jdW1lbnRJRD4KICAgICAgICAgPHhtcE1NOkhpc3Rvcnk+CiAgICAgICAgICAgIDxyZGY6U2VxPgogICAgICAgICAgICAgICA8cmRmOmxpIHJkZjpwYXJzZVR5cGU9IlJlc291cmNlIj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OmFjdGlvbj5jcmVhdGVkPC9zdEV2dDphY3Rpb24+CiAgICAgICAgICAgICAgICAgIDxzdEV2dDppbnN0YW5jZUlEPnhtcC5paWQ6Y2Q2NzdiZjItNmI1Yy1lODQ4LWEyNDgtZDhkZDRjZGUwZDMzPC9zdEV2dDppbnN0YW5jZUlEPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6d2hlbj4yMDIwLTEwLTAzVDExOjI5OjMyLTA0OjAwPC9zdEV2dDp3aGVuPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6c29mdHdhcmVBZ2VudD5BZG9iZSBQaG90b3Nob3AgRWxlbWVudHMgMTcuMCAoV2luZG93cyk8L3N0RXZ0OnNvZnR3YXJlQWdlbnQ+CiAgICAgICAgICAgICAgIDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpIHJkZjpwYXJzZVR5cGU9IlJlc291cmNlIj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OmFjdGlvbj5zYXZlZDwvc3RFdnQ6YWN0aW9uPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6aW5zdGFuY2VJRD54bXAuaWlkOjZjZDkyNmVlLTFhYTctMGU0MS1hMjZlLTgyYzAwZjI3YTY3ODwvc3RFdnQ6aW5zdGFuY2VJRD4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OndoZW4+MjAyMC0xMC0wM1QxMToyOTozMi0wNDowMDwvc3RFdnQ6d2hlbj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OnNvZnR3YXJlQWdlbnQ+QWRvYmUgUGhvdG9zaG9wIEVsZW1lbnRzIDE3LjAgKFdpbmRvd3MpPC9zdEV2dDpzb2Z0d2FyZUFnZW50PgogICAgICAgICAgICAgICAgICA8c3RFdnQ6Y2hhbmdlZD4vPC9zdEV2dDpjaGFuZ2VkPgogICAgICAgICAgICAgICA8L3JkZjpsaT4KICAgICAgICAgICAgPC9yZGY6U2VxPgogICAgICAgICA8L3htcE1NOkhpc3Rvcnk+CiAgICAgICAgIDxwaG90b3Nob3A6RG9jdW1lbnRBbmNlc3RvcnM+CiAgICAgICAgICAgIDxyZGY6QmFnPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MDE4MDExNzQwNzIwNjgxMTg3MUZEQjdDNzNFQzdBRjQ8L3JkZjpsaT4KICAgICAgICAgICAgPC9yZGY6QmFnPgogICAgICAgICA8L3Bob3Rvc2hvcDpEb2N1bWVudEFuY2VzdG9ycz4KICAgICAgICAgPHBob3Rvc2hvcDpDb2xvck1vZGU+MzwvcGhvdG9zaG9wOkNvbG9yTW9kZT4KICAgICAgICAgPHBob3Rvc2hvcDpJQ0NQcm9maWxlPnNSR0IgSUVDNjE5NjYtMi4xPC9waG90b3Nob3A6SUNDUHJvZmlsZT4KICAgICAgICAgPGRjOmZvcm1hdD5pbWFnZS9wbmc8L2RjOmZvcm1hdD4KICAgICAgICAgPHRpZmY6T3JpZW50YXRpb24+MTwvdGlmZjpPcmllbnRhdGlvbj4KICAgICAgICAgPHRpZmY6WFJlc29sdXRpb24+NzIwMDAwLzEwMDAwPC90aWZmOlhSZXNvbHV0aW9uPgogICAgICAgICA8dGlmZjpZUmVzb2x1dGlvbj43MjAwMDAvMTAwMDA8L3RpZmY6WVJlc29sdXRpb24+CiAgICAgICAgIDx0aWZmOlJlc29sdXRpb25Vbml0PjI8L3RpZmY6UmVzb2x1dGlvblVuaXQ+CiAgICAgICAgIDxleGlmOkNvbG9yU3BhY2U+MTwvZXhpZjpDb2xvclNwYWNlPgogICAgICAgICA8ZXhpZjpQaXhlbFhEaW1lbnNpb24+ODI8L2V4aWY6UGl4ZWxYRGltZW5zaW9uPgogICAgICAgICA8ZXhpZjpQaXhlbFlEaW1lbnNpb24+MzI8L2V4aWY6UGl4ZWxZRGltZW5zaW9uPgogICAgICA8L3JkZjpEZXNjcmlwdGlvbj4KICAgPC9yZGY6UkRGPgo8L3g6eG1wbWV0YT4KICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAKPD94cGFja2V0IGVuZD0idyI/PkjKk0kAAAAgY0hSTQAAeiUAAICDAAD5/wAAgOkAAHUwAADqYAAAOpgAABdvkl/FRgAAAgtJREFUeNrsmi1vFFEUhp97Z6b7PYtuNyQYQkIwBNNgoAKBA4FAIJAIdP/BViOQCAQCgUQSBB+GYDZtCGnShKAIgp3dndl05p6L2G2ABrrscgXivGbEuCfvc8+ZyTXM8/Dx03ONZmvHWrtljOmg+WO89yMReVHkk+17d259ADAADx49OZ92T70+c7rXTdMuePB4JfabGAwYyLIhB58+D7Pht8v3797ejYGo0Wz1exvr3XitznCUI14hnhRrDMland7GevegLPvAjRhIrI2uJrUaxWGplP4i4j3VoZDUalgbbQFJDERAuyoFr01cKlUpAC0gigEr3uMU4tJx3h8dgzYGEBFERMmsovmcWzwf52ghV16FfoAU8YjXRq7WyF9ACl60kv+stnjR3XHlVUiOq60gg6gtqnYAtXX9CQVS1Va1/7ep7VTtQFNbGxlGbadfNqGmtjYyzNTWMzKA2l5w2sgQw0YX8mBqayMDqO29npGr5tiPXVU7iNpOZFxVrm2sVTLLtFEEJ5IfgZRpkb/Jx9m1RjtVOkukGGdMi/wVIDHg9vcG/bWktpk612m0UrSZi5tYTDKyr19G+3uDPuAMswsCnSvXb146e+Hidr3e2DTWNhXXiSDz6bR4+3Hwfufl82fvgJFhdpEqZnZjoA3UAa3kghkDTIExMAEq89PLCEjmTwW5GKQDyvmT7wMAzpNJbp+doKQAAAAASUVORK5CYII='
    download = b'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/GMzLdsAAFTiaVRYdFhNTDpjb20uYWRvYmUueG1wAAAAAAA8P3hwYWNrZXQgYmVnaW49Iu+7vyIgaWQ9Ilc1TTBNcENlaGlIenJlU3pOVGN6a2M5ZCI/Pgo8eDp4bXBtZXRhIHhtbG5zOng9ImFkb2JlOm5zOm1ldGEvIiB4OnhtcHRrPSJBZG9iZSBYTVAgQ29yZSA1LjYtYzE0NSA3OS4xNjIzMTksIDIwMTgvMDIvMTUtMjA6Mjk6NDMgICAgICAgICI+CiAgIDxyZGY6UkRGIHhtbG5zOnJkZj0iaHR0cDovL3d3dy53My5vcmcvMTk5OS8wMi8yMi1yZGYtc3ludGF4LW5zIyI+CiAgICAgIDxyZGY6RGVzY3JpcHRpb24gcmRmOmFib3V0PSIiCiAgICAgICAgICAgIHhtbG5zOnhtcD0iaHR0cDovL25zLmFkb2JlLmNvbS94YXAvMS4wLyIKICAgICAgICAgICAgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iCiAgICAgICAgICAgIHhtbG5zOnN0RXZ0PSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VFdmVudCMiCiAgICAgICAgICAgIHhtbG5zOnBob3Rvc2hvcD0iaHR0cDovL25zLmFkb2JlLmNvbS9waG90b3Nob3AvMS4wLyIKICAgICAgICAgICAgeG1sbnM6ZGM9Imh0dHA6Ly9wdXJsLm9yZy9kYy9lbGVtZW50cy8xLjEvIgogICAgICAgICAgICB4bWxuczp0aWZmPSJodHRwOi8vbnMuYWRvYmUuY29tL3RpZmYvMS4wLyIKICAgICAgICAgICAgeG1sbnM6ZXhpZj0iaHR0cDovL25zLmFkb2JlLmNvbS9leGlmLzEuMC8iPgogICAgICAgICA8eG1wOkNyZWF0b3JUb29sPkFkb2JlIFBob3Rvc2hvcCBFbGVtZW50cyAxNy4wIChXaW5kb3dzKTwveG1wOkNyZWF0b3JUb29sPgogICAgICAgICA8eG1wOkNyZWF0ZURhdGU+MjAyMC0xMC0wM1QxMTowMToyOC0wNDowMDwveG1wOkNyZWF0ZURhdGU+CiAgICAgICAgIDx4bXA6TWV0YWRhdGFEYXRlPjIwMjAtMTAtMDNUMTE6MDE6MjgtMDQ6MDA8L3htcDpNZXRhZGF0YURhdGU+CiAgICAgICAgIDx4bXA6TW9kaWZ5RGF0ZT4yMDIwLTEwLTAzVDExOjAxOjI4LTA0OjAwPC94bXA6TW9kaWZ5RGF0ZT4KICAgICAgICAgPHhtcE1NOkluc3RhbmNlSUQ+eG1wLmlpZDpmZjIwNWM4Zi05NDY4LWIyNDQtODI4NC02MDAxMGNiZjJhYzM8L3htcE1NOkluc3RhbmNlSUQ+CiAgICAgICAgIDx4bXBNTTpEb2N1bWVudElEPmFkb2JlOmRvY2lkOnBob3Rvc2hvcDoyNWI2NmI0OC0wNTg5LTExZWItOTQ3ZC04N2E5Njc3OWZkYzU8L3htcE1NOkRvY3VtZW50SUQ+CiAgICAgICAgIDx4bXBNTTpPcmlnaW5hbERvY3VtZW50SUQ+eG1wLmRpZDo0Yzk1MjVhYy02NTc5LTM0NGMtOTZhMC1kNGU2OTBkNDk0NmI8L3htcE1NOk9yaWdpbmFsRG9jdW1lbnRJRD4KICAgICAgICAgPHhtcE1NOkhpc3Rvcnk+CiAgICAgICAgICAgIDxyZGY6U2VxPgogICAgICAgICAgICAgICA8cmRmOmxpIHJkZjpwYXJzZVR5cGU9IlJlc291cmNlIj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OmFjdGlvbj5jcmVhdGVkPC9zdEV2dDphY3Rpb24+CiAgICAgICAgICAgICAgICAgIDxzdEV2dDppbnN0YW5jZUlEPnhtcC5paWQ6NGM5NTI1YWMtNjU3OS0zNDRjLTk2YTAtZDRlNjkwZDQ5NDZiPC9zdEV2dDppbnN0YW5jZUlEPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6d2hlbj4yMDIwLTEwLTAzVDExOjAxOjI4LTA0OjAwPC9zdEV2dDp3aGVuPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6c29mdHdhcmVBZ2VudD5BZG9iZSBQaG90b3Nob3AgRWxlbWVudHMgMTcuMCAoV2luZG93cyk8L3N0RXZ0OnNvZnR3YXJlQWdlbnQ+CiAgICAgICAgICAgICAgIDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpIHJkZjpwYXJzZVR5cGU9IlJlc291cmNlIj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OmFjdGlvbj5zYXZlZDwvc3RFdnQ6YWN0aW9uPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6aW5zdGFuY2VJRD54bXAuaWlkOmZmMjA1YzhmLTk0NjgtYjI0NC04Mjg0LTYwMDEwY2JmMmFjMzwvc3RFdnQ6aW5zdGFuY2VJRD4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OndoZW4+MjAyMC0xMC0wM1QxMTowMToyOC0wNDowMDwvc3RFdnQ6d2hlbj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OnNvZnR3YXJlQWdlbnQ+QWRvYmUgUGhvdG9zaG9wIEVsZW1lbnRzIDE3LjAgKFdpbmRvd3MpPC9zdEV2dDpzb2Z0d2FyZUFnZW50PgogICAgICAgICAgICAgICAgICA8c3RFdnQ6Y2hhbmdlZD4vPC9zdEV2dDpjaGFuZ2VkPgogICAgICAgICAgICAgICA8L3JkZjpsaT4KICAgICAgICAgICAgPC9yZGY6U2VxPgogICAgICAgICA8L3htcE1NOkhpc3Rvcnk+CiAgICAgICAgIDxwaG90b3Nob3A6RG9jdW1lbnRBbmNlc3RvcnM+CiAgICAgICAgICAgIDxyZGY6QmFnPgogICAgICAgICAgICAgICA8cmRmOmxpPjAwMDE1N0JCNEVDNjZDODMyQ0VBN0Q0OTgxOEYyQkI3PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+MEQyMERDMEVCMTBDQkE5Njg5N0M2NzNCRjkwNDI5ODQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT4xRDk5RjMzMUY1RkMyOUU0ODU5MkI1OERENENCRkUzMzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPjI2QzMwRDNBRTREQjZERTFFN0Y2M0JCQUE4NjBGNEI0PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+MkFFRkE4NTk0ODJBRTMxMEYwOEYxNEVCQkU3MUEyNTU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT4zNDBDQUZCNkZCMzIwRDRGREVEMjc0M0ExRjUwNUI2ODwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPjQ4NTVBMzI3NzUwOTIzODkwMzQ5NjIwRkU2NUYzNjkxPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+NjZBREMzOThERjcwMDQ1RDgxMkU4OUMwNDIzRkFGNTA8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT42QjI5MkM4MDQyRTY1QTcxMkZGMTk4NTdEMjhGQTZCRTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPjczNzQ2N0JGQzU2QkFDNTk2Q0M4QkNEOUUzNjk2QUU1PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+NzNDMEVENEM3ODE0RTg4RjlBMjQ3NzRFRjdGMTBBODk8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT44MUU0QzY3QjQ5QkFCMzlDNkU5QzExRjQxNUNEMTgyRTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPkEzQ0M2RjFEQTFDMTFBMDZDOUExOTEyQURDRDBFRjQ3PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+QTQ3QkZCNkY5NkMxRjBGMjhDMTI5RENCQkZBODRGNkQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT5BNTlDQUU4ODNCMzU0RjgyMEQ3OTFEODJCREVGRjE2MDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPkFGREUyQkMyMzA1QkUyRTc2Q0RCNTdBNDAwNzM3MEQyPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+Q0Q4RTcxRkQ1REQ3RkM5MjcyNkZFREQ5NDRBREEyMTE8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT5GM0Y5OURGQjBFMzE5QzUwQzRGNEQ2NUZCM0U1QjU5MzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPkY4REE4QjZDRUM5MDI5OTgzMzUxQkEzQzUyQTVCNzREPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+YWRvYmU6ZG9jaWQ6cGhvdG9zaG9wOjMyMjNhYzg2LWVkZTYtMTFkOS1hZGRiLWNiMWQ2NjAxOWMxNzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPmFkb2JlOmRvY2lkOnBob3Rvc2hvcDo2YjM0YmVlZi1mNDg2LTExZDktOWFiYy1mNmY3MDc0YjFkN2E8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT5hZG9iZTpkb2NpZDpwaG90b3Nob3A6NmNjNzJkZDQtYzEyMi0xMWRhLTllYTAtYjQxMDIxN2JjNjA0PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+YWRvYmU6ZG9jaWQ6cGhvdG9zaG9wOjc4ZTg3ZmZlLWY5NWEtMTFkYi1hZmE5LWY3OGRhN2FiODZmZjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPmFkb2JlOmRvY2lkOnBob3Rvc2hvcDo5NWFkMzRlOC01MTQwLTExZGEtOGFmNi1iNjQ4NmE1YjIwYjI8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT5hZG9iZTpkb2NpZDpwaG90b3Nob3A6YmViNmY0N2UtMjIzNS0xMWRjLWE0NGUtZjZiOGI4MzA4MWQ1PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+YWRvYmU6ZG9jaWQ6cGhvdG9zaG9wOmU3ZmI1OGNhLTk2NDItMTFkZC04MWEyLWRkMTQxMDNmNDUxNTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6MDFGMUEwRjYxMUVGMTFEQjg4MUVBNkJFMDVFMjc2RDE8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOjA2MDFFMzM5QkFGMkREMTE5QTlEOTEwOTg5NjI4QzVGPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDoxODkyOTc1OTlFNUREQzExOEQxOUJGODREMUU2QjZEQTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6MTk1MDE1MDhFMkVGMTFERDhCRUNDQjZCNDU1MkJFQTc8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOjJBNTJBMDEwMENGNERDMTFBQTAzRUZBRjc4RDI5ODlDPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDozMEMyMzlCODJFM0JERDExQjY1MUMwNzNDODY5RDI3MzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6MzUwMEIwQ0M2QzUwMTFEQ0E1OEE4MUNDRUFFQjk2N0Q8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOjRBMzhCMjREN0NCNkREMTE5ODk5OUZCM0IyMkVFNUNFPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDo1MDg1RkUxQzVDRjFEQzExODQ5OTlCMkQ0NzNCNDBDNTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6NTY5OTMxQUNFRTgyREMxMTk4NkVFQTgzNjFGMTQ3MTE8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOjU3RDRENjNDMDUzN0UwMTFBNjM1RUJFMzgyMzBCODQxPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDo1RjgzOTA1RDExRUYxMURCODgxRUE2QkUwNUUyNzZEMTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6NjI1OTE1RTFEMEYyREQxMUIyQUVGODg5ODVDQTU4Njg8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOjcwNkY4OEYyMzAzQkREMTFCNjUxQzA3M0M4NjlEMjczPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDo5MEFERjdENTM5OEExMURGODYxMTk0MzZBMTdDQjIyNzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6QTFFMjhDNEM1NjI5REMxMTlDNzZCNzZDOEFBMDk4NzQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOkE5Rjg5MTI5M0IxQzExREFCNTVCQzI1REIzOTc4NjA2PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDpCNTdGQjJFRUM4MEJEQzExOURCQThEMjJDNkE3OUM0NzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6QkQ1NkJGM0FGQzM4REIxMUEwNzhFRjBBNDMwOTAyRDU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOkJFREZCQTY5MEM2OERCMTE5NEE0RkFCMjk1MERCMjQ1PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDpDQUQxM0U4ODMwM0JERDExQjY1MUMwNzNDODY5RDI3MzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6RDlGRkY3NkJDMThBRTAxMTk3RTJFNkIyMDZFQUIzOTU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOkU0NUNENDFBNzdEN0REMTE4NkRFREQ1OEI3N0JFMDkxPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDpGNkFBNkUxMThCNURFMTExOTE0MUJBNTI3OTk1MzhGMDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MDE4MDExNzQwNzIwNjgxMTg3MUZFNEFENzFFRTI1QzA8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjAxODAxMTc0MDcyMDY4MTE4NzFGRUVCNTZEMUU3NDNCPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDowMTgwMTE3NDA3MjA2ODExOEY2MkVCMkFERTQ2RDI3MzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MDE4MDExNzQwNzIwNjgxMTkxMDlGQTI5N0E3QTU5MDQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjAxODAxMTc0MDcyMDY4MTE5MkIwQkFBOTA0REUwRjhEPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDowMTgwMTE3NDA3MjA2ODExOTdBNURBRjI1ODNBMEE0QjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MDE4MDExNzQwNzIwNjgxMUFCMDhFOEU4RUUzRjAyODk8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjAyODAxMTc0MDcyMDY4MTE4OEM2QTI5ODQ0M0YxMEFDPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDowMjgwMTE3NDA3MjA2ODExOTEwOUM2NUE3MDQwMTM0MDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MEEwOEJDOTI2MUEyREYxMUJCQjVDMUZFMzRDNjY0OEM8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjFDOEU5N0FBOEI0QkRGMTE4OTQxQjEwMTZEQkYxRDE0PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDoxRjgzQ0M1NjM2RTFFMDExQjc1MzgyQTY2NTdEMjA4RDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MzE5RTE0NkRCNDM3RTIxMThENjBEQkI0MTU3NjYyQkU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjM2RThEMTc2QkM3QURGMTFBM0M4QzZFMjExRUE4QkJGPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDo0RDVFODcxRTE1MTFFMjExOTU1M0RENDcyOUEyMzZDRjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6NTE0MDRGNDMzOTQ0RTIxMUEyMzlDMDRBRkQ0NDQ4MUU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjUxNTQ2RkREMTMyMDY4MTE5MkIwQkFBOTA0REUwRjhEPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDo1NDU0NkZERDEzMjA2ODExOTJCMEJBQTkwNERFMEY4RDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6NjM4ODgwRTgwOTIwNjgxMTkyQjBCQUE5MDRERTBGOEQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjY0ODg4MEU4MDkyMDY4MTE5MkIwQkFBOTA0REUwRjhEPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDo2NERGNTM3NjM1RTFFMDExQjc1MzgyQTY2NTdEMjA4RDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6NjY4ODgwRTgwOTIwNjgxMTkyQjBCQUE5MDRERTBGOEQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjY3ODg4MEU4MDkyMDY4MTE5MkIwQkFBOTA0REUwRjhEPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDo2RUQ1RTlDRDZGQzRERjExQjc1QzlCRTFENUIxNkVBMzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6NzNGREFEODU3RUNCRTExMUI1MUVCRkE3RDIwMUNDNEI8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjk2MkFGRDIxNjgzOUUyMTE4NzA1ODUzNjA1RUY2MEJBPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpBMzQ3Q0E3MUNFRENFMTExOUJFN0Y0NzJGQzQyQzU4NTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6QTY1Q0FDMUZDOTM3RTIxMThENjBEQkI0MTU3NjYyQkU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOkE5Q0NBNkFGRkI0M0UxMTE5RURCRjYzRTA1ODA0MzA0PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpCMjQwMUYyQjk2MzhFMjExQTY2MkI0N0U5QjhENzE0RTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6QkIzNTY1RTgxNTIwNjgxMTkyQjBCQUE5MDRERTBGOEQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOkMyMzU2NUU4MTUyMDY4MTE5MkIwQkFBOTA0REUwRjhEPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpDMzczQ0VDMDM2MjA2ODExOEY2MkQwRjcwMTBBQzAyRjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6Q0I1NzY4QzhGQjNBRTIxMUFBNDNGMUNGRTNEMkUyNTM8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOkNGMEY3NzJFOUY5MkUwMTFCMDBBQjc4N0Y4ODIyQjQ2PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpEMDczREU4OURFM0VFMjExQkM2QUZERjZBNEZEMTc1RjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6RDQwNTRFQzExOTIwNjgxMTkyQjBCQUE5MDRERTBGOEQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOkRBQTQ4QUM0NzYzRkUyMTE4M0EyQkZEMTQ2NjBDQTJGPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpFMEE0OEFDNDc2M0ZFMjExODNBMkJGRDE0NjYwQ0EyRjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6Rjc1RDkxOEExMTIwNjgxMTkyQjA4QkVFMjlDNzVERDI8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOkY3N0YxMTc0MDcyMDY4MTE4MDgzRUI4M0M2MkJEN0MxPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpGNzdGMTE3NDA3MjA2ODExOTEwOUY4RkUyNzcxOEQ1QTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6RkJDNEQyMDQwQTIwNjgxMTkxMDlDQzY0MkM0NEVDMEM8L3JkZjpsaT4KICAgICAgICAgICAgPC9yZGY6QmFnPgogICAgICAgICA8L3Bob3Rvc2hvcDpEb2N1bWVudEFuY2VzdG9ycz4KICAgICAgICAgPHBob3Rvc2hvcDpDb2xvck1vZGU+MzwvcGhvdG9zaG9wOkNvbG9yTW9kZT4KICAgICAgICAgPHBob3Rvc2hvcDpJQ0NQcm9maWxlPnNSR0IgSUVDNjE5NjYtMi4xPC9waG90b3Nob3A6SUNDUHJvZmlsZT4KICAgICAgICAgPGRjOmZvcm1hdD5pbWFnZS9wbmc8L2RjOmZvcm1hdD4KICAgICAgICAgPHRpZmY6T3JpZW50YXRpb24+MTwvdGlmZjpPcmllbnRhdGlvbj4KICAgICAgICAgPHRpZmY6WFJlc29sdXRpb24+NzIwMDAwLzEwMDAwPC90aWZmOlhSZXNvbHV0aW9uPgogICAgICAgICA8dGlmZjpZUmVzb2x1dGlvbj43MjAwMDAvMTAwMDA8L3RpZmY6WVJlc29sdXRpb24+CiAgICAgICAgIDx0aWZmOlJlc29sdXRpb25Vbml0PjI8L3RpZmY6UmVzb2x1dGlvblVuaXQ+CiAgICAgICAgIDxleGlmOkNvbG9yU3BhY2U+MTwvZXhpZjpDb2xvclNwYWNlPgogICAgICAgICA8ZXhpZjpQaXhlbFhEaW1lbnNpb24+MTkyPC9leGlmOlBpeGVsWERpbWVuc2lvbj4KICAgICAgICAgPGV4aWY6UGl4ZWxZRGltZW5zaW9uPjUyPC9leGlmOlBpeGVsWURpbWVuc2lvbj4KICAgICAgPC9yZGY6RGVzY3JpcHRpb24+CiAgIDwvcmRmOlJERj4KPC94OnhtcG1ldGE+CiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAg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'
    download2 = b'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/GMzLdsAAFTiaVRYdFhNTDpjb20uYWRvYmUueG1wAAAAAAA8P3hwYWNrZXQgYmVnaW49Iu+7vyIgaWQ9Ilc1TTBNcENlaGlIenJlU3pOVGN6a2M5ZCI/Pgo8eDp4bXBtZXRhIHhtbG5zOng9ImFkb2JlOm5zOm1ldGEvIiB4OnhtcHRrPSJBZG9iZSBYTVAgQ29yZSA1LjYtYzE0NSA3OS4xNjIzMTksIDIwMTgvMDIvMTUtMjA6Mjk6NDMgICAgICAgICI+CiAgIDxyZGY6UkRGIHhtbG5zOnJkZj0iaHR0cDovL3d3dy53My5vcmcvMTk5OS8wMi8yMi1yZGYtc3ludGF4LW5zIyI+CiAgICAgIDxyZGY6RGVzY3JpcHRpb24gcmRmOmFib3V0PSIiCiAgICAgICAgICAgIHhtbG5zOnhtcD0iaHR0cDovL25zLmFkb2JlLmNvbS94YXAvMS4wLyIKICAgICAgICAgICAgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iCiAgICAgICAgICAgIHhtbG5zOnN0RXZ0PSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VFdmVudCMiCiAgICAgICAgICAgIHhtbG5zOnBob3Rvc2hvcD0iaHR0cDovL25zLmFkb2JlLmNvbS9waG90b3Nob3AvMS4wLyIKICAgICAgICAgICAgeG1sbnM6ZGM9Imh0dHA6Ly9wdXJsLm9yZy9kYy9lbGVtZW50cy8xLjEvIgogICAgICAgICAgICB4bWxuczp0aWZmPSJodHRwOi8vbnMuYWRvYmUuY29tL3RpZmYvMS4wLyIKICAgICAgICAgICAgeG1sbnM6ZXhpZj0iaHR0cDovL25zLmFkb2JlLmNvbS9leGlmLzEuMC8iPgogICAgICAgICA8eG1wOkNyZWF0b3JUb29sPkFkb2JlIFBob3Rvc2hvcCBFbGVtZW50cyAxNy4wIChXaW5kb3dzKTwveG1wOkNyZWF0b3JUb29sPgogICAgICAgICA8eG1wOkNyZWF0ZURhdGU+MjAyMC0xMC0wM1QxMTowNjoxNy0wNDowMDwveG1wOkNyZWF0ZURhdGU+CiAgICAgICAgIDx4bXA6TWV0YWRhdGFEYXRlPjIwMjAtMTAtMDNUMTE6MDY6MTctMDQ6MDA8L3htcDpNZXRhZGF0YURhdGU+CiAgICAgICAgIDx4bXA6TW9kaWZ5RGF0ZT4yMDIwLTEwLTAzVDExOjA2OjE3LTA0OjAwPC94bXA6TW9kaWZ5RGF0ZT4KICAgICAgICAgPHhtcE1NOkluc3RhbmNlSUQ+eG1wLmlpZDpmMmFmNzY4OC01OGIzLWYxNDMtOWMwNC1hOWMyMWI3OWQ4MTg8L3htcE1NOkluc3RhbmNlSUQ+CiAgICAgICAgIDx4bXBNTTpEb2N1bWVudElEPmFkb2JlOmRvY2lkOnBob3Rvc2hvcDpmN2RmNGExMy0wNTg5LTExZWItOTQ3ZC04N2E5Njc3OWZkYzU8L3htcE1NOkRvY3VtZW50SUQ+CiAgICAgICAgIDx4bXBNTTpPcmlnaW5hbERvY3VtZW50SUQ+eG1wLmRpZDphNDg0ZjVlYy1lMTY0LTAyNDYtYjFjNi04NGJlYTJjZGQ5OTk8L3htcE1NOk9yaWdpbmFsRG9jdW1lbnRJRD4KICAgICAgICAgPHhtcE1NOkhpc3Rvcnk+CiAgICAgICAgICAgIDxyZGY6U2VxPgogICAgICAgICAgICAgICA8cmRmOmxpIHJkZjpwYXJzZVR5cGU9IlJlc291cmNlIj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OmFjdGlvbj5jcmVhdGVkPC9zdEV2dDphY3Rpb24+CiAgICAgICAgICAgICAgICAgIDxzdEV2dDppbnN0YW5jZUlEPnhtcC5paWQ6YTQ4NGY1ZWMtZTE2NC0wMjQ2LWIxYzYtODRiZWEyY2RkOTk5PC9zdEV2dDppbnN0YW5jZUlEPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6d2hlbj4yMDIwLTEwLTAzVDExOjA2OjE3LTA0OjAwPC9zdEV2dDp3aGVuPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6c29mdHdhcmVBZ2VudD5BZG9iZSBQaG90b3Nob3AgRWxlbWVudHMgMTcuMCAoV2luZG93cyk8L3N0RXZ0OnNvZnR3YXJlQWdlbnQ+CiAgICAgICAgICAgICAgIDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpIHJkZjpwYXJzZVR5cGU9IlJlc291cmNlIj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OmFjdGlvbj5zYXZlZDwvc3RFdnQ6YWN0aW9uPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6aW5zdGFuY2VJRD54bXAuaWlkOmYyYWY3Njg4LTU4YjMtZjE0My05YzA0LWE5YzIxYjc5ZDgxODwvc3RFdnQ6aW5zdGFuY2VJRD4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OndoZW4+MjAyMC0xMC0wM1QxMTowNjoxNy0wNDowMDwvc3RFdnQ6d2hlbj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OnNvZnR3YXJlQWdlbnQ+QWRvYmUgUGhvdG9zaG9wIEVsZW1lbnRzIDE3LjAgKFdpbmRvd3MpPC9zdEV2dDpzb2Z0d2FyZUFnZW50PgogICAgICAgICAgICAgICAgICA8c3RFdnQ6Y2hhbmdlZD4vPC9zdEV2dDpjaGFuZ2VkPgogICAgICAgICAgICAgICA8L3JkZjpsaT4KICAgICAgICAgICAgPC9yZGY6U2VxPgogICAgICAgICA8L3htcE1NOkhpc3Rvcnk+CiAgICAgICAgIDxwaG90b3Nob3A6RG9jdW1lbnRBbmNlc3RvcnM+CiAgICAgICAgICAgIDxyZGY6QmFnPgogICAgICAgICAgICAgICA8cmRmOmxpPjAwMDE1N0JCNEVDNjZDODMyQ0VBN0Q0OTgxOEYyQkI3PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+MEQyMERDMEVCMTBDQkE5Njg5N0M2NzNCRjkwNDI5ODQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT4xRDk5RjMzMUY1RkMyOUU0ODU5MkI1OERENENCRkUzMzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPjI2QzMwRDNBRTREQjZERTFFN0Y2M0JCQUE4NjBGNEI0PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+MkFFRkE4NTk0ODJBRTMxMEYwOEYxNEVCQkU3MUEyNTU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT4zNDBDQUZCNkZCMzIwRDRGREVEMjc0M0ExRjUwNUI2ODwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPjQ4NTVBMzI3NzUwOTIzODkwMzQ5NjIwRkU2NUYzNjkxPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+NjZBREMzOThERjcwMDQ1RDgxMkU4OUMwNDIzRkFGNTA8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT42QjI5MkM4MDQyRTY1QTcxMkZGMTk4NTdEMjhGQTZCRTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPjczNzQ2N0JGQzU2QkFDNTk2Q0M4QkNEOUUzNjk2QUU1PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+NzNDMEVENEM3ODE0RTg4RjlBMjQ3NzRFRjdGMTBBODk8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT44MUU0QzY3QjQ5QkFCMzlDNkU5QzExRjQxNUNEMTgyRTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPkEzQ0M2RjFEQTFDMTFBMDZDOUExOTEyQURDRDBFRjQ3PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+QTQ3QkZCNkY5NkMxRjBGMjhDMTI5RENCQkZBODRGNkQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT5BNTlDQUU4ODNCMzU0RjgyMEQ3OTFEODJCREVGRjE2MDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPkFGREUyQkMyMzA1QkUyRTc2Q0RCNTdBNDAwNzM3MEQyPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+Q0Q4RTcxRkQ1REQ3RkM5MjcyNkZFREQ5NDRBREEyMTE8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT5GM0Y5OURGQjBFMzE5QzUwQzRGNEQ2NUZCM0U1QjU5MzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPkY4REE4QjZDRUM5MDI5OTgzMzUxQkEzQzUyQTVCNzREPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+YWRvYmU6ZG9jaWQ6cGhvdG9zaG9wOjMyMjNhYzg2LWVkZTYtMTFkOS1hZGRiLWNiMWQ2NjAxOWMxNzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPmFkb2JlOmRvY2lkOnBob3Rvc2hvcDo2YjM0YmVlZi1mNDg2LTExZDktOWFiYy1mNmY3MDc0YjFkN2E8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT5hZG9iZTpkb2NpZDpwaG90b3Nob3A6NmNjNzJkZDQtYzEyMi0xMWRhLTllYTAtYjQxMDIxN2JjNjA0PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+YWRvYmU6ZG9jaWQ6cGhvdG9zaG9wOjc4ZTg3ZmZlLWY5NWEtMTFkYi1hZmE5LWY3OGRhN2FiODZmZjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPmFkb2JlOmRvY2lkOnBob3Rvc2hvcDo5NWFkMzRlOC01MTQwLTExZGEtOGFmNi1iNjQ4NmE1YjIwYjI8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT5hZG9iZTpkb2NpZDpwaG90b3Nob3A6YmViNmY0N2UtMjIzNS0xMWRjLWE0NGUtZjZiOGI4MzA4MWQ1PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+YWRvYmU6ZG9jaWQ6cGhvdG9zaG9wOmU3ZmI1OGNhLTk2NDItMTFkZC04MWEyLWRkMTQxMDNmNDUxNTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6MDFGMUEwRjYxMUVGMTFEQjg4MUVBNkJFMDVFMjc2RDE8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOjA2MDFFMzM5QkFGMkREMTE5QTlEOTEwOTg5NjI4QzVGPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDoxODkyOTc1OTlFNUREQzExOEQxOUJGODREMUU2QjZEQTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6MTk1MDE1MDhFMkVGMTFERDhCRUNDQjZCNDU1MkJFQTc8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOjJBNTJBMDEwMENGNERDMTFBQTAzRUZBRjc4RDI5ODlDPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDozMEMyMzlCODJFM0JERDExQjY1MUMwNzNDODY5RDI3MzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6MzUwMEIwQ0M2QzUwMTFEQ0E1OEE4MUNDRUFFQjk2N0Q8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOjRBMzhCMjREN0NCNkREMTE5ODk5OUZCM0IyMkVFNUNFPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDo1MDg1RkUxQzVDRjFEQzExODQ5OTlCMkQ0NzNCNDBDNTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6NTY5OTMxQUNFRTgyREMxMTk4NkVFQTgzNjFGMTQ3MTE8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOjU3RDRENjNDMDUzN0UwMTFBNjM1RUJFMzgyMzBCODQxPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDo1RjgzOTA1RDExRUYxMURCODgxRUE2QkUwNUUyNzZEMTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6NjI1OTE1RTFEMEYyREQxMUIyQUVGODg5ODVDQTU4Njg8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOjcwNkY4OEYyMzAzQkREMTFCNjUxQzA3M0M4NjlEMjczPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDo5MEFERjdENTM5OEExMURGODYxMTk0MzZBMTdDQjIyNzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6QTFFMjhDNEM1NjI5REMxMTlDNzZCNzZDOEFBMDk4NzQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOkE5Rjg5MTI5M0IxQzExREFCNTVCQzI1REIzOTc4NjA2PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDpCNTdGQjJFRUM4MEJEQzExOURCQThEMjJDNkE3OUM0NzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6QkQ1NkJGM0FGQzM4REIxMUEwNzhFRjBBNDMwOTAyRDU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOkJFREZCQTY5MEM2OERCMTE5NEE0RkFCMjk1MERCMjQ1PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDpDQUQxM0U4ODMwM0JERDExQjY1MUMwNzNDODY5RDI3MzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6RDlGRkY3NkJDMThBRTAxMTk3RTJFNkIyMDZFQUIzOTU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOkU0NUNENDFBNzdEN0REMTE4NkRFREQ1OEI3N0JFMDkxPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDpGNkFBNkUxMThCNURFMTExOTE0MUJBNTI3OTk1MzhGMDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MDE4MDExNzQwNzIwNjgxMTg3MUZFNEFENzFFRTI1QzA8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjAxODAxMTc0MDcyMDY4MTE4NzFGRUVCNTZEMUU3NDNCPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDowMTgwMTE3NDA3MjA2ODExOEY2MkVCMkFERTQ2RDI3MzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MDE4MDExNzQwNzIwNjgxMTkxMDlGQTI5N0E3QTU5MDQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjAxODAxMTc0MDcyMDY4MTE5MkIwQkFBOTA0REUwRjhEPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDowMTgwMTE3NDA3MjA2ODExOTdBNURBRjI1ODNBMEE0QjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MDE4MDExNzQwNzIwNjgxMUFCMDhFOEU4RUUzRjAyODk8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjAyODAxMTc0MDcyMDY4MTE4OEM2QTI5ODQ0M0YxMEFDPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDowMjgwMTE3NDA3MjA2ODExOTEwOUM2NUE3MDQwMTM0MDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MEEwOEJDOTI2MUEyREYxMUJCQjVDMUZFMzRDNjY0OEM8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjFDOEU5N0FBOEI0QkRGMTE4OTQxQjEwMTZEQkYxRDE0PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDoxRjgzQ0M1NjM2RTFFMDExQjc1MzgyQTY2NTdEMjA4RDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MzE5RTE0NkRCNDM3RTIxMThENjBEQkI0MTU3NjYyQkU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjM2RThEMTc2QkM3QURGMTFBM0M4QzZFMjExRUE4QkJGPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDo0RDVFODcxRTE1MTFFMjExOTU1M0RENDcyOUEyMzZDRjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6NTE0MDRGNDMzOTQ0RTIxMUEyMzlDMDRBRkQ0NDQ4MUU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjUxNTQ2RkREMTMyMDY4MTE5MkIwQkFBOTA0REUwRjhEPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDo1NDU0NkZERDEzMjA2ODExOTJCMEJBQTkwNERFMEY4RDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6NjM4ODgwRTgwOTIwNjgxMTkyQjBCQUE5MDRERTBGOEQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjY0ODg4MEU4MDkyMDY4MTE5MkIwQkFBOTA0REUwRjhEPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDo2NERGNTM3NjM1RTFFMDExQjc1MzgyQTY2NTdEMjA4RDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6NjY4ODgwRTgwOTIwNjgxMTkyQjBCQUE5MDRERTBGOEQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjY3ODg4MEU4MDkyMDY4MTE5MkIwQkFBOTA0REUwRjhEPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDo2RUQ1RTlDRDZGQzRERjExQjc1QzlCRTFENUIxNkVBMzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6NzNGREFEODU3RUNCRTExMUI1MUVCRkE3RDIwMUNDNEI8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjk2MkFGRDIxNjgzOUUyMTE4NzA1ODUzNjA1RUY2MEJBPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpBMzQ3Q0E3MUNFRENFMTExOUJFN0Y0NzJGQzQyQzU4NTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6QTY1Q0FDMUZDOTM3RTIxMThENjBEQkI0MTU3NjYyQkU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOkE5Q0NBNkFGRkI0M0UxMTE5RURCRjYzRTA1ODA0MzA0PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpCMjQwMUYyQjk2MzhFMjExQTY2MkI0N0U5QjhENzE0RTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6QkIzNTY1RTgxNTIwNjgxMTkyQjBCQUE5MDRERTBGOEQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOkMyMzU2NUU4MTUyMDY4MTE5MkIwQkFBOTA0REUwRjhEPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpDMzczQ0VDMDM2MjA2ODExOEY2MkQwRjcwMTBBQzAyRjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6Q0I1NzY4QzhGQjNBRTIxMUFBNDNGMUNGRTNEMkUyNTM8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOkNGMEY3NzJFOUY5MkUwMTFCMDBBQjc4N0Y4ODIyQjQ2PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpEMDczREU4OURFM0VFMjExQkM2QUZERjZBNEZEMTc1RjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6RDQwNTRFQzExOTIwNjgxMTkyQjBCQUE5MDRERTBGOEQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOkRBQTQ4QUM0NzYzRkUyMTE4M0EyQkZEMTQ2NjBDQTJGPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpFMEE0OEFDNDc2M0ZFMjExODNBMkJGRDE0NjYwQ0EyRjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6Rjc1RDkxOEExMTIwNjgxMTkyQjA4QkVFMjlDNzVERDI8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOkY3N0YxMTc0MDcyMDY4MTE4MDgzRUI4M0M2MkJEN0MxPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpGNzdGMTE3NDA3MjA2ODExOTEwOUY4RkUyNzcxOEQ1QTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6RkJDNEQyMDQwQTIwNjgxMTkxMDlDQzY0MkM0NEVDMEM8L3JkZjpsaT4KICAgICAgICAgICAgPC9yZGY6QmFnPgogICAgICAgICA8L3Bob3Rvc2hvcDpEb2N1bWVudEFuY2VzdG9ycz4KICAgICAgICAgPHBob3Rvc2hvcDpDb2xvck1vZGU+MzwvcGhvdG9zaG9wOkNvbG9yTW9kZT4KICAgICAgICAgPHBob3Rvc2hvcDpJQ0NQcm9maWxlPnNSR0IgSUVDNjE5NjYtMi4xPC9waG90b3Nob3A6SUNDUHJvZmlsZT4KICAgICAgICAgPGRjOmZvcm1hdD5pbWFnZS9wbmc8L2RjOmZvcm1hdD4KICAgICAgICAgPHRpZmY6T3JpZW50YXRpb24+MTwvdGlmZjpPcmllbnRhdGlvbj4KICAgICAgICAgPHRpZmY6WFJlc29sdXRpb24+NzIwMDAwLzEwMDAwPC90aWZmOlhSZXNvbHV0aW9uPgogICAgICAgICA8dGlmZjpZUmVzb2x1dGlvbj43MjAwMDAvMTAwMDA8L3RpZmY6WVJlc29sdXRpb24+CiAgICAgICAgIDx0aWZmOlJlc29sdXRpb25Vbml0PjI8L3RpZmY6UmVzb2x1dGlvblVuaXQ+CiAgICAgICAgIDxleGlmOkNvbG9yU3BhY2U+MTwvZXhpZjpDb2xvclNwYWNlPgogICAgICAgICA8ZXhpZjpQaXhlbFhEaW1lbnNpb24+MTA5PC9leGlmOlBpeGVsWERpbWVuc2lvbj4KICAgICAgICAgPGV4aWY6UGl4ZWxZRGltZW5zaW9uPjM4PC9leGlmOlBpeGVsWURpbWVuc2lvbj4KICAgICAgPC9yZGY6RGVzY3JpcHRpb24+CiAgIDwvcmRmOlJERj4KPC94OnhtcG1ldGE+CiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAg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'
    download3 = b'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/GMzLdsAAFTiaVRYdFhNTDpjb20uYWRvYmUueG1wAAAAAAA8P3hwYWNrZXQgYmVnaW49Iu+7vyIgaWQ9Ilc1TTBNcENlaGlIenJlU3pOVGN6a2M5ZCI/Pgo8eDp4bXBtZXRhIHhtbG5zOng9ImFkb2JlOm5zOm1ldGEvIiB4OnhtcHRrPSJBZG9iZSBYTVAgQ29yZSA1LjYtYzE0NSA3OS4xNjIzMTksIDIwMTgvMDIvMTUtMjA6Mjk6NDMgICAgICAgICI+CiAgIDxyZGY6UkRGIHhtbG5zOnJkZj0iaHR0cDovL3d3dy53My5vcmcvMTk5OS8wMi8yMi1yZGYtc3ludGF4LW5zIyI+CiAgICAgIDxyZGY6RGVzY3JpcHRpb24gcmRmOmFib3V0PSIiCiAgICAgICAgICAgIHhtbG5zOnhtcD0iaHR0cDovL25zLmFkb2JlLmNvbS94YXAvMS4wLyIKICAgICAgICAgICAgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iCiAgICAgICAgICAgIHhtbG5zOnN0RXZ0PSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VFdmVudCMiCiAgICAgICAgICAgIHhtbG5zOnBob3Rvc2hvcD0iaHR0cDovL25zLmFkb2JlLmNvbS9waG90b3Nob3AvMS4wLyIKICAgICAgICAgICAgeG1sbnM6ZGM9Imh0dHA6Ly9wdXJsLm9yZy9kYy9lbGVtZW50cy8xLjEvIgogICAgICAgICAgICB4bWxuczp0aWZmPSJodHRwOi8vbnMuYWRvYmUuY29tL3RpZmYvMS4wLyIKICAgICAgICAgICAgeG1sbnM6ZXhpZj0iaHR0cDovL25zLmFkb2JlLmNvbS9leGlmLzEuMC8iPgogICAgICAgICA8eG1wOkNyZWF0b3JUb29sPkFkb2JlIFBob3Rvc2hvcCBFbGVtZW50cyAxNy4wIChXaW5kb3dzKTwveG1wOkNyZWF0b3JUb29sPgogICAgICAgICA8eG1wOkNyZWF0ZURhdGU+MjAyMC0xMC0wM1QxMTowOTowOS0wNDowMDwveG1wOkNyZWF0ZURhdGU+CiAgICAgICAgIDx4bXA6TWV0YWRhdGFEYXRlPjIwMjAtMTAtMDNUMTE6MDk6MDktMDQ6MDA8L3htcDpNZXRhZGF0YURhdGU+CiAgICAgICAgIDx4bXA6TW9kaWZ5RGF0ZT4yMDIwLTEwLTAzVDExOjA5OjA5LTA0OjAwPC94bXA6TW9kaWZ5RGF0ZT4KICAgICAgICAgPHhtcE1NOkluc3RhbmNlSUQ+eG1wLmlpZDowNDFhMDAwNy1lMGJlLTk4NGYtODNlMy01ZGQxZTk2NjI5ZWQ8L3htcE1NOkluc3RhbmNlSUQ+CiAgICAgICAgIDx4bXBNTTpEb2N1bWVudElEPmFkb2JlOmRvY2lkOnBob3Rvc2hvcDo1YjgyOTQ2Mi0wNThhLTExZWItOTQ3ZC04N2E5Njc3OWZkYzU8L3htcE1NOkRvY3VtZW50SUQ+CiAgICAgICAgIDx4bXBNTTpPcmlnaW5hbERvY3VtZW50SUQ+eG1wLmRpZDoxMWMzOWY4MS0xM2VmLWIzNGMtYWNkMy0xZTVjMTI5OWNmMGM8L3htcE1NOk9yaWdpbmFsRG9jdW1lbnRJRD4KICAgICAgICAgPHhtcE1NOkhpc3Rvcnk+CiAgICAgICAgICAgIDxyZGY6U2VxPgogICAgICAgICAgICAgICA8cmRmOmxpIHJkZjpwYXJzZVR5cGU9IlJlc291cmNlIj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OmFjdGlvbj5jcmVhdGVkPC9zdEV2dDphY3Rpb24+CiAgICAgICAgICAgICAgICAgIDxzdEV2dDppbnN0YW5jZUlEPnhtcC5paWQ6MTFjMzlmODEtMTNlZi1iMzRjLWFjZDMtMWU1YzEyOTljZjBjPC9zdEV2dDppbnN0YW5jZUlEPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6d2hlbj4yMDIwLTEwLTAzVDExOjA5OjA5LTA0OjAwPC9zdEV2dDp3aGVuPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6c29mdHdhcmVBZ2VudD5BZG9iZSBQaG90b3Nob3AgRWxlbWVudHMgMTcuMCAoV2luZG93cyk8L3N0RXZ0OnNvZnR3YXJlQWdlbnQ+CiAgICAgICAgICAgICAgIDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpIHJkZjpwYXJzZVR5cGU9IlJlc291cmNlIj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OmFjdGlvbj5zYXZlZDwvc3RFdnQ6YWN0aW9uPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6aW5zdGFuY2VJRD54bXAuaWlkOjA0MWEwMDA3LWUwYmUtOTg0Zi04M2UzLTVkZDFlOTY2MjllZDwvc3RFdnQ6aW5zdGFuY2VJRD4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OndoZW4+MjAyMC0xMC0wM1QxMTowOTowOS0wNDowMDwvc3RFdnQ6d2hlbj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OnNvZnR3YXJlQWdlbnQ+QWRvYmUgUGhvdG9zaG9wIEVsZW1lbnRzIDE3LjAgKFdpbmRvd3MpPC9zdEV2dDpzb2Z0d2FyZUFnZW50PgogICAgICAgICAgICAgICAgICA8c3RFdnQ6Y2hhbmdlZD4vPC9zdEV2dDpjaGFuZ2VkPgogICAgICAgICAgICAgICA8L3JkZjpsaT4KICAgICAgICAgICAgPC9yZGY6U2VxPgogICAgICAgICA8L3htcE1NOkhpc3Rvcnk+CiAgICAgICAgIDxwaG90b3Nob3A6RG9jdW1lbnRBbmNlc3RvcnM+CiAgICAgICAgICAgIDxyZGY6QmFnPgogICAgICAgICAgICAgICA8cmRmOmxpPjAwMDE1N0JCNEVDNjZDODMyQ0VBN0Q0OTgxOEYyQkI3PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+MEQyMERDMEVCMTBDQkE5Njg5N0M2NzNCRjkwNDI5ODQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT4xRDk5RjMzMUY1RkMyOUU0ODU5MkI1OERENENCRkUzMzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPjI2QzMwRDNBRTREQjZERTFFN0Y2M0JCQUE4NjBGNEI0PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+MkFFRkE4NTk0ODJBRTMxMEYwOEYxNEVCQkU3MUEyNTU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT4zNDBDQUZCNkZCMzIwRDRGREVEMjc0M0ExRjUwNUI2ODwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPjQ4NTVBMzI3NzUwOTIzODkwMzQ5NjIwRkU2NUYzNjkxPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+NjZBREMzOThERjcwMDQ1RDgxMkU4OUMwNDIzRkFGNTA8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT42QjI5MkM4MDQyRTY1QTcxMkZGMTk4NTdEMjhGQTZCRTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPjczNzQ2N0JGQzU2QkFDNTk2Q0M4QkNEOUUzNjk2QUU1PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+NzNDMEVENEM3ODE0RTg4RjlBMjQ3NzRFRjdGMTBBODk8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT44MUU0QzY3QjQ5QkFCMzlDNkU5QzExRjQxNUNEMTgyRTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPkEzQ0M2RjFEQTFDMTFBMDZDOUExOTEyQURDRDBFRjQ3PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+QTQ3QkZCNkY5NkMxRjBGMjhDMTI5RENCQkZBODRGNkQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT5BNTlDQUU4ODNCMzU0RjgyMEQ3OTFEODJCREVGRjE2MDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPkFGREUyQkMyMzA1QkUyRTc2Q0RCNTdBNDAwNzM3MEQyPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+Q0Q4RTcxRkQ1REQ3RkM5MjcyNkZFREQ5NDRBREEyMTE8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT5GM0Y5OURGQjBFMzE5QzUwQzRGNEQ2NUZCM0U1QjU5MzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPkY4REE4QjZDRUM5MDI5OTgzMzUxQkEzQzUyQTVCNzREPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+YWRvYmU6ZG9jaWQ6cGhvdG9zaG9wOjMyMjNhYzg2LWVkZTYtMTFkOS1hZGRiLWNiMWQ2NjAxOWMxNzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPmFkb2JlOmRvY2lkOnBob3Rvc2hvcDo2YjM0YmVlZi1mNDg2LTExZDktOWFiYy1mNmY3MDc0YjFkN2E8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT5hZG9iZTpkb2NpZDpwaG90b3Nob3A6NmNjNzJkZDQtYzEyMi0xMWRhLTllYTAtYjQxMDIxN2JjNjA0PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+YWRvYmU6ZG9jaWQ6cGhvdG9zaG9wOjc4ZTg3ZmZlLWY5NWEtMTFkYi1hZmE5LWY3OGRhN2FiODZmZjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPmFkb2JlOmRvY2lkOnBob3Rvc2hvcDo5NWFkMzRlOC01MTQwLTExZGEtOGFmNi1iNjQ4NmE1YjIwYjI8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT5hZG9iZTpkb2NpZDpwaG90b3Nob3A6YmViNmY0N2UtMjIzNS0xMWRjLWE0NGUtZjZiOGI4MzA4MWQ1PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+YWRvYmU6ZG9jaWQ6cGhvdG9zaG9wOmU3ZmI1OGNhLTk2NDItMTFkZC04MWEyLWRkMTQxMDNmNDUxNTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6MDFGMUEwRjYxMUVGMTFEQjg4MUVBNkJFMDVFMjc2RDE8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOjA2MDFFMzM5QkFGMkREMTE5QTlEOTEwOTg5NjI4QzVGPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDoxODkyOTc1OTlFNUREQzExOEQxOUJGODREMUU2QjZEQTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6MTk1MDE1MDhFMkVGMTFERDhCRUNDQjZCNDU1MkJFQTc8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOjJBNTJBMDEwMENGNERDMTFBQTAzRUZBRjc4RDI5ODlDPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDozMEMyMzlCODJFM0JERDExQjY1MUMwNzNDODY5RDI3MzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6MzUwMEIwQ0M2QzUwMTFEQ0E1OEE4MUNDRUFFQjk2N0Q8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOjRBMzhCMjREN0NCNkREMTE5ODk5OUZCM0IyMkVFNUNFPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDo1MDg1RkUxQzVDRjFEQzExODQ5OTlCMkQ0NzNCNDBDNTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6NTY5OTMxQUNFRTgyREMxMTk4NkVFQTgzNjFGMTQ3MTE8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOjU3RDRENjNDMDUzN0UwMTFBNjM1RUJFMzgyMzBCODQxPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDo1RjgzOTA1RDExRUYxMURCODgxRUE2QkUwNUUyNzZEMTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6NjI1OTE1RTFEMEYyREQxMUIyQUVGODg5ODVDQTU4Njg8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOjcwNkY4OEYyMzAzQkREMTFCNjUxQzA3M0M4NjlEMjczPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDo5MEFERjdENTM5OEExMURGODYxMTk0MzZBMTdDQjIyNzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6QTFFMjhDNEM1NjI5REMxMTlDNzZCNzZDOEFBMDk4NzQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOkE5Rjg5MTI5M0IxQzExREFCNTVCQzI1REIzOTc4NjA2PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDpCNTdGQjJFRUM4MEJEQzExOURCQThEMjJDNkE3OUM0NzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6QkQ1NkJGM0FGQzM4REIxMUEwNzhFRjBBNDMwOTAyRDU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOkJFREZCQTY5MEM2OERCMTE5NEE0RkFCMjk1MERCMjQ1PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDpDQUQxM0U4ODMwM0JERDExQjY1MUMwNzNDODY5RDI3MzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6RDlGRkY3NkJDMThBRTAxMTk3RTJFNkIyMDZFQUIzOTU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOkU0NUNENDFBNzdEN0REMTE4NkRFREQ1OEI3N0JFMDkxPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDpGNkFBNkUxMThCNURFMTExOTE0MUJBNTI3OTk1MzhGMDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MDE4MDExNzQwNzIwNjgxMTg3MUZFNEFENzFFRTI1QzA8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjAxODAxMTc0MDcyMDY4MTE4NzFGRUVCNTZEMUU3NDNCPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDowMTgwMTE3NDA3MjA2ODExOEY2MkVCMkFERTQ2RDI3MzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MDE4MDExNzQwNzIwNjgxMTkxMDlGQTI5N0E3QTU5MDQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjAxODAxMTc0MDcyMDY4MTE5MkIwQkFBOTA0REUwRjhEPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDowMTgwMTE3NDA3MjA2ODExOTdBNURBRjI1ODNBMEE0QjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MDE4MDExNzQwNzIwNjgxMUFCMDhFOEU4RUUzRjAyODk8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjAyODAxMTc0MDcyMDY4MTE4OEM2QTI5ODQ0M0YxMEFDPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDowMjgwMTE3NDA3MjA2ODExOTEwOUM2NUE3MDQwMTM0MDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MEEwOEJDOTI2MUEyREYxMUJCQjVDMUZFMzRDNjY0OEM8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjFDOEU5N0FBOEI0QkRGMTE4OTQxQjEwMTZEQkYxRDE0PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDoxRjgzQ0M1NjM2RTFFMDExQjc1MzgyQTY2NTdEMjA4RDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MzE5RTE0NkRCNDM3RTIxMThENjBEQkI0MTU3NjYyQkU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjM2RThEMTc2QkM3QURGMTFBM0M4QzZFMjExRUE4QkJGPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDo0RDVFODcxRTE1MTFFMjExOTU1M0RENDcyOUEyMzZDRjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6NTE0MDRGNDMzOTQ0RTIxMUEyMzlDMDRBRkQ0NDQ4MUU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjUxNTQ2RkREMTMyMDY4MTE5MkIwQkFBOTA0REUwRjhEPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDo1NDU0NkZERDEzMjA2ODExOTJCMEJBQTkwNERFMEY4RDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6NjM4ODgwRTgwOTIwNjgxMTkyQjBCQUE5MDRERTBGOEQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjY0ODg4MEU4MDkyMDY4MTE5MkIwQkFBOTA0REUwRjhEPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDo2NERGNTM3NjM1RTFFMDExQjc1MzgyQTY2NTdEMjA4RDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6NjY4ODgwRTgwOTIwNjgxMTkyQjBCQUE5MDRERTBGOEQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjY3ODg4MEU4MDkyMDY4MTE5MkIwQkFBOTA0REUwRjhEPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDo2RUQ1RTlDRDZGQzRERjExQjc1QzlCRTFENUIxNkVBMzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6NzNGREFEODU3RUNCRTExMUI1MUVCRkE3RDIwMUNDNEI8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjk2MkFGRDIxNjgzOUUyMTE4NzA1ODUzNjA1RUY2MEJBPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpBMzQ3Q0E3MUNFRENFMTExOUJFN0Y0NzJGQzQyQzU4NTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6QTY1Q0FDMUZDOTM3RTIxMThENjBEQkI0MTU3NjYyQkU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOkE5Q0NBNkFGRkI0M0UxMTE5RURCRjYzRTA1ODA0MzA0PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpCMjQwMUYyQjk2MzhFMjExQTY2MkI0N0U5QjhENzE0RTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6QkIzNTY1RTgxNTIwNjgxMTkyQjBCQUE5MDRERTBGOEQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOkMyMzU2NUU4MTUyMDY4MTE5MkIwQkFBOTA0REUwRjhEPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpDMzczQ0VDMDM2MjA2ODExOEY2MkQwRjcwMTBBQzAyRjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6Q0I1NzY4QzhGQjNBRTIxMUFBNDNGMUNGRTNEMkUyNTM8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOkNGMEY3NzJFOUY5MkUwMTFCMDBBQjc4N0Y4ODIyQjQ2PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpEMDczREU4OURFM0VFMjExQkM2QUZERjZBNEZEMTc1RjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6RDQwNTRFQzExOTIwNjgxMTkyQjBCQUE5MDRERTBGOEQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOkRBQTQ4QUM0NzYzRkUyMTE4M0EyQkZEMTQ2NjBDQTJGPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpFMEE0OEFDNDc2M0ZFMjExODNBMkJGRDE0NjYwQ0EyRjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6Rjc1RDkxOEExMTIwNjgxMTkyQjA4QkVFMjlDNzVERDI8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOkY3N0YxMTc0MDcyMDY4MTE4MDgzRUI4M0M2MkJEN0MxPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpGNzdGMTE3NDA3MjA2ODExOTEwOUY4RkUyNzcxOEQ1QTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6RkJDNEQyMDQwQTIwNjgxMTkxMDlDQzY0MkM0NEVDMEM8L3JkZjpsaT4KICAgICAgICAgICAgPC9yZGY6QmFnPgogICAgICAgICA8L3Bob3Rvc2hvcDpEb2N1bWVudEFuY2VzdG9ycz4KICAgICAgICAgPHBob3Rvc2hvcDpDb2xvck1vZGU+MzwvcGhvdG9zaG9wOkNvbG9yTW9kZT4KICAgICAgICAgPHBob3Rvc2hvcDpJQ0NQcm9maWxlPnNSR0IgSUVDNjE5NjYtMi4xPC9waG90b3Nob3A6SUNDUHJvZmlsZT4KICAgICAgICAgPGRjOmZvcm1hdD5pbWFnZS9wbmc8L2RjOmZvcm1hdD4KICAgICAgICAgPHRpZmY6T3JpZW50YXRpb24+MTwvdGlmZjpPcmllbnRhdGlvbj4KICAgICAgICAgPHRpZmY6WFJlc29sdXRpb24+NzIwMDAwLzEwMDAwPC90aWZmOlhSZXNvbHV0aW9uPgogICAgICAgICA8dGlmZjpZUmVzb2x1dGlvbj43MjAwMDAvMTAwMDA8L3RpZmY6WVJlc29sdXRpb24+CiAgICAgICAgIDx0aWZmOlJlc29sdXRpb25Vbml0PjI8L3RpZmY6UmVzb2x1dGlvblVuaXQ+CiAgICAgICAgIDxleGlmOkNvbG9yU3BhY2U+MTwvZXhpZjpDb2xvclNwYWNlPgogICAgICAgICA8ZXhpZjpQaXhlbFhEaW1lbnNpb24+MTA5PC9leGlmOlBpeGVsWERpbWVuc2lvbj4KICAgICAgICAgPGV4aWY6UGl4ZWxZRGltZW5zaW9uPjM4PC9leGlmOlBpeGVsWURpbWVuc2lvbj4KICAgICAgPC9yZGY6RGVzY3JpcHRpb24+CiAgIDwvcmRmOlJERj4KPC94OnhtcG1ldGE+CiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgCjw/eHBhY2tldCBlbmQ9InciPz4BWge9AAAAIGNIUk0AAHolAACAgwAA+f8AAIDpAAB1MAAA6mAAADqYAAAXb5JfxUYAAAHGSURBVHja7Nu9ihNRHIbx5/xnJpOZaAKbXWIjwUULkTU2rqgoYqM2FnaWfqAXYGWpjbjVegFiue028aPbzgURFuxUtNMNYkSLOJsx59joPZzA+1zC++PMnOa4EAKA3Xq0ceLd59HaZDo746FARVHiqIos2V451Lv39P71HcC7EII9ePbq+Obr91sha3aSrIEz01qRFLxnVk+hrn5eXT188eHtKzsp0Hrzcfexa+7rNIoSS1JwTmtFoxbwsz/UVdp5+2m0BlwzoPfjtz+bFSWWZgKLLeewNCNrlown/jTQM6BbzVzLJakGitkuSZliJbBgQO6SBKcTFvmBczhLAHIDdOuYr3RNnEs1TSA0JTQlNKEpoSmhCU0JTQlNaEpoSmhKaEJTQlNCE5oSmhKa0JTQlNCEpuYIzbfLfKop4m9/0agBb8DeoN/98u9xoYq0EAIr/e5XYM+A73cuD4bt3GaaJt7aufm7lwZDYGzA6OjBxedPbp5/sbrcrZqpfnMxlafGyeXFav3GuZfH+ktDYORCCAYsAaeAC8ARoAXo7VMEX0VgAnwAtoBt4Jv7/1AeaAMHgAUgE1o0aDUwBnaBX4D/CwAA//8DAOgHcZ21310BAAAAAElFTkSuQmCC'
    download4=b'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/GMzLdsAAFTiaVRYdFhNTDpjb20uYWRvYmUueG1wAAAAAAA8P3hwYWNrZXQgYmVnaW49Iu+7vyIgaWQ9Ilc1TTBNcENlaGlIenJlU3pOVGN6a2M5ZCI/Pgo8eDp4bXBtZXRhIHhtbG5zOng9ImFkb2JlOm5zOm1ldGEvIiB4OnhtcHRrPSJBZG9iZSBYTVAgQ29yZSA1LjYtYzE0NSA3OS4xNjIzMTksIDIwMTgvMDIvMTUtMjA6Mjk6NDMgICAgICAgICI+CiAgIDxyZGY6UkRGIHhtbG5zOnJkZj0iaHR0cDovL3d3dy53My5vcmcvMTk5OS8wMi8yMi1yZGYtc3ludGF4LW5zIyI+CiAgICAgIDxyZGY6RGVzY3JpcHRpb24gcmRmOmFib3V0PSIiCiAgICAgICAgICAgIHhtbG5zOnhtcD0iaHR0cDovL25zLmFkb2JlLmNvbS94YXAvMS4wLyIKICAgICAgICAgICAgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iCiAgICAgICAgICAgIHhtbG5zOnN0RXZ0PSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VFdmVudCMiCiAgICAgICAgICAgIHhtbG5zOnBob3Rvc2hvcD0iaHR0cDovL25zLmFkb2JlLmNvbS9waG90b3Nob3AvMS4wLyIKICAgICAgICAgICAgeG1sbnM6ZGM9Imh0dHA6Ly9wdXJsLm9yZy9kYy9lbGVtZW50cy8xLjEvIgogICAgICAgICAgICB4bWxuczp0aWZmPSJodHRwOi8vbnMuYWRvYmUuY29tL3RpZmYvMS4wLyIKICAgICAgICAgICAgeG1sbnM6ZXhpZj0iaHR0cDovL25zLmFkb2JlLmNvbS9leGlmLzEuMC8iPgogICAgICAgICA8eG1wOkNyZWF0b3JUb29sPkFkb2JlIFBob3Rvc2hvcCBFbGVtZW50cyAxNy4wIChXaW5kb3dzKTwveG1wOkNyZWF0b3JUb29sPgogICAgICAgICA8eG1wOkNyZWF0ZURhdGU+MjAyMC0xMC0wM1QxMToxNjo1MC0wNDowMDwveG1wOkNyZWF0ZURhdGU+CiAgICAgICAgIDx4bXA6TWV0YWRhdGFEYXRlPjIwMjAtMTAtMDNUMTE6MTY6NTAtMDQ6MDA8L3htcDpNZXRhZGF0YURhdGU+CiAgICAgICAgIDx4bXA6TW9kaWZ5RGF0ZT4yMDIwLTEwLTAzVDExOjE2OjUwLTA0OjAwPC94bXA6TW9kaWZ5RGF0ZT4KICAgICAgICAgPHhtcE1NOkluc3RhbmNlSUQ+eG1wLmlpZDo3OTNkNzlhNS0zZTAzLTcwNDMtYmEzMS0wNGZhNTk0MTA1NWM8L3htcE1NOkluc3RhbmNlSUQ+CiAgICAgICAgIDx4bXBNTTpEb2N1bWVudElEPmFkb2JlOmRvY2lkOnBob3Rvc2hvcDo2Y2M2NTVmZC0wNThiLTExZWItOTQ3ZC04N2E5Njc3OWZkYzU8L3htcE1NOkRvY3VtZW50SUQ+CiAgICAgICAgIDx4bXBNTTpPcmlnaW5hbERvY3VtZW50SUQ+eG1wLmRpZDplZjg4OTAwZC04M2FiLTA5NDAtOGIwNC02NmZmY2Y4YjBmMDQ8L3htcE1NOk9yaWdpbmFsRG9jdW1lbnRJRD4KICAgICAgICAgPHhtcE1NOkhpc3Rvcnk+CiAgICAgICAgICAgIDxyZGY6U2VxPgogICAgICAgICAgICAgICA8cmRmOmxpIHJkZjpwYXJzZVR5cGU9IlJlc291cmNlIj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OmFjdGlvbj5jcmVhdGVkPC9zdEV2dDphY3Rpb24+CiAgICAgICAgICAgICAgICAgIDxzdEV2dDppbnN0YW5jZUlEPnhtcC5paWQ6ZWY4ODkwMGQtODNhYi0wOTQwLThiMDQtNjZmZmNmOGIwZjA0PC9zdEV2dDppbnN0YW5jZUlEPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6d2hlbj4yMDIwLTEwLTAzVDExOjE2OjUwLTA0OjAwPC9zdEV2dDp3aGVuPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6c29mdHdhcmVBZ2VudD5BZG9iZSBQaG90b3Nob3AgRWxlbWVudHMgMTcuMCAoV2luZG93cyk8L3N0RXZ0OnNvZnR3YXJlQWdlbnQ+CiAgICAgICAgICAgICAgIDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpIHJkZjpwYXJzZVR5cGU9IlJlc291cmNlIj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OmFjdGlvbj5zYXZlZDwvc3RFdnQ6YWN0aW9uPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6aW5zdGFuY2VJRD54bXAuaWlkOjc5M2Q3OWE1LTNlMDMtNzA0My1iYTMxLTA0ZmE1OTQxMDU1Yzwvc3RFdnQ6aW5zdGFuY2VJRD4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OndoZW4+MjAyMC0xMC0wM1QxMToxNjo1MC0wNDowMDwvc3RFdnQ6d2hlbj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OnNvZnR3YXJlQWdlbnQ+QWRvYmUgUGhvdG9zaG9wIEVsZW1lbnRzIDE3LjAgKFdpbmRvd3MpPC9zdEV2dDpzb2Z0d2FyZUFnZW50PgogICAgICAgICAgICAgICAgICA8c3RFdnQ6Y2hhbmdlZD4vPC9zdEV2dDpjaGFuZ2VkPgogICAgICAgICAgICAgICA8L3JkZjpsaT4KICAgICAgICAgICAgPC9yZGY6U2VxPgogICAgICAgICA8L3htcE1NOkhpc3Rvcnk+CiAgICAgICAgIDxwaG90b3Nob3A6RG9jdW1lbnRBbmNlc3RvcnM+CiAgICAgICAgICAgIDxyZGY6QmFnPgogICAgICAgICAgICAgICA8cmRmOmxpPjAwMDE1N0JCNEVDNjZDODMyQ0VBN0Q0OTgxOEYyQkI3PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+MEQyMERDMEVCMTBDQkE5Njg5N0M2NzNCRjkwNDI5ODQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT4xRDk5RjMzMUY1RkMyOUU0ODU5MkI1OERENENCRkUzMzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPjI2QzMwRDNBRTREQjZERTFFN0Y2M0JCQUE4NjBGNEI0PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+MkFFRkE4NTk0ODJBRTMxMEYwOEYxNEVCQkU3MUEyNTU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT4zNDBDQUZCNkZCMzIwRDRGREVEMjc0M0ExRjUwNUI2ODwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPjQ4NTVBMzI3NzUwOTIzODkwMzQ5NjIwRkU2NUYzNjkxPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+NjZBREMzOThERjcwMDQ1RDgxMkU4OUMwNDIzRkFGNTA8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT42QjI5MkM4MDQyRTY1QTcxMkZGMTk4NTdEMjhGQTZCRTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPjczNzQ2N0JGQzU2QkFDNTk2Q0M4QkNEOUUzNjk2QUU1PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+NzNDMEVENEM3ODE0RTg4RjlBMjQ3NzRFRjdGMTBBODk8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT44MUU0QzY3QjQ5QkFCMzlDNkU5QzExRjQxNUNEMTgyRTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPkEzQ0M2RjFEQTFDMTFBMDZDOUExOTEyQURDRDBFRjQ3PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+QTQ3QkZCNkY5NkMxRjBGMjhDMTI5RENCQkZBODRGNkQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT5BNTlDQUU4ODNCMzU0RjgyMEQ3OTFEODJCREVGRjE2MDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPkFGREUyQkMyMzA1QkUyRTc2Q0RCNTdBNDAwNzM3MEQyPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+Q0Q4RTcxRkQ1REQ3RkM5MjcyNkZFREQ5NDRBREEyMTE8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT5GM0Y5OURGQjBFMzE5QzUwQzRGNEQ2NUZCM0U1QjU5MzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPkY4REE4QjZDRUM5MDI5OTgzMzUxQkEzQzUyQTVCNzREPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+YWRvYmU6ZG9jaWQ6cGhvdG9zaG9wOjMyMjNhYzg2LWVkZTYtMTFkOS1hZGRiLWNiMWQ2NjAxOWMxNzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPmFkb2JlOmRvY2lkOnBob3Rvc2hvcDo2YjM0YmVlZi1mNDg2LTExZDktOWFiYy1mNmY3MDc0YjFkN2E8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT5hZG9iZTpkb2NpZDpwaG90b3Nob3A6NmNjNzJkZDQtYzEyMi0xMWRhLTllYTAtYjQxMDIxN2JjNjA0PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+YWRvYmU6ZG9jaWQ6cGhvdG9zaG9wOjc4ZTg3ZmZlLWY5NWEtMTFkYi1hZmE5LWY3OGRhN2FiODZmZjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPmFkb2JlOmRvY2lkOnBob3Rvc2hvcDo5NWFkMzRlOC01MTQwLTExZGEtOGFmNi1iNjQ4NmE1YjIwYjI8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT5hZG9iZTpkb2NpZDpwaG90b3Nob3A6YmViNmY0N2UtMjIzNS0xMWRjLWE0NGUtZjZiOGI4MzA4MWQ1PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+YWRvYmU6ZG9jaWQ6cGhvdG9zaG9wOmU3ZmI1OGNhLTk2NDItMTFkZC04MWEyLWRkMTQxMDNmNDUxNTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6MDFGMUEwRjYxMUVGMTFEQjg4MUVBNkJFMDVFMjc2RDE8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOjA2MDFFMzM5QkFGMkREMTE5QTlEOTEwOTg5NjI4QzVGPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDoxODkyOTc1OTlFNUREQzExOEQxOUJGODREMUU2QjZEQTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6MTk1MDE1MDhFMkVGMTFERDhCRUNDQjZCNDU1MkJFQTc8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOjJBNTJBMDEwMENGNERDMTFBQTAzRUZBRjc4RDI5ODlDPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDozMEMyMzlCODJFM0JERDExQjY1MUMwNzNDODY5RDI3MzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6MzUwMEIwQ0M2QzUwMTFEQ0E1OEE4MUNDRUFFQjk2N0Q8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOjRBMzhCMjREN0NCNkREMTE5ODk5OUZCM0IyMkVFNUNFPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDo1MDg1RkUxQzVDRjFEQzExODQ5OTlCMkQ0NzNCNDBDNTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6NTY5OTMxQUNFRTgyREMxMTk4NkVFQTgzNjFGMTQ3MTE8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOjU3RDRENjNDMDUzN0UwMTFBNjM1RUJFMzgyMzBCODQxPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDo1RjgzOTA1RDExRUYxMURCODgxRUE2QkUwNUUyNzZEMTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6NjI1OTE1RTFEMEYyREQxMUIyQUVGODg5ODVDQTU4Njg8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOjcwNkY4OEYyMzAzQkREMTFCNjUxQzA3M0M4NjlEMjczPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDo5MEFERjdENTM5OEExMURGODYxMTk0MzZBMTdDQjIyNzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6QTFFMjhDNEM1NjI5REMxMTlDNzZCNzZDOEFBMDk4NzQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOkE5Rjg5MTI5M0IxQzExREFCNTVCQzI1REIzOTc4NjA2PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDpCNTdGQjJFRUM4MEJEQzExOURCQThEMjJDNkE3OUM0NzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6QkQ1NkJGM0FGQzM4REIxMUEwNzhFRjBBNDMwOTAyRDU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOkJFREZCQTY5MEM2OERCMTE5NEE0RkFCMjk1MERCMjQ1PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDpDQUQxM0U4ODMwM0JERDExQjY1MUMwNzNDODY5RDI3MzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnV1aWQ6RDlGRkY3NkJDMThBRTAxMTk3RTJFNkIyMDZFQUIzOTU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT51dWlkOkU0NUNENDFBNzdEN0REMTE4NkRFREQ1OEI3N0JFMDkxPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+dXVpZDpGNkFBNkUxMThCNURFMTExOTE0MUJBNTI3OTk1MzhGMDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MDE4MDExNzQwNzIwNjgxMTg3MUZFNEFENzFFRTI1QzA8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjAxODAxMTc0MDcyMDY4MTE4NzFGRUVCNTZEMUU3NDNCPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDowMTgwMTE3NDA3MjA2ODExOEY2MkVCMkFERTQ2RDI3MzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MDE4MDExNzQwNzIwNjgxMTkxMDlGQTI5N0E3QTU5MDQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjAxODAxMTc0MDcyMDY4MTE5MkIwQkFBOTA0REUwRjhEPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDowMTgwMTE3NDA3MjA2ODExOTdBNURBRjI1ODNBMEE0QjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MDE4MDExNzQwNzIwNjgxMUFCMDhFOEU4RUUzRjAyODk8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjAyODAxMTc0MDcyMDY4MTE4OEM2QTI5ODQ0M0YxMEFDPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDowMjgwMTE3NDA3MjA2ODExOTEwOUM2NUE3MDQwMTM0MDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MEEwOEJDOTI2MUEyREYxMUJCQjVDMUZFMzRDNjY0OEM8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjFDOEU5N0FBOEI0QkRGMTE4OTQxQjEwMTZEQkYxRDE0PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDoxRjgzQ0M1NjM2RTFFMDExQjc1MzgyQTY2NTdEMjA4RDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MzE5RTE0NkRCNDM3RTIxMThENjBEQkI0MTU3NjYyQkU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjM2RThEMTc2QkM3QURGMTFBM0M4QzZFMjExRUE4QkJGPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDo0RDVFODcxRTE1MTFFMjExOTU1M0RENDcyOUEyMzZDRjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6NTE0MDRGNDMzOTQ0RTIxMUEyMzlDMDRBRkQ0NDQ4MUU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjUxNTQ2RkREMTMyMDY4MTE5MkIwQkFBOTA0REUwRjhEPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDo1NDU0NkZERDEzMjA2ODExOTJCMEJBQTkwNERFMEY4RDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6NjM4ODgwRTgwOTIwNjgxMTkyQjBCQUE5MDRERTBGOEQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjY0ODg4MEU4MDkyMDY4MTE5MkIwQkFBOTA0REUwRjhEPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDo2NERGNTM3NjM1RTFFMDExQjc1MzgyQTY2NTdEMjA4RDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6NjY4ODgwRTgwOTIwNjgxMTkyQjBCQUE5MDRERTBGOEQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjY3ODg4MEU4MDkyMDY4MTE5MkIwQkFBOTA0REUwRjhEPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDo2RUQ1RTlDRDZGQzRERjExQjc1QzlCRTFENUIxNkVBMzwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6NzNGREFEODU3RUNCRTExMUI1MUVCRkE3RDIwMUNDNEI8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOjk2MkFGRDIxNjgzOUUyMTE4NzA1ODUzNjA1RUY2MEJBPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpBMzQ3Q0E3MUNFRENFMTExOUJFN0Y0NzJGQzQyQzU4NTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6QTY1Q0FDMUZDOTM3RTIxMThENjBEQkI0MTU3NjYyQkU8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOkE5Q0NBNkFGRkI0M0UxMTE5RURCRjYzRTA1ODA0MzA0PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpCMjQwMUYyQjk2MzhFMjExQTY2MkI0N0U5QjhENzE0RTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6QkIzNTY1RTgxNTIwNjgxMTkyQjBCQUE5MDRERTBGOEQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOkMyMzU2NUU4MTUyMDY4MTE5MkIwQkFBOTA0REUwRjhEPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpDMzczQ0VDMDM2MjA2ODExOEY2MkQwRjcwMTBBQzAyRjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6Q0I1NzY4QzhGQjNBRTIxMUFBNDNGMUNGRTNEMkUyNTM8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOkNGMEY3NzJFOUY5MkUwMTFCMDBBQjc4N0Y4ODIyQjQ2PC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpEMDczREU4OURFM0VFMjExQkM2QUZERjZBNEZEMTc1RjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6RDQwNTRFQzExOTIwNjgxMTkyQjBCQUE5MDRERTBGOEQ8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOkRBQTQ4QUM0NzYzRkUyMTE4M0EyQkZEMTQ2NjBDQTJGPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpFMEE0OEFDNDc2M0ZFMjExODNBMkJGRDE0NjYwQ0EyRjwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6Rjc1RDkxOEExMTIwNjgxMTkyQjA4QkVFMjlDNzVERDI8L3JkZjpsaT4KICAgICAgICAgICAgICAgPHJkZjpsaT54bXAuZGlkOkY3N0YxMTc0MDcyMDY4MTE4MDgzRUI4M0M2MkJEN0MxPC9yZGY6bGk+CiAgICAgICAgICAgICAgIDxyZGY6bGk+eG1wLmRpZDpGNzdGMTE3NDA3MjA2ODExOTEwOUY4RkUyNzcxOEQ1QTwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6RkJDNEQyMDQwQTIwNjgxMTkxMDlDQzY0MkM0NEVDMEM8L3JkZjpsaT4KICAgICAgICAgICAgPC9yZGY6QmFnPgogICAgICAgICA8L3Bob3Rvc2hvcDpEb2N1bWVudEFuY2VzdG9ycz4KICAgICAgICAgPHBob3Rvc2hvcDpDb2xvck1vZGU+MzwvcGhvdG9zaG9wOkNvbG9yTW9kZT4KICAgICAgICAgPHBob3Rvc2hvcDpJQ0NQcm9maWxlPnNSR0IgSUVDNjE5NjYtMi4xPC9waG90b3Nob3A6SUNDUHJvZmlsZT4KICAgICAgICAgPGRjOmZvcm1hdD5pbWFnZS9wbmc8L2RjOmZvcm1hdD4KICAgICAgICAgPHRpZmY6T3JpZW50YXRpb24+MTwvdGlmZjpPcmllbnRhdGlvbj4KICAgICAgICAgPHRpZmY6WFJlc29sdXRpb24+NzIwMDAwLzEwMDAwPC90aWZmOlhSZXNvbHV0aW9uPgogICAgICAgICA8dGlmZjpZUmVzb2x1dGlvbj43MjAwMDAvMTAwMDA8L3RpZmY6WVJlc29sdXRpb24+CiAgICAgICAgIDx0aWZmOlJlc29sdXRpb25Vbml0PjI8L3RpZmY6UmVzb2x1dGlvblVuaXQ+CiAgICAgICAgIDxleGlmOkNvbG9yU3BhY2U+MTwvZXhpZjpDb2xvclNwYWNlPgogICAgICAgICA8ZXhpZjpQaXhlbFhEaW1lbnNpb24+MTgyPC9leGlmOlBpeGVsWERpbWVuc2lvbj4KICAgICAgICAgPGV4aWY6UGl4ZWxZRGltZW5zaW9uPjU4PC9leGlmOlBpeGVsWURpbWVuc2lvbj4KICAgICAgPC9yZGY6RGVzY3JpcHRpb24+CiAgIDwvcmRmOlJERj4KPC94OnhtcG1ldGE+CiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgCjw/eHBhY2tldCBlbmQ9InciPz48XvJIAAAAIGNIUk0AAHolAACAgwAA+f8AAIDpAAB1MAAA6mAAADqYAAAXb5JfxUYAAAR0SURBVHja7J2/bxNnHIef93z+EeB62KhqTNRABzeQNUMZytChHVpU/oEydkgkBMrkLcp/AGLI0o3NGy1dqo4sDKxJ1AwVVOCqKHGikGD77Pt28Lmx3ABSFs7H5zl5ucTLq48/et7v+XzOzBinVqsVd3Z2Pj44ODjV6XSmAIcQ6cGCIGjn8/mDSqXycmtrqzP+D24s2K5cLl/Y3d0NgAOgDXSBPmBaT5ECHOABRaAEnDp79uyrVqv1dDSjo8F2pVLp806n0wH2klD3klDHWk+RIjwgB/hJuMNisVhst9t//BduM8PMXLVanQVmgKnkjUJMSsingJlqtTprZs7M8AGWl5dLzWbTA14CbTOz2Wu3LwGLwA9AResnUkQLuA+sPXt4Z9M51wZ2ms3mJ/fu3SvcvHmz48zMzc/PT29sbLQTBYk//e7WnMvlNs99c52gehEv9sCk2CINhu2IPWO/+Sfbvz3A+v2Lf/1692nS3OHly5dL6+vrf/uAF0VRLnHqoUsvnvv6e8JgGp41odfT1lGkZuvo+T5hZRr76lu2f/9lEagn2W1HUXQa8HzAvXjxop9sFIfcCKqfDUId9bSYIj0Y0O3BdouPZmtsw49JsAF6SZadD3iHh4dRMv0YUnGxw3qRFlKkk16Eix1j+79+kmXPH55gfKRnMcTyD5Hi5rb/TaHjYUH7A2s5xqANbRhF+sN9/Fnnv/k9hnaMYhKTPWzst1S9gi0mMtdvC7Yp2CLlwT5BY9u7PhJCpBj/rT2vxhZybCEmwLFNji2y6NgDFdHXsEXGVMSkIiKLKqJxn8ikigwcWyoiMufYUhExuWgqIj6wxpaKiEw6tlREZFFFBtFWY4tUV7YcW0hFBsQGsRpbpJhY4z4hx5aKiCyriMZ9IovB1h00IpMqosYWGW1sObZIfbJP0tgo2GJSc62vrYoPTEV0l7rIpIro1jCRSRXRVERkUkV05VFk1LGlImJy0V3q4gNTkXfZuRBqbCHS4tga94nUB/sEja1HdYhJTvabG9u5d9a9EO8N544y+oZgWxiGbm9vb/T8juWLFecXoNvWIor0kS9i+RLAzujpMAwdYB7A1atXcwyeRT3kfme/hU3PYoUi5gZqokPHez8cg0xWL9De/Qfgp5HcekmW8YH4/PnzADmOfkhk7fDRz7f48jqF+S9wUVd3rIt04HlYvkD31R6vHz0AWBv5ay7JcuzMLPf48ePTV65ciYDXA602Zq/dvgQsAjeAslZUpIgWcB9Ye/bwzqY7cu2pzc1Nf25u7hAzc2ZWqtfr5TEdEWKiurxer5fNrGRmDjPDzHwzC5aWls4weAS1EBM1I1laWjpjZkGSZYbBdmZWMLOw0WiEam4xSU3daDRCMwuTDDszw9nRnNoBeaD4/PnzwtbWVn9/f7/75MmT7urqak/rJ9LCysqKv7CwUAiCoFCr1XIzMzNdoANEJFdtRoM9DHcumZYMX570RKSM4U8B90ZefUYuRY4HezTg3lioFW6RllCPhjvmmGvr/w4AJ8NnvMG503MAAAAASUVORK5CYII'
    grey1 = b'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/GMzLdsAADsHaVRYdFhNTDpjb20uYWRvYmUueG1wAAAAAAA8P3hwYWNrZXQgYmVnaW49Iu+7vyIgaWQ9Ilc1TTBNcENlaGlIenJlU3pOVGN6a2M5ZCI/Pgo8eDp4bXBtZXRhIHhtbG5zOng9ImFkb2JlOm5zOm1ldGEvIiB4OnhtcHRrPSJBZG9iZSBYTVAgQ29yZSA1LjYtYzE0NSA3OS4xNjIzMTksIDIwMTgvMDIvMTUtMjA6Mjk6NDMgICAgICAgICI+CiAgIDxyZGY6UkRGIHhtbG5zOnJkZj0iaHR0cDovL3d3dy53My5vcmcvMTk5OS8wMi8yMi1yZGYtc3ludGF4LW5zIyI+CiAgICAgIDxyZGY6RGVzY3JpcHRpb24gcmRmOmFib3V0PSIiCiAgICAgICAgICAgIHhtbG5zOnhtcD0iaHR0cDovL25zLmFkb2JlLmNvbS94YXAvMS4wLyIKICAgICAgICAgICAgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iCiAgICAgICAgICAgIHhtbG5zOnN0RXZ0PSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VFdmVudCMiCiAgICAgICAgICAgIHhtbG5zOnBob3Rvc2hvcD0iaHR0cDovL25zLmFkb2JlLmNvbS9waG90b3Nob3AvMS4wLyIKICAgICAgICAgICAgeG1sbnM6ZGM9Imh0dHA6Ly9wdXJsLm9yZy9kYy9lbGVtZW50cy8xLjEvIgogICAgICAgICAgICB4bWxuczp0aWZmPSJodHRwOi8vbnMuYWRvYmUuY29tL3RpZmYvMS4wLyIKICAgICAgICAgICAgeG1sbnM6ZXhpZj0iaHR0cDovL25zLmFkb2JlLmNvbS9leGlmLzEuMC8iPgogICAgICAgICA8eG1wOkNyZWF0b3JUb29sPkFkb2JlIFBob3Rvc2hvcCBFbGVtZW50cyAxNy4wIChXaW5kb3dzKTwveG1wOkNyZWF0b3JUb29sPgogICAgICAgICA8eG1wOkNyZWF0ZURhdGU+MjAyMC0xMC0wM1QxMToyOTozMi0wNDowMDwveG1wOkNyZWF0ZURhdGU+CiAgICAgICAgIDx4bXA6TWV0YWRhdGFEYXRlPjIwMjAtMTAtMDNUMTE6Mjk6MzItMDQ6MDA8L3htcDpNZXRhZGF0YURhdGU+CiAgICAgICAgIDx4bXA6TW9kaWZ5RGF0ZT4yMDIwLTEwLTAzVDExOjI5OjMyLTA0OjAwPC94bXA6TW9kaWZ5RGF0ZT4KICAgICAgICAgPHhtcE1NOkluc3RhbmNlSUQ+eG1wLmlpZDo2Y2Q5MjZlZS0xYWE3LTBlNDEtYTI2ZS04MmMwMGYyN2E2Nzg8L3htcE1NOkluc3RhbmNlSUQ+CiAgICAgICAgIDx4bXBNTTpEb2N1bWVudElEPmFkb2JlOmRvY2lkOnBob3Rvc2hvcDozMzlhMjcxYS0wNThkLTExZWItOTQ3ZC04N2E5Njc3OWZkYzU8L3htcE1NOkRvY3VtZW50SUQ+CiAgICAgICAgIDx4bXBNTTpPcmlnaW5hbERvY3VtZW50SUQ+eG1wLmRpZDpjZDY3N2JmMi02YjVjLWU4NDgtYTI0OC1kOGRkNGNkZTBkMzM8L3htcE1NOk9yaWdpbmFsRG9jdW1lbnRJRD4KICAgICAgICAgPHhtcE1NOkhpc3Rvcnk+CiAgICAgICAgICAgIDxyZGY6U2VxPgogICAgICAgICAgICAgICA8cmRmOmxpIHJkZjpwYXJzZVR5cGU9IlJlc291cmNlIj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OmFjdGlvbj5jcmVhdGVkPC9zdEV2dDphY3Rpb24+CiAgICAgICAgICAgICAgICAgIDxzdEV2dDppbnN0YW5jZUlEPnhtcC5paWQ6Y2Q2NzdiZjItNmI1Yy1lODQ4LWEyNDgtZDhkZDRjZGUwZDMzPC9zdEV2dDppbnN0YW5jZUlEPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6d2hlbj4yMDIwLTEwLTAzVDExOjI5OjMyLTA0OjAwPC9zdEV2dDp3aGVuPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6c29mdHdhcmVBZ2VudD5BZG9iZSBQaG90b3Nob3AgRWxlbWVudHMgMTcuMCAoV2luZG93cyk8L3N0RXZ0OnNvZnR3YXJlQWdlbnQ+CiAgICAgICAgICAgICAgIDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpIHJkZjpwYXJzZVR5cGU9IlJlc291cmNlIj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OmFjdGlvbj5zYXZlZDwvc3RFdnQ6YWN0aW9uPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6aW5zdGFuY2VJRD54bXAuaWlkOjZjZDkyNmVlLTFhYTctMGU0MS1hMjZlLTgyYzAwZjI3YTY3ODwvc3RFdnQ6aW5zdGFuY2VJRD4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OndoZW4+MjAyMC0xMC0wM1QxMToyOTozMi0wNDowMDwvc3RFdnQ6d2hlbj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OnNvZnR3YXJlQWdlbnQ+QWRvYmUgUGhvdG9zaG9wIEVsZW1lbnRzIDE3LjAgKFdpbmRvd3MpPC9zdEV2dDpzb2Z0d2FyZUFnZW50PgogICAgICAgICAgICAgICAgICA8c3RFdnQ6Y2hhbmdlZD4vPC9zdEV2dDpjaGFuZ2VkPgogICAgICAgICAgICAgICA8L3JkZjpsaT4KICAgICAgICAgICAgPC9yZGY6U2VxPgogICAgICAgICA8L3htcE1NOkhpc3Rvcnk+CiAgICAgICAgIDxwaG90b3Nob3A6RG9jdW1lbnRBbmNlc3RvcnM+CiAgICAgICAgICAgIDxyZGY6QmFnPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MDE4MDExNzQwNzIwNjgxMTg3MUZEQjdDNzNFQzdBRjQ8L3JkZjpsaT4KICAgICAgICAgICAgPC9yZGY6QmFnPgogICAgICAgICA8L3Bob3Rvc2hvcDpEb2N1bWVudEFuY2VzdG9ycz4KICAgICAgICAgPHBob3Rvc2hvcDpDb2xvck1vZGU+MzwvcGhvdG9zaG9wOkNvbG9yTW9kZT4KICAgICAgICAgPHBob3Rvc2hvcDpJQ0NQcm9maWxlPnNSR0IgSUVDNjE5NjYtMi4xPC9waG90b3Nob3A6SUNDUHJvZmlsZT4KICAgICAgICAgPGRjOmZvcm1hdD5pbWFnZS9wbmc8L2RjOmZvcm1hdD4KICAgICAgICAgPHRpZmY6T3JpZW50YXRpb24+MTwvdGlmZjpPcmllbnRhdGlvbj4KICAgICAgICAgPHRpZmY6WFJlc29sdXRpb24+NzIwMDAwLzEwMDAwPC90aWZmOlhSZXNvbHV0aW9uPgogICAgICAgICA8dGlmZjpZUmVzb2x1dGlvbj43MjAwMDAvMTAwMDA8L3RpZmY6WVJlc29sdXRpb24+CiAgICAgICAgIDx0aWZmOlJlc29sdXRpb25Vbml0PjI8L3RpZmY6UmVzb2x1dGlvblVuaXQ+CiAgICAgICAgIDxleGlmOkNvbG9yU3BhY2U+MTwvZXhpZjpDb2xvclNwYWNlPgogICAgICAgICA8ZXhpZjpQaXhlbFhEaW1lbnNpb24+ODI8L2V4aWY6UGl4ZWxYRGltZW5zaW9uPgogICAgICAgICA8ZXhpZjpQaXhlbFlEaW1lbnNpb24+MzI8L2V4aWY6UGl4ZWxZRGltZW5zaW9uPgogICAgICA8L3JkZjpEZXNjcmlwdGlvbj4KICAgPC9yZGY6UkRGPgo8L3g6eG1wbWV0YT4KICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAKPD94cGFja2V0IGVuZD0idyI/PkjKk0kAAAAgY0hSTQAAeiUAAICDAAD5/wAAgOkAAHUwAADqYAAAOpgAABdvkl/FRgAAAgtJREFUeNrsmi1vFFEUhp97Z6b7PYtuNyQYQkIwBNNgoAKBA4FAIJAIdP/BViOQCAQCgUQSBB+GYDZtCGnShKAIgp3dndl05p6L2G2ABrrscgXivGbEuCfvc8+ZyTXM8/Dx03ONZmvHWrtljOmg+WO89yMReVHkk+17d259ADAADx49OZ92T70+c7rXTdMuePB4JfabGAwYyLIhB58+D7Pht8v3797ejYGo0Wz1exvr3XitznCUI14hnhRrDMland7GevegLPvAjRhIrI2uJrUaxWGplP4i4j3VoZDUalgbbQFJDERAuyoFr01cKlUpAC0gigEr3uMU4tJx3h8dgzYGEBFERMmsovmcWzwf52ghV16FfoAU8YjXRq7WyF9ACl60kv+stnjR3XHlVUiOq60gg6gtqnYAtXX9CQVS1Va1/7ep7VTtQFNbGxlGbadfNqGmtjYyzNTWMzKA2l5w2sgQw0YX8mBqayMDqO29npGr5tiPXVU7iNpOZFxVrm2sVTLLtFEEJ5IfgZRpkb/Jx9m1RjtVOkukGGdMi/wVIDHg9vcG/bWktpk612m0UrSZi5tYTDKyr19G+3uDPuAMswsCnSvXb146e+Hidr3e2DTWNhXXiSDz6bR4+3Hwfufl82fvgJFhdpEqZnZjoA3UAa3kghkDTIExMAEq89PLCEjmTwW5GKQDyvmT7wMAzpNJbp+doKQAAAAASUVORK5CYII='
    grey2 = b'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/GMzLdsAADsHaVRYdFhNTDpjb20uYWRvYmUueG1wAAAAAAA8P3hwYWNrZXQgYmVnaW49Iu+7vyIgaWQ9Ilc1TTBNcENlaGlIenJlU3pOVGN6a2M5ZCI/Pgo8eDp4bXBtZXRhIHhtbG5zOng9ImFkb2JlOm5zOm1ldGEvIiB4OnhtcHRrPSJBZG9iZSBYTVAgQ29yZSA1LjYtYzE0NSA3OS4xNjIzMTksIDIwMTgvMDIvMTUtMjA6Mjk6NDMgICAgICAgICI+CiAgIDxyZGY6UkRGIHhtbG5zOnJkZj0iaHR0cDovL3d3dy53My5vcmcvMTk5OS8wMi8yMi1yZGYtc3ludGF4LW5zIyI+CiAgICAgIDxyZGY6RGVzY3JpcHRpb24gcmRmOmFib3V0PSIiCiAgICAgICAgICAgIHhtbG5zOnhtcD0iaHR0cDovL25zLmFkb2JlLmNvbS94YXAvMS4wLyIKICAgICAgICAgICAgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iCiAgICAgICAgICAgIHhtbG5zOnN0RXZ0PSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VFdmVudCMiCiAgICAgICAgICAgIHhtbG5zOnBob3Rvc2hvcD0iaHR0cDovL25zLmFkb2JlLmNvbS9waG90b3Nob3AvMS4wLyIKICAgICAgICAgICAgeG1sbnM6ZGM9Imh0dHA6Ly9wdXJsLm9yZy9kYy9lbGVtZW50cy8xLjEvIgogICAgICAgICAgICB4bWxuczp0aWZmPSJodHRwOi8vbnMuYWRvYmUuY29tL3RpZmYvMS4wLyIKICAgICAgICAgICAgeG1sbnM6ZXhpZj0iaHR0cDovL25zLmFkb2JlLmNvbS9leGlmLzEuMC8iPgogICAgICAgICA8eG1wOkNyZWF0b3JUb29sPkFkb2JlIFBob3Rvc2hvcCBFbGVtZW50cyAxNy4wIChXaW5kb3dzKTwveG1wOkNyZWF0b3JUb29sPgogICAgICAgICA8eG1wOkNyZWF0ZURhdGU+MjAyMC0xMC0wM1QxMTozMDoyMi0wNDowMDwveG1wOkNyZWF0ZURhdGU+CiAgICAgICAgIDx4bXA6TWV0YWRhdGFEYXRlPjIwMjAtMTAtMDNUMTE6MzA6MjItMDQ6MDA8L3htcDpNZXRhZGF0YURhdGU+CiAgICAgICAgIDx4bXA6TW9kaWZ5RGF0ZT4yMDIwLTEwLTAzVDExOjMwOjIyLTA0OjAwPC94bXA6TW9kaWZ5RGF0ZT4KICAgICAgICAgPHhtcE1NOkluc3RhbmNlSUQ+eG1wLmlpZDpiZTM1YzVhMi02NDQ3LWY2NGEtYTJkNi04NTM1NGJkYjQ1MWE8L3htcE1NOkluc3RhbmNlSUQ+CiAgICAgICAgIDx4bXBNTTpEb2N1bWVudElEPmFkb2JlOmRvY2lkOnBob3Rvc2hvcDo1MjZjNTY3NS0wNThkLTExZWItOTQ3ZC04N2E5Njc3OWZkYzU8L3htcE1NOkRvY3VtZW50SUQ+CiAgICAgICAgIDx4bXBNTTpPcmlnaW5hbERvY3VtZW50SUQ+eG1wLmRpZDo1NWI1YjZiOC0zZmM1LTU1NGMtYTNjMi01NDI2NjlkYWRlZGY8L3htcE1NOk9yaWdpbmFsRG9jdW1lbnRJRD4KICAgICAgICAgPHhtcE1NOkhpc3Rvcnk+CiAgICAgICAgICAgIDxyZGY6U2VxPgogICAgICAgICAgICAgICA8cmRmOmxpIHJkZjpwYXJzZVR5cGU9IlJlc291cmNlIj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OmFjdGlvbj5jcmVhdGVkPC9zdEV2dDphY3Rpb24+CiAgICAgICAgICAgICAgICAgIDxzdEV2dDppbnN0YW5jZUlEPnhtcC5paWQ6NTViNWI2YjgtM2ZjNS01NTRjLWEzYzItNTQyNjY5ZGFkZWRmPC9zdEV2dDppbnN0YW5jZUlEPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6d2hlbj4yMDIwLTEwLTAzVDExOjMwOjIyLTA0OjAwPC9zdEV2dDp3aGVuPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6c29mdHdhcmVBZ2VudD5BZG9iZSBQaG90b3Nob3AgRWxlbWVudHMgMTcuMCAoV2luZG93cyk8L3N0RXZ0OnNvZnR3YXJlQWdlbnQ+CiAgICAgICAgICAgICAgIDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpIHJkZjpwYXJzZVR5cGU9IlJlc291cmNlIj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OmFjdGlvbj5zYXZlZDwvc3RFdnQ6YWN0aW9uPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6aW5zdGFuY2VJRD54bXAuaWlkOmJlMzVjNWEyLTY0NDctZjY0YS1hMmQ2LTg1MzU0YmRiNDUxYTwvc3RFdnQ6aW5zdGFuY2VJRD4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OndoZW4+MjAyMC0xMC0wM1QxMTozMDoyMi0wNDowMDwvc3RFdnQ6d2hlbj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OnNvZnR3YXJlQWdlbnQ+QWRvYmUgUGhvdG9zaG9wIEVsZW1lbnRzIDE3LjAgKFdpbmRvd3MpPC9zdEV2dDpzb2Z0d2FyZUFnZW50PgogICAgICAgICAgICAgICAgICA8c3RFdnQ6Y2hhbmdlZD4vPC9zdEV2dDpjaGFuZ2VkPgogICAgICAgICAgICAgICA8L3JkZjpsaT4KICAgICAgICAgICAgPC9yZGY6U2VxPgogICAgICAgICA8L3htcE1NOkhpc3Rvcnk+CiAgICAgICAgIDxwaG90b3Nob3A6RG9jdW1lbnRBbmNlc3RvcnM+CiAgICAgICAgICAgIDxyZGY6QmFnPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MDE4MDExNzQwNzIwNjgxMTg3MUZEQjdDNzNFQzdBRjQ8L3JkZjpsaT4KICAgICAgICAgICAgPC9yZGY6QmFnPgogICAgICAgICA8L3Bob3Rvc2hvcDpEb2N1bWVudEFuY2VzdG9ycz4KICAgICAgICAgPHBob3Rvc2hvcDpDb2xvck1vZGU+MzwvcGhvdG9zaG9wOkNvbG9yTW9kZT4KICAgICAgICAgPHBob3Rvc2hvcDpJQ0NQcm9maWxlPnNSR0IgSUVDNjE5NjYtMi4xPC9waG90b3Nob3A6SUNDUHJvZmlsZT4KICAgICAgICAgPGRjOmZvcm1hdD5pbWFnZS9wbmc8L2RjOmZvcm1hdD4KICAgICAgICAgPHRpZmY6T3JpZW50YXRpb24+MTwvdGlmZjpPcmllbnRhdGlvbj4KICAgICAgICAgPHRpZmY6WFJlc29sdXRpb24+NzIwMDAwLzEwMDAwPC90aWZmOlhSZXNvbHV0aW9uPgogICAgICAgICA8dGlmZjpZUmVzb2x1dGlvbj43MjAwMDAvMTAwMDA8L3RpZmY6WVJlc29sdXRpb24+CiAgICAgICAgIDx0aWZmOlJlc29sdXRpb25Vbml0PjI8L3RpZmY6UmVzb2x1dGlvblVuaXQ+CiAgICAgICAgIDxleGlmOkNvbG9yU3BhY2U+MTwvZXhpZjpDb2xvclNwYWNlPgogICAgICAgICA8ZXhpZjpQaXhlbFhEaW1lbnNpb24+ODI8L2V4aWY6UGl4ZWxYRGltZW5zaW9uPgogICAgICAgICA8ZXhpZjpQaXhlbFlEaW1lbnNpb24+MzI8L2V4aWY6UGl4ZWxZRGltZW5zaW9uPgogICAgICA8L3JkZjpEZXNjcmlwdGlvbj4KICAgPC9yZGY6UkRGPgo8L3g6eG1wbWV0YT4KICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAKPD94cGFja2V0IGVuZD0idyI/PvOojhoAAAAgY0hSTQAAeiUAAICDAAD5/wAAgOkAAHUwAADqYAAAOpgAABdvkl/FRgAAAdNJREFUeNrsmj9rFEEYh5+ZufV2725zCRG/QI4QYmFjI4JgilQ24sfQwka4QhBByIGNhR/FysJC/NPYCHpuzkLBkD+QEOKd2dtk9x2LvYAgF70DQcj7FLvFdg/v/H4z7BhGPHj8dCmMoo4xdsUYE6OMxXvf915eDNO0/fDenQTAANzvPLnYiGdeLy8tNufmz+OB8qH8himl7e/t0k16B4P+96uP2nc/VgAXRrW1VmuhWYlidg8GiKjF07DWUI1iWq2FZpKsrwE3K0BgrbtercWk2ZFa+gtEPGl2RFifwVq3AgQWcAYauYgampDjosBAHXAWsB6vy3nKyfRlmVg7aiG1Mn2Dl7mpIlXk/ydSVOT0OakT+S+Wtjb29CJFJ1LLRkVqayva2rq0dftzdrc/mpGakZqRKlJRkSpSRZ61I6LIoMhztTIhRVEgIocnIiXL0jc721tqZkJ2tjbJsvQVIA5ws/MXvmLdrSA4V6036tjy56IydhJzNjc2WE8+9T93P9z+0ut+M4AD4murNy4vLl9qV6PwijG2prpOy0U5zNLh2173fefl82fvgL6hvBNUobwx0ADCk+xUxp8MgSEwAH4AufnlowOC0VtF/llkARyP3vwcAJizBUPv/gPvAAAAAElFTkSuQmCC'

    #todo - Try to get reload EDS to work
    execute = True
    while execute:
        app = App('basecamp.ini')
        execute = app.execute()
        break
                
    logger.info('Exiting app')


