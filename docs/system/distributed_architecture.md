# Pourquoi deux calculateurs de vol ?

FC1 porte le contrôle et la mission. FC2 porte la supervision et la décision de
sûreté. Cette séparation permet de conserver une fonction de surveillance
indépendante du calculateur primaire sur le banc à deux STM32.

La [vue d'architecture actuelle](../architecture/overview.md) décrit où ces
responsabilités s'exécutent ; les [messages inter-FC](communication.md) définissent
leur communication.

## Isolation et limites

Sur PC, le SIL et le HIL loopback emploient des objets dans un même processus :
un arrêt du processus arrête également la supervision. Sur le banc, les
firmwares s'exécutent sur deux cartes distinctes. Cette séparation matérielle
ne prouve pas une indépendance complète des alimentations, du transport ou
des causes de panne communes.

FC2 supervise mais ne remplace pas le contrôleur primaire. Une reprise de vol
nécessiterait notamment le transfert de l'état du contrôleur et l'arbitrage des
commandes actionneurs. La [FMECA](../fmeca/fmeca.md) décrit les conséquences de
cette limite.

## Alternative hôte étudiée

Deux processus hôtes distincts pourraient isoler les espaces mémoire et rendre
le crash de FC1 observable par FC2. Ils nécessiteraient un transport IPC/réseau,
un lancement coordonné et des essais propres. Les anciens noms `fc_primary`
et `fc_safety` désignaient cette proposition ; ce ne sont pas des exécutables
actuels. Voir la [feuille de route](../architecture/roadmap.md).
