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
cFS-Groundstation Controller:
The controller handles the business logic of the cFS-Groundstation.
The overall functionality includes:
    - Initializes the EDS database objects and interfaces
    - Telemetry Listener that receives and decodes EDS telemetry messages automatically
    - Command generator that creates and sends command packets to core flight instances
'''
import socket
import time

from PyQt5.QtCore import QThread
from PyQt5.QtCore import pyqtSignal

import GS_Model

import EdsLib
import CFE_MissionLib


class TlmListener(QThread):
    '''
    This QThread based class is spawned when the "Start Listening" button is pressed
    in the Telemetry system.  This will listen for messages on a given port, decode them,
    and send the raw messages to the data model for local storage.
    '''
    signal = pyqtSignal(str, str)

    def __init__(self, port):
        super().__init__()
        self.continue_listening = True
        self.port = port

        self.mission = control.mission
        self.intf_db = control.intf_db


    def DecodeMessage(self, raw_message):
        '''
        Decodes a raw bytes message into an EDS object

        Inputs:
        raw_message - received bytes message

        Outputs:
        topic_id - The Telemetry TopicId associated with the raw_message
        eds_entry - The EDS function to create the associated telemetry object
        eds_object - The decoded EDS object
        '''
        eds_id, topic_id = self.intf_db.DecodeEdsId(raw_message)
        eds_entry = EdsLib.DatabaseEntry(self.mission, eds_id)
        eds_object = eds_entry(EdsLib.PackedObject(raw_message))
        return (topic_id, eds_entry, eds_object)


    def PauseListening(self):
        '''
        Sets the flag associated with the telemetry listening to false
        '''
        self.continue_listening = False


    def RestartListening(self):
        '''
        Sets the flag associated with the telemetry listening to true
        '''
        self.continue_listening = True


    def run(self):
        '''
        Method that is run when QThread spawns a new thread.
        This opens up a port to listen on.  When messages are received,
        they are decoded and sent to the data model for storage.
        '''
        # Init udp socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind(('', self.port))

        while True:
            if self.continue_listening:
                try:
                    # Receive message
                    datagram, host = sock.recvfrom(4096) # buffer size is 1024 bytes

                    # Ignore datagram if not long enough (i.e. doesn't contain a tlm header)
                    if len(datagram) < 6:
                        continue
                    
                    decode_output = self.DecodeMessage(datagram)
                    tlm_type, message = GS_Model.data.AddTlm(control.eds_db, host, datagram, decode_output)
                    if tlm_type is None:
                        self.signal.emit('', message)
                    else:
                        self.signal.emit(tlm_type, message)

                # Handle socket errors
                except socket.error:
                    print("Ignored socket error.")
                    time.sleep(1)
            else:
                time.sleep(1)


def ValidPayload(payload_entry, payload_value):
    '''
    Checks to see if a given payload value is valid based on the payload_entry function

    Inputs:
    payload_entry - EDS function to create the object that is filled by payload_value
    payload_value - The user input value to be checked if an EDS object can be created

    Outputs:
    Boolean value of the payload_value validity
    '''
    try:
        object_test = payload_entry[1](payload_value)
        return True
    except TypeError:
        return False


class Controller(object):
    '''
    The Controller class contains routines that interact with the EDS and MissionLib databases
        - Update Instance, Topic, and Subcommand dictionaries/lists
        - Generate, set, pack, and send telecommand messages
    '''
    def __init__(self):
        self.initialized = False

        self.mission = None
        self.eds_db = None
        self.intf_db = None
        self.telecommand = None
        self.telemetry = None

        self.cmd_entry = None
        self.cmd = None

        self.payload_entry = None
        self.payload = None
        self.payload_struct = None
        self.payload_values = None


    def InitializeDatabases(self, mission):
        '''
        Initialize the EDS and MissionLib databases as well as useful Interfaces
        and associated lists

        Inputs:
        mission - mission name
        '''
        if not self.initialized:
            try:
                # If the mission name is invlaid a RuntimeError will occur here
                self.eds_db = EdsLib.Database(mission)

                # Set the mission name and the rest of the CFE_MissionLib objects
                self.mission = mission
                self.intf_db = CFE_MissionLib.Database(self.mission, self.eds_db)
                self.telecommand = self.intf_db.Interface('CFE_SB/Telecommand')
                self.telemetry = self.intf_db.Interface('CFE_SB/Telemetry')

                # Call Data Model initialization function
                GS_Model.data.InitializeLists()

                self.initialized = True
                return True
            except RuntimeError:
                return False
        else:
            return True


    def GetInstances(self):
        '''
        Returns the instance dictionary based on the instances in the interface database
        '''
        instance_list_dict = {GS_Model.data.instance_chooser : 0}
        for instance in self.intf_db:
            instance_list_dict[instance[0]] = instance[1]
        return instance_list_dict


    def GetTelecommandTopics(self):
        '''
        Returns a dictionary of Telecommand topics
        '''
        topic_list_dict = {GS_Model.data.topic_chooser : 0}
        for topic in self.telecommand:
            topic_list_dict[topic[0]] = topic[1]
        return topic_list_dict


    def GetTelemetryTopics(self):
        '''
        Returns a dictionary of Telemetry topics
        '''
        topic_list_dict = {GS_Model.data.topic_chooser : 0}
        for topic in self.telemetry:
            topic_list_dict[topic[0]] = topic[1]
        return topic_list_dict


    def GetSubcommands(self, input_topic):
        '''
        Returns a dictionary of Subcommands based on a given telecommand topic

        Inputs:
        input_topic - User specified telecommand topic name
        '''
        subcommand_list_dict = {GS_Model.data.subcommand_chooser : 0}
        try:
            topic = self.telecommand.Topic(input_topic)
            for subcommand in topic:
                subcommand_list_dict[subcommand[0]] = subcommand[1]
        except RuntimeError:
            pass
        return subcommand_list_dict


    def GetEdsIdFromTopic(self, topic_name):
        '''
        Returns the EdsId associated with a given topic name

        Inputs:
        topic_name - Telecommand topic name
        '''
        topic = self.telecommand.Topic(topic_name)
        return topic.EdsId


    def GetPayloadStruct(self, base_entry, base_object, base_name):
        '''
        Recursive function that goes through an EDS object structure (arrays and structs)
        To get down to the fundamental objects (ints, strings, and enumerations).

        Inputs:
        base_entry - EDS fucntion to create the base_object
        base_object - EDS Object that is iterated over to find the structure
        base_name - Name used in the recursion to get the full name of a fundamental object

        Outputs:
        EDS Object data structure
        '''
        struct = {}

        # Arrays
        if (self.eds_db.IsArray(base_object)):

            # Get the type of an array element
            array_type_split = str(type(base_object[0])).split("'")
            array_entry = EdsLib.DatabaseEntry(array_type_split[1], array_type_split[3])
            array_object = array_entry()

            # Loop over all the aray elements
            struct = []
            struct_name = base_name + array_entry.Name
            for i in range(len(base_object)):
                struct_name = f"{base_name}[{i}]"
                array_struct = self.GetPayloadStruct(array_entry, array_object, struct_name)
                struct.append(array_struct)

        # Containers
        elif (self.eds_db.IsContainer(base_object)):

            # Iterate over the subobjects within the container
            for subobj in base_object:
                for subentry in base_entry:
                    if subobj[0] == subentry[0]:
                        entry_eds = EdsLib.DatabaseEntry(subentry[1], subentry[2])
                        struct_name = f"{base_name}.{subobj[0]}"
                        struct[subobj[0]] = self.GetPayloadStruct(entry_eds, subobj[1], struct_name)

        # Enumeration
        elif (self.eds_db.IsEnum(base_entry)):

            struct = ()
            enum_dict = {}
            # Iterate over the Enumeration labels
            for enum in base_entry:
                enum_dict[enum[0]] = enum[1]
                struct = (base_name, base_entry, 'enum', enum_dict)

        # Anything left over uses an entry field
        else:
            struct = (base_name, base_entry, 'entry', None)

        return struct


    def GetPayload(self, eds_id):
        '''
        From a given eds_id this checks if a payload is present.  If so, payload structure
        fuctions are called which are used in the Viewer to get all the appropriate user
        input widgets as well as creating the Payload structure itself.

        Inputs:
        eds_id - The EdsId associated with the command object

        Outputs:
        The payload structure of the EDS object associated with the input EdsId
        '''
        self.cmd_entry = EdsLib.DatabaseEntry(self.mission, eds_id)
        self.cmd = self.cmd_entry()
        payload_item = None

        for item in self.cmd_entry:
            if item[0] == 'Payload':
                payload_item = item

        if payload_item is not None:
            self.payload_entry = EdsLib.DatabaseEntry(payload_item[1], payload_item[2])
            self.payload = self.payload_entry()
            self.payload_struct = self.GetPayloadStruct(self.payload_entry, self.payload,
                                                        payload_item[0])

        else:
            self.payload_entry = None
            self.payload = None
            self.payload_struct = None

        return self.payload_struct


    def SetPubSub(self, instance_id, topic_id):
        '''
        Sets the Publisher/Subscribe parameters in a command header based on the instance_id
        and topic_id.  We call the function SetPubSub defined in the CFE_Missionlib
        python bindings that set the header values of the cmd message based on the
        CFE_MissionLib runtime library.

        Inputs:
        instance_id - The ID associated with the desitination core flight instance
        topic_id - The ID associated with the desired telecommand topic
        '''
        self.intf_db.SetPubSub(instance_id, topic_id, self.cmd)
        self.cmd.CCSDS.SeqFlag = 3                     # SeqFlag is hardcoded to 3 in cmdUtil.c


    def SetPayloadValues(self, structure):
        '''
        Traverses through the payload structure found in GetPayloadStruct to create a payload
        structure and fill it with the user supplied values.

        Inputs:
        Structure - The payload structure returned from GetPayloadStruct

        Outputs:
        Python structure that can be used to fill the Payload of the EDS Command object
        '''
        if isinstance(structure, dict):
            result = {}
            for item in list(structure.keys()):
                result[item] = self.SetPayloadValues(structure[item])
        elif isinstance(structure, list):
            result = []
            for item in structure:
                result.append(self.SetPayloadValues(item))
        elif isinstance(structure, tuple):
            result = structure[1](self.payload_values[structure[0]])
        else:
            print("Something went wrong in the SetPayloadValues function")
            result = None
        return result


    def SendCommand(self, ip_address, base_port, instance_name, topic_name, subcommand_name, payload_values):
        '''
        Sends a command message to an instance of core flight
            - Checks to make sure all required parameters are set
            - Creates the EDS command object and sets the necessary header parameters
            - Generates and sets the payload values (if necessary)
            - opens up a socket and sends the packed message

        Inputs:
        ip_address - The destination IP Address
        base_port - The base port used to send the command
        instance_name - The name of the core flight instance to send the command message
        topic_name - The name of the Telecommand topic to send
        subcommand_name - The name of the subcommand to the telecommand topic
        payload_values - list of user supplied payload values

        Outputs:
        A packed bytes message sent via UDP to an instance of core flight
        Tuple that contains:
            A flag if the message was successful
            A hex representation of the command message that was sent
            A timestamp of when the message was sent
            The port the command message was sent to
        '''

        if instance_name == GS_Model.data.instance_chooser:
            return(False, "Please Choose an Instance")
        if topic_name == GS_Model.data.topic_chooser:
            return(False, "Please Choose a Topic")
        if (subcommand_name == GS_Model.data.subcommand_chooser and
                len(GS_Model.data.subcommand_keys) > 1):
            return(False, "Please Choose a Subcommand")

        instance_id = GS_Model.data.instance_dict[instance_name]
        topic_id = GS_Model.data.telecommand_topic_dict[topic_name]

        self.cmd = self.cmd_entry()
        self.SetPubSub(instance_id, topic_id)

        self.payload_values = payload_values
        if len(self.payload_values) != 0:

            eds_payload = self.SetPayloadValues(self.payload_struct)
            self.payload = self.payload_entry(eds_payload)

            self.cmd['Payload'] = self.payload

        cmd_packed = EdsLib.PackedObject(self.cmd)
        port = base_port + instance_id - 1

        try:
            opened_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            opened_socket.sendto(bytes(cmd_packed), (ip_address, port))
            time_str = time.strftime("%Y-%m-%d__%H_%M_%S", time.gmtime())
            return(True, cmd_packed.hex(), time_str, port)
        except socket.error:
            return(False, "Failed to send message.")


control = Controller()
