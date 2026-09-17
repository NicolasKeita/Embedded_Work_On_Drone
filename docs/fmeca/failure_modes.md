# Analyse détaillée des modes de défaillance

Cette analyse reprend les modes SYSTEM de la [FMECA](fmeca.md).
Les échelles et le périmètre y sont définis. Les mécanismes décrits s'appliquent
au modèle logiciel ; les adaptations matérielles sont signalées explicitement.
Les comportements attendus ne remplacent pas des captures de tests.

## FM-01 — FC1_UNAVAILABLE

### Fonction concernée

Assurer le contrôle de vol et la progression de mission.

### Failure Mode

```text
FC1_UNAVAILABLE
```

Le calculateur de contrôle primaire cesse d'être opérationnel.

En SIL/loopback, cela est représenté par :

```text
fc1_alive = false
```

ce qui entraîne l'arrêt des émissions de heartbeat et de commandes.
Sur matériel, la commande de test supprime les heartbeats inter-FC tout en
maintenant les réponses HIL ; voir la [démonstration](../demonstrations/mission_abort.md).

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
- maintien de position ;
- état final de mission `ABORTED`.

### Couverture

**Implémenté ; voir la matrice de couverture et les preuves d’exécution.**

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

## FM-02 — FC_COMMUNICATION_LOSS

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
- maintien de position ;
- passage `SAFE_MODE`.

### Couverture

**Implémenté ; couverture de composants à distinguer des scénarios lançables.**

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

## FM-03 — COMMUNICATION_DEGRADED

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

La couverture repose sur la simulation de pertes dans CommsBus. La campagne Monte Carlo CLI porte sur la dispersion physique, pas sur les fautes ; aucun scénario déterministe exhaustif ne définit la frontière entre perte absorbée et timeout.

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

## FM-04 — INVALID_SENSOR_DATA

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

En SIL/loopback, FC1 conserve la dernière mesure valide. Le chemin physique
doit être évalué séparément : voir la [démonstration capteur](../demonstrations/fault_recovery.md).

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

**Implémenté ; voir la matrice de couverture et les preuves d’exécution.**

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

## FM-05 — ACTUATOR_DEGRADED

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

**Implémenté ; voir la matrice de couverture et les preuves d’exécution.**

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

## FM-06 — CONTROL_DEADLINE_MISSED

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

## FM-07 — INVALID_NUMERICAL_STATE

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
