configure_file(version.hpp.in version.hpp)

add_executable(
    caps-log

    main.cpp
    config.cpp
    config.hpp

    ./view/calendar_component.cpp
    ./view/calendar_component.hpp
    ./view/input_handler.hpp
    ./view/promptable.cpp
    ./view/promptable.hpp
    ./view/preview.cpp
    ./view/preview.hpp
    ./view/yearly_view.cpp
    ./view/yearly_view.hpp
    ./view/ftxui_ext/extended_containers.cpp
    ./view/ftxui_ext/extended_containers.hpp

    ./log/local_log_repository.cpp
    ./log/local_log_repository.hpp
    ./log/log_file.cpp
    ./log/log_file.hpp
    ./log/log_repository_base.hpp
    ./log/log_repository_crypto_applyer.cpp
    ./log/log_repository_crypto_applyer.hpp
    ./log/year_overview_data.cpp
    ./log/year_overview_data.hpp

    ./date/date.cpp
    ./date/date.hpp

    ./utils/crypto.cpp
    ./utils/crypto.hpp
    ./utils/string.hpp
    ./utils/map.hpp
)

target_include_directories(caps-log PRIVATE ./ ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(caps-log
    ftxui::screen
    ftxui::dom
    ftxui::component 
    fmt::fmt
    Boost::program_options
    OpenSSL::Crypto
    OpenSSL::SSL
)

install(TARGETS caps-log DESTINATION "bin")
