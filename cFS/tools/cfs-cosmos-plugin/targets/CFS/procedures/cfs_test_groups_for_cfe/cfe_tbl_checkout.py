from openc3.script.suite import Group

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfe_tbl_checkout(Group):
    """
    - Methods beginning with script_ or test_ are added to Script dropdown
    """

    def test_cfe_tbl_checkout(self):
        """
        CFE TBL Checkout Test
        -Send all TBL commands as quickly as possible, then verify all were accepted
        """

        set_line_delay(0.0)

        Group.print(f"CFE_TBL checkout test on <%= target_name %>")

        # Save initial command counts
        cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")
        cmd_err_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_ERROR_COUNTER")

        set_line_delay(0.15)  # 0.15 is about as fast as it can go.  0.1 causes a missed cmd.

        cmd(f"<%= target_name %> CFE_TBL_CMD_NOOP")

        test_table_name = "SAMPLE_APP.ExampleTable"
        test_table_filename = "/cf/sample_app_tbl.tbl"
        cmd(f"<%= target_name %> CFE_TBL_CMD_LOAD with LOAD_FILENAME '{test_table_filename}'")
        cmd(f"<%= target_name %> CFE_TBL_CMD_VALIDATE with ACTIVE_TABLE_FLAG INACTIVE, TABLE_NAME '{test_table_name}'")
        wait(5) # Table validate must finish before beginning subsequent Activate command
        cmd(f"<%= target_name %> CFE_TBL_CMD_ACTIVATE with TABLE_NAME '{test_table_name}'")
        wait(5)

        cmd(f"<%= target_name %> CFE_TBL_CMD_LOAD with LOAD_FILENAME '{test_table_filename}'")
        cmd(f"<%= target_name %> CFE_TBL_CMD_ABORT_LOAD with TABLE_NAME '{test_table_name}'")
        cmd(f"<%= target_name %> CFE_TBL_CMD_DUMP_REGISTRY with DUMP_FILENAME 'dump_registry.dat'")
        cmd(f"<%= target_name %> CFE_TBL_CMD_SEND_REGISTRY with TABLE_NAME '{test_table_name}'")

        cmd(f"<%= target_name %> CFE_ES_CMD_STOP_APP with APPLICATION 'MD'")
        wait(8)

        # Note: This currently depends on a CFS app other than Sample App, because the Sample App
        #       example table is not registered as critical in FSW.  This may change in the future.
        cmd(f"<%= target_name %> CFE_TBL_CMD_DELETE_CDS with TABLE_NAME 'MD.DWELL_TABLE4'")
        cmd(f"<%= target_name %> CFE_ES_CMD_START_APP with APPLICATION 'MD', APP_ENTRY_POINT 'MD_AppMain', APP_FILE_NAME '/cf/md.so'")

        set_line_delay(0.0)
        
        # Check final command count has incremented by the number of commands sent
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER >= {cmd_count} + 9", 12)
        check(f"<%= target_name %> CFE_TBL_HK COMMAND_ERROR_COUNTER == {cmd_err_count}")

        cmd(f"<%= target_name %> CFE_TBL_CMD_RESET_COUNTERS")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == 0", 12)


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
