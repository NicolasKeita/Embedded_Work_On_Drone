'use client';

import { useMemo, useRef } from 'react';
import { useFrame } from '@react-three/fiber';
import { useGLTF } from '@react-three/drei';
import * as THREE from 'three';
import type { ComponentState } from '@/lib/twin-data';

const degToRad = (deg: number) => (deg * Math.PI) / 180;
const MODEL_POSITION: [number, number, number] = [0, .02, 0];
const MODEL_ROTATION: [number, number, number] = [degToRad(90), degToRad(180), degToRad(180)];
const MODEL_SCALE = 4.1;

function isFaulted(state: ComponentState) {
  return state === 'DEGRADED' || state === 'FAILED';
}

function faultColor(state: ComponentState) {
  return state === 'FAILED' ? '#ff5454' : '#f1b84b';
}

function findMesh(root: THREE.Object3D, names: string[]) {
  let match: THREE.Mesh | undefined;
  root.traverse((object) => {
    if (match || !('isMesh' in object) || object.isMesh !== true) return;
    const mesh = object as THREE.Mesh;
    if (names.includes(mesh.name) || names.includes(mesh.geometry.name)) match = mesh;
  });
  return match;
}

function cloneInSceneSpace(source: THREE.Mesh) {
  const clone = source.clone(false);
  const materials = (Array.isArray(source.material) ? source.material : [source.material])
    .map((material) => material.clone());
  clone.material = Array.isArray(source.material) ? materials : materials[0];
  source.matrixWorld.decompose(clone.position, clone.quaternion, clone.scale);
  clone.castShadow = true;
  clone.receiveShadow = true;
  return clone;
}

function BlinkingFaultMesh({ mesh, state }: { mesh: THREE.Mesh; state: ComponentState }) {
  const meshRef = useRef<THREE.Mesh>(null);

  useFrame(({ clock }) => {
    const renderedMesh = meshRef.current;
    if (!renderedMesh) return;
    const pulse = (Math.sin(clock.elapsedTime * 8) + 1) * .5;
    const visible = Math.sin(clock.elapsedTime * 8) >= 0;
    const color = new THREE.Color(faultColor(state));
    renderedMesh.visible = visible;
    const materials = Array.isArray(renderedMesh.material) ? renderedMesh.material : [renderedMesh.material];
    materials.forEach((material) => {
      if (!(material instanceof THREE.MeshStandardMaterial)) return;
      material.color.copy(color);
      material.emissive.copy(color);
      material.emissiveIntensity = .65 + pulse * 1.6;
    });
  });

  return <primitive ref={meshRef} object={mesh} />;
}

export function Stm32FaultModel({ components, activeFault }: { components: Record<string, ComponentState>; activeFault?: string | null }) {
  const { scene } = useGLTF('/models/carte_stm32.glb');
  const model = useMemo(() => {
    scene.updateMatrixWorld(true);
    const boardSource = findMesh(scene, ['geometry_0', 'Carte_entière']);
    const mcuSource = findMesh(scene, ['Cube', 'STM32L476RG']);
    const barometerSource = findMesh(scene, ['Cube.001', 'Barometre']);
    return {
      board: boardSource ? cloneInSceneSpace(boardSource) : undefined,
      mcu: mcuSource ? cloneInSceneSpace(mcuSource) : undefined,
      barometer: barometerSource ? cloneInSceneSpace(barometerSource) : undefined,
    };
  }, [scene]);
  const mcuState = components.mcu ?? 'UNKNOWN';
  const reportedBarometerState = components.sensors ?? 'UNKNOWN';
  const barometerState = reportedBarometerState === 'UNKNOWN' && activeFault === 'INVALID_SENSOR_DATA'
    ? 'DEGRADED'
    : reportedBarometerState;

  return (
    <group position={MODEL_POSITION} scale={MODEL_SCALE} rotation={MODEL_ROTATION}>
      {model.board && <primitive object={model.board} />}
      {model.mcu && isFaulted(mcuState) && <BlinkingFaultMesh mesh={model.mcu} state={mcuState} />}
      {model.barometer && isFaulted(barometerState) && <BlinkingFaultMesh mesh={model.barometer} state={barometerState} />}
    </group>
  );
}

useGLTF.preload('/models/carte_stm32.glb');
