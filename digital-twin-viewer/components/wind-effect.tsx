'use client';

import { useMemo, useRef } from 'react';
import { useFrame } from '@react-three/fiber';
import * as THREE from 'three';
import type { TwinSnapshot } from '@/lib/twin-data';
import { dronePosition } from '@/lib/flight-world';
import { windVelocity } from '@/lib/wind';

export function WindEffect({ snapshot }: { snapshot: TwinSnapshot }) {
  const lines = useRef<THREE.LineSegments>(null);
  const positions = useMemo(() => new Float32Array(96 * 6), []);
  const [x, z, speed] = windVelocity(snapshot);
  useFrame(() => {
    if (!lines.current || speed === 0) return;
    const attribute = lines.current.geometry.attributes.position;
    const dx = x / speed;
    const dz = z / speed;
    for (let i = 0; i < 96; i++) {
      const along = ((i * 7.31 + snapshot.time_s * speed) % 40) - 20;
      const across = ((i * 13.17) % 32) - 16;
      const height = ((i * 3.71) % 18) - 6;
      const px = dx * along - dz * across;
      const pz = dz * along + dx * across;
      attribute.setXYZ(i * 2, px, height, pz);
      attribute.setXYZ(i * 2 + 1, px - dx * (1 + speed * .35), height, pz - dz * (1 + speed * .35));
    }
    attribute.needsUpdate = true;
  });
  return <lineSegments ref={lines} position={dronePosition(snapshot.aircraft)} visible={speed > 0} frustumCulled={false}>
    <bufferGeometry><bufferAttribute attach="attributes-position" args={[positions, 3]} /></bufferGeometry>
    <lineBasicMaterial color="#e0faff" transparent opacity={.55} depthWrite={false} />
  </lineSegments>;
}
