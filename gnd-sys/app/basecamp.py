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
      1. Assumes teh exct same app name is used for
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
import importlib
import ctypes
import time
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

import logging
from logging.config import fileConfig
fileConfig('logging.ini')
logger = logging.getLogger(__name__)

import PySimpleGUI as sg

import EdsLib
import CFE_MissionLib
from cfsinterface import CmdTlmRouter
from cfsinterface import Cfe, EdsMission
from cfsinterface import TelecommandInterface, TelecommandScript
from cfsinterface import TelemetryMessage, TelemetryObserver, TelemetryQueueServer
from tools import CreateApp, ManageTutorials, crc_32c, datagram_to_str, compress_abs_path, TextEditor
from tools import AppStore, ManageUsrApps, AppSpec, TargetControl, CfeTopicIds, JsonTblTopicMap

# Shell script names should not change and are considered part of the application
# Therefore they can be defined here and not in a configuration file

DEFAULT_TARGET_NAME = 'cpu1'

SH_BUILD_CFS_TOPICIDS = './build_cfs_topicids.sh'
SH_BUILD_CFS = './build_cfs.sh'
SH_STOP_CFS  = './stop_cfs.sh'
SH_START_CFS = './start_cfs.sh'

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
        self.NULL_STR = self.eds_mission.NULL_STR
       
        self.UNDEFINED_LIST = [self.NULL_STR]

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
            'eds_entry': EdsLib.DatabaseEntry('samplemission','BASE_TYPES/PathName'),
            'eds_name': 'Payload.DirName', 'gui_type': 'BASE_TYPES/PathName',
            'gui_value': ['--null--'],
            'gui_input': 'text',
            'gui_value_key': '--null--'
         },
         'DirListOffset':
         {
            'eds_entry': EdsLib.DatabaseEntry('samplemission','BASE_TYPES/uint16'),
            'eds_name': 'Payload.DirListOffset',
            'gui_type': 'BASE_TYPES/uint16',
            'gui_value': ['--null--'],
            'gui_input': 'text',
            'gui_value_key': '--null--'
         },
         'IncludeSizeTime':
         {
            'eds_entry': EdsLib.DatabaseEntry('samplemission','FILE_MGR/BooleanUint16'),
            'eds_name': 'Payload.IncludeSizeTime',
            'gui_type': 'FILE_MGR/BooleanUint16',
            'gui_value': ['FALSE', 'TRUE'],
            'gui_input': 'combo',
            'gui_value_key': '--null--'}}
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
                gui_value = [self.NULL_STR]
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
                                                  'gui_input': gui_input, 'gui_value_key': self.NULL_STR}
            
        else:
            return_str = f'Error extracting entries from unkown payload structure instance type: {str(payload_struct)}'
        
        logger.debug(f'return_str: {return_str}')
        return return_str


    def display_payload_gui_entries(self):
        """
        See SendCmd() payload_layout definition comment for initial payload display
        When there are no payload paramaters (zero length) hide all rows except the first parameter.
        """
        for row in range(self.PAYLOAD_ROWS):
            self.window[f'-PAYLOAD_{row}_NAME-'].update(visible=False)
            self.window[f'-PAYLOAD_{row}_TYPE-'].update(visible=False)
            self.window[f'-PAYLOAD_{row}_VALUE-'].update(visible=False, value=self.UNDEFINED_LIST[0])

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
        Virtual function used by based Telesommand class set_payload_values() to retrieve values
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
        
    def execute(self, topic_name):
    
        cmd_sent = True
        cmd_text = 'Send command aborted'
        cmd_status = ''

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
                self.payload_layout += [[sg.pin(sg.Text('Name', font=row_font, size=row_label_size, key="-PAYLOAD_%d_NAME-"%row))] + [sg.pin(sg.Text('Type', font=row_font, size=row_label_size, key="-PAYLOAD_%d_TYPE-"%row))] + [sg.pin(sg.Combo((self.UNDEFINED_LIST), font=row_font, size=row_input_size, enable_events=True, key="-PAYLOAD_%d_VALUE-"%row, default_value=self.UNDEFINED_LIST[0]))]]
            else:
                self.payload_layout += [[sg.pin(sg.Text('Name', font=row_font, size=row_label_size, key="-PAYLOAD_%d_NAME-"%row))] + [sg.pin(sg.Text('Type', font=row_font, size=row_label_size, key="-PAYLOAD_%d_TYPE-"%row))] + [sg.pin(sg.Input(self.UNDEFINED_LIST[0], font=row_font, size=row_input_size, enable_events=True, key="-PAYLOAD_%d_VALUE-"%row))]]

            
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
                    cmd_sent = False
                    break
                    
                if (cmd_name == self.eds_mission.COMMAND_TITLE_KEY and len(self.command_list) > 1):
                    cmd_text  = 'Please select a command before sending a command'
                    cmd_sent = False
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

                            #payload = EdsLib.DatabaseEntry('samplemission','FILE_MGR/SendDirListTlm_Payload')({'DirName': '', 'DirListOffset': 0, 'IncludeSizeTime': 'FALSE'})
                            #todo: Check if None? payload_struct = self.get_payload_struct(payload_entry, payload, 'Payload')
                            eds_payload = self.set_payload_values(self.payload_struct)
                            payload = payload_entry(eds_payload)                   
                            cmd_obj['Payload'] = payload
    
                        except:
                           send_command = False
                           cmd_status = f'{topic_name} {cmd_name} command not sent. Error loading parameters from command window.'
                    
                    if send_command:
                        (cmd_sent, cmd_text, cmd_status) = self.send_command(cmd_obj)
                        if cmd_sent:
                            cmd_status = f'{topic_name} {cmd_name} command sent'
                    
                else:
                    popup_text = f'Error retrieving command {cmd_name} using topic ID {topic_id}' 
                    sg.popup(popup_text, title='Send Command Error', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)            


                # Keep GUI active if a command error occurs to allow user to fixed and resend or cancel
                if cmd_sent:
                    break
                    
        self.window.close()

        return (cmd_sent, cmd_text, cmd_status)


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
        
        self.sys_apps = ['CFE_ES', 'CFE_EVS', 'CFE_SB', 'CFE_TBL', 'CFE_TIME', 'OSK_C_DEMO' 'FILE_MGR' 'FILE_XFER']
        
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
    Manage the display for building and running the cFS.
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
              
    
    def add_usr_app_gui(self):
        """
        Provide steps for the user to integrate an app. This is only invoked
        after an app has been selected.
        The steps have some degree of independence in case the user doesn't do
        things in order which means some processing may be repeated. For example
        the table files are recomputed for the edit targets.cmake step and the
        copy files to cFS '_defs' steps. 
        """
        #TODO - Use a loop to construct the layout
        layout = [
                  [sg.Text("Perform the following steps to add an app. Use 'Auto' (automatically) or 'Man' (manually) buttons for step 2.\n", font=self.t_font)],
                  [sg.Text('1. Stop the cFS prior to modifying or adding an app', font=self.step_font, pad=self.b_pad)],
                  [sg.Text('', size=self.b_size), sg.Text('Close this window and click <Stop cFS>, open a terminal window & kill the cFS process, or', font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Text('Submit [sudo] password', font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Button('Submit', size=(6,1), button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-1_AUTO-'),
                   sg.InputText(password_char='*', size=(15,1), font=self.t_font, pad=self.b_pad, key='-PASSWORD-')],
                  
                  [sg.Text('2. Update cFS build configuration', font=self.step_font, pad=self.b_pad)],
                  [sg.Text('', size=self.b_size), sg.Button('Auto', size=self.b_size, button_color=('SpringGreen4'), font=self.b_font, pad=self.b_pad, enable_events=True, key='-2_AUTO-'),
                   sg.Text('Automatically perform all steps', font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Button('Man',  size=self.b_size, button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-2A_MAN-'),
                   sg.Text('Copy table files to %s' % CFS_DEFS_FOLDER, font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Button('Man',  size=self.b_size, button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-2B_MAN-'),
                   sg.Text("Update targets.cmake's %s and %s" % (self.cmake_app_list, self.cmake_file_list), font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Button('Man',  size=self.b_size, button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-2C_MAN-'),
                   sg.Text('Update cpu1_cfe_es_startup.scr', font=self.t_font)],
                  [sg.Text('', size=self.b_size), sg.Button('Man',  size=self.b_size, button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-2D_MAN-'),
                   sg.Text('Update EDS cfe-topicids.xml', font=self.t_font)], 
                  [sg.Text('', size=self.b_size), sg.Button('Man',  size=self.b_size, button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-2E_MAN-'),
                   sg.Text('Update telemetry output app table', font=self.t_font)],
                  
                  [sg.Text('3. Build the cfS', font=self.step_font, pad=self.b_pad)],
                  [sg.Text('', size=self.b_size), sg.Button('Build', size=self.b_size, button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-3_AUTO-')],
                  
                  [sg.Text('4. Exit and restart Basecamp', font=self.step_font, pad=self.b_pad)],
                  [sg.Text('', size=self.b_size), sg.Button('Restart', size=(6,1), button_color=self.b_color, font=self.b_font, pad=self.b_pad, enable_events=True, key='-4_AUTO-')],
                 ]
        # sg.Button('Exit', enable_events=True, key='-EXIT-')
        window = sg.Window(f'Add {self.usr_app_spec.app_name.upper()}', layout, resizable=True, finalize=True) # modal=True)
        
        restart_main_window = False
        while True:
        
            self.event, self.values = window.read(timeout=200)
        
            if self.event in (sg.WIN_CLOSED, 'Exit', '-EXIT-') or self.event is None:
                break
            
            ## Step 1 - Stop the cFS prior to modifying or adding an app
            
            elif self.event == '-1_AUTO-': # Stop the cFS prior to modifying or adding an app
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
                            
            elif self.event == '-1_MAN-': # Stop the cFS prior to modifying or adding an app
                popup_text = f"Open a terminal window and kill any running cFS processes. See '{SH_STOP_CFS}' for guidance" 
                sg.popup(popup_text, title='Stop the cFS', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)

            ## Step 2 - Update cFS build configuration
            
            elif self.event == '-2_AUTO-': # Autonomously perform step 2 
                """
                Errors are reported in a popup by each function. The success string is an aggregate of each successful return
                that will be reported in a single popup. 
                A boolean return value of True from each function indicates there weren't any errors, it doesn't mean a paricular
                update was performed, becuase the update may notbe required.
                """ 
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
                            if self.usr_app_spec.has_eds():
                                update_topics_passed, update_topics_text = self.update_topic_ids()
                                auto_popup_text += f'4. {update_topics_text}\n\n'
                                display_auto_popup = update_topics_passed
                            else:
                                auto_popup_text += f'4. Library without an EDS spec\n\n'
                                display_auto_popup = True
                if display_auto_popup:
                    sg.popup(auto_popup_text, title=f'Update {self.startup_scr_filename}', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
                                
                
            elif self.event == '-2A_MAN-':
                self.copy_app_tables(auto_copy=False)  # Copy table files from app dir to cFS '_defs' file
            elif self.event == '-2B_MAN-':
                self.update_targets_cmake(auto_update=False)
            elif self.event == '-2C_MAN-':
                self.update_startup_scr(auto_update=False)
            elif self.event == '-2D_MAN-':
                popup_text = f"After this dialogue, {self.cfe_topic_id_filename} will open in an editor.\n Replace spare topic IDs with the app's topic ID names"
                sg.popup(popup_text, title=f'Update {self.cfe_topic_id_filename}', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
                self.text_editor = sg.execute_py_file('texteditor.py', parms=self.cfe_topic_id_file, cwd=self.basecamp_tools_path)
            elif self.event == '-2E_MAN-':
                popup_text = f"After this dialogue, {self.kit_to_tbl_filename} will open in an editor.\n Replace spare topic IDs with the app's topic ID names"
                sg.popup(popup_text, title=f'Update {self.kit_to_tbl_filename}', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
                self.text_editor = sg.execute_py_file('texteditor.py', parms=self.kit_to_tbl_file, cwd=self.basecamp_tools_path)
                
            ## Step 3 - Build the cFS

            elif self.event == '-3_AUTO-': # Build the cfS
                build_cfs_sh = os.path.join(self.basecamp_abs_path, SH_BUILD_CFS_TOPICIDS)
                self.build_subprocess = subprocess.Popen(f'{build_cfs_sh} {self.cfs_abs_base_path}',
                                        stdout=subprocess.PIPE, shell=True, bufsize=1, universal_newlines=True)
                if self.build_subprocess is not None:
                    self.cfs_stdout = CfsStdout(self.build_subprocess, self.main_window)
                    self.cfs_stdout.start()
            
            elif self.event == '-3_MAN-': # Build the cfS
                popup_text = f"Open a terminal window, change directory to {self.cfs_abs_base_path} and build the cFS. See '{SH_BUILD_CFS_TOPICIDS}' for guidance"
                sg.popup(popup_text, title='Manually Stop the cFS', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)   

            ## Step 4 - Restart Basecamp

            elif self.event == '-4_AUTO-': # Reload cFS python EDS definitions                
                sg.popup(f'Basecamp will be closed after this dialogue.\nYou must restart Basecamp to use {self.usr_app_spec.app_name.upper()}',
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
                if self.usr_app_spec.has_eds():
                    self.restore_topic_ids()                
                if self.values['-DELETE_FILES-'] == 'Yes':
                    self.remove_app_src_files()
                sg.popup(f'Successfully removed {self.selected_app.upper()}', title='Remove App', keep_on_top=True, non_blocking=False, grab_anywhere=True, modal=True)
                break

        window.close()
        
    def execute(self, action):
        self.manage_usr_apps = ManageUsrApps(self.usr_app_path)
        self.cfs_app_specs = self.manage_usr_apps.get_app_specs()
        if len(self.cfs_app_specs) > 0:
            self.select_usr_app_gui(list(self.cfs_app_specs.keys()), action.lower())
            if self.selected_app is not None:
                self.usr_app_spec = self.manage_usr_apps.get_app_spec(self.selected_app)
                if action == 'Add':
                    self.add_usr_app_gui()
                elif action == 'Remove':
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
            popup_text = 'Library without tables to copy'
        else:
            app_table_path = os.path.join(self.usr_app_path, self.selected_app, 'fsw', 'tables')
            if auto_copy:
                target_equals_default = (DEFAULT_TARGET_NAME == self.cfs_target)
                try:
                    src=''   # Init for exception
                    dst=''
                    for table in os.listdir(app_table_path):
                        src_table = table.replace(DEFAULT_TARGET_NAME+'_','')
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
                if not line.strip().startswith('#'):  # Non-commented line
                    if not app_cmake_files['obj-file'] in line:
                        i = line.find(')')
                        line = line[:i] + ' ' + app_cmake_files['obj-file'] + line[i:]     
                        line_modified = True
                        print('app_list_new: ' + line)
            elif self.cmake_file_list in line:
                if not line.strip().startswith('#'):  # Non-commented line
                    for table in app_cmake_files['tables']:
                        if not table in line:
                            i = line.find(')')
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
                        if self.selected_app in line:
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
                popup_text = f'Added {self.selected_app} startup script entry'
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
            popup_text = f'Updated topid IDs in {self.cfe_topic_id_file} and {self.kit_to_tbl_file}'
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

class App():

    GUI_NO_IMAGE_TXT = '--None Selected--'
    GUI_NULL_TXT     = 'Null'

    def __init__(self, ini_file):

        self.path = os.getcwd()
        self.cfs_interface_dir = os.path.join(self.path, "cfsinterface")
        self.config = configparser.ConfigParser()
        self.config.read(ini_file)

        self.APP_VERSION = self.config.get('APP','VERSION')

        self.EDS_MISSION_NAME       = self.config.get('CFS_TARGET','MISSION_EDS_NAME')
        self.EDS_CFS_TARGET_NAME    = self.config.get('CFS_TARGET','CPU_EDS_NAME')

        self.CFS_TARGET_HOST_ADDR   = self.config.get('NETWORK','CFS_HOST_ADDR')
        self.CFS_TARGET_CMD_PORT    = self.config.getint('NETWORK','CFS_SEND_CMD_PORT')
        self.CFS_TARGET_TLM_PORT    = self.config.getint('NETWORK','CFS_RECV_TLM_PORT')
        self.CFS_TARGET_TLM_TIMEOUT = float(self.config.getint('CFS_TARGET','RECV_TLM_TIMEOUT'))/1000.0
        
        self.GUI_CMD_PAYLOAD_TABLE_ROWS = self.config.getint('GUI','CMD_PAYLOAD_TABLE_ROWS')

        self.cfs_exe_rel_path   = 'build/exe/' + self.EDS_CFS_TARGET_NAME.lower()
        self.cfs_exe_file       = 'core-' + self.EDS_CFS_TARGET_NAME.lower()
        self.cfs_abs_base_path  = compress_abs_path(os.path.join(self.path, self.config.get('CFS_TARGET','BASE_PATH')))
        self.cfs_subprocess     = None
        self.cfs_subprocess_log = ""
        self.cfs_stdout         = None
        self.cfe_time_event_filter = False  #todo: Retaining the state here doesn't work if user starts and stops the cFS and doesn't restart Basecamp
        self.cfs_build_subprocess  = None

        self.event_log   = ""        
        self.event_queue = queue.Queue()
        self.window = None
        
        self.cfe_apps = ['CFE_ES', 'CFE_EVS', 'CFE_SB', 'CFE_TBL', 'CFE_TIME']
        self.app_cmd_list = []  # Non-cFE apps
        self.app_tlm_list = []  # Non-cFE apps

        self.manage_tutorials = ManageTutorials(self.config.get('PATHS', 'TUTORIALS_PATH'))
        self.create_app       = CreateApp(self.config.get('PATHS', 'APP_TEMPLATES_PATH'),
                                          self.config.get('PATHS', 'USR_APP_PATH'))
        
        self.file_browser   = None
        self.script_runner  = None
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

    def send_cfs_cmd(self, app_name, cmd_name, cmd_payload):
        (cmd_sent, cmd_text, cmd_status) = self.telecommand_script.send_cfs_cmd(app_name, cmd_name, cmd_payload)
        self.display_event(cmd_status) # cmd_status will describe success and failure cases
        
    def enable_telemetry(self):
        """
        The use must enable telemetry every time the cFS is started and most if not all users want
        the time fly wheel event disabled as well so it is also done here
        """
        self.send_cfs_cmd('KIT_TO', 'EnableOutput', {'DestIp': self.CFS_TARGET_HOST_ADDR})
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
        time.sleep(self.CFS_TARGET_TLM_TIMEOUT)
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
            if app_name not in self.cfe_apps and app_name not in self.app_cmd_list:
                self.app_cmd_list.append(app_name)

    def create_app_tlm_list(self, tlm_topics):
        """
        Populate self.app_tlm_list with the app names defined in tlm_topics. Assumes the app name 
         
        """
        for topic in tlm_topics:
            app_name = topic.split('/')[0]
            if app_name not in self.cfe_apps and app_name not in self.app_tlm_list:
                self.app_tlm_list.append(app_name)

                                     
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
            self.cmd_tlm_router.add_tlm_dest(self.config.getint('NETWORK','TLM_PLOT_TLM_PORT'))                
            self.tlm_plot = sg.execute_py_file("tlmplot.py", parms=tlm_plot_cmd_parms, cwd=self.cfs_interface_dir)

    def create_window(self, sys_target_str, sys_comm_str):
        """
        Create the main window. Non-class variables are used so it can be refreshed, PySimpleGui
        layouts can't be shared.
        """
        sg.theme('LightGreen')
        sg.set_options(element_padding=(0, 0))
    
        menu_def = [
                       ['System', ['Options', 'About', 'Exit']],
                       ['Developer', ['Create App', 'Download App', 'Add App', 'Remove App', '---', 'Run Perf Monitor']], #todo: 'Certify App' 
                       ['Operator', ['Browse Files', 'Run Script', 'Plot Data', '---', 'Control Remote Target']],
                       ['Documents', ['cFS Overview', 'cFE Overview', 'App Dev Guide']],
                       ['Tutorials', self.manage_tutorials.tutorial_titles]
                   ]

        self.common_cmds = ['-- Common Commands--', 'Enable Telemetry', 'Reset Time', 'Noop/Reset App', 'Restart App', 'Configure Events', 'Ena/Dis Flywheel', 'cFE Version']


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
                     [sg.Button('Build cFS', key='-BUILD_CFS-', image_data=image_grey1, button_color=('black', sg.theme_background_color()), border_width=0),
                      sg.Button('Start cFS', enable_events=True, key='-START_CFS-', image_data=image_grey1, button_color=('black', sg.theme_background_color()), border_width=0),
                      sg.Button('Stop cFS', enable_events=True, key='-STOP_CFS-', image_data=image_grey1, button_color=('black', sg.theme_background_color()), border_width=0),
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
                     [sg.Text('cFS Process Window', font=pri_hdr_font), sg.Text('Time: ', font=sec_hdr_font, pad=(2,1)), sg.Text(self.GUI_NULL_TXT, key='-CFS_TIME-', font=sec_hdr_font, text_color='blue'), sg.Button('Restart', enable_events=True, key='-RESTART-', visible=False)],
                     #[sg.Output(font=log_font, size=(125, 10))],
                     [sg.MLine(default_text=self.cfs_subprocess_log, font=log_font, enable_events=True, size=(125, 15), key='-CFS_PROCESS_TEXT-')],
                     [sg.Text('Ground & Flight Events', font=pri_hdr_font), sg.Button('Clear', enable_events=True, key='-CLEAR_EVENTS-', pad=(5,1))],
                     [sg.MLine(default_text=self.event_log, font=log_font, enable_events=True, size=(125, 15), key='-EVENT_TEXT-')]
                 ]

        #sg.Button('Send Cmd', enable_events=True, key='-SEND_CMD-', pad=(10,1)),
        #sg.Button('View Tlm', enable_events=True, key='-VIEW_TLM-', pad=(10,1)),
        window = sg.Window('cFS Basecamp', layout, auto_size_text=True, finalize=True)
        return window
  
    def reload_eds_libs(self):
        #importlib.invalidate_caches()
        self.telecommand_gui.eds_mission.reload_libs()
        self.telecommand_script.eds_mission.reload_libs()
        self.tlm_server.eds_mission.reload_libs()
        
    def execute(self):
    
        sys_target_str = "Basecamp version %s initialized with mission %s, target %s on %s" % (self.APP_VERSION, self.EDS_MISSION_NAME, self.EDS_CFS_TARGET_NAME, datetime.now().strftime("%m/%d/%Y"))
        sys_comm_str = "Basecamp target host %s, command port %d, telemetry port %d" % (self.CFS_TARGET_HOST_ADDR, self.CFS_TARGET_CMD_PORT, self.CFS_TARGET_TLM_PORT)
    
        logger.info(sys_target_str)
        logger.info(sys_comm_str)
        
        self.tlm_monitors = {'CFE_ES': {'HK_TLM': ['Seconds']}, 'FILE_MGR': {'DIR_LIST_TLM': ['Seconds']}}
        
        try:

             # Command & Telemetry Router
            
             self.cmd_tlm_router = CmdTlmRouter(self.CFS_TARGET_HOST_ADDR, self.CFS_TARGET_CMD_PORT, self.CFS_TARGET_HOST_ADDR, self.CFS_TARGET_TLM_PORT, self.CFS_TARGET_TLM_TIMEOUT)
             self.cfs_cmd_output_queue = self.cmd_tlm_router.get_cfs_cmd_queue()
             self.cfs_cmd_input_queue  = self.cmd_tlm_router.get_cfs_cmd_source_queue()
             
             # Command Objects    
             
             self.telecommand_gui    = TelecommandGui(self.EDS_MISSION_NAME, self.EDS_CFS_TARGET_NAME, self.cfs_cmd_output_queue)
             self.telecommand_script = TelecommandScript(self.EDS_MISSION_NAME, self.EDS_CFS_TARGET_NAME, self.cfs_cmd_output_queue)
             
             # Telemetry Objects
             
             self.tlm_server  = TelemetryQueueServer(self.EDS_MISSION_NAME, self.EDS_CFS_TARGET_NAME, self.cmd_tlm_router.get_gnd_tlm_queue())
             self.tlm_monitor = BasecampTelemetryMonitor(self.tlm_server, self.tlm_monitors, self.display_tlm_monitor, self.event_queue)
             self.tlm_server.execute()      
             self.cmd_tlm_router.start()
             
             logger.info("Successfully created application objects")
        
        except RuntimeError:
            print("Error creating telecommand/telemetry objects and/or telemetry server. See log file for details")
            logger.error("Error creating application objects")
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
                print("Sent remote process command: " + datagram_to_str(datagram))

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
                repo_exclusions = self.config.get('APP','CFS_APPS').split(',')
                print(repo_exclusions)
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
                self.cmd_tlm_router.add_cmd_source(self.config.getint('NETWORK','SCRIPT_RUNNER_CMD_PORT'))
                self.cmd_tlm_router.add_tlm_dest(self.config.getint('NETWORK','SCRIPT_RUNNER_TLM_PORT'))
                self.script_runner = sg.execute_py_file("scriptrunner.py", cwd=self.cfs_interface_dir)

            elif self.event == 'Browse Files' or self.event == '-FILE_BROWSER-':
                self.cmd_tlm_router.add_cmd_source(self.config.getint('NETWORK','FILE_BROWSER_CMD_PORT'))
                self.cmd_tlm_router.add_tlm_dest(self.config.getint('NETWORK','FILE_BROWSER_TLM_PORT'))
                self.file_browser = sg.execute_py_file("filebrowser.py", cwd=self.cfs_interface_dir)

            elif self.event == 'Plot Data':
                self.launch_tlmplot()
                
            elif self.event == 'Control Remote Target':
                tools_dir = os.path.join(self.path, "tools")
                self.target_control = sg.execute_py_file("targetcontrol.py", cwd=tools_dir)


            ### DOCUMENTS ###
            
            elif self.event == 'cFS Overview':
                path_filename = os.path.join(self.path, "../../docs/cFS-Overview.pdf")  #TODO - Ini file
                webbrowser.open_new(r'file://'+path_filename)
                #subprocess.Popen([path_filename],shell=True) # Permision Denied
                #subprocess.call(["xdg-open", path_filename]) # Not portable
            
            elif self.event == 'cFE Overview':
                path_filename = os.path.join(self.path, "../../docs/cFE-Overview.pdf")  #TODO - Ini file
                webbrowser.open_new(r'file://'+path_filename)
                
            elif self.event == 'App Dev Guide':
                path_filename = os.path.join(self.path, "../../docs/OSK-App-Dev-Guide.pdf")  #TODO - Ini file
                webbrowser.open_new(r'file://'+path_filename)
                
            ### TUTORIALS ###
                   
            elif self.event in self.manage_tutorials.tutorial_titles:
                tutorial_tool_dir = os.path.join(self.path, "tools")
                tutorial_dir = self.manage_tutorials.tutorial_lookup[self.event].path
                self.tutorial = sg.execute_py_file("tutorial.py", parms=tutorial_dir, cwd=tutorial_tool_dir)
                
            #################################
            ##### TOP ROW BUTTON EVENTS #####
            #################################
 
            elif self.event == '-BUILD_CFS-':
            
                if self.cfs_subprocess is None:
                    build_cfs_sh = os.path.join(self.path, SH_BUILD_CFS)
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
                start_cfs_sh     = os.path.join(self.path, SH_START_CFS)
                cfs_abs_exe_path = os.path.join(self.cfs_abs_base_path, self.cfs_exe_rel_path) 
                #self.cfs_subprocess = subprocess.Popen('%s %s %s' % (start_cfs_sh, cfs_abs_exe_path, self.cfs_exe_file), shell=True)
                #self.cfs_subprocess = subprocess.Popen('%s %s %s' % (start_cfs_sh, cfs_abs_exe_path, self.cfs_exe_file),
                #                                       stdout=self.cfs_pty_slave, stderr=self.cfs_pty_slave, close_fds=True,
                #                                       shell=True) #, bufsize=1, universal_newlines=True)
                self.cfs_subprocess = subprocess.Popen('%s %s %s' % (start_cfs_sh, cfs_abs_exe_path, self.cfs_exe_file),
                                                       stdout=subprocess.PIPE, shell=True, bufsize=1, universal_newlines=True,
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
                    self.window["-CFS_TIME-"].update(self.GUI_NULL_TXT)
                else:
                    self.window["-CFS_IMAGE-"].update(self.GUI_NO_IMAGE_TXT)
                    self.window["-CFS_TIME-"].update(self.GUI_NULL_TXT)
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
                         self.window["-CFS_TIME-"].update(self.GUI_NULL_TXT)
                    """
        
            elif self.event == '-COMMON_CMD-':
                cfs_config_cmd = self.values['-COMMON_CMD-']
                
                if cfs_config_cmd == self.common_cmds[1]: # Enable Telemetry
                    self.enable_telemetry()

                elif cfs_config_cmd == self.common_cmds[2]: # Reset Time
                    self.send_cfs_cmd('CFE_TIME', 'SetMETCmd', {'Seconds': 0,'MicroSeconds': 0 })
                    time.sleep(0.5)
                    self.send_cfs_cmd('CFE_TIME', 'SetTimeCmd', {'Seconds': 0,'MicroSeconds': 0 })
            
                elif cfs_config_cmd == self.common_cmds[3]: # Noop/Reset App

                    pop_win = sg.Window('Noop-Reset Application',
                                        [[sg.Text("")],
                                         [sg.Text("Select App"), sg.Combo((self.app_cmd_list), size=(20,1), key='-APP_NAME-', default_value=self.app_cmd_list[0])],
                                         [sg.Text("")],
                                         [sg.Button('Noop', button_color=('SpringGreen4'), enable_events=True, key='-NOOP-', pad=(10,1)),
                                          sg.Button('Reset', button_color=('SpringGreen4'), enable_events=True, key='-RESET-', pad=(10,1)),
                                          sg.Cancel(button_color=('gray'))]])

                    while True:  # Event Loop
                        pop_event, pop_values = pop_win.read(timeout=200)
                        if pop_event in (sg.WIN_CLOSED, 'Cancel'):
                            break
                        if pop_event in ('-NOOP-', '-RESET-'):
                            app_name = pop_values['-APP_NAME-']
                            if app_name != EdsMission.TOPIC_CMD_TITLE_KEY:
                                if app_name == 'CI_LAB':  #todo: Remove CI_LAB or update to use OSK_CI that follow osk_c_fw standards
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
                    
                elif cfs_config_cmd == self.common_cmds[4]: # Restart App 
                    pop_win = sg.Window('Restart Application',
                                        [[sg.Text("")],
                                         [sg.Text("Select App"), sg.Combo((self.app_cmd_list), size=(20,1), key='-APP_NAME-', default_value=self.app_cmd_list[0])],
                                         [sg.Text("")],
                                         [sg.Button('Restart', button_color=('SpringGreen4'), enable_events=True, key='-RESTART-', pad=(10,1)),
                                          sg.Cancel(button_color=('gray'))]])
                
                    while True:  # Event Loop
                        pop_event, pop_values = pop_win.read(timeout=200)
                        if pop_event in (sg.WIN_CLOSED, 'Cancel'):
                            break
                        if pop_event == '-RESTART-':
                            app_name = pop_values['-APP_NAME-']
                            if app_name != EdsMission.TOPIC_CMD_TITLE_KEY:
                                self.send_cfs_cmd('CFE_ES', 'RestartAppCmd',  {'Application': app_name})
                            break        
                    pop_win.close()

                elif cfs_config_cmd == self.common_cmds[5]: # Configure Events
                    app_list = self.cfe_apps + self.app_cmd_list
                    pop_win = sg.Window('Configure App Events',
                                        [[sg.Text("")],
                                         [sg.Text("Select App"), sg.Combo((app_list), size=(20,1), key='-APP_NAME-', default_value=app_list[0])],
                                         [sg.Checkbox('Debug', key='-DEBUG-', default=False), sg.Checkbox('Information', key='-INFO-', default=True),
                                          sg.Checkbox('Error', key='-ERROR-', default=True),  sg.Checkbox('Critical', key='-CRITICAL-', default=True)], 
                                         [sg.Text("")],
                                         [sg.Button('Enable', button_color=('SpringGreen4'), enable_events=True, key='-ENABLE-', pad=(10,1)),
                                          sg.Button('Disable', button_color=('red4'), enable_events=True, key='-DISABLE-', pad=(10,1)), 
                                          sg.Cancel(button_color=('gray'))]])
                
                    while True:  # Event Loop
                        pop_event, pop_values = pop_win.read(timeout=200)
                        if pop_event in (sg.WIN_CLOSED, 'Cancel'):
                            break
                        if pop_event in ('-ENABLE-', '-DISABLE-'):
                            app_name = pop_values['-APP_NAME-'] 
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
                
            
                elif cfs_config_cmd == self.common_cmds[6]: # Ena/Dis Flywheel
            
                    pop_text = "cFE TIME outputs an event when it starts/stops flywheel mode\nthat occurs when time can't synch to the 1Hz pulse. Use the\nbuttons to enable/disable the flywheel event messages..."
                    pop_win = sg.Window('Flywheel Message Configuration',
                                        [[sg.Text(pop_text)],
                                        [sg.Text("")],
                                        [sg.Button('Enable', button_color=('green'), enable_events=True, key='-FLYWHEEL_ENABLE-', pad=(10,1)),
                                         sg.Button('Disable', button_color=('red'), enable_events=True, key='-FLYWHEEL_DISABLE-', pad=(10,1)), 
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

                elif cfs_config_cmd == self.common_cmds[7]: # cFE Version (CFE ES Noop)
                    self.send_cfs_cmd('CFE_ES', 'NoopCmd', {})
            
                   
            elif self.event == '-CMD_TOPICS-':
                #todo: Create a command string for event window. Raw text may be an option so people can capture commands
                cmd_topic = self.values['-CMD_TOPICS-']
                if cmd_topic != EdsMission.TOPIC_CMD_TITLE_KEY:
                    (cmd_sent, cmd_text, cmd_status) = self.telecommand_gui.execute(cmd_topic)
                    # If a command is aborted the status string is empty
                    if len(cmd_status) > 0:
                        self.display_event(cmd_status)
                    self.display_event(cmd_text)
                else:
                    sg.popup('Please select a command topic from the dropdown list', title='Command Topic', keep_on_top=True, non_blocking=True, grab_anywhere=True, modal=False)
            
            elif self.event == '-TLM_TOPICS-':
                tlm_topic = self.values['-TLM_TOPICS-']
                if tlm_topic != EdsMission.TOPIC_TLM_TITLE_KEY:
                    
                    app_name = self.tlm_server.get_app_name_from_topic(tlm_topic)
                    tlm_screen_cmd_parms = f'{self.tlm_screen_port} {app_name} {tlm_topic}'
                    self.cmd_tlm_router.add_tlm_dest(self.tlm_screen_port)
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

    image_grey1 = b'iVBORw0KGgoAAAANSUhEUgAAAFIAAAAgCAYAAACBxi9RAAAACXBIWXMAAAsTAAALEwEAmpwYAAAKT2lDQ1BQaG90b3Nob3AgSUNDIHByb2ZpbGUAAHjanVNnVFPpFj333vRCS4iAlEtvUhUIIFJCi4AUkSYqIQkQSoghodkVUcERRUUEG8igiAOOjoCMFVEsDIoK2AfkIaKOg6OIisr74Xuja9a89+bN/rXXPues852zzwfACAyWSDNRNYAMqUIeEeCDx8TG4eQuQIEKJHAAEAizZCFz/SMBAPh+PDwrIsAHvgABeNMLCADATZvAMByH/w/qQplcAYCEAcB0kThLCIAUAEB6jkKmAEBGAYCdmCZTAKAEAGDLY2LjAFAtAGAnf+bTAICd+Jl7AQBblCEVAaCRACATZYhEAGg7AKzPVopFAFgwABRmS8Q5ANgtADBJV2ZIALC3AMDOEAuyAAgMADBRiIUpAAR7AGDIIyN4AISZABRG8lc88SuuEOcqAAB4mbI8uSQ5RYFbCC1xB1dXLh4ozkkXKxQ2YQJhmkAuwnmZGTKBNA/g88wAAKCRFRHgg/P9eM4Ors7ONo62Dl8t6r8G/yJiYuP+5c+rcEAAAOF0ftH+LC+zGoA7BoBt/qIl7gRoXgugdfeLZrIPQLUAoOnaV/Nw+H48PEWhkLnZ2eXk5NhKxEJbYcpXff5nwl/AV/1s+X48/Pf14L7iJIEyXYFHBPjgwsz0TKUcz5IJhGLc5o9H/LcL//wd0yLESWK5WCoU41EScY5EmozzMqUiiUKSKcUl0v9k4t8s+wM+3zUAsGo+AXuRLahdYwP2SycQWHTA4vcAAPK7b8HUKAgDgGiD4c93/+8//UegJQCAZkmScQAAXkQkLlTKsz/HCAAARKCBKrBBG/TBGCzABhzBBdzBC/xgNoRCJMTCQhBCCmSAHHJgKayCQiiGzbAdKmAv1EAdNMBRaIaTcA4uwlW4Dj1wD/phCJ7BKLyBCQRByAgTYSHaiAFiilgjjggXmYX4IcFIBBKLJCDJiBRRIkuRNUgxUopUIFVIHfI9cgI5h1xGupE7yAAygvyGvEcxlIGyUT3UDLVDuag3GoRGogvQZHQxmo8WoJvQcrQaPYw2oefQq2gP2o8+Q8cwwOgYBzPEbDAuxsNCsTgsCZNjy7EirAyrxhqwVqwDu4n1Y8+xdwQSgUXACTYEd0IgYR5BSFhMWE7YSKggHCQ0EdoJNwkDhFHCJyKTqEu0JroR+cQYYjIxh1hILCPWEo8TLxB7iEPENyQSiUMyJ7mQAkmxpFTSEtJG0m5SI+ksqZs0SBojk8naZGuyBzmULCAryIXkneTD5DPkG+Qh8lsKnWJAcaT4U+IoUspqShnlEOU05QZlmDJBVaOaUt2ooVQRNY9aQq2htlKvUYeoEzR1mjnNgxZJS6WtopXTGmgXaPdpr+h0uhHdlR5Ol9BX0svpR+iX6AP0dwwNhhWDx4hnKBmbGAcYZxl3GK+YTKYZ04sZx1QwNzHrmOeZD5lvVVgqtip8FZHKCpVKlSaVGyovVKmqpqreqgtV81XLVI+pXlN9rkZVM1PjqQnUlqtVqp1Q61MbU2epO6iHqmeob1Q/pH5Z/YkGWcNMw09DpFGgsV/jvMYgC2MZs3gsIWsNq4Z1gTXEJrHN2Xx2KruY/R27iz2qqaE5QzNKM1ezUvOUZj8H45hx+Jx0TgnnKKeX836K3hTvKeIpG6Y0TLkxZVxrqpaXllirSKtRq0frvTau7aedpr1Fu1n7gQ5Bx0onXCdHZ4/OBZ3nU9lT3acKpxZNPTr1ri6qa6UbobtEd79up+6Ynr5egJ5Mb6feeb3n+hx9L/1U/W36p/VHDFgGswwkBtsMzhg8xTVxbzwdL8fb8VFDXcNAQ6VhlWGX4YSRudE8o9VGjUYPjGnGXOMk423GbcajJgYmISZLTepN7ppSTbmmKaY7TDtMx83MzaLN1pk1mz0x1zLnm+eb15vft2BaeFostqi2uGVJsuRaplnutrxuhVo5WaVYVVpds0atna0l1rutu6cRp7lOk06rntZnw7Dxtsm2qbcZsOXYBtuutm22fWFnYhdnt8Wuw+6TvZN9un2N/T0HDYfZDqsdWh1+c7RyFDpWOt6azpzuP33F9JbpL2dYzxDP2DPjthPLKcRpnVOb00dnF2e5c4PziIuJS4LLLpc+Lpsbxt3IveRKdPVxXeF60vWdm7Obwu2o26/uNu5p7ofcn8w0nymeWTNz0MPIQ+BR5dE/C5+VMGvfrH5PQ0+BZ7XnIy9jL5FXrdewt6V3qvdh7xc+9j5yn+M+4zw33jLeWV/MN8C3yLfLT8Nvnl+F30N/I/9k/3r/0QCngCUBZwOJgUGBWwL7+Hp8Ib+OPzrbZfay2e1BjKC5QRVBj4KtguXBrSFoyOyQrSH355jOkc5pDoVQfujW0Adh5mGLw34MJ4WHhVeGP45wiFga0TGXNXfR3ENz30T6RJZE3ptnMU85ry1KNSo+qi5qPNo3ujS6P8YuZlnM1VidWElsSxw5LiquNm5svt/87fOH4p3iC+N7F5gvyF1weaHOwvSFpxapLhIsOpZATIhOOJTwQRAqqBaMJfITdyWOCnnCHcJnIi/RNtGI2ENcKh5O8kgqTXqS7JG8NXkkxTOlLOW5hCepkLxMDUzdmzqeFpp2IG0yPTq9MYOSkZBxQqohTZO2Z+pn5mZ2y6xlhbL+xW6Lty8elQfJa7OQrAVZLQq2QqboVFoo1yoHsmdlV2a/zYnKOZarnivN7cyzytuQN5zvn//tEsIS4ZK2pYZLVy0dWOa9rGo5sjxxedsK4xUFK4ZWBqw8uIq2Km3VT6vtV5eufr0mek1rgV7ByoLBtQFr6wtVCuWFfevc1+1dT1gvWd+1YfqGnRs+FYmKrhTbF5cVf9go3HjlG4dvyr+Z3JS0qavEuWTPZtJm6ebeLZ5bDpaql+aXDm4N2dq0Dd9WtO319kXbL5fNKNu7g7ZDuaO/PLi8ZafJzs07P1SkVPRU+lQ27tLdtWHX+G7R7ht7vPY07NXbW7z3/T7JvttVAVVN1WbVZftJ+7P3P66Jqun4lvttXa1ObXHtxwPSA/0HIw6217nU1R3SPVRSj9Yr60cOxx++/p3vdy0NNg1VjZzG4iNwRHnk6fcJ3/ceDTradox7rOEH0x92HWcdL2pCmvKaRptTmvtbYlu6T8w+0dbq3nr8R9sfD5w0PFl5SvNUyWna6YLTk2fyz4ydlZ19fi753GDborZ752PO32oPb++6EHTh0kX/i+c7vDvOXPK4dPKy2+UTV7hXmq86X23qdOo8/pPTT8e7nLuarrlca7nuer21e2b36RueN87d9L158Rb/1tWeOT3dvfN6b/fF9/XfFt1+cif9zsu72Xcn7q28T7xf9EDtQdlD3YfVP1v+3Njv3H9qwHeg89HcR/cGhYPP/pH1jw9DBY+Zj8uGDYbrnjg+OTniP3L96fynQ89kzyaeF/6i/suuFxYvfvjV69fO0ZjRoZfyl5O/bXyl/erA6xmv28bCxh6+yXgzMV70VvvtwXfcdx3vo98PT+R8IH8o/2j5sfVT0Kf7kxmTk/8EA5jz/GMzLdsAADsHaVRYdFhNTDpjb20uYWRvYmUueG1wAAAAAAA8P3hwYWNrZXQgYmVnaW49Iu+7vyIgaWQ9Ilc1TTBNcENlaGlIenJlU3pOVGN6a2M5ZCI/Pgo8eDp4bXBtZXRhIHhtbG5zOng9ImFkb2JlOm5zOm1ldGEvIiB4OnhtcHRrPSJBZG9iZSBYTVAgQ29yZSA1LjYtYzE0NSA3OS4xNjIzMTksIDIwMTgvMDIvMTUtMjA6Mjk6NDMgICAgICAgICI+CiAgIDxyZGY6UkRGIHhtbG5zOnJkZj0iaHR0cDovL3d3dy53My5vcmcvMTk5OS8wMi8yMi1yZGYtc3ludGF4LW5zIyI+CiAgICAgIDxyZGY6RGVzY3JpcHRpb24gcmRmOmFib3V0PSIiCiAgICAgICAgICAgIHhtbG5zOnhtcD0iaHR0cDovL25zLmFkb2JlLmNvbS94YXAvMS4wLyIKICAgICAgICAgICAgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iCiAgICAgICAgICAgIHhtbG5zOnN0RXZ0PSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VFdmVudCMiCiAgICAgICAgICAgIHhtbG5zOnBob3Rvc2hvcD0iaHR0cDovL25zLmFkb2JlLmNvbS9waG90b3Nob3AvMS4wLyIKICAgICAgICAgICAgeG1sbnM6ZGM9Imh0dHA6Ly9wdXJsLm9yZy9kYy9lbGVtZW50cy8xLjEvIgogICAgICAgICAgICB4bWxuczp0aWZmPSJodHRwOi8vbnMuYWRvYmUuY29tL3RpZmYvMS4wLyIKICAgICAgICAgICAgeG1sbnM6ZXhpZj0iaHR0cDovL25zLmFkb2JlLmNvbS9leGlmLzEuMC8iPgogICAgICAgICA8eG1wOkNyZWF0b3JUb29sPkFkb2JlIFBob3Rvc2hvcCBFbGVtZW50cyAxNy4wIChXaW5kb3dzKTwveG1wOkNyZWF0b3JUb29sPgogICAgICAgICA8eG1wOkNyZWF0ZURhdGU+MjAyMC0xMC0wM1QxMToyOTozMi0wNDowMDwveG1wOkNyZWF0ZURhdGU+CiAgICAgICAgIDx4bXA6TWV0YWRhdGFEYXRlPjIwMjAtMTAtMDNUMTE6Mjk6MzItMDQ6MDA8L3htcDpNZXRhZGF0YURhdGU+CiAgICAgICAgIDx4bXA6TW9kaWZ5RGF0ZT4yMDIwLTEwLTAzVDExOjI5OjMyLTA0OjAwPC94bXA6TW9kaWZ5RGF0ZT4KICAgICAgICAgPHhtcE1NOkluc3RhbmNlSUQ+eG1wLmlpZDo2Y2Q5MjZlZS0xYWE3LTBlNDEtYTI2ZS04MmMwMGYyN2E2Nzg8L3htcE1NOkluc3RhbmNlSUQ+CiAgICAgICAgIDx4bXBNTTpEb2N1bWVudElEPmFkb2JlOmRvY2lkOnBob3Rvc2hvcDozMzlhMjcxYS0wNThkLTExZWItOTQ3ZC04N2E5Njc3OWZkYzU8L3htcE1NOkRvY3VtZW50SUQ+CiAgICAgICAgIDx4bXBNTTpPcmlnaW5hbERvY3VtZW50SUQ+eG1wLmRpZDpjZDY3N2JmMi02YjVjLWU4NDgtYTI0OC1kOGRkNGNkZTBkMzM8L3htcE1NOk9yaWdpbmFsRG9jdW1lbnRJRD4KICAgICAgICAgPHhtcE1NOkhpc3Rvcnk+CiAgICAgICAgICAgIDxyZGY6U2VxPgogICAgICAgICAgICAgICA8cmRmOmxpIHJkZjpwYXJzZVR5cGU9IlJlc291cmNlIj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OmFjdGlvbj5jcmVhdGVkPC9zdEV2dDphY3Rpb24+CiAgICAgICAgICAgICAgICAgIDxzdEV2dDppbnN0YW5jZUlEPnhtcC5paWQ6Y2Q2NzdiZjItNmI1Yy1lODQ4LWEyNDgtZDhkZDRjZGUwZDMzPC9zdEV2dDppbnN0YW5jZUlEPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6d2hlbj4yMDIwLTEwLTAzVDExOjI5OjMyLTA0OjAwPC9zdEV2dDp3aGVuPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6c29mdHdhcmVBZ2VudD5BZG9iZSBQaG90b3Nob3AgRWxlbWVudHMgMTcuMCAoV2luZG93cyk8L3N0RXZ0OnNvZnR3YXJlQWdlbnQ+CiAgICAgICAgICAgICAgIDwvcmRmOmxpPgogICAgICAgICAgICAgICA8cmRmOmxpIHJkZjpwYXJzZVR5cGU9IlJlc291cmNlIj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OmFjdGlvbj5zYXZlZDwvc3RFdnQ6YWN0aW9uPgogICAgICAgICAgICAgICAgICA8c3RFdnQ6aW5zdGFuY2VJRD54bXAuaWlkOjZjZDkyNmVlLTFhYTctMGU0MS1hMjZlLTgyYzAwZjI3YTY3ODwvc3RFdnQ6aW5zdGFuY2VJRD4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OndoZW4+MjAyMC0xMC0wM1QxMToyOTozMi0wNDowMDwvc3RFdnQ6d2hlbj4KICAgICAgICAgICAgICAgICAgPHN0RXZ0OnNvZnR3YXJlQWdlbnQ+QWRvYmUgUGhvdG9zaG9wIEVsZW1lbnRzIDE3LjAgKFdpbmRvd3MpPC9zdEV2dDpzb2Z0d2FyZUFnZW50PgogICAgICAgICAgICAgICAgICA8c3RFdnQ6Y2hhbmdlZD4vPC9zdEV2dDpjaGFuZ2VkPgogICAgICAgICAgICAgICA8L3JkZjpsaT4KICAgICAgICAgICAgPC9yZGY6U2VxPgogICAgICAgICA8L3htcE1NOkhpc3Rvcnk+CiAgICAgICAgIDxwaG90b3Nob3A6RG9jdW1lbnRBbmNlc3RvcnM+CiAgICAgICAgICAgIDxyZGY6QmFnPgogICAgICAgICAgICAgICA8cmRmOmxpPnhtcC5kaWQ6MDE4MDExNzQwNzIwNjgxMTg3MUZEQjdDNzNFQzdBRjQ8L3JkZjpsaT4KICAgICAgICAgICAgPC9yZGY6QmFnPgogICAgICAgICA8L3Bob3Rvc2hvcDpEb2N1bWVudEFuY2VzdG9ycz4KICAgICAgICAgPHBob3Rvc2hvcDpDb2xvck1vZGU+MzwvcGhvdG9zaG9wOkNvbG9yTW9kZT4KICAgICAgICAgPHBob3Rvc2hvcDpJQ0NQcm9maWxlPnNSR0IgSUVDNjE5NjYtMi4xPC9waG90b3Nob3A6SUNDUHJvZmlsZT4KICAgICAgICAgPGRjOmZvcm1hdD5pbWFnZS9wbmc8L2RjOmZvcm1hdD4KICAgICAgICAgPHRpZmY6T3JpZW50YXRpb24+MTwvdGlmZjpPcmllbnRhdGlvbj4KICAgICAgICAgPHRpZmY6WFJlc29sdXRpb24+NzIwMDAwLzEwMDAwPC90aWZmOlhSZXNvbHV0aW9uPgogICAgICAgICA8dGlmZjpZUmVzb2x1dGlvbj43MjAwMDAvMTAwMDA8L3RpZmY6WVJlc29sdXRpb24+CiAgICAgICAgIDx0aWZmOlJlc29sdXRpb25Vbml0PjI8L3RpZmY6UmVzb2x1dGlvblVuaXQ+CiAgICAgICAgIDxleGlmOkNvbG9yU3BhY2U+MTwvZXhpZjpDb2xvclNwYWNlPgogICAgICAgICA8ZXhpZjpQaXhlbFhEaW1lbnNpb24+ODI8L2V4aWY6UGl4ZWxYRGltZW5zaW9uPgogICAgICAgICA8ZXhpZjpQaXhlbFlEaW1lbnNpb24+MzI8L2V4aWY6UGl4ZWxZRGltZW5zaW9uPgogICAgICA8L3JkZjpEZXNjcmlwdGlvbj4KICAgPC9yZGY6UkRGPgo8L3g6eG1wbWV0YT4KICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAKPD94cGFja2V0IGVuZD0idyI/PkjKk0kAAAAgY0hSTQAAeiUAAICDAAD5/wAAgOkAAHUwAADqYAAAOpgAABdvkl/FRgAAAgtJREFUeNrsmi1vFFEUhp97Z6b7PYtuNyQYQkIwBNNgoAKBA4FAIJAIdP/BViOQCAQCgUQSBB+GYDZtCGnShKAIgp3dndl05p6L2G2ABrrscgXivGbEuCfvc8+ZyTXM8/Dx03ONZmvHWrtljOmg+WO89yMReVHkk+17d259ADAADx49OZ92T70+c7rXTdMuePB4JfabGAwYyLIhB58+D7Pht8v3797ejYGo0Wz1exvr3XitznCUI14hnhRrDMland7GevegLPvAjRhIrI2uJrUaxWGplP4i4j3VoZDUalgbbQFJDERAuyoFr01cKlUpAC0gigEr3uMU4tJx3h8dgzYGEBFERMmsovmcWzwf52ghV16FfoAU8YjXRq7WyF9ACl60kv+stnjR3XHlVUiOq60gg6gtqnYAtXX9CQVS1Va1/7ep7VTtQFNbGxlGbadfNqGmtjYyzNTWMzKA2l5w2sgQw0YX8mBqayMDqO29npGr5tiPXVU7iNpOZFxVrm2sVTLLtFEEJ5IfgZRpkb/Jx9m1RjtVOkukGGdMi/wVIDHg9vcG/bWktpk612m0UrSZi5tYTDKyr19G+3uDPuAMswsCnSvXb146e+Hidr3e2DTWNhXXiSDz6bR4+3Hwfufl82fvgJFhdpEqZnZjoA3UAa3kghkDTIExMAEq89PLCEjmTwW5GKQDyvmT7wMAzpNJbp+doKQAAAAASUVORK5CYII='

    #todo - Try to get reload EDS to work
    execute = True
    while execute:
        app = App('basecamp.ini')
        execute = app.execute()
        break
                
    logger.info('Exiting app')


