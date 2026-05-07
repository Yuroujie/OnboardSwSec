project(CFE_SAFE_SAFE_MODE_APP C)

set(APP_SRC_FILES
  fsw/src/safe_mode_app.c
  fsw/src/safe_mode_app_cmds.c
  fsw/src/safe_mode_app_utils.c
)

if (CFE_EDS_ENABLED)
  list(APPEND APP_SRC_FILES
    fsw/src/safe_mode_app_eds_dispatch.c
  )
else()
  list(APPEND APP_SRC_FILES
    fsw/src/safe_mode_app_dispatch.c
  )
endif()

add_cfe_app(safe_mode_app ${APP_SRC_FILES})

target_include_directories(safe_mode_app PUBLIC fsw/inc)
add_cfe_app_dependency(safe_mode_app sample_lib)

add_cfe_tables(safe_mode_app fsw/tables/safe_mode_app_tbl.c)

# If UT is enabled, then add the tests from the subdirectory
# Note that this is an app, and therefore does not provide
# stub functions, as other entities would not typically make
# direct function calls into this application.
if (ENABLE_UNIT_TESTS)
  add_subdirectory(unit-test)
endif (ENABLE_UNIT_TESTS)
