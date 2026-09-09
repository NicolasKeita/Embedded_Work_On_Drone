'use client';
import { useMemo } from 'react';
import { Canvas } from '@react-three/fiber';
import { Environment, Grid, Line, OrbitControls, useGLTF } from '@react-three/drei';
import type { TwinSnapshot } from '@/lib/twin-data';

function Drone({ snapshot, altitudeScale }: { snapshot: TwinSnapshot; altitudeScale: number }) {
  const { scene } = useGLTF('/models/drone.glb');
  const model = useMemo(() => scene.clone(), [scene]);
  return <primitive object={model} scale={1.8} rotation={[snapshot.aircraft.roll_rad, -snapshot.aircraft.pitch_rad, -0.35]} position={[snapshot.aircraft.x_m * .22, .35 + snapshot.aircraft.altitude_m * altitudeScale, snapshot.aircraft.y_m * .22]} />;
}
export function AircraftScene({ snapshot, trail }: { snapshot: TwinSnapshot; trail: TwinSnapshot[] }) {
  const altitudeScale = 4 / Math.max(snapshot.target.altitude_m, 1);
  const targetHeight = .35 + snapshot.target.altitude_m * altitudeScale;
  const points = trail.map((item) => [item.aircraft.x_m * .22, .35 + item.aircraft.altitude_m * altitudeScale, item.aircraft.y_m * .22] as [number, number, number]);
  return <div className="scene-canvas"><Canvas camera={{ position: [8, 6, 10], fov: 40 }} shadows><color attach="background" args={['#081119']} /><fog attach="fog" args={['#081119', 15, 30]} /><ambientLight intensity={0.9} /><directionalLight position={[5, 9, 6]} intensity={2.5} castShadow /><Drone snapshot={snapshot} altitudeScale={altitudeScale} /><Grid position={[0, 0, 0]} args={[30, 30]} cellColor="#1b4c55" sectionColor="#237e83" fadeDistance={22} infiniteGrid /><mesh position={[snapshot.target.x_m * .22, .03, snapshot.target.y_m * .22]} rotation={[-Math.PI / 2, 0, 0]}><ringGeometry args={[1.5, 1.58, 64]} /><meshBasicMaterial color="#27d7ca" transparent opacity={.75} /></mesh><Line points={[[-6, targetHeight, 0], [6, targetHeight, 0]]} color="#ff4d57" lineWidth={2} /><Line points={[[-5.8, 0, 0], [-5.8, targetHeight, 0]]} color="#ff4d57" lineWidth={1} transparent opacity={.45} />{points.length > 1 && <Line points={points} color="#3ff1dd" lineWidth={1.5} transparent opacity={.75} />}<Environment preset="city" /><OrbitControls enablePan={false} minDistance={7} maxDistance={17} maxPolarAngle={Math.PI / 2.05} /></Canvas><div className="scene-label target-label red"><span />TARGET ALTITUDE · {snapshot.target.altitude_m.toFixed(1)} m</div><div className="scene-label altitude-label"><b>{snapshot.aircraft.altitude_m.toFixed(1)} m</b><small>MSL ALTITUDE</small></div></div>;
}
useGLTF.preload('/models/drone.glb');
