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
      Provide classes that manage the MQTT interface to sensors
        
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

import board
from adafruit_lsm6ds.ism330dhcx import ISM330DHCX
import paho.mqtt.client as mqtt

import mqttconst as mqttc

###############################################################################

class MqttClient():

    def __init__(self, mqtt_config):
        self.broker_addr = mqtt_config['BROKER_ADDR']
        self.broker_port = mqtt_config['BROKER_PORT']
        self.client_name = f"{mqtt_config['TARGET_TYPE']}-{mqtt_config['TARGET_ID']}"
        self.topic_base  = f"{mqttc.MQTT_TOPIC_ROOT}/{mqtt_config['TARGET_TYPE']}/{mqtt_config['TARGET_ID']}"
        self.client = None
        self.event_msg = ""
        self.event_queue = queue.Queue()
        
    def connect(self):
        connect = False
        try:
            self.client = mqtt.Client(self.client_name)
            self.client.on_connect = self.on_connect   # Callback function for successful connection
            self.client.on_message = self.process_cmd  # Callback function for receipt of a message
            self.client.connect(self.broker_addr)
            self.client.loop_start()  # Start networking daemon             
            self.log_info_event(f'Client initialized on {self.broker_addr}:{self.broker_port}')
            connect = True
        except:
            self.log_error_event(f'Client initializaation error for {self.broker_addr}:{self.broker_port}')
        return connect

    def log_info_event(self, msg_str):
        logging.info(msg_str)
        self.event_queue.put_nowait(msg_str)
        print(msg_str)
      
    def log_error_event(self, msg_str):
        logging.error(msg_str)
        self.event_queue.put_nowait(msg_str)
        print(msg_str)

###############################################################################

class RemoteOps(MqttClient):


    def __init__(self, config_parser):
        super().__init__(config_parser['MQTT'])
        
        self.exec = config_parser['EXEC']
        self.apps = config_parser['APPS']
        logging.basicConfig(filename=self.exec['LOG_FILE'],level=logging.DEBUG)

        self.cmd_topic  = f'{self.topic_base}/{mqttc.MQTT_TOPIC_CMD}'
        self.tlm_topic  = f'{self.topic_base}/{mqttc.MQTT_TOPIC_TLM}'
  
        # List of apps not to be included in user cFS app list
        self.base_camp_apps = ['ASSERT_LIB','OSK_C_FW','CI_LAB_APP',
                               'TO_LAB_APP','SCH_LAB_APP','FILE_MGR',
                               'FILE_XFER', 'KIT_SCH', 'KIT_TO', 'OSK_C_DEMO']
  
        self.json_cmd_subsystems = [mqttc.JSON_CMD_SUBSYSTEM_CFS,
                                    mqttc.JSON_CMD_SUBSYSTEM_PYTHON,
                                    mqttc.JSON_CMD_SUBSYSTEM_TARGET]

        self.cfs_cmd    = { mqttc.JSON_CMD_CFS_START:   self.cfs_start_cmd,
                            mqttc.JSON_CMD_CFS_STOP:    self.cfs_stop_cmd,
                            mqttc.JSON_CMD_CFS_ENA_TLM: self.cfs_ena_tlm_cmd}

        self.python_cmd = { mqttc.JSON_CMD_PYTHON_LIST_APPS: self.python_list_apps_cmd,
                            mqttc.JSON_CMD_PYTHON_START:     self.python_start_cmd,
                            mqttc.JSON_CMD_PYTHON_STOP:      self.python_stop_cmd}

        self.target_cmd = { mqttc.JSON_CMD_TARGET_NOOP:     self.target_noop_cmd,
                            mqttc.JSON_CMD_TARGET_REBOOT:   self.target_reboot_cmd,
                            mqttc.JSON_CMD_TARGET_SHUTDOWN: self.target_shutdown_cmd }

        self.cmd = { mqttc.JSON_CMD_SUBSYSTEM_CFS:    self.cfs_cmd,
                     mqttc.JSON_CMD_SUBSYSTEM_PYTHON: self.python_cmd,
                     mqttc.JSON_CMD_SUBSYSTEM_TARGET: self.target_cmd }
        self.cmd_processed = ""

        # Telemetry data
        self.tlm_delay = int(self.exec['TLM_DELAY'])
        self.tlm_seq_cnt = 0
        self.cmd_cnt = 0
        self.cfs_exe  = False
        self.cfs_apps = 'None'
        self.py_exe   = False
        self.py_apps  = 'None'
        
        self.log_info_event(f'Remote Ops initialized on {self.broker_addr}:{self.broker_port}//{self.topic_base}')


    def on_connect(self, client, userdata, flags, rc):
        """
        """
        logging.info(f'Remote Ops connected with result code {rc}')
        self.client.subscribe(self.cmd_topic)
 
 
    def send_tlm(self):
        """
        """
        if not self.event_queue.empty():
            self.event_msg = self.event_queue.get_nowait()
            
        payload = '{"%s": %s, "%s": %s, "%s": "%s", "%s": "%s", "%s": "%s", "%s": "%s", "%s": "%s"}' % \
                  (mqttc.JSON_TLM_SEQ_CNT,  str(self.tlm_seq_cnt), \
                   mqttc.JSON_TLM_CMD_CNT,  str(self.cmd_cnt), \
                   mqttc.JSON_TLM_EVENT,    self.event_msg, \
                   mqttc.JSON_TLM_CFS_EXE,  str(self.cfs_exe).lower(), \
                   mqttc.JSON_TLM_CFS_APPS, self.cfs_apps, \
                   mqttc.JSON_TLM_PY_EXE,   str(self.py_exe).lower(), \
                   mqttc.JSON_TLM_PY_APPS,  self.py_apps)
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
        self.log_info_event(f'Received message : {msg.topic}=>{msg_str_single_quote}')
        cmd = json.loads(msg_str)
        for key in cmd:
            if key in self.json_cmd_subsystems:
                try:
                    self.cmd_processed = cmd[key]
                    self.cmd[key][cmd[key]]()
                    self.cmd_cnt += 1
                except Exception as e:
                    self.log_error_event(f'Error executing command: {key}:{cmd[key]}')
                    self.log_error_event(f'Error: {e}')
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

    def cmd_stub(self):
        self.log_info_event(f'Command stub called for {self.cmd_processed}')

    ###########
    ### cFS ###
    ###########
    def cfs_start_cmd(self):
        try:
            if self.cfs_exe:
                self.log_info_event("Start cFS rejected, cFS already running")
            else:
                print('1')
                self.create_cfs_app_str(self.apps['CFS_PATH'])
                print('2')
                self.cfs_process = subprocess.Popen(['sudo',self.apps['CFS_BINARY']],
                                                    cwd=self.apps['CFS_PATH'],
                                                    stdout=subprocess.PIPE,
                                                    stderr=subprocess.STDOUT,
                                                    shell=False)
                print('3')
                self.cfs_exe = True
                print('4')
                self.log_info_event(f'Start cFS, pid = {self.cfs_process.pid}')
               
        except (OSError, subprocess.CalledProcessError) as e:
            self.log_error_event(f'Start cFS failed with exception')
            self.log_error_event(f'Exception: {str(e)}')


    def cfs_stop_cmd(self):
        if self.cfs_exe:
            self.cfs_apps = ''
            self.log_info_event(f'Stopping cFS, pid = {self.cfs_process.pid}')
            subprocess.call(['sudo', 'kill', '-9', f'{self.cfs_process.pid}'])
            self.cfs_process.wait()
            self.cfs_exe = False
        else:
            self.log_error_event('Stop cFS rejected, cFS not running')
 

    def cfs_ena_tlm_cmd(self):
        self.log_info_event('Enable telemetry not implemented')

    def create_cfs_app_str(self, target_path):
        self.cfs_apps = ""
        startup_path_file = os.path.join(target_path, 'cf', 'cfe_es_startup.scr')
        print(startup_path_file)
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
        except Exception as e:
            self.log_error_event(f'Error creating app list')
            self.log_error_event(f'Exception: {str(e)}')
            self.cfs_apps = 'Error creating app list'

    ##############
    ### Python ###
    ##############
    def python_list_apps_cmd(self):
        self.log_info_event('List apps not implemented')

    def python_start_cmd(self):
        try:
            if self.python_exe:
                self.log_info_event('Start command rejected, python already running')
            else:
                #todo self.mqtt_sensor.execute()
                self.python_exe = True
                self.apps = self.mqtt_sensor.SENSORS
                self.log_info_event('Started python app')
               
        except Exception as e:
            self.log_error_event(f'Start python app failed with exception')
            self.log_error_event(f'Exception: {str(e)}')


    def python_stop_cmd(self):
        if self.python_exe:
            #todo self.mqtt_sensor.terminate()
            self.python_exe = False
            self.apps = ""
            self.log_info_event('Stopped python app')
        else:
            self.log_error_event('Stop rejected, python app not running')

    ##############
    ### Target ###
    ##############
    def target_noop_cmd(self):
        self.log_info_event(f'NOOP {self.broker_addr}:{self.broker_port}//{self.topic_base}')
 
    def target_reboot_cmd(self):
        self.log_info_event('Rebooting target')
        subprocess.Popen('reboot', shell=False)
        
    def target_shutdown_cmd(self):
        self.log_info_event('Shutting down target')
        subprocess.Popen('halt', shell=False)
        
############################################################################

if __name__ == "__main__":

    ini_file = os.path.join(os.getcwd(), 'remoteops.ini')
    config_parser = configparser.ConfigParser()
    config_parser.read(ini_file)
    remote_ops = RemoteOps(config_parser)
    remote_ops.execute()
