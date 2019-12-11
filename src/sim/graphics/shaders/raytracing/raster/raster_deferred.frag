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
#version 460
#extension GL_GOOGLE_include_directive : require
#include "../../common.h"

layout(constant_id = 0) const bool wireFrameEnabled = false;

layout(location = 0) in fs {
  vec2 inUV;
  vec3 viewPos;
  float exposure;
  float gamma;
};

layout(location = 0) out vec4 outColor;

// clang-format off
layout(input_attachment_index =0, binding = 0, set = 1) uniform subpassInput samplerPosition;
layout(input_attachment_index = 1, binding = 1, set = 1) uniform subpassInput samplerNormal;
layout(input_attachment_index = 2, binding = 2, set = 1) uniform subpassInput samplerAlbedo;
layout(input_attachment_index = 3, binding = 3, set = 1) uniform subpassInput samplerPBR;
// clang-format on
layout(set = 0, binding = 1, std430) readonly buffer LightBuffer {
  uint numLights;
  Light lights[];
};

bool shadowTrace(vec3 origin, vec3 dir) { return false; }
#include "../../brdf_direct.h"

vec3 Uncharted2Tonemap(vec3 x) {
  float A = 0.15;
  float B = 0.50;
  float C = 0.10;
  float D = 0.20;
  float E = 0.02;
  float F = 0.30;
  return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}
void main() {
  // Read G-Buffer values from previous sub pass
  MaterialInfo mat;
  vec3 fragPos = subpassLoad(samplerPosition).rgb;
  vec3 N = subpassLoad(samplerNormal).rgb;
  mat.albedo = subpassLoad(samplerAlbedo).rgb;
  mat.pbr = subpassLoad(samplerPBR).rg;

  if(fragPos.x == 0 && fragPos.y == 0 && fragPos.z == 0) discard;
  vec3 fragcolor = mat.albedo;
  // Tone mapping
  fragcolor = Uncharted2Tonemap(fragcolor * exposure);
  fragcolor = fragcolor * (1.0f / Uncharted2Tonemap(vec3(11.2f)));
  // Gamma correction
  fragcolor = pow(fragcolor, vec3(1.0f / gamma));

  outColor = vec4(fragcolor, 1.0);
}