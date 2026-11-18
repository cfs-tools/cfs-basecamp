from openc3.script.suite import Group
# Verify DS commands work properly.  Not testing error cases.

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfs_ds(Group):
    """
    - Methods beginning with script_ or test_ are added to Script dropdown
    """
    
    def test_00_Aliveness(self):
        """
        FSW Aliveness Test
        - Send a no-op command
            then verify the command was received (by checking the command counter incremented)
        - Reset the command counter
            then verify the command was received (by checking the command counter was cleared)
        """
        
        Group.print("Testing DS aliveness on <%= target_name %>")
        
        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", "DS_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> DS_HK COMMAND_COUNTER")

        # Check accepted NOOP command proving application is up and running
        cmd(f"<%= target_name %> DS_CMD_NOOP")
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check accepted Reset Counters command
        cmd(f"<%= target_name %> DS_CMD_RESET_COUNTERS")
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER == 0", 100)
    
    
    def test_01_NoOp(self):
        """
        Test the no-op command.
        """
        
        cmd_count = tlm(f"<%= target_name %> DS_HK COMMAND_COUNTER")
        
        cmd("<%= target_name %> DS_CMD_NOOP")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER >= {cmd_count + 1}", 100)


    def test_02_SetAppState(self):
        """
        Test the SetAppState command.
        """
        
        cmd_count = tlm(f"<%= target_name %> DS_HK COMMAND_COUNTER")
        
        # Send the command under test
        cmd("<%= target_name %> DS_CMD_SET_APP_STATE with ENABLE_STATE ENABLED")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER >= {cmd_count + 1}", 100)

        # Verify other telemetry changed as expected
        wait_check(f"<%= target_name %> DS_HK APP_ENABLE_STATE == 1", 100)
    

    def test_03_AddMid(self):
        """
        Test the AddMid command.
        """
        
        cmd_count = tlm(f"<%= target_name %> DS_HK COMMAND_COUNTER")
        
        # Send the command under test
        cmd("<%= target_name %> DS_CMD_ADD_MID with MESSAGE_ID 0x1A00")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER >= {cmd_count + 1}", 100)


    def test_04_SetFilterFile(self):
        """
        Test the SetFilterFile command.
        """
        
        cmd_count = tlm(f"<%= target_name %> DS_HK COMMAND_COUNTER")
        
        # Send the command under test
        cmd("<%= target_name %> DS_CMD_SET_FILTER_FILE with MESSAGE_ID 0x1A00, FILTER_PARAMS_IDX 0, FILE_TABLE_IDX 0")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER >= {cmd_count + 1}", 100)
    

    def test_05_SetFilterType(self):
        """
        Test the SetFilterType command.
        """
        
        cmd_count = tlm(f"<%= target_name %> DS_HK COMMAND_COUNTER")
        
        # Send the command under test
        cmd("<%= target_name %> DS_CMD_SET_FILTER_TYPE with MESSAGE_ID 0x1A00, FILTER_PARAMS_IDX 0, FILTER_TYPE BY_COUNT")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER >= {cmd_count + 1}", 100)
    

    def test_06_SetFilterParms(self):
        """
        Test the SetFilterParms command.
        """

        # NOTE: Source code abbreviates as "parms", while COSMOS file abbreviates as "params".
        
        cmd_count = tlm(f"<%= target_name %> DS_HK COMMAND_COUNTER")
        
        # Send the command under test
        cmd("<%= target_name %> DS_CMD_SET_FILTER_PARAMS with MESSAGE_ID 0x1A00, FILTER_PARAMS_IDX 0, ALGORITHM_N 0, ALGORITHM_X 0, ALGORITHM_O 0")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER >= {cmd_count + 1}", 100)
    

    def test_07_SetDestType(self):
        """
        Test the SetDestType command.
        """
        
        cmd_count = tlm(f"<%= target_name %> DS_HK COMMAND_COUNTER")
        
        # Send the command under test
        cmd("<%= target_name %> DS_CMD_SET_DEST_TYPE with FILE_TABLE_IDX 0, FILE_NAME_TYPE BY_COUNT")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER >= {cmd_count + 1}", 100)
    

    def test_08_SetDestState(self):
        """
        Test the SetDestState command.
        """
        
        cmd_count = tlm(f"<%= target_name %> DS_HK COMMAND_COUNTER")
        
        # Send the command under test
        cmd("<%= target_name %> DS_CMD_SET_DEST_STATE with FILE_TABLE_IDX 0, ENABLE_STATE DISABLED")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER >= {cmd_count + 1}", 100)
    

    def test_09_SetDestPath(self):
        """
        Test the SetDestPath command.
        """
        
        cmd_count = tlm(f"<%= target_name %> DS_HK COMMAND_COUNTER")
        
        # Send the command under test
        cmd("<%= target_name %> DS_CMD_SET_DEST_PATH with FILE_TABLE_IDX 0, PATHNAME '/cf/'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER >= {cmd_count + 1}", 100)
    

    def test_10_SetDestBase(self):
        """
        Test the SetDestBase command.
        """
        
        cmd_count = tlm(f"<%= target_name %> DS_HK COMMAND_COUNTER")
        
        # Send the command under test
        cmd("<%= target_name %> DS_CMD_SET_DEST_BASE with FILE_TABLE_IDX 0, BASENAME 'base'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER >= {cmd_count + 1}", 100)
    

    def test_11_SetDestExt(self):
        """
        Test the SetDestExt command.
        """
        
        cmd_count = tlm(f"<%= target_name %> DS_HK COMMAND_COUNTER")
        
        # Send the command under test
        cmd("<%= target_name %> DS_CMD_SET_DEST_EXT with FILE_TABLE_IDX 0, EXTENSION '.txt'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER >= {cmd_count + 1}", 100)


    def test_12_SetDestSize(self):
        """
        Test the SetDestSize command.
        """
        
        cmd_count = tlm(f"<%= target_name %> DS_HK COMMAND_COUNTER")
        
        # Send the command under test
        cmd("<%= target_name %> DS_CMD_SET_DEST_SIZE with FILE_TABLE_IDX 0, MAX_FILE_SIZE 1024")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER >= {cmd_count + 1}", 100)


    def test_13_SetDestAge(self):
        """
        Test the SetDestAge command.
        """
        
        cmd_count = tlm(f"<%= target_name %> DS_HK COMMAND_COUNTER")
        
        # Send the command under test
        cmd("<%= target_name %> DS_CMD_SET_DEST_AGE with FILE_TABLE_IDX 0, MAX_FILE_AGE 60")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER >= {cmd_count + 1}", 100)


    def test_14_SetDestCount(self):
        """
        Test the SetDestCount command.
        """
        
        cmd_count = tlm(f"<%= target_name %> DS_HK COMMAND_COUNTER")
        
        # Send the command under test
        cmd("<%= target_name %> DS_CMD_SET_DEST_COUNT with FILE_TABLE_IDX 0, SEQUENCE_COUNT 1")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER >= {cmd_count + 1}", 100)


    def test_15_CloseFile(self):
        """
        Test the CloseFile command.
        """
        
        cmd_count = tlm(f"<%= target_name %> DS_HK COMMAND_COUNTER")
        
        # Send the command under test
        cmd("<%= target_name %> DS_CMD_CLOSE_FILE with FILE_TABLE_IDX 0")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER >= {cmd_count + 1}", 100)


    def test_16_CloseAll(self):
        """
        Test the CloseAll command.
        """
        
        cmd_count = tlm(f"<%= target_name %> DS_HK COMMAND_COUNTER")
        
        # Send the command under test
        cmd("<%= target_name %> DS_CMD_CLOSE_ALL")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER >= {cmd_count + 1}", 100)


    def test_17_GetFileInfo(self):
        """
        Test the GetFileInfo command.
        """
        
        cmd_count = tlm(f"<%= target_name %> DS_HK COMMAND_COUNTER")
        
        # Send the command under test
        cmd("<%= target_name %> DS_CMD_GET_FILE_INFO")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER >= {cmd_count + 1}", 100)


    def test_18_RemoveMid(self):
        """
        Test the RemoveMid command.
        """
        
        cmd_count = tlm(f"<%= target_name %> DS_HK COMMAND_COUNTER")
        
        # Send the command under test
        cmd("<%= target_name %> DS_CMD_REMOVE_MID with MESSAGE_ID 0x1A00")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER >= {cmd_count + 1}", 100)


    def test_19_ResetCounters(self):
        """
        Test the ResetCounters command.
        """

        # NOTE: Current initial version is simplified to only increment COMMAND_COUNTER and COMMAND_ERROR_COUNTER before reset.

        # Increment COMMAND_COUNTER by sending CreateDirectory command
        cmd("<%= target_name %> DS_CMD_NOOP")
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER > 0", 100)
        
        # Cause COMMAND_ERROR_COUNTER to increment,
        # by sending SetFilterFile cmd with invalid message ID
        cmd("<%= target_name %> DS_CMD_SET_FILTER_FILE with MESSAGE_ID 0x9999, FILTER_PARAMS_IDX 0, FILE_TABLE_IDX 0")
        wait_check(f"<%= target_name %> DS_HK COMMAND_ERROR_COUNTER > 0", 100)

        # Send ResetCounters command
        cmd(f"<%= target_name %> DS_CMD_RESET_COUNTERS")
        
        # Verify counters are reset to zero
        wait_check(f"<%= target_name %> DS_HK COMMAND_COUNTER == 0", 100)
        wait_check(f"<%= target_name %> DS_HK COMMAND_ERROR_COUNTER == 0", 100)
        wait_check(f"<%= target_name %> DS_HK DISABLED_PKT_COUNTER == 0", 100)
        wait_check(f"<%= target_name %> DS_HK IGNORED_PKT_COUNTER == 0", 100)
        wait_check(f"<%= target_name %> DS_HK FILTERED_PKT_COUNTER < 10", 100)  # Increases from 0 before first packet post-reset
        wait_check(f"<%= target_name %> DS_HK PASSED_PKT_COUNTER == 0", 100)
        wait_check(f"<%= target_name %> DS_HK FILE_WRITE_COUNTER == 0", 100)
        wait_check(f"<%= target_name %> DS_HK FILE_WRITE_ERR_COUNTER == 0", 100)
        wait_check(f"<%= target_name %> DS_HK FILE_UPDATE_COUNTER == 0", 100)
        wait_check(f"<%= target_name %> DS_HK FILE_UPDATE_ERR_COUNTER == 0", 100)
        wait_check(f"<%= target_name %> DS_HK DEST_TBL_LOAD_COUNTER == 0", 100)
        wait_check(f"<%= target_name %> DS_HK DEST_TBL_ERR_COUNTER == 0", 100)
        wait_check(f"<%= target_name %> DS_HK FILTER_TBL_LOAD_COUNTER == 0", 100)
        wait_check(f"<%= target_name %> DS_HK FILTER_TBL_ERR_COUNTER == 0", 100)


    def setup(self):
        """
        Test Group Setup
        - Runs when Group Setup button is pressed
        - Runs before all scripts when Group Start is pressed
        """
        # Wait for a new housekeeping packet, to ensure we're using its latest status info
        wait_check_packet(f"<%= target_name %>", "DS_HK", 1, 100)

        # Ensure that DS events are enabled
        cmd("<%= target_name %> CFE_EVS_CMD_ENABLE_APP_EVENTS with APP_NAME 'DS'")
        wait(1)
        
        # Ensure that DEBUG and INFO events are enabled
        cmd("<%= target_name %> CFE_EVS_CMD_ENABLE_EVENT_TYPE with BIT_MASK DEBUG")
        cmd("<%= target_name %> CFE_EVS_CMD_ENABLE_EVENT_TYPE with BIT_MASK INFO")
        wait(1)
        pass


    def teardown(self):
        """
        Test Group Setup
        - Runs when Group Teardown button is pressed
        - Runs after all scripts when Group Start is pressed
        """
        pass