cmake_minimum_required(VERSION 4.4)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
    "f35a9ac6-8463-4d38-8eec-5d6008153e7d"
    CACHE STRING "Enable experimental C++ import std support")
project(fc2_stm32_bootstrap LANGUAGES NONE)

set(FIRMWARE_NAME fc2_stm32)
set(FIRMWARE_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
set(FIRMWARE_CORE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Simulation/Aircraft.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Simulation/PhysicsDispersion.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/SIL/Core/Comms/CommsBus.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/SIL/Core/Telemetry/Telemetry.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/SIL/Core/Telemetry/Telemetry-Chain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/SIL/Core/Telemetry/Telemetry-Validation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Safety/HealthMonitor.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Safety/HealthMonitor-Core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Safety/HealthMonitor-Evaluate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Safety/HealthMonitor-Report.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Safety/HealthMonitor-Names.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Safety/SafetyManager.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Safety/SafetyManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Embedded/Hal/HalTypes.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Embedded/InterFc/InterFcLink.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Embedded/InterFc/InterFcLink-Codec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Embedded/InterFc/InterFcLink-Crc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/Embedded/InterFc/InterFcLink-Parser.cpp
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
