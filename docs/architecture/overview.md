# Architecture actuelle

Cette page décrit les responsabilités et les modes d'exécution présents dans le
code. L'[index](../README.md) sépare les références actuelles des
[évolutions proposées](roadmap.md).

## Composants

| Composant | Responsabilité | Source |
| --- | --- | --- |
| `Aircraft` | Dynamique et vérité terrain de l'aéronef simulé | [Simulation](../../Src/Simulation/) |
| `FlightController` / FC1 | Contrôle en cascade, mixage des servos, mission | [Control](../../Src/Control/) |
| `HealthMonitor` / FC2 | Détection à partir des mesures et du lien | [Safety](../../Src/Safety/) |
| `SafetyManager` | Décision de sûreté à partir du diagnostic | [Safety](../../Src/Safety/) |
| `SilRunner` | Orchestration déterministe et injection environnementale | [SIL](../../Src/SIL/) |
| `HilRunner` | Échanges avec FC1, cadence réelle, collecte et verdict | [HIL](../../Src/Embedded/Hil/) |
| Viewer | Présentation des observations | [Digital Twin](../digital_twin/README.md) |

Le contrôleur consomme les mesures issues du chemin capteur. La vérité terrain
reste séparée pour évaluer l'erreur ; le type `AircraftState` peut représenter
l'un ou l'autre selon son producteur. Voir les [contrats de données](../system/software_interfaces.md).

## Modes d'exécution

| Mode | FC1 | FC2 | Liaisons |
| --- | --- | --- | --- |
| SIL / Monte Carlo | Objet hôte | Objets de supervision selon le runner | `CommsBus` pour le SIL |
| HIL loopback | `HostFcTarget` dans le processus hôte | Supervision hôte | Trames HIL sur `LoopbackTransport`, bus simulé |
| HIL physique | Firmware Zephyr sur STM32 | Firmware Zephyr sur une seconde STM32 | Série PC ↔ FC1, USART3 FC1 ↔ FC2 |

Les trois exécutables hôtes partagent la bibliothèque `flight_sim_core`.
Les firmwares compilent les modules métier nécessaires dans leurs propres images
via les listes CMake des applications. Il n'y a pas de processus hôtes FC1/FC2
séparés ni de transport inter-FC UDP/CAN implémenté.

## Flux logiciel

```text
Aircraft → mesures capteurs → FC1 → commandes → actionneurs simulés → Aircraft
                              │
                              └→ supervision FC2 → décision de sûreté
```

La répartition matérielle et le retour du diagnostic FC2 sont détaillés dans
l'[architecture HIL](../hil/hil_architecture.md). Le
[schéma SVG](architecture.svg) illustre la boucle logicielle SIL/loopback.

## Temps et observation

Le SIL avance à pas fixe sans cadence murale. Le HIL cadence ses échanges avec
une horloge monotone. Les firmwares ont chacun une boucle applicative Zephyr ;
ils n'implémentent pas le découpage historique en six tâches FreeRTOS.
Les fréquences et horloges de référence sont dans l'[ordonnancement](../system/scheduling.md).

Les traces d'événements décrivent les transitions ; les télémétries capteur et
vérité terrain permettent de comparer observation et simulation. Le viewer
consomme ces données sans faire avancer le modèle.

## Limites

- L'aéronef et ses périphériques sont simulés : voir les [hypothèses physiques](../system/flight_dynamics.md).
- La disponibilité du HIL physique ne prouve pas ses performances : voir le [rapport de validation](../validation/validation_report.md).
- Les cibles de faute autorisées sont limitées : voir la [taxonomie](../safety/fault_taxonomy.md).
- L'absence de reprise de pilotage FC2, de watchdog local et de garde numérique
  générique est analysée dans la [FMECA](../fmeca/fmeca.md).
- Les scénarios HIL configurables côté PC ne transmettent pas tous leurs
  paramètres au firmware : voir les [limites matérielles](../hil/hil_architecture.md#limites-actuelles).

## Principes

Le code métier reste indépendant de son exécuteur. Les erreurs sont explicites,
les buffers du chemin temps réel ont une taille bornée et les modules séparent
interfaces et implémentations. Les règles C++ sont maintenues dans
[AGENTS.md](../../AGENTS.md), la séparation détection/réponse dans la
[taxonomie de sûreté](../safety/fault_taxonomy.md).
