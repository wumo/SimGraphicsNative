#version 450
#extension GL_GOOGLE_include_directive : require
#include "sky.h"

layout(location = 0) out vec3 delta_irradiance;
layout(location = 1) out vec3 irradiance;

layout(set = 0, binding = 1) uniform LRRUniform { mat3 luminance_from_radiance; };
layout(set = 0, binding = 2) uniform ScatteringUniform { int scattering_order; };
layout(set = 0, binding = 3) uniform sampler3D single_rayleigh_scattering_texture;
layout(set = 0, binding = 4) uniform sampler3D single_mie_scattering_texture;
layout(set = 0, binding = 5) uniform sampler3D multiple_scattering_texture;

void main() {
  delta_irradiance = ComputeIndirectIrradianceTexture(
    ATMOSPHERE, single_rayleigh_scattering_texture, single_mie_scattering_texture,
    multiple_scattering_texture, gl_FragCoord.xy, scattering_order);
  irradiance = luminance_from_radiance * delta_irradiance;
}