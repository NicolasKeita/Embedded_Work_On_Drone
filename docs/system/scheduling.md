# Ordonnancement et horloges

## Exécution actuelle

| Environnement | Déclenchement | Horloge |
| --- | --- | --- |
| SIL | Pas fixe, aussi vite que le CPU le permet | Temps simulé |
| HIL hôte | Échéances absolues, pas de 10 ms par défaut | `std::chrono::steady_clock` pour la cadence |
| FC1 Zephyr | Un calcul par `SensorPacket` reçu et validé | Pas de contrôle fixe ; uptime local pour le lien |
| FC2 Zephyr | Boucle de supervision avec interrogation UART | `k_uptime_get()` |

Les firmwares exécutent leur traitement dans la boucle principale de chaque
application Zephyr, avec `k_sleep(K_MSEC(1))` lorsque leur chemin console ne
fournit pas d'octet. FC1 reçoit les octets HIL par interruption dans un ring
buffer statique. Le découpage en six tâches FreeRTOS à 200 Hz des anciennes
notes n'est pas implémenté.

## Réglages temporels du banc

| Réglage | Valeur actuelle | Source |
| --- | --- | --- |
| Pas du runner HIL | 0.01 s | [HilConfig](../../Src/Embedded/Hil/Config/HilConfig.cppm) |
| Pas du contrôleur FC1 embarqué | 0.01 s | [FC1](../../apps/fc1_stm32/src/main.cpp) |
| Heartbeat FC1 matériel | 100 ms | FC1, `kHeartbeatPeriodMs` |
| Publication périodique FC2 | 100 ms | [FC2](../../apps/fc2_stm32/src/main.cpp), `kStatusPeriodMs` |
| Timeout heartbeat FC2 après réception | 0.30 s | FC2, `kHeartbeatTimeoutSeconds` |
| Attente du premier heartbeat FC2 | 2.0 s | FC2, `kInitialHeartbeatTimeoutSeconds` |
| Timeout heartbeat HIL loopback | 0.10 s | `HilConfig::heartbeat_timeout_s` |

Ces valeurs configurées ne sont pas des latences mesurées. La publication
périodique du diagnostic et la réception côté PC s'ajoutent au délai de détection
embarqué. Une comparaison SIL/HIL doit tenir compte de ces différences.

## Distinguer les mesures

- Le **temps simulé** avance avec le pas et décrit l'état du modèle.
- Le **temps mural monotone** cadence le banc et permet de calculer un aller-retour.
- L'**uptime MCU** horodate localement la cible ; il n'est pas synchronisé sur le PC.
- Une **deadline** borne l'achèvement d'un pas ; le **jitter** décrit la variation
  de l'instant de déclenchement. Un RTT n'est pas un WCET du contrôleur.

Les [métriques HIL](../validation/hil.md) définissent les champs exposés.
Une priorité élevée ou une moyenne inférieure au budget ne garantit pas
l'absence de dépassements sous toute charge.

## Évolution vers plusieurs tâches

Le découpage capteurs/contrôle/actionneurs/mission/communication/santé est une
proposition. Avant de l'introduire, il faudrait définir périodes, budgets,
priorités, tailles de piles et propriétaires des données. Les files bornées
peuvent éviter un état global mutable partagé ; leur politique de saturation
et la fraîcheur des mesures doivent être explicites. Voir la
[feuille de route](../architecture/roadmap.md).
