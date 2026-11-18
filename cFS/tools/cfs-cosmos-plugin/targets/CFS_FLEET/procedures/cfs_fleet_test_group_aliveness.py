from openc3.script.suite import Group

# Load the cFE Fleet Class Utility Methods
load_utility('<%= target_name %>/lib/cfs_fleet.py')

# Group class name should indicate what the scripts are testing
class cfs_fleet_test_group_aliveness(Group):
    """
    - Methods beginning with script_ or test_ are added to Script dropdown
    """

    def test_aliveness(self):
        """
        FSW Aliveness Test
        - Send a no-op command to each cFS FSW app in the system
            then verify the command was received (by checking telemetry)
        - Reset the command counter
            then verify the command was received (by checking telemetry)
        """
        # Create a list of fsw targets
        fleet = CFS_Fleet()
        fsw_targets_to_test = fleet.get_cfs_fleet_target_name_list()

        # Create a list of app noops to test
        fsw_apps_to_test = []
        fsw_apps_to_test.append(["CFE_ES_CMD_NOOP",     "CFE_ES_CMD_RESET_COUNTERS",     "CFE_ES_HK",      "COMMAND_COUNTER" ])
        fsw_apps_to_test.append(["CFE_EVS_CMD_NOOP",    "CFE_EVS_CMD_RESET_COUNTERS",    "CFE_EVS_HK",     "COMMAND_COUNTER" ])
        fsw_apps_to_test.append(["CFE_SB_CMD_NOOP",     "CFE_SB_CMD_RESET_COUNTERS",     "CFE_SB_HK",      "COMMAND_COUNTER" ])
        fsw_apps_to_test.append(["CFE_TBL_CMD_NOOP",    "CFE_TBL_CMD_RESET_COUNTERS",    "CFE_TBL_HK",     "COMMAND_COUNTER" ])
        fsw_apps_to_test.append(["CFE_TIME_CMD_NOOP",   "CFE_TIME_CMD_RESET_COUNTERS",   "CFE_TIME_HK",    "COMMAND_COUNTER" ])
        fsw_apps_to_test.append(["CF_CMD_NOOP",         "CF_CMD_RESET_COUNTERS",         "CF_HK",          "COMMAND_COUNTER" ])
        fsw_apps_to_test.append(["CS_CMD_NOOP",         "CS_CMD_RESET_COUNTERS",         "CS_HK",          "COMMAND_COUNTER" ])
        fsw_apps_to_test.append(["DS_CMD_NOOP",         "DS_CMD_RESET_COUNTERS",         "DS_HK",          "COMMAND_COUNTER" ])
        fsw_apps_to_test.append(["FM_CMD_NOOP",         "FM_CMD_RESET_COUNTERS",         "FM_HK",          "COMMAND_COUNTER" ])
        fsw_apps_to_test.append(["HK_CMD_NOOP",         "HK_CMD_RESET_COUNTERS",         "HK_HK",          "COMMAND_COUNTER" ])
        fsw_apps_to_test.append(["HS_CMD_NOOP",         "HS_CMD_RESET_COUNTERS",         "HS_HK",          "COMMAND_COUNTER" ])
        fsw_apps_to_test.append(["LC_CMD_NOOP",         "LC_CMD_RESET_COUNTERS",         "LC_HK",          "COMMAND_COUNTER" ])
        fsw_apps_to_test.append(["MD_CMD_NOOP",         "MD_CMD_RESET_COUNTERS",         "MD_HK",          "COMMAND_COUNTER" ])
        fsw_apps_to_test.append(["MM_CMD_NOOP",         "MM_CMD_RESET_COUNTERS",         "MM_HK",          "COMMAND_COUNTER" ])
        fsw_apps_to_test.append(["SC_CMD_NOOP",         "SC_CMD_RESET_COUNTERS",         "SC_HK",          "COMMAND_COUNTER" ])
        fsw_apps_to_test.append(["CI_LAB_CMD_NOOP",     "CI_LAB_CMD_RESET_COUNTERS",     "CI_LAB_HK",      "COMMAND_COUNTER" ])
        fsw_apps_to_test.append(["TO_LAB_CMD_NOOP",     "TO_LAB_CMD_RESET_COUNTERS",     "TO_LAB_HK",      "COMMAND_COUNTER" ])
        fsw_apps_to_test.append(["SAMPLE_APP_CMD_NOOP", "SAMPLE_APP_CMD_RESET_COUNTERS", "SAMPLE_APP_HK",  "COMMAND_COUNTER" ])
        fsw_apps_to_test.append(["SBN_CMD_NOOP",        "SBN_CMD_HK_RESET",              "SBN_HK",         "COMMAND_COUNTER" ])

        for fsw_target in fsw_targets_to_test:
            # For each app in the list, verify its alive by sending a noop, then reset the hk counters
            for noop_cmd_name, reset_counters_cmd_name, hk_packet_name, hk_cmd_count_name in fsw_apps_to_test:
                # Verify that we have a recent packet (by waiting for a new one to arrive)
                wait_check_packet(f"{fsw_target}", f"{hk_packet_name}", 1, 240)
                # Assuming no one else is sending commands, grab the latest command count
                cmd_count = tlm(f"{fsw_target} {hk_packet_name} {hk_cmd_count_name}")
                # Check accepted NOOP command proving application is up and running
                Group.print(f"Sending {fsw_target} the {noop_cmd_name} command.")
                cmd(f"{fsw_target} {noop_cmd_name}")
                wait_check(f"{fsw_target} {hk_packet_name} {hk_cmd_count_name} == {cmd_count+1}", 240)
                # Check accepted Reset Counters command
                Group.print(f"Sending {fsw_target} the {reset_counters_cmd_name} command.")
                cmd(f"{fsw_target} {reset_counters_cmd_name}")
                wait_check(f"{fsw_target} {hk_packet_name} {hk_cmd_count_name} == 0", 240)



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
