from openc3.script.suite import Group

# Load the cFE Fleet Class Utility Methods
load_utility('<%= target_name %>/lib/cfs_fleet.py')

# Group class name should indicate what the scripts are testing
class cfs_fleet_test_group_sbn(Group):
    """
    - Methods beginning with script_ or test_ are added to Script dropdown
    """

    def test_00_sbn_cpu1_to_cpu2(self):
        """
        Demonstrate transfer from CPU-1 to CPU-2 over SBN
        """
        # Wait for a new housekeeping packets from each CPU
        # to ensure we're using its latest status info
        wait_check_packet("CFS-1", "SAMPLE_APP_HK", 1, 100)
        wait_check_packet("CFS-2", "SAMPLE_APP_HK", 1, 100)

        cpu1_sample_app_cmd_count = tlm("CFS-1 SAMPLE_APP_HK COMMAND_COUNTER")
        cpu2_sample_app_cmd_count = tlm("CFS-2 SAMPLE_APP_HK COMMAND_COUNTER")

        # Send a SAMPLE_APP No-Op command to the interface of CPU-1
        # Override the Command MID to use the MID from SAMPLE_APP of CPU-2
        cmd(f"CFS-1 SAMPLE_APP_CMD_NOOP with CCSDS_STREAMID <%= get_cfs_pkt_msg_id('SAMPLE_APP_CMD_NOOP', 2) %>")

        # Verify cpu2 received the command
        wait_check(f"CFS-1 SAMPLE_APP_HK COMMAND_COUNTER == {cpu1_sample_app_cmd_count}", 100)
        wait_check(f"CFS-2 SAMPLE_APP_HK COMMAND_COUNTER == {cpu2_sample_app_cmd_count + 1}", 100)


    def disable_test_01_sbn_cpu2_to_cpu1(self):
        """
        Demonstrate transfer from CPU-2 to CPU-1 over SBN
        """
        # Wait for a new housekeeping packets from each CPU
        # to ensure we're using its latest status info
        wait_check_packet("CFS-1", "SAMPLE_APP_HK", 1, 100)
        wait_check_packet("CFS-2", "SAMPLE_APP_HK", 1, 100)

        cpu1_sample_app_cmd_count = tlm("CFS-1 SAMPLE_APP_HK COMMAND_COUNTER")
        cpu2_sample_app_cmd_count = tlm("CFS-2 SAMPLE_APP_HK COMMAND_COUNTER")

        # Send a SAMPLE_APP No-Op command to the interface of CPU-2
        # Override the Command MID to use the MID from SAMPLE_APP of CPU-1
        cmd(f"CFS-2 SAMPLE_APP_CMD_NOOP with CCSDS_STREAMID <%= get_cfs_pkt_msg_id('SAMPLE_APP_CMD_NOOP', 1) %>")

        # Verify cpu2 received the command
        wait_check(f"CFS-1 SAMPLE_APP_HK COMMAND_COUNTER == {cpu1_sample_app_cmd_count + 1}", 100)
        wait_check(f"CFS-2 SAMPLE_APP_HK COMMAND_COUNTER == {cpu2_sample_app_cmd_count}", 100)

    def setup(self):
        """
        Test Group Setup
        - Runs when Group Setup button is pressed
        - Runs before all scripts when Group Start is pressed
        """
        pass

    def teardown(self):
        """
        Test Group Setup
        - Runs when Group Teardown button is pressed
        - Runs after all scripts when Group Start is pressed
        """
        pass
