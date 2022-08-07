cmake_minimum_required(VERSION 3.14)

project(clog
    LANGUAGES CXX
    VERSION 1.0.0
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# --------------------------------- ftxui ------------------------------- #
 
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui)
FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
    FetchContent_Populate(ftxui)
    add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# --------------------------------- tests -------------------------------- #

if(${BUILD_TESTS})
    message("Tests will be built")
    include(CTest)
    add_subdirectory(./test)
endif()

# ---------------------------------- clog -------------------------------- #

add_subdirectory(./source)
