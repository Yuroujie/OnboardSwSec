###########################################################
#
# MODE_APP mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the MODE_APP configuration
set(MODE_APP_MISSION_CONFIG_FILE_LIST
  mode_app_fcncode_values.h
  mode_app_interface_cfg_values.h
  mode_app_mission_cfg.h
  mode_app_perfids.h
  mode_app_msg.h
  mode_app_msgdefs.h
  mode_app_msgstruct.h
  mode_app_tbl.h
  mode_app_tbldefs.h
  mode_app_tblstruct.h
  mode_app_topicid_values.h
)

generate_configfile_set(${MODE_APP_MISSION_CONFIG_FILE_LIST})

