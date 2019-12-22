#version 450
#extension GL_GOOGLE_include_directive : require
#include "sky.h"

layout(location = 0) out vec3 delta_rayleigh;
layout(location = 1) out vec3 delta_mie;
layout(location = 2) out vec4 scattering;
layout(location = 3) out vec3 single_mie_scattering;

layout(set = 0, binding = 1) uniform LayerUniform { int layer; };
layout(set = 0, binding = 2) uniform LFRUniform { mat3 luminance_from_radiance; };
layout(set = 0, binding = 3) uniform sampler2D transmittance_texture;

void main() {
  ComputeSingleScatteringTexture(
    ATMOSPHERE, transmittance_texture, vec3(gl_FragCoord.xy, layer + 0.5), delta_rayleigh,
    delta_mie);
  scattering = vec4(
    luminance_from_radiance * delta_rayleigh.rgb,
    (luminance_from_radiance * delta_mie).r);
  single_mie_scattering = luminance_from_radiance * delta_mie;
}