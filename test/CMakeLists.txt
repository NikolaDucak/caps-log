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
    ./config_test.cpp
    ./controler_test.cpp
    ./local_log_repository_test.cpp
    ./log_entry_test.cpp
    ./year_overview_data_test.cpp

    ./../source/config.cpp
    ./../source/date/date.cpp
    ./../source/model/log_file.cpp
    ./../source/model/local_log_repository.cpp
    ./../source/model/year_overview_data.cpp
)

target_link_libraries(
    tests
    gtest_main
    gmock_main
    ftxui::screen
    ftxui::dom
    ftxui::component 
    fmt::fmt
    Boost::program_options
)

target_include_directories(
    tests
    PRIVATE
    ../source/
)

include(GoogleTest)
gtest_discover_tests(tests)
