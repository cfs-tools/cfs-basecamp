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
      Manage UDP connections to the cFS
    
    Notes:
      1. This design supports a singe router connected to a single cFS
         instance running UDP versions of the command ingest and telemetru
         output apps. 
      2. The app that creates the router communicates via queues to the router.
         The router supports additional UDP command and telemetry connections.
         Commands from mutliple UDP command sockets are not sent to the cFS and
         are placed in a queue that can be read by the parent app. This allows
         the app to serve as a single point for managing flight commands. 
         Telemetry is routed from the cFS socket to multiple telemetry
         monitors.
      3. 'Ground telemetry' is cFS telemetry sent to multiple ground telemetry
         destinations. It is not telemetry from a ground source.
         
"""
import socket
import logging
from queue import Queue
from threading import Thread, Lock

logger = logging.getLogger("router")

class RouterCmd():
   CLOSE_PORT      = 'ClosePort'
   SET_CFS_IP_ADDR = 'SetCfsIpAddr'
   
###############################################################################

class CmdSource():
    """
    Provide a socket to receive command and queue commands.
    """
    def __init__(self, ip_addr, port, timeout):
        
        self.enabled = True

        self.socket  = None
        self.ip_addr = ip_addr
        self.port    = port
        self.socket_addr = (self.ip_addr, self.port)
        self.timeout = timeout
        
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind(self.socket_addr)
        self.socket.setblocking(False)
        self.socket.settimeout(self.timeout)
    
    def read_cmd_port(self, queue):
        try:
            while True:
                datagram, host = self.socket.recvfrom(1024)
                queue.put((datagram, host))
                logger.debug(f'Received cmd source datagram: size={len(datagram)} {host}')
        except socket.timeout:
            pass


###############################################################################

class CmdTlmRouter(Thread):
    """
    The router and cFS command input designs are identical. Using a queue for
    router control commands is a little overkill since the only ground command 
    is to remove a telemetry port. 
    """
    def __init__(self, cfs_ip_addr, cfs_cmd_port, 
                 gnd_ip_addr, router_ctrl_port, gnd_tlm_port, gnd_tlm_timeout):
    
        super().__init__()

        self.enabled = True

        # cFS Commands
                
        self.cfs_ip_addr    = cfs_ip_addr
        self.cfs_cmd_socket = None
        self.cfs_cmd_port   = cfs_cmd_port
        self.set_cfs_ip_addr(cfs_ip_addr)
        
        self.cfs_cmd_source = {}
        self.cfs_cmd_source_queue = Queue()
        self.cfs_cmd_queue  = Queue()
        
        # Ground Commands & Telemetry
        
        self.gnd_ip_addr    = gnd_ip_addr
        
        self.router_ctrl_socket = None
        self.router_ctrl_port   = router_ctrl_port
        self.router_ctrl_socket_addr = (self.gnd_ip_addr, self.router_ctrl_port)
        self.router_ctrl_source = {}
        self.router_ctrl_queue  = Queue()
        self.add_router_ctrl_source(self.router_ctrl_port)

        self.gnd_tlm_socket = None
        self.gnd_tlm_port   = gnd_tlm_port
        self.gnd_tlm_queue  = Queue()
        self.gnd_tlm_socket_addr = (self.gnd_ip_addr, self.gnd_tlm_port)
        self.gnd_tlm_timeout = gnd_tlm_timeout

        self.tlm_dest_mutex  = Lock()
        self.tlm_dest_addr   = {}
        self.tlm_dest_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        self.tlm_dest_connect_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.tlm_dest_connect_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.tlm_dest_connect_socket.bind((self.gnd_ip_addr, 7777))
        self.tlm_dest_connect_socket.setblocking(True)

        self.tlm_dest_connect = Thread(target=self.tlm_dest_connect_thread)
        self.tlm_dest_connect.kill   = False
        self.tlm_dest_connect.daemon = True
        
        logger.info(f'CmdTlmRouter Init: cfs_cmd_socket{self.cfs_cmd_socket_addr}, gnd_tlm_socket{self.gnd_tlm_socket_addr}')


    # cFS Commands
    
    def get_cfs_cmd_queue(self):
        return self.cfs_cmd_queue

    def add_cfs_cmd_source(self, cmd_port):
        self.cfs_cmd_source[cmd_port] = CmdSource(self.cfs_ip_addr, cmd_port, 0.2)  #TODO - Decide on timeout management
        
    def get_cfs_cmd_source_queue(self):
        return self.cfs_cmd_source_queue

    def remove_cfs_cmd_source(self, cmd_port):
        try:
            del self.cfs_cmd_source[cmd_port]
        except KeyError:
            logger.error(f'Error removing nonexitent command source {cmd_port} from cfs_cmd_source dictionary')  
        
        
    # Ground Commands & Telemetry
    
    def get_router_ctrl_queue(self):
        return self.router_ctrl_queue

    def add_router_ctrl_source(self, cmd_port):
        self.router_ctrl_source[cmd_port] = CmdSource(self.gnd_ip_addr, cmd_port, 0.1)  #TODO - Decide on timeout management
        
    def remove_router_ctrl_source(self, cmd_port):
        try:
            del self.router_ctrl_source[cmd_port]
        except KeyError:
            logger.error(f'Error removing nonexitent command source {cmd_port} from router_ctrl_source dictionary')  

    def get_gnd_tlm_queue(self):
        return self.gnd_tlm_queue

    def add_gnd_tlm_dest(self, tlm_port):
        self.tlm_dest_mutex.acquire()
        self.tlm_dest_addr[tlm_port] = (self.gnd_ip_addr, tlm_port)
        self.tlm_dest_mutex.release()

    def remove_gnd_tlm_dest(self, tlm_port):
        self.tlm_dest_mutex.acquire()
        try:
            del self.tlm_dest_addr[tlm_port]
            logger.info(f'Removed telemetry destination port {tlm_port}')
        except KeyError:
            logger.error(f'Error removing nonexitent telemetry source {tlm_port} from tlm destination dictionary')  
        self.tlm_dest_mutex.release()
    
    def set_cfs_ip_addr(self, ip_addr):
        """
        This is used for switching between local and remote cFS targets
        """
        self.cfs_ip_addr = ip_addr
        self.cfs_cmd_socket_addr = (self.cfs_ip_addr, self.cfs_cmd_port)
        logger.info(f'cfS IP address set to {self.cfs_cmd_socket_addr}')

    def run(self):

        # cFS Commands
        
        self.cfs_cmd_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        
        # Ground Commands & Telemetry
        
        self.router_ctrl_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        
        self.gnd_tlm_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.gnd_tlm_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.gnd_tlm_socket.bind(self.gnd_tlm_socket_addr)
        self.gnd_tlm_socket.setblocking(False)
        self.gnd_tlm_socket.settimeout(self.gnd_tlm_timeout)

        self.tlm_dest_connect.start()
        
        try:
            while self.enabled:
                self.manage_routes()
        except OSError:
            # shutting down
            pass
        except Exception as e:
            logger.error(f'CmdTlmRouter stopped due to error: {e}')

        self.shutdown()


    def manage_routes(self):
        """
        The order of processing is intentional in terms of priority. However, Basecamp
        is not intended to support a cFS mission, so system performance just needs to
        be acceptable for STEM education type projects.
        """
        # Process cFS Commands
        # 1. Put commands from all sources on self.cfs_cmd_source_queue
        # 2. Send all commands from self.cfs_cmd_source_queue
        for cmd_source in self.cfs_cmd_source:
            self.cfs_cmd_source[cmd_source].read_cmd_port(self.cfs_cmd_source_queue)

        while not self.cfs_cmd_queue.empty():
            datagram = self.cfs_cmd_queue.get()
            self.cfs_cmd_socket.sendto(datagram, self.cfs_cmd_socket_addr)
            logger.debug(f'cFS command dequeued datagram:\n{self.datagram_to_str(datagram)}')

        # Send telemetry to ground destinations 
        try:
            while True:
                datagram, host = self.gnd_tlm_socket.recvfrom(4096)
                logger.debug(f'Received datagram: size={len(datagram)} {host}\n{self.datagram_to_str(datagram)}')
                self.gnd_tlm_queue.put((datagram, host))
                self.tlm_dest_mutex.acquire()
                for dest_addr in self.tlm_dest_addr:
                    self.tlm_dest_socket.sendto(datagram, self.tlm_dest_addr[dest_addr])
                    logger.debug(f'Sending tlm to destination {dest_addr}')
                self.tlm_dest_mutex.release()
                
        except socket.timeout:
            pass

        # Router Control Commands
        for cmd_source in self.router_ctrl_source:
            self.router_ctrl_source[cmd_source].read_cmd_port(self.router_ctrl_queue)

        while not self.router_ctrl_queue.empty():
            datagram = self.router_ctrl_queue.get()
            cmd = datagram[0].decode()
            logger.debug(f'Ground command datagram dequeued: {datagram[0].decode()}')
            cmd_token = cmd.split(':')
            logger.info(f'Router Control Command: {str(cmd_token)}')
            if cmd_token[0] == RouterCmd.CLOSE_PORT:
                self.remove_gnd_tlm_dest(int(cmd_token[1]))
            elif cmd_token[0] == RouterCmd.SET_CFS_IP_ADDR:
                self.set_cfs_ip_addr(cmd_token[1])
            else:
                logger.info(f'Invalid router command recieved: {str(cmd_token)}')


    def tlm_dest_connect_thread(self):
        
        logger.info('Starting tlm_dest_connect_thread')
        while not self.tlm_dest_connect.kill:
            datagram, host = self.tlm_dest_connect_socket.recvfrom(1024)
            self.tlm_dest_mutex.acquire()
            print(f'Accepted connection from {host}')
            print('Datagram = ', datagram.decode().split(','))
            dest_addr = datagram.decode().split(',')
            self.tlm_dest_addr[int(dest_addr[1])] = (dest_addr[0],int(dest_addr[1]))
            self.tlm_dest_mutex.release()
            logger.info(f'Accepted connection from {host}')


    def datagram_to_str(self, datagram):
        output = []
        for chunk in [datagram[i:i + 8] for i in range(0, len(datagram), 8)]:
            output.append(" ".join([f'0x{byte:02X}' for byte in chunk]))

        return "\n".join(output)

    
    def shutdown(self):
        logger.info('CmdTlm router shutdown started')
        self.enabled = False
        self.tlm_dest_connect.kill = True
        self.cfs_cmd_socket.close()
        self.router_ctrl_socket.close()
        self.gnd_tlm_socket.close()
        self.tlm_dest_socket.close()
        self.tlm_dest_connect_socket.close()
        logger.info('CmdTlm router shutdown completed')
        

###############################################################################
"""
import socket
import os
from _thread import *
ServerSideSocket = socket.socket()
host = '127.0.0.1'
port = 2004
ThreadCount = 0
try:
    ServerSideSocket.bind((host, port))
except socket.error as e:
    print(str(e))
print('Socket is listening..')
ServerSideSocket.listen(5)
def multi_threaded_client(connection):
    connection.send(str.encode('Server is working:'))
    while True:
        data = connection.recv(2048)
        response = 'Server message: ' + data.decode('utf-8')
        if not data:
            break
        connection.sendall(str.encode(response))
    connection.close()
while True:
    Client, address = ServerSideSocket.accept()
    print('Connected to: ' + address[0] + ':' + str(address[1]))
    start_new_thread(multi_threaded_client, (Client, ))
    ThreadCount += 1
    print('Thread Number: ' + str(ThreadCount))
ServerSideSocket.close()


import socket
ClientMultiSocket = socket.socket()
host = '127.0.0.1'
port = 2004
print('Waiting for connection response')
try:
    ClientMultiSocket.connect((host, port))
except socket.error as e:
    print(str(e))
res = ClientMultiSocket.recv(1024)
while True:
    Input = input('Hey there: ')
    ClientMultiSocket.send(str.encode(Input))
    res = ClientMultiSocket.recv(1024)
    print(res.decode('utf-8'))
ClientMultiSocket.close()
"""
