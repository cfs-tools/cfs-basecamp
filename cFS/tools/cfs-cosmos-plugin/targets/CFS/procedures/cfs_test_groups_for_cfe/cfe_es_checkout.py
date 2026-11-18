from openc3.script.suite import Group

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfe_es_checkout(Group):
    """
    - Methods beginning with script_ or test_ are added to Script dropdown
    """

    def test_cfe_es_checkout(self):
        """
        CFE ES Checkout Test
        -Send all ES commands as quickly as possible, then verify all were accepted
        """

        set_line_delay(0.0)

        Group.print(f"CFE_ES checkout test on <%= target_name %>")

        # Save initial command counts
        cmd_count = tlm(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER")

        set_line_delay(0.15)  # 0.15 is about as fast as it can go.  0.1 causes a missed cmd.

        cmd(f"<%= target_name %> CFE_ES_CMD_NOOP")

        # FIXME: Currently don't have a good way to restart cFS from COSMOS.
        #        Default CFS platform just stops FSW from a reset command.
        #
        #cmd(f"<%= target_name %> CFE_ES_CMD_RESTART with RESTART_TYPE PROCESSOR")
        #cmd(f"<%= target_name %> CFE_ES_CMD_RESTART with RESTART_TYPE POWER_ON")

        cmd(f"<%= target_name %> CFE_ES_CMD_QUERY_ONE with APPLICATION 'CS'")

        cmd(f"<%= target_name %> CFE_ES_CMD_STOP_APP with APPLICATION 'CS'")        
        wait(7) # wait for the application to stop
        cmd(f"<%= target_name %> CFE_ES_CMD_DELETE_CDS with CDS_NAME 'CS.CS_CDS'")
        cmd(f"<%= target_name %> CFE_ES_CMD_START_APP with APPLICATION 'CS', APP_ENTRY_POINT 'CS_AppMain', APP_FILE_NAME 'cs', STACK_SIZE 16384, EXCEPTION_ACTION 0, PRIORITY 65")
        wait(7) # wait for the application to start

        cmd(f"<%= target_name %> CFE_ES_CMD_RESTART_APP with APPLICATION 'CS'")
        wait(14) # Wait for the app to fully restart

        cmd(f"<%= target_name %> CFE_ES_CMD_RELOAD_APP with APPLICATION 'CS', APP_FILE_NAME 'cs'")

        cmd(f"<%= target_name %> CFE_ES_CMD_QUERY_ALL with FILE_NAME '/cf/apps.txt'")
        cmd(f"<%= target_name %> CFE_ES_CMD_QUERY_ALL_TASKS with FILE_NAME '/cf/cs'")
        cmd(f"<%= target_name %> CFE_ES_CMD_WRITE_SYS_LOG with FILE_NAME '/cf/logfile.txt'")
        cmd(f"<%= target_name %> CFE_ES_CMD_CLEAR_SYS_LOG")
        cmd(f"<%= target_name %> CFE_ES_CMD_OVER_WRITE_SYS_LOG with MODE DISCARD")
        cmd(f"<%= target_name %> CFE_ES_CMD_OVER_WRITE_SYS_LOG with MODE OVERWRITE")
        cmd(f"<%= target_name %> CFE_ES_CMD_WRITE_ER_LOG with FILE_NAME '/cf/logfile.txt'")
        cmd(f"<%= target_name %> CFE_ES_CMD_CLEAR_ER_LOG")
        cmd(f"<%= target_name %> CFE_ES_CMD_START_PERF_DATA with TRIGGER_MODE START")
        cmd(f"<%= target_name %> CFE_ES_CMD_STOP_PERF_DATA with DATA_FILE_NAME '/cf/datafile.txt'")
        cmd(f"<%= target_name %> CFE_ES_CMD_SET_PERF_FILTER_MASK with FILTER_MASK_NUM 1, FILTER_MASK 10")
        cmd(f"<%= target_name %> CFE_ES_CMD_SET_PERF_TRIGGER_MASK with TRIGGER_MASK_NUM 1, TRIGGER_MASK 10")
        cmd(f"<%= target_name %> CFE_ES_CMD_RESET_PR_COUNT")
        cmd(f"<%= target_name %> CFE_ES_CMD_SET_MAX_PR_COUNT with MAX_PR_COUNT 1")

        sb_mempool_handle = tlm("<%= target_name %> CFE_SB_HK MEM_POOL_HANDLE")
        cmd(f"<%= target_name %> CFE_ES_CMD_SEND_MEM_POOL_STATS with APPLICATION 'TO', POOL_HANDLE {sb_mempool_handle}")
        cmd(f"<%= target_name %> CFE_ES_CMD_DUMP_CDS_REGISTRY with DUMP_FILENAME '/cf/dumpfile.txt'")

        set_line_delay(0.0)
        
        # Check final command count has incremented by the number of commands sent
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == {cmd_count} + 23", 12)

        cmd(f"<%= target_name %> CFE_ES_CMD_RESET_COUNTERS")
        wait_check(f"<%= target_name %> CFE_ES_HK COMMAND_COUNTER == 0", 12)
    

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