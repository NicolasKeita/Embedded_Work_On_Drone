'use client';

import { Component, Suspense, useEffect, useMemo, useRef, useState, useSyncExternalStore, type ComponentRef, type ReactNode } from 'react';
import { Canvas, useThree } from '@react-three/fiber';
import { ContactShadows, Html, OrbitControls, useGLTF } from '@react-three/drei';
import { ArrowDownToLine, ArrowLeft, Box, Expand, Layers3, MousePointer2, Pause, Play, RotateCcw } from 'lucide-react';
import Link from 'next/link';
import * as THREE from 'three';
import styles from './x721-studio.module.css';

const MODEL_URL = '/models/x721-three-wing-concept.glb';
const subscribeToClientRender = () => () => {};
const clientSnapshot = () => true;
const serverSnapshot = () => false;

const VIEWS = {
  perspective: { label: 'Perspective', direction: [3, 2.3, 8] },
  top: { label: 'Dessus', direction: [0, 1, 0.0001] },
  front: { label: 'Face', direction: [0, 0, 1] },
  side: { label: 'Profil', direction: [1, 0, 0] },
} as const;

type ViewName = keyof typeof VIEWS;
type CameraCommand = { view: ViewName; revision: number };

class ModelErrorBoundary extends Component<{ children: ReactNode }, { failed: boolean }> {
  state = { failed: false };

  static getDerivedStateFromError() {
    return { failed: true };
  }

  render() {
    if (this.state.failed) {
      return <output className={styles.fallback}><Box size={28} /><strong>L’aperçu 3D est indisponible.</strong><span>Le fichier GLB reste accessible au téléchargement.</span></output>;
    }
    return this.props.children;
  }
}

function Model({ url, wireframe, onBounds }: { url: string; wireframe: boolean; onBounds: (size: THREE.Vector3) => void }) {
  const { scene } = useGLTF(url);
  const model = useMemo(() => {
    const copy = scene.clone(true);
    const bounds = new THREE.Box3().setFromObject(copy);
    const center = bounds.getCenter(new THREE.Vector3());
    const size = bounds.getSize(new THREE.Vector3());
    const scale = 6 / Math.max(size.x, size.y, size.z, 0.001);
    copy.position.sub(center);
    copy.traverse((object) => {
      if (!(object instanceof THREE.Mesh)) return;
      object.castShadow = true;
      object.receiveShadow = true;
      const cloneMaterial = (material: THREE.Material) => material.clone();
      object.material = Array.isArray(object.material) ? object.material.map(cloneMaterial) : cloneMaterial(object.material);
    });
    return { scene: copy, scale, normalizedSize: size.clone().multiplyScalar(scale), floor: -size.y * scale / 2 - 0.12 };
  }, [scene]);

  useEffect(() => {
    onBounds(model.normalizedSize);
  }, [model, onBounds]);

  useEffect(() => {
    model.scene.traverse((object) => {
      if (!(object instanceof THREE.Mesh)) return;
      const materials = Array.isArray(object.material) ? object.material : [object.material];
      for (const material of materials) {
        if (material instanceof THREE.MeshStandardMaterial) material.wireframe = wireframe;
      }
    });
  }, [model, wireframe]);

  return (
    <>
      <group scale={model.scale}><primitive object={model.scene} dispose={null} /></group>
      <ContactShadows position={[0, model.floor, 0]} opacity={0.35} scale={14} blur={2.8} far={4} resolution={512} color="#53636b" frames={1} />
      <gridHelper args={[30, 60, '#c4cecf', '#dce3e3']} position={[0, model.floor - 0.01, 0]} />
    </>
  );
}

function CameraRig({ command, autoRotate, onOrbit, bounds }: { command: CameraCommand; autoRotate: boolean; onOrbit: () => void; bounds: THREE.Vector3 | null }) {
  const controls = useRef<ComponentRef<typeof OrbitControls>>(null);
  const { camera, size } = useThree();

  useEffect(() => {
    if (!bounds || !(camera instanceof THREE.PerspectiveCamera)) return;
    const preset = VIEWS[command.view].direction;
    const direction = new THREE.Vector3(preset[0], preset[1], preset[2]).normalize();
    const right = new THREE.Vector3(0, 1, 0).cross(direction).normalize();
    const up = direction.clone().cross(right).normalize();
    const tanVertical = Math.tan(THREE.MathUtils.degToRad(camera.fov) / 2);
    const tanHorizontal = tanVertical * size.width / Math.max(size.height, 1);
    let distance = 0;
    for (const x of [-0.5, 0.5]) {
      for (const y of [-0.5, 0.5]) {
        for (const z of [-0.5, 0.5]) {
          const corner = new THREE.Vector3(bounds.x * x, bounds.y * y, bounds.z * z);
          const extent = Math.max(Math.abs(corner.dot(right)) / tanHorizontal, Math.abs(corner.dot(up)) / tanVertical);
          distance = Math.max(distance, corner.dot(direction) + extent * 1.25);
        }
      }
    }
    camera.position.copy(direction.multiplyScalar(Math.max(distance, 2)));
    camera.up.set(0, 1, 0);
    camera.lookAt(0, 0, 0);
    if (controls.current) {
      controls.current.target.set(0, 0, 0);
      controls.current.update();
    }
  }, [camera, command, bounds, size.width, size.height]);

  return <OrbitControls ref={controls} makeDefault enableDamping dampingFactor={0.08} minDistance={2} maxDistance={22} autoRotate={autoRotate} autoRotateSpeed={0.55} onStart={onOrbit} />;
}

export function X721Studio() {
  const ready = useSyncExternalStore(subscribeToClientRender, clientSnapshot, serverSnapshot);
  const [command, setCommand] = useState<CameraCommand>({ view: 'perspective', revision: 0 });
  const [selectedView, setSelectedView] = useState<ViewName | null>('perspective');
  const [autoRotate, setAutoRotate] = useState(false);
  const [wireframe, setWireframe] = useState(false);
  const [modelBounds, setModelBounds] = useState<THREE.Vector3 | null>(null);

  const chooseView = (view: ViewName) => {
    setAutoRotate(false);
    setSelectedView(view);
    setCommand((previous) => ({ view, revision: previous.revision + 1 }));
  };

  return (
    <main className={styles.studio} lang="fr">
      <header className={styles.header}>
        <Link href="/" className={styles.back}><ArrowLeft size={15} /><span>Digital Twin</span></Link>
        <span className={styles.headerLabel}>ATELIER DE MODÉLISATION</span>
        <span className={styles.conceptBadge}><span />CONCEPT VISUEL</span>
      </header>

      <section className={styles.heading}>
        <div><p className={styles.eyebrow}>ÉTUDE DE FORME / 001</p><h1>X721<span>Une silhouette, en trois dimensions.</span></h1></div>
        <a href={MODEL_URL} download className={styles.download}><ArrowDownToLine size={17} />Télécharger le modèle<span>GLB</span></a>
      </section>

      <section className={styles.viewer} aria-label="Aperçu interactif du modèle X721">
        <div className={styles.viewerTop}>
          <span className={styles.modelLabel}><Box size={15} />X721 / CONCEPT</span>
          <span className={styles.viewportLabel}>APERÇU 3D</span>
        </div>
        <div className={styles.canvas}>
          <ModelErrorBoundary>
            {ready ? (
              <Canvas camera={{ position: [3, 2.3, 8], fov: 38, near: 0.05, far: 100 }} dpr={[1, 2]} gl={{ antialias: true }} fallback={<div className={styles.fallback}>Votre navigateur ne prend pas en charge WebGL.</div>}>
                <color attach="background" args={['#eaf0f0']} />
                <fog attach="fog" args={['#eaf0f0', 13, 28]} />
                <ambientLight intensity={1.25} />
                <hemisphereLight args={['#e8f3ff', '#7d8e9e', 2]} />
                <directionalLight position={[3, 7, 5]} intensity={3.5} />
                <directionalLight position={[-5, 2, -4]} intensity={2.5} color="#cbdfff" />
                <directionalLight position={[0, 4, -6]} intensity={2} color="#ffffff" />
                <Suspense fallback={<Html center><span className={styles.loading}>Chargement du modèle…</span></Html>}>
                  <Model url={MODEL_URL} wireframe={wireframe} onBounds={setModelBounds} />
                </Suspense>
                <CameraRig command={command} autoRotate={autoRotate} onOrbit={() => setSelectedView(null)} bounds={modelBounds} />
              </Canvas>
            ) : <div className={styles.fallback}>Préparation de l’aperçu 3D…</div>}
          </ModelErrorBoundary>
        </div>
        <div className={styles.hint}><MousePointer2 size={13} /><span>Glisser pour explorer · Molette pour zoomer</span></div>
        <div className={styles.toolbar}>
          <fieldset className={styles.viewButtons} aria-label="Points de vue">
            {(Object.keys(VIEWS) as ViewName[]).map((view) => <button key={view} type="button" aria-pressed={selectedView === view} className={selectedView === view ? styles.selected : ''} onClick={() => chooseView(view)}>{view === 'perspective' && <Expand size={13} />}{VIEWS[view].label}</button>)}
          </fieldset>
          <fieldset className={styles.tools} aria-label="Options d’affichage">
            <button type="button" aria-pressed={wireframe} onClick={() => setWireframe((value) => !value)} title="Afficher le maillage"><Layers3 size={15} /><span>Maillage</span></button>
            <button type="button" aria-pressed={autoRotate} onClick={() => { setAutoRotate((value) => !value); setSelectedView(null); }} title={autoRotate ? 'Arrêter la rotation' : 'Activer la rotation'}>{autoRotate ? <Pause size={15} /> : <Play size={15} />}<span>Rotation</span></button>
            <button type="button" onClick={() => chooseView('perspective')} title="Recentrer le modèle" aria-label="Recentrer le modèle"><RotateCcw size={15} /></button>
          </fieldset>
        </div>
      </section>

      <footer className={styles.footer}>
        <div className={styles.note}><span className={styles.noteNumber}>01</span><p>Interprétation visuelle d’après les images fournies — dimensions non certifiées.</p></div>
        <div className={styles.detail}><span>FORMAT</span><strong>GLB · glTF 2.0</strong></div>
        <div className={styles.detail}><span>STATUT</span><strong>Modèle préparatoire</strong></div>
      </footer>
    </main>
  );
}
