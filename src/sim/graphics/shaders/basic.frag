// PBR shader based on the Khronos WebGL PBR implementation
// See https://github.com/KhronosGroup/glTF-WebGL-PBR
// Supports both metallic roughness and specular glossiness inputs

#version 450
#extension GL_GOOGLE_include_directive : require
#include "basic.h"
#include "tonemap.h"

layout(constant_id = 0) const uint maxNumTextures = 1;

layout(location = 0) in fs {
  vec3 inWorldPos;
  vec3 inNormal;
  vec2 inUV0;
  flat uint inMaterialID;
};

layout(set = 0, binding = 0) uniform Camera { CameraUBO cam; };
layout(set = 0, binding = 3, std430) readonly buffer MaterialBuffer {
  MaterialUBO materials[];
};
layout(set = 0, binding = 4) uniform sampler2D textures[maxNumTextures];
layout(set = 0, binding = 5) uniform LightingUBO { LightUBO lighting; };
layout(set = 0, binding = 6, std430) readonly buffer LightsBuffer {
  LightInstanceUBO lights[];
};
layout(location = 0) out vec4 outColor;

// bool shadowTrace(vec3 origin, vec3 dir) { return false; }
// vec3 getIBLContribution(PBRInfo pbrInputs, vec3 n, vec3 reflection) { return vec3(0); }
// #include "../brdf_direct.h"

// vec3 computeNormal(vec3 N) {
//   vec3 tangentNormal = N * 2.0 - 1.0;

//   vec3 q1 = dFdx(inWorldPos);
//   vec3 q2 = dFdy(inWorldPos);
//   vec2 st1 = dFdx(inUV0);
//   vec2 st2 = dFdy(inUV0);

//   N = normalize(inNormal);
//   vec3 T = normalize(q1 * st2.t - q2 * st1.t);
//   vec3 B = -normalize(cross(N, T));
//   mat3 TBN = mat3(T, B, N);

//   return normalize(TBN * tangentNormal);
// }

void main() {
  MaterialUBO material = materials[inMaterialID];
  vec3 albedo = material.colorTex >= 0 ? texture(textures[material.colorTex], inUV0).rgb :
                                         vec3(1, 1, 1);
  albedo = material.baseColorFactor.rgb * albedo;
  // MaterialInfo mat;

  // mat.albedo = node.baseColorFactor.rgb * texture(colorMap, inUV0).rgb;
  // mat.pbr.rg = node.pbrFactor.gb * texture(pbrMap, inUV0).gb;
  // mat.ao = texture(aoMap, inUV0).r;
  // mat.emissive.rgb = node.emissiveFactor.rgb * texture(emissiveMap, inUV0).rgb;

  // vec3 fragPos = inWorldPos;
  // vec3 viewPos = vec3(cam.viewInverse * vec4(0, 0, 0, 1));
  // vec3 N = computeNormal(texture(normalMap, inUV0).xyz);
  // vec3 rayDir = normalize(fragPos - viewPos);
  // vec3 fragcolor = vec3(0);
  // for(int i = 0; i < numLights; i++) {
  //   Light light = lights[i];
  //   fragcolor += brdf_direct(fragPos, rayDir, N, mat, light);
  // }
  // if(mat.ao > 0) fragcolor = mix(fragcolor, fragcolor * mat.ao, 1.0);
  // fragcolor += mat.emissive.rgb;
  // // Tone mapping
  // fragcolor = toneMap(vec4(fragcolor, 1.0), cam.exposure).rgb;
  outColor.rgb = albedo;
}