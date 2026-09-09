'use client';
import { useMemo, useRef } from 'react';
import { Canvas, useFrame } from '@react-three/fiber';
import { ContactShadows, Environment, Grid, Line, OrbitControls, useGLTF } from '@react-three/drei';
import * as THREE from 'three';
import type { TwinSnapshot } from '@/lib/twin-data';

function Drone({ snapshot, altitudeScale }: { snapshot: TwinSnapshot; altitudeScale: number }) {
  const { scene } = useGLTF('/models/drone.glb');
  const rotor = useRef<THREE.Group>(null);
  const model = useMemo(() => {
    const clone = scene.clone();
    clone.traverse((object) => {
      if (!(object instanceof THREE.Mesh)) return;
      const material = object.material as THREE.MeshStandardMaterial;
      object.material = material.clone();
      const brightMaterial = object.material as THREE.MeshStandardMaterial;
      brightMaterial.color.set('#d8fbff');
      brightMaterial.emissive.set('#3da9b8');
      brightMaterial.emissiveIntensity = 1.15;
      brightMaterial.roughness = .38;
      brightMaterial.metalness = .12;
      object.castShadow = true;
    });
    return clone;
  }, [scene]);
  useFrame((_, delta) => {
    if (!rotor.current) return;
    if (snapshot.actuators.rotor_rpm <= 0) {
      rotor.current.rotation.y = 0;
      return;
    }
    const radiansPerSecond = snapshot.actuators.rotor_rpm * Math.PI * 2 / 60;
    rotor.current.rotation.y += radiansPerSecond * delta * .08;
  });
  return <group ref={rotor} position={[snapshot.aircraft.x_m * .22, .5 + snapshot.aircraft.altitude_m * altitudeScale, snapshot.aircraft.y_m * .22]}><primitive object={model} scale={7.2} rotation={[snapshot.aircraft.roll_rad, -snapshot.aircraft.pitch_rad, -0.35]} /></group>;
}
export function AircraftScene({ snapshot, trail }: { snapshot: TwinSnapshot; trail: TwinSnapshot[] }) {
  const altitudeScale = 4 / Math.max(snapshot.target.altitude_m, 1);
  const points = trail.map((item) => [item.aircraft.x_m * .22, .35 + item.aircraft.altitude_m * altitudeScale, item.aircraft.y_m * .22] as [number, number, number]);
  return <div className="scene-canvas"><Canvas camera={{ position: [8, 6, 10], fov: 40 }} shadows><color attach="background" args={['#0b1922']} /><fog attach="fog" args={['#0b1922', 17, 32]} /><ambientLight intensity={2.4} /><hemisphereLight args={['#e4fdff', '#18382d', 2.2]} /><directionalLight position={[5, 9, 6]} color="#ffffff" intensity={5.5} castShadow /><directionalLight position={[-6, 4, -5]} color="#55eaff" intensity={4} /><pointLight position={[-5, 5, 4]} color="#7ff5ff" intensity={42} distance={18} /><Drone snapshot={snapshot} altitudeScale={altitudeScale} /><Grid position={[0, 0, 0]} args={[30, 30]} cellColor="#1b4c55" sectionColor="#237e83" fadeDistance={22} infiniteGrid /><ContactShadows position={[0, .02, 0]} opacity={.65} scale={9} blur={2.4} far={8} /><mesh position={[snapshot.target.x_m * .22, .03, snapshot.target.y_m * .22]} rotation={[-Math.PI / 2, 0, 0]}><ringGeometry args={[1.5, 1.58, 64]} /><meshBasicMaterial color="#27d7ca" transparent opacity={.75} /></mesh>{points.length > 1 && <Line points={points} color="#3ff1dd" lineWidth={1.5} transparent opacity={.75} />}<Environment preset="city" environmentIntensity={2} /><OrbitControls enablePan={false} minDistance={7} maxDistance={17} maxPolarAngle={Math.PI / 2.05} /></Canvas><div className="scene-label altitude-label"><b>{snapshot.aircraft.altitude_m.toFixed(1)} m</b><small>MSL ALTITUDE</small></div></div>;
}
useGLTF.preload('/models/drone.glb');
