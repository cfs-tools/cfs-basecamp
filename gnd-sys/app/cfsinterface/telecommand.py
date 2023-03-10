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
        Define a Telecommand interface with the main function serving as a 
        command line utility.
    
"""

import configparser
import socket
import time
import sys
import os

import logging
logger = logging.getLogger(__name__)

if __name__ == '__main__' or 'cfsinterface' in os.getcwd():
    sys.path.append('..')
    from edsmission   import EdsMission
    from edsmission   import CfeEdsTarget
    from cmdtlmrouter import CmdTlmRouter
else:
    from .edsmission   import EdsMission
    from .edsmission   import CfeEdsTarget
    from .cmdtlmrouter import CmdTlmRouter
from tools import hex_string
 
###############################################################################

class TelecommandInterface(CfeEdsTarget):
    """
    Manage an EDS-defined telecommand interface. It uses the EdsMission database for Telecommand
    message definitions and provides methods for loadng payload values and sending messages on 
    on a UDP socket. If needed, the communication design can be generalized and separated from
    this class.
    This class does not have any user interface dependencies and should be usable in GUI,
    command line, and scripted scenarios.   
    """
    @staticmethod
    def valid_payload(payload_entry, payload_value):
        """
        Checks to see if a given payload value is valid based on the payload_entry function

        Inputs:
        payload_entry - EDS function to create the object that is filled by payload_value
        payload_value - The user input value to be checked if an EDS object can be created

        Outputs:
        Boolean value of the payload_value validity
        """
        try:
            #todo: object_test = payload_entry[1](payload_value)
            object_test = payload_entry(payload_value)
            return True
        except TypeError:
            return False

    def __init__(self, mission, target, cmd_router_queue):
        super().__init__(mission, target, EdsMission.TELECOMMAND_IF)

        self.cmd_router_queue = cmd_router_queue

        # command_topic contains the topic name used to generate the current command_dict
        self.command_topic = EdsMission.TOPIC_CMD_TITLE_KEY
        self.command_dict  = {EdsMission.COMMAND_TITLE_KEY: EdsMission.NULL_ID}
        self.command_list  = [EdsMission.COMMAND_TITLE_KEY]
    
        self.cmd_entry = None
        self.cmd_obj   = None
    

    def get_topic_id(self, topic_name):

        topic_id = EdsMission.NULL_ID
        topic_text = ""
        try:
            topic_id = self.topic_dict[topic_name]
        except KeyError:
            topic_text = "Error retrieving topic %s from current target %s" % (topic_name, self.target_name)
            
        return topic_id, topic_text
        
        
    def get_topic_commands(self, topic_name):
        """
        Return a dictionary of commands based on a given telecommand topic
        """
        logger.debug("self.topic_dict = " + str(self.topic_dict))
        topic_id = self.topic_dict[topic_name]
        
        self.command_dict = {EdsMission.COMMAND_TITLE_KEY: EdsMission.NULL_ID}
        try:
            topic_obj = self.eds_mission.interface.Topic(topic_id)
            for command in topic_obj:
                self.command_dict[command[0]] = command[1]
            self.command_topic = topic_name
        except RuntimeError:
            pass
            
        return self.command_dict


    def get_cmd_id(self, command_name):

        command_id = EdsMission.NULL_ID
        command_text = ""
        try:
            command_id = self.command_dict[command_name]
        except KeyError:
            command_text = "Error retrieving command %s from current topic %s" % (command_name, self.command_topic)
            
        return command_id, command_text
        

    def get_cmd_entry(self, topic_name, command_name):
        """
        """
                
        cmd_valid = True
        
        #todo: Decide how class variables are used. Could use self.command_dict if assume its been loaded with current topic
        command_dict = self.get_topic_commands(topic_name)

        if len(command_dict) > 1:
            try:
                command_id = command_dict[command_name]
                self.cmd_entry = self.eds_mission.get_database_entry(command_id)
                self.cmd_obj   = self.cmd_entry()
            except KeyError:
                cmd_valid = False
                self.cmd_entry = None
                self.cmd_obj   = None
        else:
            eds_id = self.eds_mission.get_eds_id_from_topic(topic_name)
            self.cmd_entry = self.eds_mission.get_database_entry(eds_id)
            self.cmd_obj   = self.cmd_entry()
            
        return (cmd_valid, self.cmd_entry, self.cmd_obj)


    def set_cmd_hdr(self, topic_id, cmd_obj):
        """
        
        """
        self.eds_mission.cfe_db.SetPubSub(self.id, topic_id, cmd_obj)
        cmd_obj.CCSDS.SeqFlag = 3


    def get_cmd_entry_payload(self,cmd_entry):
        logger.debug("has_payload() - cmd_entry = " + str(cmd_entry))
        #todo: Remove loop if possible
        has_payload = False
        payload_item = None
        for item in cmd_entry:
            if item[0] == 'Payload':
                payload_item = item
                has_payload = True
        return has_payload, payload_item
        
               
    def get_payload_struct(self, base_entry, base_object, base_name):
        """
        Recursive function that goes through an EDS object structure (arrays and structs)
        To get down to the fundamental objects (ints, strings, enumerations).

        Inputs:
            eds_db - EDS database
            base_entry - EDS fucntion to create the base_object
            base_object - EDS Object that is iterated over to find the structure
            base_name - Name used in the recursion to get the full name of a fundamental object

        Outputs:
            EDS Object data structure
        """
        struct = {}

        # Arrays
        if (self.eds_mission.lib_db.IsArray(base_object)):

            # Get the type of an array element
            array_type_split = str(type(base_object[0])).split("'")
            logger.debug("array_type_split[1] = " + str(array_type_split[1]))
            logger.debug("array_type_split[3] = " + str(array_type_split[3]))
            array_entry = self.eds_mission.get_database_named_entry(array_type_split[3])
            #todo: array_entry = self.eds_mission.lib_db.DatabaseEntry(array_type_split[1], array_type_split[3])
            array_object = array_entry()

            # Loop over all the aray elements
            struct = []
            struct_name = base_name + array_entry.Name
            for i in range(len(base_object)):
                struct_name = f"{base_name}[{i}]"
                array_struct = self.get_payload_struct(array_entry, array_object, struct_name)
                struct.append(array_struct)

        # Containers
        elif (self.eds_mission.lib_db.IsContainer(base_object)):

            # Iterate over the subobjects within the container
            for subobj in base_object:
                for subentry in base_entry:
                    if subobj[0] == subentry[0]:
                        logger.debug("subentry[1] = " + str(subentry[1]))
                        logger.debug("subentry[2] = " + str(subentry[2]))
                        entry_eds = self.eds_mission.get_database_named_entry(subentry[2])
                        #todo: entry_eds = self.eds_mission.lib_db.DatabaseEntry(subentry[1], subentry[2])
                        struct_name = f"{base_name}.{subobj[0]}"
                        struct[subobj[0]] = self.get_payload_struct(entry_eds, subobj[1], struct_name)

        # Enumeration
        elif (self.eds_mission.lib_db.IsEnum(base_entry)):

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


    def set_payload_values(self, structure):
        """
        Iterating over the payload structure from get_payload_structure function,
        this create a payload object that fills in the payload of the cmd object.

        Input:
        structure - the result structure from get_payload_structure

        Output:
        result - payload structure to fill in the cmd object
        """
        if isinstance(structure, dict):
            logger.debug("Dictionary struct = " + str(structure))
            result = {}
            for item in list(structure.keys()):
                result[item] = self.set_payload_values(structure[item])
        elif isinstance(structure, list):
            logger.debug("List struct = " + str(structure))
            result = []
            for item in structure:
                result.append(self.set_payload_values(item))
        elif isinstance(structure, tuple):
            #structure = [payload_name, payload_eds_entry, payload_type, payload_list]
            logger.debug("Tuple struct = " + str(structure))
            result = self.load_payload_entry_value(structure[0],structure[1],structure[2],structure[3])
            logger.debug("@@@result = " + str(result))
        else:
            #todo: Return errors and strings to keep this independent of the user interface 
            logger.debug("Something went wrong in the Set Payload Values function")
            result = None
        
        return result


    def remove_eds_payload_name_prefix(self, eds_name):
        """
        Strip the 'Payload' prefix from an EDS payload name so only the payload
        name is used for the GUI
        """        
        return eds_name[eds_name.find('.')+1:]
        
            
    def load_payload_entry_value(self, payload_eds_name, payload_eds_entry, payload_type, payload_list):
        raise NotImplementedError

    
    def send_command(self, cmd_obj):
        """
         
        """
        cmd_packed = self.eds_mission.get_packed_obj(cmd_obj)

        cmd_sent   = True
        cmd_text   = cmd_packed.hex()
        cmd_status = "Sent command " + self.cmd_entry.Name
        
        self.cmd_router_queue.put(bytes(cmd_packed))

        return (cmd_sent, cmd_text, cmd_status)
        """
        try:
            self.cmd_router_queue.put(bytes(cmd_packed))
            self.socket.sendto(bytes(cmd_packed), self.cmd_ip_address)
        except:
            cmd_sent = False
            cmd_status = "Failed to send command on socket to %s:%d" % self.cmd_ip_address

        return (cmd_sent, cmd_text, cmd_status)
        """

###############################################################################

class TelecommandScript(TelecommandInterface):
    """
    Target designed to support scripts.
    """

    def __init__(self, mission, target, cmd_router_queue):
        super().__init__(mission, target, cmd_router_queue)

        self.cmd_payload = {}

        
    def load_payload_entry_value(self, payload_eds_name, payload_eds_entry, payload_type, payload_list):

        logger.debug("payload_eds_name = " + payload_eds_name)
        logger.debug("self.payload.keys() = " + str(self.cmd_payload.keys()))
        
        payload_name = self.remove_eds_payload_name_prefix(payload_eds_name)
        
        if payload_name in self.cmd_payload:
           result = self.cmd_payload[payload_name]
        else:
           result = ""

        return result


    def send_cfs_cmd(self, app_name, cmd_name, cmd_payload):
    
        self.cmd_payload = cmd_payload
        
        topic_name = app_name.upper() + self.eds_mission.APP_CMD_TOPIC_SUFFIX 
        topic_id, topic_text = self.get_topic_id(topic_name)
            
        cmd_valid, cmd_entry, cmd_obj = self.get_cmd_entry(topic_name, cmd_name)
        
        cmd_sent   = False
        cmd_text   = "%s: %s" % (app_name,cmd_name)
        cmd_status = "Error sending %s's %s command" % (app_name,cmd_name)
        
        if cmd_valid:    
            self.set_cmd_hdr(topic_id, cmd_obj)

            cmd_has_payload, cmd_payload_item = self.get_cmd_entry_payload(cmd_entry)
                
            if cmd_has_payload:
                
                payload_entry = self.eds_mission.get_database_named_entry(cmd_payload_item[2])
                payload = payload_entry()

                payload_struct = self.get_payload_struct(payload_entry, payload, 'Payload')
                eds_payload = self.set_payload_values(payload_struct)
                payload = payload_entry(eds_payload)

                cmd_obj['Payload'] = payload
        
            (cmd_sent, cmd_text, cmd_status) = self.send_command(cmd_obj)
            
            if cmd_sent == True:
                cmd_status = "%s %s command sent" % (app_name, cmd_name)
                logger.debug(hex_string(cmd_text, 8))        
            else:
                logger.info(cmd_status)
        
        
        return (cmd_sent, cmd_text, cmd_status)


###############################################################################

class TelecommandCmdLine(TelecommandInterface):
    """
    Command line tool to interact with a user to manually send commands to a cFS target. Helpful
    for informal verification of a system configuration.
    """

    def __init__(self, mission, target, cmd_router_queue):
        super().__init__(mission, target, cmd_router_queue)


    def load_payload_entry_value(self, payload_eds_name, payload_eds_entry, payload_type, payload_list):
   
        if payload_type == 'enum':
            print()
            for key in list(payload_list.keys()):
                print(key)
    
        while True:
    
            result = None
            value = input("\nFor {} ({}) Enter Value > ".format(payload_eds_name, payload_eds_entry))
            try:
                result = payload_eds_entry(value)
                break
            except TypeError:
                print("Invalid value for {}".format(payload_eds_name))
                continue

        return result


    def send_user_command(self):
    
        topic_dict = self.get_topics()
        logger.debug("topics = " + str(topic_dict))
        print("Topic List:")
        topic_list = []
        user_topic_id = 0
        for topic in topic_dict.keys():
            topic_list.append(topic)
            print("%2d: %s" % (user_topic_id,topic))
            user_topic_id += 1
    
        eds_topic_id = EdsMission.NULL_ID
        while True:
            user_id = int(input("\nInput numeric topic ID> "))
            if user_id > 0 and user_id < user_topic_id:
                topic_name = topic_list[user_id]
                topic_id, topic_text = self.get_topic_id(topic_name)
                print("Selected topic %s with EDS ID %d" % (topic_name, topic_id))
                break
            else:
                print("Aborted topic selection")
                break

        command_dict = self.get_topic_commands(topic_name)
        logger.debug("commands = " + str(command_dict))
        if len(command_dict) > 1:
    
            print("Command List:")
            command_list = []
            user_command_id = 0
            for command in command_dict.keys():
                command_list.append(command)
                print("%2d: %s" % (user_command_id,command))
                user_command_id += 1

            while True:
                user_id = int(input("\nInput numeric command ID> "))
                if user_id > 0 and user_id < user_command_id:
                    command_name = command_list[user_id]
                    command_id, command_text = self.get_cmd_id(command_name)
                    print("Selected command %s with EDS ID %d" % (command_name, command_id))
                    break
                else:
                    print("Aborted command selection")
                    break

        (cmd_valid, cmd_entry, cmd_obj) = self.get_cmd_entry(topic_name, command_name)

        if cmd_valid == True:
    
            logger.debug("self.cmd_entry = " + str(cmd_entry))
            logger.debug("self.cmd_obj = " + str(cmd_obj))

            self.set_cmd_hdr(topic_id, cmd_obj)

            cmd_has_payload, cmd_payload_item = self.get_cmd_entry_payload(cmd_entry)
            
            if cmd_has_payload:
            
                # Use the information from the database entry iterator to get a payload Entry and object
                logger.debug("cmd_payload_item[1] = " + str(cmd_payload_item[1]))
                logger.debug("cmd_payload_item[2] = " + str(cmd_payload_item[2]))
                #todo: payload_entry = self.eds_mission.lib_db.DatabaseEntry(cmd_payload_item[1], cmd_payload_item[2])
                payload_entry = self.eds_mission.get_database_named_entry(cmd_payload_item[2])
                payload = payload_entry()

                payload_struct = self.get_payload_struct(payload_entry, payload, 'Payload')
                eds_payload = self.set_payload_values(payload_struct)
                payload = payload_entry(eds_payload)

                cmd_obj['Payload'] = payload
    
            (cmd_sent, cmd_text, cmd_status) = self.send_command(cmd_obj)
    
            if cmd_sent == True:
                print(hex_string(cmd_text, 8))
            else:
                print(cmd_text)

        else:    
            
            print("Error retrieving command %s using topic ID %d" % (command_name, topic_id)) 
    

    def execute(self):

        while True:
            self.send_user_command()
            input_str = input("\nPress <Enter> to send another command. Enter any character to exit> ")
            if len(input_str) > 0:
                break

###############################################################################

def main():
    
    config = configparser.ConfigParser()
    config.read('../basecamp.ini')
    MISSION         = config.get('CFS_TARGET', 'MISSION_EDS_NAME')
    CFS_TARGET      = config.get('CFS_TARGET', 'CPU_EDS_NAME')
    CFS_IP_ADDR     = config.get('NETWORK', 'CFS_IP_ADDR')
    CFS_CMD_PORT    = config.getint('NETWORK', 'CFS_CMD_PORT')
    GND_IP_ADDR     = config.get('NETWORK', 'GND_IP_ADDR')
    GND_CMD_PORT    = config.getint('NETWORK','CMD_TLM_ROUTER_CMD_PORT')
    GND_TLM_PORT    = config.getint('NETWORK', 'GND_TLM_PORT')
    GND_TLM_TIMEOUT = float(config.getint('NETWORK', 'GND_TLM_TIMEOUT'))/1000.0

    system_string = f'Mission: {MISSION}, Target: {CFS_TARGET}, cFS: ({CFS_IP_ADDR}, {CFS_CMD_PORT}), Gnd: ({GND_IP_ADDR}, {GND_CMD_PORT}), {GND_TLM_PORT})'
    print(f'Creating telecommand objects for {system_string}')

    try:
        cmd_tlm_router       = CmdTlmRouter(CFS_IP_ADDR, CFS_CMD_PORT, GND_IP_ADDR, GND_CMD_PORT, GND_TLM_PORT, GND_TLM_TIMEOUT)
        telecommand_script   = TelecommandScript(MISSION, CFS_TARGET, cmd_tlm_router.get_cfs_cmd_queue())      
        telecommand_cmd_line = TelecommandCmdLine(MISSION, CFS_TARGET, cmd_tlm_router.get_cfs_cmd_queue())
        logger.info("Telecommand object created for " + system_string)
        
    except RuntimeError:
        print(f'Error creating telecommand object for {system_string}')
        sys.exit(2)

    cmd_tlm_router.start()
    telecommand_script.send_cfs_cmd('KIT_TO','EnableOutputCmd',{'dest_IP':'127.0.0.1'})
    telecommand_cmd_line.execute()    
    cmd_tlm_router.shutdown()

if __name__ == '__main__':
    main()


