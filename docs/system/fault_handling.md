# Guide d'Architecture : Sûreté de Fonctionnement et Gestion des Fautes (Fault Management)

> **Référence canonique** : la taxonomie des fautes (domaines, modes de défaillance, événements de
> détection, actions de sécurité et matrice de couverture) est définie dans
> [`docs/safety/fault_taxonomy.md`](../safety/fault_taxonomy.md). En cas de divergence entre ce guide
> et le code, la taxonomie fait foi.

## 1. Philosophie et Principes Fondamentaux

L'objectif principal de cette étape (Étape 10) est d'assurer la **sûreté de fonctionnement** (*dependability*) et la **gestion des défaillances** (*fault management*) de l'aéronef. Nous faisons évoluer l'architecture d'un fonctionnement nominal (*« ça fonctionne quand tout va bien »*) vers un système tolérant aux pannes et résilient (*« le système sait quoi faire en cas d'anomalie »*).

Cette démarche s'aligne directement sur les exigences industrielles critiques et les spécifications d'équipements embarqués hautement disponibles (ex. Health Monitoring, watchdog matériel/logiciel, safe mode, dégradation progressive).

### 1.1 Périmètre des fautes modélisées
Plutôt que d'essayer de modéliser une infinité de pannes imprévisibles, nous sélectionnons quatre pannes représentatives :

```
FC1
 │
 ├── Crash / Silence (Absence de heartbeat)
 ├── Capteur invalide (Plage hors normes / dynamique impossible)
 ├── Actionneur dégradé (Écart consigne vs mesure RPM)
 └── Rupture de communication (Perte du bus inter-calculateurs)
```

### 1.2 Cycle de vie d'une défaillance
Pour chaque événement perturbateur, le traitement respecte la séquence déterministe suivante :

```
┌─────────┐     ┌───────────┐     ┌───────────┐     ┌──────────┐     ┌──────────────────────┐
│ FAILURE │ ──► │ DETECTION │ ──► │ DIAGNOSIS │ ──► │ RESPONSE │ ──► │ RECOVERY / SAFE MODE │
└─────────┘     └───────────┘     └───────────┘     └──────────┘     └──────────────────────┘
```

---

## 2. Machine à États de Santé du Système (System Safety State Machine)

Le système maintient un état de santé global calculé en temps réel. Les états sont hiérarchisés de la manière suivante :

### 2.1 Définition des états
1. **`HEALTHY`** : Tout fonctionne conformément aux spécifications nominables.
   - FC1 : OK
   - Capteurs (IMU, Altimètre) : OK
   - Actionneurs (Moteurs/RPM) : OK
   - Communication Inter-FC : OK

2. **`DEGRADED`** : Un sous-système secondaire présente une anomalie ou une dégradation, mais l'aéronef conserve son observabilité et sa contrôlabilité globale. La mission peut continuer en mode restreint ou faire l'objet d'une reconfiguration.
   - FC1 : OK
   - Capteur ou bus de comm : Dégradé / Suspect ⚠️
   - Contrôlabilité de l'aéronef : Conservée ✅

3. **`SAFE`** : Le système subit une défaillance majeure empêchant la poursuite normale de la mission. L'objectif prioritaire devient l'amenée et le maintien de l'aéronef dans un état d'équilibre physique sécurisé.
   - Mission : `ABORT` (Abandon immédiat)
   - Contrôle de vol : `SAFE MODE` (Mode conservateur)

4. **`FAILED`** *(réservé, non implémenté)* : Perte d'intégrité totale. Cet état était déclaré dans l'énumération `HealthState` mais aucune chaîne de détection ne le produisait (état mort) ; il a été retiré de l'enumération et sera réintroduit lorsqu'un mécanisme de détection de perte d'intégrité existera. Voir `docs/safety/fault_taxonomy.md`.

### 2.2 Diagramme de transition d'états

```
                  ┌───────────┐
                  │  HEALTHY  │
                  └─────┬─────┘
                        │
                fault / anomaly
                        ▼
                  ┌───────────┐
       Recovery   │ DEGRADED  │
     ┌──────────► └─────┬─────┘
     │                  │
     │          critical fault
     │                  ▼
     │            ┌───────────┐
     └─────────── │   SAFE    │
                  └─────┬─────┘
                        │
                 unrecoverable
                        ▼
                  ┌───────────┐
                  │  FAILED   │
                  └───────────┘
```

---

## 3. Architecture Découplée : Health Monitor vs Safety Manager

Une règle fondamentale d'architecture système embarqué consiste à **séparer strictement la surveillance de la décision**.

- **Le `HealthMonitor`** collecte les données de santé individuelles, analyse l'état matériel et logiciel, puis produit une structure de données synthétique : le `HealthStatus`.
- **Le `SafetyManager`** consomme le `HealthStatus`, applique la politique de sécurité (Safety Policy) et émet des ordres de commande de sûreté : les `SafetyCommand`.

### 3.1 Flux de données et contrôle

```
             Sensors / FC / Actuators / Comms
                            │
                            ▼
                     ┌──────────────┐
                     │ HealthMonitor│
                     └──────┬───────┘
                            │
                            ▼
                     ┌──────────────┐
                     │ HealthStatus │
                     └──────┬───────┘
                            │
                            ▼
                     ┌──────────────┐
                     │ SafetyManager│
                     └──────┬───────┘
                            │
                            ▼
                     ┌──────────────┐
                     │ SafetyCommand│
                     └──────────────┘
```

### 3.2 Exemple concrétisé
```
Donnée brute : IMU invalid (Valeur aberrante)
       │
       ▼
HealthMonitor  ──► Calcule : imu_status = DEGRADED
       │
       ▼
HealthStatus   ──► overall = DEGRADED, imu = DEGRADED
       │
       ▼
SafetyManager  ──► Décision : Commutateur sur mode dégradé ou ENTER_SAFE_MODE
```

---

## 4. Traitement Détaillé des Fautes

### 4.1 Panne n°1 : Perte du calculateur principal (FC1 Crash / Silence)

#### Description
Le calculateur secondaire (FC2) surveille en permanence la présence de FC1 via un message de trame périodique (*heartbeat*).

```
FC1 ──heartbeat (périodique)──► FC2  [OK]
FC1 ─────── X (Crash FC1) ────► FC2  [TIMEOUT]
```

#### Traitement par FC2
1. **Detection** : Un temporisateur constate le dépassement de l'échéance du heartbeat (`heartbeat_timeout > 100ms`).
2. **Diagnosis** : Basculement du statut FC1 de `HEALTHY` vers `SUSPECTED_FAILURE`.
3. **Response** : Validation du silence de FC1. Activation de l'autorité exclusive de FC2.
4. **Action** : Émission de la commande `ENTER_SAFE_MODE` sur le bus système.

---

### 4.2 Panne n°2 : Défaillance Capteur (Altimètre / IMU Invalide)

#### Description
Un capteur transmet une mesure invalide ou physiquement impossible (ex. saut d'altitude brutal de 100 m à 8400 m ou valeur négative incohérente -120 m).

#### Stratégie de validation
La détection repose sur deux niveaux de filtres :
1. **Validation de plage (Range Check)** : `min_val <= value <= max_val`
2. **Validation dynamique (Rate-of-Change Check)** : `|value(t) - value(t-1)| / Δt <= max_dynamic_rate`

```
100 m  ──► Valid
101 m  ──► Valid
99 m   ──► Valid
8400 m ──► Range / Dynamic Fault !
-120 m ──► Out of Bounds Fault !
```

#### Traitement
1. **Detection** : Invalidation immédiate de la donnée brute au niveau du driver/SensorTask.
2. **Diagnosis** : `HealthMonitor` classe l'altimètre en `DEGRADED`.
3. **Response** :
   - Si un estimateur alternatif existe (ex. fusion IMU/Baro) ──► passage en mode **`DEGRADED`** (vol maintenu).
   - Si aucune source alternative n'est disponible ──► **`SAFE_MODE`**.

---

### 4.3 Panne n°3 : Dégradation Actionneur (Écart de RPM Moteur)

#### Description
L'asservissement d'un rotor ne parvient plus à atteindre la vitesse demandée (ex. frottement mécanique, perte partielle de puissance).

```
commanded_rpm = 900
actual_rpm    = 450
```

#### Algorithme de surveillance
Une alerte d'actionneur est levée si l'écart entre consigne et mesure dépasse un seuil pendant une durée continue minimum :

$$	ext{Error} = | 	ext{commanded\_rpm} - 	ext{measured\_rpm} | > 	ext{threshold\_rpm}$$
$$	ext{Condition} : 	ext{Error maintenue pendant } t > t_{	ext{persistence}}$$

#### Traitement
1. **Detection** : Le moniteur de santé lève l'événement de détection `ACTUATOR_MISMATCH` (écart consigne/mesure RPM soutenu).
2. **Diagnosis** : `HealthMonitor` enregistre `actuator_health = DEGRADED`.
3. **Response** : Le `SafetyManager` réduit l'enveloppe de vol autorisée ou déclenche le `SAFE_MODE` si la poussée minimale de sécurité n'est plus garantie.

---

### 4.4 Panne n°4 : Perte de Communication Inter-Calculateurs

#### Description
Les liaisons de communication entre FC1 et FC2 sont interrompues (ex. coupure physique du bus bus inter-board).

```
FC1 ─────────── X ───────────► FC2
```

#### Traitement
1. **Distinction essentielle** : La perte de communication ne signifie pas nécessairement que FC1 est mort, mais qu'aucun calculateur ne peut plus vérifier l'état de santé de l'autre.
2. **Philosophie conservatrice** :
   - `COMMUNICATION_TIMEOUT` détecté par la supervision du lien.
   - En l'absence de certitude quant à la coordination multi-calculateur, le système applique la règle de prudence maximale.
   - Passage immédiat en **`SAFE_MODE`**.

---

## 5. Surveillance d'Exécution : Watchdog Intelligent (Logiciel / Matériel)

### 5.1 Découplage de la preuve de fonctionnement
Le Watchdog réponds à la question : *« Le calculateur exécute-t-il toujours correctement ses tâches critiques ? »*

Un piège classique consiste à réarmer (*kick*) le watchdog au sein d'une simple boucle d'interruption ou d'un thread séparé non lié à la logique métier. Dans ce cas, un programme dont la boucle de contrôle principale est totalement bloquée continuerait de rafraîchir le watchdog.

### 5.2 Le Watchdog à Preuve Multi-Tâches (Task Progression Check)
Notre watchdog exige la preuve d'avancement déterministe de l'ensemble des tâches critiques :

```
                  ┌─────────────┐
                  │ ControlTask │ ──► Executed
                  └──────┬──────┘
                         │
                  ┌──────┴──────┐
                  │ SensorTask  │ ──► Executed
                  └──────┬──────┘
                         │
                  ┌──────┴──────┐
                  │ HealthTask  │ ──► Executed
                  └──────┬──────┘
                         │
                         ▼
               ┌──────────────────┐
               │ WatchdogManager  │
               └─────────┬────────┘
                         │
             All tasks completed?
                         │
             ┌───────────┴───────────┐
            YES                     NO
             │                       │
      Kick Watchdog            Timeout / Reset
```

---

## 6. Spécification Détaillée du Mode de Sécurité (SAFE MODE)

### 6.1 Décision d'Architecture Physique pour l'Aéronef
Lors du basculement en **`SAFE_MODE`**, l'aéronef abandonne sa mission nominale et bascule dans une configuration de vol conservatrice visant à minimiser l'énergie cinétique et garantir la stabilité.

```
                      ┌───────────┐
                      │ SAFE MODE │
                      └─────┬─────┘
                            │
            ┌───────────────┼───────────────┐
            ▼               ▼               ▼
      Mission Abort   Attitude Stabilization Safe Descent Rate
      (Stop Target)   (Roll=0°, Pitch=0°)    (Target Alt/Land)
```

### 6.2 Paramètres de consigne en SAFE MODE
1. **Gestion de Mission** :
   - State = `MISSION_ABORTED`
   - Annulation instantanée des trajectoires complexes, évitements et manœuvres.
   - Arrêt du maintien de position stationnaire de haute précision (*station keeping*).

2. **Asservissement d'Attitude** :
   - Consigne de Roulis (*Roll Target*) = $0.0^\circ$ (Rétablir l'horizontale).
   - Consigne de Tangage (*Pitch Target*) = $0.0^\circ$ (Annuler les vitesses horizontales).
   - Consigne de Vitesse de Lacet (*Yaw Rate*) = $0.0^\circ/	ext{s}$ (Fixer le cap).

3. **Gestion Verticale et Poussée** :
   - Consigne d'Altitude = `SAFE_ALTITUDE` (Ex. altitude de repli prédéfinie ou taux de descente stabilisé).
   - En cas de perte de poussée partielle : ajustement équilibré de la puissance sur les moteurs sains pour maintenir l'assiette plate.

---

## 7. Matrice Synthétique de Gestion des Fautes (Fault Matrix)

Le tableau suivant récapitule la matrice de décision de la sûreté de fonctionnement :

| Panne / Anomale | Mécanisme de Détection | Classification d'État | Réponse Système (Safety Manager) | Mode de Récupération (Recovery) |
| :--- | :--- | :--- | :--- | :--- |
| **FC1 Heartbeat perdu** | Timeout sur trame périodique ($>100	ext{ ms}$) | `DEGRADED` → `SAFE` | Transfert d'autorité à FC2, Abandon mission, Commande `ENTER_SAFE_MODE` | Non réouvrable en vol (Reset au sol) |
| **Capteur Altimètre Invalide** | Hors plage (Range Check) ou dérive extrême | `DEGRADED` | Basculement sur estimateur secondaire (IMU/Baro) ; si inexistant → `SAFE_MODE` | Réinitialisation du capteur et ré-acquisition |
| **Moteur / RPM Trop Faible** | Écart $|	ext{cmd} - 	ext{mesure}| > 	ext{seuil}$ sur $\Delta t$ | `DEGRADED` → `SAFE` | Réduction des exigences d'attitude / Réduction vitesse max ; activation `SAFE` si poussée insuffisante | Retour au mode `DEGRADED` si RPM rétabli |
| **Communication Inter-FC perdue** | Timeout bus de données | `SAFE` | Abandon immédiat de mission, prise de consignes stabilisées conservatrices | Rétablissement de la liaison de comm |
| **Tâche critique bloquée (`ControlTask`)** | Expiration temporisateur `WatchdogManager` | `SAFE` / `FAILED` | Soft-reset de la tâche ou basculement processeur de secours | Auto-récupération par reset watchdog |

---

## 8. Organisation Générale de l'Architecture de Sécurité

### 8.1 Représentation matérielle & logicielle duale

```
                       CALCULATEUR FC1
┌───────────────────────────────────────────────────────────┐
│                                                           │
│   ┌──────────────────┐             ┌──────────────────┐   │
│   │ Flight Controller│             │  Health Monitor  │   │
│   └────────┬─────────┘             └────────┬─────────┘   │
│            │                                │             │
│            └───────────────┬────────────────┘             │
│                            ▼                              │
│                   ┌──────────────────┐                    │
│                   │ Watchdog Manager │                    │
│                   └──────────────────┘                    │
└────────────────────────────┬──────────────────────────────┘
                             │ Heartbeat & Bus Status
                             ▼
                       CALCULATEUR FC2
┌───────────────────────────────────────────────────────────┐
│                                                           │
│   ┌──────────────────┐             ┌──────────────────┐   │
│   │   Comm Monitor   │             │  Health Monitor  │   │
│   └────────┬─────────┘             └────────┬─────────┘   │
│            │                                │             │
│            └───────────────┬────────────────┘             │
│                            ▼                              │
│                   ┌──────────────────┐                    │
│                   │  Safety Manager  │                    │
│                   └────────┬─────────┘                    │
│                            │                              │
└────────────────────────────┼──────────────────────────────┘
                             │
                             ▼
                     ┌───────────────┐
                     │ SafetyCommand │
                     └───────┬───────┘
                             │
                             ▼
                    AÉRONEF / ACTIONNEURS
```

---

## 9. Cahier de Tests de Validation de Sûreté (Test Suite)

Afin de valider la robustesse du système embarqué, la suite de tests unitaires et d'intégration doit inclure les scénarios d'injection de pannes suivants :

### Test A — Disparition brutale de FC1
- **Condition initiale** : Vol stabilisé nominal ($t = 0$ à $10	ext{s}$).
- **Action** : À $t = 10	ext{s}$, interruption de l'exécution de FC1 (Kill process / Coupure trame).
- **Résultat attendu** :
  - FC2 détecte le timeout de heartbeat.
  - Identification statut FC1 = `SUSPECTED_FAILURE`.
  - Transition de FC2 en statut global `SAFE_MODE`.
  - Émission de la consigne d'abandon de mission (`MISSION_ABORTED`).

### Test B — Actionneur / RPM Dégradé
- **Condition initiale** : Consigne moteur `RPM = 900`.
- **Action** : Injection d'une contrainte simulant un blocage mécanique (`Actual RPM = 400`).
- **Résultat attendu** :
  - Détection de la différence $|\Delta	ext{RPM}| = 500 > 	ext{threshold}$ sur la fenêtre temporelle.
  - Levée du drapeau de panne actionneur.
  - Transition du statut vers `DEGRADED` ou `SAFE` selon les seuils configurés.

### Test C — Données Capteur Absurdes / Invalides
- **Condition initiale** : Altimètre mesurant $100	ext{ m}$.
- **Action** : Injection d'une valeur aberrante `altitude = NaN` ou `altitude = 8400 m`.
- **Résultat attendu** :
  - Échec de la validation de plage (Range Check).
  - Basculement du statut du capteur en `INVALID`.
  - Passage en mode `DEGRADED` avec basculement sur estimateur secondaire.

### Test D — Blocage de la boucle de contrôle (`ControlTask` Stalled)
- **Condition initiale** : Fonctionnement nominal multi-tâches.
- **Action** : Suspension/bloquage artificiel de la tâche `ControlTask`.
- **Résultat attendu** :
  - Le `WatchdogManager` détecte l'absence de rafraîchissement de la tâche critique.
  - Invalidation de la séquence d'armement (*Kick denied*).
  - Expiration du Watchdog (Timeout) $
ightarrow$ Activation du mode d'urgence / Reset.

### Test E — Rupture de la liaison de communication inter-FC
- **Condition initiale** : FC1 et FC2 en fonctionnement nominal.
- **Action** : Perte totale des messages inter-calculateurs (Drop 100% des paquets).
- **Résultat attendu** :
  - Détection du timeout de communication par FC2.
  - Déclaratif `COMMUNICATION_TIMEOUT`.
  - Transition immédiate du système vers le **`SAFE_MODE`**.

---

## 10. Synthèse et Enseignements d'Architecture

La mise en œuvre de cette étape apporte des compétences fondamentales en ingénierie des systèmes embarqués critiques :

1. **Détection et classification déterministe** : Traitement prévisible et catégorisé des événements d'erreur.
2. **Watchdog basé sur le progrès fonctionnel** : Protection contre les blocages partiels et les boucles infinies.
3. **Dégradation progressive (*Graceful Degradation*)** : Maintien de la meilleure opérabilité possible avant tout basculement destructeur.
4. **Comportement Fail-Safe** : Garantie d'amenée du système dans un état sûr quelles que soient les défaillances simples survenues.
