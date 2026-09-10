'use client';

import { useMemo, useRef } from 'react';
import { Canvas, useFrame } from '@react-three/fiber';
import { Environment, OrbitControls, Text, useGLTF } from '@react-three/drei';
import * as THREE from 'three';
import type { ComponentState, TwinSnapshot } from '@/lib/twin-data';

const degToRad = (deg: number) => (deg * Math.PI) / 180;
export const AIRFRAME_DEFAULT_ROTATION: [number, number, number] = [degToRad(70), degToRad(100), degToRad(100)];
export const STM32_DEFAULT_ROTATION: [number, number, number] = [degToRad(65), degToRad(15), degToRad(-30)];

const AIRFRAME_DEFAULT_POSITION: [number, number, number] = [0, .1, 0];
const STM32_DEFAULT_POSITION: [number, number, number] = [0, .02, 0];

const stateColor = (state: ComponentState) => state === 'HEALTHY' ? '#32d296' : state === 'DEGRADED' ? '#f1b84b' : state === 'FAILED' ? '#ff5454' : '#71808b';

function Asset({ path, position, scale, rotation }: { path: string; position: [number, number, number]; scale: number; rotation: [number, number, number] }) {
  const { scene } = useGLTF(path);
  const model = useMemo(() => {
    const clone = scene.clone();
    clone.traverse((object) => {
      if (!(object instanceof THREE.Mesh)) return;
      object.castShadow = true;
      object.receiveShadow = true;
      object.material = (object.material as THREE.Material).clone();
    });
    return clone;
  }, [scene]);
  return <primitive object={model} position={position} scale={scale} rotation={rotation} />;
}

function FaultMarker({ label, state, position }: { label: string; state: ComponentState; position: [number, number, number] }) {
  const material = useRef<THREE.MeshStandardMaterial>(null);
  useFrame(({ clock }) => {
    if (!material.current) return;
    const active = state === 'DEGRADED' || state === 'FAILED';
    material.current.opacity = active ? .35 + (Math.sin(clock.elapsedTime * 8) + 1) * .3 : .22;
    material.current.emissiveIntensity = active ? 1.3 + Math.sin(clock.elapsedTime * 8) * .7 : .18;
  });
  return (
    <group position={position}>
      <mesh position={[0, .08, 0]}><cylinderGeometry args={[.24, .24, .08, 24]} /><meshStandardMaterial ref={material} color={stateColor(state)} emissive={stateColor(state)} transparent depthWrite={false} /></mesh>
      <Text position={[0, .22, 0]} rotation={[-Math.PI / 2, 0, 0]} fontSize={.13} color="#eafffb" anchorX="center">{label}</Text>
    </group>
  );
}

function SceneLighting() {
  return <><ambientLight intensity={1.8} /><hemisphereLight args={['#d9fbff', '#142c2a', 1.6]} /><directionalLight position={[3, 8, 5]} intensity={4} castShadow /><Environment preset="city" environmentIntensity={1.2} /></>;
}

function AirframePanel({ snapshot }: { snapshot: TwinSnapshot }) {
  const actuatorState = snapshot.fc1.components.actuators ?? 'UNKNOWN';
  const controlState = snapshot.fc1.components.control ?? 'UNKNOWN';
  return (
    <div style={{ minWidth: 0, position: 'relative', border: '1px solid #1b3039', background: '#09131a' }}>
      <div style={{ position: 'absolute', zIndex: 2, left: 10, top: 8, font: '700 9px var(--font-geist-mono)', letterSpacing: '.1em', color: '#88a4ae' }}>HELIBLADE · AIRFRAME</div>
      <Canvas camera={{ position: [4.8, 4.2, 6.2], fov: 38 }} shadows>
        <color attach="background" args={['#09131a']} />
        <SceneLighting />
        <Asset path="/models/drone-done.glb" position={AIRFRAME_DEFAULT_POSITION} scale={.82} rotation={AIRFRAME_DEFAULT_ROTATION} />
        <FaultMarker label="LEFT WING" state={actuatorState} position={[-.55, .55, -.85]} />
        <FaultMarker label="MOTOR" state={controlState} position={[0, .62, 0]} />
        <FaultMarker label="RIGHT WING" state={actuatorState} position={[.55, .55, .85]} />
        <OrbitControls enablePan={false} minDistance={4.5} maxDistance={11} maxPolarAngle={Math.PI * .85} />
      </Canvas>
    </div>
  );
}

function Stm32Panel({ snapshot }: { snapshot: TwinSnapshot }) {
  const components = snapshot.fc1.components;
  return (
    <div style={{ minWidth: 0, position: 'relative', border: '1px solid #1b3039', background: '#09131a' }}>
      <div style={{ position: 'absolute', zIndex: 2, left: 10, top: 8, font: '700 9px var(--font-geist-mono)', letterSpacing: '.1em', color: '#88a4ae' }}>STM32 · FC1</div>
      <Canvas camera={{ position: [3.8, 5.4, 5.2], fov: 37 }} shadows>
        <color attach="background" args={['#09131a']} />
        <SceneLighting />
        <Asset path="/models/stm32.glb" position={STM32_DEFAULT_POSITION} scale={4.1} rotation={STM32_DEFAULT_ROTATION} />
        <FaultMarker label="MCU" state={components.mcu ?? 'UNKNOWN'} position={[0, .48, 0]} />
        <FaultMarker label="COM" state={components.transport ?? 'UNKNOWN'} position={[-.85, .46, -.65]} />
        <FaultMarker label="SENS" state={components.sensors ?? 'UNKNOWN'} position={[.85, .46, -.55]} />
        <FaultMarker label="CTRL" state={components.control ?? 'UNKNOWN'} position={[-.68, .46, .68]} />
        <FaultMarker label="ACT" state={components.actuators ?? 'UNKNOWN'} position={[.72, .46, .65]} />
        <OrbitControls enablePan={false} minDistance={4.5} maxDistance={10} maxPolarAngle={Math.PI * .85} />
      </Canvas>
    </div>
  );
}

export function AvionicsScene({ snapshot }: { snapshot: TwinSnapshot }) {
  return (
    <div className="avionics-canvas" style={{ display: 'grid', gridTemplateColumns: 'minmax(0, 65fr) minmax(0, 35fr)', gap: 8, padding: 8 }}>
      <AirframePanel snapshot={snapshot} />
      <Stm32Panel snapshot={snapshot} />
      <div className="legend"><span><i className="healthy" />Healthy</span><span><i className="degraded" />Degraded / blinking</span><span><i className="failed" />Failed / blinking</span></div>
    </div>
  );
}

useGLTF.preload('/models/drone-done.glb');
useGLTF.preload('/models/stm32.glb');
