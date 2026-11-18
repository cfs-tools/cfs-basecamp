import re
from testrunner import TestrunnerAdapter

class TestGroup:

    def execute(self, method : str, **kwargs):
        print(f"execute {method}")
        f = self.test_dict[method]
        return f(**kwargs)

    # Some tests are prefixed with a number in order to
    def find_matching_test(self, method : str, **kwargs):
        result = None
        for test_name in self.test_dict.keys():
            m = re.match(r'^test_[\d_]*(\w+)$', test_name)
            if m and m.group(1) == method:
                result = test_name
        return result


    def execute_all(self, **kwargs):
        # Using "sorted" makes them execute in a deterministic order
        for test_name in sorted(self.test_dict.keys()):
            self.execute(test_name, **kwargs)

    def enable_output(self):
        self.Methods.enable_output()

    def __init__(self, method_type : type, **kwargs):
        self.Methods = TestrunnerAdapter(method_type, **kwargs)
        self.Methods.register_tests(self)
        return
