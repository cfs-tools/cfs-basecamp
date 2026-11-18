#
# Procedure: Run performance data collection
# 
# Assumptions:
#   - Telemetry output has already been enabled by the user
#   - ES housekeeping telemetry is being requested automatically
#

perf_data_file = "/cf/cfe_es_perf.dat"

perf_data_time = ask("Enter the number of seconds to let the performance analyzer run for")

wait_check(f"<%= target_name %> CFE_ES_HK PERF_STATE == 'IDLE'", 10)

# Start performance data collection
cmd(f"<%= target_name %> CFE_ES_CMD_START_PERF_DATA with TRIGGER_MODE 'START'")

wait_check(f"<%= target_name %> CFE_ES_HK PERF_STATE == 'WAITING_FOR_TRIGGER'", 10)

print("Wait while the performance data is being collected....")

# Idle while performance data is being collected
wait(perf_data_time)

# Stop performance data collection
cmd(f"<%= target_name %> CFE_ES_CMD_STOP_PERF_DATA with DATA_FILE_NAME '{perf_data_file}'")

# Verify data collection has stopped
wait_check(f"<%= target_name %> CFE_ES_HK PERF_STATE == 'IDLE'", 10)

print(f"The performance log can be found in the FSW directory at build/exe/cpu<cfs_cpu_num>{perf_data_file}")
