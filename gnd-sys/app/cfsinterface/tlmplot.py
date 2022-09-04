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
      Plot a single telemetry data item in a linear plot
      
    Notes:
      1. This is a simple single integer plot utility that only supports
         linear time plots. If more sophisticated plotting becomess
         necessary then a package such as matplot will be used.      
"""

import sys
import time
import os
import socket
import configparser
import io
import math
from contextlib import redirect_stdout
import PySimpleGUI as sg
import numpy as np

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

CCSDS   = 0
TIME    = 1
PAYLOAD = 2

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

        # Debug to help determine how to structure current value data       
        topics = self.tlm_server.get_topics()
        for topic in topics:
            #if topic != self.tlm_server.eds_mission.TOPIC_TLM_TITLE_KEY:
            if 'OSK_C_DEMO' in topic:
                print('***********topic: ' + str(topic))
                eds_id = self.tlm_server.eds_mission.get_eds_id_from_topic(topic)
                tlm_entry = self.tlm_server.eds_mission.get_database_entry(eds_id)
                tlm_obj = tlm_entry()
                print('***********tlm_entry = ' + str(tlm_obj))
                print('>>>> CCSDS: = ')
                for entry in tlm_obj.CCSDS:
                    print(str(entry))
                print('>>>> Sec: = ')
                for entry in tlm_obj.Sec:
                    print(str(entry))
                print('>>>> Payload: = ')
                for entry in tlm_obj.Payload:
                    print(str(entry))

    def update(self, tlm_msg: TelemetryMessage) -> None:
        """
        Receive telemetry updates
        """
        self.data_callback(tlm_msg)


###############################################################################

class TlmPlot():
    """
    Manage a linear plot that can plot up to self.plot_x_range data points. The
    update rate depends upon the telemetry point that is being plotted to the
    x-axis doesn't have a time scale, it is simple a data sample count
    
    The y-axis is scaled based on the data min/max values and the number of
    points on the gragh
    
    self.tlm_server.get_tlm_val(app_name, tlm_msg_name, parameter)
        Example current value usage: get_tlm_val("CFE_ES", "HK_TLM", "Sequence")
    """
    def __init__(self, gnd_ip_addr, tlm_port, tlm_timeout, min_value, max_value):

        self.tlm_server = TelemetrySocketServer('samplemission', 'cpu1', gnd_ip_addr, tlm_port, tlm_timeout)

        self.app_name    = ''
        self.tlm_topic   = ''
        self.tlm_payload = ''
        self.tlm_element = ''

        self.plot_x_range = 100
        self.plot_y_range = 100
      
        self.data_points  = 40
        self.data_max_idx = self.data_points-1

        self.data_min_value = min_value
        self.data_max_value = max_value
        self.data_range     = max_value - min_value
        
        self.y_data = np.zeros(self.data_points)
        self.linear_space = np.linspace(self.data_min_value, self.data_max_value, num=self.data_points, dtype=int)

        # Create 5 tick marks per axis
        self.x_tick_step       = int(self.plot_x_range/5)
        self.x_tick_step_value = int(self.data_points/5)
        self.x_scale_factor    = self.plot_x_range/self.data_points
        self.y_tick_step       = int(self.plot_y_range/5)
        self.y_tick_step_value = self.data_range/5.0
        self.y_scale_factor    = self.plot_y_range/self.data_range

    def create_window(self, title):
        """
        """
        sg.theme('LightGreen')
        layout = [[sg.Graph(canvas_size=(600, 600),
                   graph_bottom_left=(-20, -20),
                   graph_top_right=(110, 110),
                   key='graph')]]

        self.window = sg.Window(title, layout, grab_anywhere=True, finalize=True)
        self.graph = self.window['graph']
        self.draw_axes()
        self.draw_plot()

    def draw_axes(self):
        self.graph.DrawLine((0, 0), (self.plot_x_range, 0))
        self.graph.DrawLine((0, 0), (0, self.plot_y_range))
        
        i = 1
        for x in range(0, self.plot_x_range+1, self.x_tick_step):
            self.graph.DrawLine((x, -3), (x, 3))
            if x != 0:
                text = self.x_tick_step_value * i
                self.graph.DrawText(text, (x, -10), color='black')
                i += 1
        i=1
        for y in range(0, self.plot_y_range+1, self.y_tick_step):
            print('y=',y)
            self.graph.DrawLine((-3, y), (3, y))
            if y != 0:
                text = "{:5.1f} ".format(self.y_tick_step_value * i)
                self.graph.DrawText(text, (-10, y), color='black')
                i += 1

    def draw_plot(self):
        prev_x = prev_y = None
        for i, x_coord in enumerate(self.linear_space):
            x_coord = int(i * self.x_scale_factor)
            y_coord = int(self.y_data[i] * self.y_scale_factor)
            if prev_x is not None:
                self.graph.draw_line((prev_x, prev_y), (x_coord, y_coord),
                                     color='#595959', width=1.8)
            prev_x, prev_y = x_coord, y_coord

    def add_data(self, data):
        data = int(data)
        if data < self.data_min_value:
            data = self.data_min_value
        elif data > self.data_max_value:
            data = self.data_max_value
        self.y_data[0:self.data_max_idx] = self.y_data[1:self.data_points]
        self.y_data[self.data_max_idx]   = data
        
    def update_plot(self, tlm_msg: TelemetryMessage):
        if tlm_msg.app_name == self.app_name:
            payload = tlm_msg.payload()
            #print('payload = ', payload)
            if self.tlm_payload in str(type(payload)):
                has_element = False
                for p in payload:
                    if self.tlm_element in p[0]:
                        has_element = True
                        break
                if has_element:
                    data = payload[self.tlm_element]
                    self.add_data(data)
                    self.graph.erase()
                    self.draw_axes()
                    self.draw_plot()

    def execute(self, app_name, tlm_topic, tlm_payload, tlm_element):
        """
        The current value observer must be created after the GUI window is created
        """
        self.app_name    = app_name
        self.tlm_topic   = tlm_topic
        self.tlm_payload = tlm_payload
        self.tlm_element = tlm_element 

        self.create_window(app_name+'/'+tlm_payload+'/'+tlm_element)

        self.tlm_current_value = TelemetryCurrentValue(self.tlm_server, self.update_plot)
        self.tlm_server.execute()

        while True: # Event Loop
            event, values = self.window.read(timeout=200)
            if event in (sg.WIN_CLOSED, 'Exit'):
                break

        self.tlm_server.shutdown()


###############################################################################

if __name__ == '__main__':
    
    if len(sys.argv) > 1:
        app_name    = sys.argv[1]
        tlm_topic   = sys.argv[2]
        tlm_payload = sys.argv[3]
        tlm_element = sys.argv[4]
        min_value   = int(sys.argv[5])
        max_value   = int(sys.argv[6])
    else:
        app_name    = 'OSK_C_DEMO'
        tlm_topic   = 'OSK_C_DEMO/Application/STATUS_TLM'
        tlm_payload = 'StatusTlm'
        tlm_element = 'DeviceData'
        min_value   = 0
        max_value   = 6
    
    config = configparser.ConfigParser()
    config.read('../basecamp.ini')

    cfs_host_addr = config.get('NETWORK', 'CFS_HOST_ADDR')
    tlm_port = config.getint('NETWORK', 'TLM_PLOT_TLM_PORT')

    tlm_plot = TlmPlot(cfs_host_addr, tlm_port, 1.0, min_value, max_value)
    tlm_plot.execute(app_name, tlm_topic, tlm_payload, tlm_element) 
    
