###########################################################
#
# SCHEDULER_BURST_APP platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the SCHEDULER_BURST_APP configuration
set(SCHEDULER_BURST_APP_PLATFORM_CONFIG_FILE_LIST
  scheduler_burst_app_internal_cfg_values.h
  scheduler_burst_app_platform_cfg.h
  scheduler_burst_app_perfids.h
  scheduler_burst_app_msgids.h
  scheduler_burst_app_msgid_values.h
)

generate_configfile_set(${SCHEDULER_BURST_APP_PLATFORM_CONFIG_FILE_LIST})

