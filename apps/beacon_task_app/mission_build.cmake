###########################################################
#
# BEACON_TASK_APP mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the BEACON_TASK_APP configuration
set(BEACON_TASK_APP_MISSION_CONFIG_FILE_LIST
  beacon_task_app_fcncode_values.h
  beacon_task_app_interface_cfg_values.h
  beacon_task_app_mission_cfg.h
  beacon_task_app_perfids.h
  beacon_task_app_msg.h
  beacon_task_app_msgdefs.h
  beacon_task_app_msgstruct.h
  beacon_task_app_tbl.h
  beacon_task_app_tbldefs.h
  beacon_task_app_tblstruct.h
  beacon_task_app_topicid_values.h
)

generate_configfile_set(${BEACON_TASK_APP_MISSION_CONFIG_FILE_LIST})

