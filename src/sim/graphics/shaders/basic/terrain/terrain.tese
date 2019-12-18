#version 450
#extension GL_GOOGLE_include_directive : require
#include "../basic.h"

layout(constant_id = 0) const uint maxNumTextures = 1;

layout(triangles, equal_spacing, cw) in;

layout(location = 0) in vec3 inNormal[];
layout(location = 1) in vec2 inUV0[];
layout(location = 2) patch in PatchData data;

layout(location = 0) out vs {
  vec3 outWorldPos;
  vec3 outNormal;
  vec2 outUV0;
  out flat uint outMaterialID;
};

layout(set = 0, binding = 0) uniform Camera { CameraUBO cam; };
layout(set = 0, binding = 5) uniform sampler2D textures[maxNumTextures];

void main() {
  outMaterialID = data.materialID;
  outUV0 = BaryLerp(inUV0[0], inUV0[1], inUV0[2], gl_TessCoord);
  outNormal = BaryLerp(inNormal[0], inNormal[1], inNormal[2], gl_TessCoord);
  vec4 pos = BaryLerp(
    gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position, gl_TessCoord);

  mat4 model = data.model;
  float height = data.minHeight + texture(textures[data.heightTex], outUV0).r *
                                    (data.maxHeight - data.minHeight);
  pos.y += height;
  pos = model * pos;
  outWorldPos = pos.xyz / pos.w;

  vec3 normal = data.normalTex >= 0 ? texture(textures[data.normalTex], outUV0).rgb :
                                      outNormal;
  outNormal = normalize(transpose(inverse(mat3(model))) * normal);

  gl_Position = cam.projView * vec4(outWorldPos, 1.0);
  gl_Position.y = -gl_Position.y;
}