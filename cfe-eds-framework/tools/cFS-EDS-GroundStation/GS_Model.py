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
cFS-GroundStation Model:
This module contains classes and functions to handle the various sets of data within
the cFS-GroundStation.  This includes structures to hold incoming telemetry data,
disctionaries and lists of the instances, topics, subcommands, and telemetry types.
Finally, several functions are used to generate human readable text string to output
in the viewer based on raw or decoded messages.
'''
import time

import GS_Controller

def HexString(string, hex_per_line):
    '''
    Generates a human readable hex dump of a hex string

    Inputs:
    string - hex representation of a bytes string
    hex_per_line - number of bytes to appear on each line

    Outputs:
    hex_string - Human readable string of the hex dump
    '''
    hex_string = ''
    count = 0
    for i in range(0, len(string), 2):
        hex_string += "0x{}{} ".format(string[i].upper(), string[i+1].upper())
        count += 1
        if count % hex_per_line == 0:
            hex_string += '\n'
    return hex_string


def TlmDisplayString(eds_db, base_object, base_name, message=''):
    '''
    Generates a string to display in the telemetry log that shows the contents
    of a telemetry message object

    Inputs:
    eds_db - EDS database
    base_object - the decoded EDS telemetry object
    base_name - the base EDS name of the telemetry object
    message - string used in the recursion to keep track of the object's structure
    '''
    result = message
    # Array display string
    if (eds_db.IsArray(base_object)):
        for i in range(len(base_object)):
            result = TlmDisplayString(eds_db, base_object[i], f"{base_name}[{i}]", result)
    # Container display string
    elif (eds_db.IsContainer(base_object)):
        for item in base_object:
            result = TlmDisplayString(eds_db, item[1], f"{base_name}.{item[0]}", result)
    # Everything else (number, enumeration, string, etc.)
    else:
        result += '{:<60} = {}\n'.format(base_name, base_object)

    return result


class DataModel(object):
    '''
    The DataModel class contains strcutres related to the data saved in the cFS-Groundstation
        - Stores dictionaries of Instances, Topics, Subcommands, and Telemetry Types
        - Stores an array of raw messages based on Telmetry Type
    '''
    def __init__(self):
        self.instance_chooser = "-- Instance --"
        self.topic_chooser = "-- Topic --"
        self.subcommand_chooser = "-- Subcommand --"
        self.default_enum_label = "-- Value --"
        self.tlm_chooser = "-- Instance:Topic --"

        self.instance_dict = {}
        self.telecommand_topic_dict = {}
        self.telemetry_topic_dict = {}
        self.subcommand_dict = {}

        self.instance_keys = []
        self.telecommand_topic_keys = []
        self.telemetry_topic_keys = []
        self.subcommand_keys = []

        self.instance_values = []
        self.telemetry_topic_values = []

        self.tlm_data = {}
        self.tlm_data_keys = []


    def InitializeLists(self):
        '''
        Once the EdsDb and IntfDb have been initialized in the Controller, this function can
        be called to initialize the various dictionaries and key lists.
        '''
        self.instance_dict = GS_Controller.control.GetInstances()
        self.telecommand_topic_dict = GS_Controller.control.GetTelecommandTopics()
        self.telemetry_topic_dict = GS_Controller.control.GetTelemetryTopics()
        self.subcommand_dict = GS_Controller.control.GetSubcommands(0)

        self.instance_keys = list(self.instance_dict.keys())
        self.telecommand_topic_keys = list(self.telecommand_topic_dict.keys())
        self.telemetry_topic_keys = list(self.telemetry_topic_dict.keys())
        self.subcommand_keys = list(self.subcommand_dict.keys())

        self.instance_values = list(self.instance_dict.values())
        self.telemetry_topic_values = list(self.telemetry_topic_dict.values())


    def UpdateSubcommands(self, topic):
        '''
        When a new topic is selected in the viewer this function updates the subcommand
        disctionary and key list with new information (if available)

        Inputs:
        topic - The selected Telecommand topic
        '''
        self.subcommand_dict = GS_Controller.control.GetSubcommands(topic)
        self.subcommand_keys = list(self.subcommand_dict.keys())


    def AddTlm(self, eds_db, host, datagram, decode_output):
        '''
        Generates a telemetry indicator string based on the instance and topic names.
        Sorts the raw message into the associated data array (or creates one if it doesn't exist)
        Display strings are generated and sent to the Viewer to update in the display

        Inputs:
        eds_db - EDS Database
        host - Information where the telemetry message came from
        datagram - Raw telemetry message as a bytes string
        decode_output - Tuple containing the output from control.DecodeMessage:
            topic_id - The Topic ID associated with the telemetry message
            eds_entry - The EDS function to create an object associated with the telemetry message
            eds_object - The decoded EDS object
        '''
        topic_id = decode_output[0]
        eds_entry = decode_output[1]
        eds_object = decode_output[2]

        topic_name = self.telemetry_topic_keys[self.telemetry_topic_values.index(topic_id)]
        try:
            instance_index = self.instance_values.index(int(eds_object.CCSDS.ApidQ.SubsystemId))
            instance_name = self.instance_keys[instance_index]
            
            tlm_instance_topic = f"{instance_name}:{topic_name}"
        
        except AttributeError:
            tlm_instance_topic = f"{topic_name}"

        if tlm_instance_topic in self.tlm_data:
            self.tlm_data[tlm_instance_topic].append(datagram)
            new_tlm_type = None
        else:
            self.tlm_data[tlm_instance_topic] = [datagram]
            self.tlm_data_keys = list(self.tlm_data.keys())
            new_tlm_type = tlm_instance_topic


        disp_start = f"Telemetry Packet From: {host[0]}:UDP {host[1]}, {8*len(datagram)} bits :\n"
        message_hex_dump = HexString(datagram.hex(), 16)
        topic_str = f"Instance:Topic = {tlm_instance_topic}\n"
        object_str = TlmDisplayString(eds_db, eds_object, eds_entry.Name)
        tlm_message = disp_start + message_hex_dump + '\n' + topic_str + object_str + '\n'

        return new_tlm_type, tlm_message


    def SaveTlmType(self, tlm_choice):
        '''
        Writes the raw_messages of a given telemetry indicator string to a time stamped file.

        Inputs:
        tlm_choice - the chosen telemetry indicator string

        Outputs:
        binary file based on the tlm_choice and the time stamp.
        First the length of each packet is written as a 4-byte unsigned integer, then
        raw packets are written to the file.
        Packets that are written to the file are cleared from the data structure.
        '''
        num_tlm_messages = len(self.tlm_data[tlm_choice])
        tlm_choice_edited = tlm_choice.replace('/', '_')
        tlm_choice_edited = tlm_choice_edited.replace(':', '_')
        if num_tlm_messages != 0:
            time_str = time.strftime("%Y-%m-%d__%H_%M_%S", time.gmtime())
            filename = f"output/{tlm_choice_edited}__{time_str}.bin"
            filename.replace(':', '_')
            fout = open(filename, 'wb')

            tlm_length = len(self.tlm_data[tlm_choice][0])
            fout.write((tlm_length).to_bytes(4, byteorder='big', signed=False))

            while num_tlm_messages > 0:
                message = self.tlm_data[tlm_choice].pop(0)
                fout.write(message)
                num_tlm_messages -= 1

            fout.close()


    def SaveAllTlm(self):
        '''
        Loops over all the telemetry indicator strings and calls SaveTlm for each one
        '''
        for tlm_choice in self.tlm_data_keys:
            self.SaveTlmType(tlm_choice)


data = DataModel()
