'use client';

import { useMemo, useRef } from 'react';
import { Canvas, useFrame } from '@react-three/fiber';
import { Environment, OrbitControls, Text, useGLTF } from '@react-three/drei';
import * as THREE from 'three';
import type { ComponentState, TwinSnapshot } from '@/lib/twin-data';

const degToRad = (deg: number) => (deg * Math.PI) / 180;
export const AIRFRAME_DEFAULT_ROTATION: [number, number, number] = [degToRad(0), degToRad(-60), degToRad(90)];
export const STM32_DEFAULT_ROTATION: [number, number, number] = [degToRad(65), degToRad(15), degToRad(-30)];
export const SHOW_AIRFRAME_AXES = false;
export const AIRFRAME_AXES_SIZE = 2.8;
export const AIRFRAME_MODEL_SCALE = 1.3;

const AIRFRAME_DEFAULT_POSITION: [number, number, number] = [0, .1, 0];
const STM32_DEFAULT_POSITION: [number, number, number] = [0, .02, 0];

const stateColor = (state: ComponentState) => state === 'HEALTHY' ? '#32d296' : state === 'DEGRADED' ? '#f1b84b' : state === 'FAILED' ? '#ff5454' : '#71808b';

function brightenAirframeMaterial(source: THREE.Material) {
  const material = source.clone();
  if (material instanceof THREE.MeshStandardMaterial) {
    material.color.lerp(new THREE.Color('#d8eef2'), .38);
    material.emissive.copy(material.color);
    material.emissiveIntensity = .12;
    material.envMapIntensity = 1.8;
    material.roughness = Math.min(material.roughness, .68);
  }
  return material;
}

function Asset({ path, position, scale, rotation, brighten = false }: { path: string; position: [number, number, number]; scale: number; rotation: [number, number, number]; brighten?: boolean }) {
  const { scene } = useGLTF(path);
  const model = useMemo(() => {
    const clone = scene.clone();
    clone.traverse((object) => {
      if (!(object instanceof THREE.Mesh)) return;
      object.castShadow = true;
      object.receiveShadow = true;
      object.material = Array.isArray(object.material)
        ? object.material.map((material) => brighten ? brightenAirframeMaterial(material) : material.clone())
        : brighten ? brightenAirframeMaterial(object.material) : object.material.clone();
    });
    return clone;
  }, [brighten, scene]);
  return <primitive object={model} position={position} scale={scale} rotation={rotation} />;
}

function FaultAwareAirframe({ snapshot }: { snapshot: TwinSnapshot }) {
  const { scene } = useGLTF('/models/drone-done.glb');
  const wingState = snapshot.active_fault === 'ACTUATOR_DEGRADED'
    ? snapshot.fc1.components.actuators ?? 'DEGRADED'
    : 'HEALTHY';
  const airframe = useMemo(() => {
    const clone = scene.clone();
    const materials: THREE.MeshStandardMaterial[] = [];
    clone.traverse((object) => {
      if (!(object instanceof THREE.Mesh)) return;
      object.castShadow = true;
      object.receiveShadow = true;
      const isWing = object.name === 'helice_droite' || object.name === 'helice_gauche';
      const clonedMaterials = (Array.isArray(object.material) ? object.material : [object.material])
        .map((material) => brightenAirframeMaterial(material));
      object.material = Array.isArray(object.material) ? clonedMaterials : clonedMaterials[0];
      if (isWing) {
        clonedMaterials.forEach((material) => {
          if (material instanceof THREE.MeshStandardMaterial) materials.push(material);
        });
      }
    });
    return { model: clone, wingMaterials: materials };
  }, [scene]);

  useFrame(({ clock }) => {
    const active = wingState === 'DEGRADED' || wingState === 'FAILED';
    const color = new THREE.Color(stateColor(wingState));
    const pulse = (Math.sin(clock.elapsedTime * 8) + 1) * .5;
    airframe.wingMaterials.forEach((material) => {
      if (active) {
        material.color.copy(color);
        material.emissive.copy(color);
        material.emissiveIntensity = .65 + pulse * 1.35;
        return;
      }
      material.color.lerp(new THREE.Color('#d8eef2'), .12);
      material.emissive.copy(material.color);
      material.emissiveIntensity = .12;
    });
  });

  return <primitive object={airframe.model} position={AIRFRAME_DEFAULT_POSITION} scale={AIRFRAME_MODEL_SCALE} rotation={AIRFRAME_DEFAULT_ROTATION} />;
}

function FaultMarker({ label, state, position, showLabel = true }: { label: string; state: ComponentState; position: [number, number, number]; showLabel?: boolean }) {
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
      {showLabel && <Text position={[0, .22, 0]} rotation={[-Math.PI / 2, 0, 0]} fontSize={.13} color="#eafffb" anchorX="center">{label}</Text>}
    </group>
  );
}

function SceneLighting() {
  return <><ambientLight intensity={1.8} /><hemisphereLight args={['#d9fbff', '#142c2a', 1.6]} /><directionalLight position={[3, 8, 5]} intensity={4} castShadow /><Environment preset="city" environmentIntensity={1.2} /></>;
}

function AirframePanel({ snapshot }: { snapshot: TwinSnapshot }) {
  return (
    <div style={{ minWidth: 0, position: 'relative', border: '1px solid #31505b', background: '#18313b' }}>
      <div style={{ position: 'absolute', zIndex: 2, left: 10, top: 8, font: '700 9px var(--font-geist-mono)', letterSpacing: '.1em', color: '#88a4ae' }}>HELIBLADE · AIRFRAME</div>
      <Canvas camera={{ position: [4.8, 4.2, 6.2], fov: 38 }} shadows>
        <color attach="background" args={['#18313b']} />
        <SceneLighting />
        <directionalLight position={[-5, 4, 7]} intensity={5.5} color="#e8fbff" />
        <directionalLight position={[5, 1, -4]} intensity={3.2} color="#9edbe5" />
        {SHOW_AIRFRAME_AXES && <axesHelper args={[AIRFRAME_AXES_SIZE]} />}
        <FaultAwareAirframe snapshot={snapshot} />
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
