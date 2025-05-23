add_executable(
  caps-log
  app.hpp
  app.cpp
  config.cpp
  config.hpp
  main.cpp
  ./log/annual_log_data.cpp
  ./log/annual_log_data.hpp
  ./log/local_log_repository.cpp
  ./log/local_log_repository.hpp
  ./log/log_file.cpp
  ./log/log_file.hpp
  ./log/log_repository_base.hpp
  ./log/log_repository_crypto_applier.cpp
  ./log/log_repository_crypto_applier.hpp
  ./utils/crypto.cpp
  ./utils/crypto.hpp
  ./utils/date.hpp
  ./utils/string.hpp
  ./utils/task_executor.hpp
  ./utils/git_repo.cpp
  ./utils/git_repo.hpp
  ./view/annual_view.cpp
  ./view/annual_view.hpp
  ./view/annual_view_base.hpp
  ./view/calendar_component.cpp
  ./view/calendar_component.hpp
  ./view/ftxui_ext/extended_containers.cpp
  ./view/ftxui_ext/extended_containers.hpp
  ./view/input_handler.hpp
  ./view/preview.cpp
  ./view/preview.hpp
  ./view/promptable.cpp
  ./view/promptable.hpp)

target_include_directories(
  caps-log 
  PRIVATE 
  ./ 
  ${CMAKE_CURRENT_BINARY_DIR}, 
  ${Boost_INCLUDE_DIRS})

target_link_libraries(
  caps-log
  PUBLIC
  ftxui::screen
  ftxui::dom
  ftxui::component
  fmt::fmt
  ${Boost_LIBRARIES}
  Boost::program_options
  OpenSSL::Crypto
  OpenSSL::SSL
  ${LIBGIT2_LIBRARIES})

target_compile_definitions(
  caps-log PRIVATE CAPS_LOG_VERSION_STRING="${CAPS_LOG_VERSION}")

install(TARGETS caps-log DESTINATION "bin")
