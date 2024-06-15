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
      Generate simulated rate data and publish it in an MQTT message
        
    Notes:    
      None
       
"""
import os
import sys
sys.path.append('..')
import time
import logging
import configparser
import json
import subprocess
import queue

import paho.mqtt.client as mqtt

from mqttconst import *

X_AXIS = 0
Y_AXIS = 1
Z_AXIS = 2

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
            self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, self.client_name)
            self.client.on_connect = self.on_connect        # Callback function for successful connection
            self.client.on_message = self.process_cmd_stub  # Callback function for receipt of a message
            self.client.connect(self.broker_addr)
            self.client.loop_start()  # Start networking daemon             
            self.log_info_event(f'Client initialized on {self.broker_addr}:{self.broker_port}')
            connect = True
        except Exception as e:
            self.log_error_event(f'Client initialization error for {self.broker_addr}:{self.broker_port}')
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

###############################################################################

class HelloWorld(MqttClient):
    
    def __init__(self, ini_file):
        super().__init__(config_parser['MQTT'])

        self.exec_config = config_parser['EXEC']
        self.mqtt_config = config_parser['MQTT']
        log_filename = os.path.join(os.getcwd(), self.exec_config['LOG_FILE'])
        logging.basicConfig(filename=log_filename,level=logging.DEBUG)
        
        self.client_name = f"{self.mqtt_config['TARGET_ID']}/{self.mqtt_config['SENSOR_ID']}"
        self.rate_topic = f"{self.topic_base}/{self.mqtt_config['SENSOR_ID']}"

        self.log_info_event(f'Hello World MQTT: {self.broker_addr}:{self.broker_port}//{self.rate_topic}',queue_event=False)

        # Rotate 90deg in 5 seconds
        self.current_axis       = X_AXIS
        self.sim_cycle_time_sec = 1.0
        self.sim_cycle_axis_cnt = 1
        self.sim_cycle_axis_lim = 5.0  # 5 seconds 
        self.sim_axis_rate_rad  = 90.0 / self.sim_cycle_axis_lim * 0.0174533
        self.sim_rate           = [ 0.0, 0.0, 0.0]


    def on_connect(self, client, userdata, flags, reason_code, properties):
        """
        """
        if reason_code == 0:
            logging.info(f'Hello World successfuly connected on {self.broker_addr}')
        if reason_code > 0:
            logging.error(f'Hello World connection error with reason_code {reason_code}')


    def execute(self):
        """
        """
        if self.connect(): 
            while True:
                self.sim_step()
                try:
                    self.publish_rate_data()
                    time.sleep(self.sim_cycle_time_sec)
                except KeyboardInterrupt:
                    sys.exit()

                    
    def sim_step(self):
        if self.current_axis == X_AXIS:
            self.sim_cycle_axis_cnt += 1
            if self.sim_cycle_axis_cnt > self.sim_cycle_axis_lim:
                self.sim_cycle_axis_cnt = 1
                self.sim_rate[X_AXIS] = 0.0
                self.sim_rate[Y_AXIS] = self.sim_axis_rate_rad
                self.current_axis = Y_AXIS
        elif self.current_axis == Y_AXIS:
            self.sim_cycle_axis_cnt += 1
            if self.sim_cycle_axis_cnt > self.sim_cycle_axis_lim:
                self.sim_cycle_axis_cnt = 1
                self.sim_rate[Y_AXIS] = 0.0
                self.sim_rate[Z_AXIS] = self.sim_axis_rate_rad
                self.current_axis = Z_AXIS
        elif self.current_axis == Z_AXIS:
            self.sim_cycle_axis_cnt += 1
            if self.sim_cycle_axis_cnt > self.sim_cycle_axis_lim:
                self.sim_cycle_axis_cnt = 1
                self.sim_rate[Z_AXIS] = 0.0
                self.sim_rate[X_AXIS] = self.sim_axis_rate_rad
                self.current_axis = X_AXIS
        else:
            self.sim_cycle_axis_cnt = 1
            self.sim_rate[X_AXIS] = self.sim_axis_rate_rad
            self.sim_rate[Y_AXIS] = 0.0
            self.sim_rate[Z_AXIS] = 0.0
            self.current_axis = X_AXIS


    def publish_rate_data(self):        
        payload = '{ "rate": {"x": %2f, "y": %2f, "z": %2f}}' % \
                  (self.sim_rate[X_AXIS], self.sim_rate[Y_AXIS], self.sim_rate[Z_AXIS])         
        print(f'Publishing telemetry {self.rate_topic}, {payload}')
        self.client.publish(self.rate_topic, payload)


############################################################################

if __name__ == "__main__":

    ini_file = os.path.join(os.getcwd(), 'helloworld.ini')
    config_parser = configparser.ConfigParser()
    config_parser.read(ini_file)

    hello_world = HelloWorld(config_parser)
    hello_world.execute()

