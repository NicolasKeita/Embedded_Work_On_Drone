# Communication FC1 ↔ FC2

Cette page est la référence de la liaison inter-FC actuelle. Le protocole
PC ↔ FC1 est distinct : voir [HIL-Proto](../hil/hil_protocol.md).

## SIL et HIL loopback

`CommsBus` simule la publication de heartbeats, la coupure du lien et la perte
seedée de paquets. Ce chemin reste dans le processus hôte. Il permet de tester
les mécanismes de supervision sans reproduire l'électronique du bus.

## HIL matériel

Les firmwares utilisent `FlightCore::InterFc::IInterFcTransport`, implémenté par
`ZephyrUartInterFcTransport`. `send` et `poll` renvoient `std::expected` ; un
`poll` sans message valide renvoie une valeur optionnelle vide.
Le [banc matériel](../hil/hardware.md) fixe le câblage et le débit.

| Message | Sens | Rôle |
| --- | --- | --- |
| `Heartbeat` (1) | FC1 → FC2 | Preuve de présence |
| `Acknowledgement` (2) | FC2 → FC1 | Séquence acquittée et diagnostic |
| `Status` (3) | FC2 → FC1 | Publication périodique du diagnostic, même en absence de heartbeat |
| `MonitoringSample` (4) | FC1 → FC2 | Altitude, RPM réel et commandé |
| `ResetSupervision` (5) | FC1 → FC2 | Réarmement au début d'une nouvelle exécution HIL |

## Format de trame

Référence compilée : [InterFcLink](../../Src/Embedded/InterFc/InterFcLink.cppm),
[encodeur](../../Src/Embedded/InterFc/InterFcLink-Codec.cpp) et
[parser](../../Src/Embedded/InterFc/InterFcLink-Parser.cpp).
Chaque trame contient 22 octets, avec entiers et flottants en little-endian.

| Offset | Taille | Contenu |
| --- | --- | --- |
| 0 | 2 | Synchronisation `A5 5A` |
| 2 | 1 | Version `1` |
| 3 | 1 | Type de message |
| 4 | 2 | Séquence `std::uint16_t` |
| 6 | 1 | `NodeState` : Unknown=0, Healthy=1, Degraded=2, Safe=3 |
| 7 | 1 | `DetectionCode` : None=0, Fc1HeartbeatTimeout=1, CommunicationTimeout=2, SensorValidationFailed=3, ActuatorMismatch=4 |
| 8 | 4 | Altitude en mètres (`std::float32_t`) |
| 12 | 4 | RPM mesuré (`std::float32_t`) |
| 16 | 4 | RPM commandé (`std::float32_t`) |
| 20 | 2 | CRC-16/CCITT-FALSE sur les 20 premiers octets |

Le parser contrôle synchronisation, version, CRC et valeurs d'énumération.
La trame ne contient ni timestamp distant ni compteur de charge CPU.
La continuité des séquences reçues ne constitue pas une protection contre
les duplications/rejeux : le code de supervision rafraîchit son horloge sur
réception du heartbeat accepté.

## Supervision

Les périodes et délais sont centralisés dans l'[ordonnancement](scheduling.md).
Un timeout indique une absence de preuve de vie, sans distinguer à lui seul
un MCU arrêté d'une liaison coupée. Le firmware FC2 maintient son indicateur
`link_up` ; une coupure électrique de l'UART est donc observée via le silence
des heartbeats, et non nécessairement comme `COMMUNICATION_TIMEOUT`.

Les états de santé et la politique de récupération sont définis par la
[taxonomie](../safety/fault_taxonomy.md). Les anciens messages à enveloppe CRC32,
les sockets UDP et le CAN sont des [pistes d'évolution](../architecture/roadmap.md).
