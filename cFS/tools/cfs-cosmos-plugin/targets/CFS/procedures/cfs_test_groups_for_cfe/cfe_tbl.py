from openc3.script.suite import Group

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfe_tbl(Group):
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
        Group.print(f"Testing CFE_TBL aliveness on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_TBL_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")

        # Check accepted NOOP command proving application is up and running
        cmd(f"<%= target_name %> CFE_TBL_CMD_NOOP")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check accepted Reset Counters command
        cmd(f"<%= target_name %> CFE_TBL_CMD_RESET_COUNTERS")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == 0", 100)


    def test_01_load_single_buffered_table(self):
        """
        Test Load Table Command on a Single-Buffered table
        - The DS File Table is a Single buffered table
        """
        test_table_name = "DS.FILE_TBL"
        test_table_filename = "/cf/ds_file_tbl.tbl"

        Group.print(f"Testing CFE_TBL Load and Validate Table on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_TBL_HK", 1, 100)

        # Test Table Load command (should take up shared buffer)
        cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")
        load_pending_count = tlm(f"<%= target_name %> CFE_TBL_HK NUM_LOAD_PENDING")
        num_free_shared_bufs = tlm(f"<%= target_name %> CFE_TBL_HK NUM_FREE_SHARED_BUFS")
        cmd(f"<%= target_name %> CFE_TBL_CMD_LOAD with LOAD_FILENAME '{test_table_filename}'")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK NUM_LOAD_PENDING == {load_pending_count + 1}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK NUM_FREE_SHARED_BUFS == {num_free_shared_bufs - 1}", 100)

        # Test Table Validate command
        cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")
        tbl_validation_count = tlm(f"<%= target_name %> CFE_TBL_HK VALIDATION_COUNTER")
        tbl_validation_success_count = tlm(f"<%= target_name %> CFE_TBL_HK SUCCESS_VAL_COUNTER")
        tbl_validation_request_count = tlm(f"<%= target_name %> CFE_TBL_HK NUM_VAL_REQUESTS")
        cmd(f"<%= target_name %> CFE_TBL_CMD_VALIDATE with ACTIVE_TABLE_FLAG INACTIVE, TABLE_NAME '{test_table_name}'")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK VALIDATION_COUNTER == {tbl_validation_count + 1}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK SUCCESS_VAL_COUNTER == {tbl_validation_success_count + 1}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK NUM_VAL_REQUESTS == {tbl_validation_request_count + 1}", 100)

        # Test Table Activate command
        cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")
        load_pending_count = tlm(f"<%= target_name %> CFE_TBL_HK NUM_LOAD_PENDING")
        num_free_shared_bufs = tlm(f"<%= target_name %> CFE_TBL_HK NUM_FREE_SHARED_BUFS")
        cmd(f"<%= target_name %> CFE_TBL_CMD_ACTIVATE with TABLE_NAME '{test_table_name}'")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK NUM_LOAD_PENDING == {load_pending_count - 1}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK NUM_FREE_SHARED_BUFS == {num_free_shared_bufs + 1}", 100)

    def test_02_load_double_buffered_table(self):
        """
        Test Load Table Command on a Double-Buffered table
        - The HK Copy Tables are double-buffered, so they require the load/validate/activate steps sent separately
        """
        test_table_name = "HK.CopyTable"
        test_table_filename = "/cf/hk_cpy_tbl.tbl"

        Group.print(f"Testing CFE_TBL Load and Validate Table on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_TBL_HK", 1, 100)

        # Test Table Load command (should not take up shared buffer)
        cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")
        load_pending_count = tlm(f"<%= target_name %> CFE_TBL_HK NUM_LOAD_PENDING")
        num_free_shared_bufs = tlm(f"<%= target_name %> CFE_TBL_HK NUM_FREE_SHARED_BUFS")
        cmd(f"<%= target_name %> CFE_TBL_CMD_LOAD with LOAD_FILENAME '{test_table_filename}'")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK NUM_LOAD_PENDING == {load_pending_count + 1}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK NUM_FREE_SHARED_BUFS == {num_free_shared_bufs}", 100)

        # Test Table Validate command
        cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")
        tbl_validation_count = tlm(f"<%= target_name %> CFE_TBL_HK VALIDATION_COUNTER")
        tbl_validation_success_count = tlm(f"<%= target_name %> CFE_TBL_HK SUCCESS_VAL_COUNTER")
        tbl_validation_request_count = tlm(f"<%= target_name %> CFE_TBL_HK NUM_VAL_REQUESTS")
        cmd(f"<%= target_name %> CFE_TBL_CMD_VALIDATE with ACTIVE_TABLE_FLAG INACTIVE, TABLE_NAME '{test_table_name}'")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK VALIDATION_COUNTER == {tbl_validation_count + 1}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK SUCCESS_VAL_COUNTER == {tbl_validation_success_count + 1}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK NUM_VAL_REQUESTS == {tbl_validation_request_count + 1}", 100)

        # Test Table Activate command
        cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")
        load_pending_count = tlm(f"<%= target_name %> CFE_TBL_HK NUM_LOAD_PENDING")
        num_free_shared_bufs = tlm(f"<%= target_name %> CFE_TBL_HK NUM_FREE_SHARED_BUFS")
        cmd(f"<%= target_name %> CFE_TBL_CMD_ACTIVATE with TABLE_NAME '{test_table_name}'")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK NUM_LOAD_PENDING == {load_pending_count - 1}", 100)

    def test_03_load_bad_table(self):
        """
        Test Load Table Command on an invalid Single-Buffered table
        - The TO_Lab Subscriptions Table is a Single buffered table
        """
        test_table_name = "TO_LAB.Subscriptions"
        test_table_filename = "/cf/to_lab_sub_bad.tbl"

        Group.print(f"Testing CFE_TBL Load and Validate Table on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_TBL_HK", 1, 100)

        # Test Table Load command (should take up shared buffer)
        cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")
        load_pending_count = tlm(f"<%= target_name %> CFE_TBL_HK NUM_LOAD_PENDING")
        num_free_shared_bufs = tlm(f"<%= target_name %> CFE_TBL_HK NUM_FREE_SHARED_BUFS")
        cmd(f"<%= target_name %> CFE_TBL_CMD_LOAD with LOAD_FILENAME '{test_table_filename}'")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK NUM_LOAD_PENDING == {load_pending_count + 1}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK NUM_FREE_SHARED_BUFS == {num_free_shared_bufs - 1}", 100)

        # Test Table Validate command (should report error)
        cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")
        tbl_validation_count = tlm(f"<%= target_name %> CFE_TBL_HK VALIDATION_COUNTER")
        tbl_validation_success_count = tlm(f"<%= target_name %> CFE_TBL_HK SUCCESS_VAL_COUNTER")
        tbl_validation_failure_count = tlm(f"<%= target_name %> CFE_TBL_HK FAILED_VAL_COUNTER")
        tbl_validation_request_count = tlm(f"<%= target_name %> CFE_TBL_HK NUM_VAL_REQUESTS")
        cmd(f"<%= target_name %> CFE_TBL_CMD_VALIDATE with ACTIVE_TABLE_FLAG INACTIVE, TABLE_NAME '{test_table_name}'")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK VALIDATION_COUNTER == {tbl_validation_count + 1}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK SUCCESS_VAL_COUNTER == {tbl_validation_success_count}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK FAILED_VAL_COUNTER == {tbl_validation_failure_count + 1}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK NUM_VAL_REQUESTS == {tbl_validation_request_count + 1}", 100)

        # Abort Table Load to clear shared table buffers
        cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")
        load_pending_count = tlm(f"<%= target_name %> CFE_TBL_HK NUM_LOAD_PENDING")
        num_free_shared_bufs = tlm(f"<%= target_name %> CFE_TBL_HK NUM_FREE_SHARED_BUFS")
        cmd(f"<%= target_name %> CFE_TBL_CMD_ABORT_LOAD with TABLE_NAME '{test_table_name}'")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK NUM_LOAD_PENDING == {load_pending_count - 1}", 100)
        wait_check(f"<%= target_name %> CFE_TBL_HK NUM_FREE_SHARED_BUFS == {num_free_shared_bufs + 1}", 100)


    def test_04_dump_table_registry(self):
        """
        Test Dump Table Registry
        - Send a dump registry command
            then verify the command was received (by checking the command counter incremented)
        """
        Group.print(f"Testing CFE_TBL Dump Table Registry on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_TBL_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")

        # Check accepted Dump Registry command
        cmd(f"<%= target_name %> CFE_TBL_CMD_DUMP_REGISTRY with DUMP_FILENAME 'dump_registry.dat'")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {cmd_count + 1}", 100)


    def test_05_send_table_registry(self):
        """
        Test Send Table Registry
        - Send a send registry command
            then verify the command was received (by checking the command counter incremented)
        """
        Group.print(f"Testing CFE_TBL Send Table Registry on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_TBL_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")

        # Check accepted Send Registry command
        cmd(f"<%= target_name %> CFE_TBL_CMD_SEND_REGISTRY with TABLE_NAME 'MD.DWELL_TABLE4'")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {cmd_count + 1}", 100)


    def test_06_delete_cds(self):
        """
        Test Delete CDS Registry
        - Send a delete cds command
            then verify the command was received (by checking the command counter incremented)
        """
        Group.print(f"Testing CFE_TBL Send Delete CDS on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_TBL_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")

        # Stop MD
        cmd(f"<%= target_name %> CFE_ES_CMD_STOP_APP with APPLICATION 'MD'")
        wait(10)

        # Check accepted Delete CDS command
        cmd(f"<%= target_name %> CFE_TBL_CMD_DELETE_CDS with TABLE_NAME 'MD.DWELL_TABLE4'")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Start MD
        cmd(f"<%= target_name %> CFE_ES_CMD_START_APP with APPLICATION 'MD', APP_ENTRY_POINT 'MD_AppMain', APP_FILE_NAME '/cf/md.so'")
        wait(10)
        cmd(f"<%= target_name %> CFE_ES_CMD_RESTART_APP with APPLICATION 'MD'")
        wait(10)

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
