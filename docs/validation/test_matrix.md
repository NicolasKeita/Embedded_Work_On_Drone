# Matrice de couverture des tests

Cette matrice associe les comportements aux sources de vérification.
**Couvert** signifie qu'une assertion correspondante est présente, sans annoncer
un PASS d'exécution. **Inspection** décrit le code ; **partiel** ne couvre
qu'une partie du comportement. Voir les [conventions de preuve](evidence.md).

Les identités et paramètres des scénarios appartiennent au [catalogue](scenarios.md).
Les vérifications HIL ci-dessous concernent le loopback, sauf HIL-06.

## Mission et contrôle

| IDs | Comportement | Vérification / source | Couverture |
| --- | --- | --- | --- |
| MIS-01 | Profil progressif de décollage | [Profils](../../Tests/Scenarios/FlightScenarios-Profiles.cpp) et [contrôleur](../../Src/Control/FlightController-Takeoff.cpp) | Tests de trajectoire + inspection du profil |
| MIS-02..04, CTL-01 | Mission nominale, état COMPLETE, erreur d'altitude bornée | [SilScenarios-Nominal](../../Tests/Sil/SilScenarios-Nominal.cpp), `NOMINAL-001` | Couvert |
| MIS-05 | Mission autonome complète | [FlightScenarios-Mission](../../Tests/Scenarios/FlightScenarios-Mission.cpp), `NOMINAL-010` | Couvert |
| CTL-02..03 | Asservissement des axes X/Y | [FlightScenarios-Axes](../../Tests/Scenarios/FlightScenarios-Axes.cpp), `NOMINAL-008/009` | Couvert |
| CTL-04 | Réponse aux commandes RPM/servos | [Scenarios-Cases](../../Tests/Scenarios/Scenarios-Cases.cpp), `NOMINAL-002..007` | Couvert |
| CTL-05 | Bornes du contrôleur | [Control](../../Src/Control/) | Inspection |

## Communication, sûreté et injection

| IDs / mode | Comportement | Vérification / source | Couverture |
| --- | --- | --- | --- |
| COM-01 | Publication et numéros de heartbeat | [Observabilité heartbeat](../../Tests/Sil/Observability/SilObservability-Heartbeat.cpp) | Couvert |
| COM-02, FI-01 / FM-01 | Indisponibilité FC1, détection, délai et abandon | [SilScenarios-Core](../../Tests/Sil/SilScenarios-Core.cpp), `FAULT_INJECTOR-001` | Couvert en SIL ; [tests HIL](../../Tests/Hil/HilTests-Faults.cpp) pour détection/états/verdict |
| COM-03 / FM-02 | Coupure de lien et comptage des timeouts | [Observabilité communication](../../Tests/Sil/Observability/SilObservability-Comms.cpp) | Partiel : les assertions de statistiques ne prouvent pas toutes les latences et réponses |
| COM-04, FI-03 / FM-03 | Pertes seedées et conséquences sur le lien | [Observabilité des pertes](../../Tests/Sil/Observability/SilObservability-Dropped.cpp) | Partiel ; aucun scénario fonctionnel nommé |
| COM-05 | Retour en mission après faute critique | [SafetyManager](../../Src/Safety/SafetyManager.cppm) | Non pris en charge pendant la même exécution |
| SAF-01..02, FI-04 / FM-04 | Détection capteur, DEGRADED puis COMPENSATED | [SilScenarios-Safety](../../Tests/Sil/SilScenarios-Safety.cpp), `FAULT_INJECTOR-003` | Couvert ; [tests HIL](../../Tests/Hil/HilTests-Faults.cpp) correspondants |
| SAF-03..05 | SAFE_MODE, abandon et verdict indépendant du succès de mission | [SilScenarios-Core](../../Tests/Sil/SilScenarios-Core.cpp) et [observabilité](../../Tests/Sil/Observability/SilObservability-Comms.cpp) | Couvert pour la perte FC1 |
| SAF-06 | Retour nominal après faute capteur temporaire | [Maintien de mesure](../../Tests/Sil/Observability/Telemetry/SilObservabilityTelemetry-Hold.cpp) | Partiel : contrôler aussi transitions et résultat de récupération dans la capture |
| FM-05 | Injection actionneur et métadonnées | [FaultMetadata](../../Tests/Sil/Observability/Faults/SilObservability-FaultMetadata.cpp) | Partiel : ces assertions ne prouvent pas seules la compensation de bout en bout |
| SAF-07 | État de santé FAILED | [Taxonomie](../safety/fault_taxonomy.md) | Retiré ; ne pas confondre avec MissionState ou l'affichage |
| FI-07 / FM-06 | Faute SYSTEM d'échéance de contrôle | [Analyse](../fmeca/failure_modes.md#fm-06--control_deadline_missed) | Non implémenté ; instrumentation HIL distincte |
| FI-08 / FM-07 | Garde générique d'intégrité numérique | [Analyse](../fmeca/failure_modes.md#fm-07--invalid_numerical_state) | Non implémenté |
| FI-09 | Autres capteurs / servos indépendants | [Cibles autorisées](../safety/fault_taxonomy.md#21-target-coverage-honest-component-mapping) | Pas de voie d'injection autorisée |

La criticité, les effets et les mitigations sont maintenus dans la
[FMECA](../fmeca/fmeca.md), sans autre matrice de verdicts parallèle.

## SIL et Monte Carlo

| IDs | Comportement | Source | Couverture |
| --- | --- | --- | --- |
| SIL-01..05 | Suite partagée, détection et résultats | [Suite SIL](../../Tests/Sil/SilScenarios-Suite.cpp) | Trois scénarios partagés |
| SIL-06 | Traces, métadonnées et télémétrie | [Observability](../../Tests/Sil/Observability/) | Assertions spécialisées |
| MC-01..04 | Tirages seedés, exécution et export statistique | [MonteCarlo](../../Src/Runners/MonteCarlo/) | Outillage présent ; résultats de campagne à capturer |
| MC-05 | Campagne statistique de fautes | [Validation](../../Src/SIL/Validation/) | Bibliothèque non exposée par la CLI |

Le [guide Monte Carlo](monte_carlo.md) distingue les paramètres tirés de ceux
qui influencent effectivement la dynamique.

## HIL

| IDs | Comportement | Source | Couverture |
| --- | --- | --- | --- |
| HIL-01 | Boucle fermée cadencée | [Runner tests](../../Tests/Hil/HilTests-Runner.cpp) | Couvert en loopback |
| HIL-02 | Instrumentation des échéances | [Timing tests](../../Tests/Hil/HilTests-Timing.cpp) | Couvert ; timing réel à relever pour chaque exécution |
| HIL-03 | Codec, séquence, écho et rejet | [Transport tests](../../Tests/Hil/Transport/) | Couvert en loopback |
| HIL-04 | Données capteurs/actionneurs | [Data tests](../../Tests/Hil/HilTests-Data.cpp) | Couvert en loopback |
| HIL-05 | Deux fautes partagées, réarmement de supervision | [Fault tests](../../Tests/Hil/HilTests-Faults.cpp) | Couvert en loopback ; pas une campagne sur MCU |
| HIL-06 | Série et supervision physique FC1/FC2 | [Architecture HIL](../hil/hil_architecture.md) | Implémenté ; mesures et provenance nécessaires |
| HIL-07 | Suite `--selftest` | [HilTests-Core](../../Tests/Hil/HilTests-Core.cpp) | Regroupe les tests hôtes |

Le [plan de campagne](../hil/hil_validation.md) décrit la progression vers des
résultats mesurés, publiés selon les [conventions de preuve](evidence.md).
