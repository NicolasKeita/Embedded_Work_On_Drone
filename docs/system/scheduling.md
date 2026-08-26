# Architecture d'Ordonnancement Temps Réel — Flight Controller (Étape 7)

**Projet :** Flight Controller Simulation & Embedded Architecture  
**Document :** `docs/system/scheduling.md`  
**Version :** 1.0  
**Statut :** Spécification d'Architecture Spatio-Temporelle  

---

## 1. Vue d'Ensemble & Objectifs

Jusqu'à présent (Étape 6), le simulateur et le contrôleur de vol fonctionnaient sous la forme d'une boucle d'exécution synchrone classique (*super-loop* / *bare-metal polling loop*) :

```cpp
while (running) {
    read_sensors();
    control();
    update_actuators();
}
```

Bien que cette approche permette de valider les lois de commande et la physique de l'aéronef, elle présente plusieurs limites majeures pour un système embarqué critique :
- **Non-déterminisme temporel :** La période de boucle varie selon le temps d'exécution de chaque sous-système.
- **Absence de découplage fréquentiel :** La télémétrie, la gestion de mission ou la santé système tournent à la même fréquence élevée que la boucle de contrôle d'attitude, gaspillant des ressources processeur.
- **Risque d'affaissement du contrôle (Jitter) :** Un blocage temporaire dans un traitement secondaire (ex: E/S réseau, logs) décale l'exécution du régulateur, compromettant la stabilité de l'aéronef.

L'**Étape 7** a pour objectif de transformer le *Flight Controller* en une **architecture temps réel multi-tâches (RTOS-ready)**, modulable, déterministe et prête pour le déploiement sur microcontrôleur (STM32).

---

## 2. Stratégie d'Implémentation Progressive

Conformément à la feuille de route d'ingénierie embarquée, l'intégration se déroule sans contrainte matérielle immédiate :

```
┌─────────────────────────┐
│ Étape 6                 │  Flight Controller fonctionnel en boucle synchrone (PC)
└────────────┬────────────┘
             │
             ▼
┌─────────────────────────┐
│ Étape 7 (Actuelle)      │  Définition des tâches RTOS & architecture temporelle (PC)
└────────────┬────────────┘
             │
             ▼
┌─────────────────────────┐
│ Étape 8                 │  Distribution de l'architecture (FC1 Primary / FC2 Safety)
└────────────┬────────────┘
             │
             ▼
┌─────────────────────────┐
│ Étape HIL               │  Portage sur cible STM32 / FreeRTOS-Zephyr & Hardware-in-the-Loop
└─────────────────────────┘
```

1. **Étape 7A (Ce document) :** Spécification formelle des tâches, fréquences, priorités, deadlines et mécanismes d'échanges inter-tâches (IPC).
2. **Étape 7B :** Modélisation sur PC (threads C++23 `std::jthread` ou wrapper RTOS) pour mesurer et valider le comportement temporel avant portage sur cible matérielle.

---

## 3. Table de Synthèse des Tâches Temps Réel

Le tableau suivant définit la matrice d'ordonnancement pour l'ensemble du *Flight Controller*.

| Tâche (`Task`) | Fréquence ($f$) | Période ($T$) | Priorité | Deadline ($D$) | Mode d'Exécution | Modèle IPC |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **`ControlTask`** | **200 Hz** | **5 ms** | **Highest** (P1) | 5 ms | Périodique | Queue In / Direct Out |
| **`ActuatorTask`** | **200 Hz** | **5 ms** | **Highest** (P1) | 5 ms | Périodique / Event | Queue In / Hardware Out |
| **`SensorTask`** | **100 Hz** | **10 ms** | **High** (P2) | 8 ms | Périodique | Hardware In / Queue Out |
| **`CommunicationTask`** | Event-driven / 50 Hz | ~20 ms | **High** (P2) | 15 ms | Événementiel / Asynchrone | Dual-Queue IPC |
| **`HealthTask`** | **10 Hz** | **100 ms** | **Medium** (P3) | 50 ms | Périodique | Shared State Audit / Queue |
| **`MissionTask`** | **10 Hz** | **100 ms** | **Medium** (P3) | 50 ms | Périodique | Event Queue / Target State |

---

## 4. Spécification Détaillée des 6 Tâches Temps Réel

### 4.1 `ControlTask` (Boucle de Contrôle d'Attitude & Guidance)
* **Description :** Cœur du contrôleur de vol. Calcule les ordres de commande (poussée, moments autour des axes de roulis, tangage, lacet) à partir de l'état estimé de l'aéronef et des consignes de mission.
* **Input :** 
  - `SensorData` (Attitude, vitesses angulaires, accélérations, altitude) depuis `SensorQueue`.
  - `TargetState` (Consignes d'attitude, d'altitude et de vitesse) depuis `MissionTask`.
* **Output :** 
  - `ControlCommand` (Commandes normalisées de poussée/couple) vers `ActuatorQueue`.
* **Period ($T$) :** `5 ms` (200 Hz)
* **Deadline ($D$) :** `5 ms` (Strictement déterministe)
* **Priority :** `Highest` (P1)
* **Rationale :** Les dynamiques de vol de l'aéronef nécessitent une correction d'attitude rapide et régulière. Tout retard sur la boucle de contrôle peut conduire à une divergence physique et à une instabilité.

### 4.2 `ActuatorTask` (Gestion des Actionneurs & Abstraction Hardware)
* **Description :** Convertit les commandes de contrôle virtuelles (`ControlCommand`) en signaux physiques ou simulations d'actionneurs (PWM, commandes de servomoteurs, vitesse des moteurs).
* **Input :** 
  - `ControlCommand` depuis `ActuatorQueue`.
* **Output :** 
  - Registres matériels (PWM/Timers STM32) ou état injecté dans le moteur physique du simulateur (SIL).
* **Period ($T$) :** `5 ms` (200 Hz) — Synchronisée sur l'arrivée des données de `ControlTask`.
* **Deadline ($D$) :** `5 ms`
* **Priority :** `Highest` (P1)
* **Rationale :** Sépare strictement la logique de contrôle de l'implémentation matérielle. En simulation (SIL), elle met à jour le simulateur physique ; sur STM32 (HIL), elle pilote la couche HAL/Driver.

### 4.3 `SensorTask` (Acquisition & Filtrage des Capteurs)
* **Description :** Lit les capteurs bruts (IMU, Baromètre, GPS, Tachymètre/RPM), applique le filtrage primaire (filtre passe-bas, élimination de bruit, étalonnage) et assemble la structure `SensorData`.
* **Input :** 
  - Bus matériels (SPI/I2C/UART) ou bus de simulation (SITL).
* **Output :** 
  - Structure `SensorData` envoyée dans `SensorQueue`.
* **Period ($T$) :** `10 ms` (100 Hz)
* **Deadline ($D$) :** `8 ms`
* **Priority :** `High` (P2)
* **Rationale :** Un taux de rafraîchissement de 100 Hz offre la fraîcheur de donnée nécessaire pour la boucle de régulation 200 Hz (avec interpolation ou sur-échantillonnage de l'IMU).

### 4.4 `CommunicationTask` (Télémétrie & Flux I/O Inter-Calculateurs)
* **Description :** Envoie les données de télémétrie vers la station sol / console et traite les paquets d'ordres entrants. Servira de pont de communication IPC pour la séparation FC1/FC2 à l'Étape 8.
* **Input :** 
  - Messages de commande externes (UART/MAVLink/CAN).
  - États internes exportés pour télémétrie (`AircraftState`, `SystemHealth`).
* **Output :** 
  - Trames réseau/série.
  - Consignes transmises à `MissionTask`.
* **Period ($T$) :** Événementiel (Trigger sur interruption UART/DMA) ou périodique à `20 ms` (50 Hz).
* **Deadline ($D$) :** `15 ms`
* **Priority :** `High` (P2)
* **Rationale :** Doit répondre rapidement aux requêtes d'interruption réseau sans interrompre la boucle de contrôle critique.

### 4.5 `HealthTask` (Supervision & Sécurité - Watchdog Fonctionnel)
* **Description :** Contrôle l'intégrité globale du système : vérification de la fraîcheur des capteurs, détection de pertes de paquets, contrôle des limites de température/tension, suivi du budget temporel des tâches (jitter & dépassement de deadline).
* **Input :** 
  - États et flags de santé de tous les sous-systèmes.
* **Output :** 
  - Signaux d'alerte, mode dégradé, ou déclenchement du fail-safe (vers `MissionTask` ou arrêt d'urgence).
* **Period ($T$) :** `100 ms` (10 Hz)
* **Deadline ($D$) :** `50 ms`
* **Priority :** `Medium` (P3)
* **Rationale :** Les pannes système (chute de tension, dérive lente de capteur) évoluent sur une échelle de temps plus longue que l'attitude. 10 Hz est amplement suffisant pour déclencher une stratégie de repli.

### 4.6 `MissionTask` (Machine à États de Vol & Navigation)
* **Description :** Gère les phases haut niveau du vol (`TAKEOFF`, `CLIMB`, `STATION_KEEPING`, `WAYPOINT_NAV`, `LANDING`, `FAILSAFE_RTL`). Calcule les cibles courantes transmises à `ControlTask`.
* **Input :** 
  - Ordres de communication (`CommunicationTask`).
  - Alertes de santé (`HealthTask`).
  - Position/Altitude filtrée.
* **Output :** 
  - Structure `TargetState` transmise à `ControlTask`.
* **Period ($T$) :** `100 ms` (10 Hz)
* **Deadline ($D$) :** `50 ms`
* **Priority :** `Medium` (P3)
* **Rationale :** La logique de décision stratégique n'a pas besoin de tourner à haute fréquence.

---

## 5. Architecture Temporelle & Flux de Données (IPC)

### 5.1 Diagramme Général d'Architecture

```
                                 ┌──────────────────────┐
                                 │   CommunicationTask  │
                                 └──────────┬───────────┘
                                            │ Orders / Commands
                                            ▼
┌─────────────────┐  SensorData  ┌──────────────────────┐ TargetState  ┌──────────────────────┐
│   SensorTask    ├─────────────►│     SensorQueue      ├────────────►│     MissionTask      │
│    (100 Hz)     │              └──────────┬───────────┘             │       (10 Hz)        │
└─────────────────┘                         │                         └──────────┬───────────┘
                                            │ SensorData                         │ TargetState
                                            ▼                                    ▼
                                 ┌───────────────────────────────────────────────────────────┐
                                 │                        ControlTask                        │
                                 │                         (200 Hz)                          │
                                 └──────────────────────────────┬────────────────────────────┘
                                                                │ ControlCommand
                                                                ▼
                                                     ┌─────────────────────┐
                                                     │    ActuatorQueue    │
                                                     └──────────┬──────────┘
                                                                │ ControlCommand
                                                                ▼
                                                     ┌─────────────────────┐
                                                     │    ActuatorTask     │
                                                     │      (200 Hz)       │
                                                     └──────────┬──────────┘
                                                                │
                                                                ▼
                                                     ┌─────────────────────┐
                                                     │ Hardware / Hardware │
                                                     │     Simulator       │
                                                     └─────────────────────┘
                                                                ▲
                                                                │ Monitoring
                                                     ┌──────────┴──────────┐
                                                     │     HealthTask      │
                                                     │       (10 Hz)       │
                                                     └─────────────────────┘
```

### 5.2 Pourquoi un Modèle Message Passing (Queues) plutôt qu'un État Global Partagé ?

L'utilisation d'une variable globale partagée (`AircraftState global_state;`) accessible en lecture/écriture par toutes les tâches introduit de graves faiblesses d'architecture :
1. **Inversion de Priorité & Verrouillage (Mutex Contention) :** Si `HealthTask` (Priorité Medium) verrouille un Mutex sur `global_state` pour une inspection longue, `ControlTask` (Priorité Highest) peut se retrouver bloquée en attente de la libération du mutex.
2. **Conditions de Course (Race Conditions) :** Des accès concurrents sans protection adéquate risquent de corrompre les structures de données (ex: lecture d'un quaternion partiellement écrit).
3. **Couplage Fort :** Rend le système difficile à découper ou à distribuer sur deux microcontrôleurs (FC1 et FC2).

**La Solution retenue : File de Messages Lock-Free / Thread-Safe (`Queue`)**
- `SensorTask` **produit** un message `SensorData` et le pousse dans `SensorQueue`.
- `ControlTask` **consomme** le dernier message `SensorData` disponible.
- Les données sont immuables à la transmission (*pass-by-value* ou *zero-copy ring-buffer*).
- Cette approche garantit le découplage temporel total et facilite la transition vers un bus de communication inter-calculateur (CAN/SPI) lors de l'Étape 8.

---

## 6. Concepts Temps Réel & Déterminisme

Pour concevoir ce système, nous nous appuyons sur six piliers fondamentaux des systèmes temps réel :

1. **Période ($T$) :** Intervalle de temps entre deux déclenchements successifs d'une tâche récurrente (ex: $T = 5\text{ ms}$ pour `ControlTask`).
2. **Deadline ($D$) :** Temps maximal accordé à la tâche pour terminer son exécution après son activation. Pour nos tâches critiques, la deadline est égale à la période ($D \le T$), dit système temps réel *strict* (*Hard Real-Time*).
3. **Priorité :** Niveau de préséance attribué à une tâche par l'ordonnanceur préemptif. Une tâche de priorité plus élevée suspend immédiatement l'exécution d'une tâche de priorité inférieure.
4. **Jitter (Gigotage temporel) :** Variation de l'instant d'exécution effectif de la tâche par rapport à son instant théorique idéal :
   $$\text{Jitter} = |t_{\text{exécution\_réel}} - t_{\text{théorique}}|$$
   Un jitter élevé détériore les performances de la boucle de régulation.
5. **Latence :** Délai écoulé entre la survenue d'un événement physique (ex: interruption capteur IMU) et le début d'exécution de la réponse logicielle.
6. **Synchronisation :** Mécanismes permettant de coordonner l'exécution des tâches en respectant l'ordre de causalité (ex: `ActuatorTask` déclenchée immédiatement à la fin de `ControlTask`).

### 6.1 Analyse d'Impact du Jitter sur la `ControlTask`

Supposons que la boucle de contrôle `ControlTask` repose sur un intégrateur numérique $\int e(t) dt \approx \sum e_k \cdot \Delta t$.

Si l'ordonnanceur manque de déterminisme (par exemple si une tâche de télémétrie non prioritaire bloque le processeur), le temps d'échantillonnage $\Delta t$ fluctue :

$$\Delta t \in \{5\text{ ms}, 10\text{ ms}, 7\text{ ms}, 15\text{ ms}, \dots\}$$

**Conséquences sur le Vol :**
- **Gain d'intégration instable :** La commande calculée devient sous-estimée ou surestimée.
- **Bruit de dérivation :** Le calcul du terme dérivé $\frac{e_k - e_{k-1}}{\Delta t}$ subit des impulsions parasites brusques.
- **Instabilité physique :** Entraîne des oscillations non désirées autour des axes de vol et un risque de perte de contrôle.

En attribuant la priorité **`Highest`** à `ControlTask` et `ActuatorTask`, nous garantissons que quelle que soit la charge processeur induite par les autres tâches, la boucle de régulation s'exécute à intervals rigoureusement constants ($\Delta t = 5\text{ ms} \pm \epsilon$).

---

## 7. Rôle Stratégique de l'ActuatorTask (Abstraction SIL / HIL)

La séparation explicite de `ControlTask` et `ActuatorTask` constitue le pivot de notre architecture d'abstraction matérielle :

```
                        ┌───────────────────┐
                        │   ControlTask     │
                        └─────────┬─────────┘
                                  │ ControlCommand
                                  ▼
                        ┌───────────────────┐
                        │   ActuatorTask    │
                        └─────────┬─────────┘
                                  │
          ┌───────────────────────┴───────────────────────┐
          │ (Mode Simulation)                             │ (Mode Embarqué)
          ▼                                               ▼
┌───────────────────┐                           ┌───────────────────┐
│ SimulatedActuator │                           │ HAL / PWM Driver  │
│  (Physique SIL)   │                           │    (Cible STM32)  │
└───────────────────┘                           └───────────────────┘
```

- **En mode Software-in-the-Loop (SIL) :** `ActuatorTask` transmet les commandes au modèle dynamique de simulation d'aéronef.
- **En mode Hardware-in-the-Loop (HIL) :** `ActuatorTask` écrit directement dans les registres PWM/Timer des servomoteurs et variateurs de vitesse (ESC) via la couche HAL sans modifier la moindre ligne du code de `ControlTask`.

---