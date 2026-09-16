import assert from 'node:assert/strict';
import test from 'node:test';
import { windVelocity } from '../lib/wind.ts';

test('old recordings and idle snapshots have no wind effect', () => {
  assert.deepEqual(windVelocity({}), [0, 0, 0]);
  assert.deepEqual(windVelocity({ wind: { x_mps: 0, y_mps: 0 } }), [0, 0, 0]);
});
test('wind preserves world direction and reports vector magnitude', () => {
  assert.deepEqual(windVelocity({ wind: { x_mps: -3, y_mps: 4 } }), [-3, 4, 5]);
});
test('invalid wind telemetry cannot contaminate geometry', () => {
  assert.deepEqual(windVelocity({ wind: { x_mps: Infinity, y_mps: 4 } }), [0, 0, 0]);
});
