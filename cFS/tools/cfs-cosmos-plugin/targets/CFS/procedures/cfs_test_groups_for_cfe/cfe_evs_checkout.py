from openc3.script.suite import Group

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfe_evs_checkout(Group):
    """
    - Methods beginning with script_ or test_ are added to Script dropdown
    """

    def test_cfe_evs_checkout(self):
        """
        CFE EVS Checkout Test
        -Send all EVS commands as quickly as possible, then verify all were accepted
        """

        set_line_delay(0.0)

        Group.print(f"CFE_EVS checkout test on <%= target_name %>")

        # Save initial command counts
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")
        cmd_err_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_ERROR_COUNTER")

        set_line_delay(0.15)  # 0.15 is about as fast as it can go.  0.1 causes a missed cmd.

        cmd(f"<%= target_name %> CFE_EVS_CMD_NOOP")
        cmd(f"<%= target_name %> CFE_EVS_CMD_DISABLE_EVENT_TYPE with BIT_MASK DEBUG")
        cmd(f"<%= target_name %> CFE_EVS_CMD_ENABLE_EVENT_TYPE with BIT_MASK DEBUG")
        cmd(f"<%= target_name %> CFE_EVS_CMD_SET_EVENT_FORMAT_MODE with MSG_FORMAT SHORT")
        cmd(f"<%= target_name %> CFE_EVS_CMD_ENABLE_APP_EVENT_TYPE with APP_NAME SAMPLE_APP, BIT_MASK DEBUG")
        cmd(f"<%= target_name %> CFE_EVS_CMD_DISABLE_APP_EVENT_TYPE with APP_NAME SAMPLE_APP, BIT_MASK DEBUG")
        cmd(f"<%= target_name %> CFE_EVS_CMD_DISABLE_APP_EVENTS with APP_NAME SAMPLE_APP")
        cmd(f"<%= target_name %> CFE_EVS_CMD_ENABLE_APP_EVENTS with APP_NAME SAMPLE_APP")
        cmd(f"<%= target_name %> CFE_EVS_CMD_RESET_APP_COUNTER with APP_NAME SAMPLE_APP")
        cmd(f"<%= target_name %> CFE_EVS_CMD_ADD_EVENT_FILTER with APP_NAME SAMPLE_APP, EVENT_ID 3, MASK 0x0001")
        cmd(f"<%= target_name %> CFE_EVS_CMD_SET_FILTER with APP_NAME SAMPLE_APP, EVENT_ID 3, MASK 0x0003")
        cmd(f"<%= target_name %> CFE_EVS_CMD_RESET_FILTER with APP_NAME SAMPLE_APP, EVENT_ID 3")
        cmd(f"<%= target_name %> CFE_EVS_CMD_DELETE_EVENT_FILTER with APP_NAME SAMPLE_APP, EVENT_ID 3")
        cmd(f"<%= target_name %> CFE_EVS_CMD_RESET_ALL_FILTERS with APP_NAME SAMPLE_APP")
        cmd(f"<%= target_name %> CFE_EVS_CMD_ENABLE_PORTS with BIT_MASK 2")
        cmd(f"<%= target_name %> CFE_EVS_CMD_DISABLE_PORTS with BIT_MASK 2")
        cmd(f"<%= target_name %> CFE_EVS_CMD_WRITE_APP_DATA_FILE with APP_DATA_FILENAME '/cf/evs_app.dat'")
        cmd(f"<%= target_name %> CFE_EVS_CMD_WRITE_LOG_DATA_FILE with LOG_FILENAME '/cf/evs_log.dat'")
        cmd(f"<%= target_name %> CFE_EVS_CMD_SET_LOG_MODE with LOG_MODE OVERWRITE")
        cmd(f"<%= target_name %> CFE_EVS_CMD_SET_LOG_MODE with LOG_MODE DISCARD")
        cmd(f"<%= target_name %> CFE_EVS_CMD_CLEAR_LOG")

        set_line_delay(0.0)
        
        # Check final command count has incremented by the number of commands sent
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER >= {cmd_count} + 21", 12)
        check(f"<%= target_name %> CFE_EVS_HK COMMAND_ERROR_COUNTER == {cmd_err_count}")

        cmd(f"<%= target_name %> CFE_EVS_CMD_RESET_COUNTERS")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == 0", 12)


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