#version 450
#extension GL_GOOGLE_include_directive : require
#include "../basic.h"

layout(location = 0) in vec2 inUV0[];
layout(location = 1) in float inMinHeight[];
layout(location = 2) in float inHeightRange[];
layout(location = 3) in float inTessLevel[];
layout(location = 4) in flat uint inMaterialID[];
layout(location = 5) in flat uint inHeightTex[];
layout(location = 6) in flat uint inNormalTex[];
layout(location = 7) in mat4 inModel[];

layout(vertices = 4) out;
layout(location = 0) out vec2 outUV0[4];
layout(location = 1) patch out PatchData data;

layout(constant_id = 0) const uint maxNumTextures = 1;

layout(set = 0, binding = 0) uniform Camera { CameraUBO cam; };
layout(set = 0, binding = 5) uniform sampler2D textures[maxNumTextures];

float calcTessLevel(
  vec4 p0, vec4 p1, vec2 uv0, vec2 uv1, float tessWidth, mat4 modelView) {
  vec4 center = (p0 + p1) / 2;
  float radius = distance(p0, p1) / 2;

  vec4 sc0 = modelView * center;
  vec4 sc1 = sc0;
  sc0.x -= radius;
  sc1.x += radius;

  vec4 clip0 = cam.proj * sc0;
  vec4 clip1 = cam.proj * sc1;

  clip0 /= clip0.w;
  clip1 /= clip1.w;

  clip0.xy *= vec2(cam.w, cam.h);
  clip1.xy *= vec2(cam.w, cam.h);

  float d = distance(clip0, clip1);

  // g_tessellatedTriWidth is desired pixels per tri edge
  return clamp(d / tessWidth, 1, 64);
}

void main() {
  if(gl_InvocationID == 0) {
    data.model = inModel[0];
    data.minHeight = inMinHeight[0];
    data.heightRange = inHeightRange[0];
    data.materialID = inMaterialID[0];
    data.heightTex = inHeightTex[0];
    data.normalTex = inNormalTex[0];
    float tessWidth = inTessLevel[0];

    mat4 modelView = cam.view * data.model;

    vec4 p0 = gl_in[0].gl_Position;
    p0.y +=
      data.minHeight + texture(textures[data.heightTex], inUV0[0]).r * data.heightRange;
    vec4 p1 = gl_in[1].gl_Position;
    p1.y +=
      data.minHeight + texture(textures[data.heightTex], inUV0[1]).r * data.heightRange;
    vec4 p2 = gl_in[2].gl_Position;
    p2.y +=
      data.minHeight + texture(textures[data.heightTex], inUV0[2]).r * data.heightRange;
    vec4 p3 = gl_in[3].gl_Position;
    p3.y +=
      data.minHeight + texture(textures[data.heightTex], inUV0[3]).r * data.heightRange;

    gl_TessLevelOuter[0] =
      calcTessLevel(p3, p0, inUV0[3], inUV0[0], tessWidth, modelView);
    gl_TessLevelOuter[1] =
      calcTessLevel(p0, p1, inUV0[0], inUV0[1], tessWidth, modelView);
    gl_TessLevelOuter[2] =
      calcTessLevel(p1, p2, inUV0[1], inUV0[2], tessWidth, modelView);
    gl_TessLevelOuter[3] =
      calcTessLevel(p2, p3, inUV0[2], inUV0[3], tessWidth, modelView);

    gl_TessLevelInner[0] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[3], 0.5);
    gl_TessLevelInner[1] = mix(gl_TessLevelOuter[2], gl_TessLevelOuter[1], 0.5);
  }

  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
  outUV0[gl_InvocationID] = inUV0[gl_InvocationID];
}
