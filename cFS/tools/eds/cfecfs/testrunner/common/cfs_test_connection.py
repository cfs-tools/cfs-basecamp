
from abc import ABC,abstractmethod

class CfsTest_Connection(ABC):

    @abstractmethod
    def wait_check_packet(self, tlm_id, num_pkt, timeout):
        print (f"In wait_check_packet({tlm_id}, {timeout})")
        return

    @abstractmethod
    def tlm(self, tlm_id, param_id):
        print (f"In tlm({tlm_id})")
        return 1

    @abstractmethod
    def cmd(self, cmd_id):
        print (f"In cmd({cmd_id})")
        return 1

    @abstractmethod
    def wait_check(self, tlm_id, param_id, value, timeout):
        print (f"In wait_check({tlm_id}, {param_id}, {timeout})")
        return False

    @abstractmethod
    def spawn_child(self, app_name:str):
        pass

    @abstractmethod
    def wait(timeout):
        pass

    def print(self, msg : str):
        print(msg)
        return

    def get_app_name(self):
        return self.app_name

    def get_instance_name(self):
        return self.instance_name

    def __init__(self, method_type, **kwargs):
        self.method_obj = None
        self.app_name = None
        self.instance_name = None
        self.remote_addr = None

        if method_type is not None:
            self.method_obj = method_type(self)

        if 'app_name' in kwargs:
            self.app_name = kwargs['app_name']
        elif 'DEFAULT_APP_NAME' in method_type.__dict__:
            self.app_name = method_type.__dict__['DEFAULT_APP_NAME']

        if 'instance_name' in kwargs:
            self.instance_name = kwargs['instance_name']

        if 'remote_addr' in kwargs:
            self.remote_addr = kwargs['remote_addr']
        else:
            self.remote_addr = ('127.0.0.1',1234)

        print(f"Instantiated CfsTest_Connection with app_name={self.app_name} instance_name={self.instance_name} remote_addr={self.remote_addr}")

        return
