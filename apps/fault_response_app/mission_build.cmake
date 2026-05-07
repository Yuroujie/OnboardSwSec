###########################################################
#
# FAULT_RESPONSE_APP mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the FAULT_RESPONSE_APP configuration
set(FAULT_RESPONSE_APP_MISSION_CONFIG_FILE_LIST
  fault_response_app_fcncode_values.h
  fault_response_app_interface_cfg_values.h
  fault_response_app_mission_cfg.h
  fault_response_app_perfids.h
  fault_response_app_msg.h
  fault_response_app_msgdefs.h
  fault_response_app_msgstruct.h
  fault_response_app_tbl.h
  fault_response_app_tbldefs.h
  fault_response_app_tblstruct.h
  fault_response_app_topicid_values.h
)

generate_configfile_set(${FAULT_RESPONSE_APP_MISSION_CONFIG_FILE_LIST})

