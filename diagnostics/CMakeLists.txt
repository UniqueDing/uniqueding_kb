if(NOT CONFIG_CHARYBDIS_TRACKBALL_DIAGNOSTICS)
  return()
endif()

foreach(required_config
    CONFIG_INPUT_MODE_THREAD
    CONFIG_ZMK_BLE
    CONFIG_ZMK_STUDIO
    CONFIG_ZMK_STUDIO_TRANSPORT_BLE
    CONFIG_ZMK_POINTING
    CONFIG_ZMK_SPLIT_ROLE_CENTRAL
    CONFIG_LOG
    CONFIG_LOG_MODE_DEFERRED
    CONFIG_ZMK_LOGGING_MINIMAL
    CONFIG_ZMK_LOG_LEVEL_OFF
    CONFIG_INPUT_LOG_LEVEL_OFF)
  if(NOT ${required_config})
    message(FATAL_ERROR
      "CHARYBDIS_TRACKBALL_DIAGNOSTICS requires ${required_config}=y")
  endif()
endforeach()

if(CONFIG_INPUT_EVENT_DUMP)
  message(FATAL_ERROR "CHARYBDIS_TRACKBALL_DIAGNOSTICS requires CONFIG_INPUT_EVENT_DUMP=n")
endif()
if(CONFIG_ZMK_STUDIO_TRANSPORT_UART)
  message(FATAL_ERROR "CHARYBDIS_TRACKBALL_DIAGNOSTICS requires CONFIG_ZMK_STUDIO_TRANSPORT_UART=n")
endif()
if(CONFIG_ZMK_USB)
  message(FATAL_ERROR "CHARYBDIS_TRACKBALL_DIAGNOSTICS requires CONFIG_ZMK_USB=n")
endif()
if(CONFIG_LOG_BLOCK_IN_THREAD)
  message(FATAL_ERROR "CHARYBDIS_TRACKBALL_DIAGNOSTICS requires CONFIG_LOG_BLOCK_IN_THREAD=n")
endif()
if(NOT DEFINED CONFIG_LOG_DEFAULT_LEVEL OR NOT CONFIG_LOG_DEFAULT_LEVEL STREQUAL "0")
  message(FATAL_ERROR "CHARYBDIS_TRACKBALL_DIAGNOSTICS requires CONFIG_LOG_DEFAULT_LEVEL=0")
endif()

if(NOT COMMAND zephyr_ld_options)
  message(FATAL_ERROR "Zephyr zephyr_ld_options() is required for diagnostic linker wrapping")
endif()
if(NOT CMAKE_NM OR NOT CMAKE_OBJDUMP)
  message(FATAL_ERROR "Diagnostic ELF verification requires CMAKE_NM and CMAKE_OBJDUMP")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

function(charybdis_diag_git_revision output_var source_dir description)
  execute_process(
    COMMAND git -C "${source_dir}" rev-parse --verify --short=12 HEAD
    RESULT_VARIABLE git_result
    OUTPUT_VARIABLE git_revision
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
  if(NOT git_result EQUAL 0 OR git_revision STREQUAL "")
    message(FATAL_ERROR "Diagnostics require a real git HEAD for ${description}: ${source_dir}")
  endif()
  set(${output_var} "${git_revision}" PARENT_SCOPE)
endfunction()

charybdis_diag_git_revision(diag_config_rev "${CMAKE_CURRENT_LIST_DIR}" "configuration/module")
charybdis_diag_git_revision(diag_zmk_rev "${APPLICATION_SOURCE_DIR}" "ZMK application")
charybdis_diag_git_revision(diag_zephyr_rev "${ZEPHYR_BASE}" "Zephyr")
if(CMAKE_BUILD_TYPE)
  set(diag_build_type "${CMAKE_BUILD_TYPE}")
else()
  set(diag_build_type "unspecified")
endif()

target_sources(app PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/src/trackball_diag.c
  ${CMAKE_CURRENT_LIST_DIR}/src/diag_stats.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
target_compile_definitions(app PRIVATE
  CHARYBDIS_DIAG_CONFIG_REV="${diag_config_rev}"
  CHARYBDIS_DIAG_ZMK_REV="${diag_zmk_rev}"
  CHARYBDIS_DIAG_ZEPHYR_REV="${diag_zephyr_rev}"
  CHARYBDIS_DIAG_BUILD_TYPE="${diag_build_type}"
  CHARYBDIS_DIAG_COMPILER="${CMAKE_C_COMPILER_ID}-${CMAKE_C_COMPILER_VERSION}")

zephyr_ld_options(-Wl,--wrap=input_report -Wl,--wrap=zmk_hog_send_mouse_report)

# Zephyr 3.5 consumes this GLOBAL property when attaching post-build commands
# to zephyr_final. Keep the final-target generator expression for that consumer.
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND "${Python3_EXECUTABLE}"
          "${CMAKE_CURRENT_LIST_DIR}/scripts/verify_postlink.py"
          --elf "$<TARGET_FILE:zephyr_final>"
          --nm "${CMAKE_NM}"
          --objdump "${CMAKE_OBJDUMP}")
