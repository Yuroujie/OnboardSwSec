###########################################################
#
# AUTONOMY_GUARD_APP platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the AUTONOMY_GUARD_APP configuration
set(AUTONOMY_GUARD_APP_PLATFORM_CONFIG_FILE_LIST
  autonomy_guard_app_internal_cfg_values.h
  autonomy_guard_app_platform_cfg.h
  autonomy_guard_app_perfids.h
  autonomy_guard_app_msgids.h
  autonomy_guard_app_msgid_values.h
)

generate_configfile_set(${AUTONOMY_GUARD_APP_PLATFORM_CONFIG_FILE_LIST})

