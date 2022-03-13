add_executable(
    clog
    main.cpp
    ./view/yearly_view.cpp
    ./view/yearly_view.hpp
    ./view/highlight_menu.cpp
    ./view/highlight_menu.hpp
    ./view/calendar_component.hpp
    ./view/calendar_component.cpp
    ./view/highlight_menu.hpp
    ./view/highlight_menu.cpp
    ./view/ftxui_ext/extended_containers.hpp
    ./view/ftxui_ext/extended_containers.cpp
    ./model/Date.cpp
    ./model/Date.hpp
    ./model/DefaultLogRepository.cpp
    ./model/DefaultLogRepository.hpp
    ./model/LogRepository.hpp
    ./model/LogFile.cpp
    ./model/LogFile.hpp
    ./model/LogModel.cpp
    ./model/LogModel.hpp
)

target_link_libraries(clog
    PRIVATE ftxui::screen
    PRIVATE ftxui::dom
    PRIVATE ftxui::component # Not needed for this example.
)
