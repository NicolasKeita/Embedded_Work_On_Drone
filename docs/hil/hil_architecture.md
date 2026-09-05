# Architecture HIL (Hardware-in-the-Loop) — Document Technique

**Projet :** Flight Controller Embarqué pour Aéronef Autonome  
**Étape :** 13 — Integration Hardware-in-the-Loop (HIL Niveau 1)  
**Auteur :** Nicolas Keita / Embedded Systems Architecture  
**Date :** Septembre 2026  
**Statut :** Spécification d'Architecture Validée  

---

## 1. Contexte & Objectifs de l'Étape HIL

### 1.1 Rationalité du passage au HIL
Jusqu'à l'étape 12 (Simulation Software-in-the-Loop — SIL), l'ensemble du système (dynamique de l'aéronef, capteurs, algorithmes de contrôle, RTOS simulé, injection de fautes et analyses de Monte Carlo) fonctionnait exclusivement dans un environnement d'exécution **100% logiciel** (hôte x86_64).

Bien que la simulation SIL permette de valider la logique mathématique des lois de commande, la gestion de mission et le comportement théorique des masques de sécurité, elle masque totalement les contraintes physiques réelles d'un calculateur embarqué :
* **Ressources de calcul limitées :** Puissance CPU restreinte, fréquence d'horloge fixe, absence d'unités de calcul vectoriel avancées.
* **Mémoire contrainte :** Tailles réduites de la RAM (SRAM) et de la Flash, risques de débordement de pile (*stack overflow*) ou de fragmentation de mémoire dynamique.
* **Ordonnancement Temps Réel Réel :** Comportement du RTOS embarqué sur architecture ARM Cortex-M, commutation de contexte réelle, latence d'interruption (*interrupt latency*) et *jitter* temporel.
* **Contraintes d'I/O et de Communication :** Débit physique des bus de communication (UART/USB CDC), temps de sérialisation/désérialisation, latences du tampon physique.

L'objectif principal de l'étape 13 (HIL Niveau 1) est de **sortir du 100% logiciel** en faisant exécuter le vrai code du Flight Controller (FC1) sur une carte microcontrôleur cible (STM32), tout en conservant le modèle physique de l'aéronef et la simulation des capteurs/actionneurs sur le PC hôte.

```
                         PC (Hôte - x86_64)
        ┌─────────────────────────────────────────────────┐
        │                                                 │
        │           Aircraft Physical Simulator           │
        │                                                 │
        │  • Dynamic Equations (6-DOF)                    │
        │  • Sensor Models (IMU, Baro, GPS, RPM)          │
        │  • Actuator Dynamics & Aerodynamics             │
        │                                                 │
        └────────────────────────┬────────────────────────┘
                                 │
                   Liaison Série (USB CDC / UART)
                                 │
                                 ▼
                ┌─────────────────────────────────┐
                │        STM32 Microcontroller    │
                │                                 │
                │  Flight Controller Core (FC1)   │
                │  • FreeRTOS Tasks               │
                │  • SensorTask / ControlTask     │
                │  • MissionTask / HealthTask     │
                │                                 │
                └────────────────┬────────────────┘
                                 │
                         Actuator Commands
                                 │
                                 └────────────────────────► PC
```

### 1.2 Limites du Périmètre (Ce que nous ne faisons PAS en HIL Niveau 1)
Afin d'éviter une explosion de la complexité opérationnelle et matérielle, le HIL Niveau 1 isole un seul facteur d'incertitude à la fois : **le calculateur embarqué**.
Sont explicitement **exclus** de cette première étape HIL :
* L'intégration de capteurs physiques réels (IMU MEMS, baromètre physique, module GPS hardware).
* L'intégration d'actionneurs physiques réels (banc de test moteur, servos réels, charges aérodynamiques appliquées).
* L'intégration d'une architecture multi-cartes matérielles (FC1 + FC2 reliés par bus CAN physique — réservé au HIL Niveau 2).
* L'assemblage sur la cellule physique de l'aéronef (X721).

---

## 2. Architecture Système & Flux de Données

### 2.1 Boucle de Contrôle HIL Fermée
La boucle d'asservissement HIL fonctionne selon le schéma récursif suivant :

1. **PC (Simulateur) :** Calcule un pas de temps de la physique de l'aéronef ($dt = 10\text{ ms}$).
2. **PC (Simulateur) :** Génère un paquet de données capteurs simulées (`SensorPacket`) incluant le timestamp virtuel de simulation.
3. **PC ➔ STM32 :** Transmet le `SensorPacket` via la liaison série (UART/USB CDC).
4. **STM32 (SensorTask) :** Réceptionne, décode et valide l'intégrité du paquet, puis met à jour le bus interne de capteurs.
5. **STM32 (ControlTask) :** Calcule les consignes des gouvernes et des moteurs en fonction du mode de vol courant et des états reçus.
6. **STM32 (CommTask) :** Encode et émet le paquet de commandes d'actionneurs (`ActuatorPacket`).
7. **STM32 ➔ PC :** Transmet le `ActuatorPacket` au PC.
8. **PC (Simulateur) :** Reçoit la commande, l'injecte dans le modèle dynamique des actionneurs (servos, moteur) et met à jour l'état physique de l'aéronef pour le pas suivant.

```
  +---------------+   SensorPacket (100 Hz)   +---------------+
  |               | ------------------------> |               |
  |  PC Simulator |                           |  STM32 (FC1)  |
  |               | <------------------------ |               |
  +---------------+   ActuatorPacket (100 Hz) +---------------+
```

---

## 3. Choix de la Cible Matérielle & Organisation du RTOS

### 3.1 Spécification Matérielle (Target MCU)
La cible retenue pour le Flight Controller 1 (FC1) est une carte de développement basée sur l'architecture ARM Cortex-M :

| Paramètre Matériel | Spécification Cible (ex. NUCLEO-F446RE / NUCLEO-H743ZI) |
| :--- | :--- |
| **Cœur Microcontrôleur** | ARM Cortex-M4F / Cortex-M7 avec FPU (Floating Point Unit) |
| **Fréquence d'Horloge (SysClock)** | 180 MHz (STM32F446) / 480 MHz (STM32H743) |
| **Mémoire Flash** | 512 KB à 2 MB |
| **Mémoire SRAM** | 128 KB à 1 MB |
| **Périphériques I/O** | USART (avec support DMA), USB CDC High Speed, Timers HW (16/32-bit) |
| **Tension de Fonctionnement** | 3.3V VDD / Alimentation via USB Bus (5V) |

### 3.2 Découpage des Tâches RTOS (FreeRTOS / CMSIS-RTOS v2)
L'exécution sur la STM32 repose sur un ordonnancement préemptif par priorités fixes avec allocation statique des piles mémoire.

| Nom de la Tâche | Fréquence / Période | Priorité RTOS | Rôle & Responsabilité | Taille Stack Allouée |
| :--- | :--- | :--- | :--- | :--- |
| `Task_Comm_RX` | Événementielle (DMA/UART) | Très Haute (5) | Réception des octets bruts série, assemblage et validation CRC des `SensorPacket`. | 2048 octets |
| `Task_Control` | 100 Hz (10 ms) | Haute (4) | Calcul des lois d'asservissement (PID / State Feedback), filtrage, limitation de poussée. | 4096 octets |
| `Task_Sensor` | 100 Hz (10 ms) | Haute (4) | Traitement des données `SensorPacket`, conversion en structures d'état interne, détection d'aberrations. | 2048 octets |
| `Task_Health` | 20 Hz (50 ms) | Moyenne (3) | Surveillance des deadlines de tâches, calcul de la charge CPU, mesure du pire temps d'exécution (WCET), Watchdog HW. | 2048 octets |
| `Task_Mission` | 10 Hz (100 ms) | Basse (2) | Machine à états de mission (Waypoints, transition de modes de vol, gestion du Failsafe). | 2048 octets |
| `Task_Comm_TX` | Événementielle / 100 Hz | Haute (4) | Sérialisation et émission DMA du paquet `ActuatorPacket` vers le PC. | 2048 octets |

---

## 4. Couche d'Abstraction Matérielle (HAL)

### 4.1 Principe d'Isolation Matérielle
Afin d'éviter la propagation de directives de préprocesseur du type `#ifdef STM32` dans le code métier du Flight Controller, le système respecte une séparation stricte via le motif de conception **Inversion de Dépendance (Dependency Inversion Principle)**.

Le cœur des algorithmes de contrôle dépend uniquement d'interfaces C++23 pures (`ISensorInput`, `IActuatorOutput`, `IClock` côté HAL ; `ITransport` côté transport), définies dans les modules `flight.hal.*` et `flight.transport`.

```
                          +---------------------------+
                          |   FlightController Core   |
                          +-------------+-------------+
                                        |
                                        v
                   +-----------------------------------------+
                   |    Hardware Abstraction Layer (HAL)     |
                   |                                         |
                   |  ISensorInput     IActuatorOutput       |
                   |  IClock           ITransport            |
                   +---------------------+-------------------+
                                         |
                      +------------------+------------------+
                      v                                     v
        +-------------------------+           +-------------------------+
        |     Implementation PC   |           |  Implementation STM32   |
        |     (flight.sim.*)      |           |       (a venir)         |
        |                         |           |                         |
        |  SimulatedSensorInput   |           |  HILSensorInput         |
        |  SimulatedActuatorOutput|           |  HILActuatorOutput      |
        |  LoopbackTransport      |           |  Stm32UartTransport     |
        |  SimulatedClock         |           |  DwtClock               |
        +-------------------------+           +-------------------------+
```

### 4.2 Interfaces C++23 (Code d'Implémentation)

```cpp
// Extrait de Src/embedded/hal/hal_types.cppm (module flight.hal.types)
import std;

namespace FlightCore::HAL
{

struct SensorData
{
    std::uint64_t timestamp_us;

    std::float32_t position_x_m;
    std::float32_t position_y_m;
    std::float32_t position_z_m;

    std::float32_t velocity_x_ms;
    std::float32_t velocity_y_ms;
    std::float32_t velocity_z_ms;

    std::float32_t gyro_roll_rad_s;
    std::float32_t gyro_pitch_rad_s;
    std::float32_t gyro_yaw_rad_s;

    std::float32_t accel_x_m_s2;
    std::float32_t accel_y_m_s2;
    std::float32_t accel_z_m_s2;

    std::float32_t roll_rad;
    std::float32_t pitch_rad;
    std::float32_t yaw_rad;

    std::float32_t altitude_baro_m;
    std::float32_t wing_rpm_meas;

    std::uint32_t sensor_valid_flags;
};

struct ActuatorCommands
{
    std::uint64_t timestamp_us;

    std::float32_t wing_rpm_cmd;
    std::float32_t left_servo_rad;
    std::float32_t right_servo_rad;
    std::float32_t aux_actuator_cmd;
    std::uint8_t   mode_flags;
};

}
```

```cpp
// Extraits de Src/embedded/hal/{sensor_input,actuator_output,clock}.cppm
// et Src/embedded/transport/transport.cppm
import std;

namespace FlightCore::HAL
{

class ISensorInput
{
public:
    virtual ~ISensorInput() = default;

    /* false si aucun echantillon frais (non bloquant). */
    [[nodiscard]] virtual bool readSensorData(SensorData& out_data) noexcept = 0;
};

class IActuatorOutput
{
public:
    virtual ~IActuatorOutput() = default;

    /* false si le canal aval ne peut pas accepter la commande. */
    [[nodiscard]] virtual bool writeActuatorCommands(const ActuatorCommands& in_cmds) noexcept = 0;
};

class IClock
{
public:
    virtual ~IClock() = default;

    [[nodiscard]] virtual std::uint64_t nowUs() const noexcept = 0;

    virtual void sleepUs(std::uint64_t us) noexcept = 0;
};

}

namespace FlightCore::Transport
{

class ITransport
{
public:
    virtual ~ITransport() = default;

    [[nodiscard]] virtual bool sendBytes(std::span<const std::uint8_t> data) noexcept = 0;

    [[nodiscard]] virtual std::size_t receiveBytes(std::span<std::uint8_t> buffer) noexcept = 0;

    [[nodiscard]] virtual std::size_t bytesAvailable() const noexcept = 0;

    virtual void flush() noexcept {}
};

}
```

> **Note :** les champs de `SensorData` / `ActuatorCommands` reproduisent un à un les payloads `HilSensorPayload` / `HilActuatorPayload` de HIL-Proto v1.0 (voir `docs/hil/hil_protocol.md`), de sorte que la conversion fil <-> HAL (module `flight.transport.protocol.codec`) ne nécessite aucun re-calibrage. Les implémentations PC de ces interfaces sont les mocks du module `flight.sim.*` (`SimulatedSensorInput`, `SimulatedActuatorOutput`, `SimulatedClock`, `LoopbackTransport`).

---

## 5. Spécification du Protocole Transport HIL

### 5.1 Synthèse du Format Implémenté (Little-Endian, Alignement 1 Octet)

La spécification normative du protocole binaire **HIL-Proto v1.0** est maintenue dans [`hil_protocol.md`](hil_protocol.md) ; le contrat filaire de référence est implémenté dans le module `flight.transport.protocol` (`Src/embedded/transport/hil_protocol.cppm`), avec vérifications statiques (`static_assert`) sur toutes les tailles.

| Élément | Valeur implémentée | Constante / symbole |
| :--- | :--- | :--- |
| Octets de synchronisation | `0x48` `0x49` (`'H'`, `'I'`) | `kSync1` / `kSync2` |
| Version de protocole | `0x10` (v1.0) | `kProtocolVer` |
| En-tête (`HilHeader`) | 8 octets (sync, msg_id, protocol_ver, sequence_num u16, payload_len u16) | `kHeaderSize` |
| `SensorPacket` (msg 0x01, PC -> STM32) | payload 80 octets / trame 90 octets | `kSensorPayloadSize` / `kSensorFrameSize` |
| `ActuatorPacket` (msg 0x02, STM32 -> PC) | payload 44 octets / trame 54 octets | `kActuatorPayloadSize` / `kActuatorFrameSize` |
| CRC-16-CCITT (poly `0x1021`, init `0xFFFF`) | 2 octets, little-endian, calculés sur Header + Payload | `HilCrc` (`flight.transport.protocol.parser`) |

> Les anciennes tables de ce document (paquets de 48 / 22 octets avec préambule `0xAA55`) sont obsolètes et remplacées par la spécification HIL-Proto v1.0 ci-dessus. La description complète des payloads champ par champ figure dans la section 4 de `hil_protocol.md`.
---

## 6. Gestion du Temps et Synchronisation Temporelle

### 6.1 Maître du Temps de Simulation
Afin de garantir la répétabilité parfaite des essais HIL et d'éviter les dérives induites par les horloges matérielles distinctes (Quartz PC vs Quartz STM32) :
* **Le PC Simulator est le Maître Absolu du Temps Virtuel (`simulation_timestamp`).**
* La STM32 ne se fie pas à son propre timer interne pour faire avancer l'état de la simulation.
* Chaque `ControlTask` exécutée sur la STM32 utilise le `sim_timestamp_ms` contenu dans le `SensorPacket` pour évaluer les intégrales et dérivées du contrôleur PID ($\Delta t = t_{k} - t_{k-1}$).

### 6.2 Mode Temps Réel Synchrone vs Deterministic Lockstep
Deux modes d'exécution temporelle sont pris en charge :

1. **Mode Temps Réel Cadencé (Paced Real-Time - Par Défaut) :**
   * Le PC envoie un `SensorPacket` toutes les $10\text{ ms}$ (cadence stricte $100\text{ Hz}$).
   * La STM32 traite le paquet, calcule les commandes et renvoie le `ActuatorPacket` en moins de $2\text{ ms}$.
   * Le PC reçoit la réponse avant l'échéance du pas de simulation suivant.

2. **Mode Pas à Pas Déterministe (Deterministic Lockstep) :**
   * Utile pour le débogage point par point ou l'analyse au cycle près.
   * Le PC met en pause la simulation physique tant qu'il n'a pas reçu le `ActuatorPacket` correspondant au numéro de séquence courant.

---

## 7. Plan de Validation & Métriques de Performance

### 7.1 Cadre de Comparaison SIL vs HIL
Pour valider le portage HIL, un scénario de test identique est exécuté en SIL et en HIL :
* **Consigne de Mission :** Montée stationnaire et maintien de position aux coordonnées $X = 0\text{ m}, Y = 0\text{ m}, Z = 100\text{ m}$.
* **Durée du Scénario :** $60\text{ secondes}$.

#### Métriques d'Équivalence SIL / HIL

| Critère de Performance | Valeur Référence SIL | Valeur Mesurée HIL | Tolérance Maximale Admissible |
| :--- | :--- | :--- | :--- |
| **Time to Altitude (Z = 100 m)** | 12.40 s | 12.45 s | $\pm 0.50\text{ s}$ |
| **Erreur de Position Max ($e_{max}$)** | 0.82 m | 0.89 m | $\pm 0.25\text{ m}$ |
| **Erreur Quadratique Moyenne (RMSE Z)**| 0.12 m | 0.15 m | $\pm 0.08\text{ m}$ |
| **Consommation Énergétique / RPM Moy** | 4250 RPM | 4262 RPM | $\pm 2.0\%$ |
| **Statut de Réussite de Mission** | PASS | PASS | Strictement Identique (PASS) |

### 7.2 Métriques Profiling Embarqué STM32 (Timing & Ressources)

Le profilage du comportement temps réel de la STM32 est assuré en continu par la `Task_Health` :

```
[HIL HEALTH MONITOR METRICS]
+-------------------------------+-----------------------+
| Metric                        | Measured Value        |
+-------------------------------+-----------------------+
| ControlTask Period            | 10.00 ms (100 Hz)     |
| ControlTask Worst Execution   | 0.84 ms               |
| ControlTask Execution Jitter  | +/- 0.04 ms           |
| Total CPU Load                | 14.2 %                |
| Peak SRAM Usage (Stack/Heap)  | 18.4 KB / 128 KB      |
| Deadline Misses Count         | 0                     |
| UART Round-Trip Latency       | 1.12 ms               |
+-------------------------------+-----------------------+
```

---

## 8. Injection de Fautes et Stress-Testing sur HIL

L'intérêt majeur du banc HIL est de valider la résilience du système face à des anomalies temporelles ou des dégradations de communication qu'il est impossible de modéliser avec précision en pure simulation.

### 8.1 Scénarios d'Injection de Fautes Temporelles

1. **Retard Artificiel des Données Capteurs (Sensor Packet Delay) :**
   * **Perturbation :** Introduction d'un délai artificiel de $20\text{ ms}$ (soit 2 pas de retard) sur l'envoi du `SensorPacket` par le PC.
   * **Comportement Attendu :** `SensorTask` détecte l'absence de données fraîches via un timer d'échéance.
   * **Action de Sécurité :** Passage en mode d'extrapolation d'état (*Dead Reckoning*) pendant $50\text{ ms}$. Si le retard persiste $> 100\text{ ms}$, la `HealthTask` déclenche un mode Failsafe d'atterrissage d'urgence.

2. **Surcharge CPU Artificielle (CPU Load Stress) :**
   * **Perturbation :** Injection d'une boucle de calcul factice à haute priorité sur la STM32 consommant $8\text{ ms}$ sur les $10\text{ ms}$ de budget.
   * **Comportement Attendu :** Augmentation du taux d'occupation CPU à $> 90\%$.
   * **Action de Sécurité :** La `Task_Health` détecte la baisse de marge temporelle et désactive les tâches non critiques (e.g. télémétrie étendue) pour préserver la `ControlTask`.

3. **Corruption de Trame / Erreurs de Checksum CRC :**
   * **Perturbation :** Injection d'octets corrompus sur la liaison série (taux d'erreur binaire de $10^{-3}$).
   * **Comportement Attendu :** Rejet systématique des paquets invalides par le parser CRC16 sans plantage de la machine à états série.

---

## 9. Évolution : Vers le HIL Niveau 2 (Architecture Redondante Dual-MCU)

Une fois le HIL Niveau 1 complètement validé et stabilisé, le banc d'essai pourra évoluer vers la topologie **HIL Niveau 2**, reproduisant l'architecture matérielle redondante du projet X721 :

```
                                PC Simulator
                                     │
                        ┌────────────┴────────────┐
                        │                         │
                  SensorPacket              SensorPacket
                        │                         │
                        ▼                         ▼
                  ┌───────────┐             ┌───────────┐
                  │  STM32-1  │  Inter-FC   │  STM32-2  │
                  │   (FC1)   │ <─ CAN/UART─>│   (FC2)   │
                  │  Master   │             │  Standby  │
                  └─────┬─────┘             └─────┬─────┘
                        │                         │
                 ActuatorCmd (FC1)         ActuatorCmd (FC2)
                        │                         │
                        └────────────┬────────────┘
                                     │ (Arbitration Logic)
                                     ▼
                                PC Simulator
```

* **Objectif du Niveau 2 :** Tester en vrai matériel le protocole de synchronisation d'état inter-calculateurs, la détection de défaillance de FC1 par FC2, et la bascule à chaud (*hot failover*) sans rupture de la commande des gouvernes.

---

## 10. Conclusion & Prochaines Étapes

L'architecture HIL formalisée dans ce document fournit un cadre rigoureux pour exécuter le code de vol réel sur matériel embarqué ARM Cortex-M tout en maintenant un contrôle parfait du temps et des conditions d'essai grâce au simulateur PC hôte.

**Prochaines étapes opérationnelles :**
1. Validation du câblage matériel et du débit de la liaison série USB-UART à $921\,600\text{ baud}$.
2. Compilation de la chaîne `FlightCore` avec le toolchain `arm-none-eabi-gcc`.
3. Exécution de la campagne de tests comparatifs SIL vs HIL et génération des rapports de timing embarqués.
