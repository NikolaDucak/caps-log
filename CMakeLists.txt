cmake_minimum_required(VERSION 3.14)

project(clog
    LANGUAGES CXX
    VERSION 1.0.0
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# ------------------------------- Fetch FTXUI ----------------------------- #
 
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui)
FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
    FetchContent_Populate(ftxui)
    add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()


# ---------------------------------- gtest -------------------------------- #

if(${BUILD_TESTS})
    message("Tests will be built")
    set(INSTALL_GTEST OFF)

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
    )

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_subdirectory(./test)
endif()

# ---------------------------------- clog -------------------------------- #

add_subdirectory(./source)
install (TARGETS clog RUNTIME DESTINATION bin)
