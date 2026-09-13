'use client';

import { useMemo } from 'react';
import { Canvas, useFrame } from '@react-three/fiber';
import { Environment, OrbitControls, useGLTF } from '@react-three/drei';
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

type FaultMesh = {
  baseColors: THREE.Color[];
  materials: THREE.MeshStandardMaterial[];
  state: ComponentState;
};

function isNamedPart(object: THREE.Object3D, names: string[]) {
  return names.includes(object.name);
}

function FaultAwareStm32({ components }: { components: Record<string, ComponentState> }) {
  const { scene } = useGLTF('/models/carte_stm32.glb');
  const board = useMemo(() => {
    const clone = scene.clone();
    const faultMeshes: FaultMesh[] = [];
    clone.traverse((object) => {
      if (!(object instanceof THREE.Mesh)) return;
      object.castShadow = true;
      object.receiveShadow = true;
      const materials = (Array.isArray(object.material) ? object.material : [object.material])
        .map((material) => material.clone())
        .filter((material): material is THREE.MeshStandardMaterial => material instanceof THREE.MeshStandardMaterial);
      object.material = Array.isArray(object.material) ? materials : materials[0];
      if (isNamedPart(object, ['Cube', 'STM32L476RG'])) {
        faultMeshes.push({ baseColors: materials.map((material) => material.color.clone()), materials, state: components.mcu ?? 'UNKNOWN' });
      }
      if (isNamedPart(object, ['Cube.001', 'Barometre'])) {
        faultMeshes.push({ baseColors: materials.map((material) => material.color.clone()), materials, state: components.sensors ?? 'UNKNOWN' });
      }
    });
    return { model: clone, faultMeshes };
  }, [components.mcu, components.sensors, scene]);

  useFrame(({ clock }) => {
    const pulse = (Math.sin(clock.elapsedTime * 8) + 1) * .5;
    board.faultMeshes.forEach(({ baseColors, materials, state }) => {
      const active = state === 'DEGRADED' || state === 'FAILED';
      const color = new THREE.Color(stateColor(state));
      materials.forEach((material, index) => {
        material.color.copy(active ? color : baseColors[index]);
        material.emissive.copy(active ? color : new THREE.Color('#000000'));
        material.emissiveIntensity = active ? .65 + pulse * 1.6 : 0;
      });
    });
  });

  return <primitive object={board.model} position={STM32_DEFAULT_POSITION} scale={4.1} rotation={STM32_DEFAULT_ROTATION} />;
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

function Stm32Panel({ title, components }: { title: string; components: Record<string, ComponentState> }) {
  return (
    <div style={{ minWidth: 0, position: 'relative', border: '1px solid #1b3039', background: '#09131a' }}>
      <div style={{ position: 'absolute', zIndex: 2, left: 10, top: 8, font: '700 9px var(--font-geist-mono)', letterSpacing: '.1em', color: '#88a4ae' }}>{title}</div>
      <Canvas camera={{ position: [3.8, 5.4, 5.2], fov: 37 }} shadows>
        <color attach="background" args={['#09131a']} />
        <SceneLighting />
        <FaultAwareStm32 components={components} />
        <OrbitControls enablePan={false} minDistance={4.5} maxDistance={10} maxPolarAngle={Math.PI * .85} />
      </Canvas>
    </div>
  );
}

export function AvionicsScene({ snapshot }: { snapshot: TwinSnapshot }) {
  return (
    <div className="avionics-canvas" style={{ display: 'grid', gridTemplateColumns: 'minmax(0, 2fr) minmax(0, 1fr) minmax(0, 1fr)', gap: 8, padding: 8 }}>
      <AirframePanel snapshot={snapshot} />
      <Stm32Panel title="Flight Controller 1" components={snapshot.fc1.components} />
      <Stm32Panel title="Flight Controller 2" components={snapshot.fc2.components} />
      <div className="legend"><span><i className="healthy" />Healthy</span><span><i className="degraded" />Degraded / blinking</span><span><i className="failed" />Failed / blinking</span></div>
    </div>
  );
}

useGLTF.preload('/models/drone-done.glb');
useGLTF.preload('/models/carte_stm32.glb');
