/*
Filename: Src/Simulation/PhysicsModel.cpp
Description: Integration of vertical dynamics (thrust, gravity, drag) into the aircraft state.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module PhysicsModel;

import std;

namespace
{
    constexpr double kGravityMps2 = 9.81;
    constexpr double kMassKg = 1.2;
    // Thrust (N) = kThrustCoeff * rpm^2 : ~17.7 N a 1000 RPM, soit ~1.5x le poids.
    constexpr double kThrustCoeff = 1.77e-5;
    constexpr double kDragCoeff = 0.05; // Drag (N) = kDragCoeff * v * |v|
}

void PhysicsModel::Update(double dt, double thrustNewtons)
{
    const double weight = kMassKg * kGravityMps2;
    const double velocity = m_state.verticalSpeedMps;
    const double drag = kDragCoeff * velocity * std::abs(velocity);

    const double acceleration = (thrustNewtons - weight - drag) / kMassKg;

    m_state.verticalSpeedMps += acceleration * dt;
    m_state.altitudeMeters += m_state.verticalSpeedMps * dt;

    // Le sol est une contrainte simple : pas d'altitude negative.
    if (m_state.altitudeMeters < 0.0)
    {
        m_state.altitudeMeters = 0.0;
        m_state.verticalSpeedMps = 0.0;
    }
}

const AircraftState& PhysicsModel::GetState() const
{
    return m_state;
}
