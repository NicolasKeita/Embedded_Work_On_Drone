/*
Filename: Src/SIL/FaultInjectorFactory.cppm
Description: Single factory turning declarative fault scenarios into polymorphic injectors.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module FaultInjectorFactory;

import std;

import IFaultInjector;
import SilTypes;

export namespace sim::sil {

[[nodiscard]] std::unique_ptr<IFaultInjector> make_fault_injector(const FaultScenario& scenario);

}
