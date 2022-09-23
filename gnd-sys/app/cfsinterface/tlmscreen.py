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
      Display a single telemetry message
      
    Notes:
      1. The telemetry is displayed in a text box with each row containing
         one data point. The data point labels are extracted from the EDS
         message definition. The screen layout can't be altered.
                
"""

import sys
import time
import os
import socket
import configparser
import io
import PySimpleGUI as sg

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
from tools import crc_32c, compress_abs_path, TextEditor

###############################################################################

class TelemetryCurrentValue(TelemetryObserver):
    """
    callback_functions
       [app_name] : {packet: [item list]} 
    
    """

    def __init__(self, tlm_server: TelemetrySocketServer, data_callback): 
        super().__init__(tlm_server)

        self.data_callback = data_callback
        
        for msg in self.tlm_server.tlm_messages:
            tlm_msg = self.tlm_server.tlm_messages[msg]
            self.tlm_server.add_msg_observer(tlm_msg, self)        
            print("TelemetryCurrentValue adding observer for %s: %s" % (tlm_msg.app_name, tlm_msg.msg_name))

    def update(self, tlm_msg: TelemetryMessage) -> None:
        """
        Receive telemetry updates
        """
        self.data_callback(tlm_msg)


###############################################################################

class TlmScreen():
    """
    Create a screen that displays a single telemetry message
    """
    def __init__(self, gnd_ip_addr, tlm_port, tlm_timeout):

        self.tlm_server = TelemetrySocketServer('samplemission', 'cpu1', gnd_ip_addr, tlm_port, tlm_timeout)

        self.app_name  = ''
        self.tlm_topic = ''

        self.NULL_STR = self.tlm_server.eds_mission.NULL_STR


        self.payload_fmt_str = "{:<50}: {}\n"
        self.payload_str_max_len = 0
                
        self.paused = False
        self.payload_text = None

    def create_window(self, title):
        """
        """
        hdr_label_font = ('Arial bold',12)
        hdr_value_font = ('Arial',12)
        sg.theme('LightGreen')
        layout = [[sg.Text('App ID: ', font=hdr_label_font),  sg.Text(self.NULL_STR, font=hdr_value_font, size=(12,1), key='-APP_ID-'), 
                   sg.Text('Length: ', font=hdr_label_font),  sg.Text(self.NULL_STR, font=hdr_value_font, size=(12,1), key='-LENGTH-'),
                   sg.Text('Seq Cnt: ', font=hdr_label_font), sg.Text(self.NULL_STR, font=hdr_value_font, size=(12,1), key='-SEQ_CNT-'),
                   sg.Text('Time: ', font=hdr_label_font),    sg.Text(self.NULL_STR, font=hdr_value_font, size=(12,1), key='-TIME-')],
                  [sg.Text('')], 
                  [sg.Text('Payload', font = ('Arial bold',14)), sg.Text('', font=hdr_value_font, key='-PAUSED-', pad=(10,0))],
                  [sg.MLine(default_text='-- No Messages Received --', font = ('Courier',12), enable_events=True, size=(65, 30), key='-PAYLOAD_TEXT-')],
                  [sg.Button('Pause'), sg.Button('Resume'), sg.Button('Close'), sg.Button('', key='-TLM_UPDATE-', visible=False)]]

        window = sg.Window(title, layout, resizable=True, grab_anywhere=True)
        
        return window
        
    def execute(self, app_name, tlm_topic):
        """
        The current value observer must be created after the GUI window is created and the
        first window read is performed 
        """
        self.app_name  = app_name
        self.tlm_topic = tlm_topic
        self.tlm_msg   = self.tlm_server.get_tlm_msg_from_topic(tlm_topic)

        self.tlm_current_value = TelemetryCurrentValue(self.tlm_server, self.update)
        self.tlm_server.execute()

        self.window = self.create_window(tlm_topic)

        while True:  # Event Loop

            event, values = self.window.read(timeout=200)
            
            if event in (sg.WIN_CLOSED, 'Close') or event is None:       
                break
            
            elif event == 'Pause':
                self.paused = True
                self.window['-PAUSED-'].update('Display Paused')

            elif event == 'Resume':
                self.paused = False
                self.window['-PAUSED-'].update('')

            elif event == '-TLM_UPDATE-':
                self.window['-APP_ID-'].update(self.current_msg.pri_hdr().AppId)
                self.window['-LENGTH-'].update(self.current_msg.pri_hdr().Length)
                self.window['-SEQ_CNT-'].update(self.current_msg.pri_hdr().Sequence)
                self.window['-TIME-'].update(str(self.current_msg.sec_hdr().Seconds))
                self.payload_text = ""
                self.format_payload_text(self.current_msg.eds_obj, self.current_msg.eds_entry.Name)
                self.window['-PAYLOAD_TEXT-'].update(self.payload_text)
     
        self.window.close()
        self.tlm_server.shutdown()

    def update(self, tlm_msg: TelemetryMessage):
        """
        Receive telemetry updates. Using window['-TLM_UPDATE-'].click() synchronizes
        the update with the window read loop. If this method weren't used additional
        logic would be reuired to make sure the first tlm update is process after the
        window read() ha executed at least once.
        
        self.payload_str_max_len is set here as opposed to the constructor because an
        initial tlm_msg object does not have its eds_obj and eds_entry attributes set
        """
        if tlm_msg.app_name == self.app_name:
            # Compute max length if it hasn't been done yet
            if self.payload_str_max_len == 0:
                self.payload_str_max(self.tlm_msg.eds_obj, self.tlm_msg.eds_entry.Name)
                if self.payload_str_max_len > 0:
                    self.payload_fmt_str = "{:<%d}: {}\n" % self.payload_str_max_len

            if tlm_msg.app_id == self.tlm_msg.app_id:
                self.current_msg = tlm_msg
                if not self.paused:
                    self.window['-TLM_UPDATE-'].click()


    def format_payload_text(self, base_object, base_name):
        """
        Recursive function that iterates over an EDS object and creates a string that can be displayed.
        """
        # Array display string
        if (self.tlm_server.eds_mission.lib_db.IsArray(base_object)):
            for i in range(len(base_object)):
                self.format_payload_text(base_object[i], f"{base_name}[{i}]")
        # Container display string
        elif (self.tlm_server.eds_mission.lib_db.IsContainer(base_object)):
            for item in base_object:
                self.format_payload_text(item[1], f"{base_name}.{item[0]}")
        # Everything else (number, enumeration, string, etc.)
        else:
            if '.Payload.' in base_name:
                self.payload_text += self.payload_fmt_str.format(base_name, base_object)

    def payload_str_max(self, base_object, base_name):
        """
        Recursive function that determines the longest payload string. This
        is helpful for formatting displayes .
        """
        # Array display string
        if (self.tlm_server.eds_mission.lib_db.IsArray(base_object)):
            for i in range(len(base_object)):
                self.payload_str_max(base_object[i], f"{base_name}[{i}]")
        # Container display string
        elif (self.tlm_server.eds_mission.lib_db.IsContainer(base_object)):
            for item in base_object:
                self.payload_str_max(item[1], f"{base_name}.{item[0]}")
        # Everything else (number, enumeration, string, etc.)
        else:
            if '.Payload.' in base_name:
                base_name_len = len(base_name)
                if base_name_len > self.payload_str_max_len:
                    self.payload_str_max_len = base_name_len

###############################################################################

if __name__ == '__main__':
    
    config = configparser.ConfigParser()
    config.read('../basecamp.ini')

    if len(sys.argv) > 1:
        tlm_port    = int(sys.argv[1])
        app_name    = sys.argv[2]
        tlm_topic   = sys.argv[3]
    else:     
        tlm_port    = config.getint('NETWORK', 'TLM_SCREEN_TLM_PORT')
        app_name    = 'OSK_C_DEMO'
        tlm_topic   = 'OSK_C_DEMO/Application/STATUS_TLM'
        app_name    = 'MQTT_GW'
        tlm_topic   = 'MQTT_GW/Application/HK_TLM'

    cfs_host_addr = config.get('NETWORK', 'CFS_HOST_ADDR')

    tlm_screen = TlmScreen(cfs_host_addr, tlm_port, 1.0)
    tlm_screen.execute(app_name, tlm_topic) 
    
