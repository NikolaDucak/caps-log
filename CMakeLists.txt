cmake_minimum_required(VERSION 3.14)

# If version number is not provided, use commit id.
if(NOT CAPS_LOG_VERSION)
  find_package(Git)
  if(Git_FOUND)
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
      OUTPUT_VARIABLE CAPS_LOG_COMMIT_ID
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  else()
    set(CAPS_LOG_COMMIT_ID "NOT_FOUND")
  endif()
  set(CAPS_LOG_VERSION "commit-${CAPS_LOG_COMMIT_ID}")
endif()

message(STATUS "Captain's Log version: ${CAPS_LOG_VERSION}")

# Project setup
project(caps-log LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)

# -------------------------- Fetech ftxui ------------------------------- #

find_package(ftxui 5)
if(NOT ftxui_FOUND)
  message(STATUS "ftxui Not found... fetching")
  FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
    GIT_TAG v5.0.0)
  FetchContent_GetProperties(ftxui)
  if(NOT ftxui_POPULATED)
    FetchContent_Populate(ftxui)
    add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
endif()

# --------------------------------- boost -------------------------------- #
find_package(
  Boost 
  COMPONENTS program_options
  REQUIRED)

if(NOT Boost_FOUND)
  message(FATAL_ERROR "Boost Not found")
endif()

# ------------------------------- OpenSSL -------------------------------- #

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)

# ------------------------------- LibGit2 -------------------------------- #

set(BUILD_SHARED_LIBS OFF)
find_package(LibGit2 3 REQUIRED)

# ------------------------------- Fetch FMT ------------------------------ #

find_package(fmt 9)
if(NOT fmt_FOUND)
  message(STATUS "fmt Not found... fetching")
  FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG 9.1.0)
  FetchContent_GetProperties(fmt)
  if(NOT fmt_POPULATED)
    FetchContent_Populate(fmt)
    add_subdirectory(${fmt_SOURCE_DIR} ${fmt_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
endif()

# --------------------------------- tests -------------------------------- #

if(${CAPS_LOG_BUILD_TESTS})
  message("Tests will be built")
  include(CTest)
  add_subdirectory(./test)
endif()

# ------------------------------ caps-log -------------------------------- #

add_subdirectory(./source)
