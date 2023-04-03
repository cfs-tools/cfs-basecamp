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
import os
import socket
import configparser
import json
import logging
logger = logging.getLogger(__name__)

import paho.mqtt.client as mqtt
import PySimpleGUI as sg

if __name__ == '__main__':
    sys.path.append('..')
    from cfeconstants  import Cfe
    from cmdtlmprocess import CmdProcess
    from cmdtlmrouter  import RouterCmd
else:
    from .cfeconstants  import Cfe
    from .cmdtlmprocess import CmdProcess
    from .cmdtlmrouter  import RouterCmd
from remoteops import mqttconst as mc
from tools import get_ip_addr


###############################################################################

class EnableTlm():
    """
    Choose how remote telemetry is managed
    """
    NONE   = 0
    SOCKET = 1
    MQTT   = 2
    def __init__(self):
        self.window   = None
        self.tlm_type = EnableTlm.NONE
        
    def create_window(self):
        hdr_label_font = ('Arial bold',12)
        hdr_value_font = ('Arial',12)
        layout = [
                     [sg.Radio("Socket", "TLM", default=True,  font=hdr_label_font, size=(15,0), key='-SOCKET-')],  
                     [sg.Radio("MQTT",   "TLM", default=False, font=hdr_label_font, size=(15,0), key='-MQTT-')],
                     [sg.Button('OK', font=hdr_label_font, button_color=('SpringGreen4')), sg.Button('Cancel', font=hdr_label_font)]
                 ]
        window = sg.Window('Select Telemetry Connection', layout, modal=False)
        return window

    def gui(self):
        """
        """        
        self.window = self.create_window() 
        
        while True: # Event Loop
            
            self.event, self.values = self.window.read()

            if self.event in (sg.WIN_CLOSED, 'Cancel') or self.event is None:       
                break
            
            if self.event == 'OK':
                if self.values["-SOCKET-"] == True:
                    self.tlm_type = EnableTlm.SOCKET
                else:
                    self.tlm_type = EnableTlm.MQTT
                break
                
        self.window.close()

    def execute(self):
        """
        """
        self.gui()
        return self.tlm_type


###############################################################################

class TargetControl(CmdProcess):
    """
    Manage the target interface 
    """
    TIMER_SET_CFS_IP = 6
    TIMER_ENABLE_TLM = 12
    def __init__(self, mission_name, gnd_ip_addr, router_ctrl_port, router_cmd_port, 
                target_mqtt_topic, broker_addr, broker_port, client_name,
                local_network_adapter):
        """
        """
        super().__init__(mission_name, gnd_ip_addr, router_cmd_port)
        
        self.local_network_adapter = local_network_adapter
        try:
           self.local_ip_addr = get_ip_addr(local_network_adapter)
           print(f'Local ip_addr: {self.local_ip_addr}')
        except Exception as e:
           err_str = f'Error obtaining host IP address using local adapter {local_network_adapter}\nUse ifconfig to verify adapter name in basecamp.ini\n{str(e)}'
           logger.error(err_str)
           sg.popup(err_str, title='Target Control Error', modal=False)
      
        self.router_ctrl_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.router_ctrl_socket_addr = (gnd_ip_addr, router_ctrl_port)
        
        self.remote_ip_addr = None

        # cFS Configuration        
        self.cfe_time_event_filter = False
        
        # MQTT Configuration        
        self.target_mqtt_topic = target_mqtt_topic
        self.broker_addr = broker_addr
        self.broker_port = broker_port
        self.client_name = client_name
        self.client_connected = False
        self.client = None
                
        self.cmd_topic = f'{target_mqtt_topic}/{mc.MQTT_TOPIC_CMD}'
        self.tlm_topic = f'{target_mqtt_topic}/{mc.MQTT_TOPIC_TLM}'

        self.json_tlm_keys =  [mc.JSON_TLM_IP_ADDR, mc.JSON_TLM_SEQ_CNT,
                               mc.JSON_TLM_CMD_CNT, mc.JSON_TLM_EVENT,
                               mc.JSON_TLM_CFS_EXE, mc.JSON_TLM_CFS_APPS,
                               mc.JSON_TLM_PY_EXE,  mc.JSON_TLM_PY_APPS]

        self.window = None
        
    def client_on_connect(self, client, userdata, flags, rc):
        """
        """
        print(f'Connected with result code {rc}') 
        print(f'Subscribing to {self.tlm_topic}')
        self.client.subscribe(self.tlm_topic)
        self.client_connected = True 
 
    def client_on_disconnect(self, client, userdata, flags):
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
            err_str = f'Error configuring MQTT client {self.client_name} on {broker_addr}:{broker_port}\n   {str(e)}'
            logger.error(err_str)
            sg.popup(err_str, title='Target Control Error', modal=False)


    def client_disconnect(self):
        """
        This method will be called on the MQTT initiating a disconnect. It has not been detmereined whether calling
        the client's diconnect in this scenario causes an issue 
        """
        self.client_connected=False
        self.client.disconnect()


    def publish_cmd(self, subsystem, cmd, param=''):
        """
        """
        payload = '{"%s": "%s", "%s": "%s"}' % \
                  (subsystem, cmd, mc.JSON_CMD_PARAMETER, param)  
        print(f'Publish: {self.cmd_topic}, {payload}')
        self.client.publish(self.cmd_topic, payload)

 
    def process_tlm(self, client, userdata, msg):
        """
        The callback for when a PUBLISH message is received from the server. The payload is
        a JSON telemetry object with the following fields:
        
            {
                "ip-addr":  "x.x.x.x",
                "seq-cnt":  integer,
                "cmd-cnt":  integer,
                "event":    "Event message string",
                "cfs-exe":  boolean, Is the cFS running?
                "cfs-apps": "Comma separated app names (cfS non-runtime app suite apps)",
                "py-exe":   boolean, Is a python app running?
                "py-apps":  "Comma separated python scripts (An asterick indicates the script is running)",
            }
        
            The field names must match the GUI keys. See the class defined constants.
        """
        # Connection establish prior to window creation so messages can be received before the window is created
        if self.window is None:
            return
        msg_str = msg.payload.decode()
        print("Message received-> " + msg.topic + " " + msg_str)
        tlm = json.loads(msg.payload.decode())
        for key in tlm:
            if key in self.json_tlm_keys:
                self.window[key].update(tlm[key])
                if key == 'ip-addr':
                   self.remote_ip_addr = tlm[key]
                   print(f'remote ip_addr: {self.remote_ip_addr}')
            #TODO - Is it an error to have keys not recognized by this GUI?
            

    def start_remote_cfs_tlm(self, timer):
        """
        The timer is used to space out sending commands to the cmdtlmrouter. 
        Sleep() can't be used because it blocks the window event loop and if
        telemetry is received when the loop is blocked then an error occurs.
        If the event loop timeout value is changed then this timing will need
        to change.
        """
        ret_status = True
        if timer == 0:
            print('Commanding KIT_TO to remote')
            self.send_cfs_cmd('KIT_TO', 'SetTlmSource',  {'Source': 'REMOTE'})
        elif timer == TargetControl.TIMER_SET_CFS_IP:
            print(f'Setting router cFS IP address to {self.remote_ip_addr}')
            datagram = f'{RouterCmd.SET_CFS_IP_ADDR}:{self.remote_ip_addr}'.encode('utf-8')
            self.router_ctrl_socket.sendto(datagram, self.router_ctrl_socket_addr)
        elif timer > TargetControl.TIMER_ENABLE_TLM:
            print(f'Commanding remote KIT_TO to enable telemetry to ground IP address {self.gnd_ip_addr}')
            self.send_cfs_cmd('KIT_TO', 'EnableOutput', {'DestIp': self.gnd_ip_addr})
            ret_status = False
            
        return ret_status

    def create_window(self):

        hdr_label_font = ('Arial bold',12)
        hdr_value_font = ('Arial',11)
        but_row_text_size = (8,1)
        but_text_size     = (9,1)
        tlm_label_size    = (7,1)
        
        centered_title = [[sg.Text(f'MQTT Client {self.client_name}:', font=hdr_label_font), sg.Text('Disconnected', font=hdr_label_font, key='-CLIENT_STATE-')]]
        
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
                sg.Button('Start',      size=but_text_size,     font=hdr_label_font, enable_events=True, key='-CFS_START-',     pad=((10,5),(12,12))),
                sg.Button('Stop',       size=but_text_size,     font=hdr_label_font, enable_events=True, key='-CFS_STOP-',      pad=((10,5),(12,12))),
                sg.Button('Start Tlm',  size=but_text_size,     font=hdr_label_font, enable_events=True, key='-CFS_START_TLM-', pad=((10,5),(12,12)))],
                [sg.Text('Python:',     size=but_row_text_size, font=hdr_label_font, pad=((5,0),(12,12))), 
                sg.Button('Start',      size=but_text_size,     font=hdr_label_font, enable_events=True, key='-PYTHON_START-', pad=((10,5),(12,12))),
                sg.Button('Stop',       size=but_text_size,     font=hdr_label_font, enable_events=True, key='-PYTHON_STOP-',  pad=((10,5),(12,12)))]])
            ],    
            [sg.Text('  ', pad=((10,5),(12,12)))],
            [sg.Frame('Remote Target Status',
                [[sg.Text('IP Addr:',   size=tlm_label_size, font=hdr_label_font, pad=((5,0),(6,6))), 
                sg.Text('0.0.0.0',      size=(50,1),         font=hdr_value_font, key=mc.JSON_TLM_IP_ADDR)],
                [sg.Text('Seq Cnt:',    size=tlm_label_size, font=hdr_label_font, pad=((5,0),(6,6))), 
                sg.Text('0',            size=tlm_label_size, font=hdr_value_font, key=mc.JSON_TLM_SEQ_CNT)],
                [sg.Text('Cmd Cnt:',    size=tlm_label_size, font=hdr_label_font, pad=((5,0),(6,6))), 
                sg.Text('0',            size=tlm_label_size, font=hdr_value_font, key=mc.JSON_TLM_CMD_CNT)],
                [sg.Text('Event:',      size=tlm_label_size, font=hdr_label_font, pad=((5,0),(6,6))), 
                sg.Text(mc.JSON_VAL_NONE,  size=(50,1),         font=hdr_value_font, key=mc.JSON_TLM_EVENT)],
                [sg.HorizontalSeparator()],
                [sg.Text('cFS Exe:',    size=tlm_label_size, font=hdr_label_font, pad=((5,0),(6,6))), 
                sg.Text('False',        size=tlm_label_size, font=hdr_value_font, key=mc.JSON_TLM_CFS_EXE)],
                [sg.Text('Apps:',       size=tlm_label_size, font=hdr_label_font, pad=((5,0),(6,6))), 
                sg.Text(mc.JSON_VAL_NONE,  size=(50,1),         font=hdr_value_font, key=mc.JSON_TLM_CFS_APPS)],
                [sg.HorizontalSeparator()],
                [sg.Text('Py Exe:',     size=tlm_label_size, font=hdr_label_font, pad=((5,0),(6,6))), 
                sg.Text('False',        size=tlm_label_size, font=hdr_value_font, key=mc.JSON_TLM_PY_EXE)],
                [sg.Text('Apps:',       size=tlm_label_size, font=hdr_label_font, pad=((5,0),(6,6))), 
                sg.Text(mc.JSON_VAL_NONE,  size=(50,1),      font=hdr_value_font, key=mc.JSON_TLM_PY_APPS)]])
            ]
            
        ]

        window = sg.Window('Control Target: %s'%self.target_mqtt_topic, layout, auto_size_text=True, finalize=True)
        return window


    def execute(self):
        
        self.client_connect();          
        self.window = self.create_window()
        
        start_remote_cfs_tlm   = False
        start_remote_cfs_timer = 0
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
                self.publish_cmd(mc.JSON_CMD_SUBSYSTEM_TARGET, mc.JSON_CMD_TARGET_NOOP)

            elif self.event == '-TARGET_REBOOT-':
                self.publish_cmd(mc.JSON_CMD_SUBSYSTEM_TARGET, mc.JSON_CMD_TARGET_REBOOT)
            
            elif self.event == '-TARGET_SHUTDOWN-':
                self.publish_cmd(mc.JSON_CMD_SUBSYSTEM_TARGET, mc.JSON_CMD_TARGET_SHUTDOWN)

            elif self.event == '-CFS_START-':
                self.publish_cmd(mc.JSON_CMD_SUBSYSTEM_CFS, mc.JSON_CMD_CFS_START)
                self.cfe_time_event_filter = False
                                 
            elif self.event == '-CFS_START_TLM-':
                enable_tlm = EnableTlm()
                tlm_type = enable_tlm.execute()
                if tlm_type == EnableTlm.SOCKET:
                    start_remote_cfs_tlm   = True
                    start_remote_cfs_timer = TargetControl.TIMER_SET_CFS_IP
                elif tlm_type == EnableTlm.MQTT:
                    start_remote_cfs_tlm   = True
                    start_remote_cfs_timer = 0

            elif self.event == '-CFS_STOP-':
                self.publish_cmd(mc.JSON_CMD_SUBSYSTEM_CFS, mc.JSON_CMD_CFS_STOP)

            elif self.event == '-PYTHON_START-':
                py_app_list = self.window[mc.JSON_TLM_PY_APPS].get()
                if py_app_list == mc.JSON_VAL_NONE:
                    sg.popup('No python apps on the remote target', title='Start Remote Python App')
                else:
                    py_app = self.select_py_app_gui(py_app_list, 'start')
                    if len(py_app) > 0:
                        self.publish_cmd(mc.JSON_CMD_SUBSYSTEM_PYTHON, mc.JSON_CMD_PYTHON_START, param=py_app)
                                 
            elif self.event == '-PYTHON_STOP-':
                py_app_list = self.window[mc.JSON_TLM_PY_APPS].get()
                if py_app_list == mc.JSON_VAL_NONE:
                    sg.popup('No python apps on the remote target', title='Stop Remote Python App')
                else:
                    py_app = self.select_py_app_gui(py_app_list, 'stop')
                    if len(py_app) > 0:
                        self.publish_cmd(mc.JSON_CMD_SUBSYSTEM_PYTHON, mc.JSON_CMD_PYTHON_STOP, param=py_app)

            elif self.event == '-PYTHON_LIST_APPS-':
                self.publish_cmd(mc.JSON_CMD_SUBSYSTEM_PYTHON, mc.JSON_CMD_PYTHON_LIST_APPS)
    
            # Perform processing that requires multiple event loop cycles
            if start_remote_cfs_tlm:
               start_remote_cfs_tlm = self.start_remote_cfs_tlm(start_remote_cfs_timer)
               start_remote_cfs_timer += 1
            
        self.window.close()
        
        
    def select_py_app_gui(self, py_app_list, action_text):
        """
        Select the python app to be started or stopped. The action text should be lower case.
        py_app_list is taken from the telemetry data so astericks may need to be remove for
        scripts that are executing.
        """
        selected_app = ''
        print(py_app_list)
        py_app_list = py_app_list.split(',')
        print(py_app_list)
        b_pad  = ((0,2),(2,2))
        b_font = ('Arial bold', 11)

        layout = [
                  [sg.Text(f'Select python app to {action_text} from the dropdown iist and click <Submit>\n', font=b_font)],
                  [sg.Combo(py_app_list, pad=b_pad, font=b_font, enable_events=True, key="-PYTHON_APP-", default_value=py_app_list[0]),
                   sg.Button('Submit', button_color=('SpringGreen4'), pad=b_pad, key='-SUBMIT-'),
                   sg.Button('Cancel', button_color=('gray'), pad=b_pad, key='-CANCEL-')]
                 ]      

        window = sg.Window('Select Remote Python App', layout, resizable=True, modal=True)
        
        while True:
        
            event, values = window.read(timeout=200)
        
            if event in (sg.WIN_CLOSED, '-CANCEL-') or event is None:
                break
                
            elif event == '-SUBMIT-':
                selected_app = values['-PYTHON_APP-'].strip().strip('*')
                break
        
        window.close()
        return selected_app

###############################################################################

if __name__ == '__main__':

    config = configparser.ConfigParser()
    config.read('../basecamp.ini')

    mission_name     = config.get('CFS_TARGET','MISSION_EDS_NAME')
    router_ctrl_port = config.getint('NETWORK','CMD_TLM_ROUTER_CTRL_PORT')
    
    gnd_ip_addr = config.get('NETWORK','GND_IP_ADDR')
    cmd_port    = config.getint('NETWORK','TARGET_CONTROL_CMD_PORT')

    broker_addr = config.get('NETWORK','MQTT_BROKER_ADDR')
    broker_port = config.get('NETWORK','MQTT_BROKER_PORT')
    print("Broker Address: %s, Port: %s" % (broker_addr, broker_port))
    
    client_name   = config.get('NETWORK','MQTT_CLIENT_NAME')
    remote_target_mqtt_topic = config.get('NETWORK','REMOTE_TARGET_MQTT_TOPIC')
    
    local_network_adapter = config.get('NETWORK','LOCAL_NET_ADAPTER')
    
    target_control = TargetControl(mission_name, gnd_ip_addr, router_ctrl_port, cmd_port, 
                                   remote_target_mqtt_topic, broker_addr, broker_port, client_name,
                                   local_network_adapter)
    target_control.execute()
    

