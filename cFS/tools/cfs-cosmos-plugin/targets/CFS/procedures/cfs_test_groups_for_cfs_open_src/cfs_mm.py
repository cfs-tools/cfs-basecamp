from openc3.script.suite import Group

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfs_mm(Group):
    """
    - Methods beginning with script_ or test_ are added to Script dropdown
    """

    def test_0_aliveness(self):
        """
        FSW Aliveness Test
        - Send a no-op command
            then verify the command was received (by checking the command counter incremented)
        - Reset the command counter
            then verify the command was received (by checking the command counter was cleared)
        """

        Group.print(f"Testing MM aliveness on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"MM_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> MM_HK COMMAND_COUNTER")

        # Check accepted NOOP command proving application is up and running
        cmd(f"<%= target_name %> MM_CMD_NOOP")
        wait_check(f"<%= target_name %> MM_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check accepted Reset Counters command
        cmd(f"<%= target_name %> MM_CMD_RESET_COUNTERS")
        wait_check(f"<%= target_name %> MM_HK COMMAND_COUNTER == 0", 100)


    def test_1_LookupSym_test(self):

        Group.print(f"Testing MM MM_CMD_LOOKUP_SYM command on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"MM_HK", 1, 100)

        cmd_count = tlm(f"<%= target_name %> MM_HK COMMAND_COUNTER")

        # Check accepted MM_CMD_LOOKUP_SYM command and LAST_ACTION set correctly
        cmd(f"<%= target_name %> MM_CMD_LOOKUP_SYM with SYMNAME 'SAMPLE_LIB_Buffer'")
        wait_check(f"<%= target_name %> MM_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> MM_HK LAST_ACTION == 'SYM_LOOKUP'", 100)

        # Check accepted Reset Counters command
        cmd(f"<%= target_name %> MM_CMD_RESET_COUNTERS")
        wait_check(f"<%= target_name %> MM_HK COMMAND_COUNTER == 0", 100)


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
