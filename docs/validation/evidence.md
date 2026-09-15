# Résultats, couverture et preuves

Cette page définit les conventions communes à tous les rapports de validation.

## Statuts

| Terme | Signification |
| --- | --- |
| Implémenté | Un chemin existe dans les sources |
| Couvert | Un test ou une assertion exerce un comportement identifié |
| À mesurer | Une exécution identifiée est nécessaire pour conclure |
| PASS / FAIL | Verdict effectivement produit par une exécution conservée |
| Non implémenté | Le chemin nécessaire n'existe pas |
| Non exposé | Du code existe mais n'est pas accessible par le runner concerné |

La présence d'une assertion ne constitue pas un PASS. Une moyenne temporelle
attendue ne constitue pas une mesure. La disponibilité des STM32 ne prouve
pas l'identité du firmware chargé ni l'absence de dépassements d'échéance.

## Provenance d'une exécution

Conserver avec le résultat : date, commit et éventuelles modifications locales,
versions de compilation, commande complète, scénario, durée, pas, seed,
interface réellement sélectionnée, politique d'échéance et code de sortie.
Sur matériel, ajouter les identités FC1/FC2, versions des images chargées,
configuration Zephyr et câblage.

Joindre les traces, télémétries et résumé produits. Les fichiers sous
`docs/validation/data/` sont des sorties générées et ignorées par Git ; leur
présence locale sans provenance ne suffit pas à valider la révision courante.

## Interprétation

- **Mission success** et **test verdict** sont distincts : un abandon attendu
  peut produire un verdict PASS.
- Un essai loopback reste un essai hôte, même si une sonde STM32 est détectée.
- Un essai matériel couvre sa configuration et ses conditions, sans prouver
  tous les scénarios ni toutes les charges possibles.
- Les extraits illustratifs doivent être étiquetés comme tels, sans chiffres
  présentés comme mesurés.

Cette révision documentaire repose sur la lecture du code. Elle ne publie pas
une nouvelle campagne d'exécution SIL/HIL/Monte Carlo. Les anciens exemples
sans capture ont été retirés des conclusions de validation.
