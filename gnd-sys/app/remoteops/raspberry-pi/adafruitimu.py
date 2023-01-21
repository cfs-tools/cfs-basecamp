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
sys.path.append('..')
import time
import logging
import configparser
import json
import subprocess
import queue

import board
from adafruit_lsm6ds.ism330dhcx import ISM330DHCX
import paho.mqtt.client as mqtt

from mqttconst import *
from remoteprocess import RemoteProcess

###############################################################################

class AdaFruitImu(RemoteProcess):

    
    def __init__(self, ini_file):
        super().__init__(ini_file)

        self.i2c = None
        self.control_delay = 0.5 # int(self.exec_config['CONTROL_DELAY'])
        
        self.client_name = f"{self.mqtt_config['TARGET_ID']}/{self.mqtt_config['SENSOR_ID']}"
        self.imu_rate_topic = f"{self.topic_base}/{self.mqtt_config['SENSOR_ID']}"

        self.log_info_event(f'AdaFruit IMU defaults {self.broker_addr}:{self.broker_port}//{self.imu_rate_topic}',queue_event=False)

    def on_connect(self, client, userdata, flags, rc):
        """
        """
        logging.info(f'AdaFruit IMU connected with result code {rc}')

    def execute(self):
        """
        Initialize I2C here as opposed to the constructor so a user can reconnect to hardware
        without retstarting the app
        """
        try:
            self.i2c = board.I2C()  # uses board.SCL and board.SDA
            self.sensor = ISM330DHCX(self.i2c)
            self.log_info_event("AdaFruit IMU board initialized")

            if self.connect(): 
                while True:
                    try:
                        self.publish_imu_data()
                        time.sleep(self.control_delay)
                    except KeyboardInterrupt:
                        sys.exit()
        except Exception as e:
            self.log_error_event(f'Error initializing I2C/IMU')
            self.log_error_event(f'Error: {e}')
                    
        
    def publish_imu_data(self):
        
        #print("Acceleration: X:%.2f, Y: %.2f, Z: %.2f m/s^2" % (self.sensor.acceleration))
        #print("Gyro X:%.2f, Y: %.2f, Z: %.2f radians/s\n" % (self.sensor.gyro))
        payload = '{ "coord": {"x": %2f, "y": %2f, "z": %2f} }' % \
                  (self.sensor.gyro[0], self.sensor.gyro[1], self.sensor.gyro[2])         
        print(f'Publishing telemetry {self.imu_rate_topic}, {payload}')
        self.client.publish(self.imu_rate_topic, payload)

############################################################################

if __name__ == "__main__":

    ini_file = os.path.join(os.getcwd(), 'adafruitimu.ini')
    adafruit_imu = AdaFruitImu(ini_file)
    adafruit_imu.execute()

