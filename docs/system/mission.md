# Mission

## Objectif et progression

Atteindre une altitude cible puis maintenir la position dans la zone tolérée
pendant la durée requise. La progression nominale du contrôleur est :

```text
SPIN_UP → TAKEOFF → CLIMB → STATION_KEEPING → COMPLETE
```

Une réponse critique peut interrompre la mission vers `ABORTED`.
`FAILED` existe aussi dans `MissionState` ; il ne faut pas le confondre avec
les états de santé définis dans la [taxonomie](../safety/fault_taxonomy.md).

## Configuration

Les valeurs par défaut sont définies dans
[ControllerConfig](../../Src/Control/Types/FlightControllerTypes.cppm) :
spin-up de 3 s, transition de décollage de 12 s, vitesse de montée de
0.617 m/s, tolérance d'altitude de 0.5 m, tolérance de position de 1 m
et maintien de 5 s. Les transitions sont implémentées dans
[FlightController-Mission.cpp](../../Src/Control/FlightController-Mission.cpp)
et [FlightController-Takeoff.cpp](../../Src/Control/FlightController-Takeoff.cpp).

L'altitude cible et la durée totale appartiennent à la configuration du
[scénario](../validation/scenarios.md), et non à une constante universelle de
mission. La durée totale d'essai est distincte du temps de maintien.
Les limites de transmission de configuration au MCU sont décrites dans
l'[architecture HIL](../hil/hil_architecture.md#limites-actuelles).

## Critère de maintien

Le contrôleur compare l'erreur horizontale et l'erreur d'altitude aux tolérances
configurées et accumule le temps de maintien lorsque les conditions sont
satisfaites. Le modèle initial de rectangle géographique indépendant ne doit
pas être interprété comme un géofencing matériel implémenté.

La réussite de mission et le verdict de test sont distincts ; voir les
[conventions de preuve](../validation/evidence.md).
