###########################################################
#
# RECORDER_CACHE_APP platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the RECORDER_CACHE_APP configuration
set(RECORDER_CACHE_APP_PLATFORM_CONFIG_FILE_LIST
  recorder_cache_app_internal_cfg_values.h
  recorder_cache_app_platform_cfg.h
  recorder_cache_app_perfids.h
  recorder_cache_app_msgids.h
  recorder_cache_app_msgid_values.h
)

generate_configfile_set(${RECORDER_CACHE_APP_PLATFORM_CONFIG_FILE_LIST})

