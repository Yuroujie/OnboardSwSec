###########################################################
#
# SAFE_SAFE_MODE_APP platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the SAFE_SAFE_MODE_APP configuration
set(SAFE_SAFE_MODE_APP_PLATFORM_CONFIG_FILE_LIST
  safe_mode_app_internal_cfg_values.h
  safe_mode_app_platform_cfg.h
  safe_mode_app_perfids.h
  safe_mode_app_msgids.h
  safe_mode_app_msgid_values.h
)

generate_configfile_set(${SAFE_SAFE_MODE_APP_PLATFORM_CONFIG_FILE_LIST})

