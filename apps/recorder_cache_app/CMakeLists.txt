project(CFE_RECORDER_CACHE_APP C)

set(APP_SRC_FILES
  fsw/src/recorder_cache_app.c
  fsw/src/recorder_cache_app_cmds.c
  fsw/src/recorder_cache_app_utils.c
)

if (CFE_EDS_ENABLED)
  list(APPEND APP_SRC_FILES
    fsw/src/recorder_cache_app_eds_dispatch.c
  )
else()
  list(APPEND APP_SRC_FILES
    fsw/src/recorder_cache_app_dispatch.c
  )
endif()

add_cfe_app(recorder_cache_app ${APP_SRC_FILES})

target_include_directories(recorder_cache_app PUBLIC fsw/inc)
add_cfe_app_dependency(recorder_cache_app sample_lib)

add_cfe_tables(recorder_cache_app fsw/tables/recorder_cache_app_tbl.c)

# If UT is enabled, then add the tests from the subdirectory
# Note that this is an app, and therefore does not provide
# stub functions, as other entities would not typically make
# direct function calls into this application.
if (ENABLE_UNIT_TESTS)
  add_subdirectory(unit-test)
endif (ENABLE_UNIT_TESTS)
