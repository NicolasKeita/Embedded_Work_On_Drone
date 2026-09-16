import type { TwinSnapshot } from './twin-data';

export function windVelocity(snapshot: Pick<TwinSnapshot, 'wind'>): [number, number, number] {
  const x = snapshot.wind?.x_mps ?? 0;
  const y = snapshot.wind?.y_mps ?? 0;
  if (!Number.isFinite(x) || !Number.isFinite(y)) return [0, 0, 0];
  return [x, y, Math.hypot(x, y)];
}
