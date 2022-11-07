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
        Provide JSON base class.
"""
import os
import socket
import fcntl
import struct

###############################################################################

def compress_abs_path(path):
    """
    Remove '../' entries from a path
    """
    path_array = path.split(os.path.sep)
    path_array.pop(0)   # Abs paths start with separator so first split array is ''

    new_path_array = []
    for p in path_array:
        if p == '..':
            new_path_array.pop()
        else:
            new_path_array.append(p)
    
    new_path = os.path.sep
    for p in new_path_array:
        new_path += p + os.path.sep
    
    return new_path


###############################################################################

CRC_POLY_32C = 0x82f63b78  # CRC-32C (iSCSI) polynomial in reversed bit order
CRC_POLY_32  = 0xedb88320  # CRC-32 (Ethernet, ZIP, etc.) polynomial in reversed bit order

def crc_32c(crc, bytes_obj):
 
    crc = ~crc
    
    for byte in bytes_obj:
    
        crc = crc ^ byte
        for i in range(8):
            crc = (crc >> 1) ^ CRC_POLY_32C if crc & 1 else crc >> 1
        
    return ~crc    


###############################################################################

def datagram_to_str(datagram):
    datagram_str = ""

    for chunk in [datagram[i:i + 8] for i in range(0, len(datagram), 8)]:
        datagram_str += " ".join([f"0x{byte:02X}" for byte in chunk])

    return datagram_str #TODO - Decide on '\n'


###############################################################################

def get_ip_addr(ifname):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    return socket.inet_ntoa(fcntl.ioctl(
        s.fileno(),
        0x8915,  # SIOCGIFADDR
        struct.pack('256s', ifname[:15].encode('utf-8'))
    )[20:24])


###############################################################################

def hex_string(string, hex_per_line):
    """
    Generates a human readable hex dump of a hex string,
    """
    hex_string = ''
    count = 0
    for i in range(0, len(string), 2):
        hex_string += "0x{}{} ".format(string[i].upper(), string[i+1].upper())
        count += 1
        if count % hex_per_line == 0:
            hex_string += '\n'
    return hex_string 

