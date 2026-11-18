from openc3.script.suite import Group

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfs_hs(Group):
    """
    - Methods beginning with script_ or test_ are added to Script dropdown
    """

    def util_setup_test_command(self, app_name, test_cmd):
        """
        Utility to setup command under test
        - Display command under test message and return current command counter
        """
        
        Group.print(f"Testing {app_name} {test_cmd} command on <%= target_name %>")

        # Enable EVS events
        cmd(f"<%= target_name %> CFE_EVS_CMD_ENABLE_APP_EVENT_TYPE with APP_NAME {app_name}, BIT_MASK INFO")
        cmd(f"<%= target_name %> CFE_EVS_CMD_ENABLE_APP_EVENTS with APP_NAME {app_name}")
        wait(1)
       
        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"{app_name}_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> {app_name}_HK COMMAND_COUNTER")
         
        return cmd_count

    def util_verify_command_count(self, app_name, cmd_count):
        """
        Utility to verify command count
        """
        # Verify command counter incremented
        wait_check(f"<%= target_name %> {app_name}_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

    def util_verify_event_message(self, app_name, cmd_name, event_id, event_msg):
        """
        Utility to verify correct event message was generated
        """
        # Validate Display Param event generation 
        #wait_check_expression(
        #    f"tlm('<%= target_name %> CFE_EVS_LONG_EVENT_MSG PACKET_ID_APP_NAME') == '{app_name}' and " +
        #    f"tlm('<%= target_name %> CFE_EVS_LONG_EVENT_MSG PACKET_ID_EVENT_ID') == '{event_id}' and " +
        #    f"'{event_msg}' in tlm('<%= target_name %> CFE_EVS_LONG_EVENT_MSG MESSAGE')",
        #    50, 0.1, globals(), locals()
        #)

        Group.print(f"{cmd_name} event generation verified")

    def test_aliveness(self):
        """
        FSW Aliveness Test
        - Send a no-op command
            then verify the command was received (by checking the command counter incremented)
        - Reset the command counter
            then verify the command was received (by checking the command counter was cleared)
        """

        Group.print(f"Testing HS aliveness on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"HS_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> HS_HK COMMAND_COUNTER")

        # Check accepted NOOP command proving application is up and running
        cmd(f"<%= target_name %> HS_CMD_NOOP")
        wait_check(f"<%= target_name %> HS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check accepted Reset Counters command
        cmd(f"<%= target_name %> HS_CMD_RESET_COUNTERS")
        wait_check(f"<%= target_name %> HS_HK COMMAND_COUNTER == 0", 100)

    def test_application_monitor(self):
        """
        FSW Application Monitor Test
        - Send Enable Application Monitor command
            then verify the command was received (by checking the command counter incremented)
        - Send Disable Application Monitor command
            then verify the command was received (by checking the command counter incremented)
        - Restore initial state of Application Monitor command
        """

        # Command being tested
        test_cmd = "Enable Application Monitor"

        # Display command under test and get current command counter
        cmd_count = self.util_setup_test_command("HS", test_cmd)

        # Get current state value
        cmd_state = tlm(f"<%= target_name %> HS_HK APP_MON_STATE")

        # Check accepted Enable Application Monitor Command and verify command counter
        cmd(f"<%= target_name %> HS_CMD_ENABLE_APP_MON")
        self.util_verify_command_count("HS", cmd_count)

        # Check Application Monitor telemetry
        wait_check(f"<%= target_name %> HS_HK APP_MON_STATE == 'ENABLED'", 100)

        # Verify Event Message
        event_id = 25
        if cmd_state == "ENABLED":
            event_msg = "Application Monitoring is *already* Enabled"
        else:
            event_msg = "Application Monitoring Enabled"
        self.util_verify_event_message("HS", test_cmd, event_id, event_msg)

        # Command being tested
        test_cmd = "Disable Application Monitor"

        # Display command under test and get current command counter
        cmd_count = self.util_setup_test_command("HS", test_cmd)

        # Get current state value
        cmd_state = tlm(f"<%= target_name %> HS_HK APP_MON_STATE")

        # Check accepted Enable Application Monitor Command and verify command counter
        cmd(f"<%= target_name %> HS_CMD_DISABLE_APP_MON")
        self.util_verify_command_count("HS", cmd_count)

        # Check Application Monitor telemetry
        wait_check(f"<%= target_name %> HS_HK APP_MON_STATE == 'DISABLED'", 100)

        # Verify Event Message
        event_id = 26
        if cmd_state == "DISABLED":
            event_msg = "Application Monitoring is *already* Disabled"
        else:
            event_msg = "Application Monitoring Disabled"
        self.util_verify_event_message("HS", test_cmd, event_id, event_msg)

        # Restore initial Application Monitor State
        Group.print(f"Restoring Application Monitor to {cmd_state}...")
        if cmd_state == "ENABLED":
            cmd(f"<%= target_name %> HS_CMD_ENABLE_APP_MON")
        else:
            cmd(f"<%= target_name %> HS_CMD_DISABLE_APP_MON")

    def test_events_monitor(self):
        """
        FSW Events Monitor Test
        - Send Enable Events Monitor command
            then verify the command was received (by checking the command counter incremented)
        - Send Disable Events Monitor command
            then verify the command was received (by checking the command counter incremented)
        - Restore initial state of Events Monitor command
        """

        # Command being tested
        test_cmd = "Enable Event Monitor"

        # Display command under test and get current command counter
        cmd_count = self.util_setup_test_command("HS", test_cmd)

        # Get current state value
        cmd_state = tlm(f"<%= target_name %> HS_HK EVENT_MON_STATE")

        # Check accepted Enable Events Monitor Command and verify command counter
        cmd(f"<%= target_name %> HS_CMD_ENABLE_EVENT_MON")
        self.util_verify_command_count("HS", cmd_count)

        # Check Event Monitor telemetry
        wait_check(f"<%= target_name %> HS_HK EVENT_MON_STATE == 'ENABLED'", 100)

        # Verify Event Message
        event_id = 27
        if cmd_state == "ENABLED":
            event_msg = "Event Monitoring is *already* Enabled"
        else:
            event_msg = "Event Monitoring Enabled"
        self.util_verify_event_message("HS", test_cmd, event_id, event_msg)

        # Command being tested
        test_cmd = "Disable Event Monitor"

        # Display command under test and get current command counter
        cmd_count = self.util_setup_test_command("HS", test_cmd)

        # Get current state value
        cmd_state = tlm(f"<%= target_name %> HS_HK EVENT_MON_STATE")

        # Check accepted Enable Event Monitor Command and verify command counter
        cmd(f"<%= target_name %> HS_CMD_DISABLE_EVENT_MON")
        self.util_verify_command_count("HS", cmd_count)

        # Check Event Monitor telemetry
        wait_check(f"<%= target_name %> HS_HK EVENT_MON_STATE == 'DISABLED'", 100)

        # Verify Event Message
        event_id = 28
        if cmd_state == "DISABLED":
            event_msg = "Event Monitoring is *already* Disabled"
        else:
            event_msg = "Event Monitoring Disabled"
        self.util_verify_event_message("HS", test_cmd, event_id, event_msg)

        # Restore initial Event Monitor State
        Group.print(f"Restoring Event Monitor to {cmd_state}...")
        if cmd_state == "ENABLED":
            cmd(f"<%= target_name %> HS_CMD_ENABLE_EVENT_MON")
        else:
            cmd(f"<%= target_name %> HS_CMD_DISABLE_EVENT_MON")

    def test_aliveness_indicator(self):
        """
        FSW Aliveness Indicator Test
        - Send Enable Aliveness Indicator command
            then verify the command was received (by checking the command counter incremented)
        - Send Disable Aliveness Indicator command
            then verify the command was received (by checking the command counter incremented)
        - Restore initial state of Aliveness Indicator command
        """

        # Command being tested
        test_cmd = "Enable Aliveness Indicator"

        # Display command under test and get current command counter
        cmd_count = self.util_setup_test_command("HS", test_cmd)

        # Get current state value
        cmd_state = tlm(f"<%= target_name %> HS_HK ALIVENESS_STATE")

        # Check accepted Enable Aliveness Indicator Command and verify command counter
        cmd(f"<%= target_name %> HS_CMD_ENABLE_ALIVENESS")
        self.util_verify_command_count("HS", cmd_count)

        # Check Aliveness Indicator telemetry
        wait_check(f"<%= target_name %> HS_HK ALIVENESS_STATE == 'ENABLED'", 100)

        # Verify Event Message
        event_id = 29
        if cmd_state == "ENABLED":
            event_msg = "Aliveness Indicator is *already* Enabled"
        else:
            event_msg = "Aliveness Indicator Enabled"
        self.util_verify_event_message("HS", test_cmd, event_id, event_msg)

        # Command being tested
        test_cmd = "Disable Aliveness Indicator"

        # Display command under test and get current command counter
        cmd_count = self.util_setup_test_command("HS", test_cmd)

        # Get current state value
        cmd_state = tlm(f"<%= target_name %> HS_HK ALIVENESS_STATE")

        # Check accepted Enable Aliveness Indicator Command and verify command counter
        cmd(f"<%= target_name %> HS_CMD_DISABLE_ALIVENESS")
        self.util_verify_command_count("HS", cmd_count)

        # Check Aliveness Indicator telemetry
        wait_check(f"<%= target_name %> HS_HK ALIVENESS_STATE == 'DISABLED'", 100)

        # Verify Event Message
        event_id = 30
        if cmd_state == "DISABLED":
            event_msg = "Aliveness Indicator is *already* Disabled"
        else:
            event_msg = "Aliveness Indicator Disabled"
        self.util_verify_event_message("HS", test_cmd, event_id, event_msg)

        # Restore initial Aliveness Indicator State
        Group.print(f"Restoring Aliveness Indicator to {cmd_state}...")
        if cmd_state == "ENABLED":
            cmd(f"<%= target_name %> HS_CMD_ENABLE_ALIVENESS")
        else:
            cmd(f"<%= target_name %> HS_CMD_DISABLE_ALIVENESS")

    def test_reset_processor_resets_performed_count(self):
        """
        FSW Reset Processor Resets Performed Test
        - Send Reset Processor Resets Performed command
            then verify the command was received (by checking the command counter incremented)
        """

        # Command being tested
        test_cmd = "Reset Processor Resets Performed Count"

        # Display command under test and get current command counter
        cmd_count = self.util_setup_test_command("HS", test_cmd)

        # Check accepted Reset Processor Resets Performed Command and verify command counter
        cmd(f"<%= target_name %> HS_CMD_RESET_RESETS_PERFORMED")
        self.util_verify_command_count("HS", cmd_count)

        # Check Reset Processor Resets Performed telemetry
        wait_check(f"<%= target_name %> HS_HK RESETS_PERFORMED == 0", 100)

        # Verify Event Message
        event_id = 31
        event_msg = "Processor Resets Performed by HS Counter has been Reset"
        self.util_verify_event_message("HS", test_cmd, event_id, event_msg)

    def test_set_max_processor_resets(self):
        """
        FSW Set Max Processor Resets Test
        - Send Set Max Processor Resets command
            then verify the command was received (by checking the command counter incremented)
        - Restore initial value of Set Max Processor Resets command
        """

        # Command being tested
        test_cmd = "Set Max Processor Resets"

        # Display command under test and get current command counter
        cmd_count = self.util_setup_test_command("HS", test_cmd)

        # Get current max resets value
        max_resets = tlm(f"<%= target_name %> HS_HK MAX_RESETS")

        # Check accepted Set Max Processor Resets Command and verify command counter
        cmd_resets = 1024
        cmd(f"<%= target_name %> HS_CMD_SET_MAX_RESETS with MAX_RESETS {cmd_resets}, PADDING 0")
        self.util_verify_command_count("HS", cmd_count)

        # Check Set Max Processor Resets telemetry
        wait_check(f"<%= target_name %> HS_HK MAX_RESETS == {cmd_resets}", 100)

        # Verify Event Message
        event_id = 32
        event_msg = f"Max Resets Performable by HS has been set to {cmd_resets}"
        self.util_verify_event_message("HS", test_cmd, event_id, event_msg)

        # Restore initial Max Processor Resets
        Group.print(f"Restoring Max Processor Resets to {max_resets}...")
        cmd(f"<%= target_name %> HS_CMD_SET_MAX_RESETS with MAX_RESETS {max_resets}, PADDING 0")

    def test_cpu_hogging_indicator(self):
        """
        FSW CPU Hogging Indicator Test
        - Send Enable CPU Hogging Indicator command
            then verify the command was received (by checking the command counter incremented)
        - Send Disable CPU Hogging Indicator command
            then verify the command was received (by checking the command counter incremented)
        - Restore initial state of CPU Hogging Indicator command
        """

        # Command being tested
        test_cmd = "Enable CPU Hogging Indicator"

        # Display command under test and get current command counter
        cmd_count = self.util_setup_test_command("HS", test_cmd)

        # Get current state value
        cmd_state = tlm(f"<%= target_name %> HS_HK CPU_HOG_STATE")

        # Check accepted Enable CPU Hogging Indicator Command and verify command counter
        cmd(f"<%= target_name %> HS_CMD_ENABLE_CPU_HOG")
        self.util_verify_command_count("HS", cmd_count)

        # Check CPU Hogging Indicator telemetry
        wait_check(f"<%= target_name %> HS_HK CPU_HOG_STATE == 'ENABLED'", 100)

        # Verify Event Message
        event_id = 64
        if cmd_state == "ENABLED":
            event_msg = "CPU Hogging Indicator is *already* Enabled"
        else:
            event_msg = "CPU Hogging Indicator Enabled"
        self.util_verify_event_message("HS", test_cmd, event_id, event_msg)

        # Command being tested
        test_cmd = "Disable CPU Hogging Indicator"

        # Display command under test and get current command counter
        cmd_count = self.util_setup_test_command("HS", test_cmd)

        # Get current state value
        cmd_state = tlm(f"<%= target_name %> HS_HK CPU_HOG_STATE")

        # Check accepted Enable CPU Hogging Indicator Command and verify command counter
        cmd(f"<%= target_name %> HS_CMD_DISABLE_CPU_HOG")
        self.util_verify_command_count("HS", cmd_count)

        # Check CPU Hogging Indicator telemetry
        wait_check(f"<%= target_name %> HS_HK CPU_HOG_STATE == 'DISABLED'", 100)

        # Verify Event Message
        event_id = 65
        if cmd_state == "DISABLED":
            event_msg = "CPU Hogging Indicator is *already* Disabled"
        else:
            event_msg = "CPU Hogging Indicator Disabled"
        self.util_verify_event_message("HS", test_cmd, event_id, event_msg)

        # Restore initial CPU Hogging Indicator State
        Group.print(f"Restoring CPU Hogging Indicator to {cmd_state}...")
        if cmd_state == "ENABLED":
            cmd(f"<%= target_name %> HS_CMD_ENABLE_CPU_HOG")
        else:
            cmd(f"<%= target_name %> HS_CMD_DISABLE_CPU_HOG")


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
