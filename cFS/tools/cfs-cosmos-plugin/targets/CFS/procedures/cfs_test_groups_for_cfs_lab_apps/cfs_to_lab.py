from openc3.script.suite import Group

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfs_to_lab(Group):
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
        Group.print(f"Testing TO_LAB aliveness on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"TO_LAB_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> TO_LAB_HK COMMAND_COUNTER")

        # Check accepted NOOP command proving application is up and running
        cmd(f"<%= target_name %> TO_LAB_CMD_NOOP")
        wait_check(f"<%= target_name %> TO_LAB_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check accepted Reset Counters command
        cmd(f"<%= target_name %> TO_LAB_CMD_RESET_COUNTERS")
        wait_check(f"<%= target_name %> TO_LAB_HK COMMAND_COUNTER == 0", 100)


    def test_01_manage_tlm_subscriptions(self):
        """
        Test Management of Tlm Subscriptions
        - Load the Alternate TO_LAB Subscription Table (reduced telemetry to CFE/TO/CI)
        - Add a new subscription (not in the alt table)
        - Remove the new subscription
        """
        test_table_name = "TO_LAB.Subscriptions"
        test_table_filename = "/cf/to_lab_sub_alt.tbl"

        Group.print(f"Testing TO_Lab alt table on <%= target_name %>")

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

        # Get a Message ID for test input (must not be included in alt table config)
        test_mid = <%= get_cfs_pkt_msg_id('FM_HK', cfs_cpu_num_from_target_name(target_name)) %>

        # Verify that we have a recent TO_LAB HK packet
        wait_check_packet(f"<%= target_name %>", f"TO_LAB_HK", 1, 100)
        cmd_count = tlm(f"<%= target_name %> TO_LAB_HK COMMAND_COUNTER")

        # Check accepted Add Packet command
        cmd(f"<%= target_name %> TO_LAB_CMD_ADD_PACKET with STREAM_VALUE {test_mid}, FLAGS_PRIORITY 0, FLAGS_RELIABILITY 0, BUF_LIMIT 4")
        wait_check(f"<%= target_name %> TO_LAB_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Verify that we start receiving the new packet subscription
        wait_check_packet(f"<%= target_name %>", f"FM_HK", 1, 100)

        # Check accepted Remove Packet command
        cmd(f"<%= target_name %> TO_LAB_CMD_REMOVE_PACKET with STREAM_VALUE {test_mid}")
        wait_check(f"<%= target_name %> TO_LAB_HK COMMAND_COUNTER == {cmd_count + 1}", 100)


    def test_03_remove_all_packet_subscriptions(self):
        """
        Test Removing All Packet Subscriptions
        - Then reload the default table to bring telemetry back online
        """
        Group.print(f"Testing TO_LAB Removing All Packet Subscriptions on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"TO_LAB_HK", 1, 100)

        # Send Remove All command
        # The next few steps have to be done without telemetry confirmation
        cmd(f"<%= target_name %> TO_LAB_CMD_REMOVE_ALL")

        # Load, Validate, Activate the default table (to re-enable tlm flow)
        default_table_name = "TO_LAB.Subscriptions"
        default_table_filename = "/cf/to_lab_sub.tbl"
        cmd(f"<%= target_name %> CFE_TBL_CMD_LOAD with LOAD_FILENAME '{default_table_filename}'")
        wait(8)
        cmd(f"<%= target_name %> CFE_TBL_CMD_VALIDATE with ACTIVE_TABLE_FLAG INACTIVE, TABLE_NAME '{default_table_name}'")
        wait(8)
        cmd(f"<%= target_name %> CFE_TBL_CMD_ACTIVATE with TABLE_NAME '{default_table_name}'")

        # Verify that we are getting telemetry again
        wait_check_packet(f"<%= target_name %>", f"TO_LAB_HK", 1, 50)
        wait_check_packet(f"<%= target_name %>", f"CFE_ES_HK", 1, 50)


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
