# Conception du runner HIL

Le runner possède le modèle aéronef, l'injection, les canaux, les observations et
le verdict. La [répartition hôte/STM32](hil_architecture.md) change avec le canal,
sans remplacer le contrôleur par un algorithme de démonstration.

## Un pas de boucle

1. Réinitialiser l'environnement puis appliquer les injecteurs actifs.
2. Échantillonner les capteurs depuis la vérité terrain privée ; appliquer la corruption.
3. Encoder et transmettre la mesure. Sur matériel, ajouter si nécessaire la commande de suppression du heartbeat inter-FC.
4. Faire répondre la cible loopback, ou attendre la réponse série de FC1.
5. Vérifier la trame, sa séquence et l'écho du temps simulé ; relever les diagnostics.
6. Évaluer la santé sur l'hôte ou décoder celle de FC2, puis appliquer la sûreté aux actionneurs simulés.
7. Avancer la physique, enregistrer les métriques et attendre l'échéance absolue suivante.

Les sources sont dans [Runner](../../Src/Embedded/Hil/Runner/), notamment
[Step](../../Src/Embedded/Hil/Runner/HilRunner-Step.cpp),
[Loop](../../Src/Embedded/Hil/Runner/HilRunner-Loop.cpp) et
[Safety](../../Src/Embedded/Hil/Runner/Safety/).

## Échéances

L'échéance avance d'une période à chaque pas à partir d'une origine monotone.
Une surcharge ne redéfinit pas l'origine de l'horloge. La politique `Warn`
enregistre les dépassements, `Fail` les fait échouer au verdict et `Abort`
interrompt l'exécution. Les valeurs configurées et les métriques sont dans le
[guide HIL](../validation/hil.md).

## Contrats

Le [protocole](hil_protocol.md) définit les octets, le
[guide du code](hil_code_guide.md) les points d'entrée et
l'[ordonnancement](../system/scheduling.md) la séparation des horloges.
