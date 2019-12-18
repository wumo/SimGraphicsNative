#version 450
#extension GL_GOOGLE_include_directive : require
#include "../basic.h"

layout(constant_id = 0) const uint maxNumTextures = 1;

layout(vertices = 3) out;

layout(location = 0) in vec3 inNormal[];
layout(location = 1) in vec2 inUV0[];
layout(location = 2) in flat uint inMeshInstanceID[];

layout(location = 0) out vec3 outNormal[3];
layout(location = 1) out vec2 outUV0[3];
layout(location = 2) patch out PatchData data;

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
layout(set = 0, binding = 4, std430) readonly buffer MaterialBuffer {
  MaterialUBO materials[];
};
layout(set = 0, binding = 5) uniform sampler2D textures[maxNumTextures];

void main() {
  if(gl_InvocationID == 0) {
    MeshInstanceUBO mesh = meshes[inMeshInstanceID[0]];
    PrimitiveUBO primitive = primitives[mesh.primitive];
    mat4 model = transforms[mesh.instance] * transforms[mesh.node];
    MaterialUBO material = materials[mesh.material];
    data.model = model;
    data.materialID = mesh.material;
    data.minHeight = primitive.min.y;
    data.maxHeight = primitive.max.y;
    data.heightTex = material.heightTex;
    data.normalTex = material.normalTex;

    float tessLevel = primitive.tesselationLevel;
    // Tessellation factor can be set to zero by example
    // to demonstrate a simple passthrough
    gl_TessLevelInner[0] = tessLevel;
    gl_TessLevelInner[1] = tessLevel;
    gl_TessLevelOuter[0] = tessLevel;
    gl_TessLevelOuter[1] = tessLevel;
    gl_TessLevelOuter[2] = tessLevel;
  }

  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
  outNormal[gl_InvocationID] = inNormal[gl_InvocationID];
  outUV0[gl_InvocationID] = inUV0[gl_InvocationID];
}
