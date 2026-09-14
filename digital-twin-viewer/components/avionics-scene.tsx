'use client';

/* eslint-disable react/react-compiler: oxlint react-compiler false-positives on standard ref={callbackRef} JSX for the fc1/fc2 panels */
import { useMemo } from 'react';
import { useFrame } from '@react-three/fiber';
import { OrbitControls, useGLTF } from '@react-three/drei';
import * as THREE from 'three';
import { PortalView } from '@/components/portal-view';
import { Stm32FaultModel } from '@/components/stm32-fault-model';
import type { ComponentState, TwinSnapshot } from '@/lib/twin-data';

const degToRad = (deg: number) => (deg * Math.PI) / 180;
const AIRFRAME_DEFAULT_ROTATION: [number, number, number] = [degToRad(0), degToRad(-60), degToRad(90)];
const SHOW_AIRFRAME_AXES = false;
const AIRFRAME_AXES_SIZE = 2.8;
const AIRFRAME_MODEL_SCALE = 1.3;

const AIRFRAME_DEFAULT_POSITION: [number, number, number] = [0, .1, 0];

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

function SceneLighting() {
  return <><ambientLight intensity={1.8} /><hemisphereLight args={['#d9fbff', '#142c2a', 1.6]} /><directionalLight position={[3, 8, 5]} intensity={4} castShadow /></>;
}

export type AvionicsPanelRefs = {
  airframe: { ref: React.RefObject<HTMLDivElement | null>; domElement: HTMLDivElement | undefined; callbackRef: (node: HTMLDivElement | null) => void };
  fc1: { ref: React.RefObject<HTMLDivElement | null>; domElement: HTMLDivElement | undefined; callbackRef: (node: HTMLDivElement | null) => void };
  fc2: { ref: React.RefObject<HTMLDivElement | null>; domElement: HTMLDivElement | undefined; callbackRef: (node: HTMLDivElement | null) => void };
};

export function AvionicsSceneContent({ refs, snapshot }: { refs: AvionicsPanelRefs; snapshot: TwinSnapshot }) {
  return (
    <>
      <PortalView track={refs.airframe.ref} cameraConfig={{ position: [4.8, 4.2, 6.2], fov: 38 }} background="#18313b">
        <SceneLighting />
        <directionalLight position={[-5, 4, 7]} intensity={5.5} color="#e8fbff" />
        <directionalLight position={[5, 1, -4]} intensity={3.2} color="#9edbe5" />
        {SHOW_AIRFRAME_AXES && <axesHelper args={[AIRFRAME_AXES_SIZE]} />}
        <FaultAwareAirframe snapshot={snapshot} />
        <OrbitControls domElement={refs.airframe.domElement} enablePan={false} minDistance={4.5} maxDistance={11} maxPolarAngle={Math.PI * .85} />
      </PortalView>
      <PortalView track={refs.fc1.ref} cameraConfig={{ orthographic: true, position: [0, 0, 8], zoom: 48 }} background="#09131a">
        <SceneLighting />
        <Stm32FaultModel components={snapshot.fc1.components} activeFault={snapshot.active_fault} />
        <OrbitControls domElement={refs.fc1.domElement} enablePan={false} minDistance={4.5} maxDistance={10} maxPolarAngle={Math.PI * .85} />
      </PortalView>
      <PortalView track={refs.fc2.ref} cameraConfig={{ orthographic: true, position: [0, 0, 8], zoom: 48 }} background="#09131a">
        <SceneLighting />
        <Stm32FaultModel components={snapshot.fc2.components} />
        <OrbitControls domElement={refs.fc2.domElement} enablePan={false} minDistance={4.5} maxDistance={10} maxPolarAngle={Math.PI * .85} />
      </PortalView>
    </>
  );
}

export function AvionicsSceneDom({ refs }: { refs: AvionicsPanelRefs }) {
  return (
    <div className="avionics-canvas" style={{ display: 'grid', gridTemplateColumns: 'minmax(0, 2fr) minmax(0, 1fr) minmax(0, 1fr)', gap: 8, padding: 8 }}>
      <div ref={refs.airframe.callbackRef} style={{ minWidth: 0, position: 'relative', zIndex: 2, border: '1px solid #31505b', background: 'transparent', pointerEvents: 'auto' }}>
        <div style={{ position: 'absolute', zIndex: 2, left: 10, top: 8, font: '700 9px var(--font-geist-mono)', letterSpacing: '.1em', color: '#88a4ae', pointerEvents: 'none' }}>HELIBLADE · AIRFRAME</div>
      </div>
      <div ref={refs.fc1.callbackRef} style={{ minWidth: 0, position: 'relative', zIndex: 2, border: '1px solid #1b3039', background: 'transparent', pointerEvents: 'auto' }}>
        <div style={{ position: 'absolute', zIndex: 2, left: 10, top: 8, font: '700 9px var(--font-geist-mono)', letterSpacing: '.1em', color: '#88a4ae', pointerEvents: 'none' }}>Flight Controller 1</div>
      </div>
      <div ref={refs.fc2.callbackRef} style={{ minWidth: 0, position: 'relative', zIndex: 2, border: '1px solid #1b3039', background: 'transparent', pointerEvents: 'auto' }}>
        <div style={{ position: 'absolute', zIndex: 2, left: 10, top: 8, font: '700 9px var(--font-geist-mono)', letterSpacing: '.1em', color: '#88a4ae', pointerEvents: 'none' }}>Flight Controller 2</div>
      </div>
      <div className="legend"><span><i className="healthy" />Healthy</span><span><i className="degraded" />Degraded / blinking</span><span><i className="failed" />Failed / blinking</span></div>
    </div>
  );
}

useGLTF.preload('/models/drone-done.glb');
