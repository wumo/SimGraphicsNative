#version 450
#extension GL_GOOGLE_include_directive : require
#include "../basic.h"

layout(quads, equal_spacing, cw) in;

layout(location = 0) in vec2 inUV0[];
layout(location = 1) patch in PatchData data;

layout(location = 0) out vs {
  vec3 outWorldPos;
  vec3 outNormal;
  vec2 outUV0;
  out flat uint outMaterialID;
};

layout(constant_id = 0) const uint maxNumTextures = 1;

layout(set = 0, binding = 0) uniform Camera { CameraUBO cam; };
layout(set = 0, binding = 5) uniform sampler2D textures[maxNumTextures];

void main() {
  outMaterialID = data.materialID;
  vec2 uv1 = mix(inUV0[0], inUV0[1], gl_TessCoord.x);
  vec2 uv2 = mix(inUV0[3], inUV0[2], gl_TessCoord.x);
  outUV0 = mix(uv1, uv2, gl_TessCoord.y);

  mat4 model = data.model;

  vec4 pos1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
  vec4 pos2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
  vec4 pos = mix(pos1, pos2, gl_TessCoord.y);
  float height =
    data.minHeight + texture(textures[data.heightTex], outUV0).r * data.heightRange;
  pos.y += height;
  pos = model * pos;
  outWorldPos = pos.xyz / pos.w;

  vec3 normal = texture(textures[data.normalTex], outUV0).rgb;
  outNormal = normalize(transpose(inverse(mat3(model))) * normal);

  gl_Position = cam.projView * vec4(outWorldPos, 1.0);
  gl_Position.y = -gl_Position.y;
}