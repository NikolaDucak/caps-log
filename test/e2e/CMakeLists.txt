# --------------------------------- tests -------------------------------- #

set(TEST_SOURCES
  ./caps_log_e2e_test_fixture.hpp
  ./caps_log_e2e_cli_options_test.cpp
  ./caps_log_e2e_config_file_test.cpp
  ./caps_log_e2e_logs_basic_usecase_test.cpp
  ./caps_log_e2e_scratchpads_basic_usecase_test.cpp
)

set(SOURCE_FILES
  ./../../source/app.hpp
  ./../../source/app.cpp
  ./../../source/config.cpp
  ./../../source/config.hpp
  ./../../source/log/annual_log_data.cpp
  ./../../source/log/annual_log_data.hpp
  ./../../source/log/local_log_repository.cpp
  ./../../source/log/local_log_repository.hpp
  ./../../source/log/log_file.cpp
  ./../../source/log/log_file.hpp
  ./../../source/log/log_repository_base.hpp
  ./../../source/log/log_repository_crypto_applier.cpp
  ./../../source/log/log_repository_crypto_applier.hpp
  ./../../source/utils/crypto.cpp
  ./../../source/utils/crypto.hpp
  ./../../source/utils/date.hpp
  ./../../source/utils/git_repo.cpp
  ./../../source/utils/git_repo.hpp
  ./../../source/utils/string.hpp
  ./../../source/utils/task_executor.hpp
  ./../../source/utils/async_git_repo.hpp
  ./../../source/view/annual_view_layout.cpp
  ./../../source/view/annual_view_layout.hpp
  ./../../source/view/annual_view_layout_base.hpp
  ./../../source/view/calendar_component.cpp
  ./../../source/view/calendar_component.hpp
  ./../../source/view/ftxui_ext/extended_containers.cpp
  ./../../source/view/ftxui_ext/extended_containers.hpp
  ./../../source/view/input_handler.hpp
  ./../../source/view/preview.cpp
  ./../../source/view/preview.hpp
  ./../../source/view/scratchpad_view_layout.cpp
  ./../../source/view/scratchpad_view_layout.hpp
  ./../../source/view/scratchpad_view_layout_base.hpp
  ./../../source/view/view.cpp
  ./../../source/view/view.hpp
  ./../../source/view/view_layout_base.hpp
  ./../../source/utils/date.hpp
)

add_executable(caps_log_e2e_tests ${TEST_SOURCES} ${SOURCE_FILES})

target_link_libraries(
  caps_log_e2e_tests
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

target_include_directories(caps_log_e2e_tests PRIVATE
  ../../source/
  ${CMAKE_CURRENT_BINARY_DIR}
  ${Boost_INCLUDE_DIRS}
  ${OPENSSL_INCLUDE_DIR}
)

target_include_directories(
  caps_log_e2e_tests
  PRIVATE
  ${Boost_INCLUDE_DIRS}
)

target_compile_definitions(
  caps_log_e2e_tests PRIVATE CAPS_LOG_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/test/data")
target_compile_definitions(
  caps_log_e2e_tests PRIVATE CAPS_LOG_VERSION_STRING="${CAPS_LOG_VERSION}")

include(GoogleTest)
gtest_discover_tests(caps_log_e2e_tests)

