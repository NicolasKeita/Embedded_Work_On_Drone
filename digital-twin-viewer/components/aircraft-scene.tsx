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

  return (
    <group ref={rotor} position={[snapshot.aircraft.x_m * .22, .5 + snapshot.aircraft.altitude_m * altitudeScale, snapshot.aircraft.y_m * .22]}>
      <primitive object={model} scale={7.2} rotation={[snapshot.aircraft.roll_rad, -snapshot.aircraft.pitch_rad, -0.35]} />
    </group>
  );
}

function Tree({ position, scale = 1 }: { position: [number, number, number]; scale?: number }) {
  return (
    <group position={position} scale={scale}>
      <mesh position={[0, .42, 0]} castShadow><cylinderGeometry args={[.09, .13, .84, 8]} /><meshStandardMaterial color="#62442b" roughness={1} /></mesh>
      <mesh position={[0, 1.05, 0]} castShadow><coneGeometry args={[.55, 1.45, 9]} /><meshStandardMaterial color="#245742" roughness={.9} /></mesh>
      <mesh position={[0, 1.6, 0]} castShadow><coneGeometry args={[.4, 1.05, 9]} /><meshStandardMaterial color="#307257" roughness={.9} /></mesh>
    </group>
  );
}

function LaunchSite({ opacity }: { opacity: number }) {
  return (
    <group visible={opacity > .01}>
      <mesh position={[0, -.08, 0]} receiveShadow><cylinderGeometry args={[3.1, 3.1, .14, 64]} /><meshStandardMaterial color="#5a6870" roughness={.86} transparent opacity={opacity} /></mesh>
      <mesh position={[0, .012, 0]} rotation={[-Math.PI / 2, 0, 0]}><ringGeometry args={[1.55, 1.66, 64]} /><meshBasicMaterial color="#d7eef0" transparent opacity={opacity * .85} /></mesh>
      <group position={[4.1, .64, -2.5]}>
        <mesh castShadow receiveShadow><boxGeometry args={[2.6, 1.25, 1.8]} /><meshStandardMaterial color="#aeb8ba" roughness={.72} transparent opacity={opacity} /></mesh>
        <mesh position={[0, .76, 0]} rotation={[0, 0, .12]} castShadow><boxGeometry args={[2.85, .12, 2]} /><meshStandardMaterial color="#3c515a" roughness={.62} transparent opacity={opacity} /></mesh>
        <mesh position={[-.65, 0, .91]}><planeGeometry args={[.62, .7]} /><meshStandardMaterial color="#173c4d" emissive="#1b6179" emissiveIntensity={.35} transparent opacity={opacity} /></mesh>
        <mesh position={[.55, -.2, .91]}><planeGeometry args={[.65, .85]} /><meshStandardMaterial color="#313b3f" transparent opacity={opacity} /></mesh>
      </group>
      <Tree position={[-4.5, 0, -2.2]} scale={1.15} />
      <Tree position={[-3.6, 0, 3.1]} scale={.78} />
      <Tree position={[5.4, 0, 2.5]} scale={.92} />
      <Tree position={[3.6, 0, 4.3]} scale={.65} />
    </group>
  );
}

function Cloud({ position, scale }: { position: [number, number, number]; scale: number }) {
  const lobes = [[0, 0, 0], [.55, .08, .05], [-.55, .02, .08], [.1, .18, -.28], [-.15, .12, .32]];
  return (
    <group position={position} scale={scale}>
      {lobes.map((offset, index) => <mesh key={index} position={offset as [number, number, number]}><sphereGeometry args={[.62, 16, 10]} /><meshStandardMaterial color="#e8f2f3" emissive="#6f8e96" emissiveIntensity={.25} transparent opacity={.68} roughness={1} depthWrite={false} /></mesh>)}
    </group>
  );
}

function FlightEnvironment({ snapshot, altitudeScale }: { snapshot: TwinSnapshot; altitudeScale: number }) {
  const altitude = Math.max(0, snapshot.aircraft.altitude_m);
  const groundOpacity = THREE.MathUtils.clamp(1 - altitude / 3500, 0, 1);
  const spaceBlend = THREE.MathUtils.smoothstep(altitude, 7000, 18000);
  const cloudHeight = Math.min(2000 * altitudeScale + .25, 2.4);
  const skyColor = new THREE.Color('#102a38').lerp(new THREE.Color('#02050d'), spaceBlend);

  return (
    <>
      <color attach="background" args={[skyColor]} />
      <fog attach="fog" args={[skyColor, 17, 34]} />
      <mesh position={[0, -18.05, 0]} visible={spaceBlend > .02}>
        <sphereGeometry args={[18, 96, 48]} />
        <meshStandardMaterial color="#174c69" emissive="#0a2238" emissiveIntensity={.7} roughness={1} transparent opacity={spaceBlend * .98} />
      </mesh>
      <mesh position={[0, -17.94, 0]} visible={spaceBlend > .05}>
        <sphereGeometry args={[18.06, 96, 48]} />
        <meshBasicMaterial color="#4aa4d1" transparent opacity={spaceBlend * .18} side={THREE.BackSide} />
      </mesh>
      <group visible={groundOpacity > .01}>
        <mesh position={[0, -.16, 0]} receiveShadow><cylinderGeometry args={[18, 18, .3, 64]} /><meshStandardMaterial color="#315a42" roughness={1} transparent opacity={groundOpacity} /></mesh>
        <Grid position={[0, .01, 0]} args={[36, 36]} cellColor="#567c66" sectionColor="#829b80" fadeDistance={25} infiniteGrid />
        <LaunchSite opacity={groundOpacity} />
      </group>
      <group position={[0, cloudHeight, 0]} visible={altitude > 350}>
        <Cloud position={[-4.5, 0, -2]} scale={1.4} />
        <Cloud position={[3.8, -.25, -4.4]} scale={1.8} />
        <Cloud position={[5.2, .1, 2.8]} scale={1.2} />
        <Cloud position={[-3.2, -.2, 4.7]} scale={1.7} />
        <Cloud position={[.4, -.45, -6.5]} scale={2.1} />
      </group>
    </>
  );
}

export function AircraftScene({ snapshot, trail }: { snapshot: TwinSnapshot; trail: TwinSnapshot[] }) {
  const altitudeScale = 4 / Math.max(snapshot.target.altitude_m, 1);
  const points = trail.map((item) => [item.aircraft.x_m * .22, .5 + item.aircraft.altitude_m * altitudeScale, item.aircraft.y_m * .22] as [number, number, number]);

  return (
    <div className="scene-canvas">
      <Canvas camera={{ position: [8, 6, 10], fov: 40 }} shadows>
        <FlightEnvironment snapshot={snapshot} altitudeScale={altitudeScale} />
        <ambientLight intensity={1.9} />
        <hemisphereLight args={['#e4fdff', '#18382d', 1.8]} />
        <directionalLight position={[5, 9, 6]} color="#fff7e7" intensity={5.2} castShadow />
        <directionalLight position={[-6, 4, -5]} color="#55eaff" intensity={3.5} />
        <pointLight position={[-5, 5, 4]} color="#7ff5ff" intensity={36} distance={18} />
        <Drone snapshot={snapshot} altitudeScale={altitudeScale} />
        <ContactShadows position={[0, .02, 0]} opacity={.6} scale={9} blur={2.4} far={8} />
        <mesh position={[snapshot.target.x_m * .22, .03, snapshot.target.y_m * .22]} rotation={[-Math.PI / 2, 0, 0]}>
          <ringGeometry args={[1.5, 1.58, 64]} />
          <meshBasicMaterial color="#27d7ca" transparent opacity={.75} />
        </mesh>
        {points.length > 1 && <Line points={points} color="#3ff1dd" lineWidth={1.5} transparent opacity={.75} />}
        <Environment preset="city" environmentIntensity={1.6} />
        <OrbitControls enablePan={false} minDistance={7} maxDistance={17} maxPolarAngle={Math.PI / 2.05} />
      </Canvas>
      <div className="scene-label altitude-label"><b>{snapshot.aircraft.altitude_m.toFixed(1)} m</b><small>MSL ALTITUDE</small></div>
      {snapshot.aircraft.altitude_m > 7000 && <div className="scene-label" style={{ left: 18, top: 18, color: '#8bc8e5' }}>STRATOSPHERIC ASCENT · {(snapshot.aircraft.altitude_m / 1000).toFixed(1)} km</div>}
    </div>
  );
}

useGLTF.preload('/models/drone.glb');
