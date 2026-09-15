# Index de la documentation

Chaque sujet possède une référence principale. Les guides renvoient à cette
référence au lieu de recopier ses tables, commandes ou états.

## Découvrir et exécuter

| Document | Responsabilité |
| --- | --- |
| [Présentation](../README.md) | Objectif et parcours de démarrage |
| [Compilation](build/build_targets.md) | Prérequis, commandes, noms des produits |
| [Architecture](architecture/overview.md) | Composants et modes SIL/loopback/matériel |
| [Schéma logiciel](architecture/architecture.svg) | Vue SIL/loopback |
| [Banc STM32](hil/hardware.md) | Inventaire, identités et câblage |
| [Flash](hil/flashing.md) | Chargement et vérification des rôles |
| [SIL](validation/sil.md) | Exécution et observation de la simulation |
| [HIL](validation/hil.md) | Sélection de cible, exécution et métriques |
| [Monte Carlo](validation/monte_carlo.md) | Dispersion, reproductibilité et statistiques |
| [Digital Twin](digital_twin/README.md) | Lancement, live et replay du viewer |

## Comprendre les contrats

| Document | Responsabilité |
| --- | --- |
| [Aéronef](system/aircraft.md) | Composants du modèle |
| [Dynamique](system/flight_dynamics.md) | Causalité actionneurs/mouvement et hypothèses |
| [Mission](system/mission.md) | Phases et critères de maintien |
| [Interfaces](system/software_interfaces.md) | Données, producteurs et unités |
| [Répartition FC1/FC2](system/distributed_architecture.md) | Justification de l'isolation et limites |
| [Ordonnancement](system/scheduling.md) | Horloges, périodes et délais |
| [Communication inter-FC](system/communication.md) | Messages et trames FC1 ↔ FC2 |
| [Architecture HIL](hil/hil_architecture.md) | Répartition physique, boucle et limites du firmware |
| [Protocole HIL](hil/hil_protocol.md) | Contrat binaire PC ↔ FC1 |
| [Conception du runner](hil/hil_runner_design.md) | Étapes de la boucle et échéances |
| [Guide du code HIL](hil/hil_code_guide.md) | Carte de lecture des modules |
| [Télémétrie du viewer](digital_twin/telemetry.md) | Contrat et sémantique d'affichage |
| [Assets 3D](digital_twin/assets.md) | Critères de conception des modèles visuels |

## Sûreté et validation

| Document | Responsabilité |
| --- | --- |
| [Taxonomie](safety/fault_taxonomy.md) | Vocabulaire, états, événements et cibles autorisées |
| [Gestion des fautes](system/fault_handling.md) | Parcours de lecture de la chaîne de sûreté |
| [FMECA](fmeca/fmeca.md) | Périmètre, échelles et synthèse de l'analyse |
| [Modes FMECA détaillés](fmeca/failure_modes.md) | Causes, effets, criticité et mitigation par mode |
| [Catalogue](validation/scenarios.md) | Scénarios lançables, paramètres par cible |
| [Matrice](validation/test_matrix.md) | Traçabilité des comportements vers les tests |
| [Preuves](validation/evidence.md) | Statuts, provenance et interprétation des résultats |
| [Campagne HIL](hil/hil_validation.md) | Ordre des essais et critères à relever |
| [Bilan](validation/validation_report.md) | État des conclusions publiées |
| [Récupération capteur](demonstrations/fault_recovery.md) | Déroulement commenté d'une faute temporaire |
| [Abandon de mission](demonstrations/mission_abort.md) | Déroulement commenté d'une faute critique |

## Contribution et évolution

- [Règles C++](../AGENTS.md) : référence des conventions de contribution.
- [Formatter/linter](../formatter_and_linter/README.md) : utilisation et comportements des outils.
- [Feuille de route](architecture/roadmap.md) : propositions non implémentées.

## Entretien documentaire

Mettre à jour le document propriétaire d'une information, puis ses liens et
les éventuels résumés. Extraire une section lorsqu'elle constitue un sujet
indépendant ; conserver un point d'entrée à l'ancien emplacement si utile.
Distinguer code implémenté, projet d'évolution et résultat capturé. Ne pas
éditer à la main les sorties générées sous `validation/data/`.

Les anciens points d'entrée [test_simulation.md](test_simulation.md),
[hil_runner_results.md](hil/hil_runner_results.md) et
[digital_twin.md](../digital_twin.md) renvoient aux guides actuels.
