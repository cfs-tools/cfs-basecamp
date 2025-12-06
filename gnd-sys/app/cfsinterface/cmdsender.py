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
      Provide classes to manage command sender files. 

    Notes:
      None

"""

import sys
import time
import os
import socket
import configparser
from queue import Queue
from datetime import datetime

import logging
logger = logging.getLogger(__name__)

if __name__ == '__main__':
    sys.path.append('..')
    from cfeconstants  import Cfe
    from telecommand   import TelecommandScript
    from telemetry     import TelemetryMessage, TelemetryObserver, TelemetrySocketServer
    from cmdtlmprocess import CmdTlmProcess
else:
    from .cfeconstants  import Cfe
    from .telecommand   import TelecommandScript
    from .telemetry     import TelemetryMessage, TelemetryObserver, TelemetrySocketServer
    from .cmdtlmprocess import CmdTlmProcess
from tools import crc_32c, compress_abs_path, bin_hex_decode, bin_hex_encode, TextEditor, PySimpleGUI_License

import PySimpleGUI as sg

CMD_SENDER_FILE_EXT                = 'txt'
CMD_SENDER_START_CMD_DELIMITER     = '>'
CMD_SENDER_START_COMMENT_DELIMITER = '>#'
CMD_SENDER_CMD_COMMENT_DELIMITER   = '##'
CMD_SENDER_CMD_FIELD_DELIMITER     = ','

###############################################################################

class CmdSenderDir():
    """
    Manage command sender files. The constructor creates a list of the command
    sender files. load_file() reads a command sender file.
    This class isn't currently used. I left it in in case the need arises to 
    present a drop down menu of command senders contained in the cmd-sender
    folder.
    """
    def __init__(self, cmd_sender_dir):
        
        self.cmd_sender_dir   = cmd_sender_dir
        self.cmd_sender_files = []           # List of files without extension (for GUI)
        self.cmd_sender_file  = None         # Current sender file being displayed
        self.cmd_sender_list  = []           # List of current sender commands and comments
        
        try:
            for file in os.listdir(self.cmd_sender_dir).sort():
                file_name, file_extension = os.path.splitext(file)
                if file_extension == CMD_SENDER_FILE_EXT:
                    self.cmd_sender_files.append(file_name)
        except Exception as e:
            sg.popup('Exception:\n'+str(e), title="Command Sender Directory Error", modal=False)

        if len() == 0:
            sg.popup(f'No command sender files found in {cmd_sender_dir}' , title="Command Sender Directory Warning", modal=False)
       
        print(f'self.cmd_sender_files = {self.cmd_sender_files}')

        
    def load_file(self, cmd_sender_file):
        """
        cmd_sender_file: Basename of file since it came from drop down menu
        
        Read a command sender file and creates a list of dictionaries. Each 
        dictionary contains the command and its comment.
        
        Command Line syntax: > App_Name, Command, {comma separated parameters} ## Comment
                             > CFE_ES, QueryOneCmd, {APP_C_DEMO}  ## CFE_ES APP_TLM populated with APP_C_DEMO information
        """

        self.cmd_sender_file = os.path.join(self.cmd_sender_dir, cmd_sender_file+'.'+CMD_SENDER_FILE_EXT)
        self.cmd_sender_list = []
    
        try:
            with open(self.cmd_sender_file) as f:
                i = 0
                for line in f:
                    line = line.strip()
                    i += 1
                    if line.startswith(CMD_SENDER_START_CMD_DELIMITER):
                        tokens = line.split(CMD_SENDER_CMD_COMMENT_DELIMITER)
                        if len <= 2: 
                            self.cmd_sender_list.append(tokens)
                        else:
                            sg.popup(f'File line {i} has multiple instances of the comment delimeter: "{CMD_SENDER_CMD_FIELD_DELIMITER}"', title="Command Sender File Error", modal=False)
        except Exception as e:
            sg.popup('Exception:\n'+str(e), title="Command Sender File Error", modal=False)

        print(f'self.cmd_sender_list = {self.cmd_sender_list}')

    def cmd_sender(self):
        return self.cmd_sender


###############################################################################

class CmdSenderFile():
    """
    Extract commands from a command sender file.
    """
    def __init__(self):
        
        self.cmd_sender_pathfile = None   # Current sender path/file being displayed
        self.cmd_sender_filename = None   # Current sender file being displayed
        self.cmd_sender_commands = []     # List of current sender commands
        self.cmd_sender_comments = []     # List of current sender comments
        
        
    def load_file(self, cmd_sender_pathfile):
        """
        cmd_sender_file: Basename of file since it came from drop down menu
        
        Read a command sender file and creates a list of dictionaries. Each 
        dictionary contains the command and its comment.
        
        Command Line syntax: > App_Name, Command, {dictionary of parameters} ## Comment
                             > 'CFE_ES', 'QueryOneCmd', {'Application': 'APP_C_DEMO'}  ## CFE_ES APP_TLM populated with APP_C_DEMO information
        """

        self.cmd_sender_pathfile = cmd_sender_pathfile
        self.cmd_sender_filename = os.path.basename(cmd_sender_pathfile)
        self.cmd_sender_commands = []
        self.cmd_sender_comments = []
    
        try:
            with open(self.cmd_sender_pathfile) as f:
                i = 0
                for line in f:
                    line = line.strip()
                    print(line)
                    i += 1
                    if line.startswith(CMD_SENDER_START_COMMENT_DELIMITER):
                        self.cmd_sender_commands.append(line)  
                        self.cmd_sender_comments.append('  ')                        
                    elif line.startswith(CMD_SENDER_START_CMD_DELIMITER):
                        tokens = line.split(CMD_SENDER_CMD_COMMENT_DELIMITER)
                        token_len = len(tokens)
                        print(f'{token_len}: {tokens}')
                        if token_len <= 2: 
                            self.cmd_sender_commands.append(tokens[0].replace(CMD_SENDER_START_CMD_DELIMITER,"",1).strip())
                            if token_len == 2:
                                self.cmd_sender_comments.append(tokens[1].strip())
                            else:
                                self.cmd_sender_comments.append('')
                        else:
                            sg.popup(f'File line {i} has multiple instances of the command comment delimeter: "{CMD_SENDER_CMD_FIELD_DELIMITER}"' , title="Command Sender File Error", modal=False)
                            break
        except Exception as e:
            sg.popup('Exception:\n'+str(e), title="Command Sender File Error", modal=False)

        return (self.cmd_sender_filename, self.cmd_sender_commands, self.cmd_sender_comments)


    def cmd_sender(self):
        return self.cmd_sender


###############################################################################

class HelpText():
    """
    """
    def __init__(self):
  
        self.text = \
           ("Command sender allows users to load a list of commands from a\n"
           "command sender file into the GUI. The user sends commands by right\n"
           "clicking on the command line and selecting 'Send' from the drop\n"
           "down menu. Comments describing a command's behavior are displayed\n"
           "in the 'Comments' window.\n")
            
    def display(self):
            
        sg.popup(self.text, line_width=85, font=('Courier',12), title='Command Sender Help', grab_anywhere=True)


###############################################################################

class CmdSenderTelemetryMonitor(TelemetryObserver):
    """
    callback_functions
       [app_name] : {packet: [item list]} 
    
    """

    def __init__(self, tlm_server: TelemetrySocketServer, tlm_monitors, event_callback): 
        super().__init__(tlm_server)

        self.tlm_monitors = tlm_monitors
        self.event_callback = event_callback
        
        self.sys_apps = ['CFE_ES', 'CFE_EVS']
        
        for msg in self.tlm_server.tlm_messages:
            tlm_msg = self.tlm_server.tlm_messages[msg]
            if tlm_msg.app_name in self.sys_apps:
                self.tlm_server.add_msg_observer(tlm_msg, self)        
                #logger.info("system telemetry adding observer for %s: %s" % (tlm_msg.app_name, tlm_msg.msg_name))
                print("CmdSenderTelemetryMonitor adding observer for %s: %s" % (tlm_msg.app_name, tlm_msg.msg_name))
        

    def update(self, tlm_msg: TelemetryMessage) -> None:
        """
        Receive telemetry updates
        """
        #todo: Determine best tlm identification method: if int(tlm_msg.app_id) == int(self.cfe_es_hk.app_id):
        
        if tlm_msg.app_name == 'CFE_EVS':
            if tlm_msg.msg_name == 'LONG_EVENT_MSG':
                payload = tlm_msg.payload()
                pkt_id = payload.PacketID
                event_text = "FSW Event at %s: %s, %d - %s" % \
                             (str(tlm_msg.sec_hdr().Seconds), pkt_id.AppName, pkt_id.EventType, payload.Message)
                self.event_callback(event_text)


###############################################################################

class CmdSender(CmdTlmProcess):
    """
    Provide a user interface for issuing commands from a command sender file.
    """
    def __init__(self, mission_name, cmd_sender_path, gnd_ip_addr, router_ctrl_port, browser_cmd_port, browser_tlm_port, browser_tlm_timeout):
        super().__init__(mission_name, gnd_ip_addr, router_ctrl_port, browser_cmd_port, browser_tlm_port, browser_tlm_timeout)

        self.cmd_sender_path = cmd_sender_path
        self.event_history = ""
        self.init_cycle = True
        self.help_text = HelpText()
            
        self.cmd_sender_filename = None
        self.cmd_sender_commands = []
        self.cmd_sender_comments = []
            
    def event_callback(self, event_txt):
        self.display_event(event_txt)
            
    def update_event_history_str(self, new_event_text):
        time = datetime.now().strftime("%H:%M:%S")
        event_str = time + " - " + new_event_text + "\n"        
        self.event_history += event_str
 
    def display_event(self, new_event_text):
        self.update_event_history_str(new_event_text)
        self.window["-EVENT_TEXT-"].update(self.event_history)
                
    def get_filename(self, gui_filename):
        """
        Get the filename from a GUI file listing. Both ground and flight listing
        start with the filename followed by a space.
        """
        return gui_filename.split(' ')[0]
        
    def gui(self):
        
        window_width = 100
        col_width  = int(window_width/2-3)
        col_height = 10
        menu_layout = [['File',['&Open...','---','Help','Exit']]]

        col_title_font = ('Arial bold',16)
        pri_hdr_font   = ('Arial bold',14)
        list_font      = ('Courier',11)
        log_font       = ('Courier',11)
        self.cmd_file_menu = ['_', ['Send']]
        self.command_col = [
            [sg.Text('Commands', font=col_title_font)],
            [sg.Listbox(values=[], font=list_font, enable_events=True, size=(col_width,col_height), key='-COMMAND_LIST-', right_click_menu=self.cmd_file_menu)]]
        
        self.comment_col = [
            [sg.Text('Comments', font=col_title_font)],
            [sg.MLine(default_text='', font=log_font, enable_events=True, size=(col_width,col_height), key='-COMMENT_LIST-')]]
            #[sg.Listbox(values=[], font=list_font, enable_events=True, size=(col_width,col_height), key='-COMMENT_LIST-')]]

        self.layout = [
            [sg.Menu(menu_layout)],
            [sg.Text('Command File:', font=pri_hdr_font), sg.Text('', font=pri_hdr_font, size=(col_width,1), relief=sg.RELIEF_RAISED, border_width=1, justification='center', key='-COMMAND_FILE-')],
            [sg.Text('Commands', font=col_title_font)],
            [sg.Listbox(values=[], font=list_font, enable_events=True, size=(window_width,10), key='-COMMAND_LIST-', right_click_menu=self.cmd_file_menu)],
            [sg.Text('Command Comment', font=col_title_font)],
            [sg.MLine(default_text='', font=log_font, enable_events=True, size=(window_width,5), key='-COMMENT_LIST-')],
            #[sg.Column(self.command_col, element_justification='c'), sg.VSeperator(), sg.Column(self.comment_col, element_justification='c')],
            [sg.Text('Events', font=pri_hdr_font), sg.Button('Clear', enable_events=True, key='-CLEAR_EVENTS-', pad=(5,1))],
            [sg.MLine(default_text=self.event_history, font=log_font, enable_events=True, size=(window_width, 5), key='-EVENT_TEXT-')]]
            
 
        self.window = sg.Window('Commmand Sender', self.layout, resizable=True)
        
        self.tlm_monitors = {'CFE_ES': {'HK_TLM': ['Seconds']}, 'FILE_MGR': {'DIR_LIST_TLM': ['Seconds']}}        
        self.tlm_monitor = CmdSenderTelemetryMonitor(self.tlm_server, self.tlm_monitors, self.event_callback)
        self.tlm_server.execute()

        while True:

            self.event, self.values = self.window.read(timeout=100)
        
            if self.init_cycle:
                self.init_cycle = False

            if self.event in (sg.WIN_CLOSED, 'Exit') or self.event is None:
                break

            ### Events listed in order of likelihood ###
            
            elif self.event == '-COMMAND_LIST-':
                if len(self.cmd_sender_comments) > 0:
                    selected_indices = self.window['-COMMAND_LIST-'].get_indexes()
                    if len(selected_indices) > 0:
                        self.window['-COMMENT_LIST-'].update(self.cmd_sender_comments[selected_indices[0]])

            elif self.event in ('Send'):
                command_line = self.values['-COMMAND_LIST-'][0]
                if len(command_line) > 0:
                    print(f'{command_line}')
                    if not command_line.startswith(CMD_SENDER_START_COMMENT_DELIMITER):
                        tokens = command_line.split(CMD_SENDER_CMD_FIELD_DELIMITER)                    
                        # Crude sanity check 
                        if len(tokens) >= 3:
                            cmd_str = f'self.send_cfs_cmd({command_line})'                        
                            try:
                               exec(cmd_str)
                            except Exception as e:
                               sg.popup('Error executing command\n'+str(e), title="Send Command Error", modal=False)
                        else:
                            sg.popup('Command must contain 3 fields separated by commas: App,Command,{Parameters}' , title="Command Sender Command Error", modal=False)
                    else:
                        sg.popup("This is a comment line that can't be sent", title="Command Sender Command Error", modal=False)                        
                else:
                    sg.popup("Please select/highlight a command to be sent", title='Send Command Error', grab_anywhere=True, modal=False)

            elif self.event == 'Open...':
                cmd_sender_file = sg.popup_get_file('', title='Command Sender File', no_window=True, default_path=self.cmd_sender_path, initial_folder=self.cmd_sender_path, file_types=(("Text Files", "*.txt"),), default_extension=CMD_SENDER_FILE_EXT) # , history=True)
                if cmd_sender_file is not None:
                    print(f'file={cmd_sender_file}')
                    (self.cmd_sender_filename, self.cmd_sender_commands, self.cmd_sender_comments) = CmdSenderFile().load_file(cmd_sender_file)
                    self.window['-COMMAND_FILE-'].update(self.cmd_sender_filename)
                    self.window['-COMMAND_LIST-'].update(self.cmd_sender_commands)
                
            elif self.event == 'Help':
                self.help_text.display()
                
            elif self.event == '-CLEAR_EVENTS-':
                self.event_history = ""
                self.display_event("Cleared event display")
                   
        self.shutdown()


    def execute(self):
        self.gui()
    
        
    def shutdown(self):

        self.window.close()       
        self.tlm_server.shutdown()    


###############################################################################

if __name__ == '__main__':

    config = configparser.ConfigParser()
    config.read('../basecamp.ini')

    cmd_sender_path = config.get('PATHS','CMD_SENDER_PATH')
    cmd_sender_path = compress_abs_path(os.path.join(os.getcwd(), '..', cmd_sender_path))
    print(f'cmd_sender_path = {cmd_sender_path}')
    cfs_ip_addr = config.get('NETWORK','CFS_IP_ADDR')
    router_ctrl_port = config.getint('NETWORK','CMD_TLM_ROUTER_CTRL_PORT')
    sender_cmd_port = config.getint('NETWORK','CMD_SENDER_CMD_PORT')
    sender_tlm_port = config.getint('NETWORK','CMD_SENDER_TLM_PORT')
    mission_name     = config.get('CFS_TARGET','MISSION_EDS_NAME')
    
    cmd_sender = CmdSender(mission_name, cmd_sender_path, cfs_ip_addr, router_ctrl_port, sender_cmd_port, sender_tlm_port, 1.0)
    cmd_sender.execute()
    
