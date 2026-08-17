cmake_minimum_required(VERSION 4.1)

project(HelloWorld VERSION 1.0.0 LANGUAGES CXX)

add_executable(HelloWorld Src/main.cpp Src/InputParser.cpp)

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