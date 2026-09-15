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

## Périmètre

Le banc exécute les deux firmwares sous Zephyr. Le PC conserve le modèle
aéronef et les actionneurs simulés. La supervision FC2 est renvoyée au PC par
FC1 ; il n'existe pas de reprise complète du pilotage par FC2 ni de commande
d'actionneurs physiques dans cette boucle.

Suite : [flash et vérification des rôles](flashing.md) ·
[architecture HIL](hil_architecture.md) · [campagne de validation](hil_validation.md).
