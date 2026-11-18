from openc3.script.suite import Group
# Verify ES commands work properly.  Not testing error cases.

sample_app_needs_shutdown_at_end = False

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfe_es(Group):
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
        
        Group.print("Testing CFE_ES aliveness on <%= target_name %>")
        
        # Assuming no one else is sending commands, grab the latest command count
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm("<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send NOOP command, then check result to prove application is up and running
        cmd("<%= target_name %> CFE_ES_CMD_NOOP")
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
        
        # Send Reset Counters command, check resullt
        cmd("<%= target_name %> CFE_ES_CMD_RESET_COUNTERS")
        wait_check("<%= target_name %> CFE_ES_HK COMMAND_COUNTER == 0", 20)
    
    
    def test_01_NoOp(self):
        """
        Test the no-op command.
        """
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        cmd("<%= target_name %> CFE_ES_CMD_NOOP")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
    
    
    # FIXME: Currently don't have a good way to restart cFS from COSMOS.
    #        Default CFS platform just stops FSW from a reset command.
    def test_02_ProcessorRestart(self):
        """
        Test the Restart command specifying Processor Restart.
        """
            
        Group.print("Test Skipped: Currently do not have a good way to restart cFS from COSMOS")
        Group.print("              Default CFS platform just stops FSW from a reset command.")
        
        ## Send command under test
        #cmd(f"<%= target_name %> CFE_ES_CMD_RESTART with RESTART_TYPE PROCESSOR")
        #
    
    
    # FIXME: Currently don't have a good way to restart cFS from COSMOS.
    #        Default CFS platform just stops FSW from a reset command.
    def test_03_PowerOnRestart(self):
        """
        Test the Restart command specifying Power-On Restart.
        """
        Group.print("Test Skipped: Currently do not have a good way to restart cFS from COSMOS")
        Group.print("              Default CFS platform just stops FSW from a reset command.")
        
        ## Send command under test
        #cmd(f"<%= target_name %> CFE_ES_CMD_RESTART with RESTART_TYPE POWER_ON")
        #
    
    
    def test_04_StopApp_StartApp(self):
        """
        Test the StopApp and StartApp commands.
        It is determined whether the Sample_App is running at the start or not
        and then the order of the test steps are changed based on this info.
        the Sample_App will be in the same state as it was when this test began 
        """
        
        if(self.is_Sample_App_Running()):
            #Sample_App is running
            self.StopApp_test()
            wait(1)
            self.StartApp_test()
            wait(1)
        else:
            # Sample_app is not running
            self.StartApp_test()
            wait(1)
            self.StopApp_test()
            wait(1)
    
    
    def test_05_RestartApp(self):
        """
        Test the RestartApp command.
        """
        
        stopSampleAppAtEnd = False
        # This test needs the Sample_App running
        # If it is not running, start it, setting a var to stop it at the end of the test
        # This is to make this test runnable even if the setup function was not executed
        if(not self.is_Sample_App_Running()):
            stopSampleAppAtEnd = True
            self.StartApp_test()
            wait(1)
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_RESTART_APP with APPLICATION 'SAMPLE_APP'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
        
        wait(10) # Wait for the app to fully restart (5 for stop, 5 for start).
        
        if(stopSampleAppAtEnd):
            self.StopApp_test()
    
    
    def test_06_ReloadApp(self):
        """
        Test the ReloadApp command.
        """
        
        stopSampleAppAtEnd = False
        # This test needs the Sample_App running
        # If it is not running, start it, setting a var to stop it at the end of the test
        # This is to make this test runnable even if the setup function was not executed
        if(not self.is_Sample_App_Running()):
            stopSampleAppAtEnd = True
            self.StartApp_test()
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_RELOAD_APP with APPLICATION 'SAMPLE_APP', APP_FILE_NAME 'sample_app'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
        
        wait(10) # Wait for the app to fully restart (5 for stop, 5 for start).

        if(stopSampleAppAtEnd):
            self.StopApp_test()
    
    
    def test_07_QueryOne(self):
        """
        Test the QueryOne command.
        """
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_QUERY_ONE with APPLICATION 'CS'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
    
    
    def test_08_QueryAll(self):
        """
        Test the QueryAll command.
        """
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_QUERY_ALL with FILE_NAME '/cf/apps.txt'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
    
    
    def test_09_QueryAllTasks(self):
        """
        Test the QueryAllTasks command.
        """
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under tests
        cmd(f"<%= target_name %> CFE_ES_CMD_QUERY_ALL_TASKS with FILE_NAME '/cf/sample_app'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
    
    
    def test_10_WriteSysLog(self):
        """
        Test the WriteSysLog command.
        """
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm("<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd("<%= target_name %> CFE_ES_CMD_WRITE_SYS_LOG with FILE_NAME '/cf/logfile.txt'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
    
    
    def test_11_ClearSysLog(self):
        """
        Test the ClearSysLog command.
        """
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_CLEAR_SYS_LOG")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
    
    
    def test_12_OverwriteSysLog_Discard(self):
        """
        Test the OverwriteSysLog command.
        """
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_OVER_WRITE_SYS_LOG with MODE DISCARD")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
    
    
    def test_13_OverwriteSysLog_Overwrite(self):
        """
        Test the OverwriteSysLog command.
        """
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_OVER_WRITE_SYS_LOG with MODE OVERWRITE")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
    
    
    def test_14_WriteERLog(self):
        """
        Test the WriteERLog command.
        """
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_WRITE_ER_LOG with FILE_NAME '/cf/logfile.txt'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
    
    
    def test_15_ClearERLog(self):
        """
        Test the ClearERLog command.
        """
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_CLEAR_ER_LOG")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
    
    
    def test_16_StartPerfData(self):
        """
        Test the StartPerfData command.
        """
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_START_PERF_DATA with TRIGGER_MODE START")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
    
    
    def test_17_StopPerfData(self):
        """
        Test the StopPerfData command.
        """
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_STOP_PERF_DATA with DATA_FILE_NAME '/cf/datafile.txt'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
    
    
    def test_18_SetPerfFilterMask(self):
        """
        Test the SetPerfFilterMask command.
        """
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_SET_PERF_FILTER_MASK with FILTER_MASK_NUM 1, FILTER_MASK 10")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
    
    
    def test_19_SetPerfTriggerMask(self):
        """
        Test the SetPerfTriggerMask command.
        """
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_SET_PERF_TRIGGER_MASK with TRIGGER_MASK_NUM 1, TRIGGER_MASK 10")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
    
    
    def test_20_ResetPRCount(self):
        """
        Test the ResetPRCount command.
        """
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_RESET_PR_COUNT")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
    
    
    def test_21_SetMaxPRCount(self):
        """
        Test the SetMaxPRCount command.
        """
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_SET_MAX_PR_COUNT with MAX_PR_COUNT 1")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
    
    
    # FIXME: Add this back when we start implementing non-nominal test cases.
    #
    # def test_X_DeleteCDS_Error_AppActive(self):
    #     """
    #     Test the DeleteCDS command.
    #     """
    #
    #     # First, delete the CS app, so we're allowed to delete its CDS
    #     CFE_ES_DeleteApp(AppID)
    #
    #     wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
    #     cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
    #
    #     # Send command under test
    #     cmd(f"<%= target_name %> CFE_ES_CMD_DELETE_CDS with CDS_NAME 'CS.CS_CDS'")
    #
    #     # Verify command count incremented
    #     wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
    #
    
    
    def test_22_DeleteCDS(self):
        """
        Test the DeleteCDS command.
        """
        
        # First, stop the CS app, so we're allowed to delete its CDS
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        cmd(f"<%= target_name %> CFE_ES_CMD_STOP_APP with APPLICATION 'CS'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
        
        wait(5)  # Wait for the CS app to finish exiting
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_DELETE_CDS with CDS_NAME 'CS.CS_CDS'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
        
        # Re-start the CS app, to restore the system state to as it was before this step
        cmd(f"<%= target_name %> CFE_ES_CMD_START_APP with APPLICATION 'CS', APP_ENTRY_POINT 'CS_AppMain', APP_FILE_NAME 'cs', STACK_SIZE 16384, EXCEPTION_ACTION 0, PRIORITY 65")
    
    
    def test_23_SendMemPoolStats(self):
        """
        Test the SendMemPoolStats command.
        """
        
        # Get and save SB HK mempool handle.  Needed for command.
        wait_check_packet("<%= target_name %>", "CFE_SB_HK", 1, 20)
        sb_mempool_handle = tlm("<%= target_name %> CFE_SB_HK MEM_POOL_HANDLE")
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_SEND_MEM_POOL_STATS with APPLICATION 'TO', POOL_HANDLE {sb_mempool_handle}")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
    
    
    def test_24_DumpCDSRegistry(self):
        """
        Test the DumpCDSRegistry command.
        """
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_DUMP_CDS_REGISTRY with DUMP_FILENAME '/cf/dumpfile.txt'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
    
    
    def test_25_ResetCounters(self):
        """
        Test the ResetCounters command.
        """
        
        # First, cause command error counter to increment.  Send cmd with invalid parameter.
        cmd(f"<%= target_name %> CFE_ES_CMD_DUMP_CDS_REGISTRY with DUMP_FILENAME 1")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_RESET_COUNTERS")
        
        # Verify counter are reset to zero
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == 0", 20)
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_ERROR_COUNTER == 0", 20)
    
    
    def is_Sample_App_Running(self) -> bool:
        """
        This function determines if the Sample_App is running
        
        Returns:
            bool: 
                - True if Sample_App is running
                - False if Sample_App is notrunning
        """
        # save counters to dermine if Sample_App is running
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        saved_cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        seq_count = tlm(f"<%= target_name %> CFE_ES_HK CCSDS_SEQUENCE")
        
        # Send QUERY_ONE command to test if Sample_app is running or not
        cmd(f"<%= target_name %> CFE_ES_CMD_QUERY_ONE with APPLICATION 'SAMPLE_APP'")
        
        # wait for a new packet
        wait(1)
        
        # If this check fails, stop the test, no telemetry packet was received within the timeout
        wait_check(f"<%= target_name %> CFE_ES_HK CCSDS_SEQUENCE > {seq_count}", 20)
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        current_cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        if(current_cmd_count == (saved_cmd_count + 1)):
            # Sample_app is found to be running
            return True
        else:
            # Sample_app is not found to be running or the wait_check failed (no tlm received)
            return False
    
    
    def StopApp_test(self):
        """
        Test the StopApp command.
        Called from the StopApp_StartApp test
        """
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_STOP_APP with APPLICATION 'SAMPLE_APP'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)
        
        wait(5) # wait for the application to stop
    
    
    def StartApp_test(self):
        """
        Test the StartApp command.
        Called from the StopApp_StartApp test
        """
        
        wait_check_packet("<%= target_name %>", "CFE_ES_HK", 1, 20)
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")

        # Send command under test
        cmd(f"<%= target_name %> CFE_ES_CMD_START_APP with APPLICATION 'SAMPLE_APP', APP_ENTRY_POINT 'SAMPLE_APP_Main', APP_FILE_NAME 'sample_app', STACK_SIZE 16384, EXCEPTION_ACTION 0, PRIORITY 50")

        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count + 1}", 20)

        wait(5) # wait for the application to start
    
    
    def setup(self):
        """
        Test Group Setup
        - Runs when Group Setup button is pressed
        - Runs before all scripts when Group Start is pressed
        """
        global sample_app_needs_shutdown_at_end # lets python know we plan to change this global in this function

        # Wait for a new housekeeping packet, to ensure we're using its latest status info
        wait_check_packet(f"<%= target_name %>", f"CFE_ES_HK", 1, 20)

        # If the Sample_App is not running, start it, setting a var to stop it at the end of the test
        # this call here ensures that the system is in the state needed 
        if(not self.is_Sample_App_Running()):
            sample_app_needs_shutdown_at_end = True
            self.StartApp_test()
        
        # Ensure that ES events are enabled
        cmd("<%= target_name %> CFE_EVS_CMD_ENABLE_APP_EVENTS with APP_NAME 'CFE_ES'")
        wait(1)
        
        # Ensure that DEBUG and INFO events are enabled
        cmd("<%= target_name %> CFE_EVS_CMD_ENABLE_EVENT_TYPE with BIT_MASK DEBUG")
        cmd("<%= target_name %> CFE_EVS_CMD_ENABLE_EVENT_TYPE with BIT_MASK INFO")
        wait(1)
        

    def teardown(self):
        """
        Test Group Setup
        - Runs when Group Teardown button is pressed
        - Runs after all scripts when Group Start is pressed
        """
        
        # this call here ensures that the system is in the state it was in when it started 
        if(sample_app_needs_shutdown_at_end):
            self.StopApp_test()
        
