# Banc matériel STM32

Référence de l'inventaire, des rôles et du câblage du banc. Les STM32 sont
physiquement disponibles ; les deux cartes documentées sont des **Nucleo-L476RG**.
Leur présence ne constitue pas à elle seule un résultat de validation.

## Identités et ports

| Rôle | Numéro de série ST-LINK | Port persistant | Débit console |
| --- | --- | --- | --- |
| FC1 | `066FFF525771555067225635` | `/dev/serial/by-id/usb-STMicroelectronics_STM32_STLink_066FFF525771555067225635-if02` | 460800 baud |
| FC2 | `066CFF525771555067193653` | `/dev/serial/by-id/usb-STMicroelectronics_STM32_STLink_066CFF525771555067193653-if02` | 115200 baud |

```sh
ls -l /dev/serial/by-id/usb-STMicroelectronics_STM32_STLink_*-if02
```

Les numéros `/dev/ttyACM0` et `/dev/ttyACM1` peuvent changer après reconnexion.
En cas de remplacement, mettre à jour cette table et l'identité autorisée par
la configuration du runner ([HilConfig](../../Src/Embedded/Hil/Config/HilConfig.cppm)).

## Câblage

Le port USB ST-LINK de FC1 porte le protocole HIL PC ↔ FC1. Celui de FC2 sert
au diagnostic. Les cartes échangent directement par USART3 :

| FC1 | FC2 |
| --- | --- |
| PB10 / TX | PB11 / RX |
| PB11 / RX | PB10 / TX |
| GND | GND |

USART3 fonctionne à 115200 baud. Les configurations sont les overlays
[FC1](../../apps/fc1_stm32/boards/nucleo_l476rg.overlay) et
[FC2](../../apps/fc2_stm32/boards/nucleo_l476rg.overlay) et le
[transport Zephyr](../../Src/Embedded/InterFc/ZephyrUartInterFcTransport.cpp).

## Voyant d’activité LD2

Les deux firmwares pilotent la LED verte utilisateur LD2 (`led0`, PA5) :

| Rythme | Signification |
| --- | --- |
| Lent, 1 clignotement/s | Firmware en fonctionnement, en attente d’activité |
| Rapide, 4 clignotements/s | Activité récente : cycle de contrôle terminé sur FC1, heartbeat FC1 traité par FC2 |
| Trois flashs courts, puis pause, toutes les 2 s | Anomalie détectée, prioritaire sur l’activité |

L’activité reste indiquée pendant 500 ms après le dernier travail observé.
FC2 peut donc clignoter rapidement sans essai PC : il supervise déjà FC1.
FC1 revient au rythme lent à la fin d’un essai si la liaison reste saine.

FC1 signale les états dégradé/sûr ou les détections transmis par FC2, la
suppression de heartbeat et l’absence de statut FC2 pendant 500 ms (délai
initial de 2 s). FC2 utilise ses propres détections et son état de sûreté.
Un état de sûreté maintenu peut conserver les trois flashs après une faute.

La mise à jour s’effectue dans la boucle applicative, sans temporisation
supplémentaire ni allocation. Un blocage de cette boucle fige le voyant dans
son dernier état ; il ne garantit donc pas une extinction. Le voyant ne
constitue pas une preuve de respect des échéances temps réel. Une erreur
d’initialisation GPIO est signalée sur la console sans arrêter le contrôle.

## Périmètre

Le banc exécute les deux firmwares sous Zephyr. Le PC conserve le modèle
aéronef et les actionneurs simulés. La supervision FC2 est renvoyée au PC par
FC1 ; il n'existe pas de reprise complète du pilotage par FC2 ni de commande
d'actionneurs physiques dans cette boucle.

Suite : [flash et vérification des rôles](flashing.md) ·
[architecture HIL](hil_architecture.md) · [campagne de validation](hil_validation.md).
