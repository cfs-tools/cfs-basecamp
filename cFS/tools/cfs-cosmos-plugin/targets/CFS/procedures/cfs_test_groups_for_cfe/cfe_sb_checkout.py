from openc3.script.suite import Group

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfe_sb_checkout(Group):
    """
    - Methods beginning with script_ or test_ are added to Script dropdown
    """
    def test_cfe_sb_checkout(self):
        """
        CFE SB Checkout Test
        -Send all SB commands as quickly as possible, then verify all were accepted
        """

        set_line_delay(0.0)

        Group.print(f"CFE_SB checkout test on <%= target_name %>")

        # Save initial command count
        cmd_count = tlm(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER")
        cmd_err_count = tlm(f"<%= target_name %> CFE_SB_HK COMMAND_ERROR_COUNTER")

        set_line_delay(0.15)  # 0.15 is about as fast as it can go.  0.1 causes a missed cmd.
        
        cmd(f"<%= target_name %> CFE_SB_CMD_NOOP")
        cmd(f"<%= target_name %> CFE_SB_CMD_SEND_SB_STATS")
        wait(0.5) # Without these waits, some commands fail
        cmd(f"<%= target_name %> CFE_SB_CMD_WRITE_ROUTING_INFO with FILENAME '/cf/func_file_route.dat'")
        wait(0.5) # Without these waits, some commands fail
        cmd(f"<%= target_name %> CFE_SB_CMD_WRITE_PIPE_INFO with FILENAME '/cf/func_file_pipe.dat'")
        wait(0.5) # Without these waits, some commands fail
        cmd(f"<%= target_name %> CFE_SB_CMD_WRITE_MAP_INFO with FILENAME '/cf/func_file_map.dat'")
        input_cmd_msg_id = <%= get_cfs_pkt_msg_id('CFE_EVS_CMD_NOOP', cfs_cpu_num_from_target_name(target_name)) %>
        pipe_id = tlm(f"<%= target_name %> CFE_SB_STATS PIPE_DEPTH_STATS_1_PIPE_ID")
        cmd(f"<%= target_name %> CFE_SB_CMD_DISABLE_ROUTE with PIPE {pipe_id}, MSG_ID_VALUE {input_cmd_msg_id}")
        cmd(f"<%= target_name %> CFE_SB_CMD_ENABLE_ROUTE with PIPE {pipe_id}, MSG_ID_VALUE {input_cmd_msg_id}")

        set_line_delay(0.0)
        
        # Check final command count has incremented by the number of commands sent
        wait_check(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER >= {cmd_count} + 7", 12)
        check(f"<%= target_name %> CFE_SB_HK COMMAND_ERROR_COUNTER == {cmd_err_count}")

        cmd(f"<%= target_name %> CFE_SB_CMD_RESET_COUNTERS")
        wait_check(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER == 0", 12)


    def setup(self):
        """
        Test Group Setup
        - Runs when Group Setup button is pressed
        - Runs before all scripts when Group Start is pressed
        """
        pass

    def teardown(self):
        """
        Test Group Teardown
        - Runs when Group Teardown button is pressed
        - Runs after all scripts when Group Start is pressed
        """
        pass
