# Stratégie des assets 3D

The user should not need to manually model assets in Blender.

Two different strategies are recommended depending on the asset.

## Aircraft model — AI-generated GLB

The aircraft is mostly a visual object. An AI-generated 3D asset is appropriate here.

Recommended workflow:

```text
text prompt / reference image
            ↓
AI 3D generator
            ↓
GLB
            ↓
Three.js GLTFLoader / React Three Fiber
```

Les générateurs externes restent un choix de production d’asset, pas une dépendance du banc.

For this project, the generated aircraft does not need to match a real Heliblade vehicle. It should visually communicate the simplified project aircraft and must not imply that it represents X721's proprietary design.

A useful generation prompt would describe something such as:

> Minimal lightweight experimental high-altitude drone, symmetric twin rotating-wing concept, engineering prototype, clean topology, low-poly real-time asset, no weapons, no landing gear emphasis, neutral materials, isolated object.

The model should be kept lightweight enough for browser rendering.

## Flight-controller board — procedural 3D, not a monolithic AI GLB

For the electronic board view, a monolithic AI-generated GLB is **not recommended**.

The viewer needs to independently change the state of individual components. A generated mesh may not provide predictable component names, hierarchy, or separation.

Instead, construct a simplified board procedurally in React Three Fiber from basic meshes.

Conceptually:

```text
Nucleo board
├── PCB
├── MCU
├── communication interface
├── sensor interface
├── actuator interface
├── clock / scheduler indicator
└── power / platform block
```

The representation does not need to reproduce every resistor or trace on the real Nucleo-L476RG.

It is an **architectural health view**, not an electrical CAD model.

Each component should have a stable logical identifier, for example:

```text
fc1.mcu
fc1.transport
fc1.sensor_input
fc1.control
fc1.actuator_output

fc2.mcu
fc2.transport
fc2.supervision
fc2.safety_manager
```

This allows telemetry to update individual component materials directly.

Example state mapping:

```text
HEALTHY     → normal/green indication
DEGRADED    → yellow indication
FAILED/SAFE → red indication when semantically appropriate
UNKNOWN     → neutral/grey indication
```

The exact meaning of red/yellow must follow the project's existing `HealthState`, `SafetyMode`, and fault vocabulary rather than creating a new safety taxonomy solely for the UI.

An AI coding agent can generate this geometry directly from React Three Fiber primitives, so no manual 3D modelling step is required.

---


## État du dépôt

Deux modèles sont conservés dans le dépôt :

- `x721-three-wing-concept.glb` : drone à trois ailes, utilisé dans les scènes de
  vol, d'avionique et dans le studio `/models/x721`.
- `carte_stm32.glb` : carte STM32 utilisée dans les panneaux FC1 et FC2.

La [maquette X721](x721_model.md) dispose d'une source procédurale modifiable.
Son générateur produit uniquement le modèle retenu. Les anciennes variantes,
les anciens drones et les copies redondantes ont été supprimés.

Les modèles utilisés et disponibles sont sous
[public/models](../../digital-twin-viewer/public/models/). Les scènes sont dans
[components](../../digital-twin-viewer/components/), notamment les vues aéronef,
avionique et `stm32-fault-model.tsx`. Le texte ci-dessus conserve les critères
de conception ; il ne constitue pas une liste de fonctionnalités restant toutes
à implémenter ni une certification de fidélité électronique.

La sémantique des couleurs vient du [contrat de télémétrie](telemetry.md).
