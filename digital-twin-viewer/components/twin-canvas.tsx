'use client';

import { type ReactNode } from 'react';
import { Canvas } from '@react-three/fiber';
import * as THREE from 'three';

/*
Root canvas for the digital twin dashboard.

The dashboard renders several 3D panels (flight view, airframe, two STM32
boards). Giving each panel its own <Canvas> made @react-three/fiber emit
"Detected multiple renderers concurrently rendering the same context
provider. This is currently unsupported." because fiber's internal React
context is a module-level singleton shared across every renderer.

Hosting every panel inside this single <Canvas> keeps one renderer and
silences the warning. Each panel projects its scene through a <PortalView>
(see portal-view.tsx) that renders into a tracked DOM element via a scissor
viewport, preserving the existing per-panel layout and independent orbit
controls.

The canvas is fixed, covers the viewport, and ignores pointer events so the
DOM UI underneath stays interactive; each panel's <OrbitControls> binds its
own pointer events to its tracked element instead.
*/
export function TwinCanvas({ children }: { children: ReactNode }) {
  return (
    <Canvas
      style={{ position: 'fixed', inset: 0, width: '100vw', height: '100vh', pointerEvents: 'none', zIndex: 0 }}
      gl={{ antialias: true, preserveDrawingBuffer: false, autoClear: false }}
      camera={{ position: [0, 0, 0], fov: 40 }}
      shadows={{ type: THREE.PCFShadowMap }}
    >
      {children}
    </Canvas>
  );
}
