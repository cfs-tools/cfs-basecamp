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
      1. TODO - THIS IS A PROTOTYPE!!!!      
"""

import sys
import time
import os
import socket
import configparser
import io
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
    
    self.tlm_server.get_tlm_val(app_name, tlm_msg_name, parameter)
        Example current value usage: get_tlm_val("CFE_ES", "HK_TLM", "Sequence")
    """
    def __init__(self, gnd_ip_addr, tlm_port, tlm_timeout, min_value, max_value):

        self.tlm_server = TelemetrySocketServer('samplemission', 'cpu1', gnd_ip_addr, tlm_port, tlm_timeout)  #TODO - Use kwarg?

        self.app_name    = 'OSK_C_DEMO'
        self.tlm_topic   = ''
        self.tlm_payload = ''
        self.tlm_element = ''

        self.dataSize = 40
        self.dataMaxIdx = self.dataSize-1
        self.dataRangeMin = min_value
        self.dataRangeMax = max_value
        
        self.xData = np.zeros(self.dataSize)
        self.yData = np.linspace(self.dataRangeMin, self.dataRangeMax, num=self.dataSize, dtype=int)


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
        self.drawAxis()
        self.drawTicks(20)
        self.drawPlot()

    def drawAxis(self):
        self.graph.DrawLine((self.dataRangeMin, 0), (self.dataRangeMax, 0))
        self.graph.DrawLine((0, self.dataRangeMin), (0, self.dataRangeMax))

    def drawTicks(self, step):
        for x in range(self.dataRangeMin, self.dataRangeMax+1, step):
            self.graph.DrawLine((x, -3), (x, 3))
            if x != 0:
                self.graph.DrawText(x, (x, -10), color='black')
        for y in range(self.dataRangeMin, self.dataRangeMax+1, step):
            self.graph.DrawLine((-3, y), (3, y))
            if y != 0:
                self.graph.DrawText(y, (-10, y), color='black')

    def drawPlot(self):
        prev_x = prev_y = None
        for i, xCoord in enumerate(self.yData):
            yCoord = self.xData[i]
            if prev_x is not None:
                self.graph.draw_line((prev_x, prev_y), (xCoord, yCoord),
                                     color='#595959', width=1.8)
            prev_x, prev_y = xCoord, yCoord

    def addData(self, data):
        data = int(data)
        if data < self.dataRangeMin:
            data = self.dataRangeMin
        elif data > self.dataRangeMax:
            data = self.dataRangeMax
        self.xData[0:self.dataMaxIdx] = self.xData[1:self.dataSize]
        self.xData[self.dataMaxIdx]   = data   #todo np.random.randint(self.dataRangeMin, self.dataRangeMax)

    def updatePlot(self, tlm_msg: TelemetryMessage):
        if tlm_msg.app_name == self.app_name:
            payload = tlm_msg.payload()
            print('payload = ', payload)
            if self.tlm_payload in str(type(payload)):
                has_element = False
                for p in payload:
                    if self.tlm_element in p[0]:
                        has_element = True
                        break
                if has_element:
                    data = payload[self.tlm_element]
                    self.addData(data)
                    self.graph.erase()
                    self.drawAxis()
                    self.drawTicks(20)
                    self.drawPlot()

    def execute(self, app_name, tlm_topic, tlm_payload, tlm_element):
        """
        Must start the current value observer after the GUI window is created
        """
        self.app_name    = app_name
        self.tlm_topic   = tlm_topic
        self.tlm_payload = tlm_payload
        self.tlm_element = tlm_element 

        self.create_window(app_name+'/'+tlm_payload+'/'+tlm_element)

        self.tlm_current_value = TelemetryCurrentValue(self.tlm_server, self.updatePlot)
        self.tlm_server.execute()

        while True: # Event Loop

            event, values = self.window.read(timeout=200)

            if event == 'Exit' or event == sg.WIN_CLOSED:
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
        max_value   = 100
    
    config = configparser.ConfigParser()
    config.read('../basecamp.ini')

    cfs_host_addr = config.get('NETWORK', 'CFS_HOST_ADDR')
    tlm_port = config.getint('NETWORK', 'TLM_PLOT_TLM_PORT')

    tlm_plot = TlmPlot(cfs_host_addr, tlm_port, 1.0, min_value, max_value)
    tlm_plot.execute(app_name, tlm_topic, tlm_payload, tlm_element) 
    
