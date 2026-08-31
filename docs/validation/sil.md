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
| Derniere sequence recue | 6001 |
| Timeouts de communication | 0 |
| Latence min / moyenne / max (ms) | 4.00 / 4.00 / 4.00 |
| Mission reussie | oui |
| Verdict | **PASS** |

> Verdict : Mission nominale completee sans comportement anormal


#### Telemetrie mission

| t(s) | x(m) | y(m) | z(m) | vx(m/s) | vy(m/s) | vz(m/s) | pitch(deg) | roll(deg) | rpm |
|---|---|---|---|---|---|---|---|---|---|
| 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 2.50 | 0.00 | 0.00 | 8.63 | 0.00 | 0.00 | 2.53 | 0.00 | 0.00 | 761.68 |
| 5.00 | 0.00 | 0.00 | 11.95 | 0.00 | 0.00 | 0.45 | 0.00 | 0.00 | 794.85 |
| 7.50 | 0.00 | 0.00 | 12.01 | 0.00 | 0.00 | -0.24 | 0.00 | 0.00 | 810.79 |
| 10.00 | 0.00 | 0.00 | 11.24 | 0.00 | 0.00 | -0.32 | 0.00 | 0.00 | 816.39 |
| 12.50 | 0.00 | 0.00 | 10.55 | 0.00 | 0.00 | -0.22 | 0.00 | 0.00 | 817.43 |
| 15.00 | 0.00 | 0.00 | 10.13 | 0.00 | 0.00 | -0.12 | 0.00 | 0.00 | 816.99 |
| 17.50 | 0.00 | 0.00 | 9.93 | 0.00 | 0.00 | -0.05 | 0.00 | 0.00 | 816.36 |
| 20.00 | 0.00 | 0.00 | 9.85 | 0.00 | 0.00 | -0.01 | 0.00 | 0.00 | 815.91 |
| 22.50 | 0.00 | 0.00 | 9.84 | 0.00 | 0.00 | -0.00 | 0.00 | 0.00 | 815.66 |
| 25.00 | 0.00 | 0.00 | 9.84 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.55 |
| 27.50 | 0.00 | 0.00 | 9.85 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.52 |
| 30.00 | 0.00 | 0.00 | 9.86 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.51 |
| 32.50 | 0.00 | 0.00 | 9.87 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.52 |
| 35.00 | 0.00 | 0.00 | 9.87 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.52 |
| 37.50 | 0.00 | 0.00 | 9.88 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.52 |
| 40.00 | 0.00 | 0.00 | 9.88 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.53 |
| 42.50 | 0.00 | 0.00 | 9.88 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.53 |
| 45.00 | 0.00 | 0.00 | 9.89 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.53 |
| 47.50 | 0.00 | 0.00 | 9.89 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.53 |
| 50.00 | 0.00 | 0.00 | 9.89 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.53 |
| 52.50 | 0.00 | 0.00 | 9.89 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.53 |
| 55.00 | 0.00 | 0.00 | 9.90 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.53 |
| 57.50 | 0.00 | 0.00 | 9.90 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.53 |
| 60.00 | 0.00 | 0.00 | 9.90 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.53 |

#### Evenements

| t(s) | Categorie | Evenement |
|---|---|---|
| 0.000 | SIL | SIMULATION_START (SIL run started) |
| 0.000 | FC | FC_STARTUP (FC1 online) |
| 0.770 | MISSION | MISSION_STATE_TRANSITION TAKEOFF -> CLIMB (controller progression) |
| 2.890 | MISSION | MISSION_STATE_TRANSITION CLIMB -> STATION_KEEPING (controller progression) |
| 17.750 | MISSION | MISSION_STATE_TRANSITION STATION_KEEPING -> COMPLETE (station hold completed) |
| 60.010 | FC | FC_SHUTDOWN (simulation end) |
| 60.010 | SIL | SIMULATION_END COMPLETE (mission completed) |

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
| Derniere sequence recue | 3000 |
| Timeouts de communication | 1 |
| Latence min / moyenne / max (ms) | 4.00 / 4.00 / 4.00 |
| Mission reussie | non |
| Verdict | **PASS** |

> Verdict : Defaillance detectee et SAFE_MODE engage dans les limites requises


#### Telemetrie mission

| t(s) | x(m) | y(m) | z(m) | vx(m/s) | vy(m/s) | vz(m/s) | pitch(deg) | roll(deg) | rpm |
|---|---|---|---|---|---|---|---|---|---|
| 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 2.50 | 0.00 | 0.00 | 8.63 | 0.00 | 0.00 | 2.53 | 0.00 | 0.00 | 761.68 |
| 5.00 | 0.00 | 0.00 | 11.95 | 0.00 | 0.00 | 0.45 | 0.00 | 0.00 | 794.85 |
| 7.50 | 0.00 | 0.00 | 12.01 | 0.00 | 0.00 | -0.24 | 0.00 | 0.00 | 810.79 |
| 10.00 | 0.00 | 0.00 | 11.24 | 0.00 | 0.00 | -0.32 | 0.00 | 0.00 | 816.39 |
| 12.50 | 0.00 | 0.00 | 10.55 | 0.00 | 0.00 | -0.22 | 0.00 | 0.00 | 817.43 |
| 15.00 | 0.00 | 0.00 | 10.13 | 0.00 | 0.00 | -0.12 | 0.00 | 0.00 | 816.99 |
| 17.50 | 0.00 | 0.00 | 9.93 | 0.00 | 0.00 | -0.05 | 0.00 | 0.00 | 816.36 |
| 20.00 | 0.00 | 0.00 | 9.85 | 0.00 | 0.00 | -0.01 | 0.00 | 0.00 | 815.91 |
| 22.50 | 0.00 | 0.00 | 9.84 | 0.00 | 0.00 | -0.00 | 0.00 | 0.00 | 815.66 |
| 25.00 | 0.00 | 0.00 | 9.84 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.55 |
| 27.50 | 0.00 | 0.00 | 9.85 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.52 |

#### Evenements

| t(s) | Categorie | Evenement |
|---|---|---|
| 0.000 | SIL | SIMULATION_START (SIL run started) |
| 0.000 | FC | FC_STARTUP (FC1 online) |
| 0.770 | MISSION | MISSION_STATE_TRANSITION TAKEOFF -> CLIMB (controller progression) |
| 2.890 | MISSION | MISSION_STATE_TRANSITION CLIMB -> STATION_KEEPING (controller progression) |
| 17.750 | MISSION | MISSION_STATE_TRANSITION STATION_KEEPING -> COMPLETE (station hold completed) |
| 30.000 | FAULT | FAULT_INJECTED FC1_FAILURE |
| 30.000 | FC | FC_FAILURE FC1_FAILURE |
| 30.090 | SAFETY | WATCHDOG_TIMEOUT FC1_HEARTBEAT_TIMEOUT (supervised link silent) |
| 30.090 | FAULT | FAULT_DETECTED FC1_HEARTBEAT_TIMEOUT |
| 30.090 | FAULT | FAULT_CLASSIFIED FC1_HEARTBEAT_TIMEOUT (fault domain classification) |
| 30.090 | SAFETY | SAFETY_STATE_TRANSITION HEALTHY -> SAFE (fault flags raised) |
| 30.090 | SAFETY | SAFETY_STATE_TRANSITION NORMAL -> SAFE_MODE (FC1_HEARTBEAT_TIMEOUT) |
| 30.090 | SAFETY | SAFETY_RESPONSE ENTER_SAFE_MODE |
| 60.010 | SIL | SIMULATION_END ABORTED (mission not completed) |

#### Telemetrie apres faulte

| t(s) | x(m) | y(m) | z(m) | vx(m/s) | vy(m/s) | vz(m/s) | pitch(deg) | roll(deg) | rpm |
|---|---|---|---|---|---|---|---|---|---|
| 30.00 | 0.00 | 0.00 | 9.86 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.51 |
| 32.50 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 35.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 37.50 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 40.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 42.50 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 45.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 47.50 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 50.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 52.50 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 55.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 57.50 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 60.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |

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
| Derniere sequence recue | 6001 |
| Timeouts de communication | 1 |
| Latence min / moyenne / max (ms) | 4.00 / 4.00 / 4.00 |
| Mission reussie | non |
| Verdict | **PASS** |

> Verdict : Defaillance detectee et SAFE_MODE engage dans les limites requises


#### Telemetrie mission

| t(s) | x(m) | y(m) | z(m) | vx(m/s) | vy(m/s) | vz(m/s) | pitch(deg) | roll(deg) | rpm |
|---|---|---|---|---|---|---|---|---|---|
| 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 2.50 | 0.00 | 0.00 | 8.63 | 0.00 | 0.00 | 2.53 | 0.00 | 0.00 | 761.68 |
| 5.00 | 0.00 | 0.00 | 11.95 | 0.00 | 0.00 | 0.45 | 0.00 | 0.00 | 794.85 |
| 7.50 | 0.00 | 0.00 | 12.01 | 0.00 | 0.00 | -0.24 | 0.00 | 0.00 | 810.79 |
| 10.00 | 0.00 | 0.00 | 11.24 | 0.00 | 0.00 | -0.32 | 0.00 | 0.00 | 816.39 |
| 12.50 | 0.00 | 0.00 | 10.55 | 0.00 | 0.00 | -0.22 | 0.00 | 0.00 | 817.43 |
| 15.00 | 0.00 | 0.00 | 10.13 | 0.00 | 0.00 | -0.12 | 0.00 | 0.00 | 816.99 |
| 17.50 | 0.00 | 0.00 | 9.93 | 0.00 | 0.00 | -0.05 | 0.00 | 0.00 | 816.36 |
| 20.00 | 0.00 | 0.00 | 9.85 | 0.00 | 0.00 | -0.01 | 0.00 | 0.00 | 815.91 |
| 22.50 | 0.00 | 0.00 | 9.84 | 0.00 | 0.00 | -0.00 | 0.00 | 0.00 | 815.66 |
| 25.00 | 0.00 | 0.00 | 9.84 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.55 |
| 27.50 | 0.00 | 0.00 | 9.85 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.52 |

#### Evenements

| t(s) | Categorie | Evenement |
|---|---|---|
| 0.000 | SIL | SIMULATION_START (SIL run started) |
| 0.000 | FC | FC_STARTUP (FC1 online) |
| 0.770 | MISSION | MISSION_STATE_TRANSITION TAKEOFF -> CLIMB (controller progression) |
| 2.890 | MISSION | MISSION_STATE_TRANSITION CLIMB -> STATION_KEEPING (controller progression) |
| 17.750 | MISSION | MISSION_STATE_TRANSITION STATION_KEEPING -> COMPLETE (station hold completed) |
| 30.000 | FAULT | FAULT_INJECTED COMMUNICATION_LOSS |
| 30.000 | SAFETY | WATCHDOG_TIMEOUT COMMUNICATION_LOST (supervised link silent) |
| 30.000 | FAULT | FAULT_DETECTED COMMUNICATION_LOST |
| 30.000 | FAULT | FAULT_CLASSIFIED COMMUNICATION_LOST (fault domain classification) |
| 30.000 | SAFETY | SAFETY_STATE_TRANSITION HEALTHY -> SAFE (fault flags raised) |
| 30.000 | SAFETY | SAFETY_STATE_TRANSITION NORMAL -> SAFE_MODE (COMMUNICATION_LOST) |
| 30.000 | SAFETY | SAFETY_RESPONSE ENTER_SAFE_MODE |
| 60.010 | FC | FC_SHUTDOWN (simulation end) |
| 60.010 | SIL | SIMULATION_END ABORTED (mission not completed) |

#### Telemetrie apres faulte

| t(s) | x(m) | y(m) | z(m) | vx(m/s) | vy(m/s) | vz(m/s) | pitch(deg) | roll(deg) | rpm |
|---|---|---|---|---|---|---|---|---|---|
| 30.00 | 0.00 | 0.00 | 9.86 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.51 |
| 32.50 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 35.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 37.50 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 40.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 42.50 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 45.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 47.50 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 50.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 52.50 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 55.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 57.50 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 60.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |

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
| Derniere sequence recue | 6001 |
| Timeouts de communication | 0 |
| Latence min / moyenne / max (ms) | 4.00 / 4.00 / 4.00 |
| Mission reussie | oui |
| Verdict | **PASS** |

> Verdict : Mission nominale completee sans comportement anormal


#### Telemetrie mission

| t(s) | x(m) | y(m) | z(m) | vx(m/s) | vy(m/s) | vz(m/s) | pitch(deg) | roll(deg) | rpm |
|---|---|---|---|---|---|---|---|---|---|
| 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 2.50 | 0.00 | 0.00 | 8.63 | 0.00 | 0.00 | 2.53 | 0.00 | 0.00 | 761.68 |
| 5.00 | 0.00 | 0.00 | 11.95 | 0.00 | 0.00 | 0.45 | 0.00 | 0.00 | 794.85 |
| 7.50 | 0.00 | 0.00 | 12.01 | 0.00 | 0.00 | -0.24 | 0.00 | 0.00 | 810.79 |
| 10.00 | 0.00 | 0.00 | 11.24 | 0.00 | 0.00 | -0.32 | 0.00 | 0.00 | 816.39 |
| 12.50 | 0.00 | 0.00 | 10.55 | 0.00 | 0.00 | -0.22 | 0.00 | 0.00 | 817.43 |
| 15.00 | 0.00 | 0.00 | 10.13 | 0.00 | 0.00 | -0.12 | 0.00 | 0.00 | 816.99 |
| 17.50 | 0.00 | 0.00 | 9.93 | 0.00 | 0.00 | -0.05 | 0.00 | 0.00 | 816.36 |

#### Evenements

| t(s) | Categorie | Evenement |
|---|---|---|
| 0.000 | SIL | SIMULATION_START (SIL run started) |
| 0.000 | FC | FC_STARTUP (FC1 online) |
| 0.770 | MISSION | MISSION_STATE_TRANSITION TAKEOFF -> CLIMB (controller progression) |
| 2.890 | MISSION | MISSION_STATE_TRANSITION CLIMB -> STATION_KEEPING (controller progression) |
| 17.750 | MISSION | MISSION_STATE_TRANSITION STATION_KEEPING -> COMPLETE (station hold completed) |
| 20.000 | FAULT | FAULT_INJECTED SENSOR_FAULT (ALTITUDE_OUT_OF_RANGE) value=99999.00 |
| 20.000 | FAULT | SENSOR_FAULT value=99999.00 |
| 20.000 | FAULT | FAULT_DETECTED SENSOR_INVALID |
| 20.000 | FAULT | FAULT_CLASSIFIED SENSOR_INVALID (fault domain classification) |
| 20.000 | SAFETY | SAFETY_STATE_TRANSITION HEALTHY -> DEGRADED (fault flags raised) |
| 20.000 | SAFETY | SAFETY_STATE_TRANSITION NORMAL -> COMPENSATED (SENSOR_INVALID) |
| 20.000 | SAFETY | SAFETY_RESPONSE ENTER_COMPENSATED |
| 30.000 | FAULT | FAULT_CLEARED SENSOR_FAULT (activation window closed) value=10.00 |
| 30.000 | RECOVERY | RECOVERY_START SENSOR_INVALID (fault flag cleared) |
| 30.000 | SAFETY | SAFETY_STATE_TRANSITION DEGRADED -> HEALTHY (all flags clear) |
| 30.000 | SAFETY | SAFETY_STATE_TRANSITION COMPENSATED -> NORMAL (health restored) |
| 30.000 | RECOVERY | RECOVERY_END (health restored) |
| 30.000 | RECOVERY | WATCHDOG_RECOVERY (health restored) |
| 60.010 | FC | FC_SHUTDOWN (simulation end) |
| 60.010 | SIL | SIMULATION_END COMPLETE (mission completed) |

#### Telemetrie apres faulte

| t(s) | x(m) | y(m) | z(m) | vx(m/s) | vy(m/s) | vz(m/s) | pitch(deg) | roll(deg) | rpm |
|---|---|---|---|---|---|---|---|---|---|
| 20.00 | 0.00 | 0.00 | 99999.00 | 0.00 | 0.00 | -0.01 | 0.00 | 0.00 | 815.91 |
| 22.50 | 0.00 | 0.00 | 99999.00 | 0.00 | 0.00 | -0.01 | 0.00 | 0.00 | 815.56 |
| 25.00 | 0.00 | 0.00 | 99999.00 | 0.00 | 0.00 | -0.01 | 0.00 | 0.00 | 815.58 |
| 27.50 | 0.00 | 0.00 | 99999.00 | 0.00 | 0.00 | -0.01 | 0.00 | 0.00 | 815.60 |
| 30.00 | 0.00 | 0.00 | 9.75 | 0.00 | 0.00 | -0.00 | 0.00 | 0.00 | 815.62 |
| 32.50 | 0.00 | 0.00 | 9.86 | 0.00 | 0.00 | 0.02 | 0.00 | 0.00 | 815.07 |
| 35.00 | 0.00 | 0.00 | 9.89 | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 815.36 |
| 37.50 | 0.00 | 0.00 | 9.89 | 0.00 | 0.00 | -0.00 | 0.00 | 0.00 | 815.49 |
| 40.00 | 0.00 | 0.00 | 9.89 | 0.00 | 0.00 | -0.00 | 0.00 | 0.00 | 815.53 |
| 42.50 | 0.00 | 0.00 | 9.89 | 0.00 | 0.00 | -0.00 | 0.00 | 0.00 | 815.54 |
| 45.00 | 0.00 | 0.00 | 9.89 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.54 |
| 47.50 | 0.00 | 0.00 | 9.89 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.53 |
| 50.00 | 0.00 | 0.00 | 9.89 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.53 |
| 52.50 | 0.00 | 0.00 | 9.89 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.53 |
| 55.00 | 0.00 | 0.00 | 9.90 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.53 |
| 57.50 | 0.00 | 0.00 | 9.90 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.53 |
| 60.00 | 0.00 | 0.00 | 9.90 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 815.53 |

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
| Derniere sequence recue | 6001 |
| Timeouts de communication | 0 |
| Latence min / moyenne / max (ms) | 4.00 / 4.00 / 4.00 |
| Mission reussie | non |
| Verdict | **PASS** |

> Verdict : Defaillance degradee compensee, mission poursuivie


#### Telemetrie mission

| t(s) | x(m) | y(m) | z(m) | vx(m/s) | vy(m/s) | vz(m/s) | pitch(deg) | roll(deg) | rpm |
|---|---|---|---|---|---|---|---|---|---|
| 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| 2.50 | 0.00 | 0.00 | 8.63 | 0.00 | 0.00 | 2.53 | 0.00 | 0.00 | 761.68 |
| 5.00 | 0.00 | 0.00 | 11.95 | 0.00 | 0.00 | 0.45 | 0.00 | 0.00 | 794.85 |
| 7.50 | 0.00 | 0.00 | 12.01 | 0.00 | 0.00 | -0.24 | 0.00 | 0.00 | 810.79 |
| 10.00 | 0.00 | 0.00 | 11.24 | 0.00 | 0.00 | -0.32 | 0.00 | 0.00 | 816.39 |
| 12.50 | 0.00 | 0.00 | 10.55 | 0.00 | 0.00 | -0.22 | 0.00 | 0.00 | 817.43 |
| 15.00 | 0.00 | 0.00 | 10.13 | 0.00 | 0.00 | -0.12 | 0.00 | 0.00 | 816.99 |

#### Evenements

| t(s) | Categorie | Evenement |
|---|---|---|
| 0.000 | SIL | SIMULATION_START (SIL run started) |
| 0.000 | FC | FC_STARTUP (FC1 online) |
| 0.770 | MISSION | MISSION_STATE_TRANSITION TAKEOFF -> CLIMB (controller progression) |
| 2.890 | MISSION | MISSION_STATE_TRANSITION CLIMB -> STATION_KEEPING (controller progression) |
| 15.010 | FAULT | FAULT_INJECTED ACTUATOR_DEGRADATION value=0.60 |
| 15.010 | FAULT | ACTUATOR_FAULT value=0.60 |
| 15.530 | FAULT | FAULT_DETECTED ACTUATOR_MISMATCH |
| 15.530 | FAULT | FAULT_CLASSIFIED ACTUATOR_MISMATCH (fault domain classification) |
| 15.530 | SAFETY | SAFETY_STATE_TRANSITION HEALTHY -> DEGRADED (fault flags raised) |
| 15.530 | SAFETY | SAFETY_STATE_TRANSITION NORMAL -> COMPENSATED (ACTUATOR_MISMATCH) |
| 15.530 | SAFETY | SAFETY_RESPONSE ENTER_COMPENSATED |
| 60.010 | FC | FC_SHUTDOWN (simulation end) |
| 60.010 | SIL | SIMULATION_END FAILED (mission not completed) |

#### Telemetrie apres faulte

| t(s) | x(m) | y(m) | z(m) | vx(m/s) | vy(m/s) | vz(m/s) | pitch(deg) | roll(deg) | rpm |
|---|---|---|---|---|---|---|---|---|---|
| 15.05 | 0.00 | 0.00 | 10.12 | 0.00 | 0.00 | -0.37 | 0.00 | 0.00 | 492.92 |
| 17.55 | 0.00 | 0.00 | 6.53 | 0.00 | 0.00 | -0.05 | 0.00 | 0.00 | 850.37 |
| 20.05 | 0.00 | 0.00 | 8.05 | 0.00 | 0.00 | 0.94 | 0.00 | 0.00 | 818.81 |
| 22.55 | 0.00 | 0.00 | 10.34 | 0.00 | 0.00 | 0.81 | 0.00 | 0.00 | 810.31 |
| 25.05 | 0.00 | 0.00 | 11.94 | 0.00 | 0.00 | 0.47 | 0.00 | 0.00 | 810.20 |
| 27.55 | 0.00 | 0.00 | 12.77 | 0.00 | 0.00 | 0.21 | 0.00 | 0.00 | 812.12 |
| 30.05 | 0.00 | 0.00 | 13.08 | 0.00 | 0.00 | 0.06 | 0.00 | 0.00 | 813.82 |
| 32.55 | 0.00 | 0.00 | 13.13 | 0.00 | 0.00 | -0.01 | 0.00 | 0.00 | 814.84 |
| 35.05 | 0.00 | 0.00 | 13.07 | 0.00 | 0.00 | -0.03 | 0.00 | 0.00 | 815.34 |
| 37.55 | 0.00 | 0.00 | 12.98 | 0.00 | 0.00 | -0.04 | 0.00 | 0.00 | 815.53 |
| 40.05 | 0.00 | 0.00 | 12.89 | 0.00 | 0.00 | -0.04 | 0.00 | 0.00 | 815.58 |
| 42.55 | 0.00 | 0.00 | 12.81 | 0.00 | 0.00 | -0.03 | 0.00 | 0.00 | 815.57 |
| 45.05 | 0.00 | 0.00 | 12.73 | 0.00 | 0.00 | -0.03 | 0.00 | 0.00 | 815.56 |
| 47.55 | 0.00 | 0.00 | 12.65 | 0.00 | 0.00 | -0.03 | 0.00 | 0.00 | 815.55 |
| 50.05 | 0.00 | 0.00 | 12.58 | 0.00 | 0.00 | -0.03 | 0.00 | 0.00 | 815.54 |
| 52.55 | 0.00 | 0.00 | 12.52 | 0.00 | 0.00 | -0.03 | 0.00 | 0.00 | 815.54 |
| 55.05 | 0.00 | 0.00 | 12.45 | 0.00 | 0.00 | -0.03 | 0.00 | 0.00 | 815.54 |
| 57.55 | 0.00 | 0.00 | 12.39 | 0.00 | 0.00 | -0.03 | 0.00 | 0.00 | 815.54 |

