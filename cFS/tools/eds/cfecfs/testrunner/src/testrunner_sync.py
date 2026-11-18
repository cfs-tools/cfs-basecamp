'''
Copyright (c) 2025 United States Government as represented by
the Administrator of the National Aeronautics and Space Administration.
All Rights Reserved.
'''

import operator
from testrunner.core.transport_base import TransportBase
from testrunner.core.controller_sync import ControllerSync
from testrunner.core.data_model_sync import DataModelSync
from testrunner.core.timestamp import Timestamp

class TestrunnerSync:
    """
    High-level API for test scripts to interact with TESTRUNNER.
    """

    # The data model handles encoding and decoding of commands and telemetry,
    # and also stores all received messages into a dictionary for future checks
    DataModel = None

    # The controller manages the connection(s) to the target, sending raw commands
    # and receiving raw telemetry, and also starting and stopping cfs if relevant
    # (such as powering on or off or invoking a restart)
    Controller = None

    OPS = {
        "==": operator.eq,
        "!=": operator.ne,
        "<":  operator.lt,
        "<=": operator.le,
        ">":  operator.gt,
        ">=": operator.ge,
    }

    def __init__(
        self,
        transport: TransportBase,
        **kwargs
    ):
        if 'mission_name' in kwargs:
            self.mission_name = kwargs['mission_name']
        else:
            self.mission_name = 'samplemission'

        if 'instance_name' in kwargs:
            self.instance_name = kwargs['instance_name']
        else:
            self.instance_name = 'cpu1'

        self.transport = transport

        # jphfix TBD - multiple instances of this share the same controller and data model (maybe)
        self.Controller = ControllerSync(self.mission_name)
        self.DataModel = DataModelSync(self.mission_name)

    def get_last_msg(self, topic_name:str):
        last_msg = self.DataModel.get_last_entry(topic_name)
        if last_msg:
            last_msg = last_msg.get_msg_data()
        return last_msg

    def trigger_hk(self, app_name:str):
        sendhk_name = f"{app_name}/Application/SEND_HK"
        self.send_cmd(sendhk_name)

    def get_last_seq(self, topic_name:str):
        last_seq = None
        last_entry = self.DataModel.get_last_entry(topic_name)
        if last_entry:
            last_seq = last_entry.get_msg_seq()
        return last_seq

    def process_traffic(self, expire:Timestamp=None, pkt_limit:int=None):

        got_pkts = 0
        while True:
            if pkt_limit is not None and got_pkts >= pkt_limit:
                break

            raw_data, host = self.transport.receive(expire)
            if raw_data is None:
                break

            try:
                decoded_obj, topic_name = self.Controller.identify_datagram(raw_data)

                msg_seq = self.Controller.get_sequence(topic_name, decoded_obj)
                history_entry = DataModelSync.HistoryEntry(host,msg_seq,raw_data,decoded_obj)
                self.DataModel.append_to_topic(topic_name, history_entry)
                got_pkts += 1
            except Exception as e:
                print (f"Failed to identify and decode: {e}")

        return got_pkts

    def send_cmd(self, topic_name, cmd_name=None, **kwargs):
        print (f"In TestrunnerSync.sendcmd({topic_name})")
        msg_data = self.Controller.generate_message(topic_name, cmd_name)
        if hasattr(msg_data, 'Payload'):
            msg_data.Payload = kwargs
        self.Controller.setup_header(msg_data, self.instance_name, topic_name)
        encoded_obj = self.Controller.encode_datagram(msg_data)
        msg_seq = self.Controller.get_sequence(topic_name, msg_data)
        history_entry = DataModelSync.HistoryEntry(None,msg_seq,encoded_obj,msg_data)
        self.DataModel.append_to_topic(topic_name, history_entry)
        return self.transport.transmit(encoded_obj)

    def get_msg_id_from_topic(self, topic_name:str):
        return self.Controller.get_msg_id_from_topic(topic_name, self.instance_name)
