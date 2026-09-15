# FMECA — Distributed Flight Control & Safety Testbed

## Objet et périmètre

Analyse simplifiée des modes de défaillance du domaine SYSTEM : contrôle,
supervision, capteurs/actionneurs simulés et communication inter-calculateurs.
Le banc possède désormais deux STM32 physiques sous Zephyr ; voir
l'[architecture](../architecture/overview.md) et le [banc matériel](../hil/hardware.md).
La présence du matériel n'étend pas cette analyse à l'électronique complète,
aux alimentations ou au vol réel.

Les erreurs de CLI/fichiers/outillage et de protocole/transport du banc sont
classées séparément dans les domaines INFRASTRUCTURE et HIL par la
[taxonomie](../safety/fault_taxonomy.md), qui définit aussi le vocabulaire.
La [mission](../system/mission.md) fixe l'objectif fonctionnel.

## Analyse des modes

Les causes, effets locaux/système, détection, réponse, gravité, occurrence,
détectabilité et limites sont développés dans un document dédié :

| Mode | Analyse |
| --- | --- |
| FM-01 | [FC1 indisponible](failure_modes.md#fm-01--fc1_unavailable) |
| FM-02 | [Perte de communication](failure_modes.md#fm-02--fc_communication_loss) |
| FM-03 | [Communication dégradée](failure_modes.md#fm-03--communication_degraded) |
| FM-04 | [Mesure capteur invalide](failure_modes.md#fm-04--invalid_sensor_data) |
| FM-05 | [Actionneur dégradé](failure_modes.md#fm-05--actuator_degraded) |
| FM-06 | [Échéance de contrôle dépassée](failure_modes.md#fm-06--control_deadline_missed) |
| FM-07 | [État numérique invalide](failure_modes.md#fm-07--invalid_numerical_state) |

## Échelles qualitatives

Afin d'éviter une fausse précision, les niveaux suivants sont qualitatifs.

### Gravité

| Niveau | Signification |
|---:|---|
| 1 | Impact faible, mission pratiquement inchangée |
| 2 | Dégradation fonctionnelle, mission potentiellement maintenue |
| 3 | Perte importante d'une fonction ou abandon probable de mission |
| 4 | Perte d'une fonction critique / passage en état sûr nécessaire |

### Détectabilité

| Niveau | Signification |
|---:|---|
| 1 | Détection rapide et explicite |
| 2 | Détection explicite avec délai limité |
| 3 | Détection indirecte ou dépendante du comportement |
| 4 | Détection absente ou non implémentée |

### Occurrence

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


## Couverture et traçabilité

La [taxonomie](../safety/fault_taxonomy.md) précise les cibles injectables ;
la [matrice de tests](../validation/test_matrix.md) associe mécanismes et tests.
Le [catalogue](../validation/scenarios.md) distingue les scénarios lançables
et les mécanismes de composants. La campagne
[Monte Carlo](../validation/monte_carlo.md) CLI ne couvre pas les fautes.

Un mécanisme implémenté ou une assertion présente ne suffit pas à conclure
« validé » : appliquer les [conventions de preuve](../validation/evidence.md).
Les critères temporels sont ceux des scénarios/tests, pas des exigences
universelles aéronautiques. Les délais de la cible matérielle sont dans
l'[ordonnancement](../system/scheduling.md).

## Lacunes majeures et suites

- FC2 ne reprend pas entièrement le pilotage après perte de FC1.
- Les injections capteurs et actionneurs couvrent des canaux limités.
- La supervision heartbeat distante ne remplace pas un watchdog local de tâche.
- Le comptage des deadlines du banc ne couvre pas FM-06 au niveau système.
- La validation d'altitude ne remplace pas une garde générique d'intégrité numérique.

Les recommandations par mode restent dans l'analyse détaillée ; les travaux
transversaux sont regroupés dans la [feuille de route](../architecture/roadmap.md).

## Limites de l'analyse

Cette FMECA concerne le prototype de contrôle avec aéronef simulé. Elle ne
constitue pas une analyse de certification, électrique, structurelle,
batterie/énergie ou des conditions opérationnelles réelles. Les occurrences
restent qualitatives et devront être recalibrées à partir de données terrain.
