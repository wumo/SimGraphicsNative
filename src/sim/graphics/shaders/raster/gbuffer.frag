#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#include "../common.h"

layout(location = 0) in fs {
  vec3 inWorldPos;
  vec3 inNormal;
  vec2 inUV;
  flat uint materialID;
};

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outAlbedo;
layout(location = 3) out vec2 outPBR;
layout(location = 4) out vec4 outEmissive;

layout(set = 0, binding = 3, std430) readonly buffer MaterialBuffer {
  Material materials[];
};
layout(set = 0, binding = 4) uniform sampler2D samplers[];

#include "../fetch_materials.h"

vec3 computeNormal(vec3 N) {
  vec3 tangentNormal = N * 2.0 - 1.0;

  vec3 q1 = dFdx(inWorldPos);
  vec3 q2 = dFdy(inWorldPos);
  vec2 st1 = dFdx(inUV);
  vec2 st2 = dFdy(inUV);

  N = normalize(inNormal);
  vec3 T = normalize(q1 * st2.t - q2 * st1.t);
  vec3 B = -normalize(cross(N, T));
  mat3 TBN = mat3(T, B, N);

  return normalize(TBN * tangentNormal);
}

void main() {
  Material material = materials[materialID];

  MaterialInfo mat;
  vec4 dst = vec4(0, 0, 0, 0);
  fetchAlbedo(material, inUV, dst, mat);
  fetchPBR(material, inUV, dst, mat);
  fetchNormal(material, inUV, dst, mat);
  vec3 N = mat.hasNormal ? computeNormal(mat.normal) : normalize(inNormal);
  // vec3 N = normalize(inNormal);
  fetchOcclusionTex(material, inUV, dst, mat);
  fetchEmissive(material, inUV, dst, mat);

  outPosition = vec4(inWorldPos, 1.0);
  outNormal = (material.flag == eNone) ? vec4(0.0) : vec4(N, 1.0);
  outAlbedo.rgb = mat.albedo;
  outAlbedo.a = mat.ao;
  outPBR.rg = mat.pbr;
  outEmissive = mat.emissive;
}