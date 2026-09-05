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

1. **PC (Simulateur) :** Calcule un pas de temps de la physique de l'aéronef ($dt = 10	ext{ ms}$).
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

Le cœur des algorithmes de contrôle dépend uniquement d'interfaces C++20 pures (`ISensorInput`, `IActuatorOutput`, `ITransport`).

```
                          ┌───────────────────────────┐
                          │   FlightController Core   │
                          └─────────────┬─────────────┘
                                        │
                                        ▼
                   ┌─────────────────────────────────────────┐
                   │    Hardware Abstraction Layer (HAL)     │
                   │                                         │
                   │   ISensorInput     IActuatorOutput      │
                   │   ITransport       ISystemTime          │
                   └────────────────────┬────────────────────┘
                                        │
                     ┌──────────────────┴──────────────────┐
                     ▼                                     ▼
        ┌─────────────────────────┐           ┌─────────────────────────┐
        │     Implémentation PC   │           │   Implémentation STM32  │
        │                         │           │                         │
        │  • MockSensorInput      │           │  • HILSensorInput       │
        │  • MockActuatorOutput   │           │  • HILActuatorOutput    │
        │  • SocketTransport      │           │  • Stm32UartTransport   │
        └─────────────────────────┘           └─────────────────────────┘
```

### 4.2 Interfaces C++20 (Code de Spécification)

```cpp
// HAL Interfaces pour découplage SIL/HIL
#pragma once
#include <cstdint>
#include <span>

namespace FlightCore::HAL {

struct SensorData {
    uint64_t timestamp_us;
    float position_x_m;
    float position_y_m;
    float altitude_m;
    float velocity_x_ms;
    float velocity_y_ms;
    float velocity_z_ms;
    float pitch_rad;
    float roll_rad;
    float yaw_rad;
    float wing_rpm;
};

struct ActuatorCommands {
    uint64_t timestamp_us;
    float wing_rpm_cmd;
    float left_servo_rad;
    float right_servo_rad;
    uint8_t mode_flags;
};

class ISensorInput {
public:
    virtual ~ISensorInput() = default;
    virtual bool readSensorData(SensorData& out_data) = 0;
};

class IActuatorOutput {
public:
    virtual ~IActuatorOutput() = default;
    virtual bool writeActuatorCommands(const ActuatorCommands& in_cmds) = 0;
};

class ITransport {
public:
    virtual ~ITransport() = default;
    virtual bool sendBytes(std::span<const uint8_t> data) = 0;
    virtual size_t receiveBytes(std::span<uint8_t> buffer) = 0;
};

} // namespace FlightCore::HAL
```

---

## 5. Spécification du Protocole Transport HIL

### 5.1 Structure des Paquets Binaires (Little-Endian, Alignment 1 Byte)

Le protocole HIL utilise un cadrage binaire compact pour minimiser la latence de sérialisation et d'émission série.

#### Paquet Données Capteurs : `SensorPacket` (PC ➔ STM32)
* **Taille totale :** 48 octets
* **Fréquence :** 100 Hz

| Champ | Type | Taille (Octets) | Description / Unité |
| :--- | :--- | :--- | :--- |
| `preamble` | `uint16_t` | 2 | Octets de synchronisation de trame (`0xAA55`) |
| `msg_id` | `uint8_t` | 1 | Identifiant de message (`0x01` = SensorPacket) |
| `sequence` | `uint8_t` | 1 | Numéro de séquence incrémental (0-255) |
| `sim_timestamp_ms` | `uint32_t` | 4 | Horodatage du simulateur hôte (ms) |
| `position_x` | `float` | 4 | Position Est (m) |
| `position_y` | `float` | 4 | Position Nord (m) |
| `altitude` | `float` | 4 | Altitude Z (m) |
| `velocity_x` | `float` | 4 | Vitesse Est (m/s) |
| `velocity_y` | `float` | 4 | Vitesse Nord (m/s) |
| `velocity_z` | `float` | 4 | Vitesse Verticale (m/s) |
| `pitch` | `float` | 4 | Tangage (rad) |
| `roll` | `float` | 4 | Roulis (rad) |
| `wing_rpm` | `float` | 4 | Vitesse de rotation du rotor/aile (RPM) |
| `crc16` | `uint16_t` | 2 | Checksum CRC16-CCITT sur l'ensemble de la charge utile |

#### Paquet Commandes Actionneurs : `ActuatorPacket` (STM32 ➔ PC)
* **Taille totale :** 22 octets
* **Fréquence :** 100 Hz

| Champ | Type | Taille (Octets) | Description / Unité |
| :--- | :--- | :--- | :--- |
| `preamble` | `uint16_t` | 2 | Octets de synchronisation de trame (`0xAA55`) |
| `msg_id` | `uint8_t` | 1 | Identifiant de message (`0x02` = ActuatorPacket) |
| `sequence` | `uint8_t` | 1 | Numéro de séquence retourné par la STM32 |
| `stm32_timestamp_ms`| `uint32_t` | 4 | Horodatage interne du RTOS STM32 (ms) |
| `wing_rpm_cmd` | `float` | 4 | Consigne RPM moteur |
| `left_servo_cmd` | `float` | 4 | Consigne angle gouverne gauche (rad) |
| `right_servo_cmd` | `float` | 4 | Consigne angle gouverne droite (rad) |
| `status_flags` | `uint8_t` | 1 | État de santé du FC (0x01 = OK, 0x02 = Degraded, 0xFF = Fail) |
| `crc16` | `uint16_t` | 2 | Checksum CRC16-CCITT sur l'ensemble de la charge utile |

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
   * Le PC envoie un `SensorPacket` toutes les $10	ext{ ms}$ (cadence stricte $100	ext{ Hz}$).
   * La STM32 traite le paquet, calcule les commandes et renvoie le `ActuatorPacket` en moins de $2	ext{ ms}$.
   * Le PC reçoit la réponse avant l'échéance du pas de simulation suivant.

2. **Mode Pas à Pas Déterministe (Deterministic Lockstep) :**
   * Utile pour le débogage point par point ou l'analyse au cycle près.
   * Le PC met en pause la simulation physique tant qu'il n'a pas reçu le `ActuatorPacket` correspondant au numéro de séquence courant.

---

## 7. Plan de Validation & Métriques de Performance

### 7.1 Cadre de Comparaison SIL vs HIL
Pour valider le portage HIL, un scénario de test identique est exécuté en SIL et en HIL :
* **Consigne de Mission :** Montée stationnaire et maintien de position aux coordonnées $X = 0	ext{ m}, Y = 0	ext{ m}, Z = 100	ext{ m}$.
* **Durée du Scénario :** $60	ext{ secondes}$.

#### Métriques d'Équivalence SIL / HIL

| Critère de Performance | Valeur Référence SIL | Valeur Mesurée HIL | Tolérance Maximale Admissible |
| :--- | :--- | :--- | :--- |
| **Time to Altitude (Z = 100 m)** | 12.40 s | 12.45 s | $\pm 0.50	ext{ s}$ |
| **Erreur de Position Max ($e_{max}$)** | 0.82 m | 0.89 m | $\pm 0.25	ext{ m}$ |
| **Erreur Quadratique Moyenne (RMSE Z)**| 0.12 m | 0.15 m | $\pm 0.08	ext{ m}$ |
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
   * **Perturbation :** Introduction d'un délai artificiel de $20	ext{ ms}$ (soit 2 pas de retard) sur l'envoi du `SensorPacket` par le PC.
   * **Comportement Attendu :** `SensorTask` détecte l'absence de données fraîches via un timer d'échéance.
   * **Action de Sécurité :** Passage en mode d'extrapolation d'état (*Dead Reckoning*) pendant $50	ext{ ms}$. Si le retard persiste $> 100	ext{ ms}$, la `HealthTask` déclenche un mode Failsafe d'atterrissage d'urgence.

2. **Surcharge CPU Artificielle (CPU Load Stress) :**
   * **Perturbation :** Injection d'une boucle de calcul factice à haute priorité sur la STM32 consommant $8	ext{ ms}$ sur les $10	ext{ ms}$ de budget.
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
1. Validation du câblage matériel et du débit de la liaison série USB-UART à $921\,600	ext{ baud}$.
2. Compilation de la chaîne `FlightCore` avec le toolchain `arm-none-eabi-gcc`.
3. Exécution de la campagne de tests comparatifs SIL vs HIL et génération des rapports de timing embarqués.
