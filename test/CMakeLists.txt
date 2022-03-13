enable_testing()

add_executable(
    all_tests
    ./model/date_test.cpp
    ./model/log_file_test.cpp
    ./model/log_model_test.cpp

    # TODO: move to a library
    ../source/model/date.cpp
    ../source/model/log_file.cpp
    ../source/model/log_model.cpp
)

target_link_libraries(
    all_tests
    gtest_main
    stdc++fs
)

target_include_directories(
    all_tests
    PRIVATE
    ../source/
)

include(GoogleTest)
gtest_discover_tests(all_tests)
