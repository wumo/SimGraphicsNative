#version 450
#extension GL_GOOGLE_include_directive : require
#include "sky.h"
layout(location = 0) in fs { vec2 inUV; };
layout(location = 0) out vec3 transmittance;
void main() {
  transmittance =
    ComputeTransmittanceToTopAtmosphereBoundaryTexture(ATMOSPHERE, gl_FragCoord.xy);
}