#include <kimia/Shaders.h>

namespace kimia {
namespace shaders {

const char* phongVertex = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
uniform mat4 uModel;
uniform mat4 uViewProj;
uniform mat4 uLightViewProj;
uniform mat4 uNormalMat;
out vec3 vWorldPos;
out vec3 vNormal;
out vec4 vShadowCoord;
out vec2 vUV;
void main() {
  vec4 world = uModel * vec4(aPos, 1.0);
  vWorldPos = world.xyz;
  vNormal = mat3(uNormalMat) * aNormal;
  vShadowCoord = uLightViewProj * world;
  vUV = aUV;
  gl_Position = uViewProj * world;
}
)GLSL";

const char* phongFragment = R"GLSL(
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec4 vShadowCoord;
in vec2 vUV;
uniform vec3 uColor;
uniform float uRoughness;
uniform vec3 uLightDir;
uniform vec3 uAmbient;
uniform vec3 uCameraPos;
uniform sampler2DShadow uShadowMap;
out vec4 fragColor;
void main() {
  vec3 N = normalize(vNormal);
  vec3 L = normalize(-uLightDir);
  float diff = max(dot(N, L), 0.0);
  vec3 V = normalize(uCameraPos - vWorldPos);
  vec3 H = normalize(L + V);
  float shininess = mix(64.0, 2.0, clamp(uRoughness, 0.0, 1.0));
  float spec = pow(max(dot(N, H), 0.0), shininess);
  float shadow = 1.0;
  vec3 shadowProj = vShadowCoord.xyz / vShadowCoord.w;
  shadowProj = shadowProj * 0.5 + 0.5;
  if (shadowProj.x >= 0.0 && shadowProj.x <= 1.0 && shadowProj.y >= 0.0 && shadowProj.y <= 1.0) {
    vec2 texel = 1.0 / textureSize(uShadowMap, 0);
    shadow = 0.0;
    for (int x = -1; x <= 1; ++x) {
      for (int y = -1; y <= 1; ++y) {
        shadow += texture(uShadowMap, shadowProj.xyz + vec3(vec2(float(x), float(y)) * texel, 0.0));
      }
    }
    shadow /= 9.0;
  }
  vec3 base = uColor * (uAmbient + diff * (1.0 - uAmbient));
  float specStrength = (1.0 - uRoughness) * 0.6;
  vec3 color = shadow * base + vec3(specStrength * spec);
  color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));  // gamma
  fragColor = vec4(color, 1.0);
}
)GLSL";

const char* depthVertex = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 uModel;
uniform mat4 uLightViewProj;
void main() {
  vec4 world = uModel * vec4(aPos, 1.0);
  gl_Position = uLightViewProj * world;
  gl_Position.z -= 0.002;  // depth bias against shadow acne
}
)GLSL";

const char* depthFragment = R"GLSL(
#version 330 core
void main() {
  // Depth-only pass; the depth value is written automatically.
}
)GLSL";

}  // namespace shaders
}  // namespace kimia
