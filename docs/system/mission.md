# Mission

## Objective

The aircraft must reach a target altitude and maintain its
position inside a predefined operational zone for a given
duration.

## Mission phases

1. Takeoff / ascent
2. Climb to target altitude
3. Station keeping
4. Mission completion

## Station keeping

During the station-keeping phase, the aircraft must remain
inside the defined operational zone while maintaining the
target altitude.

External disturbances such as wind may cause the aircraft
to drift. The flight control system must compensate for
these disturbances.

## Mission parameters

- Target altitude: TBD
- Zone size: TBD
- Mission duration: TBD

## Aircraft state

The aircraft state contains:

- X position
- Y position
- Altitude
- Pitch
- Roll

Schema needed:
```
            Z / altitude
                   ↑
                   │
                   ●
                  / \
                 /   \
                /     \
               ↓
              Y

               └──────────→ X
```

## Flight Zone

The flight zone is defined as a simple 2D bounding rectangle (top-down view):

```
                 Y
                 ↑

        +-------------------+
        |                   |
        |                   |
        |        ●          |  ← aircraft
        |                   |
        |                   |
        +-------------------+

                 └──────────→ X
```

### Boundary Conditions

$$x_{min} \le X \le x_{max}$$
$$y_{min} \le Y \le y_{max}$$

Based on these condition boundaries, the system determines the state:

- **INSIDE**
- **OUTSIDE**


## Mission Success Criteria

The mission succeeds if the aircraft stays:
- Inside the zone
- Within target altitude tolerance
- For the required duration