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
      Provide classes that manage the MQTT interface to python apps
      and cFS targets
        
    Notes:    
      None
       
"""
import os
import time
import threading
import logging
import configparser
import json
import subprocess
import queue
import socket
import fcntl
import struct
import sys
from datetime import datetime
import paho.mqtt.client as mqtt

from mqttconst import *

SH_STOP_CFS  = './stop_cfs.sh'
SH_START_CFS = './start_cfs.sh'

###############################################################################

class MqttClient():

    def __init__(self, mqtt_config):
        self.broker_addr = mqtt_config['BROKER_ADDR']
        self.broker_port = mqtt_config['BROKER_PORT']
        self.client_name = f"{mqtt_config['TARGET_ID']}"
        self.topic_base  = f"{MQTT_TOPIC_ROOT}/{mqtt_config['TARGET_ID']}"
        self.client = None
        self.event_msg = ''
        self.event_queue = queue.Queue()
        
    def connect(self):
        connect = False
        try:
            self.client = mqtt.Client(mqtt.CallbackAPIVersion.API_VERSION2, self.client_name)
            self.client.on_connect = self.on_connect   # Callback function for successful connection
            self.client.on_message = self.process_cmd  # Callback function for receipt of a message
            self.client.connect(self.broker_addr)
            self.client.loop_start()  # Start networking daemon             
            self.log_info_event(f'Client initialized on {self.broker_addr}:{self.broker_port}')
            connect = True
        except Exception as e:
            self.log_error_event(f'Client connection error for {self.broker_addr}:{self.broker_port}')
            self.log_error_event(f'Error: {e}')
        return connect

    def log_info_event(self, msg_str, queue_event=True):
        logging.info(msg_str)
        if queue_event:
            self.event_queue.put_nowait(msg_str)
        print(msg_str)
      
    def log_error_event(self, msg_str, queue_event=True):
        logging.error(msg_str)
        if queue_event:
            self.event_queue.put_nowait(msg_str)
        print(msg_str)

###############################################################################

class RemoteOps(MqttClient):


    def __init__(self, config_parser):
        super().__init__(config_parser['MQTT'])
        
        self.exec    = config_parser['EXEC']
        self.apps    = config_parser['APPS']
        self.network = config_parser['NETWORK']
        log_filename = os.path.join(os.getcwd(), self.exec['LOG_FILE'])
        logging.basicConfig(filename=log_filename,level=logging.DEBUG)

        self.ip_addr = self.get_ip_address(self.network['LOCAL_NET_ADAPTER'])

        self.cmd_topic  = f'{self.topic_base}/{MQTT_TOPIC_CMD}'
        self.tlm_topic  = f'{self.topic_base}/{MQTT_TOPIC_TLM}'
  
        # List of apps not to be included in user cFS app list
        self.base_camp_apps = ['ASSERT_LIB','APP_C_FW','CI_LAB_APP',
                               'TO_LAB_APP','SCH_LAB_APP','FILE_MGR',
                               'FILE_XFER', 'KIT_SCH', 'KIT_TO', 'APP_C_DEMO']
  
        self.json_cmd_subsystems = [JSON_CMD_SUBSYSTEM_CFS,
                                    JSON_CMD_SUBSYSTEM_PYTHON,
                                    JSON_CMD_SUBSYSTEM_TARGET]

        self.cfs_cmd    = { JSON_CMD_CFS_START:   self.cfs_start_cmd,
                            JSON_CMD_CFS_STOP:    self.cfs_stop_cmd,
                            JSON_CMD_CFS_ENA_TLM: self.cfs_ena_tlm_cmd}

        self.python_cmd = { JSON_CMD_PYTHON_LIST_APPS: self.python_list_apps_cmd,
                            JSON_CMD_PYTHON_START:     self.python_start_cmd,
                            JSON_CMD_PYTHON_STOP:      self.python_stop_cmd}

        self.target_cmd = { JSON_CMD_TARGET_NOOP:     self.target_noop_cmd,
                            JSON_CMD_TARGET_REBOOT:   self.target_reboot_cmd,
                            JSON_CMD_TARGET_SHUTDOWN: self.target_shutdown_cmd }

        self.cmd = { JSON_CMD_SUBSYSTEM_CFS:    self.cfs_cmd,
                     JSON_CMD_SUBSYSTEM_PYTHON: self.python_cmd,
                     JSON_CMD_SUBSYSTEM_TARGET: self.target_cmd }
        self.cmd_processed = ""

        self.cfs_process = None
        self.python_process = {}

        self.python_exe_cnt = 0
        self.python_exe_app = {}
        
        # Telemetry data
        self.tlm_delay = int(self.exec['TLM_DELAY'])
        self.tlm_seq_cnt = 0
        self.cmd_cnt     = 0
        self.cfs_exe     = False
        self.cfs_apps    = JSON_VAL_NONE
        self.python_exe  = False
        self.python_apps = JSON_VAL_NONE
        
        self.python_app_path = os.path.join(os.getcwd(), self.apps['PYTHON_PATH'])
        self.create_python_app_str(self.apps['PYTHON_APPS'])
        self.log_info_event(f'Remote Ops defaults {self.broker_addr}:{self.broker_port}//{self.topic_base}',queue_event=False)


    def get_ip_address(self, ifname):
        """
        Can't use "socket.gethostbyname(socket.gethostname())"
        because it will return something like 127.0.1.1
        """
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        addr = socket.inet_ntoa(fcntl.ioctl(
            s.fileno(),
            0x8915,  # SIOCGIFADDR
            struct.pack('256s', ifname[:15].encode('utf-8'))
        )[20:24])
        print (f'IP Adress: {addr}')
        return addr

    def on_connect(self, client, userdata, flags, reason_code, properties):
        """
        """
        if reason_code == 0:
            logging.info(f'Remote Ops connected with reason_code {reason_code}')
            self.client.subscribe(self.cmd_topic)
        if reason_code > 0:
            logging.error(f'Remote Ops connection error with reason_code {reason_code}')
 
 
    def send_tlm(self):
        """
        """
        if not self.event_queue.empty():
            self.event_msg = self.event_queue.get_nowait()
            
        payload = '{"%s": "%s", "%s": %s, "%s": %s, "%s": "%s", "%s": "%s", "%s": "%s", "%s": "%s", "%s": "%s"}' % \
                  (JSON_TLM_IP_ADDR,  self.ip_addr, \
                   JSON_TLM_SEQ_CNT,  str(self.tlm_seq_cnt), \
                   JSON_TLM_CMD_CNT,  str(self.cmd_cnt), \
                   JSON_TLM_EVENT,    self.event_msg, \
                   JSON_TLM_CFS_EXE,  str(self.cfs_exe), \
                   JSON_TLM_CFS_APPS, self.cfs_apps, \
                   JSON_TLM_PY_EXE,   str(self.python_exe), \
                   JSON_TLM_PY_APPS,  self.python_apps)
        #print("Publishing telemetry %s, %s" % (self.tlm_topic, payload))
        self.client.publish(self.tlm_topic, payload)
        self.tlm_seq_cnt += 1

 
    def process_cmd(self, client, userdata, msg):
        """
        The callback for when a PUBLISH message is received from the server. The payload is
        a JSON command object with the following fields:
                 
            {
              "[cfs | python | target]": "<command>",
              "parameter": "command specific string"
            }
  
        remoteopsconst.py has a complete definition.
        Most commands don't have a parameter.
        """
        msg_str = msg.payload.decode()
        msg_str_single_quote = msg_str.replace('"',"'")
        self.log_info_event(f'Received message : {msg.topic}=>{msg_str_single_quote}',queue_event=False)
        cmd = json.loads(msg_str)
        for key in cmd:
            if key in self.json_cmd_subsystems:
                param = ''
                if JSON_CMD_PARAMETER in cmd:
                    param = cmd[JSON_CMD_PARAMETER]
                    print (f'param = {param}')
                try:
                    self.cmd_processed = cmd[key]
                    self.cmd[key][cmd[key]](param)
                    self.cmd_cnt += 1
                except Exception as e:
                    self.log_error_event(f'Error executing command: {key}:{cmd[key]}')
                    self.log_error_event(f'Error: {e}')
                break
            else:
                self.log_error_event(f'Received invalid command {key}')
      
    def execute(self):
        if self.connect(): 
            while True:
                try:
                    self.send_tlm()
                    time.sleep(self.tlm_delay)
                except KeyboardInterrupt:
                    sys.exit()

    def cmd_stub(self, param):
        self.log_info_event(f'Command stub called for {self.cmd_processed}')

    ###########
    ### cFS ###
    ###########
    
    def cfs_start_cmd(self, param):
        try:
            if self.cfs_exe:
                self.log_info_event("Start cFS rejected, cFS already running")
            else:
                #cfs_path = os.path.join(os.getcwd(), self.apps['CFS_PATH'])
                #self.cfs_process = subprocess.Popen(['sudo',self.apps['CFS_BINARY']],
                #                                    cwd=cfs_path,
                #                                    stdout=subprocess.PIPE,
                #                                    stderr=subprocess.STDOUT,
                #                                    shell=False)
                start_sh     = os.path.join(os.getcwd(), SH_START_CFS)
                cfs_path     = os.path.join(os.getcwd(), self.apps['CFS_PATH'])
                cfs_bin_file = self.apps['CFS_BINARY']
                password     = self.exec['PASSWORD']
                popen_str = f'{start_sh} {cfs_path} {cfs_bin_file} {password}'
                print(f'Start popen_str: {popen_str}')
                self.cfs_process = subprocess.Popen(popen_str, stdout=subprocess.PIPE, shell=True, 
                                   bufsize=1, universal_newlines=True,
                                   preexec_fn = lambda : (os.setsid(), os.nice(10)))

                self.create_cfs_app_str(cfs_path)
                self.cfs_exe = True
                self.log_info_event(f'Start cFS, pid = {self.cfs_process.pid}')
               
        except (OSError, subprocess.CalledProcessError) as e:
            self.log_error_event(f'Start cFS failed with exception')
            self.log_error_event(f'Exception: {str(e)}')


    def cfs_stop_cmd(self, param):
        if self.cfs_exe:
            self.cfs_apps = ''
            self.log_info_event(f'Stopping cFS, pid = {self.cfs_process.pid}')
            stop_sh   = os.path.join(os.getcwd(), SH_STOP_CFS)
            password  = self.exec['PASSWORD']
            popen_str = f'{stop_sh} {password}'
            print(f'Stop popen_str: {popen_str}')
            self.cfs_process = subprocess.Popen(popen_str, stdout=subprocess.PIPE, shell=True)
            #subprocess.run(['sudo', 'kill', f'{self.cfs_process.pid}']) #'-9', 
            #todo: self.cfs_process.kill()
            #self.cfs_process.wait()
            self.cfs_exe = False
        else:
            self.log_error_event('Stop cFS rejected, cFS not running')
 

    def cfs_ena_tlm_cmd(self, param):
        self.log_info_event('Enable telemetry not implemented')

    def create_cfs_app_str(self, target_path):
        self.cfs_apps = ''
        startup_path_file = os.path.join(target_path, 'cf', 'cfe_es_startup.scr')
        print(f'startup_path_file: {startup_path_file}')
        try:
            with open(startup_path_file) as f:
                startup_file = f.readlines()
          
            first_app = True
            for startup_line in startup_file:
                line = startup_line.split(',')
                if '!' in line[0]:
                    break
                else:
                    app = line[3].strip()
                    if app not in self.base_camp_apps:
                        if first_app:
                            self.cfs_apps += app
                            first_app = False
                        else:
                            self.cfs_apps += ', ' + app
            if self.cfs_apps == '':
               self.cfs_apps = 'No user apps'
        except Exception as e:
            self.log_error_event(f'Error creating app list')
            self.log_error_event(f'Exception: {str(e)}')
            self.cfs_apps = 'Error creating app list'

    ##############
    ### Python ###
    ##############
    
    def python_list_apps_cmd(self, param):
        self.log_info_event('List apps not implemented')

    def python_start_cmd(self, param):
        """
        Mark an executing app with an asterick
        """
        try:
            if self.python_exe_app[param]:
                self.log_info_event(f'Start command rejected, {param}.py already running')
            else:
                # Using 'exec' allows the kill() to work with shell=True
                self.python_process[param] = subprocess.Popen(
                    f'exec python3 {param}.py', 
                    cwd=self.python_app_path,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    shell=True)
                self.python_exe = True
                self.python_exe_app[param] = True
                self.python_exe_cnt += 1
                self.python_apps = self.python_apps.replace(param,f'{param}*')
                self.log_info_event(f'Started {param}.py, pid = {self.python_process[param].pid}')
               
        except Exception as e:
            self.log_error_event(f'Start {param}.py failed with exception')
            self.log_error_event(f'Exception: {str(e)}')


    def python_stop_cmd(self, param):
        try:
            if self.python_exe_app[param]:
                if param in self.python_process:
                    pid = self.python_process[param].pid
                    self.log_info_event(f'Stopping {param}.py, pid = {pid}')
                    #todo: subprocess.run(['sudo', 'kill', '-9', f'{pid}'])
                    self.python_process[param].kill()
                    self.python_process[param].wait()
                    self.python_exe_app[param] = False
                    self.python_exe_cnt -= 1
                    if self.python_exe_cnt == 0:
                        self.python_exe = False
                    self.python_apps = self.python_apps.replace(f'{param}*',param)
                    self.log_info_event(f'Stopped {param}.py')
                else:
                    self.log_error_event(f'Stop rejected, no process ID for {param}.py')
            else:
                self.log_error_event(f'Stop rejected, {param}.py not running')
        except Exception as e:
            self.log_error_event(f'Stop {param}.py failed with exception')
            self.log_error_event(f'Exception: {str(e)}')
            
    def create_python_app_str(self, app_list):
        """
        app_list is comma separate list of python script names without the
        '.py' extension.
        Add one space per app regardless of how the initial list is defined
        """
        self.python_apps = ''
        first_app = True
        app_list = app_list.split(',')
        for app in app_list:
            app = app.strip()
            if first_app:
                self.python_apps += app
                first_app = False
            else:
                self.python_apps += ', ' + app
            self.python_exe_app[app] = False
            
    ##############
    ### Target ###
    ##############
    
    def target_noop_cmd(self, param):
        timestamp = datetime.now().strftime("%m/%d/%Y-%H:%M:%S")
        self.log_info_event(f'Noop received at {timestamp}')
        self.log_info_event(f'Noop status: {self.broker_addr}:{self.broker_port}//{self.topic_base}')
        
    def target_reboot_cmd(self, param):
        self.log_info_event('Rebooting target')
        subprocess.Popen(self.exec['REBOOT_CMD'], shell=False)
        
    def target_shutdown_cmd(self, param):
        self.log_info_event('Shutting down target')
        subprocess.Popen(self.exec['HALT_CMD'], shell=False)
        
############################################################################

if __name__ == "__main__":

    ini_file = os.path.join(os.getcwd(), 'remoteops.ini')
    config_parser = configparser.ConfigParser()
    config_parser.read(ini_file)
    remote_ops = RemoteOps(config_parser)
    remote_ops.execute()