from openc3.script.suite import Group

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfs_sc(Group):
    """
    - Methods beginning with script_ or test_ are added to Script dropdown
    """

    def test_00_aliveness(self):
        """
        FSW Aliveness Test
        - Send a no-op command
            then verify the command was received (by checking the command counter incremented)
        - Reset the command counter
            then verify the command was received (by checking the command counter was cleared)
        """

        Group.print(f"Testing SC aliveness on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"SC_HK", 1, 20)

        # Assuming no one else is sending commands, grab the latest command count
        exp_cmd_count = tlm(f"<%= target_name %> SC_HK COMMAND_COUNTER")

        # Check accepted NOOP command proving application is up and running
        cmd(f"<%= target_name %> SC_CMD_NOOP")
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER == {exp_cmd_count + 1}", 20)

        # Check accepted Reset Counters command
        cmd(f"<%= target_name %> SC_CMD_RESET_COUNTERS")
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER == 0", 20)
    

    def test_01_NoOp(self):
        """
        Test the no-op command.
        """
        
        exp_cmd_count = tlm(f"<%= target_name %> SC_HK COMMAND_COUNTER")
        
        cmd(f"<%= target_name %> SC_CMD_NOOP")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER == {exp_cmd_count + 1}", 20)


    def test_02_continue_ats_on_failure(self):
        """
        Continue ATS on Failure Test
        - Send a continue ats on failure command
            then verify the command was received (by checking the command counter incremented)
            and set back to default value (True)
        """
        
        Group.print(f"Testing SC Continue ATS on Failure on <%= target_name %>")

        # Assuming no one else is sending commands, grab the latest command count
        exp_cmd_count = tlm(f"<%= target_name %> SC_HK COMMAND_COUNTER")

        # Check accepted Continue ATS on Failure command
        cmd(f"<%= target_name %> SC_CMD_CONTINUE_ATS_ON_FAILURE with CONTINUE_STATE FALSE")
        exp_cmd_count = exp_cmd_count + 1
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER == {exp_cmd_count}", 20)
        wait_check(f"<%= target_name %> SC_HK CONT_ATS_ON_FAIL == 'FALSE'", 20)

        # Set Continue ATS on Failure to Default Value (True)
        cmd(f"<%= target_name %> SC_CMD_CONTINUE_ATS_ON_FAILURE with CONTINUE_STATE TRUE")
        exp_cmd_count = exp_cmd_count + 1
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER == {exp_cmd_count}", 20)
        wait_check(f"<%= target_name %> SC_HK CONT_ATS_ON_FAIL == 'TRUE'", 20)
    

    def test_03_ats_operations(self):
        """
        ATS Operations Test
        - Test start ATS command
        - Test jump ATS command
        - Test switch ATS command
        - Test append ATS command
        - Test stop ATS 
        """
        
        Group.print(f"Testing SC ATS operations on <%= target_name %>")

        tbl_exp_cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")

        # Load ATS A table
        cmd(f"<%= target_name %> CFE_TBL_CMD_LOAD with LOAD_FILENAME '/cf/sc_ats1.tbl'")
        tbl_exp_cmd_count = tbl_exp_cmd_count + 1
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {tbl_exp_cmd_count}", 20)

        # Validate ATS A table
        cmd(f"<%= target_name %> CFE_TBL_CMD_VALIDATE with ACTIVE_TABLE_FLAG INACTIVE, TABLE_NAME 'SC.ATS_TBL1'")
        tbl_exp_cmd_count = tbl_exp_cmd_count + 1
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {tbl_exp_cmd_count}", 20)
        
        # Activate ATS A table
        cmd(f"<%= target_name %> CFE_TBL_CMD_ACTIVATE with TABLE_NAME 'SC.ATS_TBL1'")
        tbl_exp_cmd_count = tbl_exp_cmd_count + 1
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {tbl_exp_cmd_count}", 20)

        # Load ATS B table
        cmd(f"<%= target_name %> CFE_TBL_CMD_LOAD with LOAD_FILENAME '/cf/sc_ats2_test.tbl'")
        tbl_exp_cmd_count = tbl_exp_cmd_count + 1
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {tbl_exp_cmd_count}", 20)

        # Validate ATS B table
        cmd(f"<%= target_name %> CFE_TBL_CMD_VALIDATE with ACTIVE_TABLE_FLAG INACTIVE, TABLE_NAME 'SC.ATS_TBL2'")
        tbl_exp_cmd_count = tbl_exp_cmd_count + 1
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {tbl_exp_cmd_count}", 20)
        
        # Activate ATS B table
        cmd(f"<%= target_name %> CFE_TBL_CMD_ACTIVATE with TABLE_NAME 'SC.ATS_TBL2'")
        tbl_exp_cmd_count = tbl_exp_cmd_count + 1
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {tbl_exp_cmd_count}", 20)

        prev_append_ld_count = tlm(f"<%= target_name %> SC_HK APPEND_LOAD_COUNT")

        # Load ATS Append table
        cmd(f"<%= target_name %> CFE_TBL_CMD_LOAD with LOAD_FILENAME '/cf/sc_append_test.tbl'")
        tbl_exp_cmd_count = tbl_exp_cmd_count + 1
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {tbl_exp_cmd_count}", 20)

        # Validate ATS Append table
        cmd(f"<%= target_name %> CFE_TBL_CMD_VALIDATE with ACTIVE_TABLE_FLAG INACTIVE, TABLE_NAME 'SC.APPEND_TBL'")
        tbl_exp_cmd_count = tbl_exp_cmd_count + 1
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {tbl_exp_cmd_count}", 20)
        
        # Activate ATS Append table
        cmd(f"<%= target_name %> CFE_TBL_CMD_ACTIVATE with TABLE_NAME 'SC.APPEND_TBL'")
        tbl_exp_cmd_count = tbl_exp_cmd_count + 1
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {tbl_exp_cmd_count}", 20)

        # Append HK values should auto-update when table activated
        wait_check(f"<%= target_name %> SC_HK APPEND_ENTRY_COUNT == 4", 20)
        wait_check(f"<%= target_name %> SC_HK APPEND_BYTE_COUNT == 36", 20)
        wait_check(f"<%= target_name %> SC_HK APPEND_LOAD_COUNT == {prev_append_ld_count + 1}", 20)

        # Set SC time to 1000000 (because RTS1 starts shortly after that time).
        hk_cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")
        cmd(f"<%= target_name %> CFE_TIME_CMD_SET_TIME with SECONDS 1000000, MICROSECONDS 0")
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {hk_cmd_count + 1}", 20)

        ########################
        # Test Start ATS command
        ########################

        check(f"<%= target_name %> SC_HK ATP_STATE == 'IDLE'")

        exp_cmd_count = tlm(f"<%= target_name %> SC_HK COMMAND_COUNTER")

        cmd(f"<%= target_name %> SC_CMD_START_ATS with ATS_NUM 1")

        exp_cmd_count = exp_cmd_count + 1
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER == {exp_cmd_count}", 20)
        wait_check(f"<%= target_name %> SC_HK ATP_STATE == 'EXECUTING'", 20)

        # NOTE: expected event message text: "ATS A Execution Started"

        wait(3)

        ########################
        # Test Switch ATS command
        ########################

        check(f"<%= target_name %> SC_HK SWITCH_PEND_FLAG == 'NO'")

        cmd(f"<%= target_name %> SC_CMD_SWITCH_ATS")
        
        exp_cmd_count = exp_cmd_count + 1
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER == {exp_cmd_count}", 20)

        # Note: SWITCH_PEND_FLAG should go to YES, and then back to NO when switch completes.
        # #     This can't be checked, because the switch completes before the next HK packet.

        # NOTE: expected event messages text:
        #       "Switch ATS is Pending"
        #       "ATS Switched from A to B"

        ########################
        # Test Append ATS command
        ########################

        # Send this command twice: first to have a known initial value of APPEND_CMD_ARG

        cmd(f"<%= target_name %> SC_CMD_APPEND_ATS with ATS_NUM 1")
        
        exp_cmd_count = exp_cmd_count + 1
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER == {exp_cmd_count}", 20)
        wait_check(f"<%= target_name %> SC_HK APPEND_CMD_ARG == 1", 20)

        cmd(f"<%= target_name %> SC_CMD_APPEND_ATS with ATS_NUM 2")
        
        exp_cmd_count = exp_cmd_count + 1
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER == {exp_cmd_count}", 20)
        wait_check(f"<%= target_name %> SC_HK APPEND_CMD_ARG == 2", 20)

        # NOTE: expected event message text: "Append ATS B command: %d ATS entries appended"

        ########################
        # Test Jump ATS command
        ########################

        new_time = 1000101

        check(f"<%= target_name %> SC_HK NEXT_ATS_TIME != {new_time}")

        cmd(f"<%= target_name %> SC_CMD_JUMP_ATS with NEW_TIME {new_time}")
        
        exp_cmd_count = exp_cmd_count + 1
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER == {exp_cmd_count}", 20)
        wait_check(f"<%= target_name %> SC_HK NEXT_ATS_TIME == 1000130", 20)

        # NOTE: expected event messages text:
        #       "Jump Cmd: Jump time less than or equal to list entry %u"
        #       "Next ATS command time in the ATP was set to %s"
        #       "Jump Cmd: Skipped %d ATS commands"

        ########################
        # Test Stop ATS command
        ########################

        check(f"<%= target_name %> SC_HK ATP_STATE == 'EXECUTING'")
        
        cmd(f"<%= target_name %> SC_CMD_STOP_ATS")

        exp_cmd_count = exp_cmd_count + 1
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER == {exp_cmd_count}", 20)
        wait_check(f"<%= target_name %> SC_HK ATP_STATE == 'IDLE'", 20)

        # NOTE: expected event message text: "ATS B stopped"

        wait(3)


    def test_04_rts_operations(self):
        """
        RTS Operations Test
        - Test disable RTS command
        - Test enable RTS command
        - Test start RTS command
        - Test stop RTS command
        """

        Group.print(f"Testing SC RTS operations on <%= target_name %>")

        exp_cmd_count = tlm(f"<%= target_name %> SC_HK COMMAND_COUNTER")

        # First, ensure all W0 RTSs are enabled (0)
        cmd(f"<%= target_name %> SC_CMD_ENABLE_RTS_GRP with FIRST_RTS_NUM 1, LAST_RTS_NUM 4")
        exp_cmd_count = exp_cmd_count + 1
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER == {exp_cmd_count}", 20)
        wait_check(f"<%= target_name %> SC_HK RTS_W0_DIS_STATUS == 0x0000", 20)

        ##############################
        # Test Disable RTS (1) command
        ##############################

        # Check RTS_W0_DIS_STATUS, bit 0, equals 0 (enabled)
        check(f"<%= target_name %> SC_HK RTS_W0_DIS_STATUS & 0x0001 == 0x0000")

        cmd(f"<%= target_name %> SC_CMD_DISABLE_RTS with RTS_NUM 1")
        exp_cmd_count = exp_cmd_count + 1
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER == {exp_cmd_count}", 20)

        # Check RTS_W0_DIS_STATUS, bit 0, equals 1 (disabled)
        wait_check(f"<%= target_name %> SC_HK RTS_W0_DIS_STATUS & 0x0001 == 0x0001", 20)

        # NOTE: expected event message text: "Disabled RTS 001"

        #############################
        # Test Enable RTS (1) command
        #############################

        cmd(f"<%= target_name %> SC_CMD_ENABLE_RTS with RTS_NUM 1")
        exp_cmd_count = exp_cmd_count + 1
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER == {exp_cmd_count}", 20)
        
        # Check RTS_W0_DIS_STATUS, bit 0, equals 0 (enabled)
        wait_check(f"<%= target_name %> SC_HK RTS_W0_DIS_STATUS & 0x0001 == 0x0000", 20)
        
        # NOTE: expected event message text: "Enabled RTS 001"

        ########################
        # Test Start RTS command
        ########################

        # Check RTS_W0_EXE_STATUS, bit 0, equals 0 (idle)
        check(f"<%= target_name %> SC_HK RTS_W0_EXE_STATUS & 0x0001 == 0x0000")

        check(f"<%= target_name %> SC_HK NUM_RTS_ACTIVE == 0")

        cmd(f"<%= target_name %> SC_CMD_START_RTS with RTS_NUM 1")
        exp_cmd_count = exp_cmd_count + 1
        # Verify command accepted.  Will increment more than 1 before packet, due to RTS cmds.
        # The RTS will send 6 commands, but we can't check that here, because we need to
        # call Stop RTS before all commands have been sent, to test Stop RTS.
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER >= {exp_cmd_count}", 20)

        # Check RTS_W0_EXE_STATUS, bit 0, equals 1 (executing)
        wait_check(f"<%= target_name %> SC_HK RTS_W0_EXE_STATUS & 0x0001 == 0x0001", 20)
        
        wait_check(f"<%= target_name %> SC_HK NUM_RTS_ACTIVE == 1", 20)

        # NOTE: expected event message text: "RTS Number 001 Started"

        wait(3)

        #######################
        # Test Stop RTS command
        #######################
        
        check(f"<%= target_name %> SC_HK NUM_RTS_ACTIVE == 1")

        # Note: Can't reliably check for an exact value for command count here, because the
        # command must be sent while the RTS is still running, and it's hard to predict
        # how many RTS commands will be called before the tlm packet arrives.
        cmd(f"<%= target_name %> SC_CMD_STOP_RTS with RTS_NUM 1")
        exp_cmd_count = exp_cmd_count + 1
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER >= {exp_cmd_count}", 20)
        
        # Check RTS_W0_EXE_STATUS, bit 0, equals 0 (idle)
        wait_check(f"<%= target_name %> SC_HK RTS_W0_EXE_STATUS & 0x0001 == 0x0000", 20)

        wait_check(f"<%= target_name %> SC_HK NUM_RTS_ACTIVE == 0", 20)

        # NOTE: expected event message text: "RTS Number 001 Aborted"


    def test_05_rts_group_operations(self):
        """
        RTS Group Operations Test
        - Test disable rts group command
        - Test enable rts group command
        - Test start rts group command
        - Test stop rts group command
        """

        Group.print(f"Testing SC RTS group operations on <%= target_name %>")

        # First, ensure all W0 RTSs are enabled (0)
        exp_cmd_count = tlm(f"<%= target_name %> SC_HK COMMAND_COUNTER")
        cmd(f"<%= target_name %> SC_CMD_ENABLE_RTS_GRP with FIRST_RTS_NUM 1, LAST_RTS_NUM 4")
        exp_cmd_count = exp_cmd_count + 1
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER == {exp_cmd_count}", 20)
        wait_check(f"<%= target_name %> SC_HK RTS_W0_DIS_STATUS == 0x0000", 20)

        ##############################################
        # Test Disable RTS group (2 through 4) command
        ##############################################

        # Check RTS_W0_DIS_STATUS, bits 2-4, equal 0 (enabled)
        check(f"<%= target_name %> SC_HK RTS_W0_DIS_STATUS & 0x000E == 0x0000")
        
        cmd(f"<%= target_name %> SC_CMD_DISABLE_RTS_GRP with FIRST_RTS_NUM 2, LAST_RTS_NUM 4")
        exp_cmd_count = exp_cmd_count + 1
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER == {exp_cmd_count}", 20)
        
        # Check RTS_W0_DIS_STATUS, bits 2-4, equal 1 (disabled)
        wait_check(f"<%= target_name %> SC_HK RTS_W0_DIS_STATUS & 0x000E == 0x000E", 20)

        # NOTE: expected event message text: "Disable RTS group: FirstID=2, LastID=4, Modified=3"

        #############################################
        # Test Enable RTS group (2 through 4) command
        #############################################

        cmd(f"<%= target_name %> SC_CMD_ENABLE_RTS_GRP with FIRST_RTS_NUM 2, LAST_RTS_NUM 4")
        exp_cmd_count = exp_cmd_count + 1
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER == {exp_cmd_count}", 20)
        
        # Check RTS_W0_DIS_STATUS, bits 2-4, equal 0 (enabled)
        wait_check(f"<%= target_name %> SC_HK RTS_W0_DIS_STATUS & 0x000E == 0x0000", 20)

        # NOTE: expected event message text: "Enable RTS group: FirstID=2, LastID=4, Modified=3"

        ##############################################
        # Test Start RTS group (2 through 4) command
        ##############################################

        # Check RTS_W0_EXE_STATUS, bit 2-4, equal 0 (idle)
        check(f"<%= target_name %> SC_HK RTS_W0_EXE_STATUS & 0x000E == 0x0000")

        cmd(f"<%= target_name %> SC_CMD_START_RTS_GRP with FIRST_RTS_NUM 2, LAST_RTS_NUM 4")
        exp_cmd_count = exp_cmd_count + 1
        # Verify command accepted.  Will increment more than 1 before packet, due to RTS cmds.
        # The RTS will send 9 commands, but we can't check that here, because we need to
        # call Stop RTS Group before all commands have been sent, to test Stop RTS Group.
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER >= {exp_cmd_count}", 20)
        
        # Check RTS_W0_EXE_STATUS, bit 2-4, equal 1 (executing)
        wait_check(f"<%= target_name %> SC_HK RTS_W0_EXE_STATUS == 0x000E", 20)

        wait_check(f"<%= target_name %> SC_HK NUM_RTS_ACTIVE == 3", 20)

        # NOTE: expected event message text: "Start RTS group: FirstID=2, LastID=4, Modified=3"

        wait(3)

        ##############################################
        # Test Stop RTS group (2 through 4) command
        ##############################################

        # Note: Can't reliably check for an exact value for command count here, because the
        # command must be sent while the RTSs are still running, and it's hard to predict
        # how many RTS commands will be called before the tlm packet arrives.
        cmd(f"<%= target_name %> SC_CMD_STOP_RTS_GRP with FIRST_RTS_NUM 2, LAST_RTS_NUM 4")
        exp_cmd_count = exp_cmd_count + 1
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER >= {exp_cmd_count}", 20)

        # Check RTS_W0_EXE_STATUS, bit 2-4, equal 0 (idle)
        wait_check(f"<%= target_name %> SC_HK RTS_W0_EXE_STATUS & 0x000E == 0x0000", 20)

        wait_check(f"<%= target_name %> SC_HK NUM_RTS_ACTIVE == 0", 20)

        # NOTE: expected event message text: "Stop RTS group: FirstID=2, LastID=4, Modified=3"


    def test_06_ResetCounters(self):
        """
        Test the ResetCounters command.
        """

        # NOTE: Current initial version is simplified to only increment COMMAND_COUNTER and COMMAND_ERROR_COUNTER before reset.

        # Cause COMMAND_COUNTER to increment
        cmd("<%= target_name %> DS_CMD_NOOP")
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER > 0", 20)
        
        # Cause COMMAND_ERROR_COUNTER to increment
        cmd(f"<%= target_name %> SC_CMD_START_RTS with RTS_NUM 2")
        wait(5)
        cmd(f"<%= target_name %> SC_CMD_START_RTS with RTS_NUM 2")
        wait_check(f"<%= target_name %> SC_HK COMMAND_ERROR_COUNTER > 0", 20)
        wait(7)

        # Send ResetCounters command
        cmd(f"<%= target_name %> SC_CMD_RESET_COUNTERS")
        
        # Verify counters are reset to zero
        wait_check(f"<%= target_name %> SC_HK COMMAND_COUNTER == 0", 20)
        wait_check(f"<%= target_name %> SC_HK COMMAND_ERROR_COUNTER == 0", 20)
        wait_check(f"<%= target_name %> SC_HK RTS_ACTIVE_CTR == 0", 20)
        wait_check(f"<%= target_name %> SC_HK RTS_ACTIVE_ERR_CTR == 0", 20)
        wait_check(f"<%= target_name %> SC_HK ATS_CMD_CTR == 0", 20)
        wait_check(f"<%= target_name %> SC_HK ATS_CMD_ERR_CTR == 0", 20)
        wait_check(f"<%= target_name %> SC_HK RTS_CMD_CTR == 0", 20)
        wait_check(f"<%= target_name %> SC_HK RTS_CMD_ERR_CTR == 0", 20)
        

    def setup(self):
        """
        Test Group Setup
        - Runs when Group Setup button is pressed
        - Runs before all scripts when Group Start is pressed
        """
        pass


    def teardown(self):
        """
        Test Group Teardown
        - Runs when Group Teardown button is pressed
        - Runs after all scripts when Group Start is pressed
        """
        pass
