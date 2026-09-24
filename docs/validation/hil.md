# Exécuter et interpréter le HIL

Le runner peut utiliser l'émulateur hôte ou le banc physique. Voir
l'[architecture HIL](../hil/hil_architecture.md) pour la répartition des calculs.
Les commandes ci-dessous partent de la racine après [compilation](../build/build_targets.md).
Un exécutable Linux précompilé compatible convient également ; l'identité des
cartes se règle dans un fichier matériel, sans recompiler le runner. Les
paramètres du banc, du modèle physique et des scénarios sont eux aussi lus au
démarrage ; distribuer le dossier `config/` avec les exécutables.

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

`auto` est la sélection CLI par défaut : elle charge
[`config/hil_hardware.conf`](../../config/hil_hardware.conf), relatif au répertoire
de travail courant, puis recherche l'identité FC1 autorisée. Pour un autre banc,
copier ce fichier, y renseigner les deux numéros ST-LINK, puis sélectionner la
copie :

```sh
./artifacts/linux/hil_runner --scenario NOMINAL-001 --interface auto \
  --hardware-config /chemin/vers/mon_banc.conf
```

Le fichier fournit obligatoirement `fc1_stlink_serial` et `fc2_stlink_serial`,
avec deux numéros distincts de 24 caractères hexadécimaux. FC2 reste une identité
de configuration ; le canal HIL PC passe uniquement par FC1.
Un fichier requis absent ou invalide fait échouer la commande **avant** la
découverte matérielle. Le mode série explicite charge également ce fichier.
Le mode `--interface loopback` n'en exige aucun, sauf si `--hardware-config`
est fourni. `--help`, `--list` et `--selftest` n'exigent pas ce fichier.

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
| `--config <path>` | Paramètres du runner, par défaut `config/hil.conf` |
| `--simulation-config <path>` | Modèle physique, par défaut `config/simulation.conf` |
| `--scenarios-dir <path>` | Répertoire des 11 profils partagés, par défaut `config/scenarios` |
| `--config-output <path>` | Snapshot de configuration, par défaut `docs/validation/data/hil_configuration.txt` |
| `--hardware-config <path>` | Fichier des identités ST-LINK FC1 et FC2, par défaut `config/hil_hardware.conf` |
| `--duration <s>` | Durée totale, pouvant tronquer volontairement un essai |
| `--telemetry-period <s>` | Période du tableau lisible |
| `--seed <n>` | Graine aléatoire |
| `--noise <stddev>` | Bruit de mesure |
| `--deadline Warn\|Fail\|Abort` | Politique de dépassement |

Les paramètres modifiables sont documentés dans
[`config/hil.conf`](../../config/hil.conf),
[`config/simulation.conf`](../../config/simulation.conf) et les
[profils de scénario](../../config/scenarios/). Le
[guide de configuration](../../config/README.md) décrit le format `clé = valeur`,
les unités, les contraintes et les commandes de vérification. Chaque fichier
requis est chargé et validé avant la boucle ; les clés inconnues ou dupliquées
et les valeurs invalides font échouer le démarrage.

La priorité est **défauts compilés → fichier du runner → profil du scénario →
options CLI**. Une clé connue absente reprend son défaut compilé. Les chemins
relatifs partent du répertoire courant. Le snapshot conserve les paramètres
résolus et les overrides CLI ; utiliser un chemin différent pour archiver chaque
essai. Les commandes `--help`, `--list` et `--selftest` utilisent le catalogue et
les valeurs de référence intégrés sans charger ces fichiers.

`--duration` autorise volontairement un essai plus court que la fenêtre de vent
ou la date d'injection. Les fenêtres restent à leurs dates configurées ; une
panne prévue après la fin de l'essai n'est donc pas injectée. C'est utile pour
tester le démarrage, mais ce résultat ne valide pas le scénario complet et peut
produire un verdict FAIL. Les fenêtres des profils, avant override CLI, restent
vérifiées par rapport à leur durée de mission.

`tracking_tolerance_m` appartient aux mesures et critères SIL concernés. Le HIL
conserve son verdict fondé sur le contrôleur et la supervision. Pour les pannes,
les clés `hil_fault_start_s` et `hil_fault_duration_s` règlent l'activation ; une
durée nulle signifie une panne permanente. Seul `FAULT_INJECTOR-003` expose aussi
`fault_corrupted_altitude_m` pour la mesure injectée.

Les périodes de contrôle et délais de supervision sont centralisés dans
l'[ordonnancement](../system/scheduling.md).
`Warn` enregistre les dépassements sans imposer à lui seul un verdict FAIL.
Pour une campagne exigeant l'absence de dépassement :

```sh
./artifacts/linux/hil_runner --scenario NOMINAL-001 --interface auto --deadline Fail
```

La configuration PC ne modifie pas les gains compilés ni le pas du firmware.
Le HIL physique exige `dt_s = 0.01` et des paramètres de contrôleur compatibles
avec le firmware ; les gains personnalisés sont réservés au loopback. Les
paramètres de mission pris en charge par le protocole restent transmis.
Lire les [limites actuelles](../hil/hil_architecture.md#limites-actuelles)
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
