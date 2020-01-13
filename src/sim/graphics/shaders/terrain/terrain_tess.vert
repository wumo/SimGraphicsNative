#version 450
#extension GL_GOOGLE_include_directive : require
#include "../basic.h"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV0;

layout(set = 0, binding = 1, std430) readonly buffer PrimitivesBuffer {
  PrimitiveUBO primitives[];
};
layout(set = 0, binding = 2, std430) readonly buffer MeshesBuffer {
  MeshInstanceUBO meshes[];
};
layout(set = 0, binding = 3, std430) readonly buffer TransformBuffer {
  mat4 transforms[];
};
layout(set = 0, binding = 4, std430) readonly buffer MaterialBuffer {
  MaterialUBO materials[];
};

layout(location = 0) out vec2 outUV0;
layout(location = 1) out float minHeight;
layout(location = 2) out float heightRange;
layout(location = 3) out int outTessLod;
layout(location = 4) out float outTessLevel;
layout(location = 5) out flat uint outMaterialID;
layout(location = 6) out flat uint outHeightTex;
layout(location = 7) out flat uint outNormalTex;
layout(location = 8) out mat4 outModel;

void main() {
  MeshInstanceUBO mesh = meshes[gl_InstanceIndex];
  PrimitiveUBO primitive = primitives[mesh.primitive];
  MaterialUBO material = materials[mesh.material];

  outUV0 = inUV0;
  minHeight = primitive.min.y;
  heightRange = abs(primitive.max.y - primitive.min.y);
  outTessLod = primitive.lod;
  outTessLevel = primitive.tesselationLevel;
  outMaterialID = mesh.material;
  outHeightTex = material.heightTex;
  outNormalTex = material.normalTex;
  outModel = transforms[mesh.instance] * transforms[mesh.node];

  gl_Position = vec4(inPos, 1.0);
}
