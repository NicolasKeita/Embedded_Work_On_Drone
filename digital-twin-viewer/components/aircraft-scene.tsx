'use client';
import { Canvas } from '@react-three/fiber';
import { Environment, Grid, Line, OrbitControls, useGLTF } from '@react-three/drei';
import type { TwinSnapshot } from '@/lib/twin-data';

function Drone({ snapshot }: { snapshot: TwinSnapshot }) {
  const { scene } = useGLTF('/models/drone.glb');
  return <primitive object={scene.clone()} scale={1.8} rotation={[snapshot.aircraft.roll_rad, -snapshot.aircraft.pitch_rad, -0.35]} position={[0, 1.6, 0]} />;
}
export function AircraftScene({ snapshot, trail }: { snapshot: TwinSnapshot; trail: TwinSnapshot[] }) {
  const points = trail.map((item) => [(item.aircraft.x_m - snapshot.aircraft.x_m) * 0.16, 1.6 + (item.aircraft.altitude_m - snapshot.aircraft.altitude_m) * 0.08, (item.aircraft.y_m - snapshot.aircraft.y_m) * 0.16] as [number, number, number]);
  return <div className="scene-canvas"><Canvas camera={{ position: [8, 5.5, 9], fov: 40 }} shadows><color attach="background" args={['#081119']} /><fog attach="fog" args={['#081119', 14, 28]} /><ambientLight intensity={0.9} /><directionalLight position={[5, 9, 6]} intensity={2.5} castShadow /><Drone snapshot={snapshot} /><Grid position={[0, 0, 0]} args={[30, 30]} cellColor="#1b4c55" sectionColor="#237e83" fadeDistance={22} infiniteGrid /><mesh position={[(snapshot.target.x_m - snapshot.aircraft.x_m) * .16, .03, (snapshot.target.y_m - snapshot.aircraft.y_m) * .16]} rotation={[-Math.PI / 2, 0, 0]}><ringGeometry args={[1.5, 1.58, 64]} /><meshBasicMaterial color="#27d7ca" transparent opacity={.75} /></mesh>{points.length > 1 && <Line points={points} color="#3ff1dd" lineWidth={1.5} transparent opacity={.75} />}<Environment preset="city" /><OrbitControls enablePan={false} minDistance={7} maxDistance={17} maxPolarAngle={Math.PI / 2.05} /></Canvas><div className="scene-label target-label"><span />STATION ZONE</div><div className="scene-label altitude-label"><b>{snapshot.aircraft.altitude_m.toFixed(1)} m</b><small>MSL ALTITUDE</small></div></div>;
}
useGLTF.preload('/models/drone.glb');
