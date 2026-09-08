# FMECA — Distributed Flight Control & Safety Testbed

## 1. Objet

Cette FMECA constitue une analyse simplifiée de sûreté de fonctionnement du prototype logiciel **Distributed Flight Control & Safety Testbed**.

Le système étudié simule un aéronef de type Heliblade-like avec :

- un Flight Controller primaire (FC1) ;
- un calculateur de supervision (FC2) ;
- des capteurs simulés ;
- des actionneurs simulés ;
- une communication inter-calculateurs ;
- un modèle dynamique simplifié ;
- un dispositif de simulation SIL ;
- une infrastructure HIL destinée à être exécutée ultérieurement avec un microcontrôleur réel.

### Objectif de mission

L'aéronef doit atteindre une altitude cible puis maintenir sa position dans une zone opérationnelle pendant une durée déterminée.

La mission nominale est structurée ainsi :

```text
TAKEOFF
   ↓
CLIMB
   ↓
STATION_KEEPING
   ↓
COMPLETE
```

En cas de défaillance critique :

```text
FAILURE
   ↓
DETECTION
   ↓
DIAGNOSIS / CLASSIFICATION
   ↓
SAFETY ACTION
   ↓
SAFE MODE / RECOVERY
```

---

# 2. Périmètre

Cette analyse porte uniquement sur les **Failure Modes du domaine SYSTEM**.

Sont exclus de la FMECA principale :

- erreurs de configuration des scénarios ;
- erreurs CLI ;
- erreurs d'écriture de fichiers ;
- erreurs internes de l'outillage de test ;
- erreurs spécifiques du protocole HIL ;
- erreurs de transport propres au banc HIL.

Ces éléments appartiennent respectivement aux domaines `HIL` ou `INFRASTRUCTURE` et sont volontairement séparés des défaillances de l'aéronef ou de son logiciel embarqué.

---

# 3. Architecture considérée

```text
                       ┌─────────────────────┐
                       │   Aircraft Model    │
                       │  Physics / Sensors  │
                       └──────────┬──────────┘
                                  │
                             Sensor data
                                  │
                                  ▼
                       ┌─────────────────────┐
                       │        FC1          │
                       │                     │
                       │ Flight Control      │
                       │ Mission Control     │
                       └──────────┬──────────┘
                                  │
                           Actuator commands
                                  │
                                  ▼
                       ┌─────────────────────┐
                       │     Actuators       │
                       └─────────────────────┘


                       ┌─────────────────────┐
                       │        FC2          │
                       │                     │
                       │ Health Monitoring   │
                       │ Safety Manager      │
                       └──────────┬──────────┘
                                  │
                             Safety actions
                                  │
                                  ▼
                                 FC1
```

Communication FC1 ↔ FC2 :

```text
FC1 ─────────────────────────────► FC2
       heartbeat / status

FC2 ─────────────────────────────► FC1
       safety command
```

---

# 4. Safety vocabulary

## 4.1 Failure Mode

Une **Failure Mode** décrit la cause ou condition défaillante injectée ou postulée.

Exemples :

```text
FC1_UNAVAILABLE
FC_COMMUNICATION_LOSS
INVALID_SENSOR_DATA
```

## 4.2 Detection Event

Une **Detection Event** décrit le mécanisme ayant détecté une anomalie.

Exemples :

```text
FC1_HEARTBEAT_TIMEOUT
COMMUNICATION_TIMEOUT
SENSOR_VALIDATION_FAILED
ACTUATOR_MISMATCH
```

## 4.3 Health State

```text
HEALTHY
DEGRADED
SAFE
```

## 4.4 Safety Mode

```text
NORMAL
COMPENSATED
SAFE_MODE
```

## 4.5 Safety Action

```text
RESUME_NORMAL
ENTER_COMPENSATED
ENTER_SAFE_MODE
```

Cette séparation est normative pour cette analyse : un événement de détection ne doit pas être utilisé comme nom du Failure Mode, et inversement.

---

# 5. Échelle utilisée

Afin d'éviter une fausse précision, les niveaux suivants sont qualitatifs.

## 5.1 Gravité

| Niveau | Signification |
|---:|---|
| 1 | Impact faible, mission pratiquement inchangée |
| 2 | Dégradation fonctionnelle, mission potentiellement maintenue |
| 3 | Perte importante d'une fonction ou abandon probable de mission |
| 4 | Perte d'une fonction critique / passage en état sûr nécessaire |

## 5.2 Détectabilité

| Niveau | Signification |
|---:|---|
| 1 | Détection rapide et explicite |
| 2 | Détection explicite avec délai limité |
| 3 | Détection indirecte ou dépendante du comportement |
| 4 | Détection absente ou non implémentée |

## 5.3 Occurrence

L'occurrence n'est **pas une probabilité réelle d'apparition en exploitation**.

Le projet ne dispose pas de données terrain permettant de calculer une fréquence de panne crédible.

Les valeurs indiquées ci-dessous représentent uniquement la plausibilité relative du mode de défaillance dans le modèle du prototype :

| Niveau | Signification |
|---:|---|
| 1 | peu probable / cas particulier |
| 2 | plausible |
| 3 | facilement provoqué ou fréquent dans le modèle |
| 4 | structurel / très facile à reproduire |

L'occurrence devra être recalibrée avec des données réelles si ce modèle devait évoluer vers une analyse industrielle.

---

# 6. FMECA principale

## FM-01 — FC1_UNAVAILABLE

### Fonction concernée

Assurer le contrôle de vol et la progression de mission.

### Failure Mode

```text
FC1_UNAVAILABLE
```

Le calculateur de contrôle primaire cesse d'être opérationnel.

Dans le prototype, cela est représenté par :

```text
fc1_alive = false
```

ce qui entraîne l'arrêt des émissions de heartbeat et de commandes.

### Cause / scénario

- crash du processus ;
- arrêt de l'exécution ;
- perte du comportement attendu de FC1 ;
- absence prolongée de heartbeat.

### Effets locaux

FC1 ne produit plus :

- heartbeat ;
- état ;
- nouvelles commandes de contrôle.

### Effets système

Perte de la fonction primaire de contrôle.

La mission ne peut plus être poursuivie normalement.

### Détection

```text
FC1_HEARTBEAT_TIMEOUT
```

Le mécanisme de supervision observe l'absence de message FC1 pendant le délai configuré.

### État système

```text
HEALTHY
   ↓
SAFE
```

### Safety Action

```text
ENTER_SAFE_MODE
```

### Réponse

- abandon de mission ;
- passage en `SAFE_MODE` ;
- descente contrôlée ;
- état final de mission `ABORTED`.

### Couverture

**Implémenté et testé.**

### Critères temporels actuels

Détection attendue dans les limites du scénario ; les tests utilisent un objectif de détection inférieur ou égal à 300 ms.

### Gravité

**4 — Critique**

### Occurrence prototype

**2 — Plausible**

### Détectabilité

**1 — Très bonne**

### Mitigation

Heartbeat supervision + `SAFE_MODE` + mission abort.

### Limites

Le système ne réalise pas actuellement un takeover complet de FC1 par FC2.

---

# FM-02 — FC_COMMUNICATION_LOSS

### Fonction concernée

Communication et coordination FC1 ↔ FC2.

### Failure Mode

```text
FC_COMMUNICATION_LOSS
```

FC1 reste actif mais le chemin de communication inter-calculateurs devient indisponible.

### Cause / scénario

```text
comms_link_up = false
```

Tous les paquets du lien sont perdus.

### Effets locaux

FC2 ne reçoit plus :

- heartbeat ;
- état FC1 ;
- données de supervision.

### Effets système

FC2 ne peut plus garantir la supervision de FC1.

La mission nominale n'est plus considérée comme sûre.

### Détection

```text
COMMUNICATION_TIMEOUT
```

### État système

```text
HEALTHY
   ↓
SAFE
```

### Safety Action

```text
ENTER_SAFE_MODE
```

### Réponse

- abandon mission ;
- descente contrôlée ;
- passage `SAFE_MODE`.

### Couverture

**Implémenté et testé en SIL et HIL.**

### Gravité

**4 — Critique**

### Occurrence prototype

**2 — Plausible**

### Détectabilité

**1 — Très bonne**

### Mitigation

Supervision de lien + safe mode.

### Remarque

Ce mode de panne est distinct de `FC1_UNAVAILABLE` même si les deux peuvent produire une réponse de sécurité identique.

---

# FM-03 — COMMUNICATION_DEGRADED

### Fonction concernée

Communication FC1 ↔ FC2.

### Failure Mode

```text
COMMUNICATION_DEGRADED
```

La liaison reste disponible mais certains messages sont perdus.

### Cause / scénario

```text
comms_loss_probability = p
```

### Effets locaux

- certains heartbeats sont perdus ;
- la latence ou la continuité de supervision peut être perturbée ;
- les compteurs de perte augmentent.

### Effets système

Deux comportements possibles :

#### Cas nominalement absorbé

La supervision continue à recevoir suffisamment de heartbeats.

```text
COMMUNICATION_DEGRADED
        ↓
aucun passage SAFE
```

La mission continue.

#### Cas aggravé

Le silence devient suffisamment important pour dépasser la fenêtre de supervision.

```text
COMMUNICATION_DEGRADED
        ↓
FC1_HEARTBEAT_TIMEOUT
        ↓
SAFE
```

### Détection

La perte individuelle d'un paquet n'entraîne pas directement de mode SAFE.

Le système utilise la supervision temporelle du heartbeat.

### Couverture

**Implémenté.**

**Couverture déterministe limitée.**

La couverture actuelle repose principalement sur la simulation de pertes et la campagne Monte Carlo ; aucun scénario déterministe exhaustif ne définit aujourd'hui la frontière exacte entre perte absorbée et perte conduisant à un timeout.

### Gravité

**2 à 3 selon le niveau de dégradation**

### Occurrence prototype

**3 — Facilement reproductible**

### Détectabilité

**2 — Détection par conséquences temporelles**

### Mitigation

Tolérance aux pertes de paquets + supervision heartbeat.

### Point de validation restant

Définir précisément les performances minimales acceptables en fonction de :

- taux de perte ;
- latence ;
- durée du silence.

---

# FM-04 — INVALID_SENSOR_DATA

### Fonction concernée

Mesure de l'état de l'aéronef.

### Failure Mode

```text
INVALID_SENSOR_DATA
```

Un canal capteur fournit une mesure invalide.

### Modes actuellement modélisés

```text
AltitudeNaN
AltitudeOutOfRange
ExtremeNoise
```

### Effets locaux

Le Flight Controller ne peut plus considérer la mesure comme fiable.

Dans le cas actuel, FC1 conserve la dernière mesure valide.

### Détection

```text
SENSOR_VALIDATION_FAILED
```

Validation :

- NaN ;
- borne physique ;
- cohérence élémentaire de la mesure.

### État système

```text
HEALTHY
   ↓
DEGRADED
```

### Safety Action

```text
ENTER_COMPENSATED
```

Avec marge de poussée nominale pour une faute capteur.

### Réponse

- conserver la dernière mesure valide ;
- poursuivre la mission ;
- rester en mode dégradé ;
- retour possible vers `NORMAL` lorsque les données redeviennent valides.

### Couverture actuelle

**Implémenté et testé.**

Mais le périmètre réel est limité.

### Cible réellement injectable

```text
ALTITUDE / BAROMETER
```

### Non couvert actuellement

```text
IMU
GNSS
RPM feedback
position sensor corruption
```

Ces cibles sont déclarées mais aucune voie d'injection correspondante n'est actuellement autorisée par l'usine.

### Gravité

**2 — Importante mais contrôlable**

### Occurrence prototype

**3 — Facilement reproductible**

### Détectabilité

**1 à 2** pour les cas NaN / hors plage.

### Mitigation

Validation capteur + hold de la dernière valeur valide + mode compensé.

### Limitation importante

La détection actuelle n'est pas équivalente à une détection de dérive silencieuse ou de biais physique.

Une valeur incorrecte mais restant dans les limites peut passer la validation.

---

# FM-05 — ACTUATOR_DEGRADED

### Fonction concernée

Production de la force/commande nécessaire au maintien de l'aéronef.

### Failure Mode

```text
ACTUATOR_DEGRADED
```

L'actionneur rotatif principal ne produit pas l'effet demandé avec l'efficacité attendue.

### Représentation actuelle

```text
actuator_efficiency < 1
```

L'efficacité réduit la réponse réelle au RPM commandé.

### Effets locaux

```text
RPM commanded ≠ RPM actual
```

### Effets système

- réduction de la capacité de contrôle ;
- capacité de maintien d'altitude dégradée ;
- risque d'augmentation de l'erreur de trajectoire.

### Détection

```text
ACTUATOR_MISMATCH
```

La détection exige :

```text
|commanded_rpm - actual_rpm| > threshold
```

pendant la durée minimale configurée.

### Paramètres actuels

```text
mismatch threshold = 60 RPM
hold time = 0.50 s
```

### État système

```text
HEALTHY
   ↓
DEGRADED
```

### Safety Action

```text
ENTER_COMPENSATED
```

### Réponse

Application d'une marge de poussée de :

```text
1.7 ×
```

pour la faute actionneur actuellement modélisée.

La mission peut continuer.

### Couverture

**Implémenté et testé.**

### Cible couverte

```text
ROTATION_SYSTEM / MAIN ROTOR
```

### Non couvert

```text
LEFT_SERVO
RIGHT_SERVO
```

### Gravité

**3 — Importante**

### Occurrence prototype

**3 — Facilement reproductible**

### Détectabilité

**2 — Détection après maintien du mismatch**

### Mitigation

Monitoring RPM + compensation + mode dégradé.

### Limitation

La détection ne couvre pas actuellement toutes les formes de panne de servo ou de perte de dynamique des commandes d'attitude.

---

# FM-06 — CONTROL_DEADLINE_MISSED

### Fonction concernée

Exécution périodique de la boucle de contrôle.

### Failure Mode

```text
CONTROL_DEADLINE_MISSED
```

Une boucle de contrôle ne termine pas dans le délai prévu.

### Origine potentielle

- surcharge CPU ;
- blocage ;
- traitement trop long ;
- contention ;
- problème d'ordonnancement.

### Effets système potentiels

- commandes retardées ;
- dégradation de la stabilité ;
- perte de qualité du station keeping ;
- risque de perte de contrôle si les dépassements deviennent persistants.

### Détection actuelle

Le banc HIL possède :

```text
DEADLINE_MISSED
```

mais cette détection appartient au **domaine HIL**.

Elle mesure le respect de la boucle du banc.

### Couverture du Failure Mode

**Non implémentée comme faute SYSTEM.**

Il n'existe actuellement pas :

```text
CONTROL_TASK_STALL
```

ou équivalent injectable dans FC1.

### Safety Response

**Non définie pour le système embarqué réel.**

### Gravité

**4 — Potentiellement critique**

### Occurrence prototype

**1 — Non modélisée**

### Détectabilité

**4 — Non couverte au niveau système**

### Recommandation future

Introduire un modèle de tâche bloquée ou de dépassement de deadline côté FC1, puis définir :

```text
deadline miss
    ↓
detection
    ↓
diagnosis
    ↓
safe / degraded response
```

---

# FM-07 — INVALID_NUMERICAL_STATE

### Fonction concernée

Intégrité numérique du modèle et des données de contrôle.

### Failure Mode

```text
INVALID_NUMERICAL_STATE
```

Le système physique ou logiciel produit une valeur non valide :

```text
NaN
Infinity
invalid physical state
```

### Causes potentielles identifiées

- dispersion physique extrême ;
- paramètre physique invalide ;
- propagation numérique ;
- calcul divergent.

Le code actuel présente notamment des cas potentiels liés à des dispersions non bornées.

### Effets système

Une valeur invalide pourrait contaminer :

```text
AircraftState
      ↓
Sensor data
      ↓
Controller
      ↓
Actuator command
```

et rendre les résultats de contrôle non fiables.

### Détection actuelle

Aucune détection générique de l'état physique n'est implémentée.

La validation NaN de l'altitude appartient au chemin :

```text
FM-04 INVALID_SENSOR_DATA
```

et ne constitue pas une détection générique de l'intégrité numérique du modèle.

### Couverture

**Non implémentée.**

### Gravité

**4 — Potentiellement critique**

### Occurrence prototype

**2 — Cas extrême mais reproductible dans certaines dispersions**

### Détectabilité

**4 — Non couverte actuellement**

### Recommandation future

Ajouter des gardes :

```text
isfinite()
```

sur les états critiques :

- position ;
- vitesse ;
- altitude ;
- attitude ;
- commandes.

Puis définir une réponse de sécurité explicite.

---

# 7. Modes identifiés mais volontairement non couverts

Les modes suivants ne doivent **pas être présentés comme implémentés**.

## FM-F01 — IMU failure

### Statut

Non implémenté.

### Motif

La cible `SensorImu` existe dans la taxonomie mais l'injection est rejetée.

### Intérêt futur

Tester :

- perte d'accélération ;
- perte de vitesse angulaire ;
- mesure incohérente ;
- biais ;
- blocage de valeur.

---

## FM-F02 — GNSS failure

### Statut

Non implémenté.

La cible GNSS est déclarée mais aucune injection n'est disponible.

---

## FM-F03 — Left servo failure

### Statut

Non implémenté.

La cible existe mais la dégradation actuelle ne modélise pas un servo gauche indépendant.

---

## FM-F04 — Right servo failure

### Statut

Non implémenté.

Même limitation que pour le servo gauche.

---

## FM-F05 — Control task stall / local watchdog failure

### Statut

Non implémenté.

Le vocabulaire `watchdog` est réservé à ce concept local.

La supervision heartbeat FC1↔FC2 n'est pas un watchdog.

---

# 8. Analyse de la couverture actuelle

| Failure Mode | Injection | Détection | Safety Response | SIL | HIL | Monte Carlo |
|---|:---:|:---:|:---:|:---:|:---:|:---:|
| FM-01 FC1_UNAVAILABLE | ✅ | ✅ | ✅ SAFE | ✅ | ✅ | ✅ |
| FM-02 FC_COMMUNICATION_LOSS | ✅ | ✅ | ✅ SAFE | ✅ | ✅ | ✅ |
| FM-03 COMMUNICATION_DEGRADED | ✅ | ✅ indirecte | conditionnelle | ✅ | partielle | ✅ |
| FM-04 INVALID_SENSOR_DATA | ✅ altitude | ✅ | ✅ COMPENSATED | ✅ | ✅ | ✅ |
| FM-05 ACTUATOR_DEGRADED | ✅ rotor | ✅ | ✅ COMPENSATED | ✅ | ✅ | ✅ |
| FM-06 CONTROL_DEADLINE_MISSED | ❌ | HIL only | ❌ | ❌ | bench only | ❌ |
| FM-07 INVALID_NUMERICAL_STATE | ❌ | ❌ | ❌ | ❌ | ❌ | partiellement exposé |

---

# 9. Chaînes de sécurité principales

## 9.1 Défaillance critique

```text
FM-01 / FM-02
      ↓
Detection Event
      ↓
FC1_HEARTBEAT_TIMEOUT
or
COMMUNICATION_TIMEOUT
      ↓
HealthState::SAFE
      ↓
SafetyAction::ENTER_SAFE_MODE
      ↓
SafetyMode::SAFE_MODE
      ↓
Mission ABORTED
      ↓
Controlled descent
```

## 9.2 Défaillance dégradée

```text
FM-04 / FM-05
      ↓
Detection Event
      ↓
SENSOR_VALIDATION_FAILED
or
ACTUATOR_MISMATCH
      ↓
HealthState::DEGRADED
      ↓
SafetyAction::ENTER_COMPENSATED
      ↓
SafetyMode::COMPENSATED
      ↓
Mission continues
```

---

# 10. Détection et délais

Les exigences actuellement utilisées dans le prototype incluent :

| Detection / response | Valeur actuelle |
|---|---:|
| FC1 unavailable detection | ≤ 300 ms |
| Communication loss detection | ≤ 300 ms |
| Sensor validation | ≤ 500 ms |
| Actuator compensation | ≤ 600 ms |

Ces valeurs sont des **critères de validation du prototype**, pas des exigences aéronautiques réelles.

---

# 11. Points faibles identifiés par la FMECA

Cette FMECA fait ressortir plusieurs lacunes importantes.

## 11.1 Pas de takeover FC2 → FC1

En cas de perte de FC1, FC2 met le système en sécurité mais ne reprend pas actuellement le contrôle de vol.

Le système est donc :

```text
FC1 failure
   ↓
SAFE
```

et non :

```text
FC1 failure
   ↓
FC2 takeover
```

---

## 11.2 Couverture capteurs limitée

Le mécanisme générique existe mais l'injection est essentiellement limitée au canal altitude.

Il n'est donc pas correct d'affirmer une couverture complète de :

```text
IMU
GNSS
RPM feedback
position sensor
```

---

## 11.3 Couverture actionneurs limitée

L'actionneur rotatif principal est couvert.

Les servos individuels ne le sont pas.

---

## 11.4 Watchdog local absent

Le système possède une supervision heartbeat distante mais pas de watchdog local de tâche.

Cette distinction doit rester explicite.

---

## 11.5 Intégrité numérique insuffisamment surveillée

Le modèle ne possède pas encore de mécanisme générique garantissant :

```text
isfinite(position)
isfinite(velocity)
isfinite(attitude)
isfinite(command)
```

---

# 12. Correspondance FMECA ↔ tests

La FMECA doit être reliée à la validation.

| Failure Mode | Test principal |
|---|---|
| FM-01 | FC1 failure / heartbeat timeout |
| FM-02 | communication loss |
| FM-03 | packet loss campaign |
| FM-04 | invalid altitude sensor |
| FM-05 | actuator degradation |
| FM-06 | future timing-fault test |
| FM-07 | future numerical-integrity test |

Chaque future nouvelle ligne FMECA devrait idéalement conduire à :

```text
Failure Mode
      ↓
Implementation
      ↓
Detection
      ↓
Mitigation
      ↓
Test
      ↓
Observed Result
```

---

# 13. Limites de cette FMECA

Cette analyse est volontairement limitée au prototype logiciel.

Elle ne constitue pas :

- une analyse de certification ;
- une FMECA aéronautique réglementaire ;
- une analyse complète du véhicule physique ;
- une analyse électrique ;
- une analyse structurelle ;
- une analyse batterie/énergie ;
- une analyse des conditions opérationnelles réelles ;
- une analyse de sécurité de vol réelle.

Le modèle aérodynamique lui-même est volontairement simplifié et destiné au développement du logiciel de contrôle, pas à prédire le comportement réel d'un Heliblade.

---

# 14. Conclusion

La couverture actuelle peut être résumée ainsi :

```text
                 SYSTEM FMECA
                      │
          ┌───────────┴───────────┐
          │                       │
      COVERED                  FUTURE
          │                       │
   FM-01 FC1 unavailable    FM-06 deadline
   FM-02 comm loss          FM-07 numerical state
   FM-03 comm degraded     IMU failure
   FM-04 sensor invalid    GNSS failure
   FM-05 actuator degraded servo failures
```

Le système possède déjà une chaîne de sécurité cohérente :

```text
Failure Mode
     ↓
Detection Event
     ↓
Health State
     ↓
Safety Action
     ↓
Safety Mode
     ↓
Mission outcome
```

Les modes critiques actuellement couverts sont principalement :

```text
FC1 unavailable
Communication loss
```

et les modes dégradés actuellement couverts :

```text
Invalid sensor data
Actuator degraded
```

La prochaine évolution logique de la sûreté de fonctionnement est la couverture du **timing de la boucle de contrôle** et de **l'intégrité numérique de l'état**, car ces deux domaines constituent les principaux trous identifiés par l'analyse actuelle.