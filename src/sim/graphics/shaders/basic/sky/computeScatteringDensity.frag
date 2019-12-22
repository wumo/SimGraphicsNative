#version 450
#extension GL_GOOGLE_include_directive : require
#include "sky.h"

layout(location = 0) out vec3 scattering_density;

layout(set = 0, binding = 1) uniform LayerUniform { int layer; };
layout(set = 0, binding = 2) uniform ScatteringUniform { int scattering_order; };
layout(set = 0, binding = 3) uniform sampler2D transmittance_texture;
layout(set = 0, binding = 4) uniform sampler3D single_rayleigh_scattering_texture;
layout(set = 0, binding = 5) uniform sampler3D single_mie_scattering_texture;
layout(set = 0, binding = 6) uniform sampler3D multiple_scattering_texture;
layout(set = 0, binding = 7) uniform sampler2D irradiance_texture;

void main() {
  scattering_density = ComputeScatteringDensityTexture(
    ATMOSPHERE, transmittance_texture, single_rayleigh_scattering_texture,
    single_mie_scattering_texture, multiple_scattering_texture, irradiance_texture,
    vec3(gl_FragCoord.xy, layer + 0.5), scattering_order);
}