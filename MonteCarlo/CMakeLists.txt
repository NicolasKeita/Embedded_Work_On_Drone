cmake_minimum_required(VERSION 4.1)

project(MonteCarlo VERSION 1.0.0 LANGUAGES CXX)

# Sources du moteur Monte-Carlo (.cppm + .cpp conserves ensemble).
set(SRC_FILES
    MonteCarlo.cppm
    MonteCarlo.cpp
)

add_executable(monte_carlo main.cpp)
target_sources(monte_carlo
    PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
        FILES
            ${SRC_FILES}
)

if(MSVC)
    target_compile_options(monte_carlo PRIVATE
        /W4
        /external:W0
        /external:anglebrackets
    )
else()
    target_compile_options(monte_carlo PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_compile_features(monte_carlo PRIVATE cxx_std_23)

install(TARGETS monte_carlo RUNTIME DESTINATION bin)