#version 450
#extension GL_GOOGLE_include_directive : require
#include "sky.h"

layout(location = 0) out vec3 delta_multiple_scattering;
layout(location = 1) out vec4 scattering;

layout(set = 0, binding = 1) uniform LayerUniform { int layer; };
layout(set = 0, binding = 2) uniform LRRUniform { mat3 luminance_from_radiance; };
layout(set = 0, binding = 3) uniform sampler2D transmittance_texture;
layout(set = 0, binding = 4) uniform sampler3D scattering_density_texture;

void main() {
  float nu;
  delta_multiple_scattering = ComputeMultipleScatteringTexture(
    ATMOSPHERE, transmittance_texture, scattering_density_texture,
    vec3(gl_FragCoord.xy, layer + 0.5), nu);
  scattering = vec4(
    luminance_from_radiance * delta_multiple_scattering.rgb / RayleighPhaseFunction(nu),
    0.0);
}