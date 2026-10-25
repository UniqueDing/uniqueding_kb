cmake_minimum_required(VERSION 3.20)
project(charybdis_diag_postbuild_fixture C)

get_filename_component(DIAG_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(APPLICATION_SOURCE_DIR "${DIAG_REPO_ROOT}")
set(ZEPHYR_BASE "${DIAG_REPO_ROOT}")
find_program(CMAKE_NM NAMES nm REQUIRED)
find_program(CMAKE_OBJDUMP NAMES objdump REQUIRED)

set(CONFIG_CHARYBDIS_TRACKBALL_DIAGNOSTICS ON)
set(CONFIG_INPUT_MODE_THREAD ON)
set(CONFIG_ZMK_BLE ON)
set(CONFIG_ZMK_STUDIO ON)
set(CONFIG_ZMK_STUDIO_TRANSPORT_BLE ON)
set(CONFIG_ZMK_STUDIO_TRANSPORT_UART OFF)
set(CONFIG_ZMK_USB OFF)
set(CONFIG_ZMK_POINTING ON)
set(CONFIG_ZMK_SPLIT_ROLE_CENTRAL ON)
set(CONFIG_LOG ON)
set(CONFIG_LOG_MODE_DEFERRED ON)
set(CONFIG_ZMK_LOGGING_MINIMAL ON)
set(CONFIG_ZMK_LOG_LEVEL_OFF ON)
set(CONFIG_LOG_DEFAULT_LEVEL 0)
set(CONFIG_INPUT_EVENT_DUMP OFF)
set(CONFIG_INPUT_LOG_LEVEL_OFF ON)
set(CONFIG_LOG_BLOCK_IN_THREAD OFF)

add_executable(zephyr_final main.c)
add_library(app STATIC main.c)

function(zephyr_ld_options)
  target_link_options(zephyr_final PRIVATE ${ARGV})
endfunction()

add_subdirectory("${DIAG_REPO_ROOT}/diagnostics" diagnostics_module)

get_property(post_build GLOBAL PROPERTY extra_post_build_commands)
if(NOT post_build)
  message(FATAL_ERROR "diagnostics module did not append a global post-build command")
endif()
string(JOIN " " post_build_text ${post_build})
foreach(required_text
    "COMMAND"
    "${Python3_EXECUTABLE}"
    "verify_postlink.py"
    "$<TARGET_FILE:zephyr_final>"
    "--nm"
    "${CMAKE_NM}"
    "--objdump"
    "${CMAKE_OBJDUMP}")
  string(FIND "${post_build_text}" "${required_text}" found_at)
  if(found_at EQUAL -1)
    message(FATAL_ERROR "global extra_post_build_commands lacks: ${required_text}")
  endif()
endforeach()

# Match Zephyr 3.5's global-property consumer: post-build commands are attached
# to the final ELF target, and its generator expression remains intact.
add_custom_command(TARGET zephyr_final POST_BUILD ${post_build})
