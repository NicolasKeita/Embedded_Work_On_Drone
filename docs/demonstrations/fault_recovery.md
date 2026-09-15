# Démonstration — dégradation et récupération capteur

Scénario `FAULT_INJECTOR-003` : l'injection temporaire d'altitude hors plage est
définie dans le [catalogue](../validation/scenarios.md). L'analyse est dans la
[FMECA FM-04](../fmeca/failure_modes.md#fm-04--invalid_sensor_data).

## Lancer

```sh
./artifacts/linux/hil_runner --scenario FAULT_INJECTOR-003 --interface loopback
./artifacts/linux/sil_runner --scenario FAULT_INJECTOR-003
```

Pour les cartes préparées, utiliser `--interface auto` avec les vérifications du
[guide HIL](../validation/hil.md).

## Observations attendues

```text
Altitude hors plage → SENSOR_VALIDATION_FAILED → DEGRADED / COMPENSATED
Fin de l'injection → mesure valide → disparition du drapeau → HEALTHY / NORMAL
```

Le moniteur valide la plage et les valeurs numériques ; un biais restant dans
la plage peut lui échapper. La compensation ne majore la poussée que si un
écart actionneur est également détecté.

En SIL/loopback, la cible conserve sa dernière mesure valide. Le firmware FC1
physique convertit directement le paquet reçu vers le contrôleur : ce guide
ne lui attribue pas le même mécanisme de maintien sans vérification. FC2 reçoit
les mesures de supervision et renvoie son diagnostic via FC1.

## Séparer affichage, mesure et trajectoire

Le [viewer](../digital_twin/telemetry.md) peut figer l'altitude affichée pendant
la faute et garder sa valeur brute. Cela ne prouve pas que le contrôleur ou
l'aéronef restent à altitude constante. Comparer les commandes, les mesures
corrompues et la vérité terrain, y compris après disparition de la faute.

Une faute temporaire n'implique pas à elle seule une mission réussie : le verdict
et le retour à l'état nominal doivent être relevés dans la capture. Les fenêtres
SIL et HIL diffèrent ; voir le catalogue plutôt qu'une chronologie recopiée.

Aucune trace chiffrée mesurée n'est publiée dans ce guide. Conserver les sorties
selon les [conventions de preuve](../validation/evidence.md).
