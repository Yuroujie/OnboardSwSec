project(CFE_PAYLOAD_TASK_APP C)

set(APP_SRC_FILES
  fsw/src/payload_task_app.c
  fsw/src/payload_task_app_cmds.c
  fsw/src/payload_task_app_utils.c
)

if (CFE_EDS_ENABLED)
  list(APPEND APP_SRC_FILES
    fsw/src/payload_task_app_eds_dispatch.c
  )
else()
  list(APPEND APP_SRC_FILES
    fsw/src/payload_task_app_dispatch.c
  )
endif()

add_cfe_app(payload_task_app ${APP_SRC_FILES})

target_include_directories(payload_task_app PUBLIC fsw/inc)
add_cfe_app_dependency(payload_task_app sample_lib)

add_cfe_tables(payload_task_app fsw/tables/payload_task_app_tbl.c)

# If UT is enabled, then add the tests from the subdirectory
# Note that this is an app, and therefore does not provide
# stub functions, as other entities would not typically make
# direct function calls into this application.
if (ENABLE_UNIT_TESTS)
  add_subdirectory(unit-test)
endif (ENABLE_UNIT_TESTS)
