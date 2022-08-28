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
        Provide classes that manage the MQTT interface to a remote target
        
    Notes:    
      None
       
"""

import sys
import time
import os
import configparser
import json
import logging
logger = logging.getLogger(__name__)

import paho.mqtt.client as mqtt
import PySimpleGUI as sg


###############################################################################

class TargetControl():


    # String constants used for JSON command message and GUI keys
    JSON_CMD_SUBSYSTEM_CFS    = 'cfs'
    JSON_CMD_SUBSYSTEM_PYTHON = 'python'
    JSON_CMD_SUBSYSTEM_TARGET = 'target' 
    
    JSON_CMD_TARGET_NOOP      = 'noop'
    JSON_CMD_TARGET_REBOOT    = 'reboot'
    JSON_CMD_TARGET_SHUTDOWN  = 'shutdown'

    JSON_CMD_CFS_START        = 'start'
    JSON_CMD_CFS_ENA_TLM      = 'ena-tlm'
    JSON_CMD_CFS_STOP         = 'stop'

    JSON_CMD_PYTHON_START     = 'start'
    JSON_CMD_PYTHON_STOP      = 'stop'


    # String constants used for JSON telemetry message and GUI keys
    JSON_TLM_SEQ_CNT = 'seq-cnt'
    JSON_TLM_CMD_CNT = 'cmd-cnt'
    JSON_TLM_EVENT   = 'event'

    JSON_TLM_CFS_RUNNING = 'cfs_running'
    JSON_TLM_CFS_APPS    = 'apps'

    JSON_TLM_PYTHON_RUNNING = 'app_running'
    JSON_TLM_PYTHON_SENSORS = 'sensors'   
    '''
    Manage the target interface 
    '''
    def __init__(self, target_name, broker_addr, broker_port, client_name):
        """
        """
        self.target_name = target_name
        self.broker_addr = broker_addr
        self.broker_port = broker_port
        self.client_name = client_name
        self.client_connected = False
        self.client = None
                
        self.cmd_topic = "osk/%s/cmd" % target_name
        self.tlm_topic = "osk/%s/tlm" % target_name

        self.json_tlm_keys =  [self.JSON_TLM_SEQ_CNT, self.JSON_TLM_CMD_CNT, self.JSON_TLM_EVENT, self.JSON_TLM_CFS_RUNNING, self.JSON_TLM_CFS_APPS, self.JSON_TLM_PYTHON_RUNNING, self.JSON_TLM_PYTHON_SENSORS]

        
    def client_on_connect(self, client, userdata, flags, rc):
        """
        """
        print("Connected with result code {0}".format(str(rc)))  # Print result of connection attempt 
        print("Subscribing to %s" % self.tlm_topic)
        self.client.subscribe(self.tlm_topic)
        self.client_connected = True 
 
    def client_on_disconnect(self, client, userdata, flags, rc):
        """
        """
        self.client_disconnect()


    def client_connect(self):
        try:
            self.client = mqtt.Client(self.client_name)
            self.client.on_connect    = self.client_on_connect     # Callback function for successful connection
            self.client.on_disconnect = self.client_on_disconnect  # Callback function for successful disconnect
            self.client.on_message    = self.process_tlm           # Callback function for receipt of a message
            self.client.connect(self.broker_addr)
            self.client.loop_start()  # Start networking daemon
            
        except Exception as e:
            err_str = "Error configuring MQTT client %s on %s:%s\n   %s" % (self.client_name, broker_addr, broker_port, str(e))
            logger.error(err_str)
            sg.popup(err_str, title="Target Control Error", modal=False)


    def client_disconnect(self):
        """
        This method will be called on the MQTT initiating a disconnect. It has not been detmereined whether calling
        the client's diconnect in this scenario causes an issue 
        """
        self.client_connected=False
        self.client.disconnect()


    def publish_cmd(self, subsystem, cmd):
        """
        """
        payload = '{"%s": "%s"}' % (subsystem, cmd)
        print("Publish: %s, %s" % (self.cmd_topic, payload))
        self.client.publish(self.cmd_topic, payload)

 
    def process_tlm(self, client, userdata, msg):
        """
        The callback for when a PUBLISH message is received from the server. The payload is
        a JSON telemetry object with the following fields:
        
            {
                "seq-cnt": integer,
                "cmd-cnt": integer,
                "event":   "Event message string",
                "running": boolean,
                "apps":    "Comma separated app names (cfS non-runtime app suite apps)",
                "sensors": "Comma separated sensor names (python sensor interfaces)",
            }
        
            The field names must match the GUI keys. See the class defined constants.
            
        Test string (paste into hivemq browser MQTT broker 
            {"seq-cnt": 1, "seq-cnt": 1, "running": true, "apps": "LAB_CI, FILE_MGR,...", "event": "This is a test"}
        
        """
        msg_str = msg.payload.decode()
        print("Message received-> " + msg.topic + " " + msg_str)
        tlm = json.loads(msg.payload.decode())
        for key in tlm:
            if key in self.json_tlm_keys:
                self.window[key].update(tlm[key])
            #TODO - Is it an error to have keys not recognized by this GUI?
            
    def create_window(self):

        hdr_label_font = ('Arial bold',12)
        hdr_value_font = ('Arial',11)
        but_row_text_size = (10,1)
        but_text_size     = (9,1) 
        
        centered_title = [[sg.Text('MQTT Client ', font=hdr_label_font), sg.Text('Disconnected', font=hdr_label_font, key='-CLIENT_STATE-')]]
        
        layout = [
            [sg.Column(centered_title, vertical_alignment='center', justification='center')],
            #[sg.Text('MQTT Client Disconnected', font=hdr_label_font, justification='center')],
            [sg.Text('')],
            [sg.Frame('Commands', 
                [[sg.Text('MQTT:',      size=but_row_text_size, font=hdr_label_font, pad=((5,0),(12,12))), 
                sg.Button('Connect',    size=but_text_size,     font=hdr_label_font, enable_events=True, key='-CLIENT_CONNECT-',    pad=((10,5),(12,12))),
                sg.Button('Disconnect', size=but_text_size,     font=hdr_label_font, enable_events=True, key='-CLIENT_DISCONNECT-', pad=((10,5),(12,12)))],
                [sg.Text('Target:',     size=but_row_text_size, font=hdr_label_font, pad=((5,0),(12,12))), 
                sg.Button('Noop',       size=but_text_size,     font=hdr_label_font, enable_events=True, key='-TARGET_NOOP-',     pad=((10,5),(12,12))),
                sg.Button('Reboot',     size=but_text_size,     font=hdr_label_font, enable_events=True, key='-TARGET_REBOOT-',   pad=((10,5),(12,12))),
                sg.Button('Shutdown',   size=but_text_size,     font=hdr_label_font, enable_events=True, key='-TARGET_SHUTDOWN-', pad=((10,5),(12,12)))],
                [sg.Text('cFS:',        size=but_row_text_size, font=hdr_label_font, pad=((5,0),(12,12))), 
                sg.Button('Start',      size=but_text_size,     font=hdr_label_font, enable_events=True, key='-CFS_START-',   pad=((10,5),(12,12))),
                sg.Button('Enable Tlm', size=but_text_size,     font=hdr_label_font, enable_events=True, key='-CFS_ENA_TLM-', pad=((10,5),(12,12))),
                sg.Button('Stop',       size=but_text_size,     font=hdr_label_font, enable_events=True, key='-CFS_STOP-',    pad=((10,5),(12,12)))],
                [sg.Text('Python App:', size=but_row_text_size, font=hdr_label_font, pad=((5,0),(12,12))), 
                sg.Button('Start',      size=but_text_size,     font=hdr_label_font, enable_events=True, key='-PYTHON_START-', pad=((10,5),(12,12))),
                sg.Button('Stop',       size=but_text_size,     font=hdr_label_font, enable_events=True, key='-PYTHON_STOP-',  pad=((10,5),(12,12)))]])
            ],    
            [sg.Text('  ', pad=((10,5),(12,12)))],
            [sg.Frame('cFS or Python App Status',
                [[sg.Text('Seq Cnt:',   size=(9,1),   font=hdr_label_font, pad=((5,0),(6,6))), 
                sg.Text('0',            size=(9,1),   font=hdr_value_font, key=self.JSON_TLM_SEQ_CNT)],
                [sg.Text('Cmd Cnt:',    size=(9,1),   font=hdr_label_font, pad=((5,0),(6,6))), 
                sg.Text('0',            size=(9,1),   font=hdr_value_font, key=self.JSON_TLM_CMD_CNT)],
                [sg.Text('Event:',      size=(9,1),   font=hdr_label_font, pad=((5,0),(6,6))), 
                sg.Text('None',         size=(50,1),  font=hdr_value_font, key=self.JSON_TLM_EVENT)],
                [sg.HorizontalSeparator()],
                [sg.Text('cFS Exe:',    size=(9,1),   font=hdr_label_font, pad=((5,0),(6,6))), 
                sg.Text('False',        size=(9,1),   font=hdr_value_font, key=self.JSON_TLM_CFS_RUNNING)],
                [sg.Text('User Apps:',  size=(9,1),   font=hdr_label_font, pad=((5,0),(6,6))), 
                sg.Text('None',         size=(45,1),  font=hdr_value_font, key=self.JSON_TLM_CFS_APPS)],
                [sg.HorizontalSeparator()],
                [sg.Text('App Exe:',    size=(9,1),   font=hdr_label_font, pad=((5,0),(6,6))), 
                sg.Text('False',        size=(9,1),   font=hdr_value_font, key=self.JSON_TLM_PYTHON_RUNNING)],
                [sg.Text('Sensors:',    size=(9,1),   font=hdr_label_font, pad=((5,0),(6,6))), 
                sg.Text('None',         size=(45,1),  font=hdr_value_font, key=self.JSON_TLM_PYTHON_SENSORS)]])
            ]
            
        ]

        window = sg.Window('Control Target %s'%self.target_name, layout, auto_size_text=True, finalize=True)
        return window


    def execute(self):
        self.client_connect();
            
        self.window = self.create_window()
                        
        while True:
    
            self.event, self.values = self.window.read(timeout=250)

            if self.event in (sg.WIN_CLOSED, 'Exit') or self.event is None:
                break

            if self.client_connected:
               self.window['-CLIENT_STATE-'].Update('Connected to ' + self.broker_addr, text_color='white')
            else:
               self.window['-CLIENT_STATE-'].Update('Disconnected', text_color='red')
                                       
            if self.event == '-CLIENT_CONNECT-':
               self.client_connect();
            
            elif self.event == '-CLIENT_DISCONNECT-':
               self.client_disconnect();
            
            elif self.event == '-TARGET_NOOP-':
                self.publish_cmd(self.JSON_CMD_SUBSYSTEM_TARGET, self.JSON_CMD_TARGET_NOOP)

            elif self.event == '-TARGET_REBOOT-':
                self.publish_cmd(self.JSON_CMD_SUBSYSTEM_TARGET, self.JSON_CMD_TARGET_REBOOT)
            
            elif self.event == '-TARGET_SHUTDOWN-':
                self.publish_cmd(self.JSON_CMD_SUBSYSTEM_TARGET, self.JSON_CMD_TARGET_SHUTDOWN)

            elif self.event == '-CFS_START-':
                self.publish_cmd(self.JSON_CMD_SUBSYSTEM_CFS, self.JSON_CMD_CFS_START)
                                 
            elif self.event == '-CFS_ENA_TLM-':
                self.publish_cmd(self.JSON_CMD_SUBSYSTEM_CFS, self.JSON_CMD_CFS_ENA_TLM)

            elif self.event == '-CFS_STOP-':
                self.publish_cmd(self.JSON_CMD_SUBSYSTEM_CFS, self.JSON_CMD_CFS_STOP)

            elif self.event == '-PYTHON_START-':
                self.publish_cmd(self.JSON_CMD_SUBSYSTEM_PYTHON, self.JSON_CMD_PYTHON_START)
                                 
            elif self.event == '-PYTHON_STOP-':
                self.publish_cmd(self.JSON_CMD_SUBSYSTEM_PYTHON, self.JSON_CMD_PYTHON_STOP)

###############################################################################

if __name__ == '__main__':

    config = configparser.ConfigParser()
    config.read('../basecamp.ini')

    broker_addr = config.get('NETWORK','MQTT_BROKER_ADDR')
    broker_port = config.get('NETWORK','MQTT_BROKER_PORT')
    print("Broker Address: %s, Port: %s" % (broker_addr, broker_port))
    
    client_name = config.get('NETWORK','MQTT_CLIENT_NAME')
    target_control = TargetControl('pisat-1', broker_addr, broker_port, client_name)
    
    target_control.execute()
    

