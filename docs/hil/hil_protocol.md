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
* **Déterminisme et Latence Bounding :** Boucle rafraîchie à $100	ext{ Hz}$ ($T = 10	ext{ ms}$). Overhead de sérialisation/désérialisation $< 200	ext{ }\mu	ext{s}$ sur STM32.
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
| **Baud Rate** | **921 600 bps** (ou $1.5	ext{ Mbps}$) | Permet de transférer ~92 KiB/s ($< 1.5	ext{ ms}$ de bus load pour $100	ext{ Hz}$) |
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
#include <cstdint>

#pragma pack(push, 1)
struct HilHeader {
    uint8_t  sync_byte_1;     // 0x48 ('H')
    uint8_t  sync_byte_2;     // 0x49 ('I')
    uint8_t  msg_id;          // Message Identifier (0x01 = Sensor, 0x02 = Actuator, etc.)
    uint8_t  protocol_ver;    // Protocol Version (0x10 = v1.0)
    uint16_t sequence_num;    // Compteur d'envoi incrémentiel (0 -> 65535)
    uint16_t payload_len;     // Longueur du payload en octets (hors Header et CRC)
};
#pragma pack(pop)
```

### 3.2 Identifiants de Messages (`MsgID`)

| Msg ID (Hex) | Nom du Message | Source | Destination | Périodicité | Description |
| :---: | :--- | :---: | :---: | :---: | :--- |
| `0x01` | `SensorPacket` | PC Host | STM32 FC1 | $100	ext{ Hz}$ ($10	ext{ ms}$) | Injecte les mesures capteurs simulées |
| `0x02` | `ActuatorPacket` | STM32 FC1 | PC Host | $100	ext{ Hz}$ ($10	ext{ ms}$) | Retourne les commandes des gouvernes/moteur |
| `0x03` | `TimeSyncRequest` | PC Host | STM32 FC1 | $1	ext{ Hz}$ / On-Demand | Synchronisation d'horloge / Ping |
| `0x04` | `TimeSyncResponse` | STM32 FC1 | PC Host | Sur requête | Réponse d'horloge avec timestamp local |
| `0xFE` | `FaultInjectionCmd`| PC Host | STM32 FC1 | Atypique | Injection de défaillances logicielles/matérielles |
| `0xFF` | `AckNackPacket` | STM32 FC1 | PC Host | Asynchrone | Accusé de réception / Erreur de protocole |

---

## 4. Spécification Complète des Payloads

### 4.1 Message `SensorPacket` (PC Host $ightarrow$ STM32 FC1)
* **Message ID:** `0x01`
* **Taille Payload:** 68 octets
* **Taille Totale Trame:** Header (8) + Payload (68) + CRC (2) = **78 octets**
* **Fréquence d'Émission:** $100	ext{ Hz}$ ($10.0	ext{ ms}$)

#### Structure Binaire C++ (`HilSensorPayload`)

```cpp
#pragma pack(push, 1)
struct HilSensorPayload {
    uint64_t sim_timestamp_us;   // Timestamp absolu de simulation PC (en microsecondes)
    
    // Position Kinematics (Repère local NED / WGS-84 tangent)
    float position_x_m;          // Position Nord (m)
    float position_y_m;          // Position Est (m)
    float position_z_m;          // Position Bas / Altitude inversée (m)
    
    // Linear Velocities (Repère Corps / Body Frame)
    float velocity_x_ms;         // Vitesse longitudinale u (m/s)
    float velocity_y_ms;         // Vitesse latérale v (m/s)
    float velocity_z_ms;         // Vitesse verticale w (m/s)
    
    // Angular Rates (Gyromètres simulés avec bruit / bias)
    float gyro_p_rad_s;          // Vitesse de roulis (rad/s)
    float gyro_q_rad_s;          // Vitesse de tangage (rad/s)
    float gyro_r_rad_s;          // Vitesse de lacet (rad/s)
    
    // Linear Accelerations (Accéléromètres simulés)
    float accel_x_m_s2;          // Accélération spécifique X (m/s²)
    float accel_y_m_s2;          // Accélération spécifique Y (m/s²)
    float accel_z_m_s2;          // Accélération spécifique Z (m/s²)
    
    // Euler Angles (Attitude estimée du banc simu)
    float roll_rad;              // Roulis (rad) [-pi, +pi]
    float pitch_rad;             // Tangage (rad) [-pi/2, +pi/2]
    float yaw_rad;               // Cap / Lacet (rad) [0, 2*pi]
    
    // Airdata / Auxiliary
    float altitude_baro_m;       // Altitude barométrique filtrée (m)
    float wing_rpm_meas;         // Mesure réelle vitesse hélice/aile (tr/min)
    
    // Health & Validity Status Flags
    uint32_t sensor_valid_flags; // Bitmask d'état de validité des capteurs simulés
};
#pragma pack(pop)
```

#### Mapping des Bits `sensor_valid_flags`
* **Bit 0 :** IMU_1_OK ($1 =$ Valide, $0 =$ Erreur / Bruité hors tolérance)
* **Bit 1 :** IMU_2_OK
* **Bit 2 :** BARO_OK
* **Bit 3 :** GPS_FIX_OK
* **Bit 4 :** TACHOMETER_OK
* **Bits 5-31 :** Réservés (doivent être mis à 0)

---

### 4.2 Message `ActuatorPacket` (STM32 FC1 $ightarrow$ PC Host)
* **Message ID:** `0x02`
* **Taille Payload:** 44 octets
* **Taille Totale Trame:** Header (8) + Payload (44) + CRC (2) = **54 octets**
* **Fréquence d'Émission:** $100	ext{ Hz}$ ($10.0	ext{ ms}$, synchrone à la réception du `SensorPacket`)

#### Structure Binaire C++ (`HilActuatorPayload`)

```cpp
#pragma pack(push, 1)
struct HilActuatorPayload {
    uint64_t fc_timestamp_us;       // Horloge locale de la STM32 (DWT cycle counter / microsecondes)
    uint64_t echo_sim_timestamp_us; // Écho exact du sim_timestamp_us reçu (Mesure RTT)
    
    // Actuator Commands
    float wing_rpm_cmd;            // Consigne de vitesse de rotation moteur/aile (tr/min)
    float left_servo_cmd_rad;      // Consigne d'angle servo gauche (rad)
    float right_servo_cmd_rad;     // Consigne d'angle servo droit (rad)
    float aux_actuator_cmd;        // Consigne auxiliaire (ex: aérofreins / volets)
    
    // Flight Controller Diagnostic Metadata
    uint32_t cpu_usage_pct_x100;   // Charge CPU en milliemes de % (ex: 1250 = 12.50 %)
    uint16_t stack_watermark_words;// Marge minimale de pile RTOS restante (mots de 32 bits)
    uint16_t deadline_miss_count;  // Compteur cumulé de dépassements d'échéance RTOS
    uint8_t  fc_mode;              // Mode de vol actif (0=INIT, 1=MANUAL, 2=AUTO, 3=FAILSAFE)
    uint8_t  fc_health_status;     // Statut global du Health Monitor (0=OK, 1=WARNING, 2=CRITICAL)
    uint16_t reserved;             // Alignement mémoire (padding 32 bits)
};
#pragma pack(pop)
```

---

## 5. Contrôle d'Intégrité et Sérialisation

### 5.1 Algorithme CRC-16-CCITT
Afin de garantir une détection rigoureuse contre le bruit sur la ligne série (octets tronqués, inversion de bits), chaque trame est suivie d'un code de contrôle d'erreur **CRC-16-CCITT** sur 16 bits.

* **Polynôme générateur :** $x^{16} + x^{12} + x^5 + 1$ (`0x1021`)
* **Valeur Initiale :** `0xFFFF`
* **Champ de Calcul :** Calculé sur la totalité du `HilHeader` + `Payload`.

```cpp
#include <cstdint>
#include <cstddef>

class HilCrc {
public:
    static uint16_t CalculateCRC16(const uint8_t* data, size_t length) {
        uint16_t crc = 0xFFFF;
        for (size_t i = 0; i < length; ++i) {
            crc ^= (static_cast<uint16_t>(data[i]) << 8);
            for (uint8_t bit = 0; bit < 8; ++bit) {
                if (crc & 0x8000) {
                    crc = (crc << 1) ^ 0x1021;
                } else {
                    crc = (crc << 1);
                }
            }
        }
        return crc;
    }
};
```

---

## 6. Modèle Temporel & Modèle de Synchronisation (Lockstep vs Real-Time)

### 6.1 Le PC comme Horloge Maître (Time Master)
En architecture HIL embarquée, deux stratégies temporelles peuvent être envisagées :
1. **Hard Real-Time Strict (Asynchrone) :** Le PC et la STM32 tournent à leurs horloges respectives.
2. **PC-Master Driven (Lockstep Synchrone) :** Le PC orchestre l'avancement global du temps de simulation.

Pour le protocole **HIL-Proto v1.0**, la stratégie **PC-Master Driven** est sélectionnée :
* La STM32 ne déclenche pas sa boucle d'asservissement `ControlTask` sur son propre timer interne, mais **à la réception complète et validée d'un `SensorPacket`**.
* Le PC envoie un `SensorPacket` toutes les $10.0	ext{ ms}$ ($100	ext{ Hz}$).
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

$$	ext{RTT} = t_{	ext{PC\_RX\_Actuator}} - t_{	ext{PC\_TX\_Sensor}}$$

$$	ext{Execution Delay (STM32)} = t_{	ext{FC\_TX}} - t_{	ext{FC\_RX\_Sensor}}$$

Si $	ext{RTT} > 15.0	ext{ ms}$, une alerte de gigue temporelle (jitter) est émise par le PC de simulation, signalant une saturation du bus série ou un débordement d'échéance RTOS sur la cible.

---

## 7. Interface d'Abstraction Matérielle (HAL C++)

Afin de garantir que le code du calculateur de vol (`FlightController`) reste totalement agnostique du support d'exécution (SIL sous Windows/Linux vs HIL sur STM32 bare-metal/FreeRTOS), une interface C++ pure est définie : `ITransportStream`.

```cpp
#pragma once
#include <cstdint>
#include <cstddef>

namespace hil {

class ITransportStream {
public:
    virtual ~ITransportStream() = default;
    
    // Transmet un tampon de données de façon synchrone ou non-bloquante
    virtual bool Write(const uint8_t* data, size_t length) = 0;
    
    // Lit jusqu'à max_length octets depuis le buffer de réception
    virtual size_t Read(uint8_t* buffer, size_t max_length) = 0;
    
    // Indique le nombre d'octets disponibles en lecture
    virtual size_t BytesAvailable() const = 0;
};

} // namespace hil
```

### 7.1 Parser de Trame Robuste (State Machine)
La réception des trames sur STM32 s'effectue via une machine à états finis (FSM) pour éviter tout blocage ou décalage de buffer en cas d'octet parasite :

```cpp
namespace hil {

enum class ParseState {
    WAIT_SYNC_1,
    WAIT_SYNC_2,
    READ_HEADER,
    READ_PAYLOAD,
    READ_CRC
};

class HilFrameParser {
private:
    ParseState state_ = ParseState::WAIT_SYNC_1;
    HilHeader header_{};
    uint8_t payload_buffer_[128];
    size_t bytes_read_ = 0;
    uint16_t received_crc_ = 0;

public:
    bool ProcessByte(uint8_t byte, HilHeader& out_header, uint8_t* out_payload) {
        switch (state_) {
            case ParseState::WAIT_SYNC_1:
                if (byte == 0x48) state_ = ParseState::WAIT_SYNC_2;
                break;

            case ParseState::WAIT_SYNC_2:
                if (byte == 0x49) {
                    state_ = ParseState::READ_HEADER;
                    bytes_read_ = 2; // Sync bytes déja consommés
                    header_.sync_byte_1 = 0x48;
                    header_.sync_byte_2 = 0x49;
                } else {
                    state_ = ParseState::WAIT_SYNC_1;
                }
                break;

            case ParseState::READ_HEADER:
                reinterpret_cast<uint8_t*>(&header_)[bytes_read_++] = byte;
                if (bytes_read_ == sizeof(HilHeader)) {
                    if (header_.payload_len > sizeof(payload_buffer_)) {
                        state_ = ParseState::WAIT_SYNC_1; // TRAME INVALIDE
                    } else {
                        bytes_read_ = 0;
                        state_ = ParseState::READ_PAYLOAD;
                    }
                }
                break;

            case ParseState::READ_PAYLOAD:
                payload_buffer_[bytes_read_++] = byte;
                if (bytes_read_ == header_.payload_len) {
                    bytes_read_ = 0;
                    state_ = ParseState::READ_CRC;
                }
                break;

            case ParseState::READ_CRC:
                reinterpret_cast<uint8_t*>(&received_crc_)[bytes_read_++] = byte;
                if (bytes_read_ == 2) {
                    state_ = ParseState::WAIT_SYNC_1;
                    
                    // Vérification du CRC
                    uint16_t computed_crc = HilCrc::CalculateCRC16(
                        reinterpret_cast<const uint8_t*>(&header_), sizeof(HilHeader));
                    computed_crc = HilCrc::CalculateCRC16(payload_buffer_, header_.payload_len); // Accumulation
                    
                    // Remarque: Dans la pratique, le calcul CRC englobe Header + Payload
                    if (computed_crc == received_crc_) {
                        out_header = header_;
                        std::memcpy(out_payload, payload_buffer_, header_.payload_len);
                        return true; // Trame valide extraite
                    }
                }
                break;
        }
        return false;
    }
};

} // namespace hil
```

---

## 8. Procédure de Traitement des Erreurs et Modes Degradés

### 8.1 Perte de Communication (Watchdog Protocol)
Un **Watchdog Télémétrique** est configuré sur la STM32 FC1 :
* Si aucun message `SensorPacket` valide n'est reçu pendant un intervalle $\Delta t > 50.0	ext{ ms}$ (soit 5 trames consécutives perdues) :
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
| **TEST-PROT-01** | Nominal Frame Rate | Envoi continu à $100	ext{ Hz}$ pendant $10	ext{ minutes}$ ($60	ext{ }000$ trames) | $0$ pertes de trames, $0$ erreur CRC, RTT moyen $< 2.5	ext{ ms}$ |
| **TEST-PROT-02** | Noise Resilience | Injection de $1	ext{ \%}$ d'octets corrompus aléatoires sur le bus série | Rejet intégral des trames altérées par le parser, absence de crash RTOS |
| **TEST-PROT-03** | Communication Timeout | Interruption brutale du câble USB pendant $200	ext{ ms}$ | Transition confirmée de FC1 en mode `FAILSAFE` sous $50	ext{ ms}$ |
| **TEST-PROT-04** | Latency Measurement | Écho systématique des timestamps entre PC et STM32 | Traitement local STM32 (unpack -> control -> pack) $< 1.0	ext{ ms}$ |

---
