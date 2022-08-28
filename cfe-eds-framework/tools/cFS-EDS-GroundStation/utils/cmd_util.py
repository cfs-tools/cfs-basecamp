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
cmd_util.py
This module allows a user to send a packed command message to an instance of core flight
The lists of instances, topics, subcommands, and payload entries are automatically generated
'''
import socket

import EdsLib
import CFE_MissionLib


def set_pubsub(intf_db, instance_id, topic_id, cmd):
    '''
    Calls the SetPubSub method defined in the CFE_MissionLib python bindings
    This function takes the instance and topic ids and sets the appropriate 
    spacepacket header values in the cmd message based on the CFE_MissionLib
    runtime library.

    Inputs:
    intf_db - CFE_MissionLib Interface Database
    instance_id - Id associated with a core flight instance
    topic_id - Id associated with a telecommand topic
    cmd - EDS object of the command structure
    '''
    intf_db.SetPubSub(instance_id, topic_id, cmd)
    cmd.CCSDS.SeqFlag = 3             # SeqFlag is hardcoded to 3 in cmdUtil.c


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


def get_payload_struct(eds_db, base_entry, base_object, base_name):
    '''
    Recursive function that goes through an EDS object structure (arrays and structs)
    To get down to the fundamental objects (ints, strings, enumerations).

    Inputs:
    eds_db - EDS database
    base_entry - EDS fucntion to create the base_object
    base_object - EDS Object that is iterated over to find the structure
    base_name - Name used in the recursion to get the full name of a fundamental object

    Outputs:
    EDS Object data structure
    '''
    struct = {}

    # Arrays
    if (eds_db.IsArray(base_object)):

        # Get the type of an array element
        array_type_split = str(type(base_object[0])).split("'")
        array_entry = EdsLib.DatabaseEntry(array_type_split[1], array_type_split[3])
        array_object = array_entry()

        # Loop over all the aray elements
        struct = []
        struct_name = base_name + array_entry.Name
        for i in range(len(base_object)):
            struct_name = f"{base_name}[{i}]"
            array_struct = get_payload_struct(eds_db, array_entry, array_object, struct_name)
            struct.append(array_struct)

    # Containers
    elif (eds_db.IsContainer(base_object)):

        # Iterate over the subobjects within the container
        for subobj in base_object:
            for subentry in base_entry:
                if subobj[0] == subentry[0]:
                    entry_eds = EdsLib.DatabaseEntry(subentry[1], subentry[2])
                    struct_name = f"{base_name}.{subobj[0]}"
                    struct[subobj[0]] = get_payload_struct(eds_db, entry_eds, subobj[1], struct_name)

    # Enumeration
    elif (eds_db.IsEnum(base_entry)):

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


def set_payload_values(structure):
    '''
    Iterating over the payload structure from get_payload_structure function,
    this create a payload object that fills in the payload of the cmd object.

    Input:
    structure - the result structure from get_payload_structure

    Output:
    result - payload structure to fill in the cmd object
    '''
    if isinstance(structure, dict):
        result = {}
        for item in list(structure.keys()):
            result[item] = set_payload_values(structure[item])
    elif isinstance(structure, list):
        result = []
        for item in structure:
            result.append(set_payload_values(item))
    elif isinstance(structure, tuple):
        if structure[2] == 'enum':
            print()
            for key in list(structure[3].keys()):
                print(key)
        while True:
            value = input("\nFor {} ({}) Enter Value > ".format(structure[0], structure[1]))
            try:
                result = structure[1](value)
                break
            except TypeError:
                print("Invalid value for {}".format(structure[0]))
                continue
    else:
        print("Something went wrong in the Set Payload Values function")
        result = None
    return result


def get_payload(eds_db, cmd_entry):
    '''
    Iterating over the command entry object, check to see if a payload is needed.
    If so, the user is prompted to fill in the required payload values.

    Inputs:
    eds_db - EDS database
    cmd_entry - EDS function to create the command object associated with the topic/subcommand

    Outputs:
    payload - EDS object of the command's payload filled in by the user
    '''
    is_payload = False
    for item in cmd_entry:
        if item[0] == 'Payload':
            payload_item = item
            is_payload = True

    if is_payload:
        # Use the information from the database entry iterator to get a payload Entry and object
        payload_entry = EdsLib.DatabaseEntry(payload_item[1], payload_item[2])
        payload = payload_entry()

        payload_struct = get_payload_struct(eds_db, payload_entry, payload, 'Payload')
        eds_payload = set_payload_values(payload_struct)
        payload = payload_entry(eds_payload)
    else:
        payload = None

    return payload


def get_cmd_entry(eds_db, topic):
    '''
    This routine checks if the input topic has any subcommands, and if so, prompts
    the user to select a command.  Otherwise, the command is based on the topic itself

    Inputs:
    eds_db - EDS Database object
    topic - a CFE_MissionLib interface topic

    Outputs:
    cmd_entry - EDS function to create the command object associated with the topic/subcommand
    '''
    try:
        subcommand_list = {}
        for subcommand in topic:
            subcommand_list[subcommand[0]] = subcommand[1]

        print("Subcommand List:")
        for subcommand in topic:
            print(subcommand[0])

        while True:
            subcommand_name = input("\nSelect Subcommand > ")
            try:
                subcommand_eds_id = subcommand_list[subcommand_name]
                break
            except KeyError:
                print("Invalid Subcommand")

        cmd_entry = EdsLib.DatabaseEntry(eds_db, subcommand_eds_id)

    # If the Topic doesn't have subcommands then the first for loop over will return
    # with a runtime error: use the EdsId from the Topic itself instead
    except RuntimeError:
        cmd_entry = EdsLib.DatabaseEntry(eds_db, topic.EdsId)

    return cmd_entry


def get_topic_id(interface):
    '''
    Iterating over the provided interface to get the list of topics,
    this routine lets the user pick which topic to use

    Inputs:
    interface - CFE_MissionLib Interface ("CFE_SB/Telecommand")

    Outputs:
    topic_id - The ID associated with the desired topic
    '''
    # Set up the topic list and print it out for the user
    print("TopicList:")
    topic_list = {}
    for topic in interface:
        topic_list[topic[0]] = topic[1]
        print(topic[0])

    while True:
        topic_name = input("\nSelect topic > ")
        try:
            topic_id = topic_list[topic_name]
            print(f'Topic selected {topic_id}\n')
            break
        except KeyError:
            print("Invalid Topic")
    return topic_id


def get_instance_id(intf_db):
    '''
    Iterating over the provided interface database to get the list of instances,
    this routine lets the user pick which instance to send a command to

    Inputs:
    intf_db - CFE_MissionLib Interface Database

    Outputs:
    instance_id - The ID associated with the desired instance
    '''
    print("\ncFS Instance List:")
    instance_list = {}
    for instance in intf_db:
        instance_list[instance[0]] = instance[1]
        print(instance[0])

    while True:
        instance_name = input("\nSelect cFS instance > ")
        try:
            instance_id = instance_list[instance_name]
            print(f'cFS Instance selected {instance_id}\n')
            break
        except KeyError:
            print("Invalid Instance")
    return instance_id



def main():
    '''
    With a series of prompts to select the instance, topic, and subcommand, along with prompts to
    fill in relevant payload values, this function allows a user to send a packed EDS command via
    UDP to a core flight instance.
    '''
    
    mission = "@CFS_EDS_GS_MISSION_NAME@".lower()
    try:
        # Initialize databases
        eds_db = EdsLib.Database(mission)
        intf_db = CFE_MissionLib.Database(mission, eds_db)
        interface = intf_db.Interface("CFE_SB/Telecommand")
    except RuntimeError:
        print("cmdUtil.py is not properly configured")
        sys.exit(2)

    # Get the instance and topic from user input
    instance_id = get_instance_id(intf_db)
    topic_id = get_topic_id(interface)

    # Get the topic from the interface database to check for subcommands
    topic = interface.Topic(topic_id)

    # Get the associated command with the desired topic/subcommand
    cmd_entry = get_cmd_entry(eds_db, topic)
    cmd = cmd_entry()

    # Call set_pubsub to set the relevant header parameters
    set_pubsub(intf_db, instance_id, topic_id, cmd)

    # Fill in the payload if required
    payload = get_payload(eds_db, cmd_entry)

    if not payload is None:
        cmd['Payload'] = payload

    cmd_packed = EdsLib.PackedObject(cmd)

    while True:
        dest = input("\nEnter destination IP (Press Enter for 127.0.0.1) > ")
        if dest == '':
            dest = '127.0.0.1'

        base_port = input("\nEnter base UDP port (Press Enter for 1234) > ")
        if base_port == '':
            base_port = 1234
        
        port = base_port + instance_id - 1
        
        try:
            opened_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            opened_socket.sendto(bytes(cmd_packed), (dest, port))

            print(f"\nSending data to '{dest}'; port {port}")
            print("Data to send:")
            print(hex_string(cmd_packed.hex(), 8))
            break
        except socket.error:
            print("Invalid IP address")



if __name__ == "__main__":
    main()
