#version 450
#extension GL_GOOGLE_include_directive : require
#include "basic.h"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV0;

layout(set = 0, binding = 0) uniform Camera { CameraUBO cam; };
layout(set = 0, binding = 1, std430) readonly buffer PrimitivesBuffer {
  PrimitiveUBO primitives[];
};
layout(set = 0, binding = 2, std430) readonly buffer MeshesBuffer {
  MeshInstanceUBO meshes[];
};
layout(set = 0, binding = 3, std430) readonly buffer TransformBuffer {
  mat4 transforms[];
};

layout(location = 0) out vs {
  vec3 outWorldPos;
  vec3 outNormal;
  vec2 outUV0;
  out flat uint outMaterialID;
};

out gl_PerVertex { vec4 gl_Position; };

void main() {
  MeshInstanceUBO mesh = meshes[gl_InstanceIndex];
  mat4 model = transforms[mesh.instance] * transforms[mesh.node];
  vec4 pos = model * vec4(inPos, 1.0);
  outWorldPos = pos.xyz / pos.w;
  outNormal = normalize(transpose(inverse(mat3(model))) * inNormal);
  outUV0 = inUV0;
  outMaterialID = mesh.material;
  gl_Position = cam.projView * vec4(outWorldPos, 1.0);
  gl_Position.y = -gl_Position.y;
}
