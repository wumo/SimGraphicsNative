#version 450
#extension GL_GOOGLE_include_directive : require
#include "../basic.h"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV0;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outUV0;
layout(location = 2) out flat uint outMeshInstanceID;

void main() {
  outNormal = inNormal;
  outUV0 = inUV0;
  outMeshInstanceID = gl_InstanceIndex;

  gl_Position = vec4(inPos, 1.0);
}
