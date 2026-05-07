###########################################################
#
# DOWNLINK_QUEUE_APP platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the DOWNLINK_QUEUE_APP configuration
set(DOWNLINK_QUEUE_APP_PLATFORM_CONFIG_FILE_LIST
  downlink_queue_app_internal_cfg_values.h
  downlink_queue_app_platform_cfg.h
  downlink_queue_app_perfids.h
  downlink_queue_app_msgids.h
  downlink_queue_app_msgid_values.h
)

generate_configfile_set(${DOWNLINK_QUEUE_APP_PLATFORM_CONFIG_FILE_LIST})

