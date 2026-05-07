###########################################################
#
# AUTONOMY_GUARD_APP mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the AUTONOMY_GUARD_APP configuration
set(AUTONOMY_GUARD_APP_MISSION_CONFIG_FILE_LIST
  autonomy_guard_app_fcncode_values.h
  autonomy_guard_app_interface_cfg_values.h
  autonomy_guard_app_mission_cfg.h
  autonomy_guard_app_perfids.h
  autonomy_guard_app_msg.h
  autonomy_guard_app_msgdefs.h
  autonomy_guard_app_msgstruct.h
  autonomy_guard_app_tbl.h
  autonomy_guard_app_tbldefs.h
  autonomy_guard_app_tblstruct.h
  autonomy_guard_app_topicid_values.h
)

generate_configfile_set(${AUTONOMY_GUARD_APP_MISSION_CONFIG_FILE_LIST})

