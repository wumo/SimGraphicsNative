#ifndef SIMGRAPHICSNATIVE_DIRECT_H
#define SIMGRAPHICSNATIVE_DIRECT_H

// ref in shaders
struct CameraUBO {
  mat4 view;
  mat4 proj;
  mat4 projView;
  // mat4 viewInv;
  // mat4 projInv;
  vec4 eye;
  // vec4 r, v;
  // float w, h, fov;
  // float zNear, zFar;
};

// ref in shaders
struct LightUBO {
  uint numLights;
  float exposure, gamma;
};

const uint LightType_Directional = 1u;
const uint LightType_Point = 2u;
const uint LightType_Spot = 3u;

// ref in shaders
struct LightInstanceUBO {
  vec3 color;
  float intensity;
  vec3 direction;
  float range;
  vec3 location;
  float spotInnerConeAngle, spotOuterConeAngle;
  uint type;
};

struct Mesh {
  uint primitive, material, node, instance;
};

const uint MaterialType_BRDF = 0x1u;
const uint MaterialType_BRDFSG = 0x2u;
const uint MaterialType_Reflective = 0x4u;
const uint MaterialType_Refractive = 0x8u;
const uint MaterialType_None = 0x10u;
const uint MaterialType_Translucent = 0x20u;
const uint MaterialType_Terrain = 0x40u;

// ref in shaders
struct MaterialUBO {
  vec4 baseColorFactor;
  vec4 pbrFactor;
  vec4 emissiveFactor;
  float occlusionStrength;
  float alphaCutoff;
  int colorTex, pbrTex, normalTex, occlusionTex, emissiveTex, heightTex;
  uint type;
};

#define PRECISION 0.000001
bool isZero(float val) {
  return step(-PRECISION, val) * (1.0 - step(PRECISION, val)) == 1.0;
}

#endif //SIMGRAPHICSNATIVE_DIRECT_H
