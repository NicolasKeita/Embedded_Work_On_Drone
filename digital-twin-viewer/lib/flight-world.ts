export type WorldPoint = [number, number, number];

export const WORLD = {
  towerHeight: 330,
  droneSpan: 1.2,
  launchDeckHeight: 1.2,
  towerPosition: [-180, 0, -210] as WorldPoint,
  houseWallHeight: 6.4,
  houseHeight: 9.2,
  groundExtent: 12000,
};

/* One scene unit is one metre on every axis; the launch datum is y = 0. */
export function flightPosition(aircraft: { x_m: number; y_m: number; altitude_m: number }): WorldPoint {
  return [aircraft.x_m, aircraft.altitude_m, aircraft.y_m];
}

/* Ground clearance only: airborne metric positions, including 330 m, are unchanged. */
export function dronePosition(aircraft: { x_m: number; y_m: number; altitude_m: number }): WorldPoint {
  const position = flightPosition(aircraft);
  position[1] = Math.max(WORLD.launchDeckHeight, position[1]);
  return position;
}

/* A 2% visual time scale keeps typical operating RPM legible at display refresh rates. */
export function visualSpinStep(rpm: number, deltaSeconds: number) {
  if (!Number.isFinite(rpm) || rpm <= 0 || !Number.isFinite(deltaSeconds) || deltaSeconds <= 0) return 0;
  return rpm * Math.PI * 2 / 60 * .02 * Math.min(deltaSeconds, .1);
}
