# Modèle visuel X721

Une maquette 3D a été reconstruite à partir des trois images fournies. Elle reprend
les ailes indépendantes très allongées, leurs extrémités relevées, les petits
appendices, les matériaux anthracite et la capsule centrale rouge suspendue.
Les sections, dimensions, détails des hélices et trajectoires des câbles sont
interprétés : ce modèle n'est ni une CAO constructeur ni une définition aérodynamique.

## Examiner et récupérer le modèle

Depuis `digital-twin-viewer`, lancer `npm run dev`, puis ouvrir
[/models/x721](http://localhost:3000/models/x721). Le studio propose trois variantes,
les vues perspective/dessus/face/profil, le zoom, la rotation de caméra et le maillage.
Le bouton de téléchargement fournit le GLB de la variante sélectionnée.

| Fichier | Contenu |
| --- | --- |
| [x721-concept.glb](../../digital-twin-viewer/public/models/x721-concept.glb) | Deux ailes indépendantes, câbles et capsule rouge |
| [x721-three-wing-concept.glb](../../digital-twin-viewer/public/models/x721-three-wing-concept.glb) | Variante à trois ailes, visible dans la deuxième référence |
| [x721-wing.glb](../../digital-twin-viewer/public/models/x721-wing.glb) | Aile isolée pour examiner la silhouette et les détails |

Les GLB sont autonomes : géométrie, matériaux PBR et métadonnées sont intégrés,
sans texture externe, service distant ni extension de compression obligatoire.
Ils s'ouvrent également dans Blender ou tout lecteur glTF 2.0 compatible.

Les première et troisième images montrent deux ailes, sans permettre de conclure
qu'une troisième n'est pas hors champ. Le choix de deux variantes conserve cette
ambiguïté. Le [communiqué officiel X721 de juillet 2026](https://www.x721-stratos.com/fr/_files/ugd/0d53d5_1654b2b8206246a9b590c9a1bedfa73b.pdf)
décrit également un démonstrateur à deux ailes ; la présente maquette ne prétend
pas reproduire ce démonstrateur. Les liaisons et la capsule de la maquette restent
une interprétation visuelle des images fournies.

## Modifier et régénérer

La source éditable est
[generate-x721-model.mjs](../../digital-twin-viewer/scripts/generate-x721-model.mjs).
Les paramètres de silhouette sont regroupés en tête du fichier. Après modification :

```sh
cd digital-twin-viewer
npm run model:x721
```

La commande régénère les trois GLB et
[x721-model-info.json](../../digital-twin-viewer/public/models/x721-model-info.json),
qui contient les bornes, le nombre de triangles et les paramètres utilisés. Elle
vérifie les coordonnées, les indices et l'orientation des normales de la voilure.

L'échelle est arbitraire : une aile mesure 2,2 unités et l'ensemble à deux ailes
6,3 unités. **Ces valeurs ne sont pas des dimensions réelles en mètres.** Le
studio ajuste seulement l'échelle d'affichage. Il faut une dimension connue pour
caler une échelle réelle ; des vues orthogonales faciliteraient une révision des formes.

## Intégration ultérieure

Le modèle actif `drone.glb`, le modèle avionique `drone-done.glb` et la physique
du simulateur sont conservés. Ce nouvel asset est préparé séparément.

L'axe vertical est **Y**, l'ensemble à deux ailes s'étend sur **X** et la capsule
se situe sous l'origine. Les groupes sont nommés :

```text
X721_Concept
├── rotor_assembly
│   ├── wing_1
│   │   ├── wing_1_shell
│   │   ├── wing_1_central_fairing / wing_1_upper_fairing
│   │   ├── wing_1_dorsal_fin / wing_1_ventral_fin
│   │   └── wing_1_propeller
│   ├── wing_2 (+ wing_3 selon variante)
│   └── tether_1 / tether_2 (+ tether_3)
└── central_capsule
```

Ces groupes servent au repérage visuel, sans imposer une cinématique physique.
Les noms des sous-pièces portent le préfixe `wing_N_` pour rester uniques après
import. Leur rôle commun figure aussi dans `userData.role` ; l'aile isolée conserve
les noms simples (`wing_shell`, `propeller`, etc.). Le futur chargement devra préserver les matériaux :
`aircraft-scene.tsx` recolore actuellement tous les meshes en cyan et normalise
leur taille à `WORLD.droneSpan`. Il faudra également revoir la rotation globale,
le repère, le point de référence d'altitude et le repérage des défauts avioniques
avant de substituer ce modèle à ceux des scènes existantes.
