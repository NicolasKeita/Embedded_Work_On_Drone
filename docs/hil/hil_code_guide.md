# Guide du Code HIL — Implémentation et Utilisation

**Document ID:** GUIDE-HIL-CODE-001
**Version:** 1.0.0
**Date:** 5 Septembre 2026
**Auteur:** Nicolas Keita — Embedded Flight Control Systems Architect
**Périmètre:** Skeleton HIL étape 13 — code sous `Src/Embedded/`, exécutable de démonstration `hil_demo`

---

## 1. Objet du Document

Ce document explique le code HIL **réellement implémenté** dans le dépôt et la manière de l'utiliser. Il complète trois documents voisins :

| Document | Rôle |
| :--- | :--- |
| [`hil_protocol.md`](hil_protocol.md) | Spécification normative du protocole binaire HIL-Proto v1.0 |
| [`hil_architecture.md`](hil_architecture.md) | Architecture cible (STM32, RTOS, HAL) et stratégie matérielle |
| [`hil_validation.md`](hil_validation.md) | Stratégie de validation du banc HIL |

Ici, l'accent est mis sur : la cartographie des modules C++23, le flux de données d'un pas de simulation lockstep, la construction et l'exécution de la démonstration, la réutilisation des briques et le portage vers la cible STM32.

---

## 2. Cartographie des Modules

Tous les modules vivent sous `Src/Embedded/` et suivent la convention du dépôt : interface `.cppm` + implémentations `.cpp` appariées (au plus un suffixe `-Responsabilite`).

| Module C++ | Fichiers | Responsabilité |
| :--- | :--- | :--- |
| `HalTypes` | `Hal/HalTypes.cppm` | Structures HAL `SensorData` / `ActuatorCommands` et masques de validité `kFlagImu1Ok` ... `kFlagTachometerOk` |
| `SensorInput` | `Hal/SensorInput.cppm` | Interface abstraite `ISensorInput` (acquisition capteurs, non bloquante) |
| `ActuatorOutput` | `Hal/ActuatorOutput.cppm` | Interface abstraite `IActuatorOutput` (émission des commandes) |
| `Clock` | `Hal/Clock.cppm` | Interface abstraite `IClock` (temps monotone en microsecondes) |
| `Transport` | `Transport/Transport.cppm` | Interfaces `ITransport` (canal d'octets full-duplex) et `ITransportStream` (vue flux brut, réservée au driver UART STM32) |
| `HilProtocol` | `Transport/HilProtocol.cppm` | Contrat filaire HIL-Proto v1.0 : constantes, `HilHeader`, payloads `#pragma pack(1)`, `ActuatorDiagnostics`, `static_assert` de tailles |
| `HilProtocolParser` | `Transport/Parser/HilProtocolParser.cppm` + `-Crc.cpp`, `-Sync.cpp`, `-Frame.cpp` | Réception : `HilCrc` (CRC-16-CCITT) et `HilFrameParser` (FSM octet par octet, zéro allocation) |
| `HilProtocolCodec` | `Transport/Codec/HilProtocolCodec.cppm` + `-Sensor.cpp`, `-Actuator.cpp` | Conversion HAL <-> payload wire et sérialisation de trames complètes (Header + Payload + CRC) |
| `SimClock` | `Sim/SimClock.cppm` | `SimulatedClock` : horloge virtuelle avancée par le driver lockstep (PC) |
| `LoopbackTransport` | `Sim/LoopbackTransport.cppm` + `.cpp` | `LoopbackTransport` : ring buffer fixe de 512 octets simulant le lien série |
| `SimSensorInput` | `Sim/SimSensorInput.cppm` + `.cpp` | `SimulatedSensorInput` : injecte un échantillon simulé, le FC le lit via `ISensorInput` |
| `SimActuatorOutput` | `Sim/SimActuatorOutput.cppm` + `.cpp` | `SimulatedActuatorOutput` : enregistre la dernière commande émise par le FC |
| `Demo` | `Demo/Demo.cppm` + `Demo-Sensor.cpp`, `Demo-Actuator.cpp`, `Demo-Step.cpp`, `Demo-Report.cpp` | Démonstration de fermeture de boucle d'un pas lockstep complet |
| — | `main.cpp` | Point d'entrée de l'exécutable `hil_demo` (appelle `FlightCore::Demo::runLockstepStep()`) |

---

## 3. Contrat Filaire Implémenté (Rappel)

Les constantes et vérifications de compilation proviennent de `HilProtocol` :

| Élément | Valeur | Symbole |
| :--- | :--- | :--- |
| Octets de synchronisation | `0x48` `0x49` (`'H'`, `'I'`) | `kSync1`, `kSync2` |
| Identifiants de message | `0x01` capteurs, `0x02` actionneurs | `kMsgIdSensor`, `kMsgIdActuator` |
| Version de protocole | `0x10` (v1.0) | `kProtocolVer` |
| En-tête / CRC | 8 / 2 octets | `kHeaderSize`, `kCrcSize` |
| Payload capteurs / trame | 80 / 90 octets | `kSensorPayloadSize`, `kSensorFrameSize` |
| Payload actionneurs / trame | 44 / 54 octets | `kActuatorPayloadSize`, `kActuatorFrameSize` |
| Taille maximale payload | 128 octets | `kMaxPayload` |

Des `static_assert` vérifient à la compilation : `sizeof(HilHeader) == 8`, `sizeof(HilSensorPayload) == 80`, `sizeof(HilActuatorPayload) == 44`. Le CRC-16-CCITT (polynôme `0x1021`, init `0xFFFF`) couvre Header + Payload et est sérialisé en little-endian. La description champ par champ des payloads est maintenue dans [`hil_protocol.md`](hil_protocol.md) (sections 3 et 4).

---

## 4. Flux d'un Pas Lockstep (`runLockstepStep`)

La démonstration (`Demo/Demo-Step.cpp`) exécute une fermeture de boucle complète. Dans ce skeleton, le PC et le « Flight Controller » tournent dans le même processus et le `LoopbackTransport` boucle les octets localement — c'est la préfiguration exacte du banc HIL réel où `ITransport` sera remplacé par le driver UART STM32.

```text
 TruthState (PC, privé)        LoopbackTransport          Flight Controller (mock)
 ----------------------        -----------------          ------------------------
 buildSensorSample
   makeSensorPayload
   encodeSensorFrame   ---> [octets 0x48 0x49 ...] ---> receiveFrame (parser FSM)
                                                    decodeSensorPayload
                                                    toSensorData
                                                    sensor_input.inject/read
                                                    computeFakeControl
                                                    actuator_output.write
                              [octets 0x48 0x49 ...] <--- makeActuatorPayload
                                                          encodeActuatorFrame
 receiveFrame <---
 decodeActuatorPayload
 printReport (RTT, rejets)
```

Étapes détaillées :

1. **Initialisation** : `DemoContext` agrège le transport, l'horloge simulée, les mocks capteur/actionneur, le parser et un buffer de payload (`Demo.cppm`).
2. **Avance du temps PC** : `context.clock.advanceUs(10000)` — le PC est maître du temps (lockstep, 100 Hz).
3. **Échantillon capteur** (`Demo-Sensor.cpp`) : `buildSensorSample()` dérive une *mesure* (jamais la vérité terrain brute, conformément à `hil_validation.md` 4.3) ; `exchangeSensorSample()` encode et émet la trame, la récupère via `receiveFrame()` (drain du transport + `HilFrameParser::processByte`), décode le payload et l'injecte dans le FC via `SimulatedSensorInput`.
4. **Commandes actionneurs** (`Demo-Actuator.cpp`) : `computeFakeControl()` est la loi de commande factice (maintien d'altitude à 100 m autour de 4000 RPM, gain 30, ailes à plat) ; `exchangeActuatorCommands()` horodate les commandes, écrit via `IActuatorOutput`, construit l'`ActuatorPacket` avec `echo_sim_timestamp_us` = timestamp du `SensorPacket` reçu (mesure RTT), l'émet puis décode l'écho.
5. **Rapport** (`Demo-Report.cpp`) : affiche l'état observable du pas — timestamp PC, numéro de séquence, valeurs capteurs TX/RX, commandes FC, écho de timestamp, santé FC, RTT et nombre de trames rejetées par le parser.

---

## 5. Construction et Exécution de la Démonstration

La cible `hil_demo` est définie dans `CMakeLists.txt` (variable `HIL_FILES`, C++23, modules activés).

```bash
# Linux (Ninja)
cmake -S . -B build-lin -G Ninja
cmake --build build-lin --target hil_demo
./build-lin/hil_demo
```

Sous Windows, utiliser les presets de `CMakePresets.json` (`default`, `debug`, `release`) puis `cmake --build build --target hil_demo`.

Sortie attendue (mesurée sur ce dépôt) :

```text
==== HIL lockstep step-closure ====
sim_timestamp_us (PC master) : 10000
sequence_num               : 1
TX sensor alt_baro (m)     : 1.00
RX decoded alt_baro (m)     : 1.00
RX decoded wing_rpm_meas   : 1000.00
FC wing_rpm_cmd             : 6970.00
FC left_servo_rad           : -0.01
FC right_servo_rad          : -0.01
FC mode_flags              : 2
ActuatorPacket echo_sim_ts  : 10000
ActuatorPacket fc_mode      : 2
ActuatorPacket health       : 0
Round-trip time (us)        : 0
Rejected frames by parser    : 0
```

Points de contrôle : `TX alt_baro == RX alt_baro` (intégrité bout-en-bout), `echo_sim_ts == sim_timestamp_us` (chemin aller-retour complet), `Rejected frames == 0` (FSM saine).

---

## 6. Réutiliser les Briques Côté PC (Simulateur Hôte)

Le simulateur PC hôte n'a besoin que des modules `Sim.*`, `Transport.*` et `HalTypes`. Squelette de boucle lockstep 100 Hz :

```cpp
import std;

import HalTypes;
import Transport;
import HilProtocol;
import HilProtocolCodec;
import HilProtocolParser;
import LoopbackTransport;

Sim::LoopbackTransport                    transport{};
Transport::HilFrameParser                 parser{};
std::array<std::uint8_t, Transport::kMaxPayload> payload{};

for (std::uint16_t seq = 0; seq < max_steps; ++seq) {
    HAL::SensorData sample = /* modèle de capteurs alimenté par la vérité terrain */;

    const auto wire  = Transport::makeSensorPayload(sample);
    const auto frame = Transport::encodeSensorFrame(wire, seq);
    transport.sendBytes(frame);

    // Côté FC : drain du transport jusqu'à une trame CRC-valide
    Transport::HilHeader header{};
    if (receiveFrame(transport, parser, header, payload)) {
        Transport::HilSensorPayload decoded{};
        const std::span<const std::uint8_t> bytes(payload.data(), header.payload_len);
        if (Transport::decodeSensorPayload(bytes, decoded)) {
            const HAL::SensorData fc_input = Transport::toSensorData(decoded);
            /* ... loi de commande ... */
        }
    }
}
```

La fonction utilitaire `receiveFrame()` (démo, `Demo-Step.cpp`) montre le drain canonique : lire par blocs `receiveBytes()` et alimenter `processByte()` octet par octet jusqu'à ce qu'une trame complète soit assemblée. Ce motif est directement transposable à la réception par interruption UART.

---

## 7. Côté Flight Controller (Portage STM32)

Le code métier du FC ne dépend que des interfaces `ISensorInput`, `IActuatorOutput` et `IClock`. Le portage HIL consiste à fournir les implémentations STM32 sans toucher au cœur :

| Implémentation à écrire | Interface satisfaite | Pistes matérielles |
| :--- | :--- | :--- |
| `Stm32UartTransport` | `FlightCore::Transport::ITransport` | RX DMA en ring buffer, TX DMA, callbacks d'interruption |
| `DwtClock` | `FlightCore::HAL::IClock` | Compteur de cycles DWT (`DWT->CYCCNT`) converti en µs |
| `HilSensorInput` | `FlightCore::HAL::ISensorInput` | Draine le transport via `HilFrameParser`, expose le dernier `SensorData` décodé + indicateur de fraîcheur (timestamp) |
| `HilActuatorOutput` | `FlightCore::HAL::IActuatorOutput` | Encode (`makeActuatorPayload` + `encodeActuatorFrame`) et émet l'`ActuatorPacket` |

Contraintes respectées par les briques existantes pour la cible embarquée :

1. **Zéro allocation dynamique** : tous les buffers sont des `std::array` de taille fixe ; aucun tas dans les chemins temps réel.
2. **Pas d'exceptions** (`-fno-exceptions`) : les erreurs sont signalées par des valeurs de retour `bool` / compteurs, jamais par `throw`.
3. **FSM réentrante par conception simple** : `HilFrameParser` peut être alimentée octet par octet depuis une ISR UART ; son état est un ensemble fixe de membres.
4. **Types à largeur fixe** : `std::uint8_t` ... `std::uint64_t`, `std::float32_t` via `import std;` — layout wire identique entre PC (x86_64) et cible (Cortex-M), little-endian des deux côtés.

---

## 8. Garanties d'Implémentation

* **Déterminisme** : horloge virtuelle avancée explicitement (`SimulatedClock::advanceUs`), FIFO du `LoopbackTransport` déterministe — exécutions bit-reproductibles sur PC.
* **Séparation vérité terrain / mesure** : la vérité terrain (`TruthState`) reste privée du simulateur ; seul un échantillon dérivé est transmis au FC (règle 4.3 de `hil_validation.md`).
* **Intégrité** : toute trame passe par le CRC-16-CCITT du `HilFrameParser` ; les trames invalides sont rejetées sans corrompre l'état et comptabilisées (`rejectedFrames()`).
* **Mesure RTT** : l'`ActuatorPacket` échoïse `sim_timestamp_us` ; le PC calcule `RTT = t_reception - t_émission` (alerte au-delà de 15 ms, section 6.2 de `hil_protocol.md`).
* **Contrat compilé** : les `static_assert` de `HilProtocol` empêchent toute divergence silencieuse entre la doc et le code.

---

## 9. Étendre le Protocole (Exemple : Message `0x03`)

Pour ajouter un nouveau type de message (ex. `TimeSyncRequest`) :

1. Déclarer la constante `kMsgIdTimeSync = 0x03` et la structure `#pragma pack(1)` correspondante dans `HilProtocol`, avec son `static_assert` de taille.
2. Ajouter `kTimeSyncFrameSize` et les fonctions `make/decode/encode` dans un nouveau translation unit `hil_protocol_codec-time.cpp` (une seule paire d'implémentations par responsabilité, conformément aux règles du dépôt).
3. Étendre le dispatch côté réception après `receiveFrame()` en fonction de `header.msg_id` (le parser est déjà agnostique au contenu du payload).
4. Mettre à jour [`hil_protocol.md`](hil_protocol.md) (table des Msg ID, section 4) et la table du contrat de ce guide.

---

## 10. Références

* [`hil_protocol.md`](hil_protocol.md) — spécification HIL-Proto v1.0 (header, payloads, CRC, temps, erreurs)
* [`hil_architecture.md`](hil_architecture.md) — architecture matérielle cible, RTOS, HAL, validation SIL vs HIL
* [`hil_validation.md`](hil_validation.md) — stratégie de validation du banc HIL
* `CMakeLists.txt` — cible `hil_demo` et variable `HIL_FILES`
* Code : `Src/Embedded/` (modules listés en section 2)
