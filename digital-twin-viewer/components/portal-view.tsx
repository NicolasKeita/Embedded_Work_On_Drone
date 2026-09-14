'use client';

import { type ReactNode, useMemo, useState } from 'react';
import { createPortal, useFrame, useThree } from '@react-three/fiber';
import * as THREE from 'three';

/*
Single-renderer portal view for the digital twin dashboard.

@react-three/fiber exposes one module-level React context and renders
<context.Provider> inside every <Canvas>. Mounting several <Canvas> components
at once therefore trips React's reconciler warning:
"Detected multiple renderers concurrently rendering the same context provider.
This is currently unsupported." (once per extra renderer).

To keep a single renderer (and silence the warning) while preserving the
existing per-panel DOM layout and independent orbit controls, the dashboard
hosts one root <Canvas> and each 3D panel projects its scene through a
<PortalView>. A portal view owns:
  - a virtual THREE.Scene rendered via fiber's createPortal (same renderer),
  - a camera (perspective or orthographic) with an aspect matched to the panel,
  - a scissor viewport computed from the tracked DOM element's rect,
  - a ref to the tracked element so drei <OrbitControls> can bind its pointer
    events to that element via its `domElement` prop, keeping every panel's
    interactions independent without sharing fiber's global event layer.

The imperative three.js work (viewport/scissor/projection) lives in a plain
module-level function so the react-compiler does not attribute three.js
mutations to React render values.
*/

export type CameraConfig =
  | { orthographic: true; position: [number, number, number]; zoom: number }
  | { orthographic?: false; position: [number, number, number]; fov: number; far?: number };

function isOrthographicConfig(config: CameraConfig): config is { orthographic: true; position: [number, number, number]; zoom: number } {
  return config.orthographic === true;
}

function createCamera(config: CameraConfig): THREE.OrthographicCamera | THREE.PerspectiveCamera {
  if (isOrthographicConfig(config)) {
    const cam = new THREE.OrthographicCamera(-1, 1, 1, -1, 0.1, 1000);
    cam.position.set(...config.position);
    cam.zoom = config.zoom;
    cam.updateProjectionMatrix();
    return cam;
  }
  const cam = new THREE.PerspectiveCamera(config.fov, 1, 0.1, config.far ?? 1000);
  cam.position.set(...config.position);
  cam.updateProjectionMatrix();
  return cam;
}

function renderPortalViewport(
  scene: THREE.Scene,
  camera: THREE.Camera,
  gl: THREE.WebGLRenderer,
  track: React.RefObject<HTMLDivElement | null>,
  cameraConfig: CameraConfig,
  background: THREE.Color | null,
) {
  const element = track.current;
  if (!element) return;
  const bounds = element.getBoundingClientRect();
  if (bounds.width === 0 || bounds.height === 0) return;
  const canvasBounds = gl.domElement.getBoundingClientRect();
  const left = bounds.left - canvasBounds.left;
  const bottom = canvasBounds.height - (bounds.bottom - canvasBounds.top);
  const width = bounds.width;
  const height = bounds.height;
  const aspect = width / height;
  if (background) scene.background = background;
  if (isOrthographicConfig(cameraConfig)) {
    const ortho = camera as THREE.OrthographicCamera;
    if (ortho.left !== width / -2 || ortho.right !== width / 2 || ortho.top !== height / 2 || ortho.bottom !== height / -2) {
      ortho.left = width / -2;
      ortho.right = width / 2;
      ortho.top = height / 2;
      ortho.bottom = height / -2;
      ortho.updateProjectionMatrix();
    }
  } else {
    const persp = camera as THREE.PerspectiveCamera;
    if (persp.aspect !== aspect) {
      persp.aspect = aspect;
      persp.updateProjectionMatrix();
    }
  }
  const previousAutoClear = gl.autoClear;
  gl.autoClear = false;
  gl.setViewport(left, bottom, width, height);
  gl.setScissor(left, bottom, width, height);
  gl.setScissorTest(true);
  gl.clearDepth();
  gl.render(scene, camera);
  gl.setScissorTest(false);
  gl.autoClear = previousAutoClear;
}

function PortalRenderer({
  track,
  cameraConfig,
  background,
}: {
  track: React.RefObject<HTMLDivElement | null>;
  cameraConfig: CameraConfig;
  background: THREE.Color | null;
}) {
  const scene = useThree((state) => state.scene);
  const camera = useThree((state) => state.camera);
  const gl = useThree((state) => state.gl);

  useFrame(() => {
    renderPortalViewport(scene, camera, gl, track, cameraConfig, background);
  }, 1);

  return null;
}

export function PortalView({
  track,
  cameraConfig,
  background,
  children,
}: {
  track: React.RefObject<HTMLDivElement | null>;
  cameraConfig: CameraConfig;
  background?: string;
  children: ReactNode;
}) {
  const [scene] = useState(() => new THREE.Scene());
  const [camera] = useState(() => createCamera(cameraConfig));
  const backgroundColor = useMemo(() => (background ? new THREE.Color(background) : null), [background]);

  return createPortal(
    <>
      <PortalRenderer track={track} cameraConfig={cameraConfig} background={backgroundColor} />
      {children}
    </>,
    scene,
    { camera },
  );
}
