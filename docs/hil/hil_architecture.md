# Architecture HIL

Le HIL possède deux configurations exécutables : émulation hôte et banc STM32.
L'[inventaire matériel](hardware.md) décrit les cartes disponibles ; la
[vue générale](../architecture/overview.md) situe le HIL dans le projet.

## Répartition physique

```text
PC : HilRunner + Aircraft + capteurs/actionneurs simulés + traces
  │ SensorPacket                              ▲ ActuatorPacket + diagnostic FC2
  ▼                                           │
FC1 : Zephyr + FlightController ────────────────┘
  │ heartbeat + mesures de supervision        ▲ acquittement + état/détection
  ▼                                           │
FC2 : Zephyr + HealthMonitor + SafetyManager ───┘
```

Le PC orchestre les échanges avec FC1. FC1 traite les mesures et exécute le
contrôleur partagé. FC2 reçoit les heartbeats ainsi que l'altitude et les RPM
mesurés/commandés, évalue la santé et renvoie son diagnostic à FC1.
FC1 insère ce diagnostic dans sa réponse HIL. Le runner le convertit en
`HealthReport`, puis applique la politique de sûreté au modèle simulé.

Sources : [FC1](../../apps/fc1_stm32/src/main.cpp),
[FC2](../../apps/fc2_stm32/src/main.cpp),
[décodage de la santé côté PC](../../Src/Embedded/Hil/Runner/Safety/HilRunnerSafety-Health.cpp).

## Configuration loopback

Le runner instancie `HostFcTarget` dans son processus et échange les mêmes trames
binaires via `LoopbackTransport`. La supervision est alors exécutée sur le PC.
Ce mode sert aux tests sans dépendance aux cartes et n'apporte aucune mesure MCU.

Le [câblage logiciel des canaux](../../Src/Embedded/Hil/Runner/Wiring/HilRunner-Channel.cpp)
sélectionne `HostFcTarget` ou `RemoteFcTarget` selon l'interface résolue.

## Références par responsabilité

| Sujet | Référence |
| --- | --- |
| Cartes, ports et câblage | [Banc matériel](hardware.md) |
| Compilation / flash | [Compilation](../build/build_targets.md) · [Flash](flashing.md) |
| Trames PC ↔ FC1 | [HIL-Proto](hil_protocol.md) |
| Messages FC1 ↔ FC2 | [Communication inter-FC](../system/communication.md) |
| Horloges et cadence | [Ordonnancement](../system/scheduling.md) |
| Boucle du runner | [Conception du runner](hil_runner_design.md) |
| Exécution et métriques | [Guide HIL](../validation/hil.md) |

## Limites actuelles

- Le firmware FC1 utilise une cible fixe `(0, 0, 10 m)` et un pas de `0.01 s`.
  Le protocole ne transmet pas `HilConfig::target` ni les gains du contrôleur.
  Les profils `NOMINAL-012..017` sont configurés côté hôte ; leur présence dans
  le catalogue ne démontre pas leur équivalence sur la carte.
- L'injection d'indisponibilité FC1 sur matériel supprime ses heartbeats
  inter-FC, tout en conservant les réponses HIL. Elle ne coupe pas l'alimentation
  du MCU. Voir la [démonstration d'abandon](../demonstrations/mission_abort.md).
- Le chemin série exploite le diagnostic FC2 relayé par FC1. La supervision
  et l'application de la sûreté doivent donc être évaluées aussi lorsque le
  lien PC ↔ FC1 ou FC1 lui-même cesse complètement de répondre.
- Les champs de profilage CPU/pile/deadline du protocole ne sont pas renseignés
  par une instrumentation complète du firmware. Zéro par défaut ne signifie
  pas zéro utilisation ou zéro dépassement mesuré.
- Les actionneurs restent sur le PC. La reprise autonome du pilotage et les
  autres extensions sont décrites dans la [feuille de route](../architecture/roadmap.md).
