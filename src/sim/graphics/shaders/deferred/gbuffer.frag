#version 450
#extension GL_GOOGLE_include_directive : require
#include "../basic.h"
#include "../tonemap.h"

layout(constant_id = 0) const uint maxNumTextures = 1;

layout(location = 0) in fs {
  vec3 inWorldPos;
  vec3 inNormal;
  vec2 inUV0;
  flat uint inMaterialID;
};

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outDiffuse;
layout(location = 3) out vec4 outSpecular;
layout(location = 4) out vec4 outEmissive;

layout(set = 0, binding = 4, std430) readonly buffer MaterialBuffer {
  MaterialUBO materials[];
};
layout(set = 0, binding = 5) uniform sampler2D textures[maxNumTextures];

vec3 computeNormal(vec3 sampledNormal) {
  vec3 pos_dx = dFdx(inWorldPos);
  vec3 pos_dy = dFdy(inWorldPos);
  vec3 tex_dx = dFdx(vec3(inUV0, 0.0));
  vec3 tex_dy = dFdy(vec3(inUV0, 0.0));
  vec3 t =
    (tex_dy.t * pos_dx - tex_dx.t * pos_dy) / (tex_dx.s * tex_dy.t - tex_dy.s * tex_dx.t);
  vec3 ng = normalize(inNormal);
  t = normalize(t - ng * dot(ng, t));
  vec3 b = normalize(cross(ng, t));
  mat3 tbn = mat3(t, b, ng);
  return normalize(tbn * (2 * sampledNormal - 1));
}

void main() {
  MaterialUBO material = materials[inMaterialID];
  vec3 albedo = material.colorTex >= 0 ? texture(textures[material.colorTex], inUV0).rgb :
                                         vec3(1, 1, 1);
  albedo = material.baseColorFactor.rgb * albedo;

  vec3 normal;
  if(material.type == MaterialType_None) normal = vec3(0);
  else if(material.type == MaterialType_Terrain) {
    normal = inNormal;
    vec3 pos_dx = dFdx(inWorldPos);
    vec3 pos_dy = dFdy(inWorldPos);
    normal = normalize(cross(pos_dx, -pos_dy));
  } else
    normal =
      material.normalTex >= 0 ?
        computeNormal(LINEARtoSRGB(texture(textures[material.normalTex], inUV0).rgb)) :
        inNormal;

  vec3 diffuseColor;
  vec3 specularColor;
  float perceptualRoughness;
  if(material.type == MaterialType_BRDFSG) {
    vec4 specular = material.pbrTex >= 0 ? texture(textures[material.pbrTex], inUV0) :
                                           vec4(1, 1, 1, 1);
    vec3 f0 = specular.rgb * material.pbrFactor.rgb;
    specularColor = f0;
    float oneMinusSpecularStrength = 1.0 - max(max(f0.r, f0.g), f0.b);
    perceptualRoughness =
      (1.0 - specular.w * material.pbrFactor.w); // glossiness to roughness
    diffuseColor = albedo * oneMinusSpecularStrength;
  } else {
    vec2 pbr = material.pbrTex >= 0 ?
                 LINEARtoSRGB(texture(textures[material.pbrTex], inUV0).rgb).gb :
                 vec2(1, 1);
    pbr = material.pbrFactor.gb * pbr;
    perceptualRoughness = pbr.x;
    float metallic = pbr.y;
    vec3 f0 = vec3(0.04);
    diffuseColor = albedo * (vec3(1.0) - f0) * (1.0 - metallic);
    specularColor = mix(f0, albedo, metallic);
  }
  float ao = material.occlusionTex >= 0 ?
               LINEARtoSRGB(texture(textures[material.occlusionTex], inUV0).rgb).r :
               1;
  ao = mix(1, ao, material.occlusionStrength);

  vec3 emissive = material.emissiveTex >= 0 ?
                    texture(textures[material.emissiveTex], inUV0).rgb :
                    vec3(0, 0, 0);
  emissive = material.emissiveFactor.rgb * emissive;

  int applyIBL = 1;
  if(material.type == MaterialType_None) applyIBL = 0;
  outPosition.rgb = inWorldPos;
  outNormal = vec4(normal, applyIBL);
  outDiffuse = vec4(diffuseColor, ao);
  outSpecular = vec4(specularColor, perceptualRoughness);
  outEmissive.rgb = emissive;
}