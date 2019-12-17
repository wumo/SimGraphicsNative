#version 450
#extension GL_GOOGLE_include_directive : require
#include "../basic.h"

layout(constant_id = 0) const uint maxNumTextures = 1;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV0;

layout(set = 0, binding = 0) uniform Camera { CameraUBO cam; };
layout(set = 0, binding = 1, std430) readonly buffer MeshesBuffer { Mesh meshes[]; };
layout(set = 0, binding = 2, std430) readonly buffer TransformBuffer {
  mat4 transforms[];
};
layout(set = 0, binding = 3, std430) readonly buffer MaterialBuffer {
  MaterialUBO materials[];
};
layout(set = 0, binding = 4) uniform sampler2D textures[maxNumTextures];

layout(location = 0) out vs {
  vec3 outWorldPos;
  vec3 outNormal;
  vec2 outUV0;
  out flat uint outMaterialID;
};

out gl_PerVertex { vec4 gl_Position; };

void main() {
  Mesh mesh = meshes[gl_InstanceIndex];
  mat4 model = transforms[mesh.instance] * transforms[mesh.node];
  MaterialUBO material = materials[mesh.material];
  float height = texture(textures[material.heightTex], inUV0).r*10;
  vec3 normal =
    material.normalTex >= 0 ? texture(textures[material.normalTex], inUV0).rgb : inNormal;
  vec4 pos = vec4(inPos, 1.0);
  pos.y += height;
  pos = model * pos;
  outWorldPos = pos.xyz / pos.w;

  outNormal = normalize(transpose(inverse(mat3(model))) * normal);
  outUV0 = inUV0;
  outMaterialID = mesh.material;
  gl_Position = cam.projView * vec4(outWorldPos, 1.0);
  gl_Position.y = -gl_Position.y;
}
