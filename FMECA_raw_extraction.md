# Extraction des erreurs et comportements logiciels pour FMECA

**Périmètre analysé :** l'intégralité du dépôt (219 fichiers C++23 `.cppm`/`.cpp` sous `Src/` et `Tests/`, plus `docs/system/fault_handling.md`). Analyse statique ligne par ligne, sans compilation. Chaque élément est référencé `chemin:ligne`. Aucun comportement n'est extrapolé au-delà du code présent.

**Architecture de sécurité identifiée :** deux calculateurs — **FC1** (contrôle de vol, `Src/Control/FlightController*`) et **FC2** (supervision : `Src/Safety/HealthMonitor*` + `Src/Safety/SafetyManager*`). Chaîne de traitement d'une défaillance (`docs/system/fault_handling.md:24-28`) : `FAILURE → DETECTION → DIAGNOSIS → RESPONSE → RECOVERY / SAFE MODE`. Le moteur SIL (`Src/SIL/Runner/`) et le runner HIL (`Src/Embedded/Hil/Runner/`) réutilisent **les mêmes** noyaux `HealthMonitor`/`SafetyManager`, ce qui garantit une détection identique en simulation et sur cible hôte.

---

## 1. Inventaire des codes d'erreur et pannes déclarés

### 1.1 États de santé et modes de sécurité (cœur embarqué FC2)

| Module / Fichier | Identifiant / Enum | Description / Signification | Condition de déclenchement (Code) |
| :--- | :--- | :--- | :--- |
| `Src/Safety/HealthMonitor.cppm:28` | `enum class HealthState { HEALTHY, DEGRADED, SAFE, FAILED }` | État de santé global du système, hiérarchisé | Calculé par `HealthMonitor::compute_state()` (`HealthMonitor-Core.cpp:41-55`) |
| `Src/Safety/HealthMonitor.cppm:28` | `HealthState::HEALTHY` | Fonctionnement nominal | Aucun drapeau de faute levé (`HealthMonitor-Core.cpp:54`) |
| `Src/Safety/HealthMonitor.cppm:28` | `HealthState::DEGRADED` | Anomalie sous-système secondaire, contrôlabilité conservée | Drapeau `Sensor` **ou** `Actuator` levé, sans faute critique (`HealthMonitor-Core.cpp:49-53`) |
| `Src/Safety/HealthMonitor.cppm:28` | `HealthState::SAFE` | Défaillance majeure → maintien dans un état physique sûr | Drapeau `FC1Heartbeat` **ou** `Communication` levé (`HealthMonitor-Core.cpp:43-48`) |
| `Src/Safety/HealthMonitor.cppm:28` | `HealthState::FAILED` | Perte d'intégrité totale (indicateur ultime de diagnostic) | **Déclaré mais jamais produit** par `compute_state()` (valeur morte dans le code actuel) |
| `Src/Safety/HealthMonitor.cppm:30` | `enum class FaultDomain { FC1Heartbeat, Communication, Sensor, Actuator }` | Domaines de faute supervisés par FC2 (4 drapeaux `FaultFlag{raised, raised_time}`, `:32-35`) | Voir lignes suivantes |
| `Src/Safety/HealthMonitor-Names.cpp:19` | `"FC1_HEARTBEAT_TIMEOUT"` | Absence de heartbeat FC1 au-delà du timeout | `current_time - comms.last_received_time() > heartbeat_timeout_s` (`HealthMonitor-Evaluate.cpp:29-32`) — drapeau **latched** (seul un reset le lève, commentaire `:20-22`) |
| `Src/Safety/HealthMonitor-Names.cpp:21` | `"COMMUNICATION_LOST"` | Rupture physique du lien inter-calculateurs | `!comms.link_up()` (`HealthMonitor-Evaluate.cpp:25-27`) — drapeau **latched** |
| `Src/Safety/HealthMonitor-Names.cpp:23` | `"SENSOR_INVALID"` | Télémétrie capteur hors plage ou NaN | `!validate(telemetry, sensor_limits).all_valid()` (`HealthMonitor-Evaluate.cpp:43-44`) — drapeau **auto-nettoyant** dès que la télémétrie redevient valide (`:46-48`) |
| `Src/Safety/HealthMonitor-Names.cpp:25` | `"ACTUATOR_MISMATCH"` | Écart consigne/mesure RPM soutenu | `commanded_rpm > 0 && |commanded_rpm - actual_rpm| > actuator_mismatch_rpm` maintenu `>= actuator_mismatch_hold_s` (`HealthMonitor-Evaluate.cpp:59-70`) — auto-nettoyant (`:72-74`) |
| `Src/Safety/SafetyManager.cppm:23` | `enum class SafetyMode { NORMAL, COMPENSATED, SAFE_MODE }` | Mode de commande de sécurité émis par FC2 | `SafetyManager::update()` (`SafetyManager.cpp:57-72`) |
| `Src/Safety/SafetyManager.cppm:23` | `SafetyMode::NORMAL` | Commande FC1 nominale | `report.state == HEALTHY` et jamais entré en SAFE_MODE (`SafetyManager.cpp:67-69`) |
| `Src/Safety/SafetyManager.cppm:23` | `SafetyMode::COMPENSATED` | Mode dégradé : marge de poussée appliquée | `report.state == DEGRADED` ; marge = `degraded_thrust_margin` (1.7) **uniquement** si drapeau `Actuator` levé, sinon 1.0 (`SafetyManager.cpp:63-65`) |
| `Src/Safety/SafetyManager.cppm:23` | `SafetyMode::SAFE_MODE` | Mode conservateur : abandon de mission + descente contrôlée. **Transition irréversible** | `report.state == SAFE` (`SafetyManager.cpp:59-61`) ; la garde `mode_ != SAFE_MODE` (`:62`) interdit tout retour arrière |
| `Src/Safety/SafetyManager.cppm:25-28` | `struct SafetyCommand { mission_abort, thrust_margin }` | Consigne de sécurité : abandon mission + facteur de marge de poussée | Émis par `engage()` (`SafetyManager.cpp:42-49`) |
| `Src/Control/Types/FlightControllerTypes.cppm:28-35` | `enum class MissionState { TAKEOFF, CLIMB, STATION_KEEPING, COMPLETE, ABORTED, FAILED }` | Machine à états de mission du contrôleur | Transitions dans `FlightController-Mission.cpp` |
| `Src/Control/Types/FlightControllerTypes.cppm:33` | `MissionState::ABORTED` | Mission abandonnée (état terminal posé par le moteur SIL sur abort sécurité) | `mission_abort_recorded` posé quand `SAFE_MODE` atteint (`SilRunner-Run.cpp:84-89`) puis `final_state = ABORTED` (`SilRunner-Finalize.cpp:33-35`) ; le contrôleur répond par une **commande nulle** (`FlightController-Mission.cpp:91-93`) |
| `Src/Control/Types/FlightControllerTypes.cppm:34` | `MissionState::FAILED` | Fenêtre de mission épuisée sans succès (état terminal SIL) | `final_state != COMPLETE` et pas d'abort → `FAILED` (`SilRunner-Finalize.cpp:36-38`) ; commande nulle (`FlightController-Mission.cpp:91-93`) |

### 1.2 Vocabulaire des pannes injectables (SIL / HIL)

| Module / Fichier | Identifiant / Enum | Description / Signification | Condition de déclenchement (Code) |
| :--- | :--- | :--- | :--- |
| `Src/SIL/Core/SilFaultScenario.cppm:33-40` | `enum class FaultType { None, FC1Failure, CommunicationLoss, CommunicationLossRate, SensorFault, ActuatorDegradation }` | Familles de pannes modélisées (les 4 pannes représentatives de `docs/system/fault_handling.md:12-19` + variante à taux) | Injection via `FaultInjector::inject()` (`FaultInjectors-Base.cpp:43-65`) |
| `Src/SIL/Core/SilFaultScenario.cppm:35` | `FaultType::FC1Failure` | Crash / silence de FC1 (arrêt heartbeat + statut) | `state.fc1_alive = false` (`FaultInjectors-Injectors.cpp:20-23`) |
| `Src/SIL/Core/SilFaultScenario.cppm:36` | `FaultType::CommunicationLoss` | Coupure totale du bus inter-FC | `state.comms_link_up = false` (`FaultInjectors-Injectors.cpp:30-31`) |
| `Src/SIL/Core/SilFaultScenario.cppm:37` | `FaultType::CommunicationLossRate` | Pertes aléatoires de paquets à taux configuré | `state.comms_loss_probability = loss_probability` (`FaultInjectors-Injectors.cpp:33-34`) ; tirage `uniform_real_distribution{0,1} < loss_probability_` (`CommsBus.cpp:76-80`) |
| `Src/SIL/Core/SilFaultScenario.cppm:38` | `FaultType::SensorFault` | Corruption de la télémétrie capteur | `state.sensor_corruption = ...` (`FaultInjectors-Injectors.cpp:42-46`) |
| `Src/SIL/Core/SilFaultScenario.cppm:39` | `FaultType::ActuatorDegradation` | Perte d'efficacité actionneur (ex. blocage mécanique partiel) | `state.actuator_efficiency = efficiency` (`FaultInjectors-Injectors.cpp:51-54`), appliqué en facteur sur le RPM (`SilRunner-Loop.cpp:70`) |
| `Src/SIL/Core/SilFaultScenario.cppm:43-54` | `enum class FaultTarget { Unspecified, ActuatorMainRotor, ActuatorLeftServo, ActuatorRightServo, SensorBarometer, SensorImu, SensorGnss, SensorRpmFeedback, ProcessorFc1, LinkFc1Fc2 }` | Composant ciblé (identifiant canonique pour les logs) | Résolu par `effective_fault_target()` (défauts par famille : processeur FC1, liaison FC1-FC2, canal altitude, rotor principal — `SilFaultScenario.cppm:99-104`) |
| `Src/SIL/Core/SilFaultScenario.cppm:57` | `enum class FaultProfile { Permanent, Temporary }` | Temporalité de la fenêtre d'activation | `duration <= 0.0` → permanent, actif jusqu'à la fin du run (`FaultInjectors-Base.cpp:24-33`) |
| `Src/SIL/Core/SilFaultScenario.cppm:64` | `enum class FaultValueKind { None, Efficiency, LossProbability, Altitude, AltitudeNoise }` | Sémantique du paramètre numérique de la faute | `write_fault_parameters()` (`SilRunnerEvents-FaultInfo.cpp:44-73`) |
| `Src/SIL/Core/SilFaultScenario.cppm:66-71` | `struct FaultParameters { loss_probability=0.0, corruption=None, corrupted_altitude_m=99999.0, efficiency=1.0 }` | Paramètres d'injection, validés à la construction de l'injecteur | `make_fault_injector()` (`FaultInjectors-Factory.cpp:23-48`) |
| `Src/SIL/Core/Telemetry/Telemetry.cppm:27` | `enum class SensorCorruptionMode { None, AltitudeNaN, AltitudeOutOfRange, ExtremeNoise }` | Modes de corruption capteur côté environnement | `apply_corruption()` (`Telemetry-Validation.cpp:36-56`) |
| `Src/SIL/Core/Telemetry/Telemetry.cppm:27` | `SensorCorruptionMode::AltitudeNaN` | Altitude forcée à NaN | `corrupted.z = quiet_NaN()` (`Telemetry-Validation.cpp:43-44`) |
| `Src/SIL/Core/Telemetry/Telemetry.cppm:27` | `SensorCorruptionMode::AltitudeOutOfRange` | Altitude forcée hors plage physique | `corrupted.z = corrupted_altitude_m` (défaut 99999 m) (`Telemetry-Validation.cpp:46-47`) |
| `Src/SIL/Core/Telemetry/Telemetry.cppm:27` | `SensorCorruptionMode::ExtremeNoise` | Bruit déterministe d'amplitude 300 m superposé | `corrupted.z += 300.0 * sin(97.0 * z)` (`Telemetry-Validation.cpp:49-50` ; constante `kExtremeNoiseAmplitudeM = 300.0` à `Telemetry.cppm:30`) |
| `Src/SIL/Core/Telemetry/Telemetry.cppm:57-62` | `struct SensorValidity { altitude_valid, position_valid }` | Résultat de la validation de plage capteur | `validate()` (`Telemetry-Validation.cpp:21-29`) |

### 1.3 Erreurs typées des runners et usines (`std::expected`, zéro exception)

| Module / Fichier | Identifiant / Enum | Description / Signification | Condition de déclenchement (Code) |
| :--- | :--- | :--- | :--- |
| `Src/SIL/Runner/Context/SilRunnerContext.cppm:35` | `enum class SilError { TooManyScenarios, FaultScenarioRejected }` | Échecs typés d'un run SIL (aucune exception n'est jamais levée, `:34`) | Retourné via `std::expected<SilRunOutput, SilError>` (`SilRunner.cppm:29`) |
| `Src/SIL/Runner/Context/SilRunnerContext.cppm:35` | `SilError::TooManyScenarios` | Plus de scénarios de faute que la capacité fixe | `scenarios.size() > kMaxFaultInjectors` (= 8, `SilRunnerContext.cppm:32`) dans `SilRunner-Context.cpp:39-41` |
| `Src/SIL/Runner/Context/SilRunnerContext.cppm:35` | `SilError::FaultScenarioRejected` | Un scénario a été rejeté par l'usine d'injecteurs | `make_fault_injector` en erreur ≠ `NoFault` (`SilRunner-Context.cpp:46-52`) |
| `Src/SIL/Faults/FaultInjectors.cppm:26-32` | `enum class InjectorError { NoFault, UnknownFaultType, InvalidLossProbability, InvalidSensorCorruption, InvalidEfficiency }` | Raisons typées de rejet d'un scénario déclaratif | `make_fault_injector()` (`FaultInjectors-Factory.cpp:23-48`) |
| `Src/SIL/Faults/FaultInjectors.cppm:27` | `InjectorError::NoFault` | Scénario nominal (issue bénigne, ignorée par l'appelant) | `fault_type == None` (`FaultInjectors-Factory.cpp:26-27`) |
| `Src/SIL/Faults/FaultInjectors.cppm:28` | `InjectorError::UnknownFaultType` | Type de faute hors enum | `default` du switch (`FaultInjectors-Factory.cpp:47`) |
| `Src/SIL/Faults/FaultInjectors.cppm:29` | `InjectorError::InvalidLossProbability` | Probabilité de perte hors [0, 1] | `loss_probability < 0.0 \|\| > 1.0` (`FaultInjectors-Factory.cpp:32-33`) |
| `Src/SIL/Faults/FaultInjectors.cppm:30` | `InjectorError::InvalidSensorCorruption` | Faute capteur sans mode de corruption | `corruption == SensorCorruptionMode::None` (`FaultInjectors-Factory.cpp:37-38`) |
| `Src/SIL/Faults/FaultInjectors.cppm:31` | `InjectorError::InvalidEfficiency` | Efficacité actionneur hors (0, 1] | `efficiency <= 0.0 \|\| > 1.0` (`FaultInjectors-Factory.cpp:42-43`) |
| `Src/Embedded/Hil/Runner/Context/HilRunnerTypes.cppm:29-33` | `enum class HilError { TooManyScenarios, FaultScenarioRejected, InvalidConfiguration }` | Échecs typés d'un run HIL | `std::expected<HilRunOutput, HilError>` (`HilRunner.cppm:45`) |
| `Src/Embedded/Hil/Runner/Context/HilRunnerTypes.cppm:30` | `HilError::TooManyScenarios` | Plus de 8 scénarios (`kHilMaxFaultScenarios = 8`, `:37`) | `HilRunner-Context.cpp:41-43` |
| `Src/Embedded/Hil/Runner/Context/HilRunnerTypes.cppm:31` | `HilError::FaultScenarioRejected` | `make_fault_injector` (SIL) a rejeté le scénario | `HilRunner-Context.cpp:51-55` |
| `Src/Embedded/Hil/Runner/Context/HilRunnerTypes.cppm:32` | `HilError::InvalidConfiguration` | **Déclarée mais jamais retournée** (valeur morte) | aucun chemin de code |
| `Src/SIL/Reporting/Report/SilReportingReport.cppm:32` | `enum class ReportError { DirectoryCreation, FileOpen }` | Échecs d'écriture des artefacts de rapport | `std::expected<void, ReportError>` (`SilReporting.cppm:52`) |
| `Src/SIL/Reporting/Report/SilReportingReport.cppm:32` | `ReportError::DirectoryCreation` | Création de l'arborescence de sortie impossible | `std::filesystem::create_directories` positionne `ec` (`SilReporting-File.cpp:112-115`) |
| `Src/SIL/Reporting/Report/SilReportingReport.cppm:32` | `ReportError::FileOpen` | Ouverture d'un fichier artefact impossible | `!file.is_open()` sur `std::ofstream` (`SilReporting-File.cpp:27-29` et `:43-45`) |

### 1.4 Erreurs de transport HIL (liaison simulateur ↔ cible FC)

| Module / Fichier | Identifiant / Enum | Description / Signification | Condition de déclenchement (Code) |
| :--- | :--- | :--- | :--- |
| `Src/Embedded/Hil/Transport/HilTransport.cppm:31-39` | `enum class ReceiveResult { Ok, Timeout, SequenceError, MessageTypeError, EchoMismatch, InvalidPayload, SendFailed }` | Issue d'une réception d'`ActuatorPacket` | `HilTransport::receiveActuator()` / `accept_frame()` |
| `Src/Embedded/Hil/Transport/HilTransport.cppm:33` | `ReceiveResult::Timeout` | Deadline wall-clock atteinte sans trame valide | `clock.nowUs() >= deadline_wall_us` (`HilTransport-Receive.cpp:63-66`) → `stats_.record_timeout()` |
| `Src/Embedded/Hil/Transport/HilTransport.cppm:34` | `ReceiveResult::SequenceError` | `sequence_num` ≠ séquence attendue (désordre/perte) | `header.sequence_num != expected_sequence` (`HilTransport-Accept.cpp:49-52`) → `record_sequence_error()` |
| `Src/Embedded/Hil/Transport/HilTransport.cppm:35` | `ReceiveResult::MessageTypeError` | **Jamais retournée** : le cas `msg_id` ≠ actuator est couvert par `FrameAcceptanceError::NotActuatorFrame`, absorbé par la boucle de réception (valeur morte côté production ; seule une chaîne de log existe, `HilRunnerEvents-Steps.cpp:27-28`) | aucun chemin de code |
| `Src/Embedded/Hil/Transport/HilTransport.cppm:36` | `ReceiveResult::EchoMismatch` | `echo_sim_timestamp_us` ≠ timestamp sim envoyé (paquet « stale » d'un cycle précédent) | `payload.echo_sim_timestamp_us != expected_echo_sim_us` (`HilTransport-Accept.cpp:63-66`) → `record_stale()` |
| `Src/Embedded/Hil/Transport/HilTransport.cppm:37` | `ReceiveResult::InvalidPayload` | Payload trop courte ou décodage impossible | `header.payload_len < kActuatorPayloadSize` (`HilTransport-Accept.cpp:53-56`) ou échec `decodeActuatorPayload` (`:59-62`) |
| `Src/Embedded/Hil/Transport/HilTransport.cppm:38` | `ReceiveResult::SendFailed` | Échec d'émission du `SensorPacket` (canal plein) | `!ctx.transport.sendSensor(...)` (`HilRunner-Step.cpp:83-86`) |
| `Src/Embedded/Hil/Transport/HilTransport.cppm:25-27` | `enum class FrameAcceptanceError : std::uint8_t { NotActuatorFrame }` | Rejet de trame non-actionneur dans `accept_frame` | `header.msg_id != kMsgIdActuator` (`HilTransport-Accept.cpp:45`) ; absorbé silencieusement, compté en `messages_dropped` |
| `Src/Embedded/Transport/Parser/HilProtocolParser.cppm:39-46` | `enum class State : std::uint8_t { Sync1, Sync2, Header, ReadPayload, ReadCrc }` | FSM du parseur de trames byte-par-byte, sans heap | `HilProtocolParser-Frame.cpp` / `-Sync.cpp` |
| `Src/Embedded/Transport/Parser/HilProtocolParser-Crc.cpp:3` | `HilCrc` (CRC-16-CCITT, poly `0x1021`, init `0xFFFF`) | Contrôle d'intégrité de chaque trame (Header puis Payload) | Vérification à `HilProtocolParser-Frame.cpp:43-47` : `received != computed` → trame rejetée, resynchronisation |
| `Src/Embedded/Transport/HilProtocol.cppm:27-39` | `kSync1=0x48`, `kSync2=0x49`, `kMsgIdSensor=0x01`, `kMsgIdActuator=0x02`, `kProtocolVer=0x10`, `kHeaderSize=8`, `kCrcSize=2`, `kMaxPayload=128`, `kSensorPayloadSize=80`, `kActuatorPayloadSize=44` | Constantes du contrat binaire HIL-Proto v1.0 | `static_assert` de layout wire à `HilProtocol.cppm:92-96` ; **note :** `protocol_ver` n'est jamais vérifié en réception (constat §4) |
| `Src/Embedded/Hil/Transport/HilTransport.cppm:41-57` | `struct HilCommStats { messages_sent/received/dropped, sequence_errors, timeouts, stale_packets, latency_min/max/mean_us }` | Compteurs d'observabilité de la liaison HIL | `HilTransport-Stats.cpp`, alimentés par chaque rejet/timeout |
| `Src/SIL/Core/Comms/CommsBus.cppm:29-44` | `struct CommsStats { sent, delivered, dropped, duplicated, reordered, timeouts, last_sequence, latency_min/max/mean_s }` | Statistiques agrégées du bus FC1↔FC2 (SIL) | `CommsStats::record()` (`CommsBus.cpp:37-55`), `record_timeout()` (`:57-60`) ; invariant testé `sent == delivered + dropped` (`Tests/Sil/Observability/SilObservability-Dropped.cpp:49`) |

### 1.5 Classification des échecs de validation (Monte-Carlo / SIL)

`enum class FailureReason : std::uint8_t` — `Src/SIL/Validation/Types/ValidationTypes.cppm:26-37`. Hiérarchie de classification à contrôles ordonnés par sévérité (`FlightControlValidation-Classify.cpp:20-24` : « mission abort > undetected fault > watchdog > safety mode > physical bounds > comms »).

| Module / Fichier | Identifiant / Enum | Description / Signification | Condition de déclenchement (Code) |
| :--- | :--- | :--- | :--- |
| `ValidationTypes.cppm:27` | `FailureReason::None = 0` | Aucun échec | Valeur initiale conservée si aucune règle ne déclenche (`FlightControlValidation-Classify.cpp:29`) |
| `ValidationTypes.cppm:28` | `FailureReason::MissionAborted = 1` | Mission terminée en ABORTED | `final_state == ABORTED` (`FlightControlValidation-Classify.cpp:38-39`) |
| `ValidationTypes.cppm:29` | `FailureReason::MissionFailed = 2` | Mission terminée en FAILED (testé **avant** ABORTED) | `final_state == FAILED` (`Classify.cpp:36-37`) |
| `ValidationTypes.cppm:30` | `FailureReason::FaultUndetected = 3` | Faute injectée jamais détectée | `fault_type != None && !fault_detected` (`Classify.cpp:40-41`) |
| `ValidationTypes.cppm:31` | `FailureReason::WatchdogMissed = 4` | Watchdog non déclenché alors qu'attendu | `fault_type != None && !watchdog_triggered`, sauf `ActuatorDegradation` et `SensorFault` (`Classify.cpp:42-45`) |
| `ValidationTypes.cppm:32` | `FailureReason::SafetyModeNotReached = 5` | Mode sécurité non atteint | `fault_type != None && !safe_mode_reached`, sauf `ActuatorDegradation` (`Classify.cpp:46-48`) |
| `ValidationTypes.cppm:33` | `FailureReason::PositionExceeded = 6` | Erreur position horizontale hors borne | `position_error_m > 50.0` (`Classify.cpp:49-50`) — cible position codée en dur `(0,0)` (`:30-31`) |
| `ValidationTypes.cppm:34` | `FailureReason::AltitudeExceeded = 7` | Erreur altitude hors borne | `altitude_error_m > 25.0` (`Classify.cpp:51-52`) |
| `ValidationTypes.cppm:35` | `FailureReason::CommsTimeout = 8` | Timeout de communication FC1↔FC2 | `comms.timeouts > 0 && fault_type == CommunicationLoss` (`Classify.cpp:53-54`) — `CommunicationLossRate` exclu |
| `ValidationTypes.cppm:36` | `FailureReason::RunnerError = 9` | Échec d'exécution du moteur SIL | `!outcome.has_value()` sur `SILRunner::run` (`FlightControlValidation-Runner.cpp:80-85`) |

Verdict synthétique SIL : `SimulationResult::compute_verdict()` (`Src/SIL/Core/SilTypes.cpp:28-45`) — FAIL si `fault_expected != fault_detected` ; FAIL si `detection_latency > 0.5 s` (sauf domaines `FC1Heartbeat` et `Actuator`, `:33-35`) ; ABORTED exige `final_safety_mode == SAFE_MODE` (`:37-39`) ; COMPENSATED exige `final_health == DEGRADED` (`:40-42`) ; sinon exige `mission_success && HEALTHY && NORMAL` (`:43-44`). Équivalent HIL : `Src/Embedded/Hil/Runner/Context/HilRunnerTypes.cpp:25-26` (même borne de latence 0,5 s).

### 1.6 Constantes et seuils de détection / mitigation

| Fichier:ligne | Constante | Valeur | Rôle |
| :--- | :--- | :--- | :--- |
| `Src/Safety/HealthMonitor.cppm:49` | `HealthMonitorConfig::heartbeat_timeout_s` | `0.10` s | Timeout heartbeat FC1 / silence du lien |
| `Src/Safety/HealthMonitor.cppm:50` | `actuator_mismatch_rpm` | `60.0` rpm | Seuil d'écart consigne/mesure actionneur |
| `Src/Safety/HealthMonitor.cppm:51` | `actuator_mismatch_hold_s` | `0.50` s | Durée de maintien de l'écart avant levée du drapeau |
| `Src/SIL/Core/Telemetry/Telemetry.cppm:32-35` | `SensorValidationLimits { max_altitude_m, max_position_m }` | `500.0` m / `1000.0` m | Plages physiques de validation capteur (NaN rejeté en plus) |
| `Src/SIL/Core/Telemetry/Telemetry.cppm:30` | `kExtremeNoiseAmplitudeM` | `300.0` m | Amplitude du bruit extrême injecté |
| `Src/Safety/SafetyManager.cppm:31` | `degraded_thrust_margin` | `1.7` | Marge de poussée en mode COMPENSATED (faute actionneur prouvée) |
| `Src/SIL/Runner/Context/SilRunnerContext.cppm:45` | `SilConfig::safe_descent_rpm_rate` | `4000.0` rpm/s | Pente de décroissance RPM en descente SAFE_MODE |
| `Src/SIL/Runner/Context/SilRunnerContext.cppm:42-44` | `heartbeat_timeout_s=0.10`, `actuator_mismatch_rpm=60.0`, `thrust_compensation_margin=1.7` | — | Configuration SIL des seuils FC2 (miroir `HilConfig.cppm:65-68`) |
| `Src/SIL/Runner/Context/SilRunnerContext.cppm:32` | `kMaxFaultInjectors` | `8` | Capacité fixe de scénarios de faute par run SIL |
| `Src/Embedded/Hil/Runner/Context/HilRunnerTypes.cppm:37` | `kHilMaxFaultScenarios` | `8` | Capacité fixe côté HIL |
| `Src/Embedded/Hil/Config/HilConfig.cppm:97` | `transport_timeout_us` | `10000` µs | Budget wall-clock d'attente d'un `ActuatorPacket` (défaut = période de contrôle ; **non référencé dans le code**, la borne réelle est `next_deadline_us`, `HilRunner-Step.cpp:50` — constat §4) |
| `Src/SIL/Core/Events/SilEvents.cppm:94` | `SilTraceConfig::max_events` | `500000` | Capacité tampon de trace (au-delà, événements silencieusement ignorés — `SilEvents-Recorder.cpp:24-26`) |
| `Src/SIL/Validation/FlightControlValidation-Classify.cpp:49-52` | seuils physiques de validation | `50.0` m / `25.0` m | Bornes d'erreur position / altitude pour le verdict Monte-Carlo |
| `Src/SIL/Core/SilTypes.cpp:34` | borne de latence de détection | `0.5` s | `detection_latency > 0.5` → verdict FAIL (hors FC1Heartbeat/Actuator) |
| `Src/Control/Types/FlightControllerTypes.cppm:46-53` | `max_tilt_deg=15.0`, `min_rpm=300.0`, `max_rpm=12000.0`, `takeoff_rpm_factor=1.1`, `takeoff_altitude_m=2.0`, `altitude_tolerance_m=0.5`, `position_tolerance_m=1.0`, `station_hold_seconds=5.0` | — | Bornes et tolérances du contrôleur (saturations de commande) |
| `Src/Simulation/Aircraft-Core.cpp:21-24` | `kMinRpm=0.0`, `kMaxRpm=12000.0`, `kMinServoDeg=-30.0`, `kMaxServoDeg=30.0` | — | Saturations physiques actionneurs (`update_actuators`, `:52-57`) |
| `Src/Control/FlightController-Core.cpp:18` | `kAltitudeIntegralErrorBandM` | `5.0` m | Bande d'accumulation de l'intégrale (anti-windup conditionnel) |

---

## 2. Comportements et Mitigations actuels

### 2.1 Crash / silence de FC1 — `FC1_HEARTBEAT_TIMEOUT`

- **Identifiant :** `FaultDomain::FC1Heartbeat` / `"FC1_HEARTBEAT_TIMEOUT"` (faute injectée : `FaultType::FC1Failure`)
- **Emplacement :** détection `Src/Safety/HealthMonitor-Evaluate.cpp:23-34` ; injection `Src/SIL/Faults/FaultInjectors-Injectors.cpp:20-23`
- **Mécanisme de détection :** FC2 surveille le timestamp du dernier message reçu sur le bus : `comms.last_received_time() >= 0.0 && current_time - last_received > heartbeat_timeout_s` (0,10 s). L'émission côté FC1 (heartbeat séquencé à chaque pas, `SilRunner-Fc1.cpp:50-54`) cesse quand `fc1_alive == false`.
- **Action locale immédiatement exécutée :** levée du drapeau **latched** `FC1Heartbeat` (`HealthMonitor-Core.cpp:31-39`) ; `compute_state()` retourne `HealthState::SAFE` (`:43-48`). Émission des événements `WatchdogTimeout` (Warning, reason `"supervised link"`) et `MessageTimeout` (`SilRunnerEvents-Watchdog.cpp:40-61`), incrément `comms_stats.timeouts` (`:63`), mémorisation `watchdog_triggered` + `watchdog_trigger_time` (`:64-68`). Événement `FCFailure` (Warning, detail `"FC1_FAILURE"`) côté environnement (`SilRunnerEvents-Lifecycle.cpp:68-77`).
- **Action système / Fail-Safe déclenché :** `SafetyManager::update()` → `engage(SAFE_MODE, margin=1.0, abort=true)` (`SafetyManager.cpp:59-61`), **irréversible** (`:62`). Dans la boucle SIL : la commande FC1 est remplacée par une **descente contrôlée** — `safe_rpm = max(0, last_effective_rpm - 4000.0 * dt)`, servos à 0 (`SilRunner-Loop.cpp:59-64`) ; FC1 ne recalcule plus de commande (`SilRunner-Fc1.cpp:47-49`). `mission_abort_recorded` → `final_state = ABORTED` (`SilRunner-Run.cpp:84-89`, `SilRunner-Finalize.cpp:33-35`). Événements `FaultDetected` / `FaultClassified` / `SafetyStateTransition` / `SafetyResponse` (`"ENTER_SAFE_MODE"`) enregistrés (`SilRunnerEvents-Transitions.cpp:35-89`). Comportement nominal attendu (déclaré dans le code) : « Watchdog must raise FC1_HEARTBEAT_TIMEOUT within 300ms, SafetyManager must engage SAFE_MODE with a controlled descent and the mission must be aborted » (`SilRunnerEvents-FaultReasons.cpp:77-79`). Vérifié par les tests FAULT_INJECTOR-001/005 (détection ≤ 0,3 s — `Tests/ScenarioBrief-Faults.cpp:28-37,70-79`).
- **Variante HIL :** FC1 silencieux → `transport.noteTimeoutFrame()` sans blocage (`HilRunner-Step.cpp:92-95`, `HilTransport.cppm:69-70`) ; la même chaîne HealthMonitor/SafetyManager s'applique côté hôte (`HilRunner-Apply.cpp:40-86`) ; événement `HeartbeatTimeout` émis (`HilRunnerEvents-Faults.cpp:88-94`).

### 2.2 Rupture de communication FC1↔FC2 — `COMMUNICATION_LOST`

- **Identifiant :** `FaultDomain::Communication` / `"COMMUNICATION_LOST"` (faute : `FaultType::CommunicationLoss`)
- **Emplacement :** détection `Src/Safety/HealthMonitor-Evaluate.cpp:25-27` ; injection `Src/SIL/Faults/FaultInjectors-Injectors.cpp:30-31`
- **Mécanisme de détection :** drapeau physique `!comms.link_up()` — la coupure est visible immédiatement, sans attente du timeout.
- **Action locale immédiatement exécutée :** drapeau **latched** `Communication` → `HealthState::SAFE` ; mêmes événements watchdog/message-timeout que §2.1 (`SilRunnerEvents-Watchdog.cpp:46-61`).
- **Action système / Fail-Safe déclenché :** identique à §2.1 : SAFE_MODE irréversible, descente contrôlée 4000 rpm/s, mission ABORTED. Comportement attendu déclaré : « Watchdog must raise COMMUNICATION_LOST within 300ms, SafetyManager must engage SAFE_MODE and the mission must be aborted » (`SilRunnerEvents-FaultReasons.cpp:80-82`). Testé par FAULT_INJECTOR-002 (`Tests/ScenarioBrief-Faults.cpp:38-46`) et le Test E du cahier de validation (`docs/system/fault_handling.md:388-394`).

### 2.3 Pertes aléatoires de paquets — `FaultType::CommunicationLossRate`

- **Identifiant :** `FaultType::CommunicationLossRate` (paramètre `loss_probability` ∈ [0,1], validé à la construction, `FaultInjectors-Factory.cpp:32-33`)
- **Emplacement :** `Src/SIL/Core/Comms/CommsBus.cpp:69-86` ; injection `Src/SIL/Faults/FaultInjectors-Injectors.cpp:33-34`
- **Mécanisme de détection :** chaque `publish()` tire `uniform_real_distribution{0,1}` (générateur seedé pour reproductibilité Monte-Carlo) ; si `draw < loss_probability_` le message est perdu (`delivered=false`). FC2 ne détecte que si le silence dépasse `heartbeat_timeout_s`.
- **Action locale :** messages perdus comptés en `CommsStats::dropped` (`CommsBus.cpp:41-43`) ; événements `HeartbeatDropped` (Trace) (`SilRunnerEvents-Heartbeats.cpp:68-78`).
- **Action système / Fail-Safe :** aucune tant que le timeout n'est pas dépassé — comportement attendu déclaré : « Link supervision must absorb the configured packet loss rate and raise COMMUNICATION_LOST only if the heartbeat timeout is exceeded » (`SilRunnerEvents-FaultReasons.cpp:83-85`). Si le timeout est dépassé → chaîne §2.1.

### 2.4 Données capteur invalides — `SENSOR_INVALID`

- **Identifiant :** `FaultDomain::Sensor` / `"SENSOR_INVALID"` (faute : `FaultType::SensorFault`, modes `AltitudeNaN` / `AltitudeOutOfRange` / `ExtremeNoise`)
- **Emplacement :** validation `Src/SIL/Core/Telemetry/Telemetry-Validation.cpp:21-29` ; drapeau `Src/Safety/HealthMonitor-Evaluate.cpp:40-49` ; hold FC1 `Src/SIL/Runner/SilRunner-Fc1.cpp:44-46`
- **Mécanisme de détection :** contrôle de plage à chaque pas : `altitude_valid = !isnan(z) && z >= 0.0 && z <= max_altitude_m (500)` ; `position_valid = !isnan(hypot(x,y)) && hypot(x,y) <= max_position_m (1000)`.
- **Action locale immédiatement exécutée :** (1) **FC1 conserve la dernière mesure valide** — `fc1_view` n'est mis à jour que si `validate(...).all_valid()` (`SilRunner-Fc1.cpp:44-46`) ; (2) drapeau `Sensor` levé → `HealthState::DEGRADED` ; (3) événement typé `SensorFault` (Warning, avec cible/signal/sous-type) (`SilRunnerEvents-Faults.cpp:74-95`).
- **Action système / Fail-Safe déclenché :** `SafetyManager` → `COMPENSATED` avec **marge 1.0** (la poussée reste nominale car la faute est capteur, pas actionneur — `SafetyManager.cpp:63-65` et commentaire `:51-56`). Mission **poursuivie**. Drapeau **auto-nettoyant** : dès que la télémétrie redevient valide, le drapeau retombe (`HealthMonitor-Evaluate.cpp:46-48`) → événements `RecoveryStart` (« fault flag cleared », `SilRunnerEvents-Watchdog.cpp:75-93`) puis `RecoveryEnd` + `WatchdogRecovery` (« health restored », `SilRunnerEvents-Health.cpp:44-62`), retour `NORMAL`. Comportement attendu déclaré : « Health monitor must invalidate the altitude channel on the range check within 500ms, FC1 must hold the last valid altitude, SafetyManager must engage COMPENSATED with nominal thrust and the flag must clear once the telemetry validates again » (`SilRunnerEvents-FaultReasons.cpp:86-90`). Testé par FAULT_INJECTOR-003 (fenêtre t∈[20 s, 30 s], altitude forcée 99999 m — `Tests/Sil/SilScenarios-Safety.cpp:42-43`).
- **Variante HIL :** même validation des deux côtés de la liaison (`HilRunner-Step.cpp:75`, `HilFcTarget-Host.cpp:78`) ; la cible hôte fige sa dernière vue valide (`HilFcTarget-Host.cpp:81-88`) ; les drapeaux de validité sont publiés dans `sensor_valid_flags` de la trame (`HilSensorModel.cpp:53-58`).

### 2.5 Actionneur dégradé — `ACTUATOR_MISMATCH`

- **Identifiant :** `FaultDomain::Actuator` / `"ACTUATOR_MISMATCH"` (faute : `FaultType::ActuatorDegradation`, paramètre `efficiency` ∈ (0,1])
- **Emplacement :** détection `Src/Safety/HealthMonitor-Evaluate.cpp:55-75` ; injection `Src/SIL/Faults/FaultInjectors-Injectors.cpp:51-54`
- **Mécanisme de détection :** écart `|commanded_rpm - telemetry.actual_rpm| > 60.0` avec `commanded_rpm > 0`, qui doit être **soutenu ≥ 0,50 s** (filtrage anti-transitoire par `mismatch_since_`, `:62-68`).
- **Action locale immédiatement exécutée :** drapeau `Actuator` (auto-nettoyant si l'écart retombe, `:72-74`) → `HealthState::DEGRADED` ; événement typé `ActuatorFault` (Warning) (`SilRunnerEvents-Faults.cpp:79-94`).
- **Action système / Fail-Safe déclenché :** `COMPENSATED` avec **marge de poussée ×1,7** (`degraded_thrust_margin`) car la faute actionneur est prouvée (`SafetyManager.cpp:63-65`) ; appliquée sur le RPM effectif (`SilRunner-Loop.cpp:65-67`, `HilRunner-Apply.cpp:77-79`). Mission **poursuivie**. Comportement attendu déclaré : « Health monitor must raise ACTUATOR_MISMATCH after a sustained 500ms mismatch, SafetyManager must engage COMPENSATED with a 1.7x thrust margin within 600ms and the mission must continue » (`SilRunnerEvents-FaultReasons.cpp:91-94`). Testé par FAULT_INJECTOR-004 (efficacité 0,6 à t=15 s — `Tests/Sil/SilScenarios-Safety.cpp:68-69`). Note verdict : la latence de détection de cette faute est **exclue** de la borne 0,5 s car elle dépend de la dynamique de vol (`SilTypes.cpp:33-35`).

### 2.6 Épuisement de la fenêtre de mission — `MissionState::FAILED`

- **Identifiant :** `MissionState::FAILED`
- **Emplacement :** `Src/SIL/Runner/SilRunner-Finalize.cpp:36-38`
- **Mécanisme de détection :** fin de la fenêtre de simulation (`duration_s`, défaut 30 s) sans avoir atteint `COMPLETE` ni enregistré d'abort sécurité.
- **Action locale :** `final_state = FAILED`, `mission_success = false` (`SilRunner-Finalize.cpp:36-39`).
- **Action système :** le contrôleur répond aux états `ABORTED`/`FAILED` par une **commande nulle** (`ControlCommand{}`, `FlightController-Mission.cpp:91-96`). Classification validation : `FailureReason::MissionFailed` (priorité maximale, `Classify.cpp:36-37`).

### 2.7 Erreurs de trame HIL (CRC, séquence, écho, type, payload)

- **Identifiants :** `ReceiveResult::{SequenceError, EchoMismatch, InvalidPayload, MessageTypeError}`, `FrameAcceptanceError::NotActuatorFrame`, rejet CRC parseur
- **Emplacement :** `Src/Embedded/Hil/Transport/HilTransport-Accept.cpp:45-71` ; `Src/Embedded/Transport/Parser/HilProtocolParser-Frame.cpp:27-47`
- **Mécanismes de détection :**
  - **CRC-16-CCITT** (poly `0x1021`, init `0xFFFF`) recalculé sur Header+Payload et comparé au CRC reçu (`HilProtocolParser-Frame.cpp:43-47`) ; en cas d'échec la FSM se **resynchronise** sur les sync bytes (`HilProtocolParser-Sync.cpp`).
  - **Numéro de séquence** : `header.sequence_num != expected_sequence` (`HilTransport-Accept.cpp:49-52`).
  - **Écho temporel** : `payload.echo_sim_timestamp_us != expected_echo_sim_us` → paquet « stale » (`:63-66`).
  - **Longueur/décodage payload** : `payload_len < kActuatorPayloadSize` ou échec de décodage (`:53-62`).
  - **Type de message** : `msg_id != kMsgIdActuator` → `NotActuatorFrame` (`:45`).
- **Action locale immédiatement exécutée :** compteurs incrémentés (`record_sequence_error()`, `record_stale()`, `record_dropped()` — `HilTransport-Stats.cpp`) ; la trame est rejetée et la boucle de réception continue à drainer le canal jusqu'à la deadline (`HilTransport-Receive.cpp`). Événement `HilStepError` émis avec le motif texte (« actuator timeout », etc. — `HilRunnerEvents-Steps.cpp:27-34`).
- **Action système / Fail-Safe déclenché :** pas de mise à jour actionneur pour ce pas → **hold de la dernière commande** (`apply_actuators` : `effective = this_received ? to_control_command(...) : ctx.command`, `HilRunner-Apply.cpp:69`) ; si le silence se prolonge au-delà de `heartbeat_timeout_s`, la chaîne §2.1 s'engage. Suite de tests dédiée : `Tests/Hil/HilTests-Protocol.cpp:54-106` (seq/echo/type/CRC/doublon rejetés et comptés).

### 2.8 Timeout de réponse actionneur HIL — `ReceiveResult::Timeout`

- **Identifiant :** `ReceiveResult::Timeout`
- **Emplacement :** `Src/Embedded/Hil/Transport/HilTransport-Receive.cpp:63-66`
- **Mécanisme de détection :** deadline wall-clock absolue (`next_deadline_us`, fin du pas de contrôle 10 ms — `HilRunner-Step.cpp:50`) atteinte sans trame CRC-valide.
- **Action locale :** `stats_.record_timeout()`, retour `Timeout`, événement `HilStepError` (« actuator timeout », `HilRunnerEvents-Steps.cpp:33-34`). Chemin « FC1 silencieux » non bloquant : `noteTimeoutFrame()` (`HilRunner-Step.cpp:92-95`). Trame logiquement perdue purgée du canal pour préserver le lockstep (`ctx.channel.flush()`, `HilRunner-Step.cpp:44-47`).
- **Action système :** hold de la dernière commande (`HilRunner-Apply.cpp:69`) ; dépassement du timeout heartbeat → chaîne SAFE_MODE §2.1.

### 2.9 Dépassement de deadline temps réel (HIL) — `DeadlinePolicy`

- **Identifiant :** `enum class DeadlinePolicy { Warn, Fail, Abort }` (`Src/Embedded/Hil/Config/HilConfig.cppm:29-33`), événement `HilEventType::DeadlineMissed`
- **Emplacement :** détection `Src/Embedded/Hil/Clock/HilTiming.cppm:57-60` (`deadline_missed`) ; application `Src/Embedded/Hil/Runner/HilRunner-Loop.cpp:81-83`, `HilRunner-Finalize.cpp:48-50`
- **Mécanisme de détection :** comparaison de l'horloge wall-clock hôte à la deadline absolue du pas (pacing 100 Hz / 10 ms, `HilConfig.cppm:46-51`).
- **Action locale :** événement `DeadlineMissed` avec latence (`HilRunner-Inject.cpp:50-60`), compteur de statistiques.
- **Action système :** `Warn` (défaut) : enregistre et continue ; `Fail` : force le verdict FAIL en fin de run (`HilRunner-Finalize.cpp:48-50`) ; `Abort` : `aborted_on_deadline` → sortie immédiate de la boucle de mission (`HilRunner-Loop.cpp:81-83`). Testé par `Tests/Hil/HilTests-Timing.cpp:27-35` (nominal : `deadline_misses == 0`).

### 2.10 Erreurs de configuration des scénarios de fautes

- **Identifiants :** `SilError::{TooManyScenarios, FaultScenarioRejected}`, `HilError::{TooManyScenarios, FaultScenarioRejected}`, `InjectorError::{...}`
- **Emplacement :** `Src/SIL/Runner/SilRunner-Context.cpp:36-57` ; `Src/Embedded/Hil/Runner/HilRunner-Context.cpp:41-55` ; `Src/SIL/Faults/FaultInjectors-Factory.cpp:23-48`
- **Mécanisme de détection :** validation à la construction du contexte : capacité fixe (8), paramètres bornés (probabilité ∈ [0,1], efficacité ∈ (0,1], corruption ≠ None), type connu.
- **Action locale :** retour `std::unexpected(...)` — aucune exception ; les scénarios nominaux (`NoFault`) sont **ignorés silencieusement** (`SilRunner-Context.cpp:48-50`).
- **Action système :** le run ne démarre pas ; les CLI affichent l'erreur sur stderr et retournent un code non nul (ex. `Src/Embedded/Hil/Cli/main.cpp:50-53` : « HIL run failed », exit 1 ; `Src/Runners/Sil/main.cpp:92` : exit 2 sur usage invalide).

### 2.11 Erreurs d'écriture des rapports — `ReportError`

- **Identifiants :** `ReportError::{DirectoryCreation, FileOpen}`
- **Emplacement :** `Src/SIL/Reporting/Report/SilReporting-File.cpp:25-31,43-45,112-115`
- **Mécanisme de détection :** `std::error_code` de `create_directories` et test `is_open()` des flux.
- **Action locale :** retour `std::unexpected(ReportError)` ; messages stderr côté campagne Monte-Carlo (« Error: cannot write CSV report to ... », `MonteCarloCampaign-Export.cpp:134,149`).
- **Action système :** propagation au code de sortie du processus (SIL : exit 1 sur échec — `Src/Runners/Sil/main.cpp:71`). **Limite constatée :** l'état du flux n'est pas revérifié après streaming (un `failbit` disque plein passerait inaperçu — §4).

### 2.12 Gardes physiques et saturations (pré/post-conditions)

- **Saturations actionneurs (physique) :** RPM clampé [0, 12000], servos clampés [-30°, +30°] à chaque pas (`Src/Simulation/Aircraft-Core.cpp:52-57`) ; contact sol : `z <= 0 → z = 0` et `vz < 0 → vz = 0` (`Aircraft-Physics.cpp:64-69`).
- **Saturations contrôleur :** RPM de consigne clampé [min_rpm=300, max_rpm=12000] (`FlightController-Core.cpp:114`) ; consignes de tilt saturées à ±`max_tilt_deg` (15°) (`FlightController-Loops.cpp:45-48`) ; servos saturés ±30° (`:63-64`) ; anti-windup PID : intégrale clampée ±`integral_limit` et gelée hors bande ±5 m (`FlightController-Core.cpp:93-96`) ; reset des PID à chaque changement de phase pour éviter le « derivative kick » (`FlightController-Core.cpp:56-76`).
- **Garde division :** `integral_limit = max_integral_rpm / max(ki_altitude, 1e-9)` (`FlightController-Core.cpp:34`).
- **Garde latence négative :** `set_transport_latency` clampée à ≥ 0 (`CommsBus.cpp:28-31`).
- **`static_assert` de layout wire :** tailles `HilHeader`/`HilSensorPayload`/`HilActuatorPayload` et `kMaxPayload` (`Src/Embedded/Transport/HilProtocol.cppm:92-96`). **Aucun `assert()` dynamique** dans `Src/` (politique `-fno-exceptions` ; la défense repose sur gardes runtime, `[[nodiscard]]` et types à largeur fixe).

---

## 3. Logs et alertes

### 3.1 Événements SIL critiques (trace structurée `SilEvent`)

`enum class SilEventType` (26 valeurs, `Src/SIL/Core/Events/SilEvents.cppm:29-56`) ; sévérités `enum class EventSeverity { Info, Warning, Error, Debug, Trace }` (`:58`). Les Warning/Error sont **toujours conservés** quel que soit le niveau de trace (`SilEvents-Levels.cpp:38-51`). **Constat : `EventSeverity::Error` n'est jamais émis dans `Src/`** — le maximum utilisé en production est `Warning` (vérifié par recherche exhaustive).

| Événement | Sévérité | Source | Détail / Reason | Émis depuis |
| :--- | :--- | :--- | :--- | :--- |
| `FCFailure` | **Warning** | FC1 | detail `"FC1_FAILURE"` | `SilRunnerEvents-Lifecycle.cpp:68-77` |
| `WatchdogTimeout` | **Warning** | FC2 | detail = domaine (`FC1_HEARTBEAT_TIMEOUT` / `COMMUNICATION_LOST`), reason `"supervised link silent"` | `SilRunnerEvents-Watchdog.cpp:50-56` |
| `MessageTimeout` | **Warning** | FC2 | même timestamp que le watchdog | `SilRunnerEvents-Watchdog.cpp:58-61` |
| `SensorFault` | **Warning** | ENV | valeur, cible, signal, rôle/catégorie/fonction physiques | `SilRunnerEvents-Faults.cpp:74-95` |
| `ActuatorFault` | **Warning** | ENV | idem | `SilRunnerEvents-Faults.cpp:74-95` |
| `FaultInjected` | Info | ENV | type, paramètres (`value_kind`), profil Permanent/Temporary, **comportement attendu** (`expected_behavior`) | `SilRunnerEvents-Faults.cpp:25-42` |
| `FaultCleared` | Info | ENV | reason `"activation window closed"`, durée effective | `SilRunnerEvents-Faults.cpp:49-68` |
| `FaultDetected` | Info | FC2 | detail = premier domaine de faute | `SilRunnerEvents-Transitions.cpp:50-55` |
| `FaultClassified` | Info | FC2 | reason `"fault domain classification"` | `SilRunnerEvents-Transitions.cpp:56-59` |
| `SafetyResponse` | Info | FC2 | detail ∈ `ENTER_SAFE_MODE` / `ENTER_COMPENSATED` / `RESUME_NORMAL` | `SilRunnerEvents-Transitions.cpp:27-32,80-89` |
| `SafetyStateTransition` | Info | FC2 | previous/new (`HEALTHY→DEGRADED→SAFE`, `NORMAL→COMPENSATED→SAFE_MODE`), reason = domaine fautif ou `"health restored"` / `"all flags clear"` | `SilRunnerEvents-Health.cpp:25-39`, `SilRunnerEvents-Transitions.cpp:63-90` |
| `MissionStateTransition` | Info | FC1 | previous/new, reason `"controller progression"` / `"station hold completed"` | `SilRunnerEvents-Transitions.cpp:93-112` |
| `RecoveryStart` | Info | FC2 | reason `"fault flag cleared"` (front descendant d'un drapeau) | `SilRunnerEvents-Watchdog.cpp:75-93` |
| `RecoveryEnd` + `WatchdogRecovery` | Info | FC2 | reason `"health restored"` | `SilRunnerEvents-Health.cpp:44-62` |
| `HeartbeatSent` / `HeartbeatDelivered` / `HeartbeatDropped` | Trace | FC1→FC2 | numéro de séquence, latence | `SilRunnerEvents-Heartbeats.cpp:22-78` |
| `WatchdogKick` | Debug | FC2 | émis à chaque heartbeat livré (ré-armement du watchdog) | `SilRunnerEvents-Heartbeats.cpp:56-62` |
| `SimulationStart` / `FCStartup` / `FCShutdown` / `SimulationEnd` | Info | SIL/FC1 | reason `"SIL run started"` / `"FC1 online"` / `"simulation end"` / état final + `"mission completed"` ou `"mission not completed"` | `SilRunnerEvents-Lifecycle.cpp:24-63` |

### 3.2 Événements HIL critiques (trace `HilEvent`)

`enum class HilEventType` (`Src/Embedded/Hil/Events/HilEvents.cppm:28-45`) ; sévérités `{ Info, Warning, Error }` (`:47`). **Constat : `HilEventSeverity::Error` n'est jamais utilisé** — toutes les fautes sont tracées en Warning maximum. Les types `CommandSent/CommandDropped/CommandReceived` (`:42-44`) sont déclarés mais **jamais émis**.

| Événement | Sévérité | Signification | Référence |
| :--- | :--- | :--- | :--- |
| `Fc1Failure` | Warning | FC1 tué par l'environnement | `HilRunnerEvents-Faults.cpp` |
| `HeartbeatTimeout` | Warning | « FC1 heartbeat / actuator-link timeout » | `HilRunnerEvents-Faults.cpp:88-94` |
| `FaultInjected` / `FaultCleared` / `FaultDetected` | Info/Warning | chaîne injection → détection (miroir SIL) | `HilRunnerEvents-Faults.cpp` |
| `SafetyStateTransition` | Info | transitions de mode de sécurité | `HilRunnerEvents-Faults.cpp` |
| `DeadlineMissed` | Warning | dépassement de deadline wall-clock (avec latence) | `HilRunner-Inject.cpp:50-60` |
| `HilStepError` | Warning | motif texte : « actuator timeout », etc. | `HilRunnerEvents-Steps.cpp:27-34` |
| `MissionAborted` / `MissionComplete` | Info | issue de mission | `HilEvents.cppm:32-33` |
| `HilRunStart` / `HilRunEnd` | Info | bornes du run | `HilEvents.cppm:29-30` |

### 3.3 Messages stderr et codes de sortie processus

| Fichier:ligne | Message / Code | Condition |
| :--- | :--- | :--- |
| `Src/Embedded/Hil/Cli/main.cpp:51` | `std::cerr << "HIL run failed\n"` → exit **1** | `HilRunner::run` en erreur (`HilError`) |
| `Src/Runners/Hil/main.cpp:98-99` | `"HIL run failed"` → exit **1** | idem (runner HIL autonome) |
| `Src/Runners/Hil/main.cpp:37-57` | exit **0** / **1** / **2** | 0 = succès ou verdict OK ; 1 = échec run/verdict ; 2 = erreur d'usage CLI |
| `Src/Runners/Sil/main.cpp:35-96` | exit **0** / **1** / **2** | 0 = succès ; 1 = échec run ou rapport ; 2 = usage invalide ; le compteur d'échecs s'accumule sur tout le run et le code de sortie reflète le verdict (`:77-78`) |
| `Src/Runners/Sil/SilRunnerCli.cpp:30,67,77,99,104` | `Error: invalid value for --telemetry-period`, `Error: missing value for --scenario`, `Error: unknown argument`, `Error: --all cannot be combined with --scenario` | erreurs de parsing CLI SIL |
| `Src/Runners/MonteCarlo/main.cpp:27-62` | exit **0** / **2** | 0 = campagne exécutée **même si `failed_runs > 0`** (constat §4) ; 2 = usage invalide |
| `Src/Runners/MonteCarlo/MonteCarloCampaign-Run.cpp:30` | `Error: Unknown scenario <id>` | identifiant de scénario inconnu |
| `Src/Runners/MonteCarlo/MonteCarloCampaign-Export.cpp:134,149` | `Error: cannot write CSV/JSON report to <path>` | échec d'ouverture de fichier d'export |
| `Src/Embedded/Hil/Cli/HilRunnerCli-Dispatch.cpp:72` | `std::cerr << result.error()` | échec d'une option CLI HIL (`std::expected<void, std::string>`) |

### 3.4 Télémétrie descendante et artefacts produits en cas de défaillance

- **Télémétrie SIL structurée** : `TelemetrySample` à cadence fixe (défaut 20 Hz, `SilRunnerContext.cppm:49`) avec états `mission_state` et `safety_state` encodés en `uint8_t` à chaque échantillon (`SilRunner-Run.cpp:90-96`) ; flux **vérité terrain** séparé (`TrueStateSample`, jamais mélangé — `SilTelemetry.cppm:50-67`). Export CSV des champs watchdog/comms/verdict : `FlightControlValidation-CsvRows.cpp:67` (`comms.timeouts`, `watchdog_triggered`, `watchdog_trigger_time`...), en-tête à `FlightControlValidation-CsvWriter.cpp:35`.
- **Télémétrie HIL** : module `Src/Embedded/Hil/Telemetry/HilTelemetry.cpp` ; rapport humain à 1 Hz (`HilConfig.cppm:77`) ; résumé final incluant `Timeouts` (`HilReport-Summary.cpp:41`) ; table d'événements avec libellé « heartbeat timeout » (`HilReport-Table.cpp:88-89`).
- **Diagnostics FC embarqués dans la trame actionneur** : `ActuatorDiagnostics { cpu_usage_pct_x100, stack_watermark_words, deadline_miss_count, fc_health_status }` (`Src/Embedded/Transport/HilProtocol.cppm:102-108`) — **constat : valeurs constantes côté cible hôte** (toujours 0 / santé 1, `HilFcTarget-Respond.cpp:55-60` — pas de détection réelle implémentée, §4).
- **Artefacts de rapport** : `sil.md` + sections + JSON + CSV + JSONL + résumés de campagne Monte-Carlo avec taux de réussite et bornes 3-sigma (`Src/SIL/Reporting/`, `Src/Runners/MonteCarlo/MonteCarloCampaign-Summary.cpp`).
- **Raisons de verdict textuelles** (français, `Src/SIL/Core/SilTypes.cpp:51-63`) : « Comportement non conforme aux attendus du scenario » / « Defaillance detectee et SAFE_MODE engage dans les limites requises » / « Defaillance degradee compensee, mission poursuivie » / « Mission nominale completee sans comportement anormal ».

### 3.5 Comportements de sécurité attendus — spécification extraite de la suite de tests

Seuils temporels et invariants effectivement vérifiés par `Tests/` (le TestHarness `check(condition, label)` — `Tests/TestHarness-Checks.cpp:46-56`) :

| Exigence vérifiée | Seuil exact | Référence test |
| :--- | :--- | :--- |
| Détection crash FC1 / perte comms (watchdog) | ≤ **300 ms** | `Tests/ScenarioBrief-Faults.cpp:28-46,70-79` |
| Réponse sécurité après détection FC1 | ≤ **200 ms** | `Tests/ScenarioBrief-Faults.cpp:28-37` |
| Invalidation canal capteur (range check) | ≤ **500 ms** | `Tests/ScenarioBrief-Faults.cpp:47-58` |
| Compensation actionneur (marge 1,7) | ≤ **600 ms** | `SilRunnerEvents-FaultReasons.cpp:91-94` |
| Faute capteur/actionneur → mission **non** avortée, état DEGRADED + COMPENSATED | — | `Tests/Sil/SilScenarios-Safety.cpp:55-60,81-85` |
| Faute critique → SAFE_MODE + ABORTED | — | `Tests/Sil/SilScenarios-Core.cpp:70,97` |
| Ordre causal : `watchdog_trigger_time <= detection_time <= safety_response_time` | — | `Tests/Sil/Observability/Faults/SilObservability-Faults.cpp:67-70` |
| Conservation heartbeats : `sent == delivered + dropped` | — | `Tests/Sil/Observability/SilObservability-Dropped.cpp:49` |
| Neutralité observationnelle (niveau de log, débit télémétrie 5/20/50 Hz) | résultats bit-identiques | `Tests/Sil/Observability/SilObservability-Logging.cpp:83-90`, `Telemetry/SilObservabilityTelemetry-Sampling.cpp:102-115` |
| Intégrité vérité terrain : `isfinite(truth.z)` ∀ échantillon | — | `Tests/Sil/Observability/Telemetry/SilObservabilityTelemetry-Support.cpp:96,107` |
| Altitude nominale bornée | ≤ 10,5 m | `Tests/Scenarios/FlightScenarios-*` (borne nominale) |

---

## 4. Écarts et points d'attention relevés (entrées directes pour l'AMDEC)

Constats issus exclusivement du code actuel — modes de défaillance **non couverts** ou détections **mortes** à considérer dans l'analyse :

1. **`HealthState::FAILED` jamais produit** — `compute_state()` ne retourne que HEALTHY/DEGRADED/SAFE (`Src/Safety/HealthMonitor-Core.cpp:41-55`) ; l'état ultime de perte d'intégrité n'a aucun chemin d'activation.
2. **Pas de détection NaN dans la physique simulée** — `Aircraft-Core.cpp` / `Aircraft-Physics.cpp` ne contiennent aucun test `isnan`/`isfinite` ; une divergence numérique se propagerait sans alarme (les clamps `std::min/std::max` laissent passer NaN selon l'ordre des opérandes).
3. **Masse non gardée contre les dispersions extrêmes** — `hover_rpm()` (`Aircraft-Core.cpp:78-80`) et `update_translation` (`Aircraft-Physics.cpp:49-52`) calculent avec `1.2 × (1 + mass_variation)` ; une gaussienne non tronquée (`PhysicsDispersion.cpp:35`) peut produire `mass_variation ≤ -1` → racine de négatif / division par zéro, sans garde.
4. **Distributions normales non bornées** (vent, pression, température, bruits) — valeurs physiquement aberrantes possibles sans clamp post-tirage (`FlightControlValidation-Conditions.cpp:26-28`, `PhysicsDispersion.cpp:35-58`).
5. **`transport_timeout_us` mort** — déclaré (`HilConfig.cppm:97`, défaut 10000 µs) mais jamais référencé ; la borne réelle est `next_deadline_us` (`HilRunner-Step.cpp:50`).
6. **`ReceiveResult::MessageTypeError` jamais produit** — couvert par `FrameAcceptanceError::NotActuatorFrame`, absorbé silencieusement (compté en `messages_dropped`, sans événement `HilStepError`).
7. **`HilError::InvalidConfiguration` jamais retourné** (`HilRunnerTypes.cppm:32`).
8. **Sévérités `Error` jamais émises** — ni `EventSeverity::Error` (SIL) ni `HilEventSeverity::Error` (HIL) ; toutes les fautes, y compris critiques, sont tracées en **Warning** maximum.
9. **`protocol_ver` (0x10) jamais vérifié en réception** — une trame de version incorrecte mais CRC-valide serait acceptée (`HilProtocol.cppm:31`, aucun contrôle dans `HilTransport-Accept.cpp`).
10. **Diagnostics FC constants côté cible hôte** — `cpu_usage`/`stack_watermark`/`deadline_miss_count` toujours 0 et `fc_health_status` toujours 1 (`HilFcTarget-Respond.cpp:55-60`) : le canal de diagnostic embarqué existe mais ne remonte aucune détection réelle.
11. **Code de sortie Monte-Carlo insensible au verdict** — `Src/Runners/MonteCarlo/main.cpp:62` retourne 0 même si des runs ont échoué (contrairement aux runners SIL/HIL qui retournent 1).
12. **Écritures fichier non revérifiées après streaming** — `write_section_file` (`SilReporting-File.cpp:25-31`) et les exports campagne testent l'ouverture mais jamais le `failbit` final (disque plein non détecté).
13. **Troncature silencieuse `--runs`** — cast `uint64_t → uint32_t` sans contrôle de borne (`MonteCarloCampaign-Cli.cpp:60`).
14. **Options CLI HIL parsées mais non consommées** — `options.invalid`, `options.selftest`, `interface_name` jamais lus dans `Src/Embedded/Hil/Cli/main.cpp` ; un scénario inconnu produit un **run nominal silencieux** (fallback base + `FaultScenario{}`, `HilRunnerCli-Config.cpp:23,59`).
15. **Tampon de trace saturé silencieusement** — au-delà de `max_events = 500000`, `SilTrace::record` ignore les événements sans signaler (`SilEvents-Recorder.cpp:24-26`).
16. **Aucun scénario NaN capteur testé** — les corruptions testées sont `AltitudeOutOfRange` (99999 m) et `ExtremeNoise` ; la garde `isfinite` n'est vérifiée que sur la vérité terrain, pas comme faute injectée (`Tests/Sil/Observability/Telemetry/SilObservabilityTelemetry-Support.cpp:96`).
17. **Watchdog testé comme détecteur, jamais comme victime** — aucun scénario « tâche de contrôle bloquée avec watchdog lui-même inhibé » (le Test D de `docs/system/fault_handling.md:379-386` n'a pas d'équivalent injecté dans le code).
18. **Cible position codée en dur** `(0, 0)` dans le classifieur (`FlightControlValidation-Classify.cpp:30-31`) alors que la cible altitude provient du scénario.
19. **Divergence de convention** : `Scenario::corrupted_altitude_m = 0.0` (`ValidationTypes.cppm:49`) vs `FaultParameters::corrupted_altitude_m = 99999.0` (`SilFaultScenario.cppm:69`) — sans effet actuel car `corruption = None` dans le premier cas.
20. **Détection actionneur volontairement lente** — le seuil de 0,5 s de maintien + la dynamique de vol excluent cette faute de la borne de latence de verdict 0,5 s (`SilTypes.cpp:33-35`) : la couverture temporelle de cette panne est plus faible par construction.

---

*Fin de l'extraction. Document généré par analyse statique exhaustive du dépôt (branche `main`, commit `57fb009`) — toutes les références `fichier:ligne` ont été vérifiées dans le code source.*

