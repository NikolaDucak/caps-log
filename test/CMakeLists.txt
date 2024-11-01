# --------------------------------- gtest -------------------------------- #

set(INSTALL_GTEST OFF)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# --------------------------------- tests -------------------------------- #

set(TEST_SOURCES
  ./config_test.cpp
  ./controler_test.cpp
  ./local_log_repository_test.cpp
  ./log_entry_test.cpp
  ./annual_log_data_test.cpp
  ./calendar_component_test.cpp
)

set(SOURCE_FILES
  ./../source/app.cpp
  ./../source/config.cpp
  ./../source/log/log_file.cpp
  ./../source/log/log_repository_crypto_applier.cpp
  ./../source/log/local_log_repository.cpp
  ./../source/log/annual_log_data.cpp
  ./../source/view/calendar_component.cpp
  ./../source/view/ftxui_ext/extended_containers.cpp
  ./../source/utils/crypto.cpp
  ./../source/utils/git_repo.cpp
)

add_executable(tests ${TEST_SOURCES} ${SOURCE_FILES})

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
  ${LIBGIT2_LIBRARIES})

target_include_directories(tests PRIVATE
  ../source/
  ${CMAKE_CURRENT_BINARY_DIR}
  ${Boost_INCLUDE_DIRS}
  ${OPENSSL_INCLUDE_DIR}
)

target_include_directories(
  tests 
  PRIVATE
  ${Boost_INCLUDE_DIRS}
)

target_compile_definitions(
  tests PRIVATE CAPS_LOG_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/test/data")
target_compile_definitions(
  tests PRIVATE CAPS_LOG_VERSION_STRING="${CAPS_LOG_VERSION}")

include(GoogleTest)
gtest_discover_tests(tests)
