enable_testing()

# ------------------------------- Fetch FMT ------------------------------ #
 
FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt)
FetchContent_GetProperties(fmt)
if(NOT fmt_POPULATED)
    FetchContent_Populate(fmt)
    add_subdirectory(${fmt_SOURCE_DIR} ${fmt_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# --------------------------------- gtest -------------------------------- #

set(INSTALL_GTEST OFF)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# --------------------------------- tests -------------------------------- #

add_executable(
    tests
    ./controler_test.cpp
    ./model/date_test.cpp
    ./../source/date/date.cpp
    ./../source/model/log_file.cpp
    ./../source/model/default_log_repository.cpp
)

target_link_libraries(
    tests
    gtest_main
    gmock_main
    ftxui::screen
    ftxui::dom
    ftxui::component 
    fmt::fmt
)

target_include_directories(
    tests
    PRIVATE
    ../source/
)

include(GoogleTest)
gtest_discover_tests(tests)
