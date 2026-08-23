# Interfaces Logicielles et Structures de Données du Système

## Document Reference: `docs/system/software_interfaces.md`
**Étape du Projet :** Étape 4 — Définition des interfaces logicielles et contrat de données  
**Auteur :** Nicolas Keita  
**Statut :** Spécification d'Architecture — Validé  

---

## 1. Introduction et Philosophie de Conception

Ce document définit les contrats d'interface et les structures de données fondamentales de l'aéronef. Dans la méthodologie de développement du système embarqué et de simulation, cette étape marque la transition entre la modélisation de la physique du vol et la représentation informatique des informations qui circulent au sein de l'aéronef.

### 1.1 Objectifs Clés
- **Découplage strict :** Séparer la mesure (capteurs), l'estimation d'état, la logique de contrôle et l'actionnement.
- **Transparence SIL / HIL :** Permettre l'exécution du même algorithme de contrôle (`Flight Controller`) indifféremment sur PC de simulation ou sur microcontrôleur cible (ex: STM32).
- **Contrat strict sans code prématuré :** Établir les interfaces conceptuelles et les structures de données avant toute écriture de code C++ (`.hpp` / `.cpp`) ou d'intégration d'un RTOS.

---

## 2. Les 4 Types de Données Fondamentaux (+ Consigne)

Le système repose sur la circulation de 5 catégories d'informations bien structurées.

```
                   ┌─────────────────┐
                   │   TargetState   │
                   └────────┬────────┘
                            │ (Consigne)
                            ▼
 ┌───────────┐     ┌─────────────────┐     ┌───────────────────┐     ┌───────────┐
 │  Sensors  ├────►│ State Estimator ├────►│ Flight Controller ├────►│ Actuators │
 └───────────┘     └─────────────────┘     └───────────────────┘     └───────────┘
   (Mesures)           (Estimation)              (Commande)            (Action)
  SensorData          AircraftState            ControlCommand
```

---

### 2.1 `SensorData`
**Rôle :** Transporter les mesures brutes ou pré-filtrées issues des capteurs physiques ou simulés vers le système d'estimation.

#### Structure des Données
| Champ | Description | Unité | Type Conceptuel |
| :--- | :--- | :--- | :--- |
| `position_x` | Position mesurée sur l'axe X (Global / NED) | $m$ | `float` / `double` |
| `position_y` | Position mesurée sur l'axe Y (Global / NED) | $m$ | `float` / `double` |
| `altitude` | Altitude mesurée (Baromètre / GPS / Altimètre) | $m$ | `float` / `double` |
| `pitch` | Tangage mesuré par l'IMU | $rad$ | `float` |
| `roll` | Roulis mesuré par l'IMU | $rad$ | `float` |
| `acceleration` | Vecteur d'accélération linéaire $(a_x, a_y, a_z)$ | $m/s^2$ | `Vector3f` |
| `angular_velocity` | Vitesse angulaire $(p, q, r)$ | $rad/s$ | `Vector3f` |
| `wing_rpm` | Vitesse de rotation mesurée du moteur / voilure | $RPM$ | `float` |

- **Producteur :** Composants implémentant `ISensor` (Capteurs simulés ou pilotes matériels).
- **Consommateur :** Module d'estimation d'état (`State Estimator`).
- **Distinction Majeure :** `SensorData` $
eq$ `AircraftState`. `SensorData` représente uniquement les mesures disponibles, sujettes au bruit, à la dérive et aux limitations des capteurs.

---

### 2.2 `AircraftState`
**Rôle :** Représenter l'état estimé de l'aéronef calculé à partir des mesures capteurs. C'est l'image logicielle de la réalité physique du véhicule.

#### Structure des Données
```
AircraftState
├── position
│   ├── x (m)
│   └── y (m)
├── altitude (m)
├── pitch (rad)
├── roll (rad)
├── velocity
│   ├── x (m/s)
│   ├── y (m/s)
│   └── z (m/s)
└── angular_velocity
    ├── pitch (rad/s)
    └── roll (rad/s)
```

- **Producteur :** `State Estimator` (Estimateur d'état / Filtre de Kalman).
- **Consommateurs :** `Flight Controller`, `Health Monitor`, Télemesure.
- **Règle d'Architecture Anti-Triche :** La simulation connaît l'état physique exact (*True State* : `true_position`, `true_velocity`, `true_acceleration`). Cependant, le `Flight Controller` ne **doit jamais** lire le *True State*. Il consomme exclusivement le `AircraftState` estimé à partir des `SensorData`.

---

### 2.3 `TargetState`
**Rôle :** Exprimer la consigne de navigation fournie au contrôleur de vol ("où l'aéronef doit être").

#### Structure des Données
| Champ | Description | Unité |
| :--- | :--- | :--- |
| `target_x` | Position cible X | $m$ |
| `target_y` | Position cible Y | $m$ |
| `target_altitude` | Altitude cible | $m$ |

- **Producteur :** Planificateur de mission (Mission Planner), Système de navigation ou Consigne externe.
- **Consommateur :** `Flight Controller`.

---

### 2.4 `ControlCommand`
**Rôle :** Transporter les consignes d'actionnement calculées par le contrôleur de vol vers les actionneurs.

#### Structure des Données
```
ControlCommand
├── wing_rpm           (RPM demandés pour la voilure / rotor)
├── left_servo_angle   (Angle demandé pour le servo gauche, rad ou deg)
└── right_servo_angle  (Angle demandé pour le servo droit, rad ou deg)
```

- **Producteur :** `Flight Controller`.
- **Consommateur :** Composants implémentant `IActuator` (Moteurs et servomoteurs physiques ou simulés).

---

### 2.5 `HealthStatus`
**Rôle :** Centraliser l'état de santé opérationnel des sous-systèmes pour assurer la tolérance aux pannes, la surveillance et la décision de reconfiguration (notamment en architecture multi-FC).

#### Structure des Données
```
HealthStatus
├── flight_controller  (État du calculateur de vol)
├── sensors            (État de la chaîne de mesure)
├── actuators          (État de la chaîne d'actionnement)
├── communication      (État des bus et liens de com)
└── overall_state      (État global synthétisé)
```

#### Énumération des États
- `NORMAL` / `HEALTHY` : Fonctionnement nominal.
- `DEGRADED` : Dégradation non critique (ex: perte d'un capteur secondaire).
- `SAFE` : Mode de mise en sécurité activé (ex: maintien à plat, descente contrôlée).
- `FAILURE` / `FAILED` : Panne critique exigeant une bascule (failover FC2) ou une procédure d'urgence.
- `RECOVERY` : Procédure de récupération d'état en cours.

- **Producteur :** `Health Monitor`.
- **Consommateurs :** System Manager, Module de redondance (FC1 / FC2), Télemesure sol.

---

## 3. Boucle de Contrôle Conceptuelle

La relation entre ces structures définit la boucle de régulation fermée du système :

```
                        ┌──────────────┐
                        │ TargetState  │
                        └──────┬───────┘
                               │ "Où je veux être"
                               ▼
  ┌──────────────┐      ┌──────────────┐      ┌──────────────┐
  │AircraftState ├─────►│  Controller  ├─────►│ControlCommand│
  └──────────────┘      └──────────────┘      └──────────────┘
  "Où je suis"          "Calcul de             "Quoi commander"
                         correction"
```

### Exemple Concret d'Asservissement
1. **TargetState :** $X = 0\,	ext{m}, Y = 0\,	ext{m}, 	ext{Altitude} = 100\,	ext{m}$
2. **AircraftState :** $X = 15\,	ext{m}, Y = -8\,	ext{m}, 	ext{Altitude} = 97\,	ext{m}$
3. **Controller :** Génère une `ControlCommand` corrigée ($\Delta 	ext{wing\_rpm}$, modification des angles de servos).

---

## 4. Architecture Globale des Flux de Données

```
                    ┌──────────────┐
                    │   Sensors    │
                    └──────┬───────┘
                           │
                           │ SensorData
                           ▼
                    ┌──────────────┐
                    │State Estimator│
                    └──────┬───────┘
                           │
                           │ AircraftState
                           ▼
  TargetState ────► ┌──────────────┐
                    │    Flight    │
                    │  Controller  │
                    └──────┬───────┘
                           │
                           │ ControlCommand
                           ▼
                    ┌──────────────┐
                    │  Actuators   │
                    └──────┬───────┘
                           │
                           ▼
                        Aircraft

  ───────────────────────────────────────────────────

             ┌──────────────────┐
             │  Health Monitor  │
             └────────┬─────────┘
                      │
                      ▼
                 HealthStatus
```

---

## 5. Abstraction des Interfaces C++ Conceptuelles

Pour garantir l'interchangeabilité entre simulation et matériel réel, 4 interfaces majeures sont établies :

```
       ISensor                     IActuator                   ITransport
          │                            │                            │
   ┌──────┴──────┐              ┌──────┴──────┐              ┌──────┴──────┐
   ▼             ▼              ▼             ▼              ▼             ▼
Simulated     Physical       Simulated     Physical        MockTransport UDPTransport
 Capteurs     Capteurs        Moteurs       Moteurs         (In-Memory)   (Réseau/SIL)
                                                                           │
                                                                           ▼
                                                                      CANTransport
                                                                       (Hardware)
```

### 5.1 `ISensor`
Permet la lecture agnostique des données capteurs.
- **Dérivations prévues :**
  - `SimulatedPositionSensor`
  - `SimulatedIMU`
  - `SimulatedAltitudeSensor`
  - `SimulatedRPMSensor`
  - *Pilotes matériels réels (I2C / SPI / ADC)*

### 5.2 `IActuator`
Permet l'envoi agnostique de commandes aux actionneurs.
- **Dérivations prévues :**
  - `SimulatedWingMotor`
  - `SimulatedLeftServo`
  - `SimulatedRightServo`
  - *Pilotes matériels réels (PWM / CAN / Timers)*

### 5.3 `IController`
Interface générique encapsulant les algorithmes de contrôle de vol.
- Permet de remplacer ou comparer différents contrôleurs (PID, LQR, RL) sans modifier le reste du système.

### 5.4 `ITransport`
Abstraction de la couche de communication pour le transfert de données intra-système ou inter-calculateurs.
- **Dérivations prévues :**
  - `MockTransport` (communication directe en mémoire pour tests unitaires)
  - `UDPTransport` (communication socket réseau pour simulation SIL sur PC)
  - `CANTransport` (bus matériel CAN pour cible embarquée)

---

## 6. Portabilité SIL / HIL

Grâce à ce découplage, le code métier du **Flight Controller** demeure rigoureusement identique, quel que soit l'environnement d'exécution :

```
                       Même Flight Controller
                                 │
                   ┌─────────────┴─────────────┐
                   ▼                           ▼
            Environnement               Environnement
           SIMULATION (PC)              EMBARQUÉ (STM32)
         ┌─────────────────┐         ┌─────────────────┐
         │ SimulatedSensors│         │ HardwareSensors │
         │ SimulatedActuators│       │ HardwareActuators│
         │ UDPTransport    │         │ CANTransport    │
         └─────────────────┘         └─────────────────┘
```