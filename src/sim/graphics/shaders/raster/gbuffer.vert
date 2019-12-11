#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_control_flow_attributes : require
#include "../common.h"

// vertex attributes
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
// instanced attributes;

layout(location = 0) out vs {
  vec3 outWorldPos;
  vec3 outNormal;
  vec2 outUV;
  out flat uint outMaterialID;
};

layout(set = 0, binding = 0) uniform ViewProjection { CameraSetting cam; };
layout(set = 0, binding = 1, std430) readonly buffer TransformBuffer {
  Transform transforms[];
};
layout(set = 0, binding = 2, std430) readonly buffer MeshInstanceBuffer {
  MeshInstance instances[];
};

void main() {
  MeshInstance instance = instances[gl_InstanceIndex];
  vec4 pos = vec4(inPos, 1.0);
  Transform transform = transforms[instance.transform];
  mat4 model = matrix(transform);
  while(transform.parentID != -1) {
    transform = transforms[transform.parentID];
    mat4 parent = matrix(transform);
    model = parent * model;
  }
  pos = model * pos;
  outWorldPos = pos.xyz / pos.w;

  gl_Position = cam.proj * cam.view * vec4(outWorldPos, 1.0);
  gl_Position.y = -gl_Position.y;

  // Normal in world space
  outNormal = normalize(transpose(inverse(mat3(model))) * inNormal);

  outUV = inUV.xy;
  outMaterialID = instance.material;
}
