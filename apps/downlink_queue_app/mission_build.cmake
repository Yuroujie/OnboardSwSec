###########################################################
#
# DOWNLINK_QUEUE_APP mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the DOWNLINK_QUEUE_APP configuration
set(DOWNLINK_QUEUE_APP_MISSION_CONFIG_FILE_LIST
  downlink_queue_app_fcncode_values.h
  downlink_queue_app_interface_cfg_values.h
  downlink_queue_app_mission_cfg.h
  downlink_queue_app_perfids.h
  downlink_queue_app_msg.h
  downlink_queue_app_msgdefs.h
  downlink_queue_app_msgstruct.h
  downlink_queue_app_tbl.h
  downlink_queue_app_tbldefs.h
  downlink_queue_app_tblstruct.h
  downlink_queue_app_topicid_values.h
)

generate_configfile_set(${DOWNLINK_QUEUE_APP_MISSION_CONFIG_FILE_LIST})

