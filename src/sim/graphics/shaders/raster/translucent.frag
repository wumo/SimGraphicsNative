#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#include "../common.h"

layout(location = 0) in fs {
  vec2 inUV;
  flat uint materialID;
};

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 3, std430) readonly buffer MaterialBuffer {
  Material materials[];
};
layout(set = 0, binding = 4) uniform sampler2D samplers[];

void main() {
  Material m = materials[materialID];
  vec2 colorUV = uvTransform(inUV, vec4(m.cX, m.cY, m.cW, m.cH));

  vec4 color = texture(samplers[nonuniformEXT(m.colorTex)], colorUV);

  outColor.rgb = vec3(m.r, m.g, m.b) * color.rgb;
  outColor.a = color.a;
}