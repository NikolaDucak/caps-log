configure_file(version.hpp.in version.hpp)

add_executable(
    clog

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

    ./model/local_log_repository.cpp
    ./model/local_log_repository.hpp
    ./model/log_repository_base.hpp
    ./model/log_file.cpp
    ./model/log_file.hpp
    ./model/year_overview_data.cpp
    ./model/year_overview_data.hpp

    ./date/date.cpp
    ./date/date.hpp

    ./utils/string.hpp
    ./utils/map.hpp
)

target_include_directories(clog PRIVATE ./ ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(clog
    ftxui::screen
    ftxui::dom
    ftxui::component 
    fmt::fmt
    Boost::program_options
)

install(TARGETS clog DESTINATION "bin")
