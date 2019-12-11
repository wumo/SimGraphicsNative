#version 460
#extension GL_GOOGLE_include_directive : require
#include "../../common.h"

layout(location = 0) out vs {
  vec2 outUV;
  vec3 viewPos;
  float exposure;
  float gamma;
};

layout(set = 0, binding = 0) uniform CameraSettingUniform { CameraSetting cam; };

out gl_PerVertex { vec4 gl_Position; };

void main() {
  outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  viewPos = vec3(cam.viewInverse * vec4(0, 0, 0, 1));
  exposure=cam.exposure;
  gamma=cam.exposure;
  gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
}