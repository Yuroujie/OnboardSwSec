###########################################################
#
# BUFFER_MGR_APP platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the BUFFER_MGR_APP configuration
set(BUFFER_MGR_APP_PLATFORM_CONFIG_FILE_LIST
  buffer_mgr_app_internal_cfg_values.h
  buffer_mgr_app_platform_cfg.h
  buffer_mgr_app_perfids.h
  buffer_mgr_app_msgids.h
  buffer_mgr_app_msgid_values.h
)

generate_configfile_set(${BUFFER_MGR_APP_PLATFORM_CONFIG_FILE_LIST})

