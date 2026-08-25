# test_simulation

Programme de validation du simulateur physique (Heliblade-like).

Executable : `build\Debug\test_simulation.exe`

## Utilisation

```text
.\build\Debug\test_simulation.exe [scenario ...]
```

- **Sans argument** : tous les scenarios sont executes.
- **scenario** : une ou plusieurs lettres parmi `a b c d e f g h i j`
  (insensible a la casse). Les scenarios sont executes dans l'ordre
  passe en ligne de commande.
- `-h` / `--help` : affiche l'aide puis quitte.
- Tout autre argument est refuse avec un message d'erreur et l'aide.

Exemples :

```powershell
# Tous les scenarios (a -> j)
.\build\Debug\test_simulation.exe

# Scenario G uniquement (autonomie altitude)
.\build\Debug\test_simulation.exe g

# Plusieurs scenarios a la suite
.\build\Debug\test_simulation.exe a c j
```

## Scenarios disponibles

| Option | Description | Fonction |
|--------|-------------|----------|
| `a` | Repos (RPM = 0, servos = 0) | `scenarios::rest` |
| `b` | Montee (RPM > hover) | `scenarios::climb` |
| `c` | Descente (RPM < hover) | `scenarios::descent` |
| `d` | Deplacement X (hover + pitch > 0) | `scenarios::move_x` |
| `e` | Deplacement Y (hover + roll > 0) | `scenarios::move_y` |
| `f` | Combine (RPM > hover, pitch > 0, roll < 0) | `scenarios::combined` |
| `g` | Autonomie : altitude pure (z -> 100 m) | `flight_scenarios::autonomous_altitude` |
| `h` | Autonomie : axe X en cascade (x 20 -> 0) | `flight_scenarios::autonomous_position_x` |
| `i` | Autonomie : axe Y en cascade (y -15 -> 0) | `flight_scenarios::autonomous_position_y` |
| `j` | Autonomie : mission complete (TAKEOFF a COMPLETE) | `flight_scenarios::autonomous_mission` |

### Scenarios physiques (a - f)

Scenarios deterministes de boucle ouverte : ils verifient la reponse du
modele physique aux commandes moteurs et servos.

- **a - rest** : appareil pose au sol, aucune commande ; il doit rester immobile.
- **b - climb** : RPM superieur au stationnaire theorique ; montee verticale attendue.
- **c - descent** : montee puis reduction des gaz ; retour au sol attendu.
- **d - move_x** : consigne moyenne des servos positive (+10 degres) -> pitch > 0.
- **e - move_y** : servos opposes (+12 / -12 degres) -> differentiel pur, roll > 0 sans pitch.
- **f - combined** : moyenne positive (+5 degres) et differentiel negatif -> pitch > 0 et roll < 0.

### Scenarios autonomes (g - j)

Scenarios de boucle fermee pilotant le controleur de vol.

- **g - autonomous_altitude** : boucle d'altitude autonome, convergence vers z = 100 m avec metriques.
- **h - autonomous_position_x** : cascade position X -> tangage -> servos, retour de x = 20 m vers x = 0.
- **i - autonomous_position_y** : cascade position Y -> roulis -> servos, retour de y = -15 m vers y = 0.
- **j - autonomous_mission** : mission complete, de l'etat TAKEOFF jusqu'a COMPLETE.

## Conditions d'execution

- Boucle mono-thread deterministe a 100 Hz (`dt = 0.01 s`).
- Un seul runner (harness) est partage par tous les scenarios lances :
  le compteur d'echecs est cumule sur l'ensemble de la validation.
- Le RPM de stationnaire theorique est affiche au demarrage,
  calcule depuis les parametres de l'appareil de reference.

## Codes de sortie

| Code | Signification |
|------|---------------|
| `0` | Tous les scenarios lances sont valides (PASS), ou aide affichee via `-h`. |
| `1` | Au moins une verification a echoue (FAIL). |
| `2` | Argument invalide fourni en ligne de commande. |
