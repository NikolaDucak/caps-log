enable_testing()

add_executable(
    tests
    ./ui/calendar_component_test.cpp
)

target_link_libraries(
    tests
    gtest_main
    ftxui::screen
    ftxui::dom
    ftxui::component 
)

target_include_directories(
    tests
    PRIVATE
    ../source/
)

include(GoogleTest)
gtest_discover_tests(tests)
