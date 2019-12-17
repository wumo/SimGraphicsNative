#ifndef BASIC_DEFERRED_H
#define BASIC_DEFERRED_H

#include "basic.h"
#include "../tonemap.h"

layout(location = 0) out vec4 outColor;

#ifdef MULTISAMPLE
  #define SUBPASS_INPUT subpassInputMS
  #define SUBPASS_LOAD(sampler) subpassLoad(sampler, gl_SampleID)
#else
  #define SUBPASS_INPUT subpassInput
  #define SUBPASS_LOAD(sampler) subpassLoad(sampler)
#endif

// clang-format off
layout(set = 1, binding = 0,input_attachment_index = 0) uniform SUBPASS_INPUT samplerPosition;
layout(set = 1, binding = 1,input_attachment_index = 1) uniform SUBPASS_INPUT samplerNormal;
layout(set = 1, binding = 2,input_attachment_index = 2) uniform SUBPASS_INPUT samplerDiffuse;
layout(set = 1, binding = 3,input_attachment_index = 3) uniform SUBPASS_INPUT samplerSpecular;
layout(set = 1, binding = 4,input_attachment_index = 4) uniform SUBPASS_INPUT samplerEmissive;
// clang-format on

layout(set = 0, binding = 0) uniform Camera { CameraUBO cam; };
layout(set = 0, binding = 6) uniform LightingUBO { LightUBO lighting; };
layout(set = 0, binding = 7, std430) readonly buffer LightsBuffer {
  LightInstanceUBO lights[];
};

#define LIGHTS_NUM lighting.numLights
#define LIGHTS_BUFFER lights
#ifdef USE_IBL
layout(set = 2, binding = 0) uniform samplerCube samplerIrradiance;
layout(set = 2, binding = 1) uniform samplerCube prefilteredMap;
layout(set = 2, binding = 2) uniform sampler2D samplerBRDFLUT;
  #define DIFFUSE_ENV_SAMPLER samplerIrradiance
  #define SPECULAR_ENV_SAMPLER prefilteredMap
  #define BRDFLUT samplerBRDFLUT
  #define USE_TEX_LOD
  #define USE_HDR
#endif
#include "brdf.h"

void main() {
  vec3 postion = SUBPASS_LOAD(samplerPosition).rgb;
  vec4 normalIBL = SUBPASS_LOAD(samplerNormal).rgba;
  vec3 normal = normalIBL.xyz;
  float useIBL = normalIBL.a;
  vec4 diffuseAO = SUBPASS_LOAD(samplerDiffuse);
  vec3 diffuseColor = diffuseAO.rgb;
  float ao = diffuseAO.w;
  vec4 specularRoughness = SUBPASS_LOAD(samplerSpecular);
  vec3 specularColor = specularRoughness.rgb;
  float perceptualRoughness = specularRoughness.w;
  vec3 emissive = SUBPASS_LOAD(samplerEmissive).rgb;

  if(isZero(normal.x) && isZero(normal.y) && isZero(normal.z)) {
    outColor.rgb = LINEARtoSRGB(diffuseColor);
    return;
  }

  vec3 color = shadeBRDF(
    postion, normal, diffuseColor, ao, specularColor, perceptualRoughness, emissive,
    useIBL, cam.eye.xyz);

  // outColor.rgb = vec3(perceptualRoughness);
  // outColor.rgb = vec3(metallic);
  // outColor.rgb = normal;
  // outColor.rgb = LINEARtoSRGB(baseColor.rgb);
  // outColor.rgb = vec3(ao);
  // outColor.rgb = LINEARtoSRGB(emissive);
  // outColor.rgb = vec3(f0);
  //  outColor = toneMap(vec4(color, 1.0), lighting.exposure);
  outColor.rgb = LINEARtoSRGB(color);
}

#endif
