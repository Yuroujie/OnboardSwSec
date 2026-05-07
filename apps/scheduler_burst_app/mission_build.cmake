###########################################################
#
# SCHEDULER_BURST_APP mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the SCHEDULER_BURST_APP configuration
set(SCHEDULER_BURST_APP_MISSION_CONFIG_FILE_LIST
  scheduler_burst_app_fcncode_values.h
  scheduler_burst_app_interface_cfg_values.h
  scheduler_burst_app_mission_cfg.h
  scheduler_burst_app_perfids.h
  scheduler_burst_app_msg.h
  scheduler_burst_app_msgdefs.h
  scheduler_burst_app_msgstruct.h
  scheduler_burst_app_tbl.h
  scheduler_burst_app_tbldefs.h
  scheduler_burst_app_tblstruct.h
  scheduler_burst_app_topicid_values.h
)

generate_configfile_set(${SCHEDULER_BURST_APP_MISSION_CONFIG_FILE_LIST})

