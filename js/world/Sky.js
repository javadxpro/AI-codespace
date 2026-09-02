import * as THREE from '../../vendor/three.module.js';
import { CONFIG } from '../config.js';

const SKY_VERT = `
varying vec3 vDir;
void main() {
  vec4 w = modelMatrix * vec4(position, 1.0);
  vDir = w.xyz;
  gl_Position = projectionMatrix * viewMatrix * w;
}
`;

const SKY_FRAG = `
varying vec3 vDir;
uniform vec3 topColor;
uniform vec3 bottomColor;
void main() {
  float h = normalize(vDir).y;
  float t = clamp(h * 0.62 + 0.42, 0.0, 1.0);
  vec3 col = mix(bottomColor, topColor, pow(t, 0.85));
  gl_FragColor = vec4(col, 1.0);
}
`;

/**
 * Cheap atmosphere: gradient dome + hemisphere + one sun.
 * Extension point for weather / day-night (setSun).
 */
export class Sky {
  constructor(scene) {
    this.scene = scene;
    this.elevation = 0.92;
    this.azimuth = 0.52;

    this.uniforms = {
      topColor: { value: new THREE.Color(0x5b8eae) },
      bottomColor: { value: new THREE.Color(0xddd1b4) },
    };
    const skyMat = new THREE.ShaderMaterial({
      vertexShader: SKY_VERT,
      fragmentShader: SKY_FRAG,
      uniforms: this.uniforms,
      side: THREE.BackSide,
      depthWrite: false,
      fog: false,
    });
    skyMat.toneMapped = false;
    this.dome = new THREE.Mesh(new THREE.SphereGeometry(90, 16, 12), skyMat);
    this.dome.frustumCulled = false;
    this.dome.userData.ignoreRay = true;
    scene.add(this.dome);

    this.hemi = new THREE.HemisphereLight(0xc5dcea, 0x6a5a3c, 0.72);
    scene.add(this.hemi);

    this.sun = new THREE.DirectionalLight(0xfff1d2, 0.82);
    this.sun.castShadow = false;
    scene.add(this.sun);

    this.ambient = new THREE.AmbientLight(0x40444c, 0.28);
    scene.add(this.ambient);

    const waterGeo = new THREE.PlaneGeometry(180, 180, 1, 1);
    waterGeo.rotateX(-Math.PI / 2);
    this.water = new THREE.Mesh(
      waterGeo,
      new THREE.MeshPhongMaterial({
        color: 0x3a7ea6,
        transparent: true,
        opacity: 0.32,
        shininess: 70,
        depthWrite: false,
      }),
    );
    this.water.position.y = CONFIG.waterLevel;
    this.water.userData.ignoreRay = true;
    scene.add(this.water);

    scene.fog = new THREE.Fog(0xc4d2dc, CONFIG.fogNear, CONFIG.fogFar);
    this.setSun(this.elevation, this.azimuth);
  }

  setSun(elevation, azimuth) {
    this.elevation = elevation;
    this.azimuth = azimuth;
    const r = 48;
    this.sun.position.set(
      r * Math.cos(elevation) * Math.sin(azimuth),
      r * Math.sin(elevation),
      r * Math.cos(elevation) * Math.cos(azimuth),
    );
  }

  follow(position) {
    this.dome.position.copy(position);
  }
}
