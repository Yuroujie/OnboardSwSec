###########################################################
#
# PAYLOAD_TASK_APP platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the PAYLOAD_TASK_APP configuration
set(PAYLOAD_TASK_APP_PLATFORM_CONFIG_FILE_LIST
  payload_task_app_internal_cfg_values.h
  payload_task_app_platform_cfg.h
  payload_task_app_perfids.h
  payload_task_app_msgids.h
  payload_task_app_msgid_values.h
)

generate_configfile_set(${PAYLOAD_TASK_APP_PLATFORM_CONFIG_FILE_LIST})

