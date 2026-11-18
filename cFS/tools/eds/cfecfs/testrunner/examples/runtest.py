#!/usr/bin/python3

import importlib
import sys
import argparse

from testrunner.test_group import TestGroup

parser = argparse.ArgumentParser(prog='runtest',
                                 description = "Executes test script")

parser.add_argument("-i", "--instance-name", default='cpu1', help="Instance name")
parser.add_argument("-m", "--mission-name", default='samplemission', help="Mission name")
parser.add_argument("-l", "--local-addr", default='127.0.0.1:2234', help="Local IP address:port to listen on")
parser.add_argument("-r", "--remote-addr", default='127.0.0.1:1234', help="Remote IP address:port to send commands to")
parser.add_argument("app_name", help="The name of the application")
parser.add_argument("tests", help="Test names to execute", nargs='*')

# 3. Parse arguments
args = parser.parse_args()

def load_test_group(app_name : str, instance_name : str, mission_name : str, local_addr : tuple, remote_addr : tuple):
    module_name = f"{app_name}_test_methods"
    tmod = importlib.import_module(f"cfs_app_tests.{module_name}")
    tobj = None

    # Look for a class in the module with a matching name
    tc = tmod.__dict__[module_name]

    print(f"Instantiating: {tc} targeted at {instance_name}")
    return TestGroup(tc, instance_name=instance_name, mission_name = mission_name, local_addr = local_addr, remote_addr = remote_addr)

def addr_as_tuple(addr_str : str, default_ip : str, default_port : int):
    (host,port) = addr_str.split(':', 2)
    if port is None:
        port = default_port
    if host is None:
        host = default_ip
    return (host, int(port))

local_addr = addr_as_tuple(args.local_addr, '127.0.0.1', 2234)
remote_addr = addr_as_tuple(args.remote_addr, '127.0.0.1', 1234)

print (f"Starting runtest script, local={local_addr} remote={remote_addr}")

test_group = load_test_group(app_name = args.app_name, instance_name = args.instance_name,
                             mission_name = args.mission_name, local_addr = local_addr, remote_addr = remote_addr)

test_group.enable_output()
if len(args.tests) == 0:
    print ("No test specified, will execute all")
    test_group.execute_all()
else:
    for test in args.tests:
        test_group.execute(test)
