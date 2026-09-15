# Guide du code HIL

Cette page sert de carte de lecture des modules actuels. La compilation est
centralisée dans le [guide de build](../build/build_targets.md).

| Responsabilité | Sources |
| --- | --- |
| CLI, découverte et sélection de cible | [Cli](../../Src/Embedded/Hil/Cli/) · [Serial](../../Src/Embedded/Hil/Transport/Serial/) |
| Configuration et catalogue | [Config](../../Src/Embedded/Hil/Config/) |
| Orchestration et verdict | [Runner](../../Src/Embedded/Hil/Runner/) |
| Cible hôte / distante | [Target](../../Src/Embedded/Hil/Target/) |
| Types et interfaces HAL | [Hal](../../Src/Embedded/Hal/) |
| Contrat binaire | [HilProtocol.cppm](../../Src/Embedded/Transport/HilProtocol.cppm) |
| Conversion et encodage | [Codec](../../Src/Embedded/Transport/Codec/) |
| Réception et CRC | [Parser](../../Src/Embedded/Transport/Parser/) |
| Acceptation et statistiques des échanges | [HilTransport](../../Src/Embedded/Hil/Transport/) |
| Liaison matérielle inter-FC | [InterFc](../../Src/Embedded/InterFc/) |
| Exécution embarquée | [FC1](../../apps/fc1_stm32/src/main.cpp) · [FC2](../../apps/fc2_stm32/src/main.cpp) |
| Publication du viewer | [Telemetry](../../Src/Embedded/Hil/Telemetry/) |

## Parcours de lecture

Commencer par la [boucle du runner](hil_runner_design.md), puis lire le
[protocole HIL](hil_protocol.md) et les fonctions de réception. Sur STM32, FC1
alimente un ring buffer depuis l'interruption UART et traite les trames dans sa
boucle principale. L'émission utilise `uart_poll_out`. Les noms `DwtClock`,
`Stm32UartTransport` et les tâches DMA des anciennes propositions ne décrivent
pas cette implémentation.

Le parser est une machine à états avec buffers fixes. Son instance doit avoir
un propriétaire ; sa consommation octet par octet ne garantit pas un accès
concurrent sûr depuis plusieurs contextes.

## Faire évoluer un message

1. Modifier le contrat et ses vérifications de taille dans le module concerné.
2. Adapter encodeur, décodeur et dispatch des deux extrémités.
3. Définir la compatibilité/version du protocole et les comportements de rejet.
4. Ajouter les vérifications de protocole pertinentes et mettre à jour sa page de référence.
5. Recompiler les deux firmwares et le runner avec le même contrat.

Les conventions de modules et de noms de fichiers restent celles d'[AGENTS.md](../../AGENTS.md).
