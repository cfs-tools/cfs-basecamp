from openc3.script.suite import Group

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfe_sb(Group):
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
        Group.print(f"Testing CFE_SB aliveness on <%= target_name %>")

        # Verify that we have a recent HK packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_SB_HK", 1, 20)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER")

        # Check accepted NOOP command proving application is up and running
        cmd(f"<%= target_name %> CFE_SB_CMD_NOOP")
        wait_check(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER == {cmd_count + 1}", 20)

        # Check accepted Reset Counters command
        cmd(f"<%= target_name %> CFE_SB_CMD_RESET_COUNTERS")
        wait_check(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER == 0", 20)
        
    def test_01_sb_cmd_sb_stats(self):
        """
        FSW Send SB Stats Cmd Test
        - Send a CFE_SB_CMD_SEND_SB_STATS 
            then verify the command was received (by checking the command counter incremented)
            then verify the CFE_SB_STATS packet was received (by checking the received counter)
        """
        Group.print(f"Testing CFE_SB's CFE_SB_CMD_SEND_SB_STATS command functionality on <%= target_name %>")

        # Get current packet count of CFE_SB_STATS
        curr_sb_stats_pkt_count = tlm(f"<%= target_name %> CFE_SB_STATS RECEIVED_COUNT")
        if curr_sb_stats_pkt_count is None:
          curr_sb_stats_pkt_count = 0

        # Verify that we have a recent HK packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_SB_HK", 1, 20)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER")

        # Check accepted CFE_SB_CMD_SEND_SB_STATS command
        cmd(f"<%= target_name %> CFE_SB_CMD_SEND_SB_STATS")

        # Verify that the command counter increments from the CFE_SB_CMD_SEND_SB_STATS command
        wait_check(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER == {cmd_count + 1}", 20)

        # Verify that we have a recent CFE_SB_STATS packet
        wait_check(f"<%= target_name %> CFE_SB_STATS RECEIVED_COUNT >= {curr_sb_stats_pkt_count + 1}", 20)

    def test_02_sb_cmd_write_routing_info(self):
        """
        FSW Write Routing Info Cmd Test
        - Send a CFE_SB_CMD_WRITE_ROUTING_INFO which will write a file /cf/func_file_route.dat
            then verify the command was received (by checking the command counter incremented)
        - Send a FM_CMD_GET_FILE_INFO for /cf/func_file_route.dat
            then verify that we receive a FM_FILE_INFO pkt which has the correct file name and a status as closed
        """      
        Group.print(f"Testing CFE_SB's CFE_SB_CMD_WRITE_ROUTING_INFO command functionality on <%= target_name %>")

        # Delete /cf/func_file_route.dat before attempting CFE_SB_CMD_WRITE_ROUTING_INFO command
        cmd(f"<%= target_name %> FM_CMD_DELETE_FILE with PATH '/cf/func_file_route.dat'")

        # Verify that we have a recent HK packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_SB_HK", 1, 20)
        
        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER")

        # Check accepted CFE_SB_CMD_WRITE_ROUTING_INFO command
        cmd(f"<%= target_name %> CFE_SB_CMD_WRITE_ROUTING_INFO with FILENAME '/cf/func_file_route.dat'")

        # Verify the command was accepted by incrementing HK command counter
        wait_check(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER == {cmd_count + 1}", 20)

        # Get current packet count of FM_FILE_INFO
        curr_fm_file_info_pkt_count = tlm(f"<%= target_name %> FM_FILE_INFO RECEIVED_COUNT")
        if curr_fm_file_info_pkt_count is None:
          curr_fm_file_info_pkt_count = 0

        # Send FM cmd to see if file created
        cmd(f"<%= target_name %> FM_CMD_GET_FILE_INFO with CRC_METHOD 'CRC_NONE', PATH '/cf/func_file_route.dat'")

        # Verify that we have a recent FM_FILE_INFO packet (by waiting for a new one to arrive)
        wait_check(f"<%= target_name %> FM_FILE_INFO RECEIVED_COUNT >= {curr_fm_file_info_pkt_count + 1}", 20)

        # Verify the file was created
        wait_check(f"<%= target_name %> FM_FILE_INFO NAME == '/cf/func_file_route.dat'", 20)  
        wait_check(f"<%= target_name %> FM_FILE_INFO STATUS == 'CLOSED_FILE'", 20)


    def test_03_sb_cmd_write_pipe_info(self):
        """
        FSW Write Pipe Info Cmd Test
        - Send a CFE_SB_CMD_WRITE_PIPE_INFO which will write a file /cf/func_file_pipe.dat
          then verify the command was received (by checking the command counter incremented)
        - Send a FM_CMD_GET_FILE_INFO for /cf/func_file_pipe.dat
          then verify that we receive a FM_FILE_INFO pkt which has the correct file name and a status as closed
        """
        Group.print(f"Testing CFE_SB's CFE_SB_CMD_WRITE_PIPE_INFO command functionality on <%= target_name %>")

        # Delete /cf/func_file_pipe.dat before attempting CFE_SB_CMD_WRITE_PIPE_INFO command
        cmd(f"<%= target_name %> FM_CMD_DELETE_FILE with PATH '/cf/func_file_pipe.dat'")

        # Verify that we have a recent HK packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_SB_HK", 1, 20)
        
        # Assuming no one else is sending commands, grab the latest hk command count
        cmd_count = tlm(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER")

        # Check accepted CFE_SB_CMD_WRITE_PIPE_INFO command
        cmd(f"<%= target_name %> CFE_SB_CMD_WRITE_PIPE_INFO with FILENAME '/cf/func_file_pipe.dat'")

        # Wait for a new HK packet
        wait_check(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER == {cmd_count + 1}", 20)

        # Get current packet count of FM_FILE_INFO
        curr_fm_file_info_pkt_count = tlm(f"<%= target_name %> FM_FILE_INFO RECEIVED_COUNT")
        if curr_fm_file_info_pkt_count is None:
          curr_fm_file_info_pkt_count = 0

        # Send FM cmd to see if file created
        cmd(f"<%= target_name %> FM_CMD_GET_FILE_INFO with CRC_METHOD 'CRC_NONE', PATH '/cf/func_file_pipe.dat'")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check(f"<%= target_name %> FM_FILE_INFO RECEIVED_COUNT >= {curr_fm_file_info_pkt_count + 1}", 20)

        # Verify the file was created
        wait_check(f"<%= target_name %> FM_FILE_INFO NAME == '/cf/func_file_pipe.dat'", 20)  
        wait_check(f"<%= target_name %> FM_FILE_INFO STATUS == 'CLOSED_FILE'", 20)  


    def test_04_sb_cmd_write_map_info(self):
        """
        FSW Write Map Info Cmd Test
        - Send a CFE_SB_CMD_WRITE_MAP_INFO which will write a file /cf/func_file_map.dat
          then verify the command was received (by checking the command counter incremented)
        - Send a FM_CMD_GET_FILE_INFO for /cf/func_file_map.dat
          then verify that we receive a FM_FILE_INFO pkt which has the correct file name and a status as closed
        """
        Group.print(f"Testing CFE_SB's CFE_SB_CMD_WRITE_MAP_INFO command functionality on <%= target_name %>")

        # Delete /cf/func_file_map.dat before attempting CFE_SB_CMD_WRITE_MAP_INFO command
        cmd(f"<%= target_name %> FM_CMD_DELETE_FILE with PATH '/cf/func_file_map.dat'")

        # Verify that we have a recent HK packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_SB_HK", 1, 20)
        
        # Assuming no one else is sending commands, grab the latest hk command count
        cmd_count = tlm(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER")

        # Check accepted CFE_SB_CMD_WRITE_MAP_INFO command
        cmd(f"<%= target_name %> CFE_SB_CMD_WRITE_MAP_INFO with FILENAME '/cf/func_file_map.dat'")

        # Wait for a new HK packet
        wait_check(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER == {cmd_count + 1}", 20)

        # Get current packet count of FM_FILE_INFO
        curr_fm_file_info_pkt_count = tlm(f"<%= target_name %> FM_FILE_INFO RECEIVED_COUNT")
        if curr_fm_file_info_pkt_count is None:
          curr_fm_file_info_pkt_count = 0

        # Send FM cmd to see if file created
        cmd(f"<%= target_name %> FM_CMD_GET_FILE_INFO with CRC_METHOD 'CRC_NONE', PATH '/cf/func_file_map.dat'")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        wait_check(f"<%= target_name %> FM_FILE_INFO RECEIVED_COUNT >= {curr_fm_file_info_pkt_count + 1}", 20)

        # Verify the file name
        wait_check(f"<%= target_name %> FM_FILE_INFO NAME == '/cf/func_file_map.dat'", 20)  
        wait_check(f"<%= target_name %> FM_FILE_INFO STATUS == 'CLOSED_FILE'", 20)  

    def test_05_sb_cmd_enable_disable_route(self):
        """
        FSW Disable Route Command Test
        - Send a CFE_SB_CMD_DISABLE_ROUTE for the first registered Pipe ID with an CFE_ES MsgId
          then verify the command was received (by checking the command counter incremented)
        - Send a CFE_SB_CMD_DISABLE_ROUTE for a different Pipe ID with an CFE_ES MsgId
          then verify the command was rejected (by checking the error command counter incremented)
        """
        # The Enable/Disable Route commands require a Command MID input
        # for this test, we'll use one of the other existing cFE task's command MID
        input_cmd_msg_id = <%= get_cfs_pkt_msg_id('CFE_EVS_CMD_NOOP', cfs_cpu_num_from_target_name(target_name)) %>

        # Get current packet count of CFE_SB_STATS
        curr_sb_stats_pkt_count = tlm(f"<%= target_name %> CFE_SB_STATS RECEIVED_COUNT")
        if curr_sb_stats_pkt_count is None:
          curr_sb_stats_pkt_count = 0        

        # Verify that we have a recent HK packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_SB_HK", 1, 20)
        
        # Assuming no one else is sending commands, grab the latest HK command count
        cmd_count = tlm(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER")

        # Check accepted CFE_SB_CMD_SEND_SB_STATS command
        cmd(f"<%= target_name %> CFE_SB_CMD_SEND_SB_STATS")
        wait_check(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER == {cmd_count + 1}", 20)

        # Verify that we have a recent SB Stats packet (by waiting for a new one to arrive)
        wait_check(f"<%= target_name %> CFE_SB_STATS RECEIVED_COUNT >= {curr_sb_stats_pkt_count + 1}", 20)

        # Get the verify first SB Pipe ID registered
        pipe_id = tlm(f"<%= target_name %> CFE_SB_STATS PIPE_DEPTH_STATS_1_PIPE_ID")

        # Verify that we have a recent HK packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_SB_HK", 1, 20)
        
        # Assuming no one else is sending commands, grab the latest HK command count
        cmd_count = tlm(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER")

        # Check accepted VALID CFE_SB_CMD_DISABLE_ROUTE command
        cmd(f"<%= target_name %> CFE_SB_CMD_DISABLE_ROUTE with PIPE {pipe_id}, MSG_ID_VALUE {input_cmd_msg_id}")
        wait_check(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER == {cmd_count + 1}", 20)

        # Get the latest cmd/err counter values
        cmd_count = tlm(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER")
        err_count = tlm(f"<%= target_name %> CFE_SB_HK COMMAND_ERROR_COUNTER")

        # Check rejected INVALID CFE_SB_CMD_DISABLE_ROUTE command
        cmd(f"<%= target_name %> CFE_SB_CMD_DISABLE_ROUTE with PIPE {pipe_id + 1}, MSG_ID_VALUE {input_cmd_msg_id}")
        wait_check(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER == {cmd_count}", 20)
        wait_check(f"<%= target_name %> CFE_SB_HK COMMAND_ERROR_COUNTER == {err_count + 1}", 20)

        """
        FSW Enable Route Command Test
        - Send a CFE_SB_CMD_ENABLE_ROUTE for the first registered Pipe ID with an CFE_ES MsgId
          then verify the command was received (by checking the command counter incremented)
        - Send a CFE_SB_CMD_ENABLE_ROUTE for a different Pipe ID with an CFE_ES MsgId
          then verify the command was rejected (by checking the error command counter incremented)
        """
        # Get current packet count of CFE_SB_STATS
        curr_sb_stats_pkt_count = tlm(f"<%= target_name %> CFE_SB_STATS RECEIVED_COUNT")
        if curr_sb_stats_pkt_count is None:
          curr_sb_stats_pkt_count = 0        

        # Verify that we have a recent HK packet (by waiting for a new one to arrive)
        wait_check_packet(f"<%= target_name %>", f"CFE_SB_HK", 1, 20)
        
        # Assuming no one else is sending commands, grab the latest HK command count
        cmd_count = tlm(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER")

        # Check accepted VALID CFE_SB_CMD_ENABLE_ROUTE command
        cmd(f"<%= target_name %> CFE_SB_CMD_ENABLE_ROUTE with PIPE {pipe_id}, MSG_ID_VALUE {input_cmd_msg_id}")
        wait_check(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER == {cmd_count + 1}", 20)

        # Get the latest cmd/err counter values
        cmd_count = tlm(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER")
        err_count = tlm(f"<%= target_name %> CFE_SB_HK COMMAND_ERROR_COUNTER")

        # Check rejected INVALID CFE_SB_CMD_ENABLE_ROUTE command
        cmd(f"<%= target_name %> CFE_SB_CMD_ENABLE_ROUTE with PIPE {pipe_id + 1}, MSG_ID_VALUE {input_cmd_msg_id}")
        wait_check(f"<%= target_name %> CFE_SB_HK COMMAND_COUNTER == {cmd_count}", 20)
        wait_check(f"<%= target_name %> CFE_SB_HK COMMAND_ERROR_COUNTER == {err_count + 1}", 20)

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
