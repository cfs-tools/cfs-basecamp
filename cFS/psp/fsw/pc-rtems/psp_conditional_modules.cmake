# The sysmon module only works in RTEMS >= 5.
# Ideally this check should be VERSION_GREATER_EQUAL 5, but that conditional does
# not exist in all versions of cmake.
if(CMAKE_SYSTEM_VERSION VERSION_GREATER 4.99)

    list(APPEND PSP_TARGET_MODULE_LIST rtems_sysmon)

endif()

# Selecting the network module based on platform being used
# For Qemu RTEMS versions 4.11,5,6
if(RTEMS_BSP MATCHES "pc686")
    list(APPEND PSP_TARGET_MODULE_LIST network_pc686_rtems)
endif()

# Select correct implementation of ram access module
if (PSP_CONFIG_RAM_DIRECT_MODE AND PSP_CONFIG_RAM_DIRECT_STRICT_MODE)
    list(APPEND PSP_TARGET_MODULE_LIST ram_direct_strict)
elseif (PSP_CONFIG_RAM_DIRECT_MODE)
    list(APPEND PSP_TARGET_MODULE_LIST ram_direct)
else ()
    list(APPEND PSP_TARGET_MODULE_LIST ram_notimpl)
endif()

# Select ram access scratchpad module
if (PSP_CONFIG_RAM_DEBUG_SCRATCHPAD)
    list(APPEND PSP_TARGET_MODULE_LIST ram_debug)
endif()
