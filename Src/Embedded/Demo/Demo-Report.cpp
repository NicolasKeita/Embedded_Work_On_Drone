/*
Filename: Src/Embedded/Demo/Demo-Report.cpp
Description: Console report of the observable HIL lockstep step state for
validation (hil_validation.md 4.4).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module Demo;

import HalTypes;
import HilProtocol;
import std;

namespace FlightCore::Demo
{

/*
    Prints the observable state of the step for validation (hil_validation.md
    4.4): decoded sensor values, FC commands, echoed timestamp and measured RTT.
*/
void printReport(std::uint64_t                        sim_us,
                 std::uint16_t                        sequence,
                 const HAL::SensorData&               tx_sensor,
                 const HAL::SensorData&               fc_sensor,
                 const HAL::ActuatorCommands&         cmds,
                 const Transport::HilActuatorPayload& actuator,
                 std::uint64_t                        pc_rx_us,
                 std::uint64_t                        rejected)
{
    const std::int64_t rtt_us = static_cast<std::int64_t>(pc_rx_us)
                              - static_cast<std::int64_t>(actuator.echo_sim_timestamp_us);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "==== HIL lockstep step-closure ====" << "\n";
    std::cout << "sim_timestamp_us (PC master) : " << sim_us << "\n";
    std::cout << "sequence_num               : " << sequence << "\n";
    std::cout << "TX sensor alt_baro (m)     : " << tx_sensor.altitude_baro_m << "\n";
    std::cout << "RX decoded alt_baro (m)     : " << fc_sensor.altitude_baro_m << "\n";
    std::cout << "RX decoded wing_rpm_meas   : " << fc_sensor.wing_rpm_meas << "\n";
    std::cout << "FC wing_rpm_cmd             : " << cmds.wing_rpm_cmd << "\n";
    std::cout << "FC left_servo_rad           : " << cmds.left_servo_rad << "\n";
    std::cout << "FC right_servo_rad          : " << cmds.right_servo_rad << "\n";
    std::cout << "FC mode_flags              : " << static_cast<std::uint32_t>(cmds.mode_flags) << "\n";
    std::cout << "ActuatorPacket echo_sim_ts  : " << actuator.echo_sim_timestamp_us << "\n";
    std::cout << "ActuatorPacket fc_mode      : " << static_cast<std::uint32_t>(actuator.fc_mode) << "\n";
    std::cout << "ActuatorPacket health       : " << static_cast<std::uint32_t>(actuator.fc_health_status) << "\n";
    std::cout << "Round-trip time (us)        : " << rtt_us << "\n";
    std::cout << "Rejected frames by parser    : " << rejected << "\n";
}

}
