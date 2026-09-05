# HIL Protocol Specification — PC / STM32 Communication

**Document ID:** SPEC-HIL-PROT-001  
**Version:** 1.0.0  
**Date:** 5 Septembre 2026  
**Auteur:** Nicolas Keita — Embedded Flight Control Systems Architect  
**Cible Hardware:** STM32 Flight Controller (FC1) / PC Host Simulation  

---

## 1. Introduction et Objet du Document

### 1.1 Contexte
Dans le cadre du développement du système de commande de vol (Flight Control System), la transition du Software-in-the-Loop (SIL) vers le Hardware-in-the-Loop (HIL) constitue l'étape fondamentale de validation embarquée. Le calculateur physique (**STM32 FC1**) exécute son code de vol de production sous RTOS (`SensorTask`, `ControlTask`, `MissionTask`, `HealthTask`), tandis que l'aéronef, la dynamique du vol, l'environnement physique et la chaîne d'acquisition/actionnement restent intégralement simulés sur un PC hôte.

```
                      PC HOST (Simulation Master)
      ┌─────────────────────────────────────────────────────────┐
      │  Aircraft Dynamics (6-DOF) + Environment + Sensors      │
      └───────────────────────────┬─────────────────────────────┘
                                  │
                                  │ Physical Serial Transport
                                  │ (UART / USB CDC / Baud: 921600)
                                  │
                                  ▼
      ┌─────────────────────────────────────────────────────────┐
      │  STM32 FC1 (Real Hardware / RTOS / Control Loop 100 Hz) │
      └─────────────────────────────────────────────────────────┘
```

### 1.2 Objet
Le présent document définit la spécification formelle du protocole de communication binaire point-à-point **HIL-Proto v1.0**. Ce protocole régit tous les échanges de données télémétriques, commandes d'actionneurs, signaux de synchronisation et métadonnées de diagnostic entre le PC de simulation et le calculateur embarqué STM32.

### 1.3 Exigences Clés du Protocole
* **Déterminisme et Latence Bounding :** Boucle rafraîchie à $100\text{ Hz}$ ($T = 10\text{ ms}$). Overhead de sérialisation/désérialisation $< 200\text{ }\mu\text{s}$ sur STM32.
* **Intégrité Totale :** Détection d'erreurs systématique via CRC-16-CCITT sur chaque trame.
* **Zéro Allocation Dynamique :** Structure binaire à taille fixe, directement mappable en mémoire C++20 sans fragmentation du tas (heap).
* **Endianness Fixe :** Format Little-Endian (ARM Cortex-M / x86_64 natif).

---

## 2. Architecture de la Couche Transport & Encapsulation

### 2.1 Couche Physique et Liaison
Le protocole est conçu pour s'exécuter sur un lien série point-à-point bidirectionnel full-duplex.

| Paramètre | Spécification Métier HIL | Note / Justification |
| :--- | :--- | :--- |
| **Interface Physique** | UART via USB-FTDI ou USB CDC ACM | Port série virtuel à haute vitesse |
| **Baud Rate** | **921 600 bps** (ou $1.5\text{ Mbps}$) | Permet de transférer ~92 KiB/s ($< 1.5\text{ ms}$ de bus load pour $100\text{ Hz}$) |
| **Format Série** | 8N1 (8 bits data, No parity, 1 stop bit) | Standard matériel universel |
| **Framing Method** | Délimiteurs de synchronisation + CRC | En-tête binaire explicite `0x48`, `0x49` ('H', 'I') |

### 2.2 Format Général d'une Trame
Toute trame transitant sur le bus HIL respecte la structure binaire rigide ci-dessous :

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|          Sync Bytes           |    Msg ID     | Proto Version |
|     0x48 ('H')  |  0x49 ('I') |  (0x01/0x02)  |    (0x10)     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|        Sequence Number        |        Payload Length         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
~                        PAYLOAD DATA                           ~
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                            CRC-16                             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

---

## 3. Définition des En-têtes (Header Specification)

La structure de l'en-tête est strictement identique pour tous les types de messages. Elle mesure **8 octets**.

### 3.1 Structure du Header (`HilHeader`)

```cpp
// Extrait de Src/embedded/transport/hil_protocol.cppm (module flight.transport.protocol)
#pragma pack(push, 1)
struct HilHeader
{
    std::uint8_t  sync_byte_1;   // 0x48 ('H') - kSync1
    std::uint8_t  sync_byte_2;   // 0x49 ('I') - kSync2
    std::uint8_t  msg_id;        // Message Identifier (0x01 = Sensor, 0x02 = Actuator)
    std::uint8_t  protocol_ver;  // Protocol Version (0x10 = v1.0) - kProtocolVer
    std::uint16_t sequence_num;  // Compteur d'envoi incrémentiel (0 -> 65535)
    std::uint16_t payload_len;   // Longueur du payload en octets (hors Header et CRC)
};
#pragma pack(pop)

static_assert(sizeof(HilHeader) == 8);
```

### 3.2 Identifiants de Messages (`MsgID`)

| Msg ID (Hex) | Nom du Message | Source | Destination | Périodicité | Description |
| :---: | :--- | :---: | :---: | :---: | :--- |
| `0x01` | `SensorPacket` | PC Host | STM32 FC1 | $100\text{ Hz}$ ($10\text{ ms}$) | Injecte les mesures capteurs simulées |
| `0x02` | `ActuatorPacket` | STM32 FC1 | PC Host | $100\text{ Hz}$ ($10\text{ ms}$) | Retourne les commandes des gouvernes/moteur |
| `0x03` | `TimeSyncRequest` | PC Host | STM32 FC1 | $1\text{ Hz}$ / On-Demand | Synchronisation d'horloge / Ping |
| `0x04` | `TimeSyncResponse` | STM32 FC1 | PC Host | Sur requête | Réponse d'horloge avec timestamp local |
| `0xFE` | `FaultInjectionCmd`| PC Host | STM32 FC1 | Atypique | Injection de défaillances logicielles/matérielles |
| `0xFF` | `AckNackPacket` | STM32 FC1 | PC Host | Asynchrone | Accusé de réception / Erreur de protocole |

> **État d'implémentation (skeleton étape 13) :** seuls `0x01` (`SensorPacket`) et `0x02` (`ActuatorPacket`) sont définis et traités par le code (`kMsgIdSensor` / `kMsgIdActuator` du module `flight.transport.protocol`). Les identifiants `0x03`, `0x04`, `0xFE` et `0xFF` sont réservés pour les étapes ultérieures et ne sont pas encore pris en charge par le parser ni par le codec.

---

## 4. Spécification Complète des Payloads

### 4.1 Message `SensorPacket` (PC Host $\rightarrow$ STM32 FC1)
* **Message ID:** `0x01`
* **Taille Payload:** 80 octets (`kSensorPayloadSize`)
* **Taille Totale Trame:** Header (8) + Payload (80) + CRC (2) = **90 octets** (`kSensorFrameSize`)
* **Fréquence d'Émission:** $100\text{ Hz}$ ($10.0\text{ ms}$)

> **Note de dimensionnement :** la somme des 17 champs `float` vaut bien 68 octets ; le payload complet y ajoute le timestamp 64 bits `sim_timestamp_us` (8 octets) et le bitmask 32 bits `sensor_valid_flags` (4 octets), d'où un total de 80 octets. Cette taille est vérifiée à la compilation par `static_assert(sizeof(HilSensorPayload) == 80)`. Le champ `payload_len` de l'en-tête étant lu à l'exécution, le parser reste agnostique à la longueur du payload.

#### Structure Binaire C++ (`HilSensorPayload`)

```cpp
// Extrait de Src/embedded/transport/hil_protocol.cppm (module flight.transport.protocol)
#pragma pack(push, 1)
struct HilSensorPayload
{
    std::uint64_t  sim_timestamp_us;   // Timestamp absolu de simulation PC (en microsecondes)

    // Position Kinematics (Repère local NED / WGS-84 tangent)
    std::float32_t position_x_m;       // Position Nord (m)
    std::float32_t position_y_m;       // Position Est (m)
    std::float32_t position_z_m;       // Position Bas / Altitude inversée (m)

    // Linear Velocities (Repère Corps / Body Frame)
    std::float32_t velocity_x_ms;      // Vitesse longitudinale u (m/s)
    std::float32_t velocity_y_ms;      // Vitesse latérale v (m/s)
    std::float32_t velocity_z_ms;      // Vitesse verticale w (m/s)

    // Angular Rates (Gyromètres simulés avec bruit / bias)
    std::float32_t gyro_p_rad_s;       // Vitesse de roulis (rad/s)
    std::float32_t gyro_q_rad_s;       // Vitesse de tangage (rad/s)
    std::float32_t gyro_r_rad_s;       // Vitesse de lacet (rad/s)

    // Linear Accelerations (Accéléromètres simulés)
    std::float32_t accel_x_m_s2;       // Accélération spécifique X (m/s²)
    std::float32_t accel_y_m_s2;       // Accélération spécifique Y (m/s²)
    std::float32_t accel_z_m_s2;       // Accélération spécifique Z (m/s²)

    // Euler Angles (Attitude estimée du banc simu)
    std::float32_t roll_rad;           // Roulis (rad) [-pi, +pi]
    std::float32_t pitch_rad;          // Tangage (rad) [-pi/2, +pi/2]
    std::float32_t yaw_rad;            // Cap / Lacet (rad) [0, 2*pi]

    // Airdata / Auxiliary
    std::float32_t altitude_baro_m;    // Altitude barométrique filtrée (m)
    std::float32_t wing_rpm_meas;      // Mesure réelle vitesse hélice/aile (tr/min)

    // Health & Validity Status Flags
    std::uint32_t  sensor_valid_flags; // Bitmask d'état de validité des capteurs simulés
};
#pragma pack(pop)

static_assert(sizeof(HilSensorPayload) == 80);
```

#### Mapping des Bits `sensor_valid_flags`
* **Bit 0 :** IMU_1_OK ($1 =$ Valide, $0 =$ Erreur / Bruité hors tolérance)
* **Bit 1 :** IMU_2_OK
* **Bit 2 :** BARO_OK
* **Bit 3 :** GPS_FIX_OK
* **Bit 4 :** TACHOMETER_OK
* **Bits 5-31 :** Réservés (doivent être mis à 0)

---

### 4.2 Message `ActuatorPacket` (STM32 FC1 $\rightarrow$ PC Host)
* **Message ID:** `0x02`
* **Taille Payload:** 44 octets
* **Taille Totale Trame:** Header (8) + Payload (44) + CRC (2) = **54 octets**
* **Fréquence d'Émission:** $100\text{ Hz}$ ($10.0\text{ ms}$, synchrone à la réception du `SensorPacket`)

#### Structure Binaire C++ (`HilActuatorPayload`)

```cpp
// Extrait de Src/embedded/transport/hil_protocol.cppm (module flight.transport.protocol)
#pragma pack(push, 1)
struct HilActuatorPayload
{
    std::uint64_t  fc_timestamp_us;        // Horloge locale de la STM32 (DWT cycle counter / microsecondes)
    std::uint64_t  echo_sim_timestamp_us;  // Écho exact du sim_timestamp_us reçu (Mesure RTT)

    // Actuator Commands
    std::float32_t wing_rpm_cmd;           // Consigne de vitesse de rotation moteur/aile (tr/min)
    std::float32_t left_servo_cmd_rad;     // Consigne d'angle servo gauche (rad)
    std::float32_t right_servo_cmd_rad;    // Consigne d'angle servo droit (rad)
    std::float32_t aux_actuator_cmd;       // Consigne auxiliaire (ex: aérofreins / volets)

    // Flight Controller Diagnostic Metadata
    std::uint32_t  cpu_usage_pct_x100;     // Charge CPU en centièmes de % (ex: 1250 = 12.50 %)
    std::uint16_t  stack_watermark_words;  // Marge minimale de pile RTOS restante (mots de 32 bits)
    std::uint16_t  deadline_miss_count;    // Compteur cumulé de dépassements d'échéance RTOS
    std::uint8_t   fc_mode;                // Mode de vol actif (0=INIT, 1=MANUAL, 2=AUTO, 3=FAILSAFE)
    std::uint8_t   fc_health_status;       // Statut global du Health Monitor (0=OK, 1=WARNING, 2=CRITICAL)
    std::uint16_t  reserved;               // Alignement mémoire (padding 32 bits)
};
#pragma pack(pop)

static_assert(sizeof(HilActuatorPayload) == 44);
```

> **Remarque d'implémentation :** côté C++, les champs de diagnostic sont alimentés par la structure hôte `ActuatorDiagnostics` (même module), remplie par la `HealthTask` avant l'encodage de la trame via `makeActuatorPayload`.

---

## 5. Contrôle d'Intégrité et Sérialisation

### 5.1 Algorithme CRC-16-CCITT
Afin de garantir une détection rigoureuse contre le bruit sur la ligne série (octets tronqués, inversion de bits), chaque trame est suivie d'un code de contrôle d'erreur **CRC-16-CCITT** sur 16 bits.

* **Polynôme générateur :** $x^{16} + x^{12} + x^5 + 1$ (`0x1021`)
* **Valeur Initiale :** `0xFFFF`
* **Champ de Calcul :** Calculé sur la totalité du `HilHeader` + `Payload`.

```cpp
// Extraits de Src/embedded/transport/parser/hil_protocol_parser.cppm (interface,
// module flight.transport.protocol.parser) et hil_protocol_parser-crc.cpp
// (implémentation bitwise).
import std;

class HilCrc
{
public:
    HilCrc() = delete;

    /* CRC-16-CCITT (poly 0x1021, init 0xFFFF) sur un buffer unique. */
    [[nodiscard]] static constexpr std::uint16_t compute(std::span<const std::uint8_t> data) noexcept
    {
        return update(0xFFFFu, data);
    }

    /* Poursuit un CRC en cours avec des octets supplémentaires (Header puis Payload). */
    [[nodiscard]] static std::uint16_t update(std::uint16_t crc, std::span<const std::uint8_t> data) noexcept;
};
```

> **Sérialisation :** les encodeurs (`encodeSensorFrame` / `encodeActuatorFrame` du module `flight.transport.protocol.codec`) écrivent le CRC en **little-endian** (octet de poids faible d'abord) dans les deux derniers octets de la trame, et le calcul CRC couvre `HilHeader` + `Payload` (pas les octets de CRC eux-mêmes).

---

## 6. Modèle Temporel & Modèle de Synchronisation (Lockstep vs Real-Time)

### 6.1 Le PC comme Horloge Maître (Time Master)
En architecture HIL embarquée, deux stratégies temporelles peuvent être envisagées :
1. **Hard Real-Time Strict (Asynchrone) :** Le PC et la STM32 tournent à leurs horloges respectives.
2. **PC-Master Driven (Lockstep Synchrone) :** Le PC orchestre l'avancement global du temps de simulation.

Pour le protocole **HIL-Proto v1.0**, la stratégie **PC-Master Driven** est sélectionnée :
* La STM32 ne déclenche pas sa boucle d'asservissement `ControlTask` sur son propre timer interne, mais **à la réception complète et validée d'un `SensorPacket`**.
* Le PC envoie un `SensorPacket` toutes les $10.0\text{ ms}$ ($100\text{ Hz}$).
* La STM32 traite la consigne, exécute l'algorithme de contrôle, et renvoie immédiatement l' `ActuatorPacket`.

```
PC Host Simulation               Physical Transport               STM32 FC1 RTOS
───────┬──────────               ──────────────────               ───────┬──────
       │                                                                 │
[SimStep t=10ms]                                                         │
       │─────── SensorPacket (sim_t=10000us) ───────────────────────────>│ (UART RX Interrupt)
       │                                                                 │   │ Unpack & CRC Check
       │                                                                 │   ▼
       │                                                                 │ [Unblock SensorTask]
       │                                                                 │   │
       │                                                                 │   ▼
       │                                                                 │ [Execute ControlTask]
       │                                                                 │   │
       │                                                                 │   ▼
       │<────── ActuatorPacket (echo_sim_t=10000us) ─────────────────────│ [Send DMA TX]
       │                                                                 │
 [Compute RTT]                                                           │
[Advance Sim to t=20ms]                                                  │
       │                                                                 │
```

### 6.2 Mesure du Round Trip Time (RTT) et Jitter
Le PC calcule en continu la latence de boucle fermée grâce au champ `echo_sim_timestamp_us` :

$$\text{RTT} = t_{\text{PC\_RX\_Actuator}} - t_{\text{PC\_TX\_Sensor}}$$

$$\text{Execution Delay (STM32)} = t_{\text{FC\_TX}} - t_{\text{FC\_RX\_Sensor}}$$

Si $\text{RTT} > 15.0\text{ ms}$, une alerte de gigue temporelle (jitter) est émise par le PC de simulation, signalant une saturation du bus série ou un débordement d'échéance RTOS sur la cible.

---

## 7. Interface d'Abstraction Matérielle (HAL C++)

Afin de garantir que le code du calculateur de vol (`FlightController`) reste totalement agnostique du support d'exécution (SIL sous Windows/Linux vs HIL sur STM32 bare-metal/FreeRTOS), des interfaces C++ pures sont définies en modules C++23. La couche transport vit dans `flight.transport` (namespace `FlightCore::Transport`) ; les abstractions capteurs, actionneurs et horloge vivent dans `flight.hal.sensor`, `flight.hal.actuator` et `flight.hal.clock` (namespace `FlightCore::HAL`).

```cpp
// Extrait de Src/embedded/transport/transport.cppm (module flight.transport)
import std;

namespace FlightCore::Transport
{

class ITransport
{
public:
    virtual ~ITransport() = default;

    // Transmet une trame ; retourne false si le canal ne peut pas l'accepter entièrement
    [[nodiscard]] virtual bool sendBytes(std::span<const std::uint8_t> data) noexcept = 0;

    // Lit jusqu'à buffer.size() octets disponibles (jamais bloquant)
    [[nodiscard]] virtual std::size_t receiveBytes(std::span<std::uint8_t> buffer) noexcept = 0;

    [[nodiscard]] virtual std::size_t bytesAvailable() const noexcept = 0;

    virtual void flush() noexcept {}
};

// Vue de flux brut nommée dans la spécification, modernisée avec std::span.
// Réservée au futur driver UART STM32.
class ITransportStream
{
public:
    virtual ~ITransportStream() = default;

    [[nodiscard]] virtual bool write(std::span<const std::uint8_t> data) noexcept = 0;

    [[nodiscard]] virtual std::size_t read(std::span<std::uint8_t> buffer) noexcept = 0;

    [[nodiscard]] virtual std::size_t bytesAvailable() const noexcept = 0;
};

}
```

Le cœur de contrôle dépend quant à lui des trois interfaces HAL suivantes, satisfaites à la fois par les mocks PC (`flight.sim.*`) et par les futures implémentations STM32 :

| Interface | Module | Méthode | Sémantique d'erreur |
| :--- | :--- | :--- | :--- |
| `ISensorInput` | `flight.hal.sensor` | `readSensorData(SensorData&)` | `false` si aucun échantillon frais (non bloquant) |
| `IActuatorOutput` | `flight.hal.actuator` | `writeActuatorCommands(const ActuatorCommands&)` | `false` si le canal aval ne peut pas accepter la commande |
| `IClock` | `flight.hal.clock` | `nowUs()` / `sleepUs(us)` | Horloge monotone en µs (timer HW / DWT sur cible, horloge virtuelle sur PC) |

### 7.1 Parser de Trame Robuste (State Machine)
La réception des trames s'effectue via une machine à états finis (FSM) alimentée octet par octet, pour éviter tout blocage ou décalage de buffer en cas d'octet parasite. L'implémentation réelle vit dans le module `flight.transport.protocol.parser` (`Src/embedded/transport/parser/`) :

```cpp
// Extrait de Src/embedded/transport/parser/hil_protocol_parser.cppm
class HilFrameParser
{
public:
    enum class State : std::uint8_t
    {
        WaitSync1,
        WaitSync2,
        ReadHeader,
        ReadPayload,
        ReadCrc
    };

    /* Retourne le parser en chasse de sync sans remettre à zéro le compteur de rejets. */
    void reset() noexcept;

    /*
        Consomme un octet ; retourne true lorsqu'une trame complète, CRC-valide,
        a été assemblée (copie du header dans out_header et du payload dans
        out_payload, qui doit contenir au moins header.payload_len octets).
    */
    [[nodiscard]] bool processByte(std::uint8_t byte, HilHeader& out_header, std::span<std::uint8_t> out_payload);

    [[nodiscard]] std::uint64_t rejectedFrames() const noexcept;

private:
    State                                 state_{State::WaitSync1};
    HilHeader                             header_{};
    std::array<std::uint8_t, kHeaderSize> header_bytes_{};
    std::array<std::uint8_t, kMaxPayload> payload_buffer_{};
    std::array<std::uint8_t, kCrcSize>    crc_bytes_{};
    std::size_t                           index_{0};
    std::uint64_t                         rejected_{0};
};
```

Comportements clés de la FSM (`hil_protocol_parser-sync.cpp` / `hil_protocol_parser-frame.cpp`) :

1. **Chasse de synchronisation (`WaitSync1` / `WaitSync2`) :** les octets `0x48` puis `0x49` sont consommés ; un `0x48` reçu alors que la FSM attend `0x49` relance la chasse à partir de `WaitSync2`. Les octets de sync font partie du header reconstitué (`header_bytes_[0..1]`, `index_ = 2`).
2. **Validation de l'en-tête (`ReadHeader`) :** à réception des 8 octets, un `payload_len > kMaxPayload` provoque le rejet immédiat de la trame (incrémentation de `rejected_` et retour en `WaitSync1`) ; un `payload_len == 0` saute directement à `ReadCrc`.
3. **Accumulation (`ReadPayload` / `ReadCrc`) :** le payload est accumulé dans un buffer statique de `kMaxPayload` octets, puis le CRC reçu (little-endian) est comparé au CRC-16-CCITT calculé sur Header + Payload (accumulation via `HilCrc::compute` puis `HilCrc::update`).
4. **Zéro allocation dynamique :** tous les buffers sont des `std::array` membres de taille fixe ; la FSM ne bloque jamais et peut être alimentée octet par octet depuis une interruption UART.
5. **Compteur de rejets :** `rejectedFrames()` expose le nombre total de trames invalides pour la `HealthTask` (équivalent du compteur `bus_error_count`).

---

## 8. Procédure de Traitement des Erreurs et Modes Degradés

### 8.1 Perte de Communication (Watchdog Protocol)
Un **Watchdog Télémétrique** est configuré sur la STM32 FC1 :
* Si aucun message `SensorPacket` valide n'est reçu pendant un intervalle $\Delta t > 50.0\text{ ms}$ (soit 5 trames consécutives perdues) :
  1. Le `HealthTask` de la STM32 déclare l'état **HIL_COMM_LOST**.
  2. Le Flight Controller bascule automatiquement le système en mode **SAFE_RECOVERY / HOVER_HOLD** (maintien d'assiette nulle, réduction progressive du RPM).
  3. Le champ `fc_health_status` de l' `ActuatorPacket` repasse à `CRITICAL` dès le rétablissement du lien.

### 8.2 Corruption de Trame (CRC Mismatch)
* Les trames dont le CRC est invalide sont immédiatement rejetées sans altérer les structures de données internes du système de contrôle.
* Le compteur global d'erreurs de bus `bus_error_count` est incrémenté sur la cible pour évaluation statistique dans `HealthTask`.

---

## 9. Matrice de Validation du Protocole HIL

| ID Test | Désignation | Condition de Test | Critère de Succès |
| :---: | :--- | :--- | :--- |
| **TEST-PROT-01** | Nominal Frame Rate | Envoi continu à $100\text{ Hz}$ pendant $10\text{ minutes}$ ($60\text{ }000$ trames) | $0$ pertes de trames, $0$ erreur CRC, RTT moyen $< 2.5\text{ ms}$ |
| **TEST-PROT-02** | Noise Resilience | Injection de $1\text{ \%}$ d'octets corrompus aléatoires sur le bus série | Rejet intégral des trames altérées par le parser, absence de crash RTOS |
| **TEST-PROT-03** | Communication Timeout | Interruption brutale du câble USB pendant $200\text{ ms}$ | Transition confirmée de FC1 en mode `FAILSAFE` sous $50\text{ ms}$ |
| **TEST-PROT-04** | Latency Measurement | Écho systématique des timestamps entre PC et STM32 | Traitement local STM32 (unpack -> control -> pack) $< 1.0\text{ ms}$ |

---
