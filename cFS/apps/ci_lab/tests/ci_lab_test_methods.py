
from cfs_test_connection import CfsTest_Connection

class ci_lab_test_methods:

    DEFAULT_APP_NAME = "CI_LAB"

    def test_aliveness(self):
        """
        FSW Aliveness Test
        - Send a no-op command
            then verify the command was received (by checking the command counter incremented)
        - Reset the command counter
            then verify the command was received (by checking the command counter was cleared)
        """
        app_name = self.connection.get_app_name()
        instance_name = self.connection.get_instance_name()

        self.connection.print(f"Testing {app_name} aliveness on {instance_name}")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        self.connection.wait_check_packet("HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = self.connection.tlm("HK", "CommandCounter")

        # Check accepted NOOP command proving application is up and running
        self.connection.cmd("Noop")
        self.connection.wait_check("HK", "CommandCounter", cmd_count + 1, 100)

        # Check accepted Reset Counters command
        self.connection.cmd("ResetCounters")
        self.connection.wait_check("HK", "CommandCounter", 0, 100)
        return


    def __init__(self, connection : CfsTest_Connection):
        self.connection = connection
        
