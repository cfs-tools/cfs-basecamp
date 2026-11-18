from openc3.script.suite import Group

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfs_sample_app(Group):
    """
    - Methods beginning with script_ or test_ are added to Script dropdown
    """

    def test_aliveness(self):
        """
        FSW Aliveness Test
        - Send a no-op command
            then verify the command was received (by checking the command counter incremented)
        - Reset the command counter
            then verify the command was received (by checking the command counter was cleared)
        """

        Group.print(f"Testing SAMPLE_APP aliveness on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"SAMPLE_APP_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> SAMPLE_APP_HK COMMAND_COUNTER")

        # Check accepted NOOP command proving application is up and running
        cmd(f"<%= target_name %> SAMPLE_APP_CMD_NOOP")
        wait_check(f"<%= target_name %> SAMPLE_APP_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check accepted Reset Counters command
        cmd(f"<%= target_name %> SAMPLE_APP_CMD_RESET_COUNTERS")
        wait_check(f"<%= target_name %> SAMPLE_APP_HK COMMAND_COUNTER == 0", 100)

    def test_cmd_process(self):
        """
        FSW Command Process Test
        - Send command process
            then verify the command was received (by checking the command counter incremented)
        """
        
        Group.print(f"Testing SAMPLE_APP command process on <%= target_name %>")
        
        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"SAMPLE_APP_HK", 1, 100)
        
        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> SAMPLE_APP_HK COMMAND_COUNTER")
        
        # Check accepted Command Process
        cmd(f"<%= target_name %> SAMPLE_APP_CMD_PROCESS")
        wait_check(f"<%= target_name %> SAMPLE_APP_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

    def test_display_param(self):
        """
        FSW Display Param Test
        - Send display param command
            then verify the command was received (by checking the command counter incremented)
        """
        
        Group.print(f"Testing SAMPLE_APP display param command on <%= target_name %>")
        
        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"SAMPLE_APP_HK", 1, 100)
        
        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> SAMPLE_APP_HK COMMAND_COUNTER")
        
        # Check accepted Display Param command
        cmd(f"<%= target_name %> SAMPLE_APP_CMD_DISPLAY_PARAM with VAL_U32 10, VAL_I16 -4, VAL_STR 'Hello'")    
        wait_check(f"<%= target_name %> SAMPLE_APP_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

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
