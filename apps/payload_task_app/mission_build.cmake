###########################################################
#
# PAYLOAD_TASK_APP mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the PAYLOAD_TASK_APP configuration
set(PAYLOAD_TASK_APP_MISSION_CONFIG_FILE_LIST
  payload_task_app_fcncode_values.h
  payload_task_app_interface_cfg_values.h
  payload_task_app_mission_cfg.h
  payload_task_app_perfids.h
  payload_task_app_msg.h
  payload_task_app_msgdefs.h
  payload_task_app_msgstruct.h
  payload_task_app_tbl.h
  payload_task_app_tbldefs.h
  payload_task_app_tblstruct.h
  payload_task_app_topicid_values.h
)

generate_configfile_set(${PAYLOAD_TASK_APP_MISSION_CONFIG_FILE_LIST})

