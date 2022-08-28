'''
LEW-20210-1, Python Ground Station for a Core Flight System with CCSDS Electronic Data Sheets Support

Copyright (c) 2020 United States Government as represented by
the Administrator of the National Aeronautics and Space Administration.
All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
'''


'''
tlm_decode.py

This module listens for UDP messages along a user-specified port
Messages that come in are decoded into an EDS object where the object's
contents are printed to the screen as they come in real time

Command line use:
python3 tlm_decode.py -m <mission_name> -p <port_number=5021>
'''
import sys
import getopt
import socket
import time

import EdsLib
import CFE_MissionLib



def decode_message(mission, intf_db, raw_message):
    '''
    Decodes a raw input message into an EdsObject

    Inputs:
    mission - User specified mission name
    intf_db - CFE_MissionLib Interface Database
    raw_message - Packed Bytes message

    Outputs:
    eds_entry - The EdsDb function to create the EDS object associated with the input message
    eds_object - The Unpacked EdsDb Object
    '''
    eds_id, topic_id = intf_db.DecodeEdsId(raw_message)
    eds_entry = EdsLib.DatabaseEntry(mission, eds_id)
    eds_object = eds_entry(EdsLib.PackedObject(raw_message))
    return (eds_entry, eds_object)


def display_entries(eds_db, base_object, base_name):
    '''
    Recursive function that iterates over an EDS object and prints the contents of
    the sub-entries to the screen

    Inputs:
    eds_db - EDS Database
    base_object - The EDS object to iterate over
    base_name - The base name for the sub-entities printed to the screen
    '''
    # Array display string
    if (eds_db.IsArray(base_object)):
        for i in range(len(base_object)):
            display_entries(eds_db, base_object[i], f"{base_name}[{i}]")
    # Container display string
    elif (eds_db.IsContainer(base_object)):
        for item in base_object:
            display_entries(eds_db, item[1], f"{base_name}.{item[0]}")
    # Everything else (number, enumeration, string, etc.)
    else:
        print('{:<60} = {}'.format(base_name, base_object))


def hex_string(string, bytes_per_line):
    '''
    Converts a hex representation of a bytes string to a more human readable format

    Inputs:
    string - hex representation of a bytes string
    bytes_per_line - Number specifying the number of hex bytes per line

    Output:
    hex_str - string that can be printed to the screen
    '''
    hex_str = ''
    count = 0
    for i in range(0, len(string), 2):
        hex_str += "0x{}{} ".format(string[i].upper(), string[i+1].upper())
        count += 1
        if count % bytes_per_line == 0:
            hex_str += '\n'
    return hex_str


def main(argv):
    """
    Gets the mission name and port number from command line arguments
    Opens up the receive port and listens for telemetry messages
    Each message is decoded into an EDS Object and the object's contents are printed to the screen
    """
    try:
        opts, args = getopt.getopt(argv, "hp:", ["port="])
    except getopt.GetoptError:
        print("tlm_decode.py -p <port number=1235>")
        sys.exit(2)

    udp_recv_port = 1235
    mission = "@CFS_EDS_GS_MISSION_NAME@".lower()
    for opt, arg in opts:
        if opt == '-h':
            print("tlm_decode.py -p <port number=1235>")
            sys.exit()
        elif opt in ('-p', '--port'):
            udp_recv_port = int(arg)

    try:
        # Initialize databases
        eds_db = EdsLib.Database(mission)
        intf_db = CFE_MissionLib.Database(mission, eds_db)
    except RuntimeError:
        print("tlm_decode is not properly configured")
        sys.exit(2)

    print("Listening in on port {} for messages".format(udp_recv_port))

    # Init udp socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('', udp_recv_port))

    # Wait for UDP messages
    while True:
        try:
            # Receive message
            datagram, host = sock.recvfrom(4096) # buffer size is 1024 bytes

            # Ignore datagram if it is not long enough (i.e it doesnt contain tlm header)
            if len(datagram) < 6:
                continue

            print(f"Telemetry Packet From: {host[0]}:UDP {host[1]}, {8*len(datagram)} bits :")
            print(hex_string(datagram.hex(), 16))
            eds_entry, eds_object = decode_message(mission, intf_db, datagram)
            display_entries(eds_db, eds_object, eds_entry.Name)
            print()
            print()

        # Handle errors
        except socket.error:
            print('Ignored socket error.')
            time.sleep(1)


if __name__ == "__main__":
    main(sys.argv[1:])
