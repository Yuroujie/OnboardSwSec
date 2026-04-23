###########################################################
#
# BUFFER_MGR_APP mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the BUFFER_MGR_APP configuration
set(BUFFER_MGR_APP_MISSION_CONFIG_FILE_LIST
  buffer_mgr_app_fcncode_values.h
  buffer_mgr_app_interface_cfg_values.h
  buffer_mgr_app_mission_cfg.h
  buffer_mgr_app_perfids.h
  buffer_mgr_app_msg.h
  buffer_mgr_app_msgdefs.h
  buffer_mgr_app_msgstruct.h
  buffer_mgr_app_tbl.h
  buffer_mgr_app_tbldefs.h
  buffer_mgr_app_tblstruct.h
  buffer_mgr_app_topicid_values.h
)

generate_configfile_set(${BUFFER_MGR_APP_MISSION_CONFIG_FILE_LIST})

