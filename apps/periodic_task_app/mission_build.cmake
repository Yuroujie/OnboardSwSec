###########################################################
#
# PERIODIC_TASK_APP mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the PERIODIC_TASK_APP configuration
set(PERIODIC_TASK_APP_MISSION_CONFIG_FILE_LIST
  periodic_task_app_fcncode_values.h
  periodic_task_app_interface_cfg_values.h
  periodic_task_app_mission_cfg.h
  periodic_task_app_perfids.h
  periodic_task_app_msg.h
  periodic_task_app_msgdefs.h
  periodic_task_app_msgstruct.h
  periodic_task_app_tbl.h
  periodic_task_app_tbldefs.h
  periodic_task_app_tblstruct.h
  periodic_task_app_topicid_values.h
)

generate_configfile_set(${PERIODIC_TASK_APP_MISSION_CONFIG_FILE_LIST})

