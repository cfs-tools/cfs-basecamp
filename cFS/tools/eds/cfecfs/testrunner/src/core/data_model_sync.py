'''
Copyright (c) 2025 United States Government as represented by
the Administrator of the National Aeronautics and Space Administration.
All Rights Reserved.
'''

from testrunner.core.timestamp import Timestamp

class DataModelSync:

    class HistoryEntry:
        next_seq = 0

        @classmethod 
        def next_global_seq(self):
            self.next_seq = self.next_seq + 1
            return self.next_seq
        
        def get_raw_data(self) -> bytes:
            return self.raw_data

        def get_msg_seq(self) -> int:
            return self.msg_seq

        def get_msg_data(self):
            return self.msg_data

        def __init__(self, remote_host:str, msg_seq:int, raw_data:bytes, msg_data):
            self.remote_host = remote_host
            self.raw_data = raw_data
            self.msg_seq = msg_seq
            self.msg_data = msg_data
            self.when = Timestamp()
            self.global_seq = DataModelSync.HistoryEntry.next_global_seq()

    '''
    The DataModel class contains structures related to the data saved in the cFS-Groundstation
        - Stores dictionaries of Instances, Topics, Subcommands, and Telemetry Types
        - Stores an array of raw messages based on Telmetry Type
    '''
    def __init__(self, mission:str):
        self.mission = mission
        self.history_by_topic = dict()
        self.history_len = 100

    def get_topic(self, topic_name:str) -> list | None:
        topic_list = None
        if topic_name in self.history_by_topic:
            topic_list = self.history_by_topic[topic_name]
        return topic_list
    
    def get_or_add_topic(self, topic_name:str) -> list:
        if topic_name not in self.history_by_topic:
            self.history_by_topic[topic_name] = list()
        return self.get_topic(topic_name)
    
    def append_to_topic(self, topic_id, entry : HistoryEntry):
        topic_list = self.get_or_add_topic(topic_id)
        topic_list.append(entry)

        # Make sure the history does not grow excessively large
        expire_len = len(topic_list) - self.history_len
        if expire_len > 0:
            del topic_list[:expire_len] # "prune" the oldest entries from list

    def get_last_entry(self, topic_name:str) -> HistoryEntry:
        '''
        Returns the most recent history object associated with topic_id.
        '''
        last_entry = None
        topic_list = self.get_topic(topic_name)
        if isinstance(topic_list, list) and len(topic_list) > 0:
            last_entry = topic_list[-1]
        return last_entry

    def get_count(self, topic_name:str) -> int:
        '''
        Returns the most recent history object associated with topic_id.
        '''
        topic_list = self.get_topic(topic_name)
        count = 0
        if isinstance(topic_list, list):
            count = len(topic_list)
        return count
    
    def generate_message(self, instance_id : int|str, topic_id, fcn_code : int = None):
        '''
        Sorts the raw message into the associated data array (or creates one if it doesn't exist).
        Sorts the decoded EDS object from the message into the associated data array (or creates one if it doesn't exist).
        Generates a telemetry display string based on the instance and topic names.

        Inputs:
        host - Information where the telemetry message came from
        datagram - Raw telemetry message as a bytes string
        '''
        topic_obj = None
        eds_id = None
        eds_datatype = None
        msg_data = None

        if topic_id in self.telecommand:
            topic_obj = self.telecommand.Topic(topic_id)
        elif topic_id in self.telemetry: 
            topic_obj = self.telemetry.Topic(topic_id)

        if topic_obj is not None:
            if fcn_code is None:
                eds_id = topic_obj.GetEdsId()
            else:
                eds_id = topic_obj.GetCmdEdsId(fcn_code)
        
        if eds_id is not None:
            eds_datatype = self.eds_db.Entry(eds_id)
            msg_data = eds_datatype()

        return msg_data
