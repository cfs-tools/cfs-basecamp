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
      Read AdaFruit IMU data and publish it in an MQTT message
        
    Notes:    
      1. TODO: Note supported versions
       
"""
import os
import sys
import logging
import configparser
import queue

import paho.mqtt.client as mqtt

if __name__ == '__main__' or 'remoteops' in os.getcwd():
    from mqttconst import *
else:
    from .mqttconst import *


###############################################################################

class RemoteProcess():
    """
    Provides basic functionality required by all remote target processes:
    - Ini file 
    - MQTT client
    - Message logging
    """
    def __init__(self, ini_file):
        self.config_parser = configparser.ConfigParser()
        self.config_parser.read(ini_file)
        self.exec_config = self.config_parser['EXEC']
        self.mqtt_config = self.config_parser['MQTT']
        logging.basicConfig(filename=self.exec_config['LOG_FILE'],level=logging.DEBUG)

        self.broker_addr = self.mqtt_config['BROKER_ADDR']
        self.broker_port = self.mqtt_config['BROKER_PORT']
        self.client_name = f"{self.mqtt_config['TARGET_ID']}"
        self.topic_base  = f"{MQTT_TOPIC_ROOT}/{self.mqtt_config['TARGET_ID']}"
        self.client = None
        self.event_msg = ''
        self.event_queue = queue.Queue()
        
    def connect(self):
        connect = False
        try:
            self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, self.client_name)
            self.client.on_connect = self.on_connect        # Callback function for successful connection
            self.client.on_message = self.process_cmd_stub  # Callback function for receipt of a message
            self.client.connect(self.broker_addr)
            self.client.loop_start()  # Start networking daemon             
            self.log_info_event(f'Client initialized on {self.broker_addr}:{self.broker_port}')
            connect = True
        except Exception as e:
            self.log_error_event(f'Client initializaation error for {self.broker_addr}:{self.broker_port}')
            self.log_error_event(f'Error: {e}')
        return connect

    def log_info_event(self, msg_str, queue_event=False):
        logging.info(msg_str)
        if queue_event:
            self.event_queue.put_nowait(msg_str)
        print(msg_str)
      
    def log_error_event(self, msg_str, queue_event=False):
        logging.error(msg_str)
        if queue_event:
            self.event_queue.put_nowait(msg_str)
        print(msg_str)
    
    def process_cmd_stub(self, client, userdata, msg):
        """
        No input messages are expected so simply log what is received
        """
        msg_str = msg.payload.decode()
        msg_str_single_quote = msg_str.replace('"',"'")
        self.log_info_event(f'Received message : {msg.topic}=>{msg_str_single_quote}')

############################################################################

if __name__ == "__main__":

    ini_file = os.path.join(os.getcwd(), 'adafruitimu.ini')
    adafruit_imu = AdaFruitImu(ini_file)
    adafruit_imu.execute()

