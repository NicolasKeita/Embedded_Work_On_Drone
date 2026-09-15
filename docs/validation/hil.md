# Exécuter et interpréter le HIL

Le runner peut utiliser l'émulateur hôte ou le banc physique. Voir
l'[architecture HIL](../hil/hil_architecture.md) pour la répartition des calculs.
Les commandes ci-dessous partent de la racine après [compilation](../build/build_targets.md).

## Choisir la cible

```sh
./artifacts/linux/hil_runner --list
./artifacts/linux/hil_runner --selftest --interface loopback
./artifacts/linux/hil_runner --scenario NOMINAL-001 --interface loopback
```

Pour le banc [préparé](../hil/hardware.md) et [flashé](../hil/flashing.md) :

```sh
./artifacts/linux/hil_runner --scenario NOMINAL-001 --interface auto
```

`auto` est la sélection CLI par défaut : elle recherche l'identité FC1 autorisée.
Une découverte absente, ambiguë ou indisponible provoque un repli en loopback.
Un chemin série explicite est accepté seulement s'il correspond au FC1 autorisé ;
un autre périphérique entraîne aussi un repli. Vérifier **Target selection**,
**FC execution target** et **Firmware verification** dans l'en-tête.
La simple détection USB d'une sonde ST-LINK n'identifie pas le firmware exécuté.

L'option `--interface` accepte le chemin persistant FC1 de la table matérielle.
Le support série hôte est Linux.

## Paramètres du runner

| Option | Fonction |
| --- | --- |
| `--scenario <id>` | Identité du [catalogue](scenarios.md) |
| `--duration <s>` | Durée totale |
| `--telemetry-period <s>` | Période du tableau lisible |
| `--seed <n>` | Graine aléatoire |
| `--noise <stddev>` | Bruit de mesure |
| `--deadline Warn\|Fail\|Abort` | Politique de dépassement |

Les valeurs par défaut sont dans
[HilConfig](../../Src/Embedded/Hil/Config/HilConfig.cppm).
Les périodes de contrôle et délais de supervision sont centralisés dans
l'[ordonnancement](../system/scheduling.md).
`Warn` enregistre les dépassements sans imposer à lui seul un verdict FAIL.
Pour une campagne exigeant l'absence de dépassement :

```sh
./artifacts/linux/hil_runner --scenario NOMINAL-001 --interface auto --deadline Fail
```

La configuration PC ne reconfigure pas automatiquement la cible/gains/pas du
firmware. Lire les [limites actuelles](../hil/hil_architecture.md#limites-actuelles)
avant de comparer les profils étendus au SIL.

## Mesures et verdict

| Champ | Sens |
| --- | --- |
| `steps_executed` | Nombre de pas exécutés |
| `min_step_us`, `mean_step_us`, `max_step_us` | Durées de travail des pas côté hôte |
| `max_round_trip_us` | Pire aller-retour observé |
| `max_lateness_us` | Retard maximal relativement à l'échéance |
| `deadline_misses` | Nombre de pas achevés après leur échéance |
| `messages_sent/received/dropped` | Compteurs d'échanges |
| `sequence_errors`, `stale_packets`, `timeouts` | Anomalies du transport |
| `latency_min/mean/max_us` | Statistiques de latence des échanges |

Sources : [HilTiming](../../Src/Embedded/Hil/Clock/HilTiming.cppm),
[HilTransport](../../Src/Embedded/Hil/Transport/HilTransport.cppm) et
[rapports](../../Src/Embedded/Hil/Runner/Report/).

`durée simulée = pas × dt` ne donne pas la durée murale réellement écoulée.
La variation des temps de pas ne remplace pas une mesure dédiée de jitter.
Le RTT inclut transport et traitement ; il ne mesure pas isolément le WCET MCU.
Même en loopback, les dépassements dépendent de la charge et de l'ordonnanceur
hôte : aucune absence de deadline miss ne se déduit du déterminisme fonctionnel.

## Fautes et preuves

Les injections sont définies dans le [catalogue](scenarios.md). Leur déroulement
est illustré par les guides [abandon](../demonstrations/mission_abort.md) et
[récupération](../demonstrations/fault_recovery.md).

Appliquer les [conventions de preuve](evidence.md) pour enregistrer un résultat.
Le [plan de campagne](../hil/hil_validation.md) donne l'ordre des vérifications ;
la [matrice](test_matrix.md) décrit la couverture.
