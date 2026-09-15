# Évolutions proposées

Cette page regroupe les propositions des anciennes notes d'étapes. Elles ne
constituent ni des fonctions implémentées ni des résultats de tests.
L'[architecture actuelle](overview.md) reste la référence du code existant.

| Sujet | Travail restant / critère préalable |
| --- | --- |
| Plusieurs tâches RTOS | Définir budgets, priorités, piles et échanges bornés ; mesurer l'ordonnancement Zephyr avant tout découpage |
| Watchdog local | Réarmer seulement après preuve d'avancement des tâches critiques ; définir la réponse à un blocage |
| Intégrité numérique | Garde générique sur état et commande, injection et réponse explicite : FM-07 de la FMECA |
| Transport inter-FC CAN ou processus hôtes UDP | Adaptateurs, versionnement, tests de perte/duplication/réordonnancement |
| Reprise complète par FC2 | État de contrôle synchronisé, arbitrage actionneurs, continuité des consignes et tests de panne complète |
| Profilage embarqué | Mesurer temps de calcul, charge CPU et marges de pile ; exposer la validité des mesures |
| Configuration des missions sur MCU | Transmettre cible/gains/pas ou documenter un contrat de configuration commun aux deux extrémités |
| Stress du banc | Campagnes de corruption série, retards et charge CPU avec critères définis avant exécution |
| Capteurs/actionneurs réels | Pilotes et bancs dédiés, séparés de la validation du modèle logiciel |
| Statistiques de fautes | Exposer la bibliothèque Monte Carlo de fautes par une interface testée |

Les choix exploratoires FreeRTOS/CMSIS-RTOS, DMA/DWT, cartes F446/H743 et
les cibles de latence des anciennes spécifications ne sont pas des exigences
validées du banc L476RG. Toute adoption doit être justifiée et intégrée dans
les références du sujet concerné.

Références : [FMECA](../fmeca/fmeca.md) · [campagne HIL](../hil/hil_validation.md) ·
[Monte Carlo](../validation/monte_carlo.md).
