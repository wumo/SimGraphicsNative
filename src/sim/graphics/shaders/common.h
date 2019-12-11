#ifndef COMMON_H
#define COMMON_H

struct VkDrawIndexedIndirectCommand {
  uint indexCount;
  uint instanceCount;
  uint firstIndex;
  int vertexOffset;
  uint firstInstance;
};

struct RayPayload {
  vec3 color;
  float hitT;
  uint recursion, maxRecursion;
  vec3 dOdx, dOdy, dDdx, dDdy;
};

struct ShadowRayPayload {
  bool shadowed;
};

struct CameraSetting {
  mat4 view;
  mat4 proj;
  mat4 viewInverse;
  mat4 projInverse;
  vec3 eye, r, v;
  vec4 fov;
  float zNear, zFar;
  float exposure, gamma;
};

struct Light {
  vec4 position;
  vec4 prop;
};

struct Vertex {
  vec3 position;
  vec3 normal;
  vec2 uv;
};

Vertex unpack(vec4[2] vv) {
  Vertex v;
  v.position = vv[0].xyz;
  v.normal = vec3(vv[0].w, vv[1].xy);
  v.uv = vec2(vv[1].z, vv[1].w);
  return v;
}

struct ProcedurePayload {
  Vertex v0, v1, v2;
  vec2 uv;
};

struct Mesh {
  uint indexOffset;
  uint indexCount;
  uint vertexOffset;
  uint vertexCount;
  int primitive;
};

struct Transform {
  float tx, ty, tz;
  float sx, sy, sz;
  float qx, qy, qz, qw;
  int parentID;
  uint precalculated;
};

struct MeshInstance {
  uint mesh;
  uint material;
  uint transform;
};

struct Material {
  uint colorTex;
  float cX, cY, cW, cH;
  int colorLodBias;
  uint colorMaxLod;

  uint pbrTex;
  float pX, pY, pW, pH;
  int pbrLodBias;
  uint pbrMaxLod;

  float r, g, b, a;
  float specular, roughness, metallic, glossiness;

  uint normalTex;
  float nX, nY, nW, nH;
  int normalLodBias;
  uint normalMaxLod;

  uint occlusionTex;
  float oX, oY, oW, oH;
  int occlusionLodBias;
  uint occlusionMaxLod;
  float occlusionStrength;

  uint emissiveTex;
  float emX, emY, emW, emH;
  int emissiveLodBias;
  uint emissiveMaxLod;

  float emR, emG, emB;

  uint reflectiveTex;
  float rlX, rlY, rlW, rlH;
  int reflectLodBias;
  uint reflectMaxLod;

  uint refractiveTex;
  float rrX, rrY, rrW, rrH;
  int refractLodBias;
  uint refractMaxLod;

  uint flag;

  float pad0, pad1;
};

uint eBRDF = 0x1u;
uint eReflective = 0x2u;
uint eRefractive = 0x4u;
uint eNone = 0x8u;
uint eTranslucent = 0x10u;

struct MaterialInfo {
  vec3 albedo;
  vec2 pbr;
  vec3 normal;
  float ao;
  bool hasNormal, isOccluded, isEmissive, isReflective, isRefractive, noDiffuse;
  vec4 emissive, reflective, refractive;
};

// Encapsulate the various inputs used by the various functions in the
// shading equation We store values in this struct to simplify the integration
// of alternative implementations of the shading terms, outlined in the
// Readme.MD
// Appendix.
struct PBRInfo {
  float NdotL; // cos angle between normal and light direction
  float NdotV; // cos angle between normal and view direction
  float NdotH; // cos angle between normal and half vector
  float LdotH; // cos angle between light direction and half vector
  float VdotH; // cos angle between view direction and half vector
  // roughness value, as authored by the model creator (input to shader)
  float perceptualRoughness;
  float metalness;    // metallic value at the surface
  vec3 reflectance0;  // full reflectance color (normal incidence angle)
  vec3 reflectance90; // reflectance color at grazing angle
  // roughness mapped to a more linear change in the roughness (proposed by[2])
  float alphaRoughness;
  vec3 diffuseColor;  // color contribution from diffuse lighting
  vec3 specularColor; // color contribution from specular  lighting
};

const float PI = 3.141592653589793;

// shaders helper functions
vec2 BaryLerp(vec2 a, vec2 b, vec2 c, vec3 barycentrics) {
  return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

vec3 BaryLerp(vec3 a, vec3 b, vec3 c, vec3 barycentrics) {
  return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

vec2 uvTransform(vec2 uv, vec4 trans) { return trans.xy + trans.zw * uv; }

mat4 matrix(Transform t) {
  float qxx = t.qx * t.qx;
  float qyy = t.qy * t.qy;
  float qzz = t.qz * t.qz;
  float qxz = t.qx * t.qz;
  float qxy = t.qx * t.qy;
  float qyz = t.qy * t.qz;
  float qwx = t.qw * t.qx;
  float qwy = t.qw * t.qy;
  float qwz = t.qw * t.qz;
  // clang-format off
  mat4 model = mat4(
    t.sx*(1-2*(qyy+qzz)), t.sx*(2*(qxy + qwz))  ,t.sx*(2*(qxz-qwy))  , 0,
    t.sy*(2*(qxy-qwz))  , t.sy*(1-2*(qxx + qzz)),t.sy*(2*(qyz+qwx))  , 0,
    t.sz*(2*(qxz+qwy))  , t.sz*(2*(qyz-qwx))    ,t.sz*(1-2*(qxx+qyy)), 0,
    0                   , 0                     ,0                   , 1);
  // clang-format on
  model[3] = vec4(t.tx, t.ty, t.tz, 1.0);
  return model;
}

mat3 rotation(vec3 u, float theta) {
  float cosT = cos(theta);
  float _1_cosT = 1 - cosT;
  float sinT = sin(theta);
  float uxx = u.x * u.x;
  float uxy = u.x * u.y;
  float uxz = u.x * u.z;
  float uyy = u.y * u.y;
  float uyz = u.y * u.z;
  float uzz = u.z * u.z;
  return mat3(
    cosT + uxx * _1_cosT, uxy * _1_cosT + u.z * sinT, uxz * _1_cosT - u.y * sinT,
    uxy * _1_cosT - u.z * sinT, cosT + uyy * _1_cosT, uyz * _1_cosT + u.x * sinT,
    uxz * _1_cosT + u.y * sinT, uyz * _1_cosT - u.x * sinT, cosT + uzz * _1_cosT);
}

#define PRECISION 0.000001
bool isZero(float val) {
  return step(-PRECISION, val) * (1.0 - step(PRECISION, val)) == 1.0;
}
bool equal(float a, float b) { return isZero(a - b); }

#endif // SHARED_WITH_SHADERS_H
