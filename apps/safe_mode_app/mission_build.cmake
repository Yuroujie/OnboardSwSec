###########################################################
#
# SAFE_SAFE_MODE_APP mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the SAFE_SAFE_MODE_APP configuration
set(SAFE_SAFE_MODE_APP_MISSION_CONFIG_FILE_LIST
  safe_mode_app_fcncode_values.h
  safe_mode_app_interface_cfg_values.h
  safe_mode_app_mission_cfg.h
  safe_mode_app_perfids.h
  safe_mode_app_msg.h
  safe_mode_app_msgdefs.h
  safe_mode_app_msgstruct.h
  safe_mode_app_tbl.h
  safe_mode_app_tbldefs.h
  safe_mode_app_tblstruct.h
  safe_mode_app_topicid_values.h
)

generate_configfile_set(${SAFE_SAFE_MODE_APP_MISSION_CONFIG_FILE_LIST})

