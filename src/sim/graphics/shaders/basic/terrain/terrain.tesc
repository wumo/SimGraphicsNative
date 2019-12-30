#version 450
#extension GL_GOOGLE_include_directive : require
#include "../basic.h"

layout(vertices = 3) out;

layout(location = 0) in vec2 inUV0[];
layout(location = 1) in float inMinHeight[];
layout(location = 2) in float inHeightRange[];
layout(location = 3) in float inTessLevel[];
layout(location = 4) in flat uint inMaterialID[];
layout(location = 5) in flat uint inHeightTex[];
layout(location = 6) in flat uint inNormalTex[];
layout(location = 7) in mat4 inModel[];

layout(location = 0) out vec2 outUV0[3];
layout(location = 1) patch out PatchData data;

void main() {
  if(gl_InvocationID == 0) {
    data.model = inModel[0];
    data.minHeight = inMinHeight[0];
    data.heightRange = inHeightRange[0];
    data.materialID = inMaterialID[0];
    data.heightTex = inHeightTex[0];
    data.normalTex = inNormalTex[0];

    float tessLevel = inTessLevel[0];
    gl_TessLevelInner[0] = tessLevel;
    gl_TessLevelInner[1] = tessLevel;
    gl_TessLevelOuter[0] = tessLevel;
    gl_TessLevelOuter[1] = tessLevel;
    gl_TessLevelOuter[2] = tessLevel;
  }

  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
  outUV0[gl_InvocationID] = inUV0[gl_InvocationID];
}
