import assert from 'node:assert/strict';
import test from 'node:test';
import { createIdleSnapshot, hasAltitudeFault, holdLastKnownAltitude, holdLastKnownAltitudes } from '../lib/twin-data.ts';

function sample(altitude, fault = null) {
  const snapshot = createIdleSnapshot();
  snapshot.aircraft.altitude_m = altitude;
  snapshot.aircraft.z_m = -altitude;
  snapshot.active_fault = fault;
  return snapshot;
}

test('rejects injected metres, kilometres and non-finite samples before fault metadata arrives', () => {
  const previous = sample(330);
  for (const altitude of [99999, 99999000, NaN, Infinity, -Infinity]) {
    const held = holdLastKnownAltitude(sample(altitude), previous);
    assert.equal(held.aircraft.altitude_m, 330);
    assert.equal(held.aircraft.z_m, -330);
    assert.equal(hasAltitudeFault(held), true);
    assert.equal(held.raw_altitude_m, altitude);
  }
});

test('fault burst holds last good altitude and resumes on recovery in replay and live', () => {
  const samples = [sample(120), sample(99999), sample(42, 'INVALID_SENSOR_DATA'), sample(99999), sample(121)];
  const replay = holdLastKnownAltitudes(samples);
  let previous;
  const live = samples.map((snapshot) => (previous = holdLastKnownAltitude(snapshot, previous)));
  assert.deepEqual(replay, live);
  assert.deepEqual(replay.map((snapshot) => snapshot.aircraft.altitude_m), [120, 120, 120, 120, 121]);
  assert.equal(hasAltitudeFault(replay.at(-1)), false);
  assert.equal(samples[1].aircraft.altitude_m, 99999);
});

test('invalid first frame uses safe launch datum and valid high altitude remains available', () => {
  assert.equal(holdLastKnownAltitude(sample(99999)).aircraft.altitude_m, 0);
  assert.equal(holdLastKnownAltitude(sample(20000)).aircraft.altitude_m, 20000);
});
