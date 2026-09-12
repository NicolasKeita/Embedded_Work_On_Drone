# Scenarios de validation SIL et HIL

> **Document de reference central des scenarios lancables.** Decrit chaque scenario
> identifie par son ID standardise (NOMINAL-xxx, FAULT_INJECTOR-xxx), son objectif, sa
> configuration, le comportement attendu et les runners qui peuvent l'executer. Les
> autres documentations qui mentionnent des scenarios pointent vers ce document plutot
> que de dupliquer les informations.
> Related: [test matrix](test_matrix.md) · [SIL](sil.md) · [HIL](hil.md) · [Monte Carlo](monte_carlo.md).

## Sommaire

- [1. Conventions et vocabulaire](#1-conventions-et-vocabulaire)
- [2. Runners et lancement](#2-runners-et-lancement)
- [3. Scenarios nominaux (NOMINAL-001 a NOMINAL-017)](#3-scenarios-nominaux-nominal-001-a-nominal-017)
- [4. Scenarios d'injection de fautes (FAULT_INJECTOR-001 a FAULT_INJECTOR-005)](#4-scenarios-dinjection-de-fautes-fault_injector-001-a-fault_injector-005)
- [5. Synthese de disponibilite par runner](#5-synthese-de-disponibilite-par-runner)
- [6. Limitations et couverture](#6-limitations-et-couverture)

## 1. Conventions et vocabulaire

### 1.1 Identifiants standardises

Chaque scenario porte un ID standardise de la forme `FAMILLE-NNN`. Les noms ne
contiennent **jamais** la cible d'execution (SIL/HIL) : un meme ID designe la meme
mission logique, la cible etant choisie au lancement par le runner.

| Famille | Prefixe | Role |
| :-- | :-- | :-- |
| Nominal | `NOMINAL-` | Missions sans faute injectee (reference, physique, autonome, profils de decollage, stratosphere) |
| Injection de fautes | `FAULT_INJECTOR-` | Missions de maintien en station avec une faute declenchee a un instant donne |

Les familles `MonteCarlo` et `MonteCarloFaultInjection` existent dans le vocabulaire
(`FunctionalFamily`) mais ne designent pas des scenarios lancables individuels ;
elles correspondent a des campagnes statistiques (voir [Monte Carlo](monte_carlo.md)).

### 1.2 Registres de scenarios

Les scenarios sont definis dans trois registres distincts, chacun lie a un runner :

| Registre | Module | Runner | Contenu |
| :-- | :-- | :-- | :-- |
| `SilScenarioEntry` (suite moteur SIL) | `Tests/Sil/SilScenarios*` | `SIL_RUNNER` | 6 scenarios : `NOMINAL-001` + `FAULT_INJECTOR-001..005` |
| `ScenarioEntry` (catalogue physique/autonome) | `Tests/Scenarios/Scenarios*` | `SIL_RUNNER` | 17 scenarios `NOMINAL-001..017` |
| `HilScenarioRecord` (catalogue HIL) | `Src/Embedded/Hil/Config/HilScenarios*` | `HIL_RUNNER` | 11 scenarios : `NOMINAL-001`, `NOMINAL-012..017`, `FAULT_INJECTOR-001..004` |

Le registre fonctionnel cible-agnostique (`FunctionalScenarios`, `Src/SIL/Core/`)
decrit l'identite declarative de la faute (mode, parametres, issue attendue) partagee
par les suites SIL et HIL pour les scenarios `NOMINAL-001` et `FAULT_INJECTOR-001..004`.

### 1.3 Parametres communs (base HIL)

La configuration HIL de base (`HilConfig` par defaut, `hil_base_config()`) partagee
par tous les scenarios HIL :

| Parametre | Valeur par defaut | Source |
| :-- | :-- | :-- |
| Periode de controle (`dt_s`) | 0.01 s (100 Hz) | `HilConfig::dt_s` |
| Duree de mission (`duration_s`) | 30.0 s | `HilConfig::duration_s` |
| Cible (`target`) | z = 10 m | `HilConfig::target` |
| Timeout heartbeat | 0.10 s | `HilConfig::heartbeat_timeout_s` |
| Mismatch actuateur | 60.0 rpm | `HilConfig::actuator_mismatch_rpm` |
| Marge de compensation de poussee | 1.7 | `HilConfig::thrust_compensation_margin` |
| Cadence de rapport | 1.0 s | `HilConfig::report_period_s` |
| Bruit capteur | 0.0 (deterministe) | `HilConfig::sensor_noise_stddev` |
| Politique de deadline | Warn | `HilConfig::deadline_policy` |
| Graine | 42 | `HilConfig::seed` |

Chaque scenario HIL clone cette base puis fixe sa cible de vol et sa duree. Les
parametres sont surchargeables en ligne de commande (voir [section 2](#2-runners-et-lancement)).

### 1.4 Fenetres d'injection SIL vs HIL

Les memes fautes sont injectees a des instants differents selon le runner :

| Runner | Instant d'injection | Duree (capteur) |
| :-- | :-- | :-- |
| SIL (`FAULT_INJECTOR-001..004`) | t = 20.0 s (sauf 004 a t = 15.0 s) | 10.0 s pour le capteur |
| HIL (`FAULT_INJECTOR-001..004`) | t = 5.0 s | 20.0 s pour le capteur |
| SIL (`FAULT_INJECTOR-005`) | t = 2.0 s (pendant la montee) | permanente |

La fenetre HIL plus precoce laisse le temps d'observer la recuperation (ou la
descente controlee) avant la fin de la mission de 30 s.

## 2. Runners et lancement

### 2.1 SIL_RUNNER

Le runner SIL execute a vitesse CPU maximale (pas de cadencement temps reel). Il
combine deux catalogues : la suite moteur (`SilScenarios`) et le catalogue
physique/autonome (`ScenarioCatalog`).

```text
SIL_RUNNER                              # affiche l'aide + le catalogue, n'execute rien
SIL_RUNNER --all                        # balaye tous les scenarios (suite + catalogue)
SIL_RUNNER --scenario NOMINAL-001       # un scenario nominal
SIL_RUNNER --scenario FAULT_INJECTOR-001   # un scenario d'injection de faute
SIL_RUNNER --scenario NOMINAL-011       # un scenario autonome
SIL_RUNNER --scenario NOMINAL-017       # montee stratospherique
SIL_RUNNER --telemetry-period 0.5 -v   # periode de telemetrie + verbose pas-a-pas
```

Options :

| Option | Effet |
| :-- | :-- |
| `--scenario <id>` | Execute un scenario par son ID |
| `--all` | Balaye tous les scenarios deterministes (incompatible avec `--scenario`) |
| `--telemetry-period <s>` | Periode du tableau de telemetrie (defaut 1 s) |
| `-v`, `--verbose` | Journalisation pas-a-pas |
| `-h`, `--help` | Aide |

Code de sortie : 0 = tout passe, 1 = au moins une verification echouee,
2 = erreur de ligne de commande.

### 2.2 HIL_RUNNER

Le runner HIL execute en temps reel (1 s de simulation = 1 s d'horloge murale),
cadence par une echeance absolue sans derive. La cible FC est l'emulateur hote en
processus (sur loopback) ou, a terme, un STM32 physique sur un peripherique serie.

```text
HIL_RUNNER --list                       # liste les scenarios HIL disponibles
HIL_RUNNER --scenario NOMINAL-001       # nominal, 30 s, temps reel
HIL_RUNNER --scenario FAULT_INJECTOR-001   # panne FC1
HIL_RUNNER --scenario NOMINAL-017       # montee stratospherique (~9 h)
HIL_RUNNER --selftest                   # suite de validation HIL deterministe
HIL_RUNNER --scenario NOMINAL-012 --duration 60 --noise 0.05 --deadline Fail
```

Options :

| Option | Effet |
| :-- | :-- |
| `--scenario <id>` | ID du scenario fonctionnel (defaut `NOMINAL-001`) |
| `--interface <channel>` | `auto` (FC1 de confiance, defaut), `loopback` ou un peripherique serie |
| `--duration <s>` | Surcharge la duree de mission |
| `--telemetry-period <s>` | Surcharge la cadence du rapport lisible (defaut 1 s) |
| `--seed <n>` | Graine aleatoire |
| `--noise <stddev>` | Ecart-type du bruit de mesure capteur (m) |
| `--deadline <Warn|Fail|Abort>` | Politique de depassement d'echeance (defaut Warn) |
| `--selftest` | Suite de validation HIL deterministe (runner/protocole/donnees/fautes) |
| `--list` | Liste les scenarios HIL |
| `-h`, `--help` | Aide |

Code de sortie : 0 = verdict PASS, 1 = verdict FAIL (ou panne), 2 = erreur de ligne
de commande.

### 2.3 SIL_MONTE_CARLO (campagne, pas un scenario individuel)

Le runner Monte Carlo execute une campagne de dispersion physique sur un scenario
autonome du `ScenarioCatalog` (defaut `NOMINAL-001` ; aussi `008`, `009`, `010`).
Il n'injecte pas de fautes. Voir [Monte Carlo](monte_carlo.md) pour le detail.

## 3. Scenarios nominaux (NOMINAL-001 a NOMINAL-017)

Les scenarios nominaux valident le comportement sans faute. Ils se repartissent en
quatre groupes : la reference de maintien en station, la physique en boucle ouverte,
les missions autonomes en cascade, et les profils de decollage bas / stratosphere.

### 3.1 NOMINAL-001 — Maintien en station nominal (reference)

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`SilScenarioEntry` + `ScenarioEntry`), HIL (`HilScenarioRecord`) |
| **Description** | Mission de reference sans faute : decollage, montee a 10 m et maintien en station |
| **Cible** | z = 10 m |
| **Duree** | 30 s |
| **Faute** | Aucune (`FailureMode::NONE`) |
| **Verifications** | Mission COMPLETE, etat final COMPLETE, mode de securite NORMAL, aucune faute detectee, erreur d'altitude bornee (<= 10.5 m) |

C'est le scenario de reference partage par les suites SIL moteur, le catalogue
physique/autonome et le catalogue HIL. Dans le `ScenarioCatalog` il porte la
description « Autonomous altitude hold (z: 0 -> 10 m) » et mesure les metriques de
poursuite (depassement, temps de tolerance, erreur residuelle, acceleration max).

### 3.2 Physique en boucle ouverte (NOMINAL-002 a NOMINAL-007)

Ces scenarios valident la reponse du modele physique aux commandes moteur et
servo, sans controleur en boucle fermee. Disponibles uniquement via le
`ScenarioCatalog` (SIL).

#### NOMINAL-002 — Repos au sol

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`ScenarioCatalog`) |
| **Description** | Drone au sol, moteurs coupes, servos centers ; doit rester immobile |
| **Duree** | 3 s |
| **Verifications** | z = 0, vitesses nulles, attitude neutre, RPM effectif nul |

#### NOMINAL-003 — Montee verticale

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`ScenarioCatalog`) |
| **Description** | RPM = 1.1 x hover, servos a 0 ; montee verticale attendue |
| **Duree** | 6 s |
| **Verifications** | z > 1 m, vitesse verticale positive, RPM effectif au-dessus du hover |

#### NOMINAL-004 — Descente

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`ScenarioCatalog`) |
| **Description** | Montee puis reduction des gaz (RPM = 0.6 x hover) ; retour au sol attendu |
| **Duree** | 10 s (phase de descente) |
| **Verifications** | Vitesse verticale negative en fin de phase, altitude sous le pic atteint, retour au sol (z = 0) |

#### NOMINAL-005 — Translation avant (X)

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`ScenarioCatalog`) |
| **Description** | Commande servo moyenne positive (+10 deg) -> tangage > 0 -> deplacement en X |
| **Duree** | 6 s |
| **Verifications** | Tangage positif, vitesse X positive, x > 1 m, aucune derive laterale |

#### NOMINAL-006 — Translation laterale (Y)

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`ScenarioCatalog`) |
| **Description** | Servos opposes (+12 / -12 deg) -> roulis pur -> deplacement en Y sans tangage |
| **Duree** | 6 s |
| **Verifications** | Roulis positif, vitesse Y positive, y > 1 m, aucune derive longitudinale |

#### NOMINAL-007 — Translation combinee

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`ScenarioCatalog`) |
| **Description** | RPM > hover, tangage positif et roulis negatif combines |
| **Duree** | 6 s |
| **Verifications** | Tangage > 0 et roulis < 0, montee (vz > 0), deplacement X positif, deplacement Y negatif |

### 3.3 Missions autonomes en cascade (NOMINAL-008 a NOMINAL-011)

Ces scenarios valident les boucles de controle en cascade (position -> attitude ->
servos) et la machine d'etats de mission complete. Disponibles via le
`ScenarioCatalog` (SIL).

#### NOMINAL-008 — Axe X en cascade

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`ScenarioCatalog`) |
| **Description** | Phase 1 : rendezvous avec (20, 0, 100) ; phase 2 : retour de x vers 0 |
| **Cible** | x = 0 m, z = 100 m |
| **Duree** | 180 s (phase 1) + 120 s (phase 2) |
| **Verifications** | x = 0 atteint (+/- 0.5 m), depassement < 15 %, erreur residuelle < 0.5 m |

#### NOMINAL-009 — Axe Y en cascade

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`ScenarioCatalog`) |
| **Description** | Phase 1 : rendezvous avec (0, -15, 100) ; phase 2 : retour de y vers 0 |
| **Cible** | y = 0 m, z = 100 m |
| **Duree** | 180 s (phase 1) + 120 s (phase 2) |
| **Verifications** | y = 0 atteint (+/- 0.5 m), depassement < 15 %, erreur residuelle < 0.5 m |

#### NOMINAL-010 — Mission complete

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`ScenarioCatalog`) |
| **Description** | Mission complete de (20, -15, 0) vers (0, 0, 100) a travers toutes les phases |
| **Cible** | (0, 0, 100) |
| **Duree** | 240 s (phase 1) + 180 s (phase 2) |
| **Verifications** | Sequence SPIN_UP -> TAKEOFF -> CLIMB -> STATION_KEEPING -> COMPLETE, position horizontale dans la zone (+/- 1 m), altitude maintenue autour de 100 m (+/- 1 m) |

#### NOMINAL-011 — Maintien d'altitude autonome (100 m)

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`ScenarioCatalog`) |
| **Description** | Montee autonome de 0 a 100 m puis maintien |
| **Cible** | z = 100 m |
| **Duree** | 180 s |
| **Verifications** | Altitude dans la tolerance (+/- 2 m), depassement < 10 %, erreur residuelle < 1 m, mission au moins en station keeping |

### 3.4 Profils de decollage bas et stratosphere (NOMINAL-012 a NOMINAL-017)

Cinq profils de decollage bas de 30 s et une montee stratospherique longue. Ces
scenarios existent dans **les deux** catalogues (SIL `ScenarioCatalog` et HIL
`HilScenarioCatalog`) avec des descriptions identiques.

#### NOMINAL-012 — Decollage vertical bas

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`ScenarioCatalog`), HIL (`HilScenarioCatalog`) |
| **Description** | Decollage vertical bas (30 s, z = 5 m) |
| **Cible** | z = 5 m |
| **Duree** | 30 s |
| **Verifications (SIL)** | Airborne a 30 s, cible basse atteinte (+/- 1 m), acceleration douce (< 2 m/s^2) |

#### NOMINAL-013 — Decollage avant bas

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`ScenarioCatalog`), HIL (`HilScenarioCatalog`) |
| **Description** | Decollage avant bas (30 s, x = 4 m, z = 6 m) |
| **Cible** | x = 4 m, z = 6 m |
| **Duree** | 30 s |
| **Verifications (SIL)** | Airborne, cible altitude atteinte (+/- 1 m), cible longitudinale atteinte (+/- 1.5 m) |

#### NOMINAL-014 — Decollage lateral bas

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`ScenarioCatalog`), HIL (`HilScenarioCatalog`) |
| **Description** | Decollage lateral bas (30 s, y = -4 m, z = 7 m) |
| **Cible** | y = -4 m, z = 7 m |
| **Duree** | 30 s |
| **Verifications (SIL)** | Airborne, cible altitude atteinte (+/- 1 m), cible laterale atteinte (+/- 1.5 m) |

#### NOMINAL-015 — Decollage diagonal bas

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`ScenarioCatalog`), HIL (`HilScenarioCatalog`) |
| **Description** | Decollage diagonal bas (30 s, x = 3 m, y = 3 m, z = 8 m) |
| **Cible** | x = 3 m, y = 3 m, z = 8 m |
| **Duree** | 30 s |
| **Verifications (SIL)** | Airborne, cibles altitude/longitudinale/laterale atteintes |

#### NOMINAL-016 — Decollage decale bas

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`ScenarioCatalog`), HIL (`HilScenarioCatalog`) |
| **Description** | Decollage decale bas (30 s, x = -3 m, y = 2 m, z = 9 m) |
| **Cible** | x = -3 m, y = 2 m, z = 9 m |
| **Duree** | 30 s |
| **Verifications (SIL)** | Airborne, cibles altitude/longitudinale/laterale atteintes |

#### NOMINAL-017 — Montee stratospherique

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`ScenarioCatalog`), HIL (`HilScenarioCatalog`) |
| **Description** | Montee stratospherique longue duree (~9 h, z = 20 km) |
| **Cible** | z = 20 000 m |
| **Duree** | 32 430 s (~9 h) |
| **Particularites HIL** | Limite capteur altitude portee a 25 000 m ; cadence de rapport a 300 s |
| **Verifications (SIL)** | Cible stratosphere atteinte (20 km +/- 2 m), duree de montee proche de 9 h, scenario termine en l'air sans atterrissage |

> Ce scenario est long (~9 h en temps reel HIL). En SIL il s'execute a vitesse CPU
> maximale.

## 4. Scenarios d'injection de fautes (FAULT_INJECTOR-001 a FAULT_INJECTOR-005)

Ces scenarios injectent une faute pendant une mission de maintien en station (cible
z = 10 m, duree 30 s) et valident la chaine detection -> diagnostic -> reponse de
securite. Les modes de faute et les parametres sont partages entre SIL et HIL via le
registre fonctionnel (`FunctionalScenarios`) ; seuls les instants d'injection
different (voir [section 1.4](#14-fenetres-dinjection-sil-vs-hil)).

### 4.1 FAULT_INJECTOR-001 — Indisponibilite de FC1

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`SilScenarioEntry`), HIL (`HilScenarioRecord`) |
| **Mode de faute** | `FC1_UNAVAILABLE` |
| **Cible de faute** | `ProcessorFc1` (signal heartbeat) |
| **Profil** | Permanent |
| **Injection** | SIL : t = 20.0 s ; HIL : t = 5.0 s |
| **Detection attendue** | `FC1_HEARTBEAT_TIMEOUT` <= 300 ms |
| **Reponse de securite** | `SAFE_MODE` (irreversible), latence de reponse <= 200 ms |
| **Issue mission** | ABORTED |
| **Verifications** | Faute detectee, latence de detection <= 300 ms, SAFE_MODE engage, mission ABORTED, verdict PASS avec mission non reussie |

La FC1 (controleur de vol primaire) s'arrete : heartbeat et emission de commande
cessent. Le lien lui-meme reste debout, donc c'est un evenement de supervision par
heartbeat, pas une perte de communication. FC2 ne prend pas le relais (pas de hot
failover) ; la reaction est un abort + descente controlee.

### 4.2 FAULT_INJECTOR-002 — Perte de communication FC1-FC2

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`SilScenarioEntry`), HIL (`HilScenarioRecord`) |
| **Mode de faute** | `FC_COMMUNICATION_LOSS` |
| **Cible de faute** | `LinkFc1Fc2` (signal link_state) |
| **Profil** | Permanent |
| **Injection** | SIL : t = 20.0 s ; HIL : t = 5.0 s |
| **Detection attendue** | `COMMUNICATION_TIMEOUT` <= 300 ms |
| **Reponse de securite** | `SAFE_MODE` (irreversible) |
| **Issue mission** | ABORTED (regle du pas 10) |
| **Verifications** | Faute detectee, premier evenement `COMMUNICATION_TIMEOUT`, latence <= 300 ms, reaction de securite engagee, mission ABORTED |

Le lien radio entre FC1 et FC2 est coupe. La perte doit etre detectee et la mission
abortee.

### 4.3 FAULT_INJECTOR-003 — Defaillance capteur d'altitude

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`SilScenarioEntry`), HIL (`HilScenarioRecord`) |
| **Mode de faute** | `INVALID_SENSOR_DATA` |
| **Cible de faute** | `SensorBarometer` (canal altitude) |
| **Profil** | Temporaire |
| **Parametres** | `corruption = AltitudeOutOfRange`, `corrupted_altitude_m = 99999.0` |
| **Injection** | SIL : t = 20.0 s, duree 10.0 s ; HIL : t = 5.0 s, duree 20.0 s |
| **Detection attendue** | `SENSOR_VALIDATION_FAILED` <= 500 ms |
| **Reponse de securite** | `DEGRADED` -> `COMPENSATED` (maintien de la derniere mesure valide) |
| **Issue mission** | Continue (non abortie) |
| **Verifications** | Faute detectee, premier evenement `SENSOR_VALIDATION_FAILED`, latence <= 500 ms, HealthMonitor en DEGRADED, COMPENSATED engage, mission non abortie |

Le capteur d'altitude rapporte une valeur hors plage (99999 m). La faute est
temporaire et **recuperable** : a la fin de la fenetre, la validation repasse, le
drapeau se leve, l'etat de sante revient a HEALTHY et le mode de securite a NORMAL.
La fenetre HIL (t = 5 -> 25 s) laisse observer la recuperation complete avant la fin
des 30 s ; c'est le scenario utilise par la
[demonstration de recuperation](../demonstrations/fault_recovery.md).

### 4.4 FAULT_INJECTOR-004 — Degradation d'actuateur

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`SilScenarioEntry`), HIL (`HilScenarioRecord`) |
| **Mode de faute** | `ACTUATOR_DEGRADED` |
| **Cible de faute** | `ActuatorMainRotor` (signal wing_rpm) |
| **Profil** | Permanent |
| **Parametres** | `efficiency = 0.6` (perte de 40 %) |
| **Injection** | SIL : t = 15.0 s ; HIL : t = 5.0 s |
| **Detection attendue** | `ACTUATOR_MISMATCH` (maintenu 0.5 s) |
| **Reponse de securite** | `DEGRADED` -> `COMPENSATED` (marge de poussee 1.7x) |
| **Issue mission** | Continue (non abortie) |
| **Verifications** | Faute detectee, premier evenement `ACTUATOR_MISMATCH`, HealthMonitor en DEGRADED, compensation engagee, mission non abortie |

Le rotor principal perd 40 % de sa puissance. La non-concordance entre la commande
et la reponse physique est detectee. Le systeme entre en DEGRADED avec compensation
de poussee et continue la mission.

### 4.5 FAULT_INJECTOR-005 — Panne FC1 pendant la montee (SIL uniquement)

| Champ | Valeur |
| :-- | :-- |
| **Runner** | SIL (`SilScenarioEntry`) uniquement |
| **Mode de faute** | `FC1_UNAVAILABLE` |
| **Cible de faute** | `ProcessorFc1` (signal heartbeat) |
| **Profil** | Permanent |
| **Injection** | t = 2.0 s (pendant la transition de montee) |
| **Detection attendue** | `FC1_HEARTBEAT_TIMEOUT` <= 300 ms |
| **Reponse de securite** | `SAFE_MODE` (irreversible) |
| **Issue mission** | ABORTED |
| **Verifications** | Faute detectee pendant la transition de montee, latence <= 300 ms, SAFE_MODE engage, mission ABORTED, faute injectee pendant la phase de montee (<= 5.0 s), verdict PASS |

Ce scenario exerce la chaine de securite a travers un changement de mode (pendant la
montee) plutot qu'en station keeping stable. Il n'existe pas en HIL (le catalogue HIL
s'arrete a `FAULT_INJECTOR-004`).

## 5. Synthese de disponibilite par runner

Le tableau ci-dessous indique quels runners peuvent executer chaque scenario. Les
scenarios nominaux `NOMINAL-002..011` sont propres au catalogue physique/autonome
(SIL uniquement) ; les profils de decollage bas et la stratosphere sont partages.

| Scenario | SIL (`SilScenarioEntry`) | SIL (`ScenarioCatalog`) | HIL (`HilScenarioCatalog`) |
| :-- | :--: | :--: | :--: |
| `NOMINAL-001` | Oui | Oui | Oui |
| `NOMINAL-002` | — | Oui | — |
| `NOMINAL-003` | — | Oui | — |
| `NOMINAL-004` | — | Oui | — |
| `NOMINAL-005` | — | Oui | — |
| `NOMINAL-006` | — | Oui | — |
| `NOMINAL-007` | — | Oui | — |
| `NOMINAL-008` | — | Oui | — |
| `NOMINAL-009` | — | Oui | — |
| `NOMINAL-010` | — | Oui | — |
| `NOMINAL-011` | — | Oui | — |
| `NOMINAL-012` | — | Oui | Oui |
| `NOMINAL-013` | — | Oui | Oui |
| `NOMINAL-014` | — | Oui | Oui |
| `NOMINAL-015` | — | Oui | Oui |
| `NOMINAL-016` | — | Oui | Oui |
| `NOMINAL-017` | — | Oui | Oui |
| `FAULT_INJECTOR-001` | Oui | — | Oui |
| `FAULT_INJECTOR-002` | Oui | — | Oui |
| `FAULT_INJECTOR-003` | Oui | — | Oui |
| `FAULT_INJECTOR-004` | Oui | — | Oui |
| `FAULT_INJECTOR-005` | Oui | — | — |

> En SIL, `--all` execute d'abord la suite moteur (6 scenarios), puis le catalogue
> physique/autonome (17 scenarios), puis les suites d'observabilite/telemetrie et
> genere les artefacts de rapport. En HIL, un seul scenario est execute par
> invocation ; `--selftest` lance la suite de validation deterministe.

## 6. Limitations et couverture

### 6.1 Fautes sans scenario nomme

| Mode de faute | Statut | Note |
| :-- | :-- | :-- |
| `COMMUNICATION_DEGRADED` | Limite | Couche de perte de paquets dans le `CommsBus` ; pas de scenario deterministe nomme |
| `CONTROL_DEADLINE_MISSED` | Non implante | Pas de chemin d'injection (FM-06) |
| `INVALID_NUMERICAL_STATE` | Non implante | Pas de chemin d'injection (FM-07) |

### 6.2 Canaux injectables

Seuls le canal capteur **altitude/barometre** et l'actuateur **rotor principal** sont
injectables aujourd'hui. Les cibles `SensorImu`, `SensorGnss`, `SensorRpmFeedback`,
`ActuatorLeftServo` et `ActuatorRightServo` sont declarees dans le vocabulaire des
cibles de faute (`FaultTarget`) mais n'ont pas de chemin d'injection. Voir la
[taxonomie de fautes](../safety/fault_taxonomy.md) et la [FMECA](../fmeca/fmeca.md).

### 6.3 HIL : pas de STM32 physique

Le runner HIL actuel cible l'emulateur FC hote en processus sur un canal loopback.
Il n'y a pas de STM32, pas de RTOS, pas de peripheriques reels ni de medium serie
physique. Le protocole wire (HIL-Proto, CRC16) et les abstractions HAL sont
implementes et prets pour le STM32 ; seul le transport et le processus cible
restent a permuter. Voir [HIL](hil.md).

### 6.4 Monte Carlo

Le runner Monte Carlo (`SIL_MONTE_CARLO`) execute une campagne de dispersion physique
sur un scenario autonome (defaut `NOMINAL-001` ; aussi `008`, `009`, `010`). Il
n'injecte **pas** de fautes. Une campagne statistique sur fenetres de fautes existe
en code bibliotheque mais n'est pas cablee a un CLI. Voir [Monte Carlo](monte_carlo.md).
