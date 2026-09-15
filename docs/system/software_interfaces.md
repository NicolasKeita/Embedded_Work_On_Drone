# Contrats de données et interfaces

Les déclarations C++ sont la référence des champs et types. Cette page décrit
leur rôle et leurs unités, sans recopier les structures.

| Donnée | Rôle | Déclaration |
| --- | --- | --- |
| `AircraftState` | État de simulation ou copie construite à partir de mesures pour le contrôleur | [Aircraft](../../Src/Simulation/Aircraft.cppm) |
| `SensorTelemetry` | Mesures observables, éventuellement corrompues | [Telemetry](../../Src/SIL/Core/Telemetry/Telemetry.cppm) |
| `FlightCore::HAL::SensorData` | Mesures destinées au chemin HIL | [HalTypes](../../Src/Embedded/Hal/HalTypes.cppm) |
| `sim::control::TargetState` | Position cible | [FlightControllerTypes](../../Src/Control/Types/FlightControllerTypes.cppm) |
| `ControlCommand` | RPM et angles des servos | [Aircraft](../../Src/Simulation/Aircraft.cppm) |
| `FlightCore::HAL::ActuatorCommands` | Commandes transportées en HIL | [HalTypes](../../Src/Embedded/Hal/HalTypes.cppm) |
| `HealthReport` / `SafetyCommand` | Diagnostic / décision | [HealthMonitor](../../Src/Safety/HealthMonitor.cppm) · [SafetyManager](../../Src/Safety/SafetyManager.cppm) |

## Provenance et unités

La séparation mesure/vérité terrain dépend de la provenance de la valeur,
pas uniquement du nom du type. Aucun filtre de Kalman générique ne doit être
supposé entre `SensorData` et `AircraftState`.

Le modèle emploie les mètres, m/s et radians pour l'état. Les angles des servos
de `ControlCommand` sont en degrés ; les commandes HAL/HIL sont en radians.
Le firmware FC1 réalise la conversion au retour du contrôleur. `wing_rpm` est
une consigne en RPM, sans normalisation sur `[0, 1]`.

## Interfaces

Les modules [SensorInput](../../Src/Embedded/Hal/SensorInput.cppm),
[ActuatorOutput](../../Src/Embedded/Hal/ActuatorOutput.cppm) et
[Clock](../../Src/Embedded/Hal/Clock.cppm) exposent les abstractions HAL.
[ITransport](../../Src/Embedded/Transport/Transport.cppm) est un canal d'octets ;
[IInterFcTransport](../../Src/Embedded/InterFc/InterFcLink.cppm) échange des messages
inter-FC. Le contrôleur métier reçoit ses données de son adaptateur/exécuteur.

Le format sérialisé est défini par [HIL-Proto](../hil/hil_protocol.md) et la
[communication inter-FC](communication.md). Les états sont définis dans la
[taxonomie](../safety/fault_taxonomy.md), les règles de modules dans
[AGENTS.md](../../AGENTS.md).
