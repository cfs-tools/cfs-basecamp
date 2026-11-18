from openc3.script.suite import Group

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfe_time_checkout(Group):
    """
    - Methods beginning with script_ or test_ are added to Script dropdown
    """

    def test_cfe_time_checkout(self):
        """
        CFE TIME Checkout Test
        -Send all TIME commands as quickly as possible, then verify all were accepted
        """

        set_line_delay(0.0)

        Group.print(f"CFE_TIME checkout test on <%= target_name %>")

        # Save initial command counts
        cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")

        set_line_delay(0.15)  # 0.15 is about as fast as it can go.  0.1 causes a missed cmd.

        cmd(f"<%= target_name %> CFE_TIME_CMD_NOOP")
        cmd(f"<%= target_name %> CFE_TIME_CMD_SEND_DIAGNOSTIC")
        cmd(f"<%= target_name %> CFE_TIME_CMD_SET_STATE with CLOCK_STATE 'VALID'")

        # WARNING: Disabled for now, because our reference cFS bundle has
        #          CFE_PLATFORM_TIME_CFG_CLIENT set FALSE
        #cmd(f"<%= target_name %> CFE_TIME_CMD_SET_SOURCE with TIME_SOURCE 'INTERNAL'")
        #cmd(f"<%= target_name %> CFE_TIME_CMD_SET_SIGNAL with TONE_SOURCE 'TONE_PRI'")
        #cmd(f"<%= target_name %> CFE_TIME_CMD_ADD_DELAY with SECONDS 1, MICROSECONDS 2")
        #cmd(f"<%= target_name %> CFE_TIME_CMD_SUB_DELAY with SECONDS 1, MICROSECONDS 2")
        
        cmd(f"<%= target_name %> CFE_TIME_CMD_SET_TIME with SECONDS 3, MICROSECONDS 4")
        cmd(f"<%= target_name %> CFE_TIME_CMD_SET_MET with SECONDS 4, MICROSECONDS 5")
        cmd(f"<%= target_name %> CFE_TIME_CMD_SET_STCF with SECONDS 5, MICROSECONDS 6")
        cmd(f"<%= target_name %> CFE_TIME_CMD_SET_LEAP_SECONDS with LEAP_SECONDS 99")
        cmd(f"<%= target_name %> CFE_TIME_CMD_ADD_ADJUST with SECONDS 1, MICROSECONDS 2")
        cmd(f"<%= target_name %> CFE_TIME_CMD_SUB_ADJUST with SECONDS 1, MICROSECONDS 2")
        cmd(f"<%= target_name %> CFE_TIME_CMD_ADD_ONE_HZ_ADJUSTMENT with SECONDS 1, SUBSECONDS 2")
        cmd(f"<%= target_name %> CFE_TIME_CMD_SUB_ONE_HZ_ADJUSTMENT with SECONDS 3, SUBSECONDS 4")
        
        set_line_delay(0.0)
        
        # Check final command count has incremented by the number of commands sent
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count} + 11", 12)

        cmd(f"<%= target_name %> CFE_TIME_CMD_RESET_COUNTERS")
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == 0", 12)


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