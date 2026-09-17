# Démonstration — abandon après perte de heartbeat FC1

Scénario `FAULT_INJECTOR-001` : son identité et ses fenêtres sont définies dans le
[catalogue](../validation/scenarios.md), son analyse dans la
[FMECA FM-01](../fmeca/failure_modes.md#fm-01--fc1_unavailable).

## Lancer

```sh
./artifacts/linux/hil_runner --scenario FAULT_INJECTOR-001 --interface loopback
./artifacts/linux/sil_runner --scenario FAULT_INJECTOR-001
```

Pour les cartes préparées, utiliser `--interface auto` avec les vérifications du
[guide HIL](../validation/hil.md).

## Ce que l'essai provoque

En SIL/loopback, l'injection arrête la publication de heartbeat et de commandes
par la cible hôte. Sur matériel, le runner transmet la commande de suppression
du heartbeat inter-FC à FC1 ; le MCU continue de répondre au protocole HIL.
Le test matériel exerce donc le silence de heartbeat, sans éteindre FC1.

## Observations attendues

```text
Injection → absence de heartbeat → FC1_HEARTBEAT_TIMEOUT
          → santé SAFE → SAFE_MODE → mission ABORTED
```

Suivre l'événement d'injection, la détection rapportée, la transition de sûreté,
puis les commandes et la vérité terrain. Le maintien de position est appliqué
aux actionneurs simulés par le runner : après l'abandon, le drone reste sur
place jusqu'à la fin de la fenêtre. Vérifier dans la capture que l'altitude et
la position restent constantes après l'injection.

Les délais hôte et MCU diffèrent : voir l'[ordonnancement](../system/scheduling.md).
La consigne de stationnement est de 30 m en SIL et HIL (loopback et FC1 STM32
avec le firmware HIL-Proto v1.1). La faute permanente est
injectée à 40 s, sur une fenêtre de simulation de 100 s. Le maintien est prolongé
à 120 s pour éviter la fin de mission avant la faute. Vérifier dans la capture
l’altitude et la phase de vol à l’injection. Le runner transmet au firmware la cible et la durée de
maintien via le [protocole HIL](../hil/hil_protocol.md).

## Interprétation

Un verdict PASS peut accompagner une mission abandonnée si toutes les assertions
attendues passent. `SAFE_MODE` reste verrouillé pour l'exécution ; le réarmement
entre essais n'est pas une récupération en vol. Le test ne démontre pas de
reprise du contrôle par FC2.

Aucune trace chiffrée mesurée n'est publiée dans ce guide. Conserver le résultat
selon les [conventions de preuve](../validation/evidence.md).
