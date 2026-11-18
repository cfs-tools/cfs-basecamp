'''
Copyright (c) 2025 United States Government as represented by
the Administrator of the National Aeronautics and Space Administration.
All Rights Reserved.

cFS TESTRUNNER Data Interface:
Generic top-level class for data interfaces, such as UDP.
'''


from abc import ABC,abstractmethod
from testrunner.core.timestamp import Timestamp

class TransportBase(ABC):

    @abstractmethod
    def wait_readable(self, timeout:Timestamp):
        # Abstract class method
        raise NotImplementedError
    
    @abstractmethod
    def transmit(self, data:bytes):
        # Abstract class method
        raise NotImplementedError
    
    @abstractmethod
    def receive(self, timeout:Timestamp):
        # Abstract class method
        raise NotImplementedError
