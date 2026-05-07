###########################################################
#
# FAULT_RESPONSE_APP platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the FAULT_RESPONSE_APP configuration
set(FAULT_RESPONSE_APP_PLATFORM_CONFIG_FILE_LIST
  fault_response_app_internal_cfg_values.h
  fault_response_app_platform_cfg.h
  fault_response_app_perfids.h
  fault_response_app_msgids.h
  fault_response_app_msgid_values.h
)

generate_configfile_set(${FAULT_RESPONSE_APP_PLATFORM_CONFIG_FILE_LIST})

