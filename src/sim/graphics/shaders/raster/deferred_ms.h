//
// This fragment shader defines a reference implementation for Physically Based
// Shading of a microfacet surface material defined by a glTF model.
//
// References:
// [1] Real Shading in Unreal Engine 4
//     http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// [2] Physically Based Shading at Disney
//     http://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
// [3] README.md - Environment Maps
//     https://github.com/KhronosGroup/glTF-WebGL-PBR/#environment-maps
// [4] "An Inexpensive BRDF Model for Physically based Rendering" by Christophe
// Schlick
//     https://www.cs.virginia.edu/~jdl/bib/appearance/analytic%20models/schlick94b.pdf
#ifndef DEFERRED_MS_H
#define DEFERRED_MS_H
#include "../common.h"
#include "../tonemap.h"

layout(constant_id = 0) const uint samples = 1;
layout(constant_id = 1) const bool wireFrameEnabled = false;

layout(location = 0) in fs {
  vec2 inUV;
  vec3 viewPos;
  float exposure;
  float gamma;
};

layout(location = 0) out vec4 outColor;

// clang-format off
layout(set = 0, binding = 1,input_attachment_index = 0) uniform subpassInputMS samplerPosition;
layout(set = 0, binding = 2,input_attachment_index = 1) uniform subpassInputMS samplerNormal;
layout(set = 0, binding = 3,input_attachment_index = 2) uniform subpassInputMS samplerAlbedo;
layout(set = 0, binding = 4,input_attachment_index = 3) uniform subpassInputMS samplerPBR;
layout(set = 0, binding = 5,input_attachment_index = 4) uniform subpassInputMS samplerEmissive;
// clang-format on
layout(set = 0, binding = 6, std430) readonly buffer LightBuffer {
  uint numLights;
  Light lights[];
};

bool shadowTrace(vec3 origin, vec3 dir) { return false; }
#include "../brdf_direct.h"

#include "shade.h"

void main() {
  vec3 accumulate = vec3(0);
  // for(int i = 0; i < samples; i++) {
  //   MaterialInfo mat;
  //   vec3 fragPos = subpassLoad(samplerPosition, i).rgb;
  //   vec3 N = subpassLoad(samplerNormal, i).rgb;
  //   mat.albedo = subpassLoad(samplerAlbedo, i).rgb;
  //   mat.pbr = subpassLoad(samplerPBR, i).rg;

  //   accumulate += shade(fragPos, N, mat);
  // }
  MaterialInfo mat;
  vec3 fragPos = subpassLoad(samplerPosition, gl_SampleID).rgb;
  vec3 N = subpassLoad(samplerNormal, gl_SampleID).rgb;
  mat.normal = N;
  vec4 albedo = subpassLoad(samplerAlbedo, gl_SampleID).rgba;
  mat.albedo = albedo.rgb;
  mat.ao = albedo.a;
  mat.pbr = subpassLoad(samplerPBR, gl_SampleID).rg;
  mat.emissive = subpassLoad(samplerEmissive, gl_SampleID).rgba;

  accumulate += shade(fragPos, N, mat);
  outColor.rgb = accumulate;
}
#endif