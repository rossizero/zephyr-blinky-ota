# sysbuild.cmake
function(select_sysbuild_config)
    set(SYSBUILD_DIR ${CMAKE_CURRENT_LIST_DIR})
    # List of possible config files in order of preference
    set(CONFIG_CANDIDATES
        "${SYSBUILD_DIR}/sysbuild-${BOARD}.conf"
        "${SYSBUILD_DIR}/sysbuild-default.conf"
        "${SYSBUILD_DIR}/sysbuild.conf"
    )
    
    # Find the first existing config file
    foreach(CONFIG_FILE ${CONFIG_CANDIDATES})
        if(EXISTS ${CONFIG_FILE})
            set(SB_CONF_FILE ${CONFIG_FILE} PARENT_SCOPE)
            message(STATUS "Sysbuild: Selected config file: ${CONFIG_FILE}")
            return()
        endif()
    endforeach()
    
    # No config file found
    message(FATAL_ERROR "Sysbuild: No configuration file found. Tried: ${CONFIG_CANDIDATES}")
endfunction()

# Call the function
select_sysbuild_config()