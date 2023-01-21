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
from remoteprocess import RemoteProcess

###############################################################################

class DiscreteDemo(RemoteProcess):

    
    def __init__(self, ini_file):
        super().__init__(ini_file)

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
        
        payload = '{ "integer": {"item-1": %s, "item-2": %s, "item-3": %s, "item-4": %s} }' % \
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
    discrete_demo = DiscreteDemo(ini_file)
    discrete_demo.execute()
