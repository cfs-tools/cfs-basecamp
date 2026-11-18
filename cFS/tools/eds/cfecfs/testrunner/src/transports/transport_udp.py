'''
Copyright (c) 2025 United States Government as represented by
the Administrator of the National Aeronautics and Space Administration.
All Rights Reserved.
'''

import ipaddress
import socket
import selectors
import errno
from testrunner.core.transport_base import TransportBase
from testrunner.core.timestamp import Timestamp

class Transport_UDP(TransportBase):

    def __init__(self, remote_addr:tuple, local_addr:tuple=None):

        try: 
            ipaddress.IPv4Address(remote_addr[0])
        except ipaddress.AddressValueError as e:
            print (f"remote IP value is not valid {e}")
            raise

        self.remote_addr = remote_addr
        self.local_addr = local_addr if local_addr else ("127.0.0.1",2234)
        self.socket = None
        self.family = socket.AF_INET
        self.type = socket.SOCK_DGRAM
        self.usock = None
        self.rdsel = None

    def get_socket(self):
        if self.usock is None:
            print(f"Init socket bound to {self.local_addr[0]}:{self.local_addr[1]}")

            try:
                self.usock = socket.socket(self.family, self.type)
                self.usock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                self.usock.bind(self.local_addr)
                self.usock.setblocking(False)
                self.max_recv_bytes = 4096
        
            except Exception as e:
                print(f"socket initialization failed: {e}")
                self.usock = None
                raise
            
        return self.usock

    def wait_readable(self, timeout:Timestamp):
        if self.rdsel is None:
            self.rdsel = selectors.DefaultSelector()
            self.rdsel.register(self.get_socket(), selectors.EVENT_READ)

        remaining_sec = timeout.remaining_seconds()
        if remaining_sec is not None and remaining_sec < 0:
            remaining_sec = 0

        return self.rdsel.select(remaining_sec)

    def transmit(self, data:bytes):

        try:
            bytessent = self.get_socket().sendto(data, self.remote_addr)
        except Exception as e:
            print("socket.sendto() failed: {e}")
            bytessent = 0

        return (bytessent == len(data))

    def receive(self, timeout:Timestamp):

        # wait for the socket to be readable
        self.wait_readable(timeout)

        try:
            (data, recv_host) = self.get_socket().recvfrom(self.max_recv_bytes)
        except IOError as exception:
            data = None
            recv_host = None
            if exception.errno == errno.EWOULDBLOCK:
                pass

        return (data, recv_host)
