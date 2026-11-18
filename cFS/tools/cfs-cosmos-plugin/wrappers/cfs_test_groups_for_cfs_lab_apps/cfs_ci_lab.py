############################################
# 
# Wrapper allowing the ci_lab_test_methods class to be
# used in COSMOS.  This just calls the corresponding method.
#
############################################

from openc3.script.suite import Group
from cosmos_adapter import CfsTest_Cosmos_Adapter

load_utility("<%= target_name %>/procedures/cfs_test_groups_for_apps/ci_lab_test_methods.py")

# Group class name should indicate what the scripts are testing
class cfs_test_group_cfs_ci_lab(Group):
    """
    - Methods beginning with script_ or test_ are added to Script dropdown
    """
    def test_aliveness(self):
        """
        FSW Aliveness Test
        - Send a no-op command
            then verify the command was received (by checking the command counter incremented)
        - Reset the command counter
            then verify the command was received (by checking the command counter was cleared)
        """
        return self.Methods.execute("test_aliveness")
    
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
        self.Methods = CfsTest_Cosmos_Adapter(self, ci_lab_test_methods, app_name = "CI_LAB", instance_name = "<%= target_name %>")

