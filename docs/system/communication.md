# Spécification du Protocole de Communication Inter-FC (FC1 ↔ FC2)

> **Evolution note.** The concrete transports discussed here (`UDPTransport`,
> `CANTransport`, `MessageHeader` byte envelope) are **not** implemented as such.
> The current code uses the in-process `CommsBus` (SIL) and the HIL
> `LoopbackTransport`/`HilTransport` with the real HIL-Proto codec (CRC16). The
> transport *abstraction* (`Transport`) is real and underlies both. See
> [`../architecture/overview.md`](../architecture/overview.md).


## 1. Vue d'Ensemble & Objectifs

Ce document spécifie le protocole et l'architecture de communication entre le calculateur principal de vol (**FC1** - Primary Flight Controller) et le calculateur de sécurité / surveillance (**FC2** - Safety & Monitoring Flight Controller).

### 1.1 Contexte Architectural
* **FC1 (Contrôle Normal)** : Exécute les fonctions de guidage, navigation, contrôle d'attitude et suivi de la mission nominale.
* **FC2 (Safety / Monitoring)** : Assure la surveillance indépendante de la santé de FC1 et de la dynamique du véhicule. En cas de défaillance, il déclenche les procédures de sécurité d'urgence.

### 1.2 Schéma des Flux de Communication
```text
   FC1 (Primary)                             FC2 (Safety)
        │                                         │
        ├────────────── Heartbeat ───────────────►│ (Périodique : 10 Hz / 100 ms)
        ├────────────── Status ──────────────────►│ (Périodique : 50 Hz / 20 ms)
        │                                         │
        │◄───────────── SafetyCommand ────────────│ (Sur événement / Périodique : 10 Hz)
        │                                         │
```

### 1.3 Risques Réseau & Robustesse
Le protocole est conçu pour gérer et mitiger les défaillances réseau suivantes :
1. **Message perdu** (*Packet loss*)
2. **Message retardé / Jitter** (*Packet delay*)
3. **Message corrompu** (*Data corruption*)
4. **Message dupliqué** (*Packet duplication*)
5. **Déséquencement** (*Packet reordering*)
6. **Liaison interrompue** (*Communication failure*)

---

## 2. Abstraction de Transport (`ITransport`)

Afin de garantir l'indépendance vis-à-vis du support physique de communication, la couche applicative s'interface uniquement avec une classe abstraite `ITransport`.

### 2.1 Architecture en Couches
```text
 ┌────────────────────────────────────────────────────────┐
 │                    Couche Application                  │
 │          (CommunicationTask / SafetyManager)          │
 └───────────────────────────┬────────────────────────────┘
                             │
                             ▼
 ┌────────────────────────────────────────────────────────┐
 │                Interface ITransport                    │
 │    + send(msg: RawPacket) : Status                     │
 │    + receive(out msg: RawPacket, timeout) : Status     │
 └───────────────────────────┬────────────────────────────┘
                             │
       ┌─────────────────────┼─────────────────────┐
       ▼                     ▼                     ▼
┌──────────────┐      ┌──────────────┐      ┌──────────────┐
│MockTransport │      │ UDPTransport │      │ CANTransport │
│ (Test/Simu)  │      │ (SIL/Ethernet│      │  (Hardware   │
│              │      │      PC)     │      │   Embarqué)  │
└──────────────┘      └──────────────┘      └──────────────┘
```

### 2.2 Découplage Applicatif
L'application FC1 / FC2 interagit uniquement via les méthodes génériques :
* `send(message)`
* `receive(message)`

Ce découplage permet de valider toute la logique logicielle sur PC via `MockTransport` ou `UDPTransport` avant tout déploiement sur bus matériel (`CANTransport`, RS-422, Ethernet temps-réel).

---

## 3. Structure des Messages & Enveloppe Commune (`MessageHeader`)

Tous les messages échangés partagent une enveloppe commune (`MessageHeader`) encapsulant la charge utile (*Payload*).

### 3.1 Definition du `MessageHeader`

| Champ | Type | Taille (octets) | Description |
| :--- | :--- | :--- | :--- |
| `message_type` | `uint8_t` | 1 | Identifiant du message (`1` = Heartbeat, `2` = Status, `3` = SafetyCommand) |
| `version` | `uint8_t` | 1 | Version du format de message (Rétrocompatibilité) |
| `sequence_number` | `uint32_t` | 4 | Numéro de séquence strictement croissant (Monotone) |
| `timestamp` | `uint64_t` | 8 | Horodatage d'émission en microsecondes ($\mu	ext{s}$) |
| `crc32` | `uint32_t` | 4 | Somme de contrôle pour validation d'intégrité du paquet |

### 3.2 Diagramme du Layout Mémoire du Message

```text
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  message_type |    version    |           (Réservé)           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        sequence_number                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           timestamp                           |
|                          (uint64_t)                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                             crc32                             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                            PAYLOAD                            |
|                              ...                              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### 3.3 Intérêt du Champ `version`
Le champ `version` protège le système contre les incohérences de mise à jour entre FC1 et FC2 (ex. si FC1 passe en version 2 avec de nouveaux champs dans `Status` alors que FC2 est en version 1).
* FC2 rejette les messages dont le numéro de version n'est pas pris en charge.
* Évite l'interrogation erronée de données corrompues ou déplacées en mémoire.

---

## 4. Spécification Détaillée des Messages

### 4.1 Message `Heartbeat` (FC1 → FC2)
Le `Heartbeat` est un signal périodique minimal confirmant le fonctionnement nominal du système d'exploitation et de l'exécuteur applicatif de FC1.

* **Émetteur** : FC1 (Primary Flight Controller)
* **Récepteur** : FC2 (Safety Flight Controller)
* **Fréquence / Condition d'émission** : Périodique à 10 Hz (période nominale de $100	ext{ ms}$).
* **Structure du Payload** :
  * `health_state` (`uint8_t` - Enum) : `0`: HEALTHY, `1`: DEGRADED, `2`: FAULTY.
  * `cpu_load` (`uint8_t`) : Charge processeur en % ($0 - 100$).
* **Comportement à la réception (FC2)** :
  * Met à jour le timestamp du dernier heartbeat reçu (`last_heartbeat_timestamp`).
  * Vérifie la continuité de la séquence (`sequence_number`).
  * Maintient l'état de surveillance actif (`MONITORING`).

---

### 4.2 Message `Status` (FC1 → FC2)
Le message `Status` fournit à FC2 l'état cinématique et fonctionnel complet de FC1 afin de permettre des contrôles de cohérence avancés (vérification du domaine de vol, dérive d'attitude).

* **Émetteur** : FC1
* **Récepteur** : FC2
* **Fréquence / Condition d'émission** : Périodique à 50 Hz (période nominale de $20	ext{ ms}$).
* **Structure du Payload** :
  * `position_x` (`float`) : Position Nord/X (mètres).
  * `position_y` (`float`) : Position Est/Y (mètres).
  * `altitude` (`float`) : Altitude relative / AGL (mètres).
  * `pitch` (`float`) : Tangage (radians).
  * `roll` (`float`) : Roulis (radians).
  * `yaw` (`float`) : Lacet (radians).
  * `mission_state` (`uint8_t` - Enum) : `0`: INIT, `1`: STANDBY, `2`: IN_FLIGHT, `3`: RETURNING, `4`: LANDED.
  * `fault_flags` (`uint32_t` - Bitmask) : Masque de bits indiquant les pannes internes détectées par FC1 (capteurs, actionneurs, bus).
* **Comportement à la réception (FC2)** :
  * Analyse la cohérence des valeurs physiques par rapport aux enveloppes sécurisées.
  * Compare l'état estimé par FC1 avec les propres capteurs autonomes de FC2.

---

### 4.3 Message `SafetyCommand` (FC2 → FC1)
Ce message contient les ordres d'urgence transmis par FC2 pour imposer une action de mise en sécurité à FC1.

* **Émetteur** : FC2
* **Récepteur** : FC1
* **Fréquence / Condition d'émission** :
  * Émission sur événement dès détection d'une anomalie.
  * Émission périodique de maintien à 10 Hz.
* **Structure du Payload** :
  * `command` (`uint8_t` - Enum) :
    * `0`: NONE (Aucune action d'urgence requis).
    * `1`: ENTER_SAFE_MODE (Passage en mode de vol dégradé/sécurisé).
    * `2`: ABORT_MISSION (Interruption immédiate de la mission / Atterrissage d'urgence).
  * `reason_code` (`uint16_t`) : Code identifiant le motif du déclenchement.
* **Comportement à la réception (FC1)** :
  * Exécution prioritaire au sein du système d'interruption / tâche haute priorité.
  * Transition immédiate vers le mode commandé (ex: `ENTER_SAFE_MODE` ou `ABORT_MISSION`).

---

## 5. Temporalité, Timeouts & Distinction d'Erreurs

### 5.1 Configuration des Temporisations
Les seuils temporels sont configurables (non codés en dur dans la logique métier) :

```cpp
struct CommConfig {
    uint32_t heartbeat_period_ms  = 100; // Période d'émission (10 Hz)
    uint32_t status_period_ms     = 20;  // Période d'émission (50 Hz)
    uint32_t heartbeat_timeout_ms = 300; // Seuil de détection de perte (3 x Période)
    uint32_t confirmation_time_ms = 600; // Seuil de confirmation de panne franche
};
```

### 5.2 Chronogramme de Détection de Timeout

```text
FC1  ──HB(0ms)────────HB(100ms)────────HB(200ms)───────── X (Interruption des communications) ──►
        │               │               │
FC2  ──HB(0ms)────────HB(100ms)────────HB(200ms)─────────────────────────────────────────────────►
        │               │               │                  │                     │
      t=0ms           t=100ms         t=200ms            t=500ms               t=800ms
                                                      (Timeout 300ms)       (Confirmation 600ms)
                                                          ▼                     ▼
                                                   SUSPECTED_FAILURE        SAFE_MODE
```

### 5.3 Distinction : Timeout Réseau vs Panne FC1
Une absence de réception de `Heartbeat` ne signifie pas obligatoirement que FC1 s'est effondré.

* **COMMUNICATION FAILURE** : Le processeur FC1 fonctionne, mais le canal réseau subit une perte de paquets, de la latence ou des interférences.
* **FC1 FAILURE** : Le calculateur FC1 est bloqué (Crash, Watchdog Reset, Panne d'alimentation).

Afin d'éviter des décisions prématurées non justifiées, FC2 utilise l'état intermédiaire `SUSPECTED_FAILURE`. Si la communication se rétablit pendant cette phase, le système revient en `MONITORING` sans déclencher d'interruption irréversible.

---

## 6. Machine à États de Surveillance de FC2

### 6.1 Diagramme de Transition d'États

```text
                     ┌───────────────────────────┐
                     │                           │
                     │        MONITORING         │◄─────────────────────────┐
                     │ (Fonctionnement Nominal)  │                          │
                     └─────────────┬─────────────┘                          │
                                   │                                        │
                                   │ Heartbeat Timeout (> 300 ms)           │ Communication Rétablie
                                   │ ou Jitter / Perte excessive            │ (Heartbeat valide reçu)
                                   ▼                                        │
                     ┌───────────────────────────┐                          │
                     │     SUSPECTED_FAILURE     │──────────────────────────┘
                     │ (Anomalie temporaire détectée)
                     └─────────────┬─────────────┘
                                   │
                                   │ Absence prolongée (> 600 ms)
                                   │ ou Incohérence dynamique majeure
                                   ▼
                     ┌───────────────────────────┐
                     │         SAFE_MODE         │
                     │  (Procédure d'urgence /   │
                     │    Sécurisation du vol)   │
                     └─────────────┬─────────────┘
                                   │
                                   │ Ordre critique d'abandon
                                   ▼
                     ┌───────────────────────────┐
                     │       ABORT_MISSION       │
                     │ (Atterrissage/Arrêt Urg.) │
                     └───────────────────────────┘
```

### 6.2 Traitement des Transitions
1. **`MONITORING`** : FC2 reçoit les trames nominales. Il envoie `SafetyCommand(NONE)`.
2. **`SUSPECTED_FAILURE`** : Déclenché quand $t - t_{	ext{last\_hb}} > 	ext{heartbeat\_timeout\_ms}$.
   * Si un Heartbeat valide arrive avant $	ext{confirmation\_time\_ms}$, retour en `MONITORING`.
3. **`SAFE_MODE`** : Déclenché si l'anomalie persiste au-delà de $	ext{confirmation\_time\_ms}$ (600 ms). FC2 bascule en mode de repli et émet `SafetyCommand(ENTER_SAFE_MODE)`.

---

## 7. Gestion des Erreurs & Injection de Fautes

Le simulateur de transport (`MockTransport` / `FaultInjector`) doit pouvoir simuler les perturbations réseau réelles pour tester la robustesse de l'implémentation.

### 7.1 Matrice de Traitement des Fautes

| Faute Injectée | Mécanisme de Détection | Action / Mitigation Logicielle |
| :--- | :--- | :--- |
| **Packet Loss** (Message perdu) | Rupture d'incrément dans `sequence_number` ($seq_n > seq_{n-1} + 1$). | Tolérance aux pertes isolées. Déclenchement du timeout uniquement en cas de pertes consécutives. |
| **Packet Delay** (Message retardé) | Écart important entre `timestamp` et l'horloge système de réception. | Si le retard dépasse le jitter toléré, la trame est ignorée pour éviter d'utiliser des données obsolètes. |
| **Packet Duplication** (Message dupliqué) | Réception d'un message avec $seq_n \le seq_{	ext{dernier\_reçu}}$. | Rejet pur et simple de la trame dupliquée. |
| **Packet Reordering** (Déséquencement) | Réception de $seq_n < seq_{	ext{max\_reçu}}$. | Utilisation d'un tampon de réordonnancement ou rejet des trames obsolètes. |
| **Packet Corruption** (Message corrompu) | Erreur de validation de la somme `crc32`. | Destruction immédiate de la trame corrompue au niveau de la couche `ITransport`. |
| **Communication Interrompue** | Absence de paquets reçus pendant $t > 	ext{timeout}$. | Évolution de la machine à états de FC2 (`SUSPECTED_FAILURE` $ightarrow$ `SAFE_MODE`). |

---

## 8. Métriques de Performance de Communication

Pour alimenter les futures études Monte-Carlo et valider la QoS (Qualité de Service) de la communication, le module enregistre en continu les métriques suivantes :

```cpp
struct CommunicationMetrics {
    double   latency_ms;          // Latence instantanée et moyenne (ms)
    double   worst_case_latency;  // Latence au pire des cas (WCTT - ms)
    double   packet_loss_rate;    // Taux de perte de paquets (%)
    uint32_t reorder_count;       // Nombre de paquets reçus hors ordre
    uint32_t duplicate_count;     // Nombre de paquets dupliqués
    uint32_t crc_error_count;     // Nombre de paquets corrompus
    uint32_t timeout_count;       // Nombre de bascules en SUSPECTED_FAILURE
    double   recovery_time_ms;    // Temps de rétablissement moyen (ms)
};
```

Exemple d'objectifs de performance en simulation :
* **Latence moyenne** : $< 5.0	ext{ ms}$
* **Pire cas de latence (WCET/WCTT)** : $< 30.0	ext{ ms}$
* **Taux de perte toléré sans impact** : $< 2.0\%$

---

## 9. Architecture Logicielle & Modèle de Déploiement

### 9.1 Diagramme d'Architecture Logicielle

```text
                      FC1 (Primary)
                       │
                CommunicationTask
                       │
                       ▼
                  ITransport
                       │
                       │ (MockTransport / UDPTransport / CANTransport)
                       ▼
                ─── network ───
                       │
                       ▼
                  ITransport
                       │
                CommunicationTask
                       │
                       ▼
                   FC2 (Safety)
                       │
                       ▼
                 SafetyManager
                       │
                       ▼
                 SafetyCommand ───────► (Retour vers FC1)
```

### 9.2 Stratégie de Validation Progressive sur PC
L'ensemble de ce système de communication peut être développé, testé et validé à 100% sur un PC hôte sans matériel physique :
1. **Étape SIL (Software-In-the-Loop)** :
   * Compilation de `fc_primary.exe` et `fc_safety.exe`.
   * Communication via `MockTransport` (mémoire partagée / queues en mémoire) ou `UDPTransport` (Loopback 127.0.0.1).
   * Execution d'essais sous injection de fautes (perte de paquets 5%, latence 50ms, etc.).
2. **Étape HIL (Hardware-In-the-Loop)** :
   * Substitution de `UDPTransport` par `CANTransport` sur microcontrôleurs cibles (ex. STM32 / CAN-FD).
   * Le code des tâches applicatives (`CommunicationTask`, `SafetyManager`) reste 100% identique.
