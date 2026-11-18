'''
Copyright (c) 2025 United States Government as represented by
the Administrator of the National Aeronautics and Space Administration.
All Rights Reserved.
'''

import EdsLib
import CFE_MissionLib

class ControllerSync:
    '''
    The Controller class contains routines that interact with the EDS and MissionLib databases
        - Update Instance, Topic, and Subcommand dictionaries/lists
        - Generate, set, pack, and send telecommand messages
    '''
    def __init__(self, mission_name:str='samplemission'):
        self.initialized = False

        # If the mission name is invlaid a RuntimeError will occur here
        self.eds_db = EdsLib.Database(mission_name)

        # Set the mission name and the rest of the CFE_MissionLib objects
        self.mission_name = mission_name
        self.intf_db = CFE_MissionLib.Database(self.mission_name, self.eds_db)
        self.telecommand = self.intf_db.Interface('CFE_SB/Telecommand')
        self.telemetry = self.intf_db.Interface('CFE_SB/Telemetry')

        self.tlm_dict = None
        self.cmd_dict = None
        self.all_topics = None

        self.last_sequence = dict()

    def get_telecommand_topics(self):
        '''
        Returns a dictionary of Telecommand topics
        '''
        print ("Inside get_telecommand_topics")
        if self.cmd_dict is None:
            self.cmd_dict = dict()  # Initialize topic_list_dict

            for topic in self.telecommand:
                self.cmd_dict[topic[0]] = self.telecommand.Topic(topic[0])


        return self.cmd_dict


    def get_telemetry_topics(self):
        '''
        Returns a dictionary of Telemetry topics
        '''
        print ("Inside get_telemetry_topics")
        if self.tlm_dict is None:
            self.tlm_dict = dict()  # Initialize topic_list_dict

            for topic in self.telemetry:
                self.tlm_dict[topic[0]] = self.telemetry.Topic(topic[0])

        return self.tlm_dict

    def get_all_topics(self):
        if self.all_topics is None:
            self.all_topics = {**self.get_telecommand_topics(), **self.get_telemetry_topics()} # merge dictionaries

        return self.all_topics

    def get_subcommands(self, topic_name:str):
        '''
        Returns a dictionary of Subcommands based on a given telecommand topic

        Inputs:
        topic_name - User specified telecommand topic name
        '''
        subcommand_list_dict = dict()  # Initialize subcommand_list_dict
        try:
            topic = self.telecommand.Topic(topic_name)
            for subcommand in topic:
                subcommand_list_dict[subcommand[0]] = subcommand[1]
        except RuntimeError:
            pass
        return subcommand_list_dict

    def lookup_topic_name(self, topic_name:str):
        topic_obj = None
        all_topics = self.get_all_topics()

        if topic_name in all_topics:
            topic_obj = all_topics[topic_name]

        return topic_obj

    def get_eds_id_from_topic(self, topic_name:str):
        '''
        Returns the EdsId associated with a given topic name

        Inputs:
        topic_name - Telecommand topic name
        '''
        result = self.lookup_topic_name(topic_name)
        if result:
            result = result.EdsId
        return result

    def get_msg_id_from_topic(self, topic_name:str, instance_id=1):
        '''
        Returns the MsgId associated with a given topic name

        Inputs:
        topic_name - Telecommand topic name
        '''
        result = None
        topic_obj = self.lookup_topic_name(topic_name)
        if topic_obj:
            result = self.intf_db.GetMsgId(instance_id, topic_obj)
        return result

    def identify_datagram(self, datagram:bytes):
        topic_name = None
        eds_id, topic_id = self.intf_db.DecodeEdsId(datagram)
        msg_datatype = self.eds_db.Entry(eds_id)

        # most items expected to be telemetry so try that first
        topic_obj = self.telemetry.Topic(topic_id)
        if not topic_obj:
            # must be a command
            topic_obj = self.telecommand.Topic(topic_id)

        # get the name associated with this topic
        if topic_obj:
            topic_name = topic_obj.Name

        # instantiating an object of the decoded type performs decoding
        decoded_obj = msg_datatype(EdsLib.PackedObject(datagram))
        return decoded_obj, topic_name

    def get_sequence(self, topic_name:str, decoded_data) -> int:
        # note that these sequence numbers are mod 2^14, they wrap from 2^14-1 to 0
        this_seq = decoded_data['CCSDS']['Sequence'] # this gives back an edslib ref
        this_seq = this_seq() & 0x3FFF # "calling" the ref converts to plain int
        if topic_name in self.last_sequence:
            seq_diff = this_seq - (self.last_sequence[topic_name] & 0x3FFF)
            if seq_diff < 0:
                seq_diff += 0x4000
            this_seq += seq_diff
        self.last_sequence[topic_name] = this_seq
        return this_seq

    def generate_message(self, topic_name:str, cmd_name:str=None) -> EdsLib.DatabaseEntry:
        topic_obj = self.get_all_topics()[topic_name]
        eds_id = topic_obj.EdsId
        if cmd_name:
            for sc_name,sc_id in topic_obj:
                if sc_name == cmd_name:
                    eds_id = sc_id
                    break

        msg_datatype = self.eds_db.Entry(eds_id)

        # this instantiates the message with default content
        return msg_datatype()

    def setup_header(self, msg_obj:EdsLib.DatabaseEntry, instance_name:str, topic_name:str) -> EdsLib.DatabaseEntry:
        '''
        Sets the Publish/Subscribe parameters in a command header based on the instance_id
        and topic_id.  We call the function SetPubSub defined in the CFE_Missionlib
        python bindings that set the header values of the cmd message based on the
        CFE_MissionLib runtime library.

        Inputs:
        instance_id - The ID associated with the desitination core flight instance
        topic_id - The ID associated with the desired telecommand topic
        '''
        topic_obj = self.get_all_topics()[topic_name]

        this_seq = 1
        if topic_name in self.last_sequence:
            this_seq += self.last_sequence[topic_name]

        self.last_sequence[topic_name] = this_seq

        # this sets the correct msg id
        self.intf_db.SetPubSub(instance_name, topic_obj.TopicId, msg_obj)
        msg_obj['CCSDS']['Sequence'] = this_seq & 0x3FFF
        msg_obj['CCSDS']['SeqFlag'] = 3  # SeqFlag is hardcoded for now, cfs does not fragment commands
        return self

    def encode_datagram(self, msg_data) -> bytes:
        return EdsLib.PackedObject(msg_data)
