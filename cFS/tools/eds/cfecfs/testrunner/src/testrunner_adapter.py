from testrunner import TestrunnerSync
from cfs_test_connection import CfsTest_Connection
from testrunner.core.timestamp import Timestamp
from testrunner.transports.transport_udp import Transport_UDP
from types import MethodType

import re

class TestrunnerAdapter(CfsTest_Connection):

    def print(self, msg : str):
        print (msg)
        return

    def wait_next_packet(self, expire:Timestamp):
        while True:
            remain = expire.remaining_seconds()
            if remain > 1:
                cycle_expire = Timestamp.from_relative_seconds(1)
            else:
                cycle_expire = expire
            got_packets = self.testrunner.process_traffic(cycle_expire,1)
            if got_packets != 0 or remain <= 0:
                break

            # Prod the app to send its HK_TLM
            self.testrunner.trigger_hk(self.app_name)

        return (got_packets != 0)

    def wait_check_packet(self, tlm_id, num_pkt:int=1, timeout:int=None):
        topic_name = f"{self.app_name}/Application/{tlm_id}_TLM"
        print (f"In testrunner wait_check_packet({topic_name}, {num_pkt}, {timeout})")

        start_seq = self.testrunner.get_last_seq(topic_name)
        curr_seq = start_seq
        got_count = 0
        expire = Timestamp.from_relative_seconds(timeout)
        while self.wait_next_packet(expire):
            curr_seq = self.testrunner.get_last_seq(topic_name)
            if curr_seq is not None:
                if start_seq is None:
                    start_seq = curr_seq - 1 # fake it so diff will be 1
                got_count = curr_seq - start_seq
                if got_count >= num_pkt:
                    break # got the amount desired

        return got_count

    def tlm(self, tlm_id, param_id):
        topic_name = f"{self.app_name}/Application/{tlm_id}_TLM"
        print (f"In testrunner tlm({topic_name}): {param_id}")

        result = self.testrunner.get_last_msg(topic_name)
        #Returns EdsLib.DatabaseEntry: A parsed EDS object representing the telemetry packet.
        if result != None:
            result = result.Payload[param_id]()

        assert result is not None

        return result

    def debug_print_tlm(self, tlm_id):
        topic_name = f"{self.app_name}/Application/{tlm_id}_TLM"
        print (f"In testrunner debug_print_tlm({topic_name})")

        result = self.testrunner.get_last_msg(topic_name)
        #Returns EdsLib.DatabaseEntry: A parsed EDS object representing the telemetry packet.
        if result != None:
            for member in result.Payload:
                print(f'{member[0]} => {member[1]()}')


    def cmd(self, cmd_id, **kwargs):
        topic_name = f"{self.app_name}/Application/CMD"
        print (f"In testrunner cmd({topic_name}.{cmd_id}): {kwargs}")

        # Note that the command codes in EDS have a "Cmd" suffix on them
        result = self.testrunner.send_cmd(topic_name, cmd_id + 'Cmd', **kwargs)
        print (f"In testrunner cmd result: {result}")
        return result

    def wait_check(self, tlm_id, param_id, value, timeout):
        topic_name = f"{self.app_name}/Application/{tlm_id}_TLM"
        result = False
        print (f"In testrunner wait_check({topic_name}): {param_id}, {value} {timeout}")

        expire = Timestamp.from_relative_seconds(timeout)
        while True:
            last_msg = self.testrunner.get_last_msg(topic_name)
            if last_msg is not None:
                last_value = last_msg.Payload[param_id]()
                result = (last_value == value)

            if result or not self.wait_next_packet(expire):
                break

        assert result
        return result

    def spawn_child(self, app_name:str):
        return TestrunnerSecondary(self, app_name)

    def get_tlm_msg_id(self, tlm_id:str):
        topic_name = f"{self.app_name}/Application/{tlm_id}_TLM"
        return self.testrunner.get_msg_id_from_topic(topic_name)

    def get_cmd_msg_id(self):
        topic_name = f"{self.app_name}/Application/CMD"
        return self.testrunner.get_msg_id_from_topic(topic_name)

    def get_tests(self):
        return self.test_dict.keys()

    def register_test_method(self,test_obj,test_method):
        func = lambda s: test_method(self.method_obj)
        return MethodType(func, test_obj)

    def register_tests(self,test_obj):
        test_dict = dict()
        for prop,val in type(self.method_obj).__dict__.items():
            if prop.startswith("test_") and callable(val):
                print(f"Found test method: {prop}")
                test_dict[prop] = self.register_test_method(test_obj,val)
        test_obj.test_dict = test_dict

    def wait(self, timeout : Timestamp):
        expire = Timestamp.from_relative_seconds(timeout)
        self.testrunner.process_traffic(expire)

    def enable_output(self):
        to_app = self.spawn_child("TO_LAB")
        to_app.cmd("EnableOutput", dest_IP=self.local_addr[0])
        to_app.cmd("AddPacket", Stream={'Value':self.get_tlm_msg_id("HK")}, BufLimit=5)

    def __init__(self, method_type : type, **kwargs):
        CfsTest_Connection.__init__(self, method_type, **kwargs)
        # For the time being, this assumes it is always using the UDP transport provided by CI/TO_LAB
        if 'remote_addr' in kwargs:
            remote_addr = kwargs['remote_addr']
        else:
            remote_addr = ('127.0.0.1',1234)
        if 'local_addr' in kwargs:
            local_addr = kwargs['local_addr']
        else:
            local_addr = ('127.0.0.1',2234)
        self.remote_addr = remote_addr
        self.local_addr = local_addr
        default_transport = Transport_UDP(remote_addr,local_addr)
        self.testrunner = TestrunnerSync(transport=default_transport, **kwargs)

# A secondary shares the same testrunner instance and just talks to a different app
class TestrunnerSecondary(TestrunnerAdapter):
    def __init__(self, parent_obj : TestrunnerAdapter, app_name:str):
        CfsTest_Connection.__init__(self, None, app_name = app_name, instance_name = parent_obj.get_instance_name(), remote_addr = parent_obj.remote_addr)
        self.testrunner = parent_obj.testrunner
