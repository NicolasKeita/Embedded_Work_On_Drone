# Modèle visuel X721

Le modèle retenu est [x721-three-wing-concept.glb](../../digital-twin-viewer/public/models/x721-three-wing-concept.glb).
Il représente trois ailes fines et continues, reliées horizontalement au bloc
moteur rouge central. Les formes ont été adaptées aux images fournies puis aux
corrections de l'utilisateur. Les proportions restent une interprétation visuelle,
sans valeur de CAO constructeur ni de définition aérodynamique.

## Examiner et récupérer le modèle

Depuis `digital-twin-viewer`, lancer `npm run dev`, puis ouvrir
[/models/x721](http://localhost:3000/models/x721). Le studio propose les vues
perspective/dessus/face/profil, le zoom, la rotation de caméra et le maillage.
Son bouton de téléchargement fournit le modèle à trois ailes.

Le GLB est autonome : géométrie, matériaux PBR et métadonnées sont intégrés,
sans texture externe, service distant ni extension de compression obligatoire.
Il s'ouvre également dans Blender ou tout lecteur glTF 2.0 compatible.

## Modifier et régénérer

La source éditable est
[generate-x721-model.mjs](../../digital-twin-viewer/scripts/generate-x721-model.mjs).
Les paramètres de silhouette sont regroupés en tête du fichier. Après modification :

```sh
cd digital-twin-viewer
npm run model:x721
```

La commande produit uniquement le GLB à trois ailes et
[x721-model-info.json](../../digital-twin-viewer/public/models/x721-model-info.json),
qui contient ses bornes, le nombre de triangles et les paramètres utilisés.
Elle vérifie les coordonnées, les indices et l'orientation des normales.
Les anciennes variantes ne sont plus générées.

L'échelle est arbitraire : une aile mesure 2,2 unités et le rayon nominal de
l'ensemble est 3,15 unités. **Ces valeurs ne sont pas des dimensions réelles en
mètres.** Les scènes adaptent l'échelle d'affichage ; une dimension connue serait
nécessaire pour caler une échelle réelle.

## Intégration dans le viewer

Le même GLB est chargé dans la scène de vol, le panneau aéronef de la vue avionique
et le studio. La carte [carte_stm32.glb](../../digital-twin-viewer/public/models/carte_stm32.glb)
est conservée pour les panneaux FC1 et FC2. Ce sont les deux seuls modèles du
dépôt ; les anciens drones, variantes et doublons ont été supprimés.

L'axe vertical est **Y** et les trois ailes sont réparties dans le plan **XZ**.
Le centre du moteur, les attaches intérieures des ailes et les axes des liaisons
partagent la cote `wingHeight` (0,14 unité). Les groupes sont nommés :

```text
X721_Concept
├── rotor_assembly
│   ├── wing_1
│   │   ├── wing_1_shell
│   │   ├── wing_1_leading_edge
│   │   └── wing_1_inner_panel_seam / wing_1_outer_panel_seam
│   ├── wing_2
│   ├── wing_3
│   └── tether_1 / tether_2 / tether_3
└── central_capsule
```

Le groupe `central_capsule` porte le rôle `userData.role = 'central_motor'`.
Les sous-pièces portent le préfixe `wing_N_` pour garder des noms uniques ; leur
rôle commun figure aussi dans `userData.role`.

Les matériaux anthracite et rouge sont préservés. Dans la vue avionique,
`wing_shell` identifie les trois coques auxquelles appliquer l'alerte actionneur.
La normalisation de la scène de vol utilise toujours `WORLD.droneSpan`.
Le changement d'asset ne modifie ni les équations de simulation, ni le nombre
d'actionneurs simulés, ni le contrat de télémétrie.
