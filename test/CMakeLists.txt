enable_testing()

add_executable(
    tests
    ./controler_test.cpp
    ./../source/model/Date.cpp
    ./../source/model/LogFile.cpp
    ./../source/model/DefaultLogRepository.cpp
)

target_link_libraries(
    tests
    gtest_main
    gmock_main
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
