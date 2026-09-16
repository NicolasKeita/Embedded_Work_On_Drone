# Digital Twin — viewer

Le modèle aéronef exécuté par les runners constitue le jumeau numérique.
Le viewer React/TypeScript/Three.js en est la présentation : il ne pilote pas
la simulation et n'implémente pas une seconde physique.

Le code du viewer est dans [digital-twin-viewer](../../digital-twin-viewer/).
Les vues aéronef/avionique, graphes, événements, connexion live et chargement de
replay sont présents. Les sources de télémétrie proviennent du SIL ou du HIL ;
la [répartition matérielle](../hil/hil_architecture.md) définit ce qui tourne
sur les cartes.

## Lancer localement

Le [package.json](../../digital-twin-viewer/package.json) exige Node.js ≥22.13.0.
Depuis la racine du dépôt :

```sh
cd digital-twin-viewer
npm ci
npm run dev
```

Ouvrir l'adresse locale affichée par le serveur (`localhost` ou `127.0.0.1`).
Le panneau **Scenario runner** propose les scénarios de `hil_runner --list`.
Le binaire `artifacts/linux/hil_runner` doit être [compilé](../build/build_targets.md).
**RUN SCENARIO** lance le runner depuis la racine du dépôt et active le mode LIVE.

**Auto** conserve le comportement du terminal : FC1 autorisé, avec repli loopback
si indisponible. **Loopback** force l'émulateur hôte. Les logs affichent la cible
réellement sélectionnée et les résultats ; le code de sortie est conservé.
**STOP** termine le processus lancé par le panneau ; ce n'est pas une commande
d'atterrissage. Un seul lancement simultané est accepté, même entre plusieurs
onglets. Fermer l'onglet laisse le scénario tourner ; fermer le serveur local
termine son runner.

L'API fonctionne avec `npm run dev` et accepte uniquement les requêtes locales
de même origine. Elle n'est pas disponible dans le build Cloudflare / `npm start`.
Le lancement en terminal reste possible selon le [guide HIL](../validation/hil.md)
ou le [guide SIL](../validation/sil.md). Le viewer se connecte à
`ws://localhost:8765/twin` ; navigateur et runner doivent accéder au même hôte.
Un seul runner doit publier sur ce port à la fois : arrêter tout runner lancé
séparément avant d'utiliser le panneau.

## Live et replay

Le mode LIVE reçoit les snapshots du runner. Sans données fraîches, le viewer
indique l'attente/perte de télémétrie et ne doit pas inventer la poursuite du vol.
Pour les scénarios SIL partagés, le runner rejoue la télémétrie après le calcul ;
ce flux d'affichage ne transforme pas le calcul SIL en validation temps réel.

LOAD REPLAY accepte un tableau JSON ou des snapshots JSONL. Le bouton REPLAY
charge initialement la démonstration synthétique intégrée ; elle ne constitue
pas une capture HIL. Un fichier enregistré doit respecter le
[contrat de télémétrie](telemetry.md).

## Vérifier le viewer

```sh
npm run build
node --test tests/*.test.mjs
```

Les essais d'affichage ne prouvent pas l'absence d'impact sur les deadlines HIL.
La publication utilise des sockets non bloquantes ; mesurer les conditions
avec/sans client lent ou déconnecté lors de la [campagne HIL](../hil/hil_validation.md).

## Références

- [Télémétrie et affichage des fautes](telemetry.md).
- [Assets 3D et composants adressables](assets.md).
- [Démonstration capteur](../demonstrations/fault_recovery.md).
- [Démonstration d'abandon](../demonstrations/mission_abort.md).
