# Configuration des runners hôtes

Ces fichiers sont lus **une seule fois au démarrage**, sur le PC, avant les
boucles SIL/HIL ou la campagne Monte Carlo. Les valeurs validées et le registre
partagé des scénarios restent fixes pendant l'exécution. Les firmwares STM32 ne
lisent aucun de ces fichiers ; modifier un `.conf` ne nécessite pas de recompiler
les runners.

## Fichiers

| Chemin par défaut | Contenu |
| --- | --- |
| `config/sil.conf` | Pas de simulation, graine du bus, latence, supervision, contrôleur simulé, télémétrie et visualiseur |
| `config/hil.conf` | Cadence, transport, bruit des capteurs, supervision, rapports et contrôleur loopback |
| `config/simulation.conf` | Modèle de l'aéronef et distributions de dispersion Monte Carlo |
| `config/monte_carlo.conf` | Graine de campagne, nombre d'essais et scénario |
| `config/scenarios/<ID>.conf` | Cible, durée, tolérance, vent et fenêtres d'injection propres à SIL/HIL |
| `config/hil_hardware.conf` | Identifiants des sondes ST-LINK pour sélectionner les cartes |

Le répertoire des scénarios doit contenir les **11 profils**, même pour exécuter
un seul scénario : `NOMINAL-001`, `NOMINAL-012` à `NOMINAL-017`,
`FAULT_INJECTOR-001`, `FAULT_INJECTOR-003`, `WIND-001` et `WIND-002`.
Les profils `WIND-001` et `WIND-002` sont exécutables en SIL, en HIL et dans
les campagnes Monte Carlo. Les composantes, la fenêtre et la période des rafales
proviennent du même profil.

`tracking_tolerance_m` règle les mesures et critères de suivi des scénarios SIL
qui l'utilisent. Le verdict HIL conserve ses critères de contrôleur et de
supervision ; cette clé ne modifie pas ses seuils d'acceptation. Les overrides
de capteur s'appliquent aux exécutions disposant d'une chaîne de validation des
capteurs ; les missions physiques SIL directes utilisent l'état de l'aéronef.

Les fenêtres `sil_fault_start_s`/`sil_fault_duration_s` et leurs équivalents
`hil_*` sont réservées aux deux profils de panne. Une durée nulle signifie une
panne permanente. `fault_corrupted_altitude_m` existe uniquement dans
`FAULT_INJECTOR-003` ; la panne FC1 n'a pas de paramètre numérique supplémentaire.

Les constantes mathématiques, identifiants et tailles du protocole, états des
machines ainsi que les constantes des tests de référence restent dans le code.
Les fichiers externalisent les paramètres d'exécution et les profils partagés ;
ils ne remplacent pas les critères indépendants des tests d'observabilité.

## Chemins et options

Les chemins relatifs sont résolus depuis le **répertoire de travail courant**.
Lancer les commandes depuis la racine du dépôt, ou fournir les chemins souhaités.

| Option | `sil_runner` | `hil_runner` | `sil_monte_carlo` |
| --- | --- | --- | --- |
| `--config <chemin>` | `config/sil.conf` | `config/hil.conf` | `config/monte_carlo.conf` |
| `--sil-config <chemin>` | — | — | `config/sil.conf` |
| `--simulation-config <chemin>` | `config/simulation.conf` | `config/simulation.conf` | `config/simulation.conf` |
| `--scenarios-dir <chemin>` | `config/scenarios` | `config/scenarios` | `config/scenarios` |
| `--config-output <chemin>` | `docs/validation/data/sil_configuration.txt` | `docs/validation/data/hil_configuration.txt` | `docs/validation/data/monte_carlo_configuration.txt` |

HIL conserve séparément `--hardware-config <chemin>`, dont le défaut est
`config/hil_hardware.conf`. En `--interface loopback`, ce fichier matériel n'est
pas nécessaire, sauf si son chemin est explicitement fourni.

Les exécutables distribués doivent être accompagnés du dossier `config/`, ou
être lancés avec des chemins vers une configuration complète.

## Format, validation et priorité

Le format est `clé = valeur`, avec des commentaires commençant par `#`, des
booléens `true`/`false` et un point comme séparateur décimal. Les noms des clés
précisent les unités. Par exemple, dans une copie de `sil.conf` :

```ini
# Simulation à 100 Hz ; aucune attente de replay du visualiseur.
dt_s = 0.01
report_period_s = 0.5
viewer_enabled = false
```

Une clé connue omise reprend sa valeur par défaut compilée, même après un
chargement précédent dans le même processus. Un fichier requis manquant,
une clé inconnue ou dupliquée, une valeur mal formée, non finie, hors limites ou
une combinaison incohérente provoque une erreur avant le lancement de la boucle.
Les valeurs par défaut appliquées aux clés omises sont incluses dans le snapshot.

La priorité est : **valeurs par défaut → fichier du runner → profil du scénario
→ options CLI explicites**. Les champs de profil documentés comme des overrides
conservent le défaut du runner lorsque leur valeur vaut zéro. Par exemple,
`--telemetry-period` remplace la période du profil ; en SIL, `--verbose` demande
une observation à chaque pas. En Monte Carlo, `--seed`, `--runs` et `--scenario`
remplacent les valeurs de la configuration de campagne.

En HIL, `--duration` peut volontairement raccourcir un essai avant une fenêtre de
vent ou une injection prévue. La fenêtre reste inchangée : une panne future
n'est pas déclenchée si l'essai se termine avant son activation. Ce raccourci
sert aux essais de démarrage ; il ne prouve pas le scénario complet et ne
garantit pas un verdict PASS. Sans cette option, les fenêtres définies dans les
fichiers doivent tenir dans la durée du profil.

Le snapshot écrit avant l'exécution rassemble les fichiers résolus et les
overrides effectifs, pour conserver la provenance des résultats. Le chemin par
défaut est réutilisé au prochain lancement : choisir un `--config-output`
distinct pour archiver plusieurs essais. Les snapshots sont des comptes rendus,
pas un fichier unique à passer à `--config`.

## Frontière avec les firmwares

Les gains de `sil.conf` règlent le contrôleur émulé. Ceux de `hil.conf` peuvent
être personnalisés en loopback. En HIL physique, le runner vérifie la cadence
**100 Hz (`dt_s = 0.01`)** et la compatibilité des paramètres du contrôleur avec
les valeurs attendues du firmware ; le fichier ne reprogramme pas FC1. Les
paramètres de mission pris en charge par le protocole conservent leur rôle.
La référence de régime stationnaire du contrôleur physique reste celle compilée
dans FC1, même si la masse du modèle simulé change. En loopback et en SIL, cette
référence est calculée à partir du modèle configuré.

`transport_timeout_us` borne l'attente de réponse HIL après l'envoi du capteur,
sans dépasser l'échéance du pas de contrôle.

Les numéros de série identifient les sondes, pas le modèle du microcontrôleur.
Le banc actuel attend des cartes **Nucleo-L476RG / STM32L476RG** compatibles avec
les firmwares et le câblage documentés dans le [guide matériel](../docs/hil/hardware.md).

## Vérification

Après compilation, exécuter les tests HIL internes puis les contrôles CLI de
chargement, de priorité et de rejet des fichiers invalides :

```sh
./artifacts/linux/hil_runner --selftest --interface loopback
python3 Tests/Config/check_runtime_cli.py --build-dir <build>
```

Remplacer `<build>` par le répertoire de compilation contenant les runners.
Ces vérifications utilisent le PC et des copies temporaires des configurations ;
elles ne nécessitent aucune carte STM32.
