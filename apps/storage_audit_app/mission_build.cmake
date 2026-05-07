###########################################################
#
# STORAGE_AUDIT_APP mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the STORAGE_AUDIT_APP configuration
set(STORAGE_AUDIT_APP_MISSION_CONFIG_FILE_LIST
  storage_audit_app_fcncode_values.h
  storage_audit_app_interface_cfg_values.h
  storage_audit_app_mission_cfg.h
  storage_audit_app_perfids.h
  storage_audit_app_msg.h
  storage_audit_app_msgdefs.h
  storage_audit_app_msgstruct.h
  storage_audit_app_tbl.h
  storage_audit_app_tbldefs.h
  storage_audit_app_tblstruct.h
  storage_audit_app_topicid_values.h
)

generate_configfile_set(${STORAGE_AUDIT_APP_MISSION_CONFIG_FILE_LIST})

