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
Cette table conserve les identités du banc de référence. Pour utiliser d'autres
cartes, renseigner leurs numéros ST-LINK dans un fichier de configuration matériel
du runner, sans modifier ni recompiler le code.

## Configurer son banc

Les firmwares fournis ciblent deux **Nucleo-L476RG / STM32L476RG**, avec le
câblage décrit ci-dessous. Le fichier [config/hil_hardware.conf](../../config/hil_hardware.conf)
associe les sondes ST-LINK aux rôles FC1 et FC2 :

```ini
fc1_stlink_serial=VOTRE_NUMERO_STLINK_FC1
fc2_stlink_serial=VOTRE_NUMERO_STLINK_FC2
```

Remplacer ces deux valeurs par les numéros des cartes de son propre banc, après
avoir affecté et flashé leurs rôles. Les deux clés sont obligatoires, avec deux
numéros distincts de **24 caractères hexadécimaux** ; les minuscules sont
acceptées. Le fichier accepte les lignes vides et les commentaires `#`, seuls
ou en fin de ligne. Pour conserver la configuration de référence :

```sh
cp config/hil_hardware.conf /chemin/vers/mon_banc.conf
```

Modifier la copie, puis l'utiliser avec le même exécutable Linux compatible :

```sh
./artifacts/linux/hil_runner --scenario NOMINAL-001 --interface auto \
  --hardware-config /chemin/vers/mon_banc.conf
```

Un runner précompilé peut ainsi servir à plusieurs bancs sans recompilation.
Par défaut, les modes `auto` et série chargent `config/hil_hardware.conf` depuis
le répertoire de travail courant. L'option `--hardware-config` permet aussi
d'utiliser un fichier séparé du dépôt. Un fichier requis absent ou invalide
arrête le runner avant la découverte des ports.

L'identité FC1 autorise le canal HIL PC ↔ FC1. L'identité FC2 est conservée dans
la configuration du banc ; elle n'ouvre pas un second canal HIL entre le PC et
FC2. Les [modes loopback et commandes d'information](../validation/hil.md)
peuvent fonctionner sans ce fichier.

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
