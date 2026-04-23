###########################################################
#
# PERIODIC_TASK_APP platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the PERIODIC_TASK_APP configuration
set(PERIODIC_TASK_APP_PLATFORM_CONFIG_FILE_LIST
  periodic_task_app_internal_cfg_values.h
  periodic_task_app_platform_cfg.h
  periodic_task_app_perfids.h
  periodic_task_app_msgids.h
  periodic_task_app_msgid_values.h
)

generate_configfile_set(${PERIODIC_TASK_APP_PLATFORM_CONFIG_FILE_LIST})

