###########################################################
#
# MODE_APP platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the MODE_APP configuration
set(MODE_APP_PLATFORM_CONFIG_FILE_LIST
  mode_app_internal_cfg_values.h
  mode_app_platform_cfg.h
  mode_app_perfids.h
  mode_app_msgids.h
  mode_app_msgid_values.h
)

generate_configfile_set(${MODE_APP_PLATFORM_CONFIG_FILE_LIST})

