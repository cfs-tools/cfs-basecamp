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
import re

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
        topic_status = f'Retrieved topic {topic_name} from target {self.target_name}'
        
        try:
            topic_id = self.topic_dict[topic_name]
        except KeyError:
            topic_status = f'Error retrieving topic {topic_name} from target {self.target_name}'
            
        return topic_id, topic_status
        
        
    def get_topic_commands(self, topic_name):
        """
        Return a dictionary of commands based on a given telecommand topic
        """
        logger.debug("self.topic_dict = " + str(self.topic_dict))
        topic_id, topic_status = self.get_topic_id(topic_name)
        
        self.command_dict = {EdsMission.COMMAND_TITLE_KEY: EdsMission.NULL_ID}
        if topic_id != EdsMission.NULL_ID:
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
        command_status = f'Retrieved command {command_name} from topic {self.command_topic}'
        
        try:
            command_id = self.command_dict[command_name]
        except KeyError:
            command_text = f'Error retrieving command {command_name} from topic {self.command_topic}'
            
        return command_id, command_status
        

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
        
               
    def get_cfs_cmd_obj(self, app_name, cmd_name, cmd_payload):
    
        self.cmd_payload = cmd_payload
        
        cmd_valid  = False
        cmd_status = f'Error creating {app_name}/{cmd_name} command'
            
        topic_name = app_name.upper() + self.eds_mission.APP_CMD_TOPIC_SUFFIX 
        topic_id, topic_status = self.get_topic_id(topic_name)

        if topic_id == EdsMission.NULL_ID:
            cmd_status = cmd_status + ': ' + topic_status       
        
        else:
            cmd_valid, cmd_entry, cmd_obj = self.get_cmd_entry(topic_name, cmd_name)
            
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
                
                cmd_status = f'Successfully created {app_name}/{cmd_name} command'
                
        return (cmd_valid, cmd_status, cmd_obj)

    def get_cmd_obj_name(self, cmd_obj):
        """
        Assumes a valid cmd_obj
        The cmd_obj class name contains the command name. For exmAple
            EdsLib.DatabaseEntry('basecamp','KIT_TO/Noop')
        """
        return re.findall("'([^']*)'", str(cmd_obj.__class__))[1]

    def remove_eds_payload_name_prefix(self, eds_name):
        """
        Strip the 'Payload' prefix from an EDS payload name so only the payload
        name is used for the GUI
        """        
        return eds_name[eds_name.find('.')+1:]
        

    def send_command(self, cmd_obj):
        """
        """
        cmd_packed = self.eds_mission.get_packed_obj(cmd_obj)

        cmd_sent   = True
        cmd_text   = cmd_packed.hex()
        
        self.cmd_router_queue.put(bytes(cmd_packed))

        return (cmd_sent, cmd_text)
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
    
        cmd_sent   = False
        cmd_text   = f'{app_name}/{cmd_name}'
        cmd_status = f'Error sending {cmd_text} command'
        
        cmd_valid, cmd_status, cmd_obj = self.get_cfs_cmd_obj(app_name, cmd_name, cmd_payload)
    
        if cmd_valid:
            
            (cmd_sent, cmd_text) = self.send_command(cmd_obj)
            
            if cmd_sent == True:
                cmd_status = f'Sent {app_name}/{cmd_name} command'
                logger.debug(hex_string(cmd_text, 8))        
            else:
                logger.info(cmd_status)
                
        return (cmd_sent, cmd_text, cmd_status)


    def send_cfs_cmd_old(self, app_name, cmd_name, cmd_payload):
    
        self.cmd_payload = cmd_payload
        
        cmd_sent   = False
        cmd_text   = f'{app_name}/{cmd_name}'
        cmd_status = f'Error sending {cmd_text}'

        topic_name = app_name.upper() + self.eds_mission.APP_CMD_TOPIC_SUFFIX 
        topic_id, topic_status = self.get_topic_id(topic_name)

        if topic_id == EdsMission.NULL_ID:
            cmd_status = cmd_status + ': ' + topic_status       
        
        else:
            cmd_valid, cmd_entry, cmd_obj = self.get_cmd_entry(topic_name, cmd_name)
            
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
            
                (cmd_sent, cmd_text) = self.send_command(cmd_obj)
                
                if cmd_sent == True:
                    cmd_status = f'Sent {app_name}/{cmd_name} command'
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
                topic_id, topic_status = self.get_topic_id(topic_name)
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

        # TODO: Consider using get_cfs_cmd_obj()
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
    
            (cmd_sent, cmd_text) = self.send_command(cmd_obj)
    
            cmd_name = self.get_cmd_obj_name(cmd_obj)
            if cmd_sent == True:
                print(f'Sent {cmd_name} command')
                print(hex_string(cmd_text, 8))
            else:
                print(f'Error sending {cmd_name} command')

        else:
            print(f'Error retrieving command {command_name} using topic ID {topic_id}')
    

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
    GND_CMD_PORT    = config.getint('NETWORK','CMD_TLM_ROUTER_CTRL_PORT')
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


