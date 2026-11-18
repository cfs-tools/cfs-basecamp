from openc3.script import *
from openc3.script.suite import Group

# CFDP Entity ID Configuration
# This must match the cFS CF application table configuration
CFDP_GROUND_ENTITY_ID = 4    # Ground/COSMOS entity ID

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfs_cf(Group):
    """
    - Methods beginning with script_ or test_ are added to Script dropdown
    """

    def _require_hk(self, app_name, timeout=100):
        Group.print(f"Waiting for {app_name} HK on <%= target_name %>")
        try:
            wait_check_packet(f"<%= target_name %>", f"{app_name}_HK", 1, timeout)
        except Exception:
            Group.print(f"Missing {app_name} HK telemetry; check TO_LAB output and ensure {app_name} is running")
            raise

    def test_aliveness(self):
        """
        FSW Aliveness Test
        - Send a no-op command
            then verify the command was received (by checking the command counter incremented)
        - Reset the command counter
            then verify the command was received (by checking the command counter was cleared)
        """
        Group.print("Testing CF aliveness on <%= target_name %>")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        self._require_hk("CF")

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = tlm("<%= target_name %> CF_HK COMMAND_COUNTER")

        # Check accepted NOOP command proving application is up and running
        cmd("<%= target_name %> CF_CMD_NOOP")
        wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count + 1}", 100)

        # Check accepted Reset Counters command
        cmd("<%= target_name %> CF_CMD_RESET_COUNTERS")
        wait_check("<%= target_name %> CF_HK COMMAND_COUNTER == 0", 100)

    def test_channel_management(self):
        """
        CF Channel Management Commands
        - Exercise freeze / thaw and queue + polling controls on channel 0
            then verify the command counter increments when commands succeed
            and verify frozen state toggles in housekeeping telemetry
        - Purge queues to confirm the queue management command path executes without error
        """
        channel = 0

        Group.print("Testing CF channel management commands on <%= target_name %>")

        #------------- Command 1: Freeze Channel -----------------------------------
        Group.print("Testing CF freeze on <%= target_name %>")
        self._require_hk("CF")
        cmd_count = tlm("<%= target_name %> CF_HK COMMAND_COUNTER")
        err_count = tlm("<%= target_name %> CF_HK COMMAND_ERROR_COUNTER")
        cmd(
            f"<%= target_name %> CF_CMD_FREEZE with CHANNEL_NUM {channel}"
        )
        wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CF_HK COMMAND_ERROR_COUNTER == {err_count}", 100)
        # HK publishes the enumerated state string, so compare to the string value instead of the numeric code
        wait_check(f"<%= target_name %> CF_HK CHANNEL_HK_{channel}_FROZEN == 'FROZEN'", 100)
        cmd_count += 1

        #------------- Command 2: Thaw Channel -------------------------------------
        Group.print("Testing CF thaw on <%= target_name %>")
        self._require_hk("CF")
        err_count = tlm("<%= target_name %> CF_HK COMMAND_ERROR_COUNTER")
        cmd(
            f"<%= target_name %> CF_CMD_THAW with CHANNEL_NUM {channel}"
        )
        wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CF_HK COMMAND_ERROR_COUNTER == {err_count}", 100)
        wait_check(f"<%= target_name %> CF_HK CHANNEL_HK_{channel}_FROZEN == 'THAWED'", 100)
        cmd_count += 1

        #------------- Command 3: Disable Dequeue ----------------------------------
        Group.print("Testing CF disable_dequeue on <%= target_name %>")
        self._require_hk("CF")
        err_count = tlm("<%= target_name %> CF_HK COMMAND_ERROR_COUNTER")
        cmd(
            f"<%= target_name %> CF_CMD_DISABLE_DEQUEUE with CHANNEL_NUM {channel}"
        )
        wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CF_HK COMMAND_ERROR_COUNTER == {err_count}", 100)
        cmd_count += 1

        #------------- Command 4: Enable Dequeue -----------------------------------
        Group.print("Testing CF enable_dequeue on <%= target_name %>")
        self._require_hk("CF")
        err_count = tlm("<%= target_name %> CF_HK COMMAND_ERROR_COUNTER")
        cmd(
            f"<%= target_name %> CF_CMD_ENABLE_DEQUEUE with CHANNEL_NUM {channel}"
        )
        wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CF_HK COMMAND_ERROR_COUNTER == {err_count}", 100)
        cmd_count += 1

        #------------- Command 5: Enable Directory Polling -------------------------
        Group.print("Testing CF enable_dir_polling on <%= target_name %>")
        self._require_hk("CF")
        err_count = tlm("<%= target_name %> CF_HK COMMAND_ERROR_COUNTER")
        cmd(
            f"<%= target_name %> CF_CMD_ENABLE_DIR_POLLING with CHANNEL_NUM {channel}, POLL_DIR 255"
        )
        wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CF_HK COMMAND_ERROR_COUNTER == {err_count}", 100)
        cmd_count += 1

        #------------- Command 6: Disable Directory Polling ------------------------
        Group.print("Testing CF disable_dir_polling on <%= target_name %>")
        self._require_hk("CF")
        err_count = tlm("<%= target_name %> CF_HK COMMAND_ERROR_COUNTER")
        cmd(
            f"<%= target_name %> CF_CMD_DISABLE_DIR_POLLING with CHANNEL_NUM {channel}, POLL_DIR 255"
        )
        wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CF_HK COMMAND_ERROR_COUNTER == {err_count}", 100)
        cmd_count += 1

        #------------- Command 7: Purge Queue --------------------------------------
        Group.print("Testing CF purge_queue on <%= target_name %>")
        self._require_hk("CF")
        err_count = tlm("<%= target_name %> CF_HK COMMAND_ERROR_COUNTER")
        cmd(
            f"<%= target_name %> CF_CMD_PURGE_QUEUE with CHANNEL_NUM {channel}, QUEUE_TYPE 2"
        )
        wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CF_HK COMMAND_ERROR_COUNTER == {err_count}", 100)

    def test_transaction_controls(self):
        """
        CF Transaction Control Commands
        - Send suspend/resume/cancel/abandon with no active transactions
            then verify the error counter increments to reflect the guarded failure path
        """
        channel = 0

        Group.print("Testing CF transaction control commands on <%= target_name %>")

        self._require_hk("CF")
        cmd_count = tlm("<%= target_name %> CF_HK COMMAND_COUNTER")
        err_count = tlm("<%= target_name %> CF_HK COMMAND_ERROR_COUNTER")

        #------------- Command 1: Suspend -----------------------------------------
        Group.print("Testing CF suspend transaction handling on <%= target_name %>")
        cmd(
            f"<%= target_name %> CF_CMD_SUSPEND with TRANSACTION_SEQ_NUM 1, EID 23, CHAN {channel}"
        )
        wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count}", 100)
        wait_check(f"<%= target_name %> CF_HK COMMAND_ERROR_COUNTER == {err_count + 1}", 100)
        err_count += 1

        #------------- Command 2: Resume ------------------------------------------
        Group.print("Testing CF resume transaction handling on <%= target_name %>")
        cmd(
            f"<%= target_name %> CF_CMD_RESUME with TRANSACTION_SEQ_NUM 1, EID 23, CHAN {channel}"
        )
        wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count}", 100)
        wait_check(f"<%= target_name %> CF_HK COMMAND_ERROR_COUNTER == {err_count + 1}", 100)
        err_count += 1

        #------------- Command 3: Cancel ------------------------------------------
        Group.print("Testing CF cancel transaction handling on <%= target_name %>")
        cmd(
            f"<%= target_name %> CF_CMD_CANCEL with TRANSACTION_SEQ_NUM 1, EID 23, CHAN {channel}"
        )
        wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count}", 100)
        wait_check(f"<%= target_name %> CF_HK COMMAND_ERROR_COUNTER == {err_count + 1}", 100)
        err_count += 1

        #------------- Command 4: Abandon -----------------------------------------
        Group.print("Testing CF abandon transaction handling on <%= target_name %>")
        cmd(
            f"<%= target_name %> CF_CMD_ABANDON with TRANSACTION_SEQ_NUM 1, EID 23, CHAN {channel}"
        )
        wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count}", 100)
        wait_check(f"<%= target_name %> CF_HK COMMAND_ERROR_COUNTER == {err_count + 1}", 100)

    def test_file_and_config_commands(self):
        """
        CF File/Configuration Commands
        - Attempt file transfer and playback with missing files/directories
            then verify the error counter increments (expected negative test)
        - Exercise write queue with an invalid type/queue combination to confirm guarded path
        - Set and get a configuration parameter
            then verify the command counter increments for successful operations
        """
        channel = 0

        Group.print("Testing CF file/configuration commands on <%= target_name %>")
        self._require_hk("CF")
        cmd_count = tlm("<%= target_name %> CF_HK COMMAND_COUNTER")
        err_count = tlm("<%= target_name %> CF_HK COMMAND_ERROR_COUNTER")

        #------------- Command 1: TX File (expected error) -------------------------
        Group.print("Testing CF tx_file error handling on <%= target_name %>")
        cmd(
            f"<%= target_name %> CF_CMD_TX_FILE with CFDP_CLASS 0, KEEP_FILE_FLAG 1, CHAN_NUM {channel}, PRIORITY 1, DEST_ID {CFDP_GROUND_ENTITY_ID}, SRC_FILENAME '/cf/missing_src', DEST_FILENAME '/ram/missing_dst'"
        )
        try:
            wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count + 1}", 10)
            err_expected = err_count
        except Exception:
            # Some CF builds accept the command and fail later in the transaction; handle either counter path.
            wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count}", 10)
            err_expected = err_count + 1
        wait_check(f"<%= target_name %> CF_HK COMMAND_ERROR_COUNTER == {err_expected}", 10)

        #------------- Command 2: Playback Dir (expected error) --------------------
        self._require_hk("CF")
        cmd_count = tlm("<%= target_name %> CF_HK COMMAND_COUNTER")
        err_count = tlm("<%= target_name %> CF_HK COMMAND_ERROR_COUNTER")

        Group.print("Testing CF playback_dir error handling on <%= target_name %>")
        cmd(
            f"<%= target_name %> CF_CMD_PLAYBACK_DIR with CFDP_CLASS 0, KEEP 1, CHAN_NUM {channel}, PRIORITY 1, DEST_ID {CFDP_GROUND_ENTITY_ID}, SRC_FILENAME '/cf/missing_dir', DST_FILENAME '/ram/missing_dir'"
        )
        try:
            wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count + 1}", 10)
            err_expected = err_count
        except Exception:
            # Some CF builds accept the command and fail later in the transaction; handle either counter path.
            wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count}", 10)
            err_expected = err_count + 1
        wait_check(f"<%= target_name %> CF_HK COMMAND_ERROR_COUNTER == {err_expected}", 10)

        #------------- Command 3: Write Queue (expected error) ---------------------
        self._require_hk("CF")
        cmd_count = tlm("<%= target_name %> CF_HK COMMAND_COUNTER")
        err_count = tlm("<%= target_name %> CF_HK COMMAND_ERROR_COUNTER")

        Group.print("Testing CF write_queue error handling on <%= target_name %>")
        cmd(
            f"<%= target_name %> CF_CMD_WRITE_QUEUE with TYPE 1, CHAN {channel}, QUEUE 0, FILENAME '/cf/tmp/cf_queue_error.txt'"
        )
        wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count}", 100)
        wait_check(f"<%= target_name %> CF_HK COMMAND_ERROR_COUNTER == {err_count + 1}", 100)

        #------------- Command 4: Set Param ---------------------------------------
        self._require_hk("CF")
        cmd_count = tlm("<%= target_name %> CF_HK COMMAND_COUNTER")
        err_count = tlm("<%= target_name %> CF_HK COMMAND_ERROR_COUNTER")

        Group.print("Testing CF set_param on <%= target_name %>")
        cmd(
            f"<%= target_name %> CF_CMD_SET_PARAM with VALUE 4, KEY 6, CHAN_NUM {channel}"
        )
        wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CF_HK COMMAND_ERROR_COUNTER == {err_count}", 100)

        #------------- Command 5: Get Param ---------------------------------------
        self._require_hk("CF")
        cmd_count = tlm("<%= target_name %> CF_HK COMMAND_COUNTER")
        err_count = tlm("<%= target_name %> CF_HK COMMAND_ERROR_COUNTER")

        Group.print("Testing CF get_param on <%= target_name %>")
        cmd(f"<%= target_name %> CF_CMD_GET_PARAM with KEY 6, CHAN_NUM {channel}")
        wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CF_HK COMMAND_ERROR_COUNTER == {err_count}", 100)

    def test_engine_control(self):
        """
        CF Engine Control Commands
        - Toggle the CFDP engine enable state
            then verify the command counter increments and engine ends enabled
        """
        Group.print("Testing CF engine enable/disable on <%= target_name %>")
        self._require_hk("CF")
        cmd_count = tlm("<%= target_name %> CF_HK COMMAND_COUNTER")
        err_count = tlm("<%= target_name %> CF_HK COMMAND_ERROR_COUNTER")

        #------------- Command 1: Enable Engine (idempotent) -----------------------
        Group.print("Testing CF enable_engine on <%= target_name %>")
        cmd("<%= target_name %> CF_CMD_ENABLE_ENGINE")
        wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CF_HK COMMAND_ERROR_COUNTER == {err_count}", 100)

        #------------- Command 2: Disable Engine ----------------------------------
        self._require_hk("CF")
        cmd_count = tlm("<%= target_name %> CF_HK COMMAND_COUNTER")
        err_count = tlm("<%= target_name %> CF_HK COMMAND_ERROR_COUNTER")

        Group.print("Testing CF disable_engine on <%= target_name %>")
        cmd("<%= target_name %> CF_CMD_DISABLE_ENGINE")
        wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CF_HK COMMAND_ERROR_COUNTER == {err_count}", 100)

        #------------- Command 3: Enable Engine (restore) -------------------------
        self._require_hk("CF")
        cmd_count = tlm("<%= target_name %> CF_HK COMMAND_COUNTER")
        err_count = tlm("<%= target_name %> CF_HK COMMAND_ERROR_COUNTER")

        Group.print("Restoring CF engine enable state on <%= target_name %>")
        cmd("<%= target_name %> CF_CMD_ENABLE_ENGINE")
        wait_check(f"<%= target_name %> CF_HK COMMAND_COUNTER == {cmd_count + 1}", 100)
        wait_check(f"<%= target_name %> CF_HK COMMAND_ERROR_COUNTER == {err_count}", 100)

    def setup(self):
        """
        Test Group Setup
        - Runs when Group Setup button is pressed
        - Runs before all scripts when Group Start is pressed
        """
        Group.print(f"Starting CF test group on <%= target_name %>")

    def teardown(self):
        """
        Test Group Setup
        - Runs when Group Teardown button is pressed
        - Runs after all scripts when Group Start is pressed
        """
        return
