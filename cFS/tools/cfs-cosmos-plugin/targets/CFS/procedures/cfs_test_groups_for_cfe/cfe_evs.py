from openc3.script.suite import Group

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfe_evs(Group):
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

        Group.print(f"Testing CFE_EVS aliveness on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_EVS_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")

        # Check accepted NOOP command proving application is up and running
        cmd(f"<%= target_name %> CFE_EVS_CMD_NOOP")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check accepted Reset Counters command
        cmd(f"<%= target_name %> CFE_EVS_CMD_RESET_COUNTERS")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == 0", 100)

    def test_01_event_type(self):
        """
        Test Enable/Disable Event Type
        - Send enable event type
            then verify the command was received (by checking the command counter incremented)
        - Send disable event type
            then verify the command was received (by checking the command counter incremented)
        """

        Group.print(f"Testing CFE_EVS Disable/Enable Event Type on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_EVS_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")

        # Check accepted Enable Event Type command
        cmd(f"<%= target_name %> CFE_EVS_CMD_ENABLE_EVENT_TYPE with BIT_MASK DEBUG")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check accepted Disable Event Type command
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")
        cmd(f"<%= target_name %> CFE_EVS_CMD_DISABLE_EVENT_TYPE with BIT_MASK DEBUG")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

    def test_02_set_event_format_mode(self):
        """
        Test Set Event Format Mode
        - Send set event format mode
            then verify the command was received (by checking the command counter incremented)
        """

        Group.print(f"Testing CFE_EVS Set Event Format Mode on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_EVS_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")

        # Check accepted Set Event Format Mode command
        cmd(f"<%= target_name %> CFE_EVS_CMD_SET_EVENT_FORMAT_MODE with MSG_FORMAT SHORT")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Restore default event format
        cmd(f"<%= target_name %> CFE_EVS_CMD_SET_EVENT_FORMAT_MODE with MSG_FORMAT LONG")

    def test_03_app_event_type(self):
        """
        Test Enable/Disable App Event Type
        - Send enable app event type
            then verify the command was received (by checking the command counter incremented)
        - Send disable app event type
            then verify the command was received (by checking the command counter incremented)
        """

        Group.print(f"Testing CFE_EVS Enable/Disable App Event Type on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_EVS_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")

        # Check accepted Enable App Event Type command
        cmd(f"<%= target_name %> CFE_EVS_CMD_ENABLE_APP_EVENT_TYPE with APP_NAME SAMPLE_APP, BIT_MASK DEBUG")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check accepted Disable App Event Type command
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")
        cmd(f"<%= target_name %> CFE_EVS_CMD_DISABLE_APP_EVENT_TYPE with APP_NAME SAMPLE_APP, BIT_MASK DEBUG")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

    def test_04_app_events(self):
        """
        Test Disable/Enable App Events
        - Send disable app events
            then verify the command was received (by checking the command counter incremented)
        - Send enable app events
            then verify the command was received (by checking the command counter incremented)
        """

        Group.print(f"Testing CFE_EVS Enable/Disable App Events on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_EVS_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")

        # Check accepted Disable App Events Type command
        cmd(f"<%= target_name %> CFE_EVS_CMD_DISABLE_APP_EVENTS with APP_NAME SAMPLE_APP")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check accepted Enable App Events Type command
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")
        cmd(f"<%= target_name %> CFE_EVS_CMD_ENABLE_APP_EVENTS with APP_NAME SAMPLE_APP")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

    def test_05_reset_app_counter(self):
        """
        Test Reset App Counter Commands
        - Send reset app counter
            then verify the command was received (by checking the command counter incremented)
        """

        Group.print(f"Testing CFE_EVS Reset App Counter on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_EVS_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")

        # Check accepted Reset App Counter command
        cmd(f"<%= target_name %> CFE_EVS_CMD_RESET_APP_COUNTER with APP_NAME SAMPLE_APP")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

    def test_06_filter_commands(self):
        """
        Test Filter Commands
        - Send add event filter
            then verify the command was received (by checking the command counter incremented)
        - Send set filter
            then verify the command was received (by checking the command counter incremented)
        - Send reset filter
            then verify the command was received (by checking the command counter incremented)
        - Send delete event filter
            then verify the command was received (by checking the command counter incremented)
        - Send reset event filter
            then verify the command was received (by checking the command counter incremented)
        - Send reset all filters
            then verify the command was received (by checking the command counter incremented)
        """

        Group.print(f"Testing CFE_EVS Filter Commands on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_EVS_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")

        # Check accepted Add Event Filter command
        cmd(f"<%= target_name %> CFE_EVS_CMD_ADD_EVENT_FILTER with APP_NAME SAMPLE_APP, EVENT_ID 3, MASK 0x0001")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check accepted Set Filter command
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")
        cmd(f"<%= target_name %> CFE_EVS_CMD_SET_FILTER with APP_NAME SAMPLE_APP, EVENT_ID 3, MASK 0x0003")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check accepted Reset Filter command
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")
        cmd(f"<%= target_name %> CFE_EVS_CMD_RESET_FILTER with APP_NAME SAMPLE_APP, EVENT_ID 3")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check accepted Delete Event Filter command
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")
        cmd(f"<%= target_name %> CFE_EVS_CMD_DELETE_EVENT_FILTER with APP_NAME SAMPLE_APP, EVENT_ID 3")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check accepted Reset All Filters command
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")
        cmd(f"<%= target_name %> CFE_EVS_CMD_RESET_ALL_FILTERS with APP_NAME SAMPLE_APP")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

    def test_07_ports(self):
        """
        Test Enable/Disable Ports
        - Send Enable Ports
            then verify the command was received (by checking the command counter incremented)
        - Send Disable Ports
            then verify the command was received (by checking the command counter incremented)
        """

        Group.print(f"Testing CFE_EVS Enable/Disable Ports on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_EVS_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")

        # Check accepted Enable Ports command
        cmd(f"<%= target_name %> CFE_EVS_CMD_ENABLE_PORTS with BIT_MASK 2")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check accepted Disable Ports command
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")
        cmd(f"<%= target_name %> CFE_EVS_CMD_DISABLE_PORTS with BIT_MASK 2")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

    def test_08_write_app_data_file(self):
        """
        Test Write App Data Files
        - Send Write App Data File
            then verify the command was received (by checking the command counter incremented)
        """

        Group.print(f"Testing CFE_EVS Write App Data Files on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_EVS_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")

        # Check accepted Write App Data File command
        cmd(f"<%= target_name %> CFE_EVS_CMD_WRITE_APP_DATA_FILE with APP_DATA_FILENAME '/cf/evs_app.dat'")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

    def test_09_write_log_data_file(self):
        """
        Test Log Commands
        - Send Write Log Data File
            then verify the command was received (by checking the command counter incremented)
        """

        Group.print(f"Testing CFE_EVS Write Log Data File on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_EVS_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")

        # Check accepted Write Log Data File command
        cmd(f"<%= target_name %> CFE_EVS_CMD_WRITE_LOG_DATA_FILE with LOG_FILENAME '/cf/evs_log.dat'")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

    def test_10_set_log_mode(self):
        """
        Test Set Log Mode
        - Send Set Log Mode
            then verify the command was received (by checking the command counter incremented)
        """

        Group.print(f"Testing CFE_EVS Set Log Mode on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_EVS_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")

        # Check accepted Set Log Mode command
        cmd(f"<%= target_name %> CFE_EVS_CMD_SET_LOG_MODE with LOG_MODE OVERWRITE")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Restore default log mode
        cmd(f"<%= target_name %> CFE_EVS_CMD_SET_LOG_MODE with LOG_MODE DISCARD")

    def test_11_clear_log(self):
        """
        Test Clear Log
        - Send Clear Log
            then verify the command was received (by checking the command counter incremented)
        """

        Group.print(f"Testing CFE_EVS Clear Log on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_EVS_HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER")

        # Check accepted Clear Log command
        cmd(f"<%= target_name %> CFE_EVS_CMD_CLEAR_LOG")
        wait_check(f"<%= target_name %> CFE_EVS_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

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