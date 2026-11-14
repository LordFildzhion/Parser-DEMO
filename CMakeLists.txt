cmake_minimum_required(VERSION 3.16)

project(Analyzer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)
set(CMAKE_AUTOMOC ON)

function(ifelse_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(ifelse_core STATIC
    src/core/diagnostic.h
    src/core/editordocument.h
    src/core/editordocument.cpp
)
target_compile_features(ifelse_core PUBLIC cxx_std_17)
target_include_directories(ifelse_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(ifelse_core PUBLIC Qt6::Gui)
ifelse_enable_warnings(ifelse_core)

add_library(ifelse_gui STATIC
    src/gui/mainwindow.h
    src/gui/mainwindow.cpp
    src/gui/codeeditor.h
    src/gui/codeeditor.cpp
    src/gui/syntaxhighlighter.h
    src/gui/syntaxhighlighter.cpp
    src/gui/diagnosticspanel.h
    src/gui/diagnosticspanel.cpp
    src/gui/helpcontent.h
    src/gui/helpcontent.cpp
)
target_link_libraries(ifelse_gui PUBLIC ifelse_core Qt6::Widgets)
ifelse_enable_warnings(ifelse_gui)

configure_file(src/version.h.in generated/version.h @ONLY)
qt_add_executable(Analyzer src/main.cpp)
target_include_directories(Analyzer PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(Analyzer PRIVATE ifelse_gui)
ifelse_enable_warnings(Analyzer)

set_target_properties(Analyzer PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

install(TARGETS Analyzer
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.2 REQUIRED COMPONENTS Test)

    add_executable(test_document tests/test_document.cpp)
    target_link_libraries(test_document PRIVATE ifelse_core Qt6::Test)
    ifelse_enable_warnings(test_document)
    add_test(NAME document COMMAND test_document)

    add_executable(test_gui tests/test_gui.cpp)
    target_link_libraries(test_gui PRIVATE ifelse_gui Qt6::Test)
    ifelse_enable_warnings(test_gui)
    add_test(NAME gui COMMAND test_gui)

    set_tests_properties(document gui PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 30
    )
endif()
