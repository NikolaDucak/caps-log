configure_file(version.hpp.in version.hpp)

add_executable(
    caps-log

    main.cpp
    config.cpp
    config.hpp

    ./date/date.cpp
    ./date/date.hpp

    ./editor/disabled_editor.hpp
    ./editor/editor_base.hpp
    ./editor/editor_opener.hpp
    ./editor/user_set_editor.cpp
    ./editor/user_set_editor.hpp

    ./model/local_log_repository.cpp
    ./model/local_log_repository.hpp
    ./model/log_file.cpp
    ./model/log_file.hpp
    ./model/log_repository_base.hpp
    ./model/year_overview_data.cpp
    ./model/year_overview_data.hpp

    ./utils/crypto.cpp
    ./utils/crypto.hpp
    ./utils/map.hpp
    ./utils/string.hpp

    ./view/calendar_component.cpp
    ./view/calendar_component.hpp
    ./view/ftxui_ext/extended_containers.cpp
    ./view/ftxui_ext/extended_containers.hpp
    ./view/input_handler.hpp
    ./view/preview.cpp
    ./view/preview.hpp
    ./view/promptable.cpp
    ./view/promptable.hpp
    ./view/yearly_view.cpp
    ./view/yearly_view.hpp
)

target_include_directories(caps-log PRIVATE ./ ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(caps-log
    Boost::program_options
    OpenSSL::Crypto
    OpenSSL::SSL
    fmt::fmt
    ftxui::component 
    ftxui::dom
    ftxui::screen
)

if(CAPS_LOG_SNAP_BUILD) 
    target_compile_definitions(caps-log PUBLIC CAPS_LOG_SNAP_BUILD)
endif()

install(TARGETS caps-log DESTINATION "bin")
