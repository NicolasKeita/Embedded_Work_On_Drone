# Télémétrie du viewer

## Contrat actuel

Le type [TwinSnapshot](../../digital-twin-viewer/lib/twin-data.ts) définit les
champs consommés par le viewer. Les producteurs sont le
[rapport HIL](../../Src/Embedded/Hil/Runner/Report/HilReport-Twin.cpp) et le
[pont SIL](../../Tests/Sil/Twin/). Mettre à jour ces contrats ensemble.

| Groupe | Rôle |
| --- | --- |
| `source`, `time_s` | Origine SIL/HIL et temps du snapshot |
| `aircraft` | Position, altitude, attitude et vitesse présentées |
| `target` | Consigne affichée |
| `actuators` | RPM et angles de servos en degrés |
| `mission`, `health`, `safety_mode` | États observés |
| `active_fault` | Faute déclarée par le producteur |
| `fc1`, `fc2` | Statut et états des composants logiques |
| `hil` | Fréquence et compteur de dépassements |
| `events` | Événements datés et niveau de sévérité |
| `altitude_display_held`, `raw_altitude_m` | Marquage d'affichage figé et valeur brute |

Les anciens exemples de JSON incomplets ne sont pas un format alternatif.
Le fichier de replay emploie le même contrat que le flux live.

## Fautes et provenance

Pour `FAULT_INJECTOR-001`, le composant logique `mcu` de FC1 est marqué en faute.
Pour `FAULT_INJECTOR-003`, le bloc `sensors` est dégradé et le viewer conserve
la dernière altitude affichable, avec un indicateur de valeur figée.
La valeur brute reste distincte de la valeur présentée.

`ComponentState::FAILED` est une catégorie d'affichage TypeScript ; ce n'est pas
un nouvel état `HealthState` du cœur C++. Les couleurs et alertes doivent
respecter la [taxonomie](../safety/fault_taxonomy.md).
Une puce rouge représente une faute logique rapportée, pas un diagnostic de
silicium endommagé. La [démonstration capteur](../demonstrations/fault_recovery.md)
explique pourquoi une altitude figée ne prouve pas un maintien physique.

## Publication

Le [publisher](../../Src/Embedded/Hil/Telemetry/TwinWebSocketPublisher.cppm)
expose un flux local non bloquant avec buffers bornés. La publication graphique
est une observation ; elle ne doit pas devenir la cadence de contrôle.
Les fréquences de contrôle sont dans l'[ordonnancement](../system/scheduling.md).
Les événements importants doivent rester lisibles sans afficher tous les heartbeats.

L'absence d'allocation ou de blocage dans un composant ne prouve pas à elle
seule l'absence de coût de formatage/IO dans toute la chaîne de reporting.
Conserver les mesures de timing quand ce comportement est évalué.
