# Bilan de validation

## État de la documentation

La révision documentaire du 15 septembre 2026 aligne les guides sur les sources
présentes. Le banc dispose de STM32 physiques et des firmwares Zephyr FC1/FC2 ;
le HIL série est implémenté. La
[matrice](test_matrix.md) décrit les comportements couverts par les tests.

Aucune nouvelle campagne SIL, HIL physique ou Monte Carlo n'a été exécutée
pour produire ce bilan. Les anciens nombres illustratifs et les statuts PASS
inférés de la seule présence d'assertions ne sont pas des résultats mesurés.
Les [conventions de preuve](evidence.md) définissent les informations nécessaires
pour publier les prochaines exécutions.

## Capacités et périmètre

| Domaine | État vérifiable dans le dépôt | Référence |
| --- | --- | --- |
| SIL | Runner, scénarios et assertions présents | [SIL](sil.md) |
| HIL loopback | Runner cadencé, codec et tests présents | [HIL](hil.md) |
| HIL physique | Transport série et deux applications Zephyr présents | [Architecture HIL](../hil/hil_architecture.md) |
| Monte Carlo | Campagne CLI de dispersion physique présente | [Monte Carlo](monte_carlo.md) |
| Fautes | Deux scénarios fonctionnels partagés ; couverture de composants complémentaire | [Catalogue](scenarios.md) · [Matrice](test_matrix.md) |
| Digital Twin | Viewer, publication et relecture présents | [Guide](../digital_twin/README.md) |

## Ce qu'il reste à établir par les captures

Pour chaque configuration, publier les verdicts réellement observés, les traces
et la provenance. Sur matériel, mesurer le timing, vérifier l'identité des
firmwares et la propagation effective du diagnostic FC2 ; comparer au SIL avec
les limites de configuration et de temporisation documentées.

Le [plan HIL](../hil/hil_validation.md) guide cette campagne. Les limites de sûreté
et les modes non couverts restent dans la [FMECA](../fmeca/fmeca.md), sans recopier
son analyse ici. Le projet reste un banc de développement du logiciel avec
aéronef simulé, sans validation de vol réel.
