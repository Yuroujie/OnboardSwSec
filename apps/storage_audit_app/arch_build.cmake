###########################################################
#
# STORAGE_AUDIT_APP platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the STORAGE_AUDIT_APP configuration
set(STORAGE_AUDIT_APP_PLATFORM_CONFIG_FILE_LIST
  storage_audit_app_internal_cfg_values.h
  storage_audit_app_platform_cfg.h
  storage_audit_app_perfids.h
  storage_audit_app_msgids.h
  storage_audit_app_msgid_values.h
)

generate_configfile_set(${STORAGE_AUDIT_APP_PLATFORM_CONFIG_FILE_LIST})

