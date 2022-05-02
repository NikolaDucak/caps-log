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
