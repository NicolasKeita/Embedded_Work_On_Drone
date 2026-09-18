import { mkdir, writeFile } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import * as THREE from 'three';
import { GLTFExporter } from 'three/addons/exporters/GLTFExporter.js';

/** Minimal binary-only FileReader adapter for the exporter in Node.js. */
class BinaryFileReader {
  readAsArrayBuffer(blob) {
    blob.arrayBuffer().then((buffer) => {
      this.result = buffer;
      this.onloadend?.({ target: this });
    });
  }
}

globalThis.FileReader ??= BinaryFileReader;

const outputDirectory = new URL('../public/models/', import.meta.url);
const parameters = Object.freeze({
  wingSpan: 2.2,
  wingChord: 0.235,
  wingRadius: 2.05,
  wingHeight: 0.14,
  capsuleHeight: -0.38,
  spanSegments: 80,
  sectionSegments: 48,
});

const materials = {
  graphite: new THREE.MeshPhysicalMaterial({
    name: 'Graphite shell', color: '#242b31', metalness: 0.36,
    roughness: 0.3, clearcoat: 0.65, clearcoatRoughness: 0.24,
  }),
  inset: new THREE.MeshStandardMaterial({
    name: 'Satin inset', color: '#10191f', metalness: 0.28, roughness: 0.47,
  }),
  edge: new THREE.MeshStandardMaterial({
    name: 'Leading edge highlight', color: '#64717a', metalness: 0.65, roughness: 0.28,
  }),
  seam: new THREE.MeshStandardMaterial({
    name: 'Panel seams', color: '#080d12', metalness: 0.12, roughness: 0.65,
  }),
  tether: new THREE.MeshStandardMaterial({
    name: 'Tethers', color: '#76848c', metalness: 0.4, roughness: 0.58,
  }),
  red: new THREE.MeshPhysicalMaterial({
    name: 'Red suspended capsule', color: '#aa1725', metalness: 0.22,
    roughness: 0.28, clearcoat: 0.75, clearcoatRoughness: 0.22,
  }),
  lens: new THREE.MeshStandardMaterial({
    name: 'Red marker', color: '#f44844', emissive: '#f12520',
    emissiveIntensity: 0.55, roughness: 0.2,
  }),
};

/** Cosmetic dimensions inferred from the silhouette, without aerodynamic meaning. */
function wingSection(span) {
  const fraction = Math.abs(span);
  const taper = Math.pow(Math.max(0, 1 - fraction ** 2), 0.64);
  return {
    x: span * parameters.wingSpan / 2,
    chord: 0.004 + parameters.wingChord * taper,
    y: 0.007 + 0.074 * fraction ** 4,
    z: 0.032 * fraction ** 2 - 0.008 * span,
    thickness: 0.001 + 0.024 * taper,
  };
}

/** Return one point on the closed, rounded display profile of a wing section. */
function wingPoint(span, angle) {
  const section = wingSection(span);
  const chordFraction = (1 - Math.cos(angle)) / 2;
  const contour = Math.sin(angle) * (1 - 0.5 * chordFraction);
  return new THREE.Vector3(
    section.x,
    section.y + section.thickness * contour + 0.007 * section.chord / (parameters.wingChord + 0.004) * Math.sin(chordFraction * Math.PI),
    section.z + (chordFraction - 0.46) * section.chord,
  );
}

/** Build a closed loft with outward winding and sealed tips. */
function wingGeometry() {
  const positions = [];
  const indices = [];
  const spans = parameters.spanSegments;
  const sections = parameters.sectionSegments;
  for (let row = 0; row <= spans; row += 1) {
    for (let column = 0; column < sections; column += 1) {
      positions.push(...wingPoint(row / spans * 2 - 1, column / sections * Math.PI * 2));
    }
  }
  for (let row = 0; row < spans; row += 1) {
    for (let column = 0; column < sections; column += 1) {
      const a = row * sections + column;
      const b = row * sections + (column + 1) % sections;
      const c = a + sections;
      const d = b + sections;
      indices.push(a, b, c, b, d, c);
    }
  }
  for (const end of [0, spans]) {
    const section = wingSection(end === 0 ? -1 : 1);
    const center = positions.length / 3;
    positions.push(section.x, section.y, section.z);
    for (let column = 0; column < sections; column += 1) {
      const a = end * sections + column;
      const b = end * sections + (column + 1) % sections;
      indices.push(...(end === 0 ? [center, b, a] : [center, a, b]));
    }
  }
  const geometry = new THREE.BufferGeometry();
  geometry.setAttribute('position', new THREE.Float32BufferAttribute(positions, 3));
  geometry.setIndex(indices);
  geometry.computeVertexNormals();
  return geometry;
}

/** Add a named mesh whose geometry and material remain reusable on export. */
function addMesh(parent, name, geometry, material) {
  const mesh = new THREE.Mesh(geometry, material);
  mesh.name = name;
  parent.add(mesh);
  return mesh;
}

/** Add a smooth ellipsoidal fairing; all dimensions are artistic proportions. */
function ellipsoid(parent, name, material, position, scale) {
  const mesh = addMesh(parent, name, new THREE.SphereGeometry(1, 40, 24), material);
  mesh.position.set(...position);
  mesh.scale.set(...scale);
  return mesh;
}

/** Build small surface seams or cables as portable GLB triangle meshes. */
function tube(parent, name, points, radius, material, segments = 48) {
  const curve = new THREE.CatmullRomCurve3(points);
  return addMesh(parent, name, new THREE.TubeGeometry(curve, segments, radius, 6, false), material);
}

/** Create a thin bevelled silhouette for an upper or lower appendage. */
function fin(parent, name, points, width) {
  const shape = new THREE.Shape();
  points.forEach(([z, y], index) => {
    if (index === 0) shape.moveTo(z, y);
    else shape.lineTo(z, y);
  });
  shape.closePath();
  const geometry = new THREE.ExtrudeGeometry(shape, {
    depth: width, bevelEnabled: true, bevelSegments: 2,
    steps: 1, bevelSize: 0.0018, bevelThickness: 0.0018,
  });
  geometry.translate(0, 0, -width / 2);
  geometry.rotateY(-Math.PI / 2);
  return addMesh(parent, name, geometry, materials.graphite);
}

/** Create one independent wing unit from the visible reference features. */
function createWing() {
  const wing = new THREE.Group();
  wing.name = 'wing_unit';
  addMesh(wing, 'wing_shell', wingGeometry(), materials.graphite);
  const leadingPoints = [];
  for (let sample = 0; sample <= 80; sample += 1) {
    leadingPoints.push(wingPoint(sample / 80 * 1.98 - 0.99, 0.13));
  }
  tube(wing, 'leading_edge', leadingPoints, 0.0018, materials.edge, 100);
  for (const side of [-1, 1]) {
    const seamPoints = [];
    for (let sample = 0; sample <= 30; sample += 1) {
      seamPoints.push(wingPoint(side * (0.2 + sample / 30 * 0.67), 2.02).add(new THREE.Vector3(0, 0.001, 0)));
    }
    tube(wing, side < 0 ? 'inner_panel_seam' : 'outer_panel_seam', seamPoints, 0.0008, materials.seam, 36);
  }
  ellipsoid(wing, 'central_fairing', materials.graphite, [0, -0.018, 0.022], [0.084, 0.051, 0.232]);
  ellipsoid(wing, 'upper_fairing', materials.inset, [0, 0.021, 0.001], [0.059, 0.033, 0.154]);
  fin(wing, 'dorsal_fin', [[-0.035, 0.032], [0.025, 0.185], [0.046, 0.188], [0.072, 0.031]], 0.008);
  fin(wing, 'ventral_fin', [[0.036, -0.046], [0.103, -0.152], [0.119, -0.148], [0.139, -0.031]], 0.007);
  const propeller = new THREE.Group();
  propeller.name = 'propeller';
  propeller.position.set(0, -0.038, 0.237);
  propeller.rotation.z = -0.48;
  wing.add(propeller);
  ellipsoid(propeller, 'propeller_hub', materials.edge, [0, 0, 0], [0.014, 0.014, 0.024]);
  for (const side of [-1, 1]) {
    const blade = ellipsoid(propeller, side < 0 ? 'blade_a' : 'blade_b', materials.inset, [side * 0.064, 0, 0], [0.061, 0.012, 0.004]);
    blade.rotation.x = side * 0.3;
    blade.rotation.z = 0.13;
  }
  return wing;
}

/** Create the red suspended capsule visible in the first and third references. */
function createCapsule() {
  const capsule = new THREE.Group();
  capsule.name = 'central_capsule';
  capsule.position.y = parameters.capsuleHeight;
  ellipsoid(capsule, 'capsule_shell', materials.red, [0, -0.029, 0], [0.063, 0.102, 0.06]);
  ellipsoid(capsule, 'capsule_cap', materials.inset, [0, 0.054, 0], [0.041, 0.025, 0.039]);
  ellipsoid(capsule, 'capsule_marker', materials.lens, [0, 0.08, 0], [0.018, 0.016, 0.018]);
  const seam = addMesh(capsule, 'capsule_seam', new THREE.TorusGeometry(0.06, 0.0016, 6, 40), materials.inset);
  seam.rotation.x = Math.PI / 2;
  seam.scale.y = 0.97;
  seam.position.y = -0.032;
  return capsule;
}

/** Assemble radial wing units and explicitly illustrative tether paths. */
function createAssembly(count, prototype) {
  const root = new THREE.Group();
  root.name = 'X721_Concept';
  const rotor = new THREE.Group();
  rotor.name = 'rotor_assembly';
  root.add(rotor);
  for (let index = 0; index < count; index += 1) {
    const angle = index * Math.PI * 2 / count;
    const wing = prototype.clone(true);
    wing.name = `wing_${index + 1}`;
    wing.traverse((object) => {
      if (object === wing) return;
      object.userData.role = object.name;
      object.name = `${wing.name}_${object.name.replace(/^wing_/, '')}`;
    });
    wing.position.set(Math.cos(angle) * parameters.wingRadius, parameters.wingHeight, -Math.sin(angle) * parameters.wingRadius);
    wing.rotation.y = angle;
    rotor.add(wing);
    wing.updateMatrix();
    const anchor = wingPoint(-0.97, Math.PI * 1.5).applyMatrix4(wing.matrix);
    const endpoint = new THREE.Vector3(Math.cos(angle) * 0.025, parameters.capsuleHeight + 0.07, -Math.sin(angle) * 0.025);
    const midpoint = endpoint.clone().lerp(anchor, 0.5);
    midpoint.y -= 0.012;
    tube(rotor, `tether_${index + 1}`, [endpoint, midpoint, anchor], 0.0016, materials.tether, 24);
  }
  root.add(createCapsule());
  return root;
}

/** Verify both tip fans retain their outward orientation when proportions change. */
function inspectWingTips(geometry) {
  const positions = geometry.getAttribute('position');
  const indices = geometry.index;
  const firstCap = parameters.spanSegments * parameters.sectionSegments * 6;
  const a = new THREE.Vector3();
  const b = new THREE.Vector3();
  const c = new THREE.Vector3();
  for (let offset = firstCap; offset < indices.count; offset += 3) {
    a.fromBufferAttribute(positions, indices.getX(offset));
    b.fromBufferAttribute(positions, indices.getX(offset + 1));
    c.fromBufferAttribute(positions, indices.getX(offset + 2));
    const direction = Math.sign(a.x);
    if (b.sub(a).cross(c.sub(a)).x * direction <= 0) {
      throw new Error('Wing tip contains an inverted or degenerate cap triangle');
    }
  }
}

/** Reject corrupt data, invalid indices and inward-facing closed wing geometry. */
function inspectModel(root) {
  let triangles = 0;
  let meshCount = 0;
  const geometries = new Set();
  root.traverse((object) => {
    if (!object.isMesh) return;
    meshCount += 1;
    const geometry = object.geometry;
    const positions = geometry.getAttribute('position');
    const normals = geometry.getAttribute('normal');
    if (![...positions.array, ...normals.array].every(Number.isFinite)) {
      throw new Error(`Non-finite geometry: ${object.name}`);
    }
    if (geometry.index && ![...geometry.index.array].every((value) => value < positions.count)) {
      throw new Error(`Invalid index: ${object.name}`);
    }
    if (object.name === 'wing_shell' || object.userData.role === 'wing_shell') {
      const sample = parameters.sectionSegments * parameters.spanSegments / 2 + parameters.sectionSegments / 4;
      if (normals.getY(sample) < 0.5) throw new Error('Wing top normal faces inward');
      inspectWingTips(geometry);
    }
    triangles += (geometry.index?.count ?? positions.count) / 3;
    geometries.add(geometry);
  });
  const bounds = new THREE.Box3().setFromObject(root);
  return {
    meshes: meshCount,
    uniqueGeometries: geometries.size,
    triangles,
    bounds: { min: bounds.min.toArray(), max: bounds.max.toArray() },
    size: bounds.getSize(new THREE.Vector3()).toArray(),
  };
}

/** Export self-contained GLB files with provenance and unambiguous scale metadata. */
async function exportModel(filename, model, description) {
  model.userData = {
    title: description,
    purpose: 'Visual interpretation of supplied X721 reference images; not engineering CAD.',
    scale: 'Arbitrary display proportions. Numeric units must not be interpreted as measured metres.',
    orientation: 'Y up; two-wing assembly spans X; capsule below the origin.',
    uncertainFeatures: 'Wing sections, appendages, propellers, tethers and dimensions are artistic approximations.',
  };
  const statistics = inspectModel(model);
  const scene = new THREE.Scene();
  scene.name = 'X721_visual_reference';
  scene.add(model);
  const buffer = await new GLTFExporter().parseAsync(scene, { binary: true, onlyVisible: true });
  await writeFile(new URL(filename, outputDirectory), Buffer.from(buffer));
  return { filename, description, bytes: buffer.byteLength, ...statistics };
}

await mkdir(outputDirectory, { recursive: true });
const wing = createWing();
const variants = [];
variants.push(await exportModel('x721-concept.glb', createAssembly(2, wing), 'Two independent wings and suspended capsule'));
variants.push(await exportModel('x721-three-wing-concept.glb', createAssembly(3, wing), 'Three-wing visual variant from reference image 2'));
variants.push(await exportModel('x721-wing.glb', wing, 'Isolated wing unit for visual inspection'));
await writeFile(new URL('x721-model-info.json', outputDirectory), `${JSON.stringify({
  generator: 'scripts/generate-x721-model.mjs',
  parameters,
  measuredDimensions: false,
  variants,
}, null, 2)}\n`);
process.stdout.write(`Generated X721 visual models in ${fileURLToPath(outputDirectory)}\n${JSON.stringify(variants, null, 2)}\n`);
