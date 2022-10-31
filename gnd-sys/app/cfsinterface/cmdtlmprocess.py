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
        Defines a base class used by processes that are launched by Basecamp. This
        manages the command and telemetry connections to the CmdTlmRouter.
    
"""

import sys
import time
import os
import socket
from queue import Queue

if __name__ == '__main__' or 'cfsinterface' in os.getcwd():
    sys.path.append('..')
    from telecommand  import TelecommandScript
    from telemetry    import TelemetrySocketServer
else:
    from .telecommand  import TelecommandScript
    from .telemetry    import TelemetrySocketServer
    
###############################################################################

class CmdProcess():
    """
    """
    def __init__(self, gnd_ip_addr, router_cmd_port):
        self.gnd_ip_addr = gnd_ip_addr
        self.router_cmd_port = router_cmd_port
        self.router_cmd_socket = None
        self.router_cmd_socket_addr = (self.gnd_ip_addr, self.router_cmd_port)
        self.router_cmd_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        self.cfs_cmd_queue = Queue()
        self.cmd_script = TelecommandScript('samplemission', 'cpu1', self.cfs_cmd_queue)  #TODO - Use kwarg?
       
       
    def send_cfs_cmd(self, app_name, cmd_name, cmd_payload):
        (cmd_sent, cmd_text, cmd_status) = self.cmd_script.send_cfs_cmd(app_name, cmd_name, cmd_payload)
        datagram = self.cfs_cmd_queue.get()
        print(f'CmdTlmProcess sending {app_name}:{cmd_name} to router {self.router_cmd_socket_addr}')
        self.router_cmd_socket.sendto(datagram, self.router_cmd_socket_addr)

   
###############################################################################

class CmdTlmProcess(CmdProcess):
    """
    Defines a base class used by processes that are launched by Basecamp. This
    manages the command and telemetry connections to the CmdTlmRouter.
    
    TelecommandScript() is designed to send commands to a queue and not to a
    socket. This design is based on CmdTlmRouter being part of the main app. 
    Remote app sockets were added later so rather than complicate the existing
    design this CmdTlmProcess base class was created to perform the role of a
    local router. There's a little extra queueing involved but this is a 
    lightweight design that does not need to scale up.
    
    The following steps outline how to create a new Basecamp GUI-based 
    CmdTlmcProcess class. See filebrowser.py for a complete example:
      1. Create a standalone GUI class and identify cmd & tlm interface points
      2. Add CmdTlmProcess subclass 
      3. Create equivalent of FileBrowserTelemetryMonitor if you need to monitor
         telemetry points. Create a tlm callback function that populates GUI
      4. Create a shutdown method that terminates all threads
       
    """
    def __init__(self, gnd_ip_addr, router_ctrl_port, router_cmd_port, router_tlm_port, router_tlm_timeout):
        super().__init__(gnd_ip_addr, router_cmd_port)
        self.tlm_server = TelemetrySocketServer('samplemission', 'cpu1', gnd_ip_addr, router_ctrl_port,
                                                router_tlm_port, router_tlm_timeout)
 
    def shutdown(self):
        self.tlm_server.shutdown()
