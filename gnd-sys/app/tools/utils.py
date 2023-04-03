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


###############################################################################

def bin_hex_decode(in_hex_string):
    """
    Must match app_c_fw PktUtil_HexEncode() which also means in must contain an
    even number of bytes.
    """
    out_bin_array = bytearray(int(len(in_hex_string)/2))
    
    for i in range(0, len(out_bin_array)):
        hi_nibble = hex_char_2_bin(in_hex_string[int(i*2)])
        lo_nibble = hex_char_2_bin(in_hex_string[int(i*2+1)])
        if hi_nibble is None or lo_nibble is None:
            return None
        else:
            out_bin_array[i] = (hi_nibble << 4) | lo_nibble

    return out_bin_array 
    

###############################################################################

ORD_0 = ord('0')
ORD_9 = ord('9')
ORD_A = ord('A')
ORD_F = ord('F')
ORD_a = ord('a')
ORD_f = ord('f')

def hex_char_2_bin(hex_char):

    bin_val = None
    
    hex_ord = ord(hex_char)
    if (hex_ord >= ORD_0 and hex_ord <= ORD_9): 
        bin_val = hex_ord - ORD_0;
    elif (hex_ord >= ORD_A and hex_ord <= ORD_F):
        bin_val = hex_ord - ORD_A + 10;
    elif (hex_ord >= ORD_a and hex_ord <= ORD_f):
        bin_val = hex_ord - ORD_a + 10;

    return bin_val


###############################################################################

def bin_hex_encode(in_bin_buf):
    """
    Must match app_c_fw PktUtil_HexDecode() and PktUtil_HexEncode()
    Each binary numeric value is encoded using 2 hex digits regardless of 
    whether the numeric value could be represented by one digit. Each byte
    has a value between 0-255 and is represented by 0x00-0xFF. As a result,
    encoded buffer will always be twice the size of binary.
    """
    
    hex_digit  = "0123456789ABCDEF"
    hex_string = ''
    for i in range(0, len(in_bin_buf)):
        hex_string += hex_digit[in_bin_buf[i] >> 4]
        hex_string += hex_digit[in_bin_buf[i] & 0x0F]
        
    return hex_string 
