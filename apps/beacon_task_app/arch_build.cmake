###########################################################
#
# BEACON_TASK_APP platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the BEACON_TASK_APP configuration
set(BEACON_TASK_APP_PLATFORM_CONFIG_FILE_LIST
  beacon_task_app_internal_cfg_values.h
  beacon_task_app_platform_cfg.h
  beacon_task_app_perfids.h
  beacon_task_app_msgids.h
  beacon_task_app_msgid_values.h
)

generate_configfile_set(${BEACON_TASK_APP_PLATFORM_CONFIG_FILE_LIST})

