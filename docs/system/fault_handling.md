# Gestion des fautes

## Références

Les définitions et transitions sont maintenues dans la
[taxonomie de sûreté](../safety/fault_taxonomy.md). La
[FMECA](../fmeca/fmeca.md) analyse les causes, effets, gravités et limites.
La [matrice de validation](../validation/test_matrix.md) relie ces mécanismes
aux tests ; le [catalogue](../validation/scenarios.md) identifie ce qui est lançable.

## Règle d'architecture

L'injecteur perturbe l'environnement ou le chemin de communication.
`HealthMonitor` observe les conséquences et produit un `HealthReport`.
`SafetyManager` consomme ce diagnostic et produit une `SafetyCommand`.
Le moniteur ne doit pas lire l'identité de la faute pour fabriquer la détection.

L'exécution de cette chaîne sur les cartes et l'application de la commande
aux actionneurs simulés sont précisées dans l'[architecture HIL](../hil/hil_architecture.md).

## Lire une démonstration

Un test de faute peut réussir alors que la mission est abandonnée : son verdict
évalue la réponse attendue. Les parcours commentés sont la
[récupération capteur](../demonstrations/fault_recovery.md) et
l'[abandon après perte de heartbeat](../demonstrations/mission_abort.md).

Les anciens schémas `SUSPECTED_FAILURE`, récupération depuis `SAFE_MODE`,
estimateur de secours, parachute et watchdog multi-tâches ne décrivent pas des
fonctions actuelles. Les besoins de reprise et de watchdog figurent dans la
[feuille de route](../architecture/roadmap.md).
