############################################
# 
# Wrapper allowing the to_lab_test_methods class to be
# used in COSMOS.  This just calls the corresponding method.
#
############################################

from openc3.script.suite import Group
from cosmos_adapter import CfsTest_Cosmos_Adapter

load_utility("<%= target_name %>/procedures/cfs_test_groups_for_apps/to_lab_test_methods.py")

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfs_to_lab(Group):
    """
    - Methods beginning with script_ or test_ are added to Script dropdown
    """
    def test_00_aliveness(self):
        return self.Methods.execute("test_aliveness")

    def test_01_manage_tlm_subscriptions(self):
        return self.Methods.execute("test_manage_tlm_subscriptions")

    def test_03_remove_all_packet_subscriptions(self):
        return self.Methods.execute("test_remove_all_packet_subscriptions")

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

    def __init__(self):
        tlm_mids = {
            'FM_HK': <%= get_cfs_pkt_msg_id('FM_HK', cfs_cpu_num_from_target_name(target_name)) %>
        }
        self.Methods = CfsTest_Cosmos_Adapter(self, to_lab_test_methods,
                                              instance_name = "<%= target_name %>",
                                              tlm_mids = tlm_mids
                                              )
