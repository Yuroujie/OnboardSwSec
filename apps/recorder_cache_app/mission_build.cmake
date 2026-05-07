###########################################################
#
# RECORDER_CACHE_APP mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the RECORDER_CACHE_APP configuration
set(RECORDER_CACHE_APP_MISSION_CONFIG_FILE_LIST
  recorder_cache_app_fcncode_values.h
  recorder_cache_app_interface_cfg_values.h
  recorder_cache_app_mission_cfg.h
  recorder_cache_app_perfids.h
  recorder_cache_app_msg.h
  recorder_cache_app_msgdefs.h
  recorder_cache_app_msgstruct.h
  recorder_cache_app_tbl.h
  recorder_cache_app_tbldefs.h
  recorder_cache_app_tblstruct.h
  recorder_cache_app_topicid_values.h
)

generate_configfile_set(${RECORDER_CACHE_APP_MISSION_CONFIG_FILE_LIST})

