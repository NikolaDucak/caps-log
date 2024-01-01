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

# It sucks that this needs to be done, the cmake files are not that gread but
# they do the trick so lets leave it like this for now.
configure_file(../source/version.hpp.in version.hpp)

add_executable(
    tests
    ./config_test.cpp
    ./controler_test.cpp
    ./date_test.cpp
    ./local_log_repository_test.cpp
    ./log_entry_test.cpp
    ./year_overview_data_test.cpp

    ./../source/config.cpp
    ./../source/date/date.cpp
    ./../source/log/log_file.cpp
    ./../source/log/log_repository_crypto_applyer.cpp
    ./../source/log/local_log_repository.cpp
    ./../source/log/year_overview_data.cpp
    ./../source/utils/crypto.cpp
    ./../source/utils/git_repo.cpp
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
    OpenSSL::Crypto
    OpenSSL::SSL
    ${LIBGIT2_LIBRARIES}
)

target_include_directories(
    tests
    PRIVATE
    ../source/
    ${CMAKE_CURRENT_BINARY_DIR}
)

include(GoogleTest)
gtest_discover_tests(tests)
