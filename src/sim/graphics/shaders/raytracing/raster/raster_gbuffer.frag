#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#include "../../common.h"

layout(location = 0) in fs {
  vec3 inWorldPos;
  vec3 inNormal;
  vec2 inUV;
  flat uint materialID;
};

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outAlbedo;
layout(location = 3) out vec2 outPBR;

layout(set = 0, binding = 0) uniform CameraSettingUniform { CameraSetting cam; };
layout(set = 0, binding = 4, std430) readonly buffer MaterialBuffer {
  Material materials[];
};
layout(set = 0, binding = 5) uniform sampler2D samplers[];
layout(set = 1, binding = 4) uniform sampler2D depth;

void main() {
  vec4 pos = cam.proj * cam.view * vec4(inWorldPos, 1.0);
  float d = texture(depth, gl_FragCoord.xy / vec2(textureSize(depth, 0))).r;
  if(pos.z >= d) discard;
  outPosition = vec4(inWorldPos, 1.0);

  vec3 N = inNormal;
  outNormal = vec4(N, 1.0);

  Material m = materials[materialID];
  vec2 colorUV = uvTransform(inUV, vec4(m.cX, m.cY, m.cW, m.cH));
  vec2 pbrUV = uvTransform(inUV, vec4(m.pX, m.pY, m.pW, m.pH));
  vec2 normalUV = uvTransform(inUV, vec4(m.nX, m.nY, m.nW, m.nH));

  outAlbedo.rgb =
    vec3(m.r, m.g, m.b) * texture(samplers[nonuniformEXT(m.colorTex)], colorUV).rgb;
  outPBR.rg =
    vec2(m.roughness, m.metallic) * texture(samplers[nonuniformEXT(m.pbrTex)], pbrUV).gb;
}