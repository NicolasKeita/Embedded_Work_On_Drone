# Modèle de dynamique de vol

> **Note:** The initial flight dynamics model is intentionally simplified and is intended for control-system development, not for aerodynamic prediction.

---

## 3.1 Question Fondamentale

Le modèle représente les grandeurs suivantes :

### Actionneurs

- **Wing RPM**
- **Left servo angle**
- **Right servo angle**

### État

- **X**
- **Y**
- **Altitude**
- **Pitch**
- **Roll**

Il faut maintenant définir la chaîne de causalité :

```
              ACTIONNEURS
                   │
                   ▼
             forces / moments
                   │
                   ▼
              MOUVEMENT
                   │
                   ▼
                 ÉTAT
```

---

## 3.2 Wing RPM → Altitude

Dans le modèle simplifié :

* Plus les ailes tournent vite, plus elles génèrent de portance.

```
RPM ↑ ──► Portance ↑ ──► Altitude ↑
RPM ↓ ──► Portance ↓ ──► Altitude ↓
```

### Approximation de Simulation

La relation simplifiée est :

$$\text{Lift} = f(\text{RPM})$$

La portance est approchée par :

$$\text{Lift} \propto \text{RPM}^2$$

*Ce n'est pas notre modèle aérodynamique définitif ; c'est une approximation de simulation.*

---

## 3.3 Servos → Pitch / Roll

Le modèle emploie deux servos :

```
       LEFT WING        RIGHT WING

            \            /
             \          /
              \        /
               \      /
                ●
```

- Si les deux ailes sont commandées symétriquement, on peut produire un mouvement de **pitch** :
  - `Left servo = +10°`
  - `Right servo = +10°`
  - $\longrightarrow$ **PITCH**

- Si elles sont commandées différemment, on peut produire du **roll** :
  - `Left servo = +10°`
  - `Right servo = -10°`
  - $\longrightarrow$ **ROLL**

### Principe de Découplage

```
                     Servos
                       │
              ┌────────┴────────┐
              ▼                 ▼
          différence          moyenne
              │                 │
              ▼                 ▼
             Roll             Pitch
```

*C'est une simplification de notre modèle, pas une affirmation sur la cinématique exacte du Heliblade réel.*

---

## 3.4 Pitch → Déplacement X

Maintenant on introduit une relation extrêmement importante. Si l'aéronef s'incline :

```
          horizontal
────────────────────────

                                          ●
```

Une partie de la force de portance devient horizontale :

```
Pitch ──► force horizontale ──► accélération X ──► vitesse X ──► position X
```

### Logique du Contrôleur

```
Je veux X = 100 m
        │
        ▼
Je suis trop à gauche
        │
        ▼
Je demande du pitch
        │
        ▼
Je me déplace vers X cible
```

---

## 3.5 Roll → Déplacement Y

Même logique :

```
Roll ──► force horizontale Y ──► accélération Y ──► vitesse Y ──► position Y
```

```
             Y
             ↑

             ●
            /
           /   ← roll
          /

             └────────→ X
```

Le système peut utiliser le roll pour corriger une dérive latérale.

---

## 3.6 Notre Première Matrice de Contrôle

On peut maintenant résumer le système :

| Action / Commande | Effet Principal |
| :--- | :--- |
| **Wing RPM ↑** | Altitude ↑ |
| **Wing RPM ↓** | Altitude ↓ |
| **Servo gauche + servo droit ensemble** | Pitch |
| **Différence gauche/droite des servos** | Roll |
| **Pitch** | Mouvement X |
| **Roll** | Mouvement Y |

*Ce tableau est le cœur de notre modèle de contrôle.*

---

## 3.7 Mais il manque une chose : la Dynamique

La réalité n'est pas instantanée. Si on augmente le RPM, l'aéronef ne passe pas de `Altitude 100m` à `101m` instantanément.

Il y a une dynamique temporelle :

```
RPM ↑ ──► Force ↑ ──► Accélération ↑ ──► Vitesse ↑ ──► Altitude ↑
```

Même chose pour $X$ / $Y$ :

```
Pitch ──► accélération X ──► vitesse X ──► position X
```

Le simulateur intègre cette dynamique ; le contrôleur calcule les corrections à partir des mesures.

---

## 3.8 Notre Modèle Minimal

On peut donc représenter notre aéronef comme :

```
             ┌─────────────────────┐
             │      AIRCRAFT       │
             │                     │
 RPM ────────►  Vertical dynamics   │
             │         │            │
             │         ▼            │
             │      Altitude        │
             │                     │
 Servos ────►  Attitude dynamics    │
             │      │       │      │
             │      ▼       ▼      │
             │    Pitch     Roll    │
             │      │       │      │
             │      ▼       ▼      │
             │    X motion  Y motion│
             └─────────────────────┘
```

Puis le système complet en boucle fermée :

```
                   AIRCRAFT
                      │
                      │ sensors
                      ▼
              FLIGHT CONTROLLER
                      │
                      │ commands
                      ▼
                   AIRCRAFT
```

---

## Références

Les paramètres et l'intégration sont implémentés dans [Simulation](../../Src/Simulation/).
Le [modèle aéronef](aircraft.md) décrit les composants, la [mission](mission.md)
les objectifs et les [interfaces](software_interfaces.md) les unités.
