import assert from 'node:assert/strict';
import test from 'node:test';
import { flightPosition, WORLD } from '../lib/flight-world.ts';

test('330 metres aligns the aircraft datum with the tower tip', () => {
  assert.equal(flightPosition({ x_m: 0, y_m: 0, altitude_m: 330 })[1], WORLD.towerPosition[1] + WORLD.towerHeight);
});

test('all axes preserve metres, including high and negative altitudes', () => {
  for (const altitude of [-10, 0, 9.2, 330, 7000, 20000]) {
    assert.deepEqual(flightPosition({ x_m: 125, y_m: -80, altitude_m: altitude }), [125, altitude, -80]);
  }
});

 test('launch clearance does not shift airborne altitude', async () => {
  const { dronePosition } = await import('../lib/flight-world.ts');
  assert.equal(dronePosition({ x_m: 0, y_m: 0, altitude_m: 0 })[1], WORLD.launchDeckHeight);
  assert.equal(dronePosition({ x_m: 0, y_m: 0, altitude_m: 330 })[1], WORLD.towerHeight);
});

test('spin scales with RPM and frame time, and stops without resetting phase', async () => {
  const { visualSpinStep } = await import('../lib/flight-world.ts');
  assert.equal(visualSpinStep(6000, .01), visualSpinStep(3000, .01) * 2);
  assert.equal(visualSpinStep(3000, .02), visualSpinStep(3000, .01) * 2);
  for (const rpm of [0, -1, NaN, Infinity]) assert.equal(visualSpinStep(rpm, .016), 0);
});
