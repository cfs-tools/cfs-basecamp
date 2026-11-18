"""
TESTRUNNER: Automated System Testing for Robust Operations

A functional test framework for verifying cFS-based flight systems using
Electronic Data Sheets (EDS) for command and telemetry definitions.
"""

from testrunner.testrunner_sync import TestrunnerSync
from testrunner.testrunner_adapter import TestrunnerAdapter
from cfs_test_connection import CfsTest_Connection

__all__ = ['TestrunnerSync', 'CfsTest_Connection', "TestrunnerAdapter"]
