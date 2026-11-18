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
