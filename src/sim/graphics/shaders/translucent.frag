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
layout(set = 0, binding = 4, std430) readonly buffer MaterialBuffer {
  MaterialUBO materials[];
};
layout(set = 0, binding = 5) uniform sampler2D textures[maxNumTextures];
layout(set = 0, binding = 6) uniform LightingUBO { LightUBO lighting; };
layout(set = 0, binding = 7, std430) readonly buffer LightsBuffer {
  LightInstanceUBO lights[];
};
layout(location = 0) out vec4 outColor;

#define LIGHTS_NUM lighting.numLights
#define LIGHTS_BUFFER lights
#include "brdf.h"

void main() {
  MaterialUBO material = materials[inMaterialID];
  vec4 albedo = material.colorTex >= 0 ?
                  texture(textures[material.colorTex], inUV0).rgba :
                  vec4(1, 1, 1, 1);
  albedo = material.baseColorFactor.rgba * albedo;
  if(albedo.a < material.alphaCutoff) discard;
  outColor.a = albedo.a;

  vec3 diffuseColor;
  vec3 specularColor;
  float perceptualRoughness;
  vec2 pbr = material.pbrTex >= 0 ?
               LINEARtoSRGB(texture(textures[material.pbrTex], inUV0).rgb).gb :
               vec2(1, 1);
  pbr = material.pbrFactor.gb * pbr;
  perceptualRoughness = pbr.x;
  float metallic = pbr.y;
  vec3 f0 = vec3(0.04);
  diffuseColor = albedo.rgb * (vec3(1.0) - f0) * (1.0 - metallic);
  specularColor = mix(f0, albedo.rgb, metallic);

  if(isZero(inNormal.x) && isZero(inNormal.y) && isZero(inNormal.z)) {
    outColor.rgb = LINEARtoSRGB(diffuseColor);
    return;
  }

  float ao = material.occlusionTex >= 0 ?
               LINEARtoSRGB(texture(textures[material.occlusionTex], inUV0).rgb).r :
               1;
  ao = mix(1, ao, material.occlusionStrength);

  vec3 emissive = material.emissiveTex >= 0 ?
                    texture(textures[material.emissiveTex], inUV0).rgb :
                    vec3(0, 0, 0);
  emissive = material.emissiveFactor.rgb * emissive;
  vec3 color = shadeBRDF(
    inWorldPos, inNormal, diffuseColor, ao, specularColor, perceptualRoughness, emissive,
    0, cam.eye.xyz);

  outColor.rgb = color;
}