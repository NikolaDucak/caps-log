add_executable(
    clog

    main.cpp
    ./view/calendar_component.cpp
    ./view/calendar_component.hpp
    ./view/input_handler.hpp
    ./view/promptable.cpp
    ./view/promptable.hpp
    ./view/yearly_view.cpp
    ./view/yearly_view.hpp
    ./view/ftxui_ext/extended_containers.cpp
    ./view/ftxui_ext/extended_containers.hpp
    ./model/date.cpp
    ./model/date.hpp
    ./model/default_log_repository.cpp
    ./model/default_log_repository.hpp
    ./model/log_repository_base.hpp
    ./model/log_file.cpp
    ./model/log_file.hpp
)

target_link_libraries(clog
    ftxui::screen
    ftxui::dom
    ftxui::component 
)
