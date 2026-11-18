from openc3.script.suite import Group
# Verify TIME commands work properly.  Not testing error cases.

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfe_time(Group):
    """
    - Methods beginning with script_ or test_ are added to Script dropdown
    """
    
    def test_00_Aliveness(self):
        """
        FSW Aliveness Test
        - Send a no-op command
            then verify the command was received (by checking the command counter incremented)
        - Reset the command counter
            then verify the command was received (by checking the command counter was cleared)
        """
        
        Group.print("Testing CFE_TIME aliveness on <%= target_name %>")
        
        wait_check_packet("<%= target_name %>", "CFE_TIME_HK", 1, 100)
        
        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm("<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")
        
        # Send NOOP command, then check result to prove application is up and running
        cmd("<%= target_name %> CFE_TIME_CMD_NOOP")
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        
        # Send Reset Counters command, check resullt
        cmd("<%= target_name %> CFE_TIME_CMD_RESET_COUNTERS")
        wait_check("<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == 0", 100)
    
    
    def test_01_NoOp(self):
        """
        Test the no-op command.
        """
        
        cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")
        
        cmd("<%= target_name %> CFE_TIME_CMD_NOOP")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
    
    
    def test_02_SendDiagnosticTlm(self):
        """
        Test the SendDiagnosticTlm command.
        """
        # get the latest command count
        wait_check_packet(f"<%= target_name %>", "CFE_TIME_HK", 1, 100)
        cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")

        # Get current number of CFE_TIME_DIAG packets received by GSW
        prev_time_diag_pkt_rcvd_count = tlm(f"<%= target_name %> CFE_TIME_DIAG RECEIVED_COUNT")
        if prev_time_diag_pkt_rcvd_count is None:
          prev_time_diag_pkt_rcvd_count = 0

        # Send command to request a new packet, then wait for the packet
        # due to some telemetry drops, we may have to send this multiple times
        packet_not_received = True
        number_of_requests_sent = 0
        while packet_not_received:
            number_of_requests_sent += 1
            cmd(f"<%= target_name %> CFE_TIME_CMD_SEND_DIAGNOSTIC")
            wait(8)
            current_time_diag_pkt_rcvd_count = tlm(f"<%= target_name %> CFE_TIME_DIAG RECEIVED_COUNT")
            if current_time_diag_pkt_rcvd_count > prev_time_diag_pkt_rcvd_count:
                packet_not_received = False

        # Verify command count incremented by the number of requests sent
        # Note: while we may expect telemetry drops, we don't expect command drops
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count + number_of_requests_sent}", 100)


    def test_03_SetStateCmd_Invalid(self):
        """
        Test the SetStateCmd command specifying INVALID mode.
        """

        cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_TIME_CMD_SET_STATE with CLOCK_STATE 'INVALID'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Verify correct clock state is set
        wait_check(f"<%= target_name %> CFE_TIME_HK CLOCK_STATE_API == 'INVALID'", 100)


    def test_04_SetStateCmd_Valid(self):
        """
        Test the SetStateCmd command specifying VALID mode.
        """
        
        cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_TIME_CMD_SET_STATE with CLOCK_STATE 'VALID'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Verify correct clock state is set
        wait_check(f"<%= target_name %> CFE_TIME_HK CLOCK_STATE_API == 'VALID'", 100)


    def test_05_SetStateCmd_Flywheel(self):
        """
        Test the SetStateCmd command specifying FLYWHEEL mode.
        """
        
        cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_TIME_CMD_SET_STATE with CLOCK_STATE 'FLYWHEEL'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Verify correct clock state is set
        wait_check(f"<%= target_name %> CFE_TIME_HK CLOCK_STATE_API == 'FLYWHEEL'", 100)


    def disabled_test_06_SetSourceCmd_Internal(self):
        """
        WARNING: Disabled for now, because our reference cFS bundle has
                CFE_PLATFORM_TIME_CFG_SOURCE set FALSE
        Test the SetSourceCmd command specifying INTERNAL source.
        """
        
        cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_TIME_CMD_SET_SOURCE with TIME_SOURCE 'INTERNAL'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Verify correct clock source is set
        # Request a new diags packet to check this telemetry
        cmd(f"<%= target_name %> CFE_TIME_CMD_SEND_DIAGNOSTIC")
        wait_check_packet(f"<%= target_name %>", "CFE_TIME_DIAG", 1, 100)
        wait_check(f"<%= target_name %> CFE_TIME_DIAG CLOCK_SOURCE == 'INTERNAL'", 100)


    def disabled_test_07_SetSourceCmd_External(self):
        """
        WARNING: Disabled for now, because our reference cFS bundle has
                CFE_PLATFORM_TIME_CFG_SOURCE set FALSE
        Test the SetSourceCmd command specifying EXTERNAL source.
        """
        
        cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_TIME_CMD_SET_SOURCE with TIME_SOURCE 'EXTERNAL'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Verify correct clock source is set
        # Request a new diags packet to check this telemetry
        cmd(f"<%= target_name %> CFE_TIME_CMD_SEND_DIAGNOSTIC")
        wait_check_packet(f"<%= target_name %>", "CFE_TIME_DIAG", 1, 100)
        wait_check(f"<%= target_name %> CFE_TIME_DIAG CLOCK_SOURCE == 'EXTERNAL'", 100)


    def disabled_test_08_SetSignalCmd_Primary(self):
        """
        WARNING: Disabled for now, because our reference cFS bundle has
                CFE_PLATFORM_TIME_CFG_SIGNAL set FALSE
        Test the SetSignalCmd command specifying PRIMARY source.
        """
        
        cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_TIME_CMD_SET_SIGNAL with TONE_SOURCE 'TONE_PRI'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Verify correct tone source is set
        # Request a new diags packet to check this telemetry
        cmd(f"<%= target_name %> CFE_TIME_CMD_SEND_DIAGNOSTIC")
        wait_check_packet(f"<%= target_name %>", "CFE_TIME_DIAG", 1, 100)
        wait_check(f"<%= target_name %> CFE_TIME_DIAG CLOCK_SIGNAL == 'PRIMARY'", 100)


    def disabled_test_09_SetSignalCmd_Redundant(self):
        """
        WARNING: Disabled for now, because our reference cFS bundle has
                CFE_PLATFORM_TIME_CFG_SIGNAL set FALSE
        Test the SetSignalCmd command specifying REDUNDANT source.
        """
        
        cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_TIME_CMD_SET_SIGNAL with TONE_SOURCE 'TONE_RED'")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Verify correct tone source is set
        # Request a new diags packet to check this telemetry
        cmd(f"<%= target_name %> CFE_TIME_CMD_SEND_DIAGNOSTIC")
        wait_check_packet(f"<%= target_name %>", "CFE_TIME_DIAG", 1, 100)
        wait_check(f"<%= target_name %> CFE_TIME_DIAG CLOCK_SIGNAL == 'REDUNDANT'", 100)


    def disabled_test_10_AddDelayCmd(self):
        """
        WARNING: Disabled for now, because our reference cFS bundle has
                CFE_PLATFORM_TIME_CFG_CLIENT set FALSE
        Test the AddDelayCmd command.
        """
        
        cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_TIME_CMD_ADD_DELAY with SECONDS 1, MICROSECONDS 2")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Verify correct delay is set
        wait_check(f"<%= target_name %> CFE_TIME_HK SECONDS_DELAY == 1", 100)
        wait_check(f"<%= target_name %> CFE_TIME_HK SUBSECONDS_DELAY == 2", 100)
       


    def disabled_test_11_SubDelayCmd(self):
        """
        WARNING: Disabled for now, because our reference cFS bundle has
                CFE_PLATFORM_TIME_CFG_CLIENT set FALSE
        Test the SubDelayCmd command.
        Note: This test assumes the test for AddDelay was run previously.
        """
        
        cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_TIME_CMD_SUB_DELAY with SECONDS 1, MICROSECONDS 2")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Verify correct delay is set
        wait_check(f"<%= target_name %> CFE_TIME_HK SECONDS_DELAY == 0", 100)
        wait_check(f"<%= target_name %> CFE_TIME_HK SUBSECONDS_DELAY == 0", 100)


    def test_12_SetTimeCmd(self):
        """
        Test the SetTimeCmd command.
        """
        
        cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_TIME_CMD_SET_TIME with SECONDS 3, MICROSECONDS 4")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count + 1}", 100)


    def test_13_SetMETCmd(self):
        """
        Test the SetMETCmd command.
        """
        
        cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_TIME_CMD_SET_MET with SECONDS 4, MICROSECONDS 5")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Notes seconds / subseconds changes too quickly to be checked
        # wait_check(f"<%= target_name %> CFE_TIME_HK SECONDS_MET >= 4", 100)


    def test_14_SetSTCFCmd(self):
        """
        Test the SetSTCFCmd command.
        """
        
        cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_TIME_CMD_SET_STCF with SECONDS 5, MICROSECONDS 6")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Verify correct values are set
        # Note: microseconds converted to (and reported as) subseconds in FSW
        wait_check(f"<%= target_name %> CFE_TIME_HK STCF_SECONDS == 5", 100)
        wait_check(f"<%= target_name %> CFE_TIME_HK STCF_SUBSECONDS == 25770", 100)


    def test_15_SetLeapSecondsCmd(self):
        """
        Test the SetLeapSecondsCmd command.
        """
        
        cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_TIME_CMD_SET_LEAP_SECONDS with LEAP_SECONDS 99")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Verify correct value of leap seconds is set
        wait_check(f"<%= target_name %> CFE_TIME_HK LEAP_SECONDS == 99", 100)


    def test_16_AddAdjustCmd(self):
        """
        Test the AddAdjustCmd command.
        """
        
        cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_TIME_CMD_ADD_ADJUST with SECONDS 1, MICROSECONDS 2")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count + 1}", 100)


    def test_17_SubAdjustCmd(self):
        """
        Test the SubAdjustCmd command.
        """
        
        cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_TIME_CMD_SUB_ADJUST with SECONDS 1, MICROSECONDS 2")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count + 1}", 100)


    def test_18_AddOneHzAdjustmentCmd(self):
        """
        Test the AddOneHzAdjustmentCmd command.
        """
        
        cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_TIME_CMD_ADD_ONE_HZ_ADJUSTMENT with SECONDS 1, SUBSECONDS 2")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Get current number of CFE_TIME_DIAG packets received by GSW
        prev_time_diag_pkt_rcvd_count = tlm(f"<%= target_name %> CFE_TIME_DIAG RECEIVED_COUNT")
        if prev_time_diag_pkt_rcvd_count is None:
          prev_time_diag_pkt_rcvd_count = 0

        # Request a new diags packet to check this telemetry
        # due to some telemetry drops, we may have to send this multiple times
        packet_not_received = True
        while packet_not_received:
            cmd(f"<%= target_name %> CFE_TIME_CMD_SEND_DIAGNOSTIC")
            wait(8)
            current_time_diag_pkt_rcvd_count = tlm(f"<%= target_name %> CFE_TIME_DIAG RECEIVED_COUNT")
            if current_time_diag_pkt_rcvd_count > prev_time_diag_pkt_rcvd_count:
                packet_not_received = False

        # Verify correct values are set
        wait_check(f"<%= target_name %> CFE_TIME_DIAG ONE_HZ_DIRECTION == 'ADD'", 100)
        wait_check(f"<%= target_name %> CFE_TIME_DIAG ONE_HZ_ADJUST_SECONDS == 1", 100)
        wait_check(f"<%= target_name %> CFE_TIME_DIAG ONE_HZ_ADJUST_SUBSECONDS == 2", 100)


    def test_19_SubOneHzAdjustmentCmd(self):
        """
        Test the SubOneHzAdjustmentCmd command.  Note: This test assumes the test for AddOneHzAdjustment was run previously.
        """
        
        cmd_count = tlm(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER")
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_TIME_CMD_SUB_ONE_HZ_ADJUSTMENT with SECONDS 3, SUBSECONDS 4")
        
        # Verify command count incremented
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Get current number of CFE_TIME_DIAG packets received by GSW
        prev_time_diag_pkt_rcvd_count = tlm(f"<%= target_name %> CFE_TIME_DIAG RECEIVED_COUNT")
        if prev_time_diag_pkt_rcvd_count is None:
          prev_time_diag_pkt_rcvd_count = 0

        # Request a new diags packet to check this telemetry
        # due to some telemetry drops, we may have to send this multiple times
        packet_not_received = True
        while packet_not_received:
            cmd(f"<%= target_name %> CFE_TIME_CMD_SEND_DIAGNOSTIC")
            wait(8)
            current_time_diag_pkt_rcvd_count = tlm(f"<%= target_name %> CFE_TIME_DIAG RECEIVED_COUNT")
            if current_time_diag_pkt_rcvd_count > prev_time_diag_pkt_rcvd_count:
                packet_not_received = False

        # Verify correct values are set
        wait_check(f"<%= target_name %> CFE_TIME_DIAG ONE_HZ_DIRECTION == 'SUBTRACT'", 100)
        wait_check(f"<%= target_name %> CFE_TIME_DIAG ONE_HZ_ADJUST_SECONDS == 3", 100)
        wait_check(f"<%= target_name %> CFE_TIME_DIAG ONE_HZ_ADJUST_SUBSECONDS == 4", 100)


    def test_16_ResetCounters(self):
        """
        Test the ResetCounters command.
        """
        # Send an ADD_DELAY command, which should cause an error because
        # the CFE_PLATFORM_TIME_CFG_CLIENT config is set FALSE (currently)
        cmd(f"<%= target_name %> CFE_TIME_CMD_ADD_DELAY with SECONDS 1, MICROSECONDS 2")

        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_ERROR_COUNTER > 0", 100)
        
        # Send command under test
        cmd(f"<%= target_name %> CFE_TIME_CMD_RESET_COUNTERS")
        
        # Verify counters are reset to zero
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_COUNTER == 0", 100)
        wait_check(f"<%= target_name %> CFE_TIME_HK COMMAND_ERROR_COUNTER == 0", 100)

    def setup(self):
        """
        Test Group Setup
        - Runs when Group Setup button is pressed
        - Runs before all scripts when Group Start is pressed
        """
        # Wait for a new housekeeping packet, to ensure we're using its latest status info
        wait_check_packet(f"<%= target_name %>", "CFE_TIME_HK", 1, 100)

        # Ensure that TIME events are enabled
        cmd("<%= target_name %> CFE_EVS_CMD_ENABLE_APP_EVENTS with APP_NAME 'CFE_TIME'")
        wait(1)
        
        # Ensure that DEBUG and INFO events are enabled
        cmd("<%= target_name %> CFE_EVS_CMD_ENABLE_EVENT_TYPE with BIT_MASK DEBUG")
        cmd("<%= target_name %> CFE_EVS_CMD_ENABLE_EVENT_TYPE with BIT_MASK INFO")
        wait(1)
        pass

    def teardown(self):
        """
        Test Group Setup
        - Runs when Group Teardown button is pressed
        - Runs after all scripts when Group Start is pressed
        """
        pass
