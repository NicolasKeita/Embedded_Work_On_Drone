# Spec Technique : Validation Statistique & Campagnes Monte Carlo (SIL)

## 1. Contexte & Objectif d'Ingénierie

L'intégration d'un banc d'essai **Software-in-the-Loop (SIL)** permet d'exécuter le code de contrôle de vol réel dans un environnement simulé déterministe. Toutefois, valider un système de contrôle sur un scénario nominal unique ne garantit en rien sa robustesse opérationnelle en conditions réelles.

L'**Étape 12** introduit la **validation statistique par simulation Monte Carlo**. L'objectif principal n'est pas simplement d'exécuter la simulation $N$ fois, mais de répondre à une question centrale d'ingénierie système :

> **Dans quelle mesure notre système reste-t-il capable d'accomplir sa mission de vol en présence de variabilités environnementales, d'incertitudes capteurs/actionneurs et d'incidents réseau ou matériels ?**

---

## 2. Architecture Globale du Système Monte Carlo

Le sous-système Monte Carlo s'articule autour de quatre composants modulaires à responsabilité unique :

```
                     ┌──────────────────────────┐
                     │    MonteCarloRunner      │
                     └────────────┬─────────────┘
                                  │
         ┌────────────────────────┼────────────────────────┐
         │                        │                        │
         ▼                        ▼                        ▼
┌──────────────────┐    ┌──────────────────┐    ┌──────────────────┐
│ScenarioGenerator │    │    SILRunner     │    │ ResultCollector  │
└────────┬─────────┘    └────────┬─────────┘    └────────┬─────────┘
         │                        │                        │
         │ Scenario               │ SimulationResult       │
         └───────────────────────►┴───────────────────────►│
                                                           │
                                                           ▼
                                                    ┌──────────────┐
                                                    │ CSV / JSON   │
                                                    └──────────────┘
```

### Description des Composants

1. **`ScenarioGenerator`** : Génère des structures `Scenario` reproductibles à partir d'un tirage pseudo-aléatoire basé sur un `seed` global.
2. **`SILRunner`** : Instancie et pilote l'exécution du simulateur SIL pour un scénario donné. Il orchestre les boucles de contrôle temps réel et les modèles physiques.
3. **`ResultCollector`** : Enregistre conjointement les paramètres d'entrée du scénario et les métriques de sortie de la simulation. Il exporte les données pour analyse post-hoc.
4. **`MonteCarloRunner`** : Orchestre la boucle d'exécution, gère le découpage des campagnes et garantit l'isolation entre chaque run.

---

## 3. Paramétrage des Scénarios Monte Carlo

Pour obtenir des résultats physiquement exploitables, la randomisation est strictement structurée autour de **quatre familles principales de paramètres**, complétées par les conditions initiales du véhicule.

### 3.1 Tableau des Paramètres d'Entrée

| Famille | Paramètre C++ | Intitulé Physique | Unité | Plage [$Min, Max$] | Distribution | Justification Physique |
| :--- | :--- | :--- | :---: | :---: | :---: | :--- |
| **Environnement** | `wind_speed` | Vitesse du vent moyen | $	ext{m/s}$ | $[0.0, 10.0]$ | Uniforme | Spectre opérationnel usuel de vol micro-dronique. |
| | `wind_direction` | Direction du vent | $	ext{deg}$ | $[0.0, 360.0]$ | Uniforme | Isotropie de la rose des vents. |
| **Capteurs** | `sensor_noise` | Bruit de mesure IMU/GPS | $	ext{σ}$ (ratio) | $[0.0, 2.0]$ | Uniforme | Bruit blanc additif gaussien pondéré. |
| | `sensor_bias` | Biais statique de l'IMU | $	ext{m/s}^2$ / $	ext{rad/s}$ | $[-0.2, 0.2]$ | Normale ($\mu=0, \sigma=0.05$) | Dérive thermique et tolérance de calibration. |
| **Actionneurs** | `motor_efficiency` | Rendement moteur / poussée | Pu | $[0.80, 1.00]$ | Uniforme | Chute de tension batterie et usure mécanique. |
| | `servo_error` | Erreur d'alignement gouvernes | $	ext{deg}$ | $[-3.0, 3.0]$ | Normale ($\mu=0, \sigma=1.0$) | Jeu mécanique et imprécision des servomoteurs. |
| **Communication** | `packet_loss` | Taux de perte de paquets | $\%$ | $[0.0, 0.10]$ | Uniforme | Perte de liaison télémétrique ou ADS-B/ATC. |
| | `communication_latency` | Latence de transmission | $	ext{ms}$ | $[0, 50]$ | Uniforme | Gigue réseau et temps de traitement des messages. |
| **Conditions Initiales** | `initial_position_offset` | Écart de position initiale | $	ext{m}$ | $[-2.0, 2.0]$ | Uniforme | Erreur d'alignement au démarrage sol. |
| | `initial_mass_offset` | Variation de masse embarquée | $	ext{kg}$ | $[-0.1, 0.1]$ | Normale ($\mu=0, \sigma=0.03$) | Tolérance sur la charge utile et châssis. |

---

## 4. Distributions de Probabilité & Génération Aléatoire

Il ne suffit pas d'inscrire un intervalle $[a, b]$ ; la loi de probabilité retenue façonne directement la viabilité des tests statistiques.

### 4.1 Distribution Uniforme vs Normale

* **Distribution Uniforme ($\mathcal{U}[a, b]$) :** Utilisée lorsque chaque valeur dans l'intervalle a une probabilité égale de se produire. Adaptée aux paramètres d'environnement sans données statistiques a priori (ex: direction du vent) ou pour explorer les limites maximales d'un domaine.
* **Distribution Normale ($\mathcal{N}(\mu, \sigma^2)$) :** Utilisée pour les paramètres physiques dont les variations se concentrent autour d'une valeur nominale (ex: biais capteur, masse).

```
   Distribution Uniforme                       Distribution Normale
    ┌─────────────────┐                                  /    │                 │                                 /      │                 │                                /    ____│_________________│____                      _____/      \_____
   a                   b                             a    μ    b
```

### 4.2 Reproductibilité et Gestion des Seeds

Pour garantir la répétabilité des campagnes et la rejouabilité exacte des bugs :
1. Le **`ScenarioGenerator`** est initialisé avec un **`master_seed`** entier sur 64 bits (ex: `12345ULL`).
2. Chaque scénario génère une sous-graine déterministe : `scenario_seed = hash(master_seed, run_id)`.
3. Lorsqu'une simulation échoue, l'identifiant `run_id` et la graine exacte permettent de **rejouer la simulation isolée** sous debugger C++ ou avec des traces verbeuses.

```cpp
// Exemple de sélection déterministe du scénario
uint64_t scenario_seed = master_seed ^ (static_cast<uint64_t>(run_id) * 0x9E3779B97F4A7C15ULL);
std::mt19937_64 rng(scenario_seed);
```

---

## 5. Critères de Succès, Métriques & Classification des Échecs

### 5.1 Critères de Succès (`mission_success`)

La mission est définie par le maintien du vecteur d'état de l'aéronef à l'intérieur d'un enveloppe spatio-temporelle spécifiée pendant toute la durée du vol.

$$	ext{Mission Success} = (	ext{max\_position\_error} \le R_{	ext{max}}) \land (	ext{max\_altitude_error} \le \Delta Z_{	ext{max}}) \land (	ext{unrecovered\_faults} = 0)$$

### 5.2 Métriques Enregistrées

Pour chaque run, les métriques minimales suivantes sont calculées :
* **`max_position_error`** ($	ext{m}$) : Erreur maximale en plan horizontal $(X, Y)$ par rapport à la trajectoire consigne.
* **`max_altitude_error`** ($	ext{m}$) : Écart maximal d'altitude $(Z)$.
* **`recovery_time`** ($	ext{ms}$) : Temps écoulé entre l’apparition d'un incident/défaut et le retour à une consigne stable.
* **`fault_count`** : Nombre total d'anomalies levées pendant le vol.
* **`mission_duration`** ($	ext{s}$) : Temps de vol simulé accompli.

### 5.3 Classification des Échecs (`FailureReason`)

En cas d'échec (`mission_success == false`), le système attribue une cause principale unique selon l'énumération C++ suivante :

```cpp
enum class FailureReason : uint8_t {
    NONE = 0,               // Succès
    OUT_OF_ZONE,            // Violation de la frontière géographique maximale
    ALTITUDE_LOST,          // Perte de portance / crash / écart d'altitude critique
    CONTROL_UNSTABLE,       // Divergence des boucles de régulation (PID/LQR saturés)
    COMMUNICATION_FAILURE,  // Perte prolongée de battement réseau (timeout)
    UNRECOVERED_FAULT,      // Panne système/capteur non résolue par la FDIR
    WATCHDOG_FAILURE        // Non-respect du temps imparti dans la boucle temps réel SIL
};
```

---

## 6. Sizing des Campagnes de Simulation

L'exécution des tests statistiques s'effectue en trois phases incrémentales :

```
┌─────────────────────────┐
│ Phase 1 : Development   │  ---> 100 runs (Validation rapide, fumée)
└───────────┬─────────────┘
            ▼
┌─────────────────────────┐
│ Phase 2 : Validation    │  ---> 1 000 runs (Convergence statistique initiale)
└───────────┬─────────────┘
            ▼
┌─────────────────────────┐
│ Phase 3 : Final         │  ---> 10 000 runs (Campagne de qualification formelle)
└─────────────────────────┘
```

1. **Campagne Développement ($N = 100$) :**
   * *Objectif :* Vérifier la validité des générateurs, le respect du déterminisme, la stabilité mémoire du banc SIL et le bon formattage du fichier de sortie.
   * *Temps d'exécution :* $< 10 	ext{ secondes}$.
2. **Campagne Validation ($N = 1\,000$) :**
   * *Objectif :* Identifier les cas limites manifestes, calculer un taux de succès préliminaire, vérifier la dispersion des métriques.
   * *Temps d'exécution :* $pprox 1 	ext{ à } 2 	ext{ minutes}$.
3. **Campagne Finale de Qualification ($N = 10\,000$) :**
   * *Objectif :* Qualification formelle. Analyse de sensibilité croisée, recherche de modes de défaillance combinatoires rares (ex: vent fort + forte latence + perte paquets).
   * *Temps d'exécution :* $pprox 10 	ext{ à } 15 	ext{ minutes}$ (avec multi-threading).

---

## 7. Interfaces & Structures C++

### 7.1 Spécification des Types C++

```cpp
#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace flight_control::validation {

enum class FailureReason : uint8_t {
    NONE = 0,
    OUT_OF_ZONE,
    ALTITUDE_LOST,
    CONTROL_UNSTABLE,
    COMMUNICATION_FAILURE,
    UNRECOVERED_FAULT,
    WATCHDOG_FAILURE
};

struct Scenario {
    uint64_t run_id{0};
    uint64_t seed{0};

    // Environment
    double wind_speed{0.0};          // m/s
    double wind_direction{0.0};      // deg

    // Sensors
    double sensor_noise{1.0};        // ratio
    double sensor_bias{0.0};         // m/s^2

    // Actuators
    double motor_efficiency{1.0};    // ratio
    double servo_error{0.0};         // deg

    // Communication
    double packet_loss{0.0};         // ratio [0..1]
    double communication_latency{0}; // ms
};

struct SimulationResult {
    uint64_t run_id{0};
    bool mission_success{false};
    FailureReason failure_reason{FailureReason::NONE};

    double max_position_error{0.0};  // m
    double max_altitude_error{0.0};  // m
    double recovery_time{0.0};       // ms
    uint32_t fault_count{0};
    double simulated_time{0.0};      // s
};

class ScenarioGenerator {
public:
    explicit ScenarioGenerator(uint64_t master_seed);
    Scenario generate_scenario(uint64_t run_id);

private:
    uint64_t master_seed_;
};

class ResultCollector {
public:
    explicit ResultCollector(const std::string& output_csv_path);
    void record(const Scenario& scenario, const SimulationResult& result);
    void write_to_disk();

private:
    std::string csv_path_;
    // Stockage interne des résultats
};

class MonteCarloRunner {
public:
    MonteCarloRunner(uint64_t master_seed, size_t total_runs);
    void run_campaign();

private:
    uint64_t master_seed_;
    size_t total_runs_;
};

} // namespace flight_control::validation
```

### 7.2 Implémentation Conceptuelle de la Boucle Principale

```cpp
void MonteCarloRunner::run_campaign() {
    ScenarioGenerator generator(master_seed_);
    ResultCollector collector("docs/validation/monte_carlo_results.csv");
    SILSimulator simulator;

    for (size_t i = 1; i <= total_runs_; ++i) {
        // 1. Génération du scénario reproductible
        Scenario scenario = generator.generate_scenario(i);

        // 2. Exécution du simulateur SIL
        SimulationResult result = simulator.run(scenario);

        // 3. Collecte des paires (Scénario, Résultat)
        collector.record(scenario, result);
    }

    collector.write_to_disk();
}
```

---

## 8. Format de Stockage et d'Exportation des Données

Afin de permettre une exploitation facile via Python (Pandas, Seaborn, Matplotlib) ou des scripts de qualification, les résultats sont exportés au format **CSV** avec en-têtes explicites.

### 8.1 Extrait du Fichier `monte_carlo_results.csv`

```csv
run_id,seed,wind_speed,wind_direction,sensor_noise,sensor_bias,motor_efficiency,servo_error,packet_loss,latency_ms,success,failure_reason,max_pos_err,max_alt_err,recovery_time_ms
1,12345001,2.4,182.1,0.85,0.01,0.98,0.2,0.01,12.0,true,NONE,1.42,0.31,0.0
2,12345002,7.8,43.5,1.42,-0.08,0.88,-1.2,0.06,31.4,true,NONE,4.82,0.94,120.0
3,12345003,9.5,210.8,1.91,0.18,0.81,2.7,0.09,46.2,false,OUT_OF_ZONE,18.65,3.12,0.0
4,12345004,3.1,12.4,0.72,-0.02,0.92,0.1,0.02,18.0,false,COMMUNICATION_FAILURE,8.12,1.05,450.0
```

---

## 9. Complémentarité : Tests Déterministes vs Monte Carlo

Il est fondamental de maintenir deux approches de validation strictement complémentaires au sein de l'intégration continue (CI/CD) :

```
                  ┌────────────────────────────────────────┐
                  │          Stratégie de Test SIL         │
                  └───────────────────┬────────────────────┘
                                      │
           ┌──────────────────────────┴──────────────────────────┐
           ▼                                                     ▼
┌──────────────────────────────┐                      ┌──────────────────────────────┐
│     Tests Déterministes      │                      │     Campagnes Monte Carlo    │
│  (Regression Unit/System)    │                      │   (Exploration Statistique)  │
├──────────────────────────────┤                      ├──────────────────────────────┤
│ • Scénarios fixes nommés     │                      │ • $N=10\,000$ runs aléatoires│
│ • Validation des correctifs  │                      │ • Découverte d'angles morts  │
│ • Assertions strictes pass/fail                     │ • Analyse de sensibilité     │
│ • Rapide (exécution en CI)   │                      │ • Exécution périodique/nuit  │
└──────────────────────────────┘                      └──────────────────────────────┘
```

---

## 10. Exploitation des Résultats & Boucle d'Ingénierie

### 10.1 Exemple d'Analyse des Échecs

Après une campagne de 10 000 simulations :

```
--------------------------------------------------
RÉSULTATS DE LA CAMPAGNE MONTE CARLO (N = 10,000)
--------------------------------------------------
Total Runs            : 10000
Successful Missions   :  9732
Failed Missions       :   268
Success Rate          :  97.32 %
--------------------------------------------------
DISTRIBUTION DES ÉCHECS :
  [1] OUT_OF_ZONE           : 143 (53.4 %)
  [2] ALTITUDE_LOST         :  61 (22.8 %)
  [3] COMMUNICATION_FAILURE :  32 (11.9 %)
  [4] CONTROL_UNSTABLE      :  21 ( 7.8 %)
  [5] UNRECOVERED_FAULT     :  11 ( 4.1 %)
--------------------------------------------------
```

### 10.2 Analyse des Causes Racines (Root Cause Analysis)

En analysant la corrélation entre les variables d'entrée et les échecs :
1. **Sensibilité à la latence :** $85\%$ des échecs `COMMUNICATION_FAILURE` se produisent lorsque `communication_latency > 35 ms` ET `packet_loss > 7%`.
2. **Sensibilité à la puissance :** $90\%$ des échecs `OUT_OF_ZONE` surviennent lorsque `motor_efficiency < 0.84` en présence d'un vent `wind_speed > 8.0 m/s`.

### 10.3 Boucle d'Ingénierie Rétroactive

L'analyse statistique permet d'engager une action d'ingénierie ciblée :

$$	ext{Hypothèse} \longrightarrow 	ext{Modifications Code/PID} \longrightarrow 	ext{Nouvelle Campagne} \longrightarrow 	ext{Gain de Taux de Succès}$$

* **Action :** Implémentation d'un prédicteur de Smith dans le contrôleur d'altitude pour compenser la latence et ajustement du gain d'anti-windup pour le vent fort.
* **Résultat après re-run :** Le taux de succès passe de **97.32% à 99.41%**, validant quantitativement le gain de performance du système de contrôle de vol.
