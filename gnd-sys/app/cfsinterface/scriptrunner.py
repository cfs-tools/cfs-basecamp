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
        Provide a simple environment to execute user python scripts. This is not
        intended to replace a fuly featured ground system. Basecamp is an educational
        tool that should be simple to utilize. Basecamp and more specifically the 
        cfe-eds-framework can be used as the foundation to integrate the cFS with
        a ground system.
    
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

#print("tlm_cvt_proto['CFE_ES'][CCSDS]['Sequence'] = " + str(tlm_cvt_proto['CFE_ES'][CCSDS]['Sequence']))
# TODO - Temporary structure to try different script telemetry interfaces 
tlm_cvt_proto = {

    'CFE_ES' : [ { 'AppId': 0,   'Sequence': 0   },
                 { 'Seconds': 0, 'Subseconds': 0 },
                 { 
                   'CommandCounter': 0,
                   'CommandErrorCounter': 0
                }]
    }


###############################################################################

class TelemetryCurrentValue(TelemetryObserver):
    """
    callback_functions
       [app_name] : {packet: [item list]} 
    
    """

    def __init__(self, tlm_server: TelemetrySocketServer, event_callback): 
        super().__init__(tlm_server)

        self.event_callback = event_callback
                
        for msg in self.tlm_server.tlm_messages:
            tlm_msg = self.tlm_server.tlm_messages[msg]
            self.tlm_server.add_msg_observer(tlm_msg, self)        
            print("TelemetryCurrentValue adding observer for %s: %s" % (tlm_msg.app_name, tlm_msg.msg_name))

        # Debug to help determine how to structure current value data       
        topics = self.tlm_server.get_topics()
        for topic in topics:
            #if topic != self.tlm_server.eds_mission.TOPIC_TLM_TITLE_KEY:
            if 'ES' in topic:
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
        
        if tlm_msg.app_name == 'CFE_EVS':
            if tlm_msg.msg_name == 'LONG_EVENT_MSG':
                payload = tlm_msg.payload()
                pkt_id = payload.PacketID
                event_text = "FSW Event at %s: %s, %d - %s" % \
                             (str(tlm_msg.sec_hdr().Seconds), pkt_id.AppName, pkt_id.EventType, payload.Message)
                self.event_callback(event_text)

        elif tlm_msg.app_name == 'CFE_ES':
            payload = tlm_msg.payload()
            tlm_cvt_proto['CFE_ES'][CCSDS]['AppId']     = tlm_msg.pri_hdr().AppId
            tlm_cvt_proto['CFE_ES'][CCSDS]['Sequence']  = tlm_msg.pri_hdr().Sequence
            tlm_cvt_proto['CFE_ES'][TIME]['Seconds']    = tlm_msg.sec_hdr().Seconds
            tlm_cvt_proto['CFE_ES'][TIME]['Subseconds'] = tlm_msg.sec_hdr().Subseconds
            tlm_cvt_proto['CFE_ES'][PAYLOAD]['CommandCounter']      = payload.CommandCounter
            tlm_cvt_proto['CFE_ES'][PAYLOAD]['CommandErrorCounter'] = payload.CommandErrorCounter

          
###############################################################################

class ScriptRunner(CmdTlmProcess):
    """
    """
    def __init__(self, mission_name, gnd_ip_addr, router_ctrl_port, script_cmd_port, script_tlm_port, script_tlm_timeout):
        super().__init__(mission_name, gnd_ip_addr, router_ctrl_port, script_cmd_port, script_tlm_port, script_tlm_timeout)

        self.tlm_current_value = TelemetryCurrentValue(self.tlm_server, self.event_msg)
        self.tlm_server.execute()
        self.scrit = None
    
    def event_msg(self, event_text):
        print('ScriptRunner received event: ' + event_text)
        
    def get_tlm_val(self, app_name, tlm_msg_name, parameter):
        """
        Example usage: get_tlm_val("CFE_ES", "HK_TLM", "Sequence")
        """
        return self.tlm_server.get_tlm_val(app_name, tlm_msg_name, parameter)
            
    def tlm_wait_thread(self, tlm_msg: TelemetryMessage) -> None:
        pass
        
    def run_script(self, script):
        """
        User script passed as a string parameter and it executes within the context of
        a ScriptRunner object so it can access all of the methods 
        Design Note: I tried saving script to a file and then passing it to
        sg.execute_py_file(). A standalone script loses the ScriptRunner context and this
        method would require a new ScriptRunner main file to be generated with the script.  
        """
        with redirect_stdout(io.StringIO()) as output:
            try:
                exec(script)
            except:
                pass
        output_str = output.getvalue()
        
        if output_str is not None:
             sg.popup(output_str,title="Script Status")
    
###############################################################################

if __name__ == '__main__':

    print(f"Name of the script      : {sys.argv[0]=}")
    print(f"Arguments of the script : {sys.argv[1:]=}")
    #if len(sys.argv) > 1:
    #    script_file = sys.argv[1]
    #    #print ('filename = ' + filename)

    config = configparser.ConfigParser()
    config.read('../basecamp.ini')
    SCRIPT_PATH = config.get('PATHS', 'SCRIPT_PATH')
    print ("SCRIPT_PATH = " + SCRIPT_PATH)    
    demo_script_path = compress_abs_path(os.path.join(os.getcwd(), '..',SCRIPT_PATH))
    demo_script = os.path.join(demo_script_path, 'demo_script.py') 

    cfs_ip_addr = config.get('NETWORK', 'CFS_IP_ADDR')
    router_ctrl_port = config.getint('NETWORK','CMD_TLM_ROUTER_CTRL_PORT')
    script_cmd_port  = config.getint('NETWORK', 'SCRIPT_RUNNER_CMD_PORT')
    script_tlm_port  = config.getint('NETWORK', 'SCRIPT_RUNNER_TLM_PORT')
    mission_name     = config.get('CFS_TARGET','MISSION_EDS_NAME')

    script_runner = ScriptRunner(mission_name, cfs_ip_addr, router_ctrl_port, script_cmd_port, script_tlm_port, 1.0)
    
    text_editor = TextEditor(demo_script, run_script_callback=script_runner.run_script)
    text_editor.execute()
    
    script_runner.shutdown()

