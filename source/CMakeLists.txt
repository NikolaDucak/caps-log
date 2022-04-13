add_executable(
    clog
    main.cpp
    ./view/yearly_view.cpp
    ./view/yearly_view.hpp
    ./view/highlight_menu.hpp
    ./view/calendar_component.hpp
    ./view/calendar_component.cpp
    ./view/ftxui_ext/extended_containers.hpp
    ./view/ftxui_ext/extended_containers.cpp
    ./model/Date.cpp
    ./model/Date.hpp
    ./model/DefaultLogRepository.cpp
    ./model/DefaultLogRepository.hpp
    ./model/LogRepositoryBase.hpp
    ./model/LogFile.cpp
    ./model/LogFile.hpp
)

target_link_libraries(clog
    PRIVATE ftxui::screen
    PRIVATE ftxui::dom
    PRIVATE ftxui::component 
)
