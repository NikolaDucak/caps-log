cmake_minimum_required(VERSION 3.14)

project(clog
    LANGUAGES CXX
    VERSION 1.0.0
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# ------------------------------ Fetch FTXUI ----------------------------- #
 
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui)
FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
    FetchContent_Populate(ftxui)
    add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

if(${BUILD_TESTS})
    message("Tests will be built")
    add_subdirectory(./test)
endif()

# ---------------------------------- clog -------------------------------- #

add_subdirectory(./source)

# -------------------------------- clog-web ------------------------------ #

if (EMSCRIPTEN) 
    message(STATUS "WASM build enabled")
    target_compile_definitions(clog PUBLIC WASM_BUILD)
    target_compile_options(clog PUBLIC -fexceptions -sUSE_PTHREADS=1)
    target_link_options(clog PUBLIC -fexceptions -sUSE_PTHREADS=1 -sPROXY_TO_PTHREAD -sASYNCIFY)

    foreach(file "index.html" "run_webassembly.py")
        configure_file("web/${file}" ${file})
    endforeach(file)

    em_link_js_library(clog "web/lib.js")
    set_target_properties(clog_1_lib_js PROPERTIES LINKER_LANGUAGE CXX) # ... cmake complains without this
else()
    install(TARGETS clog RUNTIME DESTINATION bin)
endif()
