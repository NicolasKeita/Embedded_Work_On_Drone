cmake_minimum_required(VERSION 4.4)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
    "f35a9ac6-8463-4d38-8eec-5d6008153e7d"
    CACHE STRING "Enable experimental C++ import std support")
project(fc1_stm32_bootstrap LANGUAGES NONE)

set(FIRMWARE_NAME fc1_stm32)
set(FIRMWARE_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
set(FIRMWARE_CORE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Simulation/Aircraft.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Simulation/PhysicsDispersion.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Control/FlightController.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Control/FlightController-Core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Control/FlightController-Loops.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Control/FlightController-Mission.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Control/FlightController-Takeoff.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Control/Types/FlightControllerTypes.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Control/Types/FlightControllerTypes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Embedded/Hal/HalTypes.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Embedded/InterFc/InterFcLink.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Embedded/InterFc/InterFcLink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Embedded/InterFc/ZephyrUartInterFcTransport.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Embedded/InterFc/ZephyrUartInterFcTransport.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Embedded/Transport/HilProtocol.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Embedded/Transport/Parser/HilProtocolParser.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Embedded/Transport/Parser/HilProtocolParser-Crc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Embedded/Transport/Parser/HilProtocolParser-Sync.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Embedded/Transport/Parser/HilProtocolParser-Frame.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Embedded/Transport/Codec/HilProtocolCodec.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Embedded/Transport/Codec/HilProtocolCodec-Sensor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Embedded/Transport/Codec/HilProtocolCodec-Actuator.cpp
)

include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/ZephyrFirmware.cmake")
