'use client';

/* eslint-disable react/react-compiler: oxlint react-compiler false-positives on standard ref={callbackRef} JSX for the fc1/fc2 panels */
import { useMemo } from 'react';
import { useFrame } from '@react-three/fiber';
import { OrbitControls, useGLTF } from '@react-three/drei';
import * as THREE from 'three';
import { PortalView } from '@/components/portal-view';
import { Stm32FaultModel } from '@/components/stm32-fault-model';
import type { ComponentState, TwinSnapshot } from '@/lib/twin-data';

const SHOW_AIRFRAME_AXES = false;
const AIRFRAME_AXES_SIZE = 2.8;
const AIRFRAME_DISPLAY_SPAN = 5;

const stateColor = (state: ComponentState) => state === 'HEALTHY' ? '#32d296' : state === 'DEGRADED' ? '#f1b84b' : state === 'FAILED' ? '#ff5454' : '#71808b';

type WingMaterialState = {
  material: THREE.MeshStandardMaterial;
  color: THREE.Color;
  emissive: THREE.Color;
  emissiveIntensity: number;
};

function FaultAwareAirframe({ snapshot }: { snapshot: TwinSnapshot }) {
  const { scene } = useGLTF('/models/x721-three-wing-concept.glb');
  const wingState = snapshot.active_fault === 'ACTUATOR_DEGRADED'
    ? snapshot.fc1.components.actuators ?? 'DEGRADED'
    : 'HEALTHY';
  const airframe = useMemo(() => {
    const clone = scene.clone();
    const materials: WingMaterialState[] = [];
    clone.traverse((object) => {
      if (!(object instanceof THREE.Mesh)) return;
      object.castShadow = true;
      object.receiveShadow = true;
      const isWing = object.userData.role === 'wing_shell';
      const clonedMaterials = (Array.isArray(object.material) ? object.material : [object.material])
        .map((material) => material.clone());
      object.material = Array.isArray(object.material) ? clonedMaterials : clonedMaterials[0];
      if (isWing) {
        clonedMaterials.forEach((material) => {
          if (material instanceof THREE.MeshStandardMaterial) {
            materials.push({ material, color: material.color.clone(), emissive: material.emissive.clone(), emissiveIntensity: material.emissiveIntensity });
          }
        });
      }
    });
    const bounds = new THREE.Box3().setFromObject(clone);
    const size = bounds.getSize(new THREE.Vector3());
    const center = bounds.getCenter(new THREE.Vector3());
    const scale = AIRFRAME_DISPLAY_SPAN / Math.max(size.x, size.y, size.z, .001);
    clone.position.copy(center).multiplyScalar(-scale);
    clone.scale.setScalar(scale);
    return { model: clone, wingMaterials: materials };
  }, [scene]);
  const faultColor = useMemo(() => new THREE.Color(stateColor(wingState)), [wingState]);

  useFrame(({ clock }) => {
    const active = wingState === 'DEGRADED' || wingState === 'FAILED';
    const pulse = (Math.sin(clock.elapsedTime * 8) + 1) * .5;
    airframe.wingMaterials.forEach(({ material, color, emissive, emissiveIntensity }) => {
      if (active) {
        material.color.copy(faultColor);
        material.emissive.copy(faultColor);
        material.emissiveIntensity = .65 + pulse * 1.35;
        return;
      }
      material.color.copy(color);
      material.emissive.copy(emissive);
      material.emissiveIntensity = emissiveIntensity;
    });
  });

  return <primitive object={airframe.model} />;
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
      <PortalView track={refs.airframe.ref} cameraConfig={{ position: [4.8, 4.2, 6.2], fov: 38 }} background="#25282e">
        <SceneLighting />
        <directionalLight position={[-5, 4, 7]} intensity={5.5} color="#e8fbff" />
        <directionalLight position={[5, 1, -4]} intensity={3.2} color="#9edbe5" />
        {SHOW_AIRFRAME_AXES && <axesHelper args={[AIRFRAME_AXES_SIZE]} />}
        <FaultAwareAirframe snapshot={snapshot} />
        <OrbitControls domElement={refs.airframe.domElement} enablePan={false} minDistance={4.5} maxDistance={11} maxPolarAngle={Math.PI * .85} />
      </PortalView>
      <PortalView track={refs.fc1.ref} cameraConfig={{ orthographic: true, position: [0, 0, 8], zoom: 48 }} background="#1c1e23">
        <SceneLighting />
        <Stm32FaultModel components={snapshot.fc1.components} activeFault={snapshot.active_fault} />
        <OrbitControls domElement={refs.fc1.domElement} enablePan={false} minDistance={4.5} maxDistance={10} maxPolarAngle={Math.PI * .85} />
      </PortalView>
      <PortalView track={refs.fc2.ref} cameraConfig={{ orthographic: true, position: [0, 0, 8], zoom: 48 }} background="#1c1e23">
        <SceneLighting />
        <Stm32FaultModel components={snapshot.fc2.components} />
        <OrbitControls domElement={refs.fc2.domElement} enablePan={false} minDistance={4.5} maxDistance={10} maxPolarAngle={Math.PI * .85} />
      </PortalView>
    </>
  );
}

export function AvionicsSceneDom({ refs }: { refs: AvionicsPanelRefs }) {
  return (
    <div className="avionics-canvas">
      <div ref={refs.airframe.callbackRef} className="hardware-view">
        <div className="hardware-label">X721 · AIRFRAME</div>
      </div>
      <div ref={refs.fc1.callbackRef} className="hardware-view">
        <div className="hardware-label">Flight Controller 1</div>
      </div>
      <div ref={refs.fc2.callbackRef} className="hardware-view">
        <div className="hardware-label">Flight Controller 2</div>
      </div>
      <div className="legend"><span><i className="healthy" />Healthy</span><span><i className="degraded" />Degraded / blinking</span><span><i className="failed" />Failed / blinking</span></div>
    </div>
  );
}

useGLTF.preload('/models/x721-three-wing-concept.glb');
