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

Ouvrir l'adresse affichée par le serveur de développement. Dans un autre
terminal à la racine, lancer un scénario selon le [guide HIL](../validation/hil.md)
ou le [guide SIL](../validation/sil.md). Le viewer se connecte à
`ws://localhost:8765/twin` ; navigateur et runner doivent accéder au même hôte.
Un seul runner doit publier sur ce port à la fois.

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
