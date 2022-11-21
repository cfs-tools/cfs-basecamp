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
      Demonstrate managing a discrete device and sending the status
      in an MQTT message
        
    Notes:    
      1. The LED hardware configuration is described in Basecamp's
         GPIO_DEMO tutorial
       
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

import board
import RPi.GPIO as GPIO
import paho.mqtt.client as mqtt

from mqttconst import *

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
            self.client = mqtt.Client(self.client_name)
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

###############################################################################

class DiscreteDemo(MqttClient):

    
    def __init__(self, ini_file):
        super().__init__(config_parser['MQTT'])

        self.exec_config = config_parser['EXEC']
        self.mqtt_config = config_parser['MQTT']
        logging.basicConfig(filename=self.exec_config['LOG_FILE'],level=logging.DEBUG)
        
        
        self.control_delay = int(self.exec_config['CONTROL_DELAY'])
        self.led_on = True
        
        self.client_name = f"{self.mqtt_config['TARGET_ID']}/{self.mqtt_config['SENSOR_ID']}"
        self.discrete_tlm_topic  = f"{self.topic_base}/{self.mqtt_config['SENSOR_ID']}"

        self.log_info_event(f'Discrete Demo defaults {self.broker_addr}:{self.broker_port}//{self.discrete_tlm_topic}',queue_event=False)

    def on_connect(self, client, userdata, flags, rc):
        """
        """
        logging.info(f'Discrete Demo connected with result code {rc}')

    def execute(self):
        """
        Initialize GPIO here as opposed to the constructor so a user can reconnect to hardware
        without retstarting the app
        """
        GPIO.setwarnings(False)
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(18, GPIO.OUT)
        self.log_info_event('GPIO interface initialized')

        if self.connect(): 
            while True:
                try:
                    self.control_led()
                    time.sleep(self.control_delay)
                except KeyboardInterrupt:
                    sys.exit()

        
    def control_led(self):
        
        payload = '{ "discrete": {"item-1": %s, "item-2": %s, "item-3": %s, "item-4": %s} }' % \
                  (str(int(self.led_on)), '0', '0', '0')

        if self.led_on:
            GPIO.output(18, True)
            self.led_on = False
        else:
            GPIO.output(18, False)
            self.led_on = True
            
        self.client.publish(self.discrete_tlm_topic, payload)

############################################################################

if __name__ == "__main__":

    ini_file = os.path.join(os.getcwd(), 'discretedemo.ini')
    config_parser = configparser.ConfigParser()
    config_parser.read(ini_file)

    discrete_demo = DiscreteDemo(config_parser)
    discrete_demo.execute()
