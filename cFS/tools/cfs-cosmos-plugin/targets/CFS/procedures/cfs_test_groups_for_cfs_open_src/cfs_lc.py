from openc3.script.suite import Group

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfs_lc(Group):
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
        Group.print(f"Testing LC aliveness on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"LC_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> LC_HK COMMAND_COUNTER")

        # Check accepted NOOP command proving application is up and running
        cmd(f"<%= target_name %> LC_CMD_NOOP")
        wait_check(f"<%= target_name %> LC_HK COMMAND_COUNTER == {cmd_count + 1}", 20)

        # Check accepted Reset Counters command
        cmd(f"<%= target_name %> LC_CMD_RESET_COUNTERS")
        wait_check(f"<%= target_name %> LC_HK COMMAND_COUNTER == 0", 20)

    def test_01_set_lc_state(self):
        """
        FSW LC State Test
        - Send Set LC State Command for ACTIVE, PASSIVE, and DISABLED
            then verify the command was received (by checking the command counter
            incremented and state was set)
        - Send Set LC State Command with invalid state
            then verify the command was received (by checking the command error counter)
        """
        Group.print(f"Testing LC set LC State command on <%= target_name %>")
        
        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"LC_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> LC_HK COMMAND_COUNTER")
        
        # Assuming no one else is sending commands, grab the latest command err count
        cmd_err_count = tlm(f"<%= target_name %> LC_HK COMMAND_ERROR_COUNTER")
        
        # Check accepted Set LC State = 1 command
        cmd(f"<%= target_name %> LC_CMD_SET_LC_STATE with NEW_LC_STATE ACTIVE")
        wait_check(f"<%= target_name %> LC_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> LC_HK CURRENT_LC_STATE == 'ACTIVE'", 100)
        # NOTE: Expected event message: "Set LC state command: new state = 1"
        
        # Check accepted Set LC State = 2 command
        cmd(f"<%= target_name %> LC_CMD_SET_LC_STATE with NEW_LC_STATE PASSIVE")
        wait_check(f"<%= target_name %> LC_HK COMMAND_COUNTER == {cmd_count + 2}", 100)
        wait_check(f"<%= target_name %> LC_HK CURRENT_LC_STATE == 'PASSIVE'", 100)
        # NOTE: Expected event message: "Set LC state command: new state = 2"
        
        # Check accepted Set LC State = 3 command
        cmd(f"<%= target_name %> LC_CMD_SET_LC_STATE with NEW_LC_STATE DISABLED")
        wait_check(f"<%= target_name %> LC_HK COMMAND_COUNTER == {cmd_count + 3}", 100)
        wait_check(f"<%= target_name %> LC_HK CURRENT_LC_STATE == 'DISABLED'", 100)
        # NOTE: Expected event message: "Set LC state command: new state = 3"
        
        # Check accepted Set LC State = 4 (invalid) command
        cmd(f"<%= target_name %> LC_CMD_SET_LC_STATE with NEW_LC_STATE 4")
        wait_check(f"<%= target_name %> LC_HK COMMAND_ERROR_COUNTER == {cmd_err_count + 1}", 100)
        # NOTE: Expected event message: "Set LC state error: invalid state = 4"
            

    def test_02_set_ap_state(self):
        """
        FSW AP State Test
        - Send Set AP State Command for ACTIVE, PASSIVE, DISABLED, and PERM_OFF
            then verify the command was received (by checking the command counter
            incremented and state was set)
        - Send Set LC State Command with invalid state
            then verify the command was received (by checking the command error counter)
        """
        
        Group.print(f"Testing LC set ap state command on <%= target_name %>")
        
        # Load table for test
        cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")
        cmd(f"<%= target_name %> CFE_TBL_CMD_LOAD with LOAD_FILENAME '/cf/lc_def_adt-test.tbl'")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Validate table for test
        cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")
        cmd(f"<%= target_name %> CFE_TBL_CMD_VALIDATE with ACTIVE_TABLE_FLAG INACTIVE, TABLE_NAME 'LC.LC_ADT'")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        
        # Activate table for test
        cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")
        cmd(f"<%= target_name %> CFE_TBL_CMD_ACTIVATE with TABLE_NAME 'LC.LC_ADT'")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        
        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"LC_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> LC_HK COMMAND_COUNTER")
        
        # Assuming no one else is sending commands, grab the latest command err count
        cmd_err_count = tlm(f"<%= target_name %> LC_HK COMMAND_ERROR_COUNTER")
        
        # Check accepted Set AP State = 1 command
        cmd(f"<%= target_name %> LC_CMD_SET_AP_STATE with AP_NUMBER 0, NEW_AP_STATE ACTIVE")
        wait_check(f"<%= target_name %> LC_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> LC_HK AP_0_STATE == 'ACTIVE'", 100)
        # NOTE: Expected event message: "Set AP state command: AP = 0, New state = 1"
        
        # Check accepted Set AP State = 2 command
        cmd(f"<%= target_name %> LC_CMD_SET_AP_STATE with AP_NUMBER 0, NEW_AP_STATE PASSIVE")
        wait_check(f"<%= target_name %> LC_HK COMMAND_COUNTER == {cmd_count + 2}", 100)
        wait_check(f"<%= target_name %> LC_HK AP_0_STATE == 'PASSIVE'", 100)
        # NOTE: Expected event message: "Set AP state command: AP = 0, New state = 2"
        
        # Check accepted Set AP State = 3 command
        cmd(f"<%= target_name %> LC_CMD_SET_AP_STATE with AP_NUMBER 0, NEW_AP_STATE DISABLED")
        wait_check(f"<%= target_name %> LC_HK COMMAND_COUNTER == {cmd_count + 3}", 100)
        wait_check(f"<%= target_name %> LC_HK AP_0_STATE == 'DISABLED'", 100)
        # NOTE: Expected event message: "Set AP state command: AP = 0, New state = 3"
        
        # Check accepted Set AP State = 4 (invalid) command
        cmd(f"<%= target_name %> LC_CMD_SET_AP_STATE with AP_NUMBER 0, NEW_AP_STATE PERM_OFF")
        wait_check(f"<%= target_name %> LC_HK COMMAND_ERROR_COUNTER == {cmd_err_count + 1}", 100)
        # NOTE: Expected event message: "Set AP state error: AP = %d, Invalid new state = 4"


    def test_03_set_ap_perm_off(self):
        """
        FSW AP Perm Off Test
        - Send Set AP Perm Off Command with AP_NUMBER 0
            then verify the command was received (by checking the command counter
            incremented)
        """
        
        Group.print(f"Testing LC set ap perm off command on <%= target_name %>")
        
        # Load table for test
        cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")
        cmd(f"<%= target_name %> CFE_TBL_CMD_LOAD with LOAD_FILENAME '/cf/lc_def_adt-test.tbl'")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Validate table for test
        cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")
        cmd(f"<%= target_name %> CFE_TBL_CMD_VALIDATE with ACTIVE_TABLE_FLAG INACTIVE, TABLE_NAME 'LC.LC_ADT'")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        
        # Activate table for test
        cmd_count = tlm(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER")
        cmd(f"<%= target_name %> CFE_TBL_CMD_ACTIVATE with TABLE_NAME 'LC.LC_ADT'")
        wait_check(f"<%= target_name %> CFE_TBL_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"LC_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> LC_HK COMMAND_COUNTER")
        
        # Assuming no one else is sending commands, grab the latest command err count
        cmd_err_count = tlm(f"<%= target_name %> LC_HK COMMAND_ERROR_COUNTER")
        
        # Check accepted Set AP Perm Off Command
        cmd(f"<%= target_name %> LC_CMD_SET_AP_PERM_OFF with AP_NUMBER 0")
        wait_check(f"<%= target_name %> LC_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        # NOTE: Expected event message: "Set AP permanently off command: AP = 4"
        

    def test_04_reset_ap_stats(self):
        """
        FSW AP Stats Test
        - Send Reset AP Stats Command for AP_NUMBER 0
            then verify the command was received (by checking the command counter
            incremented)
        """
        
        Group.print(f"Testing LC reset ap stats command on <%= target_name %>")
        
        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"LC_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> LC_HK COMMAND_COUNTER")
        
        # Assuming no one else is sending commands, grab the latest command err count
        cmd_err_count = tlm(f"<%= target_name %> LC_HK COMMAND_ERROR_COUNTER")
        
        # Check accepted Reset AP Stats Command
        cmd(f"<%= target_name %> LC_CMD_RESET_AP_STATS with AP_NUMBER 0")
        wait_check(f"<%= target_name %> LC_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        # NOTE: Expected event message: "Reset AP stats command: AP = 0"


    def test_05_reset_wp_stats(self):
        """
        FSW WP Stats Test
        - Send Reset WP Stats Command for AP_NUMBER 0
            then verify the command was received (by checking the command counter
            incremented)
        """
        
        Group.print(f"Testing LC reset wp stats command on <%= target_name %>")
        
        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"LC_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> LC_HK COMMAND_COUNTER")
        
        # Assuming no one else is sending commands, grab the latest command err count
        cmd_err_count = tlm(f"<%= target_name %> LC_HK COMMAND_ERROR_COUNTER")
        
        # Check accepted Reset WP Stats Command
        cmd(f"<%= target_name %> LC_CMD_RESET_WP_STATS with WP_NUMBER 0")
        wait_check(f"<%= target_name %> LC_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        # NOTE: Expected event message: "Reset WP stats command: WP = 0"


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
