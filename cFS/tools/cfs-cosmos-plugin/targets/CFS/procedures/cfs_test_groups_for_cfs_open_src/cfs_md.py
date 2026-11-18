from openc3.script.suite import Group

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfs_md(Group):
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

        Group.print(f"Testing MD aliveness on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"MD_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> MD_HK COMMAND_COUNTER")

        # Check accepted NOOP command proving application is up and running
        cmd(f"<%= target_name %> MD_CMD_NOOP")
        wait_check(f"<%= target_name %> MD_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check accepted Reset Counters command
        cmd(f"<%= target_name %> MD_CMD_RESET_COUNTERS")
        wait_check(f"<%= target_name %> MD_HK COMMAND_COUNTER == 0", 100)

    def test_dwell_commands(self):
        """
        FSW Memory Dwell Command test

        Note: When running on a system that is configured with the ram_notimpl PSP module (Currently Linux
              and QNX), the memory reads will fail even though the following commands will work. This will
              be addressed in a future issue.

        This test checks out three related commands: Jam Dwell, Start Dwell, and Stop Dwell.
        The commands are tested in the order that makes sense functionally.
        - Send a jam_dwell command 
            then verify the command was received (by checking the command counter incremented)
            and verify that the command specified dwell table address count incremented (by checking the dwell table address count value)

        - Send a start_dwell command 
            then verify the command was received (by checking the command counter incremented)
            and verify that the command specified dwell table is enabled (by checking the dwell mask value)

        - Send a stop_dwell command 
            then verify the command was received (by checking the command counter incremented)
            and verify that the command specified dwell table is cleared (by checking the dwell mask value)
        """
        dwell_sym = 'MD_AppData'
        dwell_delay = 5

        Group.print(f"Testing MD Dwell Commands on <%= target_name %>")

        #------------- Command 1: Jam Dwell ----------------------------------------
        Group.print(f"Testing MD jam_dwell on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"MD_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> MD_HK COMMAND_COUNTER")

        # Assuming no one else is sending commands, grab the dwell table address count 1
        dwell_mask = tlm(f"<%= target_name %> MD_HK DWELL_TBL_ADDR_COUNT_1")

        # send the jam dwell command - parameters are: table, entry/row, field size, delay, offset, and symbol 
        cmd(f"<%= target_name %> MD_CMD_JAM_DWELL with TABLE_ID 1, ENTRY_ID 1, FIELD_LENGTH 4, DWELL_DELAY {dwell_delay}, OFFSET 0, SYM_NAME {dwell_sym} ")
        wait_check(f"<%= target_name %> MD_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check Dwell Table Address Count 1 
        wait_check(f"<%= target_name %> MD_HK DWELL_TBL_ADDR_COUNT_1 == 1", 100)
        
        # Check Dwell Num Waits per Packet 1 
        wait_check(f"<%= target_name %> MD_HK NUM_WAITS_PER_PKT_1 == {dwell_delay}", 100)

        #------------- Command 2: Start Dwell ---------------------------------------
        Group.print(f"Testing MD start_dwell on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"MD_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> MD_HK COMMAND_COUNTER")

        # Assuming no one else is sending commands, grab the latest dwell mask 
        dwell_mask = tlm(f"<%= target_name %> MD_HK DWELL_ENABLED_MASK")

        # send the start dwell command - parameters are table mask and padding 
        cmd(f"<%= target_name %> MD_CMD_START_DWELL with TABLE_MASK 1, PADDING 0")
        wait_check(f"<%= target_name %> MD_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check Dwell Mask  - Make sure the dwell enabled mask first bit is set 
        wait_check(f"<%= target_name %> MD_HK DWELL_ENABLED_MASK & 1 == 1", 100)

        #------------- Command 3: Stop Dwell ----------------------------------------
        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"MD_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> MD_HK COMMAND_COUNTER")

        # Assuming no one else is sending commands, grab the latest dwell mask 
        dwell_mask = tlm(f"<%= target_name %> MD_HK DWELL_ENABLED_MASK")

        # send the stop dwell command - parameters are table mask and padding 
        cmd(f"<%= target_name %> MD_CMD_STOP_DWELL with TABLE_MASK 1, PADDING 0")
        wait_check(f"<%= target_name %> MD_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check Dwell Mask  - Make sure the dwell enabled mask first bit is clear 
        wait_check(f"<%= target_name %> MD_HK DWELL_ENABLED_MASK & 1 == 0", 100)

        #------------- Command 4: Jam Dwell (restore original table value ) ---------
        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"MD_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> MD_HK COMMAND_COUNTER")

        # send the jam dwell command - jam the original null entry
        cmd(f"<%= target_name %> MD_CMD_JAM_DWELL with TABLE_ID 1, ENTRY_ID 1, FIELD_LENGTH 0, DWELL_DELAY 0, OFFSET 0, SYM_NAME '' ")
        # check command count
        wait_check(f"<%= target_name %> MD_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check Dwell Table Address Count 1
        wait_check(f"<%= target_name %> MD_HK DWELL_TBL_ADDR_COUNT_1 == 0", 100)

        # Check Dwell Num Waits per Packet 1
        wait_check(f"<%= target_name %> MD_HK NUM_WAITS_PER_PKT_1 == 0", 100)

    def test_cmd_set_signature(self):
        """
        FSW Memory Dwell Set Signature command Test
        - Send a set_signature command 
            then verify the command was received (by checking the command counter incremented)
        """

        Group.print(f"Testing MD set_signature on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"MD_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> MD_HK COMMAND_COUNTER")

        # send the set signature command - parameters are table and signature
        cmd(f"<%= target_name %> MD_CMD_SET_SIGNATURE with TABLE_ID 1, PADDING 0, SIGNATURE 'TABLE1SIG'")
        wait_check(f"<%= target_name %> MD_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"MD_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> MD_HK COMMAND_COUNTER")

        # send the set signature command - restore the original value
        cmd(f"<%= target_name %> MD_CMD_SET_SIGNATURE with TABLE_ID 1, PADDING 0, SIGNATURE ''")
        wait_check(f"<%= target_name %> MD_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

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
