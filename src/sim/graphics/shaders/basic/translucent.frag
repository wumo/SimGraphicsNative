#version 450
#extension GL_GOOGLE_include_directive : require
#include "basic.h"
#include "../tonemap.h"

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

void main() {
  MaterialUBO material = materials[inMaterialID];
  vec4 albedo = material.colorTex >= 0 ?
                  texture(textures[material.colorTex], inUV0).rgba :
                  vec4(1, 1, 1, 1);
  albedo = material.baseColorFactor.rgba * albedo;
  if(albedo.a < material.alphaCutoff) discard;
  outColor = albedo;
}