cmake_minimum_required(VERSION 3.13)

project(caps-log
    LANGUAGES CXX
    VERSION 1.0.0
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option (CAPS_LOG_SNAP_BUILD, "Snap build of caps-log" Off)

include(FetchContent)

# -------------------------- Fetech ftxui ------------------------------- #
 
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
    GIT_TAG v5.0.0
)
FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
    FetchContent_Populate(ftxui)
    add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# --------------------------------- boost -------------------------------- #

find_package(Boost 1.40 COMPONENTS program_options REQUIRED)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost Not found")
endif()

# ------------------------------- OpenSSL -------------------------------- #

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

# ------------------------------- Fetch FMT ------------------------------ #
 
FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG 9.1.0
)
FetchContent_GetProperties(fmt)
if(NOT fmt_POPULATED)
    FetchContent_Populate(fmt)
    add_subdirectory(${fmt_SOURCE_DIR} ${fmt_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# --------------------------------- tests -------------------------------- #

if(${CAPS_LOG_BUILD_TESTS})
    message("Tests will be built")
    include(CTest)
    add_subdirectory(./test)
endif()

# ------------------------------ caps-log -------------------------------- #

add_subdirectory(./source)
