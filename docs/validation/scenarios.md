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

## Comportement du Digital Twin

- Pour `FAULT_INJECTOR-001`, le flux de télémétrie marque le composant `mcu` de
  FC1 en panne ; la puce STM du modèle clignote en rouge.
- Pour `FAULT_INJECTOR-003`, le flux marque le composant `sensors` comme dégradé ;
  le baromètre clignote, l'altitude affichée reste sur la dernière mesure valide
  et le viewer indique explicitement que cette valeur est figée à cause de la
  panne barométrique.

## Exécution

```sh
SIL_RUNNER --scenario FAULT_INJECTOR-001
SIL_RUNNER --scenario FAULT_INJECTOR-003
HIL_RUNNER --scenario FAULT_INJECTOR-001
HIL_RUNNER --scenario FAULT_INJECTOR-003
```
