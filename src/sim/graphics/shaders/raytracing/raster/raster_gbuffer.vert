#version 460
#extension GL_GOOGLE_include_directive : require
#include "../../common.h"

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

layout(set = 0, binding = 0) uniform CameraSettingUniform { CameraSetting cam; };
layout(set = 0, binding = 2, std430) readonly buffer TransformBuffer {
  Transform transforms[];
};
layout(set = 0, binding = 3, std430) readonly buffer MeshInstanceBuffer {
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

  gl_Position = cam.proj * cam.view * model * pos;
  gl_Position.y = -gl_Position.y;

  // Vertex position in world space
  outWorldPos = vec3(model * pos);

  // Normal in world space
  mat3 mNormal = transpose(inverse(mat3(model)));
  outNormal = mNormal * inNormal;

  outUV = inUV.xy;
  outMaterialID = instance.material;
}
