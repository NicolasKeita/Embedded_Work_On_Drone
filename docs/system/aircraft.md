# Architecture Système - Spécification Aéronef (Heliblade-like)

Référence des composants du modèle aéronef simulé. Les noms de capteurs ci-dessous
décrivent des fonctions de simulation, sans pilotes de capteurs physiques associés.
Voir les [interfaces](software_interfaces.md) et les [équations](flight_dynamics.md).

---

## 1. Mission

L'objectif et les critères de maintien sont définis dans la [mission](mission.md).
La propulsion du modèle est assurée par une voilure tournante simplifiée.

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
- **Consigne logicielle :** `ControlCommand::wing_rpm`, directement en RPM.

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
  $$\text{error} = Z_{\text{cible}} - Z_{\text{mesuré}}$$

### C. Centrale Inertielle / IMU (`IMUSensor`)

- **Mesures :**
  - **Attitude :** Assiette (*pitch*), Roulis (*roll*).
  - **Dynamique :** Accélérations linéaires, vitesses angulaires.
- **Rôle :** Estimation de l'attitude et détection des variations dynamiques pour l'asservissement en stabilité.

### D. Capteur de Vitesse de Rotation (`RPMSensor`)

- **Mesure :** Vitesse réelle de rotation des pales (`wing_rpm`).
- **Rôle :** Feedback en boucle fermée sur la vitesse des ailes. Permet d'identifier les écarts de consigne, pannes ou pertes de puissance (*Health Monitoring*).

---

## 4. Références de calcul et de contrôle

Les unités et structures sont définies par les [interfaces logicielles](software_interfaces.md).
La [dynamique](flight_dynamics.md) décrit la causalité actionneurs/mouvement.
La boucle FC1/FC2 est décrite par l'[architecture](../architecture/overview.md).
