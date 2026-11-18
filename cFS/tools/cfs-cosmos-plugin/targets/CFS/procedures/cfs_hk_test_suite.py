from openc3.script.suite import Suite, Group

load_utility("<%= target_name %>/procedures/cfs_test_groups_for_cfs_open_src/cfs_hk.py")


class cfs_hk_test_suite(Suite):
  def __init__(self):
      # Add each test group
      # ------------------------------------------------------------------------
      # The order that we add these test groups affects the order
      # they're listed in the COSMOS Script Runner drop-down menu
      # To make it easier to find tests using the drop-down,
      # these are added in alphabetical order
      # -----------------------------------------------------------------------
      self.add_group(cfs_test_group_cfs_hk)
    

  def setup(self):
      # Run when Suite Setup button is pressed
      # Run before all groups when Suite Start is pressed
      Group.print(f"Starting HK Integration Test Suite against the following target:")
      Group.print(f"cFS Target Name (target_name): <%= target_name %>")
      
      # Send the command, TO_LAB_CMD_ENABLE_OUTPUT
      to_lab_dest_ip = "<%= global_tlm_output_ip %>"
      cmd(f"<%= target_name %> TO_LAB_CMD_ENABLE_OUTPUT with DEST_IP '{to_lab_dest_ip}'")

      Group.print(f"Sent TO_LAB_CMD_ENABLE_OUTPUT command to <%= target_name %> with DEST_IP '{to_lab_dest_ip}'")

      # Wait for one TO packet to be received
      wait_check_packet(f"<%= target_name %>", "TO_LAB_HK", 1, 100)

  def teardown(self):
      # Run when Suite Teardown button is pressed
      # Run after all groups when Suite Start is pressed
      pass
