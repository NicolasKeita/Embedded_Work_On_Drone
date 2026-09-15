# Compilation et produits

Cette page centralise les prérequis et commandes de compilation, depuis la racine
du dépôt. Le [flash](../hil/flashing.md) et l'[exécution HIL](../validation/hil.md)
ont leurs propres procédures.

## Chaîne hôte

Le [CMake racine](../../CMakeLists.txt) exige **CMake 4.4+**, C++23 et `import std;`.
La configuration Linux utilise Ninja et GCC 16 avec sa bibliothèque standard
compatible modules. Les [presets](../../CMakePresets.json) Windows ciblent
Visual Studio 18 2026. Une installation GCC ancienne ne suffit pas.

```sh
cmake -S . -B build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
cmake --build build/linux --target SIL_RUNNER HIL_RUNNER SIL_MONTE_CARLO
```

Sous Windows :

```sh
cmake --preset release
cmake --build --preset release
```

## Firmwares Zephyr

Les applications utilisent [ZephyrFirmware.cmake](../../cmake/ZephyrFirmware.cmake)
et ses réglages de compilation croisée. Il faut un environnement Zephyr/west
configuré et les outils de flash. Le bootstrap utilise `gnuarmemb`, avec
`GNUARMEMB_TOOLCHAIN_PATH=/usr` par défaut. Le script
[PrepareArmStdModule.cmake](../../cmake/PrepareArmStdModule.cmake) exige les sources
libstdc++ ARM **15.2.0** dans `/usr/arm-none-eabi/include/c++/15.2.0/bits`,
vérifie leur empreinte et applique le patch du dépôt à une copie dans le build.
Il requiert également Git. Une autre installation doit être adaptée à ce
contrat explicite ; la version GCC hôte et celle de la bibliothèque ARM sont
des prérequis distincts. Les sources et configurations par carte sont dans
[apps/fc1_stm32](../../apps/fc1_stm32/) et [apps/fc2_stm32](../../apps/fc2_stm32/).

```sh
west build -b nucleo_l476rg apps/fc1_stm32 -d build/fc1_stm32 --pristine
west build -b nucleo_l476rg apps/fc2_stm32 -d build/fc2_stm32 --pristine
```

## Produits

Les noms en majuscules sont les **cibles CMake**, les noms en minuscules sont les
**fichiers exécutables**. Les commandes des guides emploient les copies Linux
sous `artifacts/linux/` ; Windows ajoute l'extension `.exe`.

| Cible | Sortie nommée | Fonction |
| --- | --- | --- |
| `SIL_RUNNER` | `artifacts/linux/sil_runner` | Tests SIL |
| `HIL_RUNNER` | `artifacts/linux/hil_runner` | HIL loopback ou série Linux |
| `SIL_MONTE_CARLO` | `artifacts/linux/sil_monte_carlo` | Dispersion physique |
| `fc1_stm32` | `artifacts/stm32/fc1_stm32.{elf,hex,bin}` | Contrôle et mission |
| `fc2_stm32` | `artifacts/stm32/fc2_stm32.{elf,hex,bin}` | Supervision et sûreté |

Les exécutables existent aussi dans le répertoire de build hôte.
`west flash` utilise l'image de son répertoire de build, pas la copie nommée
sous `artifacts/`. Les anciens exécutables `fc1_hil_host` et
`hil_step_smoke_test` ne sont pas des cibles du CMake actuel.

Le [guide du viewer](../digital_twin/README.md) documente sa chaîne JavaScript.
