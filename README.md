# Distributed Flight Control & Safety Testbed

Banc C++23 de développement et de validation du contrôle de vol, de la supervision
et de la sûreté d'un aéronef simulé de type Heliblade-like.

Le projet comprend le SIL (simulation logicielle), les campagnes Monte Carlo,
le HIL (boucle avec calculateur) et un viewer Digital Twin. **Les STM32 sont
physiquement disponibles** : le banc documenté comporte deux Nucleo-L476RG,
avec les firmwares FC1 et FC2 sous Zephyr. L'aéronef, ses capteurs et ses
actionneurs restent simulés.

## Commencer

1. [Compiler les exécutables et firmwares](docs/build/build_targets.md).
2. [Choisir un scénario](docs/validation/scenarios.md) et [exécuter le SIL](docs/validation/sil.md).
3. Pour le matériel : [préparer le banc](docs/hil/hardware.md), puis [flasher FC1/FC2](docs/hil/flashing.md).
4. [Exécuter et interpréter le HIL](docs/validation/hil.md).
5. [Afficher le Digital Twin](docs/digital_twin/README.md).

## Comprendre le système

| Sujet | Référence |
| --- | --- |
| Composants, responsabilités et modes d'exécution | [Architecture](docs/architecture/overview.md) |
| Hypothèses physiques et mission | [Aéronef](docs/system/aircraft.md) · [Mission](docs/system/mission.md) |
| Défaillances, détection et réponse | [Taxonomie](docs/safety/fault_taxonomy.md) · [FMECA](docs/fmeca/fmeca.md) |
| Couverture des tests et limites des preuves | [Matrice](docs/validation/test_matrix.md) · [Rapport](docs/validation/validation_report.md) |
| Démonstrations | [Récupération capteur](docs/demonstrations/fault_recovery.md) · [Abandon de mission](docs/demonstrations/mission_abort.md) |
| Règles de contribution C++ | [AGENTS.md](AGENTS.md) |

L'[index documentaire](docs/README.md) donne le rôle de chaque document.
