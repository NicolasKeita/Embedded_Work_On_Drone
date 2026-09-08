# Architecture Distribuée (Étape 8) : Double Calculateur de Vol (FC1 / FC2)

> **Evolution note — not the current implementation.** This describes the
> *intended future* topology (independent OS processes, UDP transport, physical
> targets). The current code is a **single process** with an in-process simulated
> communication bus (`CommsBus`) and a host FC emulator. For what is actually
> built today, see [`../architecture/overview.md`](../architecture/overview.md).


## 1. Vue d'Ensemble et Justification (Pourquoi deux FC ?)

Jusqu'à présent, le système de contrôle reposait sur un monobloc applicatif (un seul *Flight Controller*). Dans le cadre d'un système critique ou embarqué à forte exigence de sûreté de fonctionnement (SdL / Dependability), la tolérance aux pannes repose sur la redondance et le cloisonnement des responsabilités.

L'objectif de cette étape 8 est de transformer l'architecture monolithe en une **architecture distribuée à deux calculateurs logiciels indépendants** :
1. **FC1 (Primary Flight Controller)** : Calculateur principal en charge de la navigation, du guidage et du pilotage actif.
2. **FC2 (Safety Flight Controller)** : Calculateur de sécurité indépendant, en charge de la surveillance de la santé de FC1 et de la prise de décisions conservatrices en cas d'anomalie.

```
                  ┌────────────────────┐
                  │ Aircraft Simulator │
                  └─────────┬──────────┘
                            │
                        SensorData
                            │
                            ▼
                ┌───────────────────────┐
                │          FC1          │
                │  Primary Controller   │
                └───────────┬───────────┘
                            │
                     ControlCommand
                            │
                            ▼
                         Aircraft

                ┌───────────────────────┐
                │          FC2          │
                │   Safety Controller   │
                └───────────┬───────────┘
                            │
                       Monitoring
                            │
                            ▼
                    FC1 Health / State
```

### 1.1 Principe Fondamental
Le point essentiel de cette étape est le suivant : **FC2 ne pilote pas encore l'aéronef en mode nominal**. Son premier rôle est d'assurer une mission de surveillance passive/active de FC1 (*Health & Safety Monitoring*).

### 1.2 Motivation Architecturale
L'architecture distribuée est justifiée par l'exigence formelle suivante :
> *« Si le calculateur principal (FC1) disparaît, subit une défaillance matérielle/logicielle ou produit un blocage (deadlock/crash), le système doit le détecter en temps contraint et adopter de manière autonome un comportement sûr (Safe State). »*

Cette séparation garantit qu'une panne sévère dans le code complexe de FC1 (guidage, planification de trajectoire, traitement de données complexes) n'entraîne pas l'effondrement de la fonction de surveillance de sécurité hébergée dans FC2.

---

## 2. Décomposition des Responsabilités et Tâches

### 2.1 FC1 — Primary Controller (Calculateur Principal)

FC1 orchestre la boucle de contrôle principale et la gestion de la mission nominale.

#### Tâches Interne de FC1 :
* **`SensorTask`** : Acquisition, filtrage et validation des données capteurs issues du simulateur ou des bus de données.
* **`ControlTask`** : Calcul des lois de pilotage (boucles d'attitude, d'altitude et de vitesse).
* **`MissionTask`** : Gestion des plans de vol, suivi de waypoints et gestion des états de mission nominale.
* **`ActuatorTask`** : Formatage et envoi des commandes aux actionneurs (*motors/servos*).
* **`CommunicationTask`** : Émission des messages d'état (*Status*) et de présence (*Heartbeat*) vers FC2, réception des ordres de sécurité (*SafetyCommand*).

#### Flux de données de FC1 :
* **Entrées (Consumed Data)** :
  * `SensorData` (depuis l'Aircraft Simulator)
  * `TargetState` / `Waypoint` (depuis la station sol ou la mission)
  * `SafetyCommand` (depuis FC2)
* **Sorties (Produced Data)** :
  * `ControlCommand` (vers les actionneurs / Aircraft Simulator)
  * `Heartbeat` (vers FC2)
  * `Status` (vers FC2)

```
[Sensors] ───► (SensorTask) ───► (ControlTask) ───► (ActuatorTask) ───► [Actuators / Aircraft]
                                      ▲
                                 (MissionTask)
```

---

### 2.2 FC2 — Safety Controller (Calculateur de Sécurité)

FC2 est conçu selon le principe de simplicité (*KISS principle*) pour garantir une fiabilité maximale et réduire le risque de bugs logiciels dans la chaîne de sécurité.

#### Tâches Internes de FC2 :
* **`MonitoringTask`** : Analyse temporelle (*timing/jitter*) et logique des heartbeats de FC1 ; évaluation de la cohérence de l'état de santé de FC1.
* **`CommunicationTask`** : Réception des messages de FC1 (`Heartbeat`, `Status`) et transmission des instructions de sécurité (`SafetyCommand`).
* **`SafetyManager`** : Machine à états de sécurité déclenchant l'avancement vers le mode dégradé ou le mode d'urgence (*Safe Mode / Abort*).

#### Flux de données de FC2 :
* **Entrées (Consumed Data)** :
  * `Heartbeat` (depuis FC1)
  * `Status` (depuis FC1)
* **Sorties (Produced Data)** :
  * `SafetyCommand` (vers FC1 et/ou le système d'arrêt d'urgence du vecteur)

```
[FC1] ─── Heartbeat / Status ───► (MonitoringTask) ───► (SafetyManager) ───► SafetyCommand ───► [FC1 / Safe State]
```

---

## 3. Mécanisme de Heartbeat et Gestion des Timeouts

### 3.1 Définition du Heartbeat
Le *Heartbeat* est un message périodique émis par FC1 vers FC2 confirmant que l'exécutable principal est actif et qu'aucune tâche critique n'est bloquée.

* **Périodicité nominale** : 100 ms ($10	ext{ Hz}$)

#### Structure du message `Heartbeat` :
```
Heartbeat
├── sequence_number : uint32   (Incrémenté à chaque émission)
├── timestamp       : uint64   (Temps système en millisecondes / microsecondes)
└── health_state    : uint8    (État auto-évalué de FC1 : HEALTHY, DEGRADED, FAILED)
```

**Exemple de trame d'émission :**
```text
seq = 1842 | timestamp = 18420 ms | health_state = HEALTHY (0x01)
seq = 1843 | timestamp = 18520 ms | health_state = HEALTHY (0x01)
seq = 1844 | timestamp = 18620 ms | health_state = HEALTHY (0x01)
```

### 3.2 Spécification du Timeout
FC2 maintient un compteur de temps écoulé depuis le dernier *Heartbeat* valide reçu.

* **Fenêtre temporelle d'attente (Period)** : $T_{hb} = 100	ext{ ms}$
* **Seuil d'invalidation (Timeout threshold)** : $T_{timeout} = 300	ext{ ms}$ (soit 3 manques consécutifs).

```
FC1  ─── Heartbeat ───► FC2  (t = 0 ms)
FC1  ─── Heartbeat ───► FC2  (t = 100 ms)
FC1  ─── X (Perte) ───► FC2  (t = 200 ms - attente)
FC1  ─── X (Perte) ───► FC2  (t = 300 ms - Attente maximale atteinte)
                         │
                         ▼
                   [TIMEOUT DETECTED]
                         │
                         ▼
             Transition -> SUSPECTED_FAILURE
```

### 3.3 Nuance Sémantique : "Suspected Failure" vs "Definitive Death"
Un timeout ne signifie pas nécessairement que l'unité matérielle FC1 est définitivement détruite ou éteinte. Il indique :
> *« FC2 n'a plus la preuve rafraîchie que FC1 s'exécute dans ses contraintes temporelles et logiques. »*

Les causes potentielles d'un timeout incluent :
1. Crash dur du processus FC1 (Segfault, crash matériel).
2. Dépassement de priorité (*Task Starvation*) ou boucle infinie dans une tâche de FC1 empêchant la tâche de communication de s'exécuter.
3. Congestion ou coupure du lien de communication inter-processus (UDP / bus réseau).

---

## 4. Machines à États (FC1 et FC2)

### 4.1 Machine à États de FC1 (Health State)
FC1 évalue dynamiquement son propre état interne et l'inclut dans ses trames :

1. **`HEALTHY`** : Fonctionnement nominal. Toutes les tâches s'exécutent dans les temps, capteurs valides.
2. **`DEGRADED`** : Perte d'un capteur secondaire ou léger retard d'exécution, mais pilotage toujours assuré.
3. **`FAILED`** : Erreur critique interne (ex: échec d'allocation mémoire, perte capteur majeur, panique système).

### 4.2 Machine à États de FC2 (Safety State)
FC2 évolue en fonction de la réception des messages et du respect des contraintes temporelles :

```
             ┌────────────────────────┐
             │       MONITORING       │
             └───────────┬────────────┘
                         │
                         │ Timeout ( > 300 ms )
                         │ ou FC1 health == FAILED
                         ▼
             ┌────────────────────────┐
             │   SUSPECTED_FAILURE    │
             └───────────┬────────────┘
                         │
                         │ Confirmation (Temps de grâce dépassé / pas de reprise)
                         ▼
             ┌────────────────────────┐
             │       SAFE_MODE        │
             └───────────┬────────────┘
                         │
                         │ (Action de récupération validée)
                         ▼
             ┌────────────────────────┐
             │        RECOVERY        │
             └────────────────────────┘
```

#### Définition des états FC2 :
* **`MONITORING`** : État initial et nominal. FC2 valide l'arrivée périodique des heartbeats.
* **`SUSPECTED_FAILURE`** : Déclenché immédiatement dès le premier timeout ($>300	ext{ ms}$) ou si FC1 rapporte `DEGRADED`/`FAILED`.
* **`SAFE_MODE`** : Confirmé après l'expiration d'un timer de sécurité sans rétablissement. Ordre de repli transmis.
* **`RECOVERY`** : Procédure optionnelle de réinitialisation ou de réengagement sous conditions strictes.

---

## 5. Traitement des Pannes et Stratégie Conservatrice

### 5.1 Pourquoi refuser le "Takeover" immédiat à l'Étape 8 ?
Une reprise en main complète du pilotage par FC2 (*Takeover*) nécessite :
* La reprise du suivi de trajectoire et de l'état d'estimation de filtres de Kalman sans à-coup (*bumpless transfer*).
* La bascule physique ou logicielle des lignes de commande des actionneurs.

Cette transition représente un saut de complexité majeur. À ce stade, nous adoptons une approche **conservatrice et robuste** : la mise en configuration de sécurité (*Safe Mode / Mission Abort*).

### 5.2 Séquence de Réponse à une Défaillance

```text
[FC1 Failure / Crash / Timeout]
              │
              ▼
    FC2 détection (Timeout 300ms)
              │
              ▼
    FC2 transition -> SUSPECTED_FAILURE -> SAFE_MODE
              │
              ▼
    FC2 émet SafetyCommand(ENTER_SAFE_MODE / ABORT_MISSION)
              │
              ▼
    Aircraft / Système bascule en configuration sûre
    (Ex: Rallumage parachute d'urgence, descente stabilisée à régime fixe, mise à zéro des gaz)
              │
              ▼
    Mission annulée (Mission Aborted)
```

---

## 6. Protocole de Communication et Trames

Trois messages minimalistes sont définis pour garantir un couplage faible entre FC1 et FC2.

### 6.1 Message `Heartbeat` (FC1 → FC2)
Périodicité : **100 ms**

| Champ | Type / Format | Description |
| :--- | :--- | :--- |
| `sequence_number` | `uint32_t` | Compteur incrémentiel de trames |
| `timestamp_ms` | `uint64_t` | Horodatage système de l'émetteur |
| `health_state` | `uint8_t` | `0: UNKNOWN`, `1: HEALTHY`, `2: DEGRADED`, `3: FAILED` |

### 6.2 Message `Status` (FC1 → FC2)
Périodicité : **50 ms à 100 ms**

| Champ | Type / Format | Description |
| :--- | :--- | :--- |
| `timestamp_ms` | `uint64_t` | Horodatage système de l'émetteur |
| `position_xyz` | `float[3]` | Position courante estimée $(x, y, z)$ (m) |
| `attitude_prh` | `float[3]` | Attitude courante : Pitch, Roll, Heading (rad) |
| `mission_state` | `uint8_t` | État de la mission nominale (ID de phase) |
| `error_flags` | `uint32_t` | Masque de bits d'erreurs logiques / matérielles |

### 6.3 Message `SafetyCommand` (FC2 → FC1)
Émission : Périodique ou Événementielle (dès réévaluation de l'état de sécurité)

| Champ | Type / Format | Description |
| :--- | :--- | :--- |
| `command_id` | `uint8_t` | Enumération des ordres de sécurité |
| `checksum` / `crc` | `uint16_t` | Code de contrôle d'intégrité de l'ordre |

**Valeurs du `command_id` :**
* `0x00`: `NONE` (Aucune restriction, fonctionnement nominal).
* `0x01`: `ABORT_MISSION` (Interruption douce du plan de vol).
* `0x02`: `ENTER_SAFE_MODE` (Passage immédiat en mode de secours / atterrissage d'urgence).

---

## 7. Décision d'Architecture Système (Processus Indépendants vs Threads)

### 7.1 Choix d'Implémentation : Deux Processus OS Séparés
Sur la plate-forme de développement/simulation PC, **FC1 et FC2 sont exécutés sous la forme de deux processus systèmes totalement indépendants** (`fc_primary.exe` et `fc_safety.exe`), communiquant via sockets UDP localhost (ou mémoire partagée/IPC).

```
┌─────────────────────────────────────────────────────────────────┐
│                    Système Hôte (PC Windows/Linux)              │
│                                                                 │
│  ┌──────────────────────┐             ┌──────────────────────┐  │
│  │ aircraft_simulator   │             │     fc_primary       │  │
│  │     (Processus)      │             │     (Processus 1)    │  │
│  └──────────┬───────────┘             └──────────┬───────────┘  │
│             │                                    │              │
│             │ Sensors (UDP 5001)                 │ Heartbeat    │
│             ▼                                    │ (UDP 5002)   │
│  ┌──────────────────────┐                        │              │
│  │      fc_safety       │◄───────────────────────┘              │
│  │     (Processus 2)    │                                       │
│  └──────────────────────┘                                       │
└─────────────────────────────────────────────────────────────────┘
```

### 7.2 Justification Comparative : Processus vs Threads

| Critère | Multi-threading dans un seul processus | Multi-processus indépendants |
| :--- | :--- | :--- |
| **Isolation Mémoire** | ❌ Nulle (Espace d'adressage partagé). Un segfault dans FC1 tue FC2. | ✅ Totale. Espace mémoire virtuelles étanches via MMU OS. |
| **Erreurs de pointeurs** | ❌ Une écriture mémoire corrompue dans FC1 peut écraser l'état de FC2. | ✅ Protégé par le système d'exploitation. |
| **Comportement au Crash** | ❌ Le crash du processus arrête l'intégralité du binaire. | ✅ Si `fc_primary` crash, `fc_safety` continue de tourner. |
| **Représentativité Materielle**| ❌ Très éloigné de deux calculateurs physiques distincts. | ✅ Proche du modèle réseau de 2 cartes électroniques. |

---

## 8. Représentation Complète de l'Architecture (Étape 8)

```
                      ┌───────────────────┐
                      │ Aircraft Simulator│
                      └─────────┬─────────┘
                                │
                           SensorData
                                │
                                ▼
                    ┌────────────────────┐
                    │        FC1         │
                    │                    │
                    │ SensorTask         │
                    │ ControlTask        │
                    │ MissionTask        │
                    │ ActuatorTask       │
                    │ CommunicationTask  │
                    └───────┬────────────┘
                            │
                       ControlCommand
                            │
                            ▼
                         Aircraft

                    ┌────────────────────┐
                    │        FC2         │
                    │                    │
                    │ MonitoringTask     │
                    │ SafetyManager      │
                    │ CommunicationTask  │
                    └─────────┬──────────┘
                              │
                        SafetyCommand
                              │
                              ▼
                             FC1
```

---

## 9. Trajectoire d'Évolution et Feuille de Route (Roadmap)

L'utilisation de processus indépendants établit une abstraction propre permettant de franchir progressivement les étapes d'incarnation matérielle :

```text
[PC Independent Processes (UDP / IPC)]   <-- Étape 8
                │
                ▼
[RTOS Tasks (FreeRTOS / Threads dédiés sur bus IPC interne)]
                │
                ▼
[STM32 Target (Emulation / Dual Core ST)]
                │
                ▼
[2 x Physical Hardware Controllers (Bus CAN / Serial link)]
```

---

## 10. Prochaine Étape : Étape 9

L'**Étape 9** abordera l'ingénierie détaillée du **Protocole de Communication Inter-FC** :
* Sérialisation / Désérialisation binaire des structures de données (`Heartbeat`, `Status`, `SafetyCommand`).
* Gestion de l'ordre des octets (*Endianness*).
* Codes de détection d'erreurs (CRC16/CRC32).
* Machine à états du protocole de communication et gestion des paquets corrompus ou perdus.
