# HIL-Proto — contrat PC ↔ FC1

Référence du format binaire actuel. Les déclarations exactes et les vérifications
de taille vivent dans [HilProtocol](../../Src/Embedded/Transport/HilProtocol.cppm).
La liaison FC1 ↔ FC2 possède un [autre protocole](../system/communication.md).

## Encapsulation

Le canal fournit un flux d'octets. Le [banc matériel](hardware.md) définit son
débit ; `LoopbackTransport` permet d'exercer le même codec en mémoire.
Les valeurs multioctets sont little-endian, les flottants `std::float32_t`.

| Offset | Taille | En-tête `HilHeader` |
| --- | --- | --- |
| 0 | 1 | Synchronisation `0x48` (`H`) |
| 1 | 1 | Synchronisation `0x49` (`I`) |
| 2 | 1 | `msg_id` : capteurs `0x01`, actionneurs `0x02` |
| 3 | 1 | `protocol_ver` : `0x10` |
| 4 | 2 | `sequence_num`, compteur 16 bits |
| 6 | 2 | `payload_len`, nombre d'octets utiles |

La trame est `header (8) + payload + CRC (2)`. Le CRC-16/CCITT-FALSE
(polynôme `0x1021`, initialisation `0xFFFF`) couvre en-tête et payload ; ses
2 octets sont ajoutés en little-endian. Le payload maximal du parser est
128 octets. Les identifiants TimeSync, FaultInjectionCmd et AckNack des anciennes
propositions ne sont pas des messages implémentés.

## SensorPacket — 80 octets utiles, 90 octets sur le fil

| Offset payload | Taille | Champ / unité |
| --- | --- | --- |
| 0 | 8 | `sim_timestamp_us`, temps simulé PC en microsecondes |
| 8 | 12 | `position_x_m`, `position_y_m`, `position_z_m` |
| 20 | 12 | `velocity_x_ms`, `velocity_y_ms`, `velocity_z_ms` |
| 32 | 12 | `gyro_p_rad_s`, `gyro_q_rad_s`, `gyro_r_rad_s` |
| 44 | 12 | `accel_x_m_s2`, `accel_y_m_s2`, `accel_z_m_s2` |
| 56 | 12 | `roll_rad`, `pitch_rad`, `yaw_rad` |
| 68 | 4 | `altitude_baro_m` |
| 72 | 4 | `wing_rpm_meas` |
| 76 | 4 | `sensor_valid_flags` |

Les 17 flottants représentent 68 octets ; timestamp et flags portent le total
à 80. Les champs reflètent les conventions du modèle, avec altitude positive
vers le haut ; la présence de champs de lacet/accélération ne prouve pas une
simulation complète 6-DOF ni une fusion GNSS/INS.

Les bits de validité 0..4 correspondent à IMU1, IMU2, baromètre, GPS et tachymètre
([HalTypes](../../Src/Embedded/Hal/HalTypes.cppm)). Le bit 31 est
`kHilCommandSuppressInterFcHeartbeat` : commande de test au firmware FC1 pour
supprimer les heartbeats inter-FC. Les autres bits restent réservés.
Il faut conserver la distinction entre validité annoncée et validation réelle
des valeurs par les consommateurs.

## ActuatorPacket — 44 octets utiles, 54 octets sur le fil

| Offset payload | Taille | Champ / unité |
| --- | --- | --- |
| 0 | 8 | `fc_timestamp_us`, horloge locale FC1 |
| 8 | 8 | `echo_sim_timestamp_us`, écho du timestamp capteur |
| 16 | 4 | `wing_rpm_cmd` |
| 20 | 4 | `left_servo_cmd_rad` |
| 24 | 4 | `right_servo_cmd_rad` |
| 28 | 4 | `aux_actuator_cmd` |
| 32 | 4 | `cpu_usage_pct_x100` |
| 36 | 2 | `stack_watermark_words` |
| 38 | 2 | `deadline_miss_count` |
| 40 | 1 | `fc_mode` |
| 41 | 1 | `fc_health_status` |
| 42 | 1 | `fc_detection_code` |
| 43 | 1 | `reserved` |

Sur le chemin FC1 physique, `fc_mode` encode `sim::control::MissionState`.
`fc_health_status` et `fc_detection_code` relaient les valeurs inter-FC définies
dans la [communication](../system/communication.md). Ce ne sont pas les anciens
codes INIT/MANUAL/AUTO ou OK/WARNING/CRITICAL.

FC1 produit son timestamp depuis l'uptime Zephyr en millisecondes multiplié
par 1000. Les champs CPU/pile/deadline restent à leur valeur par défaut faute
de profilage dédié ; ils ne sont pas des mesures valides du MCU.
Le codec transporte les octets ; leur interprétation dépend du producteur.

## Réception et acceptation

[HilFrameParser](../../Src/Embedded/Transport/Parser/HilProtocolParser.cppm)
cherche les octets de synchronisation, accumule l'en-tête et le payload dans
des buffers fixes puis vérifie le CRC. Le traitement applicatif vérifie le
type et la taille attendus.

Le [runner](../../Src/Embedded/Hil/Transport/HilTransport-Accept.cpp)
contrôle aussi la séquence et l'écho du temps simulé. Il classe les erreurs de
séquence, payload, écho et timeout ; elles appartiennent au domaine HIL de la
[taxonomie](../safety/fault_taxonomy.md).

Le RTT est `instant_RX_PC − instant_TX_PC`, mesuré avec l'horloge monotone PC.
L'écho associe la réponse au bon pas ; on ne soustrait pas l'uptime MCU au temps
PC pour obtenir une latence. La [cadence](../system/scheduling.md) et la
[politique d'échéance](../validation/hil.md) sont documentées séparément.

## Compatibilité et extensions

Les deux extrémités doivent être construites avec le même contrat. La taille
44 octets est inchangée depuis que `fc_detection_code` a pris un octet de
l'ancien champ réservé ; une taille identique ne garantit donc pas la même
sémantique avec un ancien firmware.

Les anciennes exigences d'alerte RTT à 15 ms, de repli FC1 après 50 ms et de
profilage DWT/DMA n'ont pas de chemin implémenté équivalent. Elles ne constituent
pas des garanties du protocole. Les essais à réaliser sont dans le
[plan de validation HIL](hil_validation.md).
