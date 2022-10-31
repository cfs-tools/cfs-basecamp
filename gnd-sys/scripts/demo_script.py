"""
This demo script serves as a template for new user scripts. Basecamp is
not intended to be a full featured ground system so the ScriptRunner
context provides sending commands and receiving/waiting on telemetry values.

The script executes in the context of a ScriptRunner object which is a
CmdTlmProcess child class.

"""

print ('*** Start Demo ***')

sg.popup("Example popup: Enter <OK> to start demo")

prev_cmd_counter     = int(self.get_tlm_val("CFE_ES", "HK_TLM", "CommandCounter"))
prev_cmd_err_counter = int(self.get_tlm_val("CFE_ES", "HK_TLM", "CommandErrorCounter"))
print ('prev_cmd_counter=%s, prev_cmd_err_counter=%s' % (prev_cmd_counter,prev_cmd_err_counter))

self.send_cfs_cmd('CFE_ES', 'NoopCmd', {})
time.sleep(8) # TODO: This will be replaced by a get_tlm_val_wait() when it' implemented

cmd_counter     = int(self.get_tlm_val("CFE_ES", "HK_TLM", "CommandCounter"))
cmd_err_counter = int(self.get_tlm_val("CFE_ES", "HK_TLM", "CommandErrorCounter"))
print ('cmd_counter=%d, cmd_err_counter=%d' % (cmd_counter,cmd_err_counter))

if ((cmd_counter != prev_cmd_counter) and (cmd_err_counter == prev_cmd_err_counter)):
    print (">>> TEST PASSED <<<")
else:
    print (">>> TEST FAILED <<<")

print ('*** End Demo ***')


"""
Example script snippets

for i in range(5):
   self.test_tlm_val()
   time.sleep(4)

self.send_cfs_cmd('CFE_ES', 'NoopCmd', {})
time.sleep(1)
self.send_cfs_cmd('CFE_EVS', 'NoopCmd', {})
time.sleep(1)
self.send_cfs_cmd('CFE_SB', 'NoopCmd', {})
time.sleep(1)
self.send_cfs_cmd('CFE_TBL', 'NoopCmd', {})
time.sleep(1)
self.send_cfs_cmd('CFE_TIME', 'NoopCmd', {})
"""
