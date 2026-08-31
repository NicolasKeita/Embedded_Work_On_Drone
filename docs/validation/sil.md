# Rapport de validation SIL

Validation Software-in-the-Loop du systeme de fault injection.

## Synthese

| Scenario | Faulte | Detectee | Latence det. (ms) | Latence rep. (ms) | Etat mission | Verdict |
|---|---|---|---|---|---|---|
| SIL-001 | `NOMINAL` | non | -1000.00 | -1000.00 | `COMPLETE` | PASS |
| SIL-002 | `FC1_FAILURE` | oui | 90.00 | 0.00 | `ABORTED` | PASS |
| SIL-003 | `COMMUNICATION_LOSS` | oui | 0.00 | 0.00 | `ABORTED` | PASS |
| SIL-004 | `SENSOR_FAULT` | oui | 0.00 | 0.00 | `COMPLETE` | PASS |
| SIL-005 | `ACTUATOR_DEGRADATION` | oui | 520.00 | 0.00 | `FAILED` | PASS |

## Resultats detailles

### SIL-001

| Propriete | Valeur |
|---|---|
| Type de faulte | `NOMINAL` |
| Debut (s) | 0.000 |
| Duree (s) | 0.000 |
| Detectee a (s) | -1.000 |
| Latence de detection (ms) | -1000.00 |
| Latence de reponse (ms) | -1000.00 |
| Sante finale | `HEALTHY` |
| Mode de surete final | `NORMAL` |
| Etat de mission final | `COMPLETE` |
| Erreur de position max (m) | 0.00 |
| Erreur d'altitude max (m) | 10.00 |
| Altitude finale (m) | 9.90 |
| Watchdog declenche | non |
| Messages envoyes / recus / perdus | 6001 / 6001 / 0 |
| Latence max (ms) | 4.00 |
| Mission reussie | oui |
| Verdict | **PASS** |

> Verdict : Mission nominale completee sans comportement anormal

### SIL-002

| Propriete | Valeur |
|---|---|
| Type de faulte | `FC1_FAILURE` |
| Debut (s) | 30.000 |
| Duree (s) | 0.000 |
| Detectee a (s) | 30.090 |
| Latence de detection (ms) | 90.00 |
| Latence de reponse (ms) | 0.00 |
| Sante finale | `SAFE` |
| Mode de surete final | `SAFE_MODE` |
| Etat de mission final | `ABORTED` |
| Erreur de position max (m) | 0.00 |
| Erreur d'altitude max (m) | 10.00 |
| Altitude finale (m) | 0.00 |
| Watchdog declenche | oui |
| Messages envoyes / recus / perdus | 3000 / 3000 / 0 |
| Latence max (ms) | 4.00 |
| Mission reussie | non |
| Verdict | **PASS** |

> Verdict : Defaillance detectee et SAFE_MODE engage dans les limites requises

### SIL-003

| Propriete | Valeur |
|---|---|
| Type de faulte | `COMMUNICATION_LOSS` |
| Debut (s) | 30.000 |
| Duree (s) | 0.000 |
| Detectee a (s) | 30.000 |
| Latence de detection (ms) | 0.00 |
| Latence de reponse (ms) | 0.00 |
| Sante finale | `SAFE` |
| Mode de surete final | `SAFE_MODE` |
| Etat de mission final | `ABORTED` |
| Erreur de position max (m) | 0.00 |
| Erreur d'altitude max (m) | 10.00 |
| Altitude finale (m) | 0.00 |
| Watchdog declenche | oui |
| Messages envoyes / recus / perdus | 6001 / 3000 / 3001 |
| Latence max (ms) | 4.00 |
| Mission reussie | non |
| Verdict | **PASS** |

> Verdict : Defaillance detectee et SAFE_MODE engage dans les limites requises

### SIL-004

| Propriete | Valeur |
|---|---|
| Type de faulte | `SENSOR_FAULT` |
| Debut (s) | 20.000 |
| Duree (s) | 10.000 |
| Detectee a (s) | 20.000 |
| Latence de detection (ms) | 0.00 |
| Latence de reponse (ms) | 0.00 |
| Sante finale | `HEALTHY` |
| Mode de surete final | `NORMAL` |
| Etat de mission final | `COMPLETE` |
| Erreur de position max (m) | 0.00 |
| Erreur d'altitude max (m) | 10.00 |
| Altitude finale (m) | 9.90 |
| Watchdog declenche | non |
| Messages envoyes / recus / perdus | 6001 / 6001 / 0 |
| Latence max (ms) | 4.00 |
| Mission reussie | oui |
| Verdict | **PASS** |

> Verdict : Mission nominale completee sans comportement anormal

### SIL-005

| Propriete | Valeur |
|---|---|
| Type de faulte | `ACTUATOR_DEGRADATION` |
| Debut (s) | 15.000 |
| Duree (s) | 0.000 |
| Detectee a (s) | 15.530 |
| Latence de detection (ms) | 520.00 |
| Latence de reponse (ms) | 0.00 |
| Sante finale | `DEGRADED` |
| Mode de surete final | `COMPENSATED` |
| Etat de mission final | `FAILED` |
| Erreur de position max (m) | 0.00 |
| Erreur d'altitude max (m) | 10.00 |
| Altitude finale (m) | 12.33 |
| Watchdog declenche | non |
| Messages envoyes / recus / perdus | 6001 / 6001 / 0 |
| Latence max (ms) | 4.00 |
| Mission reussie | non |
| Verdict | **PASS** |

> Verdict : Defaillance degradee compensee, mission poursuivie

