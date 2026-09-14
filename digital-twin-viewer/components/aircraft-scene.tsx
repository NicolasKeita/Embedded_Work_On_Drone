'use client';

import { useMemo, useRef } from 'react';
import { useFrame, useThree } from '@react-three/fiber';
import { Line, OrbitControls, useGLTF } from '@react-three/drei';
import * as THREE from 'three';
import type { OrbitControls as OrbitControlsImpl } from 'three-stdlib';
import { PortalView } from '@/components/portal-view';
import { FlightEnvironment } from '@/components/flight-environment';
import { dronePosition, visualSpinStep, WORLD } from '@/lib/flight-world';
import { hasAltitudeFault, type TwinSnapshot } from '@/lib/twin-data';

function Drone({ snapshot }: { snapshot: TwinSnapshot }) {
  const { scene } = useGLTF('/models/drone.glb');
  const spinner = useRef<THREE.Group>(null);
  const model = useMemo(() => {
    const clone = scene.clone();
    clone.traverse((object) => {
      if (!(object instanceof THREE.Mesh)) return;
      const material = object.material as THREE.MeshStandardMaterial;
      object.material = material.clone();
      const brightMaterial = object.material as THREE.MeshStandardMaterial;
      brightMaterial.color.set('#d8fbff');
      brightMaterial.emissive.set('#3da9b8');
      brightMaterial.emissiveIntensity = .08;
      brightMaterial.roughness = .38;
      brightMaterial.metalness = .12;
      object.castShadow = true;
    });
    const bounds = new THREE.Box3().setFromObject(clone);
    const size = bounds.getSize(new THREE.Vector3());
    const center = bounds.getCenter(new THREE.Vector3());
    const scale = WORLD.droneSpan / Math.max(size.x, size.y, size.z);
    clone.position.copy(center).multiplyScalar(-scale);
    clone.position.y = -bounds.min.y * scale;
    clone.scale.setScalar(scale);
    return clone;
  }, [scene]);


  useFrame((_, delta) => {
    if (!spinner.current) return;
    spinner.current.rotation.y = (spinner.current.rotation.y + visualSpinStep(snapshot.actuators.rotor_rpm, delta)) % (Math.PI * 2);
  });

  return (
    <group position={dronePosition(snapshot.aircraft)} rotation={[snapshot.aircraft.roll_rad, 0, -snapshot.aircraft.pitch_rad]}>
      <group ref={spinner}><primitive object={model} /></group>
    </group>
  );
}

function CameraTracker({ snapshot, domElement }: { snapshot: TwinSnapshot; domElement: HTMLElement | undefined }) {
  const camera = useThree((state) => state.camera);
  const controls = useRef<OrbitControlsImpl>(null);
  const target = useMemo(() => new THREE.Vector3(), []);
  const movement = useMemo(() => new THREE.Vector3(), []);

  useFrame((_, delta) => {
    if (!controls.current) return;
    target.set(...dronePosition(snapshot.aircraft));
    movement.copy(target).sub(controls.current.target).multiplyScalar(1 - Math.exp(-5 * delta));
    camera.position.add(movement);
    controls.current.target.add(movement);
    controls.current.update();
  });

  return <OrbitControls ref={controls} domElement={domElement} enablePan={false} minDistance={3} maxDistance={20000} maxPolarAngle={Math.PI * .49} />;
}

export function AircraftSceneContent({ track, domElement, snapshot, trail }: { track: React.RefObject<HTMLDivElement | null>; domElement: HTMLElement | undefined; snapshot: TwinSnapshot; trail: TwinSnapshot[] }) {
  const points = trail.map((item) => dronePosition(item.aircraft));

  return (
    <PortalView track={track} cameraConfig={{ position: [35, 24, 40], fov: 48, far: 60000 }}>
      <FlightEnvironment />
      <CameraTracker snapshot={snapshot} domElement={domElement} />
      <ambientLight intensity={.45} />
      <hemisphereLight args={['#e4efff', '#63734c', 1.3]} />
      <directionalLight position={[180, 350, 140]} color="#fff0d7" intensity={2.5} castShadow shadow-mapSize={[2048, 2048]} shadow-camera-left={-400} shadow-camera-right={400} shadow-camera-top={400} shadow-camera-bottom={-400} shadow-camera-far={1200} shadow-normalBias={.15} shadow-bias={-.00015} />
      <Drone snapshot={snapshot} />
      <mesh position={[snapshot.target.x_m, .05, snapshot.target.y_m]} rotation={[-Math.PI / 2, 0, 0]}>
        <ringGeometry args={[1.5, 1.58, 64]} />
        <meshBasicMaterial color="#ef5b2a" transparent opacity={.75} />
      </mesh>
      {points.length > 1 && <Line points={points} color="#f26935" lineWidth={1.5} transparent opacity={.75} />}
    </PortalView>
  );
}

export function AircraftSceneOverlay({ snapshot }: { snapshot: TwinSnapshot }) {
  return (
    <>
      <div className="scene-label altitude-label" style={hasAltitudeFault(snapshot) ? { borderColor: '#f1c584', color: '#f1c584' } : undefined}><b style={hasAltitudeFault(snapshot) ? { color: '#f1c584' } : undefined}>{snapshot.aircraft.altitude_m.toFixed(1)} m</b><small>{hasAltitudeFault(snapshot) ? 'LAST KNOWN · BAROMETER FAULT' : 'ALTITUDE · DATUM 0 M'}</small></div>
      <div className="scene-label scene-scale">1 UNIT = 1 M · HOUSE 9.2 M · EIFFEL 330 M</div>
      {snapshot.aircraft.altitude_m > 7000 && <div className="scene-label" style={{ left: 18, top: 48, color: '#8bc8e5' }}>HIGH ALTITUDE · {(snapshot.aircraft.altitude_m / 1000).toFixed(1)} km</div>}
    </>
  );
}

useGLTF.preload('/models/drone.glb');
