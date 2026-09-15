# Plan de campagne HIL

Ce document décrit les essais à conduire et les critères d'interprétation.
Il ne constitue pas un compte rendu de mesures. Les commandes et métriques
sont dans le [guide HIL](../validation/hil.md), la provenance des résultats
suit les [conventions de preuve](../validation/evidence.md).

## Préparation

1. Identifier la révision logicielle et les images des deux firmwares.
2. Exécuter la régression [SIL](../validation/sil.md) puis la suite HIL loopback.
3. Préparer le [banc](hardware.md), flasher et [vérifier les rôles](flashing.md).
4. Vérifier la cible effective dans l'en-tête du runner.

## Campagne minimale

| Essai | Vérification | Limite de conclusion |
| --- | --- | --- |
| Démarrage FC1/FC2 | Bannières de rôle, échanges inter-FC et diagnostic | Une sonde détectée seule ne suffit pas |
| Échange nominal | Réponse capteur/actionneur, séquence et écho cohérents | Le codec hôte seul ne valide pas le câble |
| Mission nominale | Transitions, trajectoire et verdict conformes au scénario | Comparer des configurations compatibles avec FC1 |
| Timing | Mesurer pas, RTT et dépassements avec politique explicite | Ne pas assimiler RTT et WCET MCU |
| `FAULT_INJECTOR-001` | Suppression heartbeat, diagnostic FC2, réponse et abandon | Ne simule pas une extinction complète de FC1 |
| `FAULT_INJECTOR-003` | Propagation de l'altitude corrompue, détection, compensation puis récupération | Ne garantit pas la stabilité physique pendant la faute |
| Répétition | Réarmement correct entre deux exécutions | Conserver la provenance de chaque run |

Les identités, fenêtres et configurations sont dans le
[catalogue](../validation/scenarios.md). Les seuils d'acceptation doivent venir
des assertions et exigences du scénario ; ne pas inventer des délais ou des
trajectoires pour faire correspondre SIL et matériel.

## Comparaison SIL / loopback / matériel

Comparer les résultats de mission, transitions de santé/sûreté, latences de
détection et de réponse, erreurs de trajectoire et compteurs du transport.
Conserver séparément mesures capteurs, commandes et vérité terrain.
Utiliser des tolérances justifiées plutôt qu'une égalité flottante stricte.

Le FC2 matériel a ses propres délais et publications périodiques. Le PC
observe donc le diagnostic après sa détection locale. Cette distinction doit
apparaître dans toute comparaison temporelle.

## Essais supplémentaires à définir

Coupure physique du lien, arrêt complet de FC1, corruption série, charge CPU,
profilage de pile et blocage de tâche demandent des procédures et critères
propres. Le watchdog local n'étant pas implémenté, un essai de watchdog ne peut
pas être annoncé comme couvert. Ces travaux sont reliés à la
[FMECA](../fmeca/fmeca.md) et à la [feuille de route](../architecture/roadmap.md).

Le verdict de chaque run doit être accompagné de ses traces et des conditions
matérielles ; voir le [rapport de validation](../validation/validation_report.md).
