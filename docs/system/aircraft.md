# Architecture Système - Spécification Aéronef (Heliblade-like)

Document de référence pour le système embarqué de l'aéronef : définition des actionneurs, des capteurs et de la boucle de contrôle de vol.

---

## 1. Description de la Mission & Principes Directeurs

**Objectif principal :** Maintenir l'aéronef dans une zone définie, à une altitude donnée, pendant une durée spécifiée ($X$ temps).

**Règle de modélisation :** Ne conserver dans le modèle que le strict nécessaire pour accomplir la mission. La propulsion indépendante est supprimée au profit d'un système à voilure tournante simplifié type *Heliblade*.

---

## 2. Interface des Actionneurs (Actuators)

L'aéronef dispose de **3 actionneurs principaux** :

```text
┌──────────────────────────┐
│       ACTUATORS          │
├──────────────────────────┤
│                          │
│  Rotation motor          │
│  Left wing servo         │
│  Right wing servo        │
│                          │
└──────────────────────────┘
```

### A. Moteur de Rotation des Ailes (`rotation_motor`)
- **Rôle :** Maintenir la rotation des pales/ailes afin de générer la portance et les forces aérodynamiques.
- **Variable physique contrôlée :** `wing_rpm` (vitesse de rotation des pales en RPM).
- **Schéma synoptique :**
  ```text
                 AILE
                   \
                    \
                     ●
                    ↻
                   /
                  /
                 AILE
                    ↑
             moteur de rotation
  ```
- **Consigne logicielle :** `rotation_motor_command` (valeur normalisée entre `0.0` [0%] et `1.0` [100%]). Le modèle dynamique convertit ensuite cette consigne en vitesse de rotation (RPM).

### B. Servo Aile Gauche (`left_wing_servo`)
- **Rôle :** Contrôler l'orientation / l'angle d'incidence de l'aile gauche (`left_servo_angle`).

### C. Servo Aile Droite (`right_wing_servo`)
- **Rôle :** Contrôler l'orientation / l'angle d'incidence de l'aile droite (`right_servo_angle`).

---

## 3. Interface des Capteurs (Sensors)

Pour assurer le guidage et la stabilisation, le calculateur de vol (*Flight Controller*) doit répondre en temps réel aux questions : *Où suis-je ? À quelle altitude ? Quelle est mon attitude et mon mouvement ?*

```text
                  SENSORS
                     │
       ┌─────────────┼──────────────┐
       │             │              │
       ▼             ▼              ▼
   Position        Altitude         IMU
    X / Y             Z          pitch/roll
       │             │              │
       └─────────────┼──────────────┘
                     │
                     ▼
                 Flight
                Controller
                     ▲
                     │
                  RPM sensor
```

### A. Capteur de Position X/Y (`PositionSensor`)
- **Mesure :** Coordonnées spatiales dans le plan horizontal ($X$, $Y$).
- **Rôle :** Vérification du maintien de l'aéronef dans la zone géofencée assignée. *(Remarque : Dans un drone réel, ces données proviendraient de la fusion GNSS/INS, mais elles sont ici représentées par un capteur simulé direct).*

### B. Altimètre (`AltitudeSensor`)
- **Mesure :** Altitude par rapport au niveau de référence ($Z$, en mètres).
- **Rôle :** Permet d'asservir l'altitude en calculant l'erreur d'asservissement :
  $$	ext{error} = Z_{	ext{cible}} - Z_{	ext{mesuré}}$$

### C. Centrale Inertielle / IMU (`IMUSensor`)
- **Mesures :**
  - **Attitude :** Assiette (*pitch*), Roulis (*roll*).
  - **Dynamique :** Accélérations linéaires, vitesses angulaires.
- **Rôle :** Estimation de l'attitude et détection des variations dynamiques pour l'asservissement en stabilité.

### D. Capteur de Vitesse de Rotation (`RPMSensor`)
- **Mesure :** Vitesse réelle de rotation des pales (`wing_rpm`).
- **Rôle :** Feedback en boucle fermée sur la vitesse des ailes. Permet d'identifier les écarts de consigne, pannes ou pertes de puissance (*Health Monitoring*).

---

## 4. Synthèse du Modèle Physique & Interfaces

### Actionneurs
| Actionneur | Commande | Fonction principale |
| :--- | :--- | :--- |
| **Rotation motor** | `motor_command` (`0.0` → `1.0`) | Maintenir/modifier la vitesse de rotation des ailes |
| **Left servo** | `left_servo_angle` | Modifier l'orientation de l'aile gauche |
| **Right servo** | `right_servo_angle` | Modifier l'orientation de l'aile droite |

### Capteurs
| Capteur | Mesure | Fonction principale |
| :--- | :--- | :--- |
| **Position sensor** | $X, Y$ | Maintien dans la zone de vol |
| **Altitude sensor** | $Z$ | Asservissement de l'altitude |
| **IMU** | Pitch, Roll, accélérations, rotations | Stabilité du vol et calcul d'attitude |
| **RPM sensor** | Vitesse réelle de rotation (`wing_rpm`) | Surveillance de la puissance / Health Monitoring |

---

## 5. Boucle de Contrôle Globale

```text
                         ┌──────────────┐
                         │   SENSORS    │
                         └───────┬──────┘
                                 │
                                 ▼
                         ┌──────────────┐
                         │    FLIGHT    │
                         │  CONTROLLER  │
                         └───────┬──────┘
                                 │
                                 ▼
                         ┌──────────────┐
                         │   ACTUATORS  │
                         └───────┬──────┘
                                 │
                                 ▼
                    ┌──────────────────────┐
                    │      AIRCRAFT        │
                    │                      │
                    │  Physics + Aero      │
                    └──────────┬───────────┘
                               │
                               └──────► Sensors
```