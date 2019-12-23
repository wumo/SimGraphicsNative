#version 450
#extension GL_GOOGLE_include_directive : require
#include "sky.h"

layout(location = 0) in fs { vec2 inUV; };

layout(location = 0) out vec3 delta_irradiance;
layout(location = 1) out vec3 irradiance;

layout(set = 0, binding = 1) uniform sampler2D transmittance_texture;

void main() {
  delta_irradiance =
    ComputeDirectIrradianceTexture(ATMOSPHERE, transmittance_texture, gl_FragCoord.xy);
  irradiance = vec3(0.0);
}