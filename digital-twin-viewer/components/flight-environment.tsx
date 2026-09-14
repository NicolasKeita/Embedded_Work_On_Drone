'use client';

import { useMemo } from 'react';
import * as THREE from 'three';
import { mergeGeometries } from 'three/addons/utils/BufferGeometryUtils.js';
import { WORLD, type WorldPoint } from '@/lib/flight-world';

function Box({ position, size, color, ...props }: { position: WorldPoint; size: WorldPoint; color: string; rotation?: WorldPoint }) {
  return <mesh position={position} castShadow={size[1] > .2} receiveShadow {...props}><boxGeometry args={size} /><meshStandardMaterial color={color} roughness={.82} /></mesh>;
}

function House({ position, rotation = 0 }: { position: WorldPoint; rotation?: number }) {
  const roof = useMemo(() => {
    const shape = new THREE.Shape();
    shape.moveTo(-6.8, 0);
    shape.lineTo(6.8, 0);
    shape.lineTo(0, WORLD.houseHeight - WORLD.houseWallHeight);
    shape.closePath();
    return shape;
  }, []);
  return <group position={position} rotation={[0, rotation, 0]}>
    <Box position={[0, 3.2, 0]} size={[12, WORLD.houseWallHeight, 9]} color="#d8cbb2" />
    <Box position={[0, .22, 0]} size={[12.4, .44, 9.4]} color="#8b8c82" />
    <mesh position={[0, WORLD.houseWallHeight, -5]} castShadow><extrudeGeometry args={[roof, { depth: 10, bevelEnabled: false }]} /><meshStandardMaterial color="#665a51" roughness={.92} /></mesh>
    <Box position={[3.5, 8.1, -1.5]} size={[.85, 2.2, .9]} color="#967363" />
    {[1.9, 4.8].flatMap((height) => [-4, 0, 4].map((x) => <group key={`${height}-${x}`} position={[x, height, 4.52]}>
      <Box position={[0, 0, 0]} size={[1.65, 1.8, .16]} color="#f2ebd9" />
      <Box position={[0, 0, .1]} size={[1.38, 1.53, .08]} color="#365969" />
      <Box position={[0, 0, .16]} size={[.07, 1.55, .04]} color="#ece2cc" />
      <Box position={[0, 0, .16]} size={[1.4, .07, .04]} color="#ece2cc" />
      <Box position={[0, -.94, .12]} size={[1.9, .15, .38]} color="#e5dcc9" />
    </group>))}
    <Box position={[1.9, 1.12, 4.61]} size={[1.1, 2.24, .16]} color="#4e635e" />
    <Box position={[1.9, .12, 5.1]} size={[1.9, .24, 1]} color="#a9a99d" />
  </group>;
}

function Tree({ position, height, seed }: { position: WorldPoint; height: number; seed: number }) {
  return <group position={position}>
    <mesh position={[0, height * .3, 0]} castShadow><cylinderGeometry args={[.18, .38, height * .6, 8]} /><meshStandardMaterial color="#655546" /></mesh>
    {[0, 1, 2, 3].map((index) => <mesh key={index} position={[Math.sin(seed + index * 2.4) * height * .13, height * (.65 + index * .035), Math.cos(seed + index * 2.4) * height * .13]} scale={[1, 1.2, 1]} castShadow>
      <icosahedronGeometry args={[height * .23, 2]} /><meshStandardMaterial color={['#52704a', '#647b4e', '#718554', '#466440'][index]} roughness={1} />
    </mesh>)}
  </group>;
}

/* Structural members are merged into one mesh to keep the landmark inexpensive. */
function towerGeometry() {
  const pieces: THREE.BufferGeometry[] = [];
  const up = new THREE.Vector3(0, 1, 0);
  const beam = (from: WorldPoint, to: WorldPoint, radius: number) => {
    const a = new THREE.Vector3(...from);
    const b = new THREE.Vector3(...to);
    const direction = b.clone().sub(a);
    const geometry = new THREE.CylinderGeometry(radius, radius, direction.length(), 5);
    geometry.applyQuaternion(new THREE.Quaternion().setFromUnitVectors(up, direction.clone().normalize()));
    geometry.translate(...a.add(b).multiplyScalar(.5).toArray());
    pieces.push(geometry);
  };
  const levels = [[0, 62.5], [18, 54], [38, 43], [57, 34], [82, 26], [116, 20], [150, 15], [185, 11], [220, 7.8], [255, 5], [276, 4], [300, 1.7]];
  const corners = [[-1, -1], [1, -1], [1, 1], [-1, 1]];
  for (let level = 0; level < levels.length - 1; level++) {
    const [y0, width0] = levels[level];
    const [y1, width1] = levels[level + 1];
    corners.forEach(([x, z], side) => {
      const [nx, nz] = corners[(side + 1) % 4];
      const a: WorldPoint = [x * width0, y0, z * width0];
      const b: WorldPoint = [x * width1, y1, z * width1];
      beam(a, b, level < 4 ? 1.05 : .55);
      if (level >= 3) {
        beam(a, [nx * width1, y1, nz * width1], .32);
        beam([nx * width0, y0, nz * width0], b, .32);
        beam(b, [nx * width1, y1, nz * width1], .48);
      } else {
        const spread = 5.5 - level;
        beam([a[0] + x * spread, y0, a[2]], [b[0] - x * spread, y1, b[2]], .42);
        beam([a[0] - x * spread, y0, a[2]], [b[0] + x * spread, y1, b[2]], .42);
        beam([a[0], y0, a[2] + z * spread], [b[0], y1, b[2] - z * spread], .42);
      }
    });
  }
  corners.forEach(([x, z], side) => {
    const [nx, nz] = corners[(side + 1) % 4];
    for (let segment = 0; segment < 20; segment++) {
      const point = (t: number): WorldPoint => [(x + (nx - x) * t) * 47, 15 + Math.sin(Math.PI * t) * 30, (z + (nz - z) * t) * 47];
      beam(point(segment / 20), point((segment + 1) / 20), .8);
    }
  });
  beam([0, 300, 0], [0, WORLD.towerHeight, 0], .42);
  const merged = mergeGeometries(pieces);
  pieces.forEach((geometry) => geometry.dispose());
  return merged;
}

function EiffelTower() {
  const geometry = useMemo(() => towerGeometry(), []);
  return <group position={WORLD.towerPosition}>
    <mesh geometry={geometry} castShadow receiveShadow><meshStandardMaterial color="#827365" metalness={.65} roughness={.52} /></mesh>
    {[[57, 72, 3], [116, 44, 3], [276, 12, 3]].map(([height, width, depth]) => <group key={height}>
      <Box position={[0, height - depth / 2, 0]} size={[width, depth, width]} color="#75695d" />
      <Box position={[0, height + .5, 0]} size={[width + 1, 1, width + 1]} color="#9d8c77" />
    </group>)}
    {[-1, 1].flatMap((x) => [-1, 1].map((z) => <Box key={`${x}-${z}`} position={[x * 62.5, 1, z * 62.5]} size={[15, 2, 15]} color="#aba797" />))}
  </group>;
}

export function FlightEnvironment() {
  return <>
    <color attach="background" args={['#b8d2df']} />
    <fog attach="fog" args={['#b8d2df', 1800, 14000]} />
    <mesh rotation={[-Math.PI / 2, 0, 0]} position={[0, -.03, 0]} receiveShadow><planeGeometry args={[WORLD.groundExtent * 2, WORLD.groundExtent * 2]} /><meshStandardMaterial color="#7d8b65" roughness={1} /></mesh>
    <Box position={[-180, -.015, -210]} size={[160, .02, 160]} color="#c5bfaa" />
    <Box position={[20, -.01, 0]} size={[18, .02, 800]} color="#686f70" />
    <Box position={[8.8, .02, 0]} size={[3, .08, 800]} color="#b6b3a5" />
    <Box position={[31.2, .02, 0]} size={[3, .08, 800]} color="#b6b3a5" />
    {Array.from({ length: 60 }, (_, i) => <Box key={i} position={[20, .015, i * 13 - 390]} size={[.16, .02, 5]} color="#e2daca" />)}
    <mesh position={[0, .015, 0]} rotation={[-Math.PI / 2, 0, 0]} receiveShadow><circleGeometry args={[5, 64]} /><meshStandardMaterial color="#465960" roughness={.9} /></mesh>
    <mesh position={[0, .025, 0]} rotation={[-Math.PI / 2, 0, 0]}><ringGeometry args={[3.8, 4, 64]} /><meshBasicMaterial color="#e6dfb9" /></mesh>
    <Box position={[-.85, .035, 0]} size={[.25, .02, 2.5]} color="#eee6cb" />
    <Box position={[.85, .035, 0]} size={[.25, .02, 2.5]} color="#eee6cb" />
    <Box position={[0, .035, 0]} size={[1.7, .02, .25]} color="#eee6cb" />
    <Box position={[0, WORLD.launchDeckHeight - .12, 0]} size={[2.4, .24, 2.4]} color="#465960" />
    {[-.9, .9].flatMap((x) => [-.9, .9].map((z) => <Box key={`launch-${x}-${z}`} position={[x, .48, z]} size={[.16, .96, .16]} color="#8b989b" />))}
    <Box position={[0, WORLD.launchDeckHeight + .008, 1.08]} size={[2.4, .016, .18]} color="#e2bd58" />
    <Box position={[0, WORLD.launchDeckHeight + .008, -1.08]} size={[2.4, .016, .18]} color="#e2bd58" />
    <House position={[-23, 0, -20]} />
    <House position={[50, 0, -45]} rotation={Math.PI} />
    <House position={[50, 0, 15]} rotation={Math.PI} />
    <House position={[-25, 0, 45]} />
    {Array.from({ length: 38 }, (_, i) => <Tree key={i} position={[i % 2 === 0 ? -45 - Math.sin(i * 7) * 9 : 73 + Math.sin(i * 7) * 9, 0, -110 + i * 9]} height={9 + (i * 7 % 8)} seed={i} />)}
    <EiffelTower />
  </>;
}
