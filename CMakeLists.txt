cmake_minimum_required(VERSION 4.1)

project(HelloWorld VERSION 1.0.0 LANGUAGES CXX)

set(SRC_FILES
    Src/InputParser.cppm
    Src/InputParser.cpp
    Src/App/Application.cppm
    Src/App/Application.cpp
)
add_executable(HelloWorld Src/main.cpp)
target_sources(HelloWorld
    PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
        FILES
            ${SRC_FILES}
)

if(MSVC)
    target_compile_options(HelloWorld PRIVATE
        /W4
        /external:W0
        /external:anglebrackets
    )
else()
    target_compile_options(HelloWorld PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_compile_features(HelloWorld PRIVATE cxx_std_23)

install(TARGETS HelloWorld RUNTIME DESTINATION bin)