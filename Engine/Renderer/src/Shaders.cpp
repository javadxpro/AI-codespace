#include <kimia/Shaders.h>

namespace kimia {
namespace shaders {

// The shader body is written once and shared by desktop GL, Android GLES3
// and WebGL2: only the version line and the mandatory fragment precision
// differ. Emscripten and Android compile GLSL ES 3.00, native desktop GL
// wants 330 core.
#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
#define KIMIA_GLSL_VERSION "#version 300 es\nprecision highp float;\nprecision highp int;\n"
#else
#define KIMIA_GLSL_VERSION "#version 330 core\n"
#endif

const char* phongVertex = KIMIA_GLSL_VERSION R"GLSL(
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

const char* phongFragment = KIMIA_GLSL_VERSION R"GLSL(
in vec3 vWorldPos;
in vec3 vNormal;
in vec4 vShadowCoord;
in vec2 vUV;
uniform vec3 uColor;
uniform float uRoughness;
uniform float uMetallic;
uniform vec3 uEmissive;
uniform float uAlpha;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;
uniform vec3 uCameraPos;
uniform sampler2DShadow uShadowMap;
uniform sampler2D uBaseTexture;
uniform float uHasTexture;
uniform int uPointLightCount;
uniform vec3 uPointLightPos[8];
uniform vec3 uPointLightColor[8];
uniform float uPointLightRadius[8];
uniform vec3 uFogColor;
uniform float uFogDensity;
out vec4 fragColor;

const float PI = 3.14159265358979323846;

// Mirrors Engine/Renderer/include/kimia/Pbr.h (single source of truth for
// the BRDF) so GL and the software rasteriser render the same frame.
float roughnessAlpha(float roughness) { return max(roughness * roughness, 0.02); }

vec3 fresnelF0(vec3 albedo, float metallic) { return mix(vec3(0.04), albedo, metallic); }

vec3 fresnelSchlick(float vDotH, vec3 f0) {
  float k = pow(max(1.0 - vDotH, 0.0), 5.0);
  return f0 + (vec3(1.0) - f0) * k;
}

float ggxDistribution(float nDotH, float alpha) {
  float a2 = alpha * alpha;
  float denom = nDotH * nDotH * (a2 - 1.0) + 1.0;
  return a2 / (PI * denom * denom);
}

float smithG1(float nDotV, float alpha) {
  float a2 = alpha * alpha;
  return (2.0 * nDotV) / (nDotV + sqrt(a2 + (1.0 - a2) * nDotV * nDotV));
}

// LINEAR outgoing radiance of the Cook-Torrance BRDF for one light. The
// light radiance is color * PI, which cancels the diffuse 1/PI (see Pbr.h).
vec3 cookTorrance(vec3 albedo, float roughness, float metallic, vec3 N, vec3 V, vec3 L,
                  vec3 lightColor) {
  float nDotL = max(dot(N, L), 0.0);
  if (nDotL <= 0.0) return vec3(0.0);
  float nDotV = max(dot(N, V), 0.0);
  vec3 H = normalize(V + L);
  float nDotH = max(dot(N, H), 0.0);
  float vDotH = max(dot(V, H), 0.0);
  float alpha = roughnessAlpha(roughness);

  vec3 f0 = fresnelF0(albedo, metallic);
  vec3 f = fresnelSchlick(vDotH, f0);
  vec3 kd = (vec3(1.0) - f) * (1.0 - metallic);

  float d = ggxDistribution(nDotH, alpha);
  float g = smithG1(nDotL, alpha) * smithG1(nDotV, alpha);
  vec3 specular = f * (d * g / max(4.0 * nDotL * nDotV, 1e-4));

  vec3 radiance = lightColor * PI;
  return (kd * albedo / PI + specular) * radiance * nDotL;
}

void main() {
  vec3 N = normalize(vNormal);
  vec3 V = normalize(uCameraPos - vWorldPos);
  vec3 L = normalize(-uLightDir);
  vec3 albedo = uColor;
  if (uHasTexture > 0.5) albedo *= texture(uBaseTexture, vUV).rgb;

  // Key light (shadowed below) + additive point lights + ambient fill.
  vec3 key = cookTorrance(albedo, uRoughness, uMetallic, N, V, L, uLightColor);
  vec3 points = vec3(0.0);
  for (int i = 0; i < uPointLightCount; ++i) {
    vec3 toLight = uPointLightPos[i] - vWorldPos;
    float dist = length(toLight);
    if (dist >= uPointLightRadius[i]) continue;
    float falloff = 1.0 - dist / max(uPointLightRadius[i], 1e-4);
    float attenuation = falloff * falloff;
    vec3 lampColor = uPointLightColor[i] * attenuation;
    points += cookTorrance(albedo, uRoughness, uMetallic, N, V, toLight / max(dist, 1e-4), lampColor);
  }

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

  // Shadow falls on the key light only; ambient and point lights are free.
  // Emissive is self-illumination, independent of every light.
  vec3 color = albedo * uAmbient + key * shadow + points + uEmissive;

  // Exponential-squared distance fog, linear space (matches the software
  // rasteriser). uFogDensity == 0 -> factor 1 -> no fog.
  float dist = distance(uCameraPos, vWorldPos);
  float fogK = uFogDensity * dist;
  float fogFactor = exp(-fogK * fogK);
  color = mix(uFogColor, color, fogFactor);

  // Display pipeline, identical to the software rasteriser: filmic tone
  // map (ACES fit) then the sRGB transfer function.
  color = max(color, vec3(0.0));
  color = (color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14);
  color = mix(color * 12.92, 1.055 * pow(color, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, color));
  fragColor = vec4(color, uAlpha);
}
)GLSL";

const char* depthVertex = KIMIA_GLSL_VERSION R"GLSL(
layout(location = 0) in vec3 aPos;
uniform mat4 uModel;
uniform mat4 uLightViewProj;
void main() {
  vec4 world = uModel * vec4(aPos, 1.0);
  gl_Position = uLightViewProj * world;
  gl_Position.z -= 0.002;  // depth bias against shadow acne
}
)GLSL";

const char* depthFragment = KIMIA_GLSL_VERSION R"GLSL(
void main() {
  // Depth-only pass; the depth value is written automatically.
}
)GLSL";

}  // namespace shaders
}  // namespace kimia
