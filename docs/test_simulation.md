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

## Validation SIL (Software-In-the-Loop)

Un exécutable dédié, `test_sil.exe`, permet de valider la robustesse du système face aux pannes (Software-In-the-Loop).

Executable : `build\Debug\test_sil.exe`

### Utilisation

```text
.\build\Debug\test_sil.exe
```

L'exécutable lance automatiquement une suite de scénarios déterministes injectant des défauts spécifiques pour vérifier la réaction du système (détection, latence, mise en sécurité).

### Scénarios SIL implémentés

Tous les scénarios partagent la même mission initiale : **une montée autonome vers une altitude cible de 10 mètres**, sur une durée totale simulée de 90 secondes.

Les scénarios suivants sont exécutés :

- **SIL-001 : Vol nominal sans faute**
  - **Description** : Exécution de la mission complète sans injection de défaut.
  - **Attentes** : La mission doit se terminer avec le statut `COMPLETE`, le mode de sûreté doit rester `NORMAL`, aucune faute ne doit être détectée, et l'erreur d'altitude doit rester maîtrisée (<= 10.5 m).

- **SIL-002 : Défaillance du contrôleur de vol principal (FC1 failure)**
  - **Description** : Arrêt brutal du contrôleur de vol principal à t = 30.0 s (pendant le maintien d'altitude).
  - **Attentes** : Le défaut doit être détecté via un timeout du heartbeat en moins de 300 ms. Le système doit basculer en `SAFE_MODE` avec une latence de réponse <= 200 ms, et la mission doit être annulée.

- **SIL-003 : Perte totale de communication (Communication loss)**
  - **Description** : Coupure de la liaison de communication à t = 30.0 s.
  - **Attentes** : L'alerte `COMMUNICATION_LOST` doit être levée en moins de 300 ms. Le système doit engager une réaction de sûreté (`SAFE_MODE`) et annuler la mission.

- **SIL-004 : Défaillance d'un capteur (Sensor fault)**
  - **Description** : Corruption des données du capteur d'altitude (valeur aberrante) à t = 20.0 s pendant 10 secondes.
  - **Attentes** : La donnée aberrante doit être invalidée en moins de 500 ms. Le `HealthMonitor` doit passer en état `DEGRADED`, le mode `COMPENSATED` doit être engagé pour maintenir le vol, et la mission ne doit pas être annulée.

- **SIL-005 : Dégradation d'un actionneur (Actuator degradation)**
  - **Description** : Baisse du rendement d'un actionneur à 60% de sa capacité à t = 15.0 s (pendant la phase de montée).
  - **Attentes** : Le désaccord entre la commande et la réponse physique doit être détecté. Le système doit passer en état `DEGRADED`, engager une consigne de compensation, et poursuivre la mission sans l'annuler.

*(Note : La perte de paquets réseau (Packet loss) est gérée au niveau de la couche communication et peut être testée via des scénarios probabilistes similaires).*

### Critères Pass/Fail

Chaque scénario évalue un objet `SimulationResult` contenant les métriques de vol. Les critères de succès incluent :
- La détection correcte du défaut injecté.
- Une latence de détection respectant les contraintes temps réel (ex: <= 300 ms).
- Le basculement dans un mode de sécurité approprié (`SAFE_MODE`, `COMPENSATED`, etc.).
- L'annulation de la mission si nécessaire.
- Le maintien de l'erreur d'altitude dans des limites acceptables.

### Rapports de validation

À la fin de l'exécution, les résultats sont exportés automatiquement dans le dossier `docs/validation/` sous plusieurs formats :

- **`sil.json`** : Fichier JSON contenant l'ensemble des données brutes de la simulation (configuration des scénarios, métriques de vol, temps de détection, latences, verdicts). Ce format est conçu pour être lu par des machines, ce qui le rend idéal pour l'intégration continue (CI) et le parsing automatisé.
- **`sil.csv`** : Fichier CSV (Comma-Separated Values) résumant les résultats sous forme de tableau (Nom du test, Verdict, Temps de détection, Latence, Mode final). Ce format est parfait pour un import rapide dans un tableur (Excel, Calc) ou pour générer des graphiques d'analyse.
- **`sil.md`** : Fichier Markdown généré automatiquement qui présente les résultats de manière lisible pour un humain. Il contient un tableau récapitulatif formaté et les détails textuels de chaque scénario, prêt à être lu directement sur un dépôt Git ou un wiki.

Ces fichiers constituent la preuve de validation automatisée du système.
