from openc3.script.suite import Suite, Group

# Load the cFE Test Groups
load_utility("<%= target_name %>/procedures/cfs_test_groups_for_cfe/cfe_es.py")
load_utility("<%= target_name %>/procedures/cfs_test_groups_for_cfe/cfe_evs.py")
load_utility("<%= target_name %>/procedures/cfs_test_groups_for_cfe/cfe_sb.py")
load_utility("<%= target_name %>/procedures/cfs_test_groups_for_cfe/cfe_tbl.py")
load_utility("<%= target_name %>/procedures/cfs_test_groups_for_cfe/cfe_time.py")

# Load the Open-Source cFS "Lab" App Test Groups
load_utility("<%= target_name %>/procedures/cfs_test_groups_for_cfs_lab_apps/cfs_ci_lab.py")
load_utility("<%= target_name %>/procedures/cfs_test_groups_for_cfs_lab_apps/cfs_to_lab.py")

# Load the Open-Source cFS App Test Groups
load_utility("<%= target_name %>/procedures/cfs_test_groups_for_cfs_open_src/cfs_cf.py")
load_utility("<%= target_name %>/procedures/cfs_test_groups_for_cfs_open_src/cfs_cs.py")
load_utility("<%= target_name %>/procedures/cfs_test_groups_for_cfs_open_src/cfs_ds.py")
load_utility("<%= target_name %>/procedures/cfs_test_groups_for_cfs_open_src/cfs_fm.py")
load_utility("<%= target_name %>/procedures/cfs_test_groups_for_cfs_open_src/cfs_hk.py")
load_utility("<%= target_name %>/procedures/cfs_test_groups_for_cfs_open_src/cfs_hs.py")
load_utility("<%= target_name %>/procedures/cfs_test_groups_for_cfs_open_src/cfs_lc.py")
load_utility("<%= target_name %>/procedures/cfs_test_groups_for_cfs_open_src/cfs_md.py")
load_utility("<%= target_name %>/procedures/cfs_test_groups_for_cfs_open_src/cfs_mm.py")
load_utility("<%= target_name %>/procedures/cfs_test_groups_for_cfs_open_src/cfs_sample_app.py")
load_utility("<%= target_name %>/procedures/cfs_test_groups_for_cfs_open_src/cfs_sc.py")

# Only test applications that have EDS implemented
if "<%= $cfs_globals_eds_enabled %>" == "false":
    load_utility("<%= target_name %>/procedures/cfs_test_groups_for_cfs_open_src/cfs_sbn.py")


class cfs_test_suite(Suite):
  def __init__(self):
      # Add each test group
      # ------------------------------------------------------------------------
      # The order that we add these test groups affects the order
      # they're listed in the COSMOS Script Runner drop-down menu
      # To make it easier to find tests using the drop-down,
      # these are added in alphabetical order
      # ------------------------------------------------------------------------
      # Add the cFE Test Groups
      self.add_group(cfs_test_group_cfe_es)
      self.add_group(cfs_test_group_cfe_evs)
      self.add_group(cfs_test_group_cfe_sb)
      self.add_group(cfs_test_group_cfe_tbl)
      self.add_group(cfs_test_group_cfe_time)
      # Load the Open-Source cFS "Lab" App Test Groups
      self.add_group(cfs_test_group_cfs_ci_lab)
      self.add_group(cfs_test_group_cfs_to_lab)
      # Load the Open-Source cFS App Test Groups
      self.add_group(cfs_test_group_cfs_cf)
      self.add_group(cfs_test_group_cfs_cs)
      self.add_group(cfs_test_group_cfs_ds)
      self.add_group(cfs_test_group_cfs_fm)
      self.add_group(cfs_test_group_cfs_hk)
      self.add_group(cfs_test_group_cfs_hs)
      self.add_group(cfs_test_group_cfs_lc)
      self.add_group(cfs_test_group_cfs_md)
      self.add_group(cfs_test_group_cfs_mm)
      self.add_group(cfs_test_group_cfs_sample_app)
      self.add_group(cfs_test_group_cfs_sc)

      # Only test applications that have EDS implemented
      if "<%= $cfs_globals_eds_enabled %>" == "false":
          self.add_group(cfs_test_group_cfs_sbn)

  def setup(self):
      # Run when Suite Setup button is pressed
      # Run before all groups when Suite Start is pressed
      Group.print(f"Starting FSW Integration Test Suite against the following target:")
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
