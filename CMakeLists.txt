cmake_minimum_required(VERSION 4.1)

project(HelloWorld VERSION 1.0.0 LANGUAGES CXX)

# Sources du module de simulation (.cppm + .cpp conserves ensemble).
set(SIMULATION_FILES
    Src/Simulation/Aircraft.cppm
    Src/Simulation/Aircraft.cpp
)

# Sources du module de controle de vol (.cppm + .cpp conserves ensemble).
set(CONTROL_FILES
    Src/Control/FlightController.cppm
    Src/Control/FlightController.cpp
)

set(SRC_FILES
    Src/App/Application.cppm
    Src/App/Application.cpp
    ${SIMULATION_FILES}
    ${CONTROL_FILES}
)

add_executable(HelloWorld Src/main.cpp)
target_sources(HelloWorld
    PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
        FILES
            ${SRC_FILES}
)

# Harness et scenarios du programme de validation (.cppm + .cpp conserves ensemble).
set(TEST_SIMULATION_FILES
    Tests/TestHarness.cppm
    Tests/TestHarness.cpp
    Tests/Scenarios.cppm
    Tests/Scenarios.cpp
    Tests/ScenarioCatalog.cppm
    Tests/ScenarioCatalog.cpp
)

# Exercice de validation des scenarios A a F (etape 5).
add_executable(test_simulation Tests/test_simulation.cpp)
target_sources(test_simulation
    PRIVATE
        FILE_SET CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
        FILES
            ${SIMULATION_FILES}
            ${TEST_SIMULATION_FILES}
)

foreach(target HelloWorld test_simulation)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /external:W0
            /external:anglebrackets
        )
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    target_compile_features(${target} PRIVATE cxx_std_23)
endforeach()

install(TARGETS HelloWorld RUNTIME DESTINATION bin)