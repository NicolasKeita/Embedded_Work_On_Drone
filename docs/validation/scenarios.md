# Catalogue partagé des scénarios SIL/HIL

## Architecture

Les identités fonctionnelles des scénarios sont définies une seule fois dans
`Src/SIL/Core/FunctionalScenarios.cpp`. Ce registre est la source canonique
partagée par les exécuteurs SIL et HIL : l'identifiant, le mode de défaillance,
les paramètres d'injection et le résultat attendu ne doivent pas être redéfinis
dans un catalogue propre à une cible.

Les couches SIL et HIL ajoutent uniquement leurs paramètres d'exécution. Elles
peuvent donc employer des instants et des durées d'injection différents tout en
exécutant le même scénario fonctionnel.

## Scénarios partagés

| Identifiant | Description | SIL | HIL |
|---|---|---:|---:|
| `NOMINAL-001` | Maintien de position nominal, sans faute injectée | oui | oui |
| `FAULT_INJECTOR-001` | Indisponibilité du calculateur de vol principal FC1 | oui | oui |
| `FAULT_INJECTOR-003` | Mesure d'altitude/barométrique hors plage | oui | oui |

Il existe exactement deux scénarios fonctionnels d'injection de faute. Les
anciens scénarios de perte de communication, de dégradation d'actuateur et de
panne FC1 pendant la montée ne font plus partie du catalogue.

## Temporisation par cible

| Scénario | SIL | HIL |
|---|---|---|
| `FAULT_INJECTOR-001` | activation à 20 s, permanente | activation à 5 s, permanente |
| `FAULT_INJECTOR-003` | activation à 20 s pendant 10 s | activation à 5 s pendant 20 s |

La différence de temporisation est une propriété de l'environnement d'exécution,
pas une duplication du scénario fonctionnel.

## Catalogue physique et autonome

Le [catalogue SIL](../../Tests/Scenarios/Scenarios-Catalog.cpp) ajoute les
scénarios ci-dessous. Le [catalogue HIL](../../Src/Embedded/Hil/Config/HilScenarios.cpp)
expose seulement ceux marqués « oui ».

| Identifiant | Objet | SIL | HIL |
| --- | --- | --- | --- |
| `NOMINAL-002` | Repos au sol | oui | non |
| `NOMINAL-003` | Montée en boucle ouverte | oui | non |
| `NOMINAL-004` | Descente en boucle ouverte | oui | non |
| `NOMINAL-005` | Translation X | oui | non |
| `NOMINAL-006` | Translation Y | oui | non |
| `NOMINAL-007` | Translation combinée | oui | non |
| `NOMINAL-008` | Asservissement X, de 20 vers 0 m | oui | non |
| `NOMINAL-009` | Asservissement Y, de −15 vers 0 m | oui | non |
| `NOMINAL-010` | Mission autonome complète | oui | non |
| `NOMINAL-011` | Maintien à 100 m | oui | non |
| `NOMINAL-012` | Décollage vertical, z=5 m | oui | oui |
| `NOMINAL-013` | Décollage avant, x=4 m, z=6 m | oui | oui |
| `NOMINAL-014` | Décollage latéral, y=−4 m, z=7 m | oui | oui |
| `NOMINAL-015` | Décollage diagonal, x=y=3 m, z=8 m | oui | oui |
| `NOMINAL-016` | Décollage décalé, x=−3 m, y=2 m, z=9 m | oui | oui |
| `NOMINAL-017` | Montée à 20 km, environ 9 h simulées | oui | oui |

`NOMINAL-001` existe aussi dans le catalogue autonome (maintien à 10 m).
Le runner SIL sélectionne d'abord la suite partagée pour un identifiant commun ;
la campagne Monte Carlo utilise le catalogue autonome.

« HIL oui » signifie présent dans le catalogue hôte. Le firmware FC1 utilise
encore une consigne fixe ; voir les [limites matérielles](../hil/hil_architecture.md#limites-actuelles).
Le HIL est cadencé en temps réel : `NOMINAL-017` représente environ neuf heures.

## Détail des fautes

- [Abandon après indisponibilité FC1](../demonstrations/mission_abort.md).
- [Dégradation puis récupération capteur](../demonstrations/fault_recovery.md).
- [Représentation dans le viewer](../digital_twin/telemetry.md).

Ces guides complètent l'identité fonctionnelle sans redéfinir ses paramètres.

## Exécution

```sh
./artifacts/linux/sil_runner --scenario FAULT_INJECTOR-001
./artifacts/linux/sil_runner --scenario FAULT_INJECTOR-003
./artifacts/linux/hil_runner --scenario FAULT_INJECTOR-001
./artifacts/linux/hil_runner --scenario FAULT_INJECTOR-003
```
