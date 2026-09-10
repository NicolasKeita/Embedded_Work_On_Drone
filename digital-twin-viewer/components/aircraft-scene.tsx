'use client';

import { useMemo, useRef } from 'react';
import { Canvas, useFrame, useThree } from '@react-three/fiber';
import { Cloud, Clouds, ContactShadows, Environment, Grid, Line, OrbitControls, useGLTF } from '@react-three/drei';
import * as THREE from 'three';
import type { OrbitControls as OrbitControlsImpl } from 'three-stdlib';
import type { TwinSnapshot } from '@/lib/twin-data';

function displayAltitude(altitude: number, targetAltitude: number) {
  const reference = Math.max(targetAltitude, 1000);
  return 4.5 * Math.log1p(Math.max(altitude, 0) / 250) / Math.log1p(reference / 250);
}

function Drone({ snapshot }: { snapshot: TwinSnapshot }) {
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
    <group ref={rotor} position={[snapshot.aircraft.x_m * .22, .5 + displayAltitude(snapshot.aircraft.altitude_m, snapshot.target.altitude_m), snapshot.aircraft.y_m * .22]}>
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

function EiffelTower({ height }: { height: number }) {
  return (
    <group position={[-5.2, 0, -3.8]}>
      <mesh position={[0, height * .5, 0]} castShadow><coneGeometry args={[height * .22, height, 4, 5, true]} /><meshStandardMaterial color="#766f66" wireframe roughness={.7} /></mesh>
      <mesh position={[0, height * .18, 0]}><boxGeometry args={[height * .34, .035, height * .34]} /><meshStandardMaterial color="#968b7e" /></mesh>
      <mesh position={[0, height * .47, 0]}><boxGeometry args={[height * .2, .025, height * .2]} /><meshStandardMaterial color="#968b7e" /></mesh>
      <mesh position={[0, height * .72, 0]}><boxGeometry args={[height * .1, .02, height * .1]} /><meshStandardMaterial color="#968b7e" /></mesh>
      <mesh position={[0, height * 1.08, 0]}><cylinderGeometry args={[.012, .018, height * .18, 6]} /><meshStandardMaterial color="#b5aaa0" /></mesh>
    </group>
  );
}

function FlightEnvironment({ snapshot }: { snapshot: TwinSnapshot }) {
  const altitude = Math.max(0, snapshot.aircraft.altitude_m);
  const groundOpacity = THREE.MathUtils.clamp(1 - altitude / 3500, 0, 1);
  const stratosphereBlend = THREE.MathUtils.smoothstep(altitude, 7000, 20000);
  const cloudHeight = .25 + displayAltitude(7000, snapshot.target.altitude_m);
  const towerHeight = displayAltitude(330, snapshot.target.altitude_m);
  const skyColor = new THREE.Color('#79b9d3').lerp(new THREE.Color('#101b36'), stratosphereBlend);

  return (
    <>
      <color attach="background" args={[skyColor]} />
      <fog attach="fog" args={[skyColor, 17, 34]} />
      <group visible={groundOpacity > .01}>
        <mesh position={[0, -.16, 0]} receiveShadow><cylinderGeometry args={[18, 18, .3, 64]} /><meshStandardMaterial color="#315a42" roughness={1} transparent opacity={groundOpacity} /></mesh>
        <Grid position={[0, .01, 0]} args={[36, 36]} cellColor="#567c66" sectionColor="#829b80" fadeDistance={25} infiniteGrid />
        <LaunchSite opacity={groundOpacity} />
        <EiffelTower height={towerHeight} />
      </group>
      <Clouds position={[0, cloudHeight, 0]} material={THREE.MeshLambertMaterial} limit={500} visible={altitude > 4500}>
        <Cloud seed={2} segments={55} bounds={[7, .45, 4]} volume={3.5} opacity={.72} color="#edf5f7" position={[-4, 0, -3]} />
        <Cloud seed={7} segments={65} bounds={[8, .55, 5]} volume={4} opacity={.7} color="#dce9ed" position={[4.5, -.18, -4]} />
        <Cloud seed={11} segments={70} bounds={[10, .5, 6]} volume={4.5} opacity={.68} color="#e8f1f3" position={[0, -.35, 4.5]} />
      </Clouds>
    </>
  );
}

function CameraTracker({ snapshot }: { snapshot: TwinSnapshot }) {
  const { camera } = useThree();
  const controls = useRef<OrbitControlsImpl>(null);
  const target = useMemo(() => new THREE.Vector3(), []);
  const movement = useMemo(() => new THREE.Vector3(), []);

  useFrame(() => {
    if (!controls.current) return;
    target.set(snapshot.aircraft.x_m * .22, .5 + displayAltitude(snapshot.aircraft.altitude_m, snapshot.target.altitude_m), snapshot.aircraft.y_m * .22);
    movement.copy(target).sub(controls.current.target).multiplyScalar(.08);
    camera.position.add(movement);
    controls.current.target.add(movement);
    controls.current.update();
  });

  return <OrbitControls ref={controls} enablePan={false} minDistance={6} maxDistance={18} maxPolarAngle={Math.PI * .86} />;
}

export function AircraftScene({ snapshot, trail }: { snapshot: TwinSnapshot; trail: TwinSnapshot[] }) {
  const points = trail.map((item) => [item.aircraft.x_m * .22, .5 + displayAltitude(item.aircraft.altitude_m, snapshot.target.altitude_m), item.aircraft.y_m * .22] as [number, number, number]);

  return (
    <div className="scene-canvas">
      <Canvas camera={{ position: [8, 6, 10], fov: 40 }} shadows>
        <FlightEnvironment snapshot={snapshot} />
        <CameraTracker snapshot={snapshot} />
        <ambientLight intensity={1.9} />
        <hemisphereLight args={['#e4fdff', '#18382d', 1.8]} />
        <directionalLight position={[5, 9, 6]} color="#fff7e7" intensity={5.2} castShadow />
        <directionalLight position={[-6, 4, -5]} color="#55eaff" intensity={3.5} />
        <pointLight position={[-5, 5, 4]} color="#7ff5ff" intensity={36} distance={18} />
        <Drone snapshot={snapshot} />
        <ContactShadows position={[0, .02, 0]} opacity={.6} scale={9} blur={2.4} far={8} />
        <mesh position={[snapshot.target.x_m * .22, .03, snapshot.target.y_m * .22]} rotation={[-Math.PI / 2, 0, 0]}>
          <ringGeometry args={[1.5, 1.58, 64]} />
          <meshBasicMaterial color="#27d7ca" transparent opacity={.75} />
        </mesh>
        {points.length > 1 && <Line points={points} color="#3ff1dd" lineWidth={1.5} transparent opacity={.75} />}
        <Environment preset="city" environmentIntensity={1.6} />
      </Canvas>
      <div className="scene-label altitude-label"><b>{snapshot.aircraft.altitude_m.toFixed(1)} m</b><small>MSL ALTITUDE</small></div>
      {snapshot.aircraft.altitude_m > 7000 && <div className="scene-label" style={{ left: 18, top: 18, color: '#8bc8e5' }}>STRATOSPHERIC ASCENT · {(snapshot.aircraft.altitude_m / 1000).toFixed(1)} km</div>}
    </div>
  );
}

useGLTF.preload('/models/drone.glb');
