#
# Procedure: Get information about a flight software file and check the return output
# 
# Assumptions:
#   - Telemetry output has already been enabled by the user
#

file_to_get = "/cf/sample_app_tbl.tbl"
exp_file_crc = 0x17DB

# Get current packet count
curr_file_info_pkt_count = tlm(f"<%= target_name %> FM_FILE_INFO RECEIVED_COUNT")
if curr_file_info_pkt_count is None:
    curr_file_info_pkt_count = 0        
exp_file_info_pkt_count = curr_file_info_pkt_count + 1

# Send FM get file information command
cmd(f"<%= target_name %> FM_CMD_GET_FILE_INFO with PATH '{file_to_get}', CRC_METHOD 'CRC_16'")

# Wait for a file info packet to be returned and validate its contents
wait_check(f"<%= target_name %> FM_FILE_INFO RECEIVED_COUNT == {exp_file_info_pkt_count}", 10)
check(f"<%= target_name %> FM_FILE_INFO NAME == '{file_to_get}'")
check(f"<%= target_name %> FM_FILE_INFO CRC == {exp_file_crc}")

print(f"File info for {file_to_get} successfully retrieved!")
