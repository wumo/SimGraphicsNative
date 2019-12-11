#ifndef FETCH_MATERIALS_H
#define FETCH_MATERIALS_H
#include "common.h"
#include "tonemap.h"

float level(uint tex, vec4 dst, int lodBias) {
  ivec2 wh = textureSize(samplers[nonuniformEXT(tex)], 0);
  float w = wh.x, h = wh.y;
  float p = max(
    sqrt(w * w * dst.x * dst.x + h * h * dst.z * dst.z),
    sqrt(w * w * dst.y * dst.y + h * h * dst.w * dst.w));
  float lambda = isZero(p) ? 0 : log2(ceil(p));
  return max(floor(lambda) + lodBias, 0);
}

void fetchAlbedo(Material material, vec2 uv, vec4 dst, inout MaterialInfo mat) {
  vec2 colorUV =
    uvTransform(uv, vec4(material.cX, material.cY, material.cW, material.cH));
  float L =
    material.colorMaxLod > 0 ? level(material.colorTex, dst, material.colorLodBias) : 0;
  mat.albedo = vec3(material.r, material.g, material.b) *
               SRGBtoLINEAR(textureLod(
                              samplers[nonuniformEXT(material.colorTex)], colorUV,
                              clamp(L, 0, material.colorMaxLod)))
                 .rgb;
}

void fetchPBR(Material material, vec2 uv, vec4 dst, inout MaterialInfo mat) {
  vec2 pbrUV = uvTransform(uv, vec4(material.pX, material.pY, material.pW, material.pH));
  float L = material.pbrMaxLod > 0 ? level(material.pbrTex, dst, material.pbrLodBias) : 0;
  mat.pbr =
    vec2(material.roughness, material.metallic) *
    textureLod(
      samplers[nonuniformEXT(material.pbrTex)], pbrUV, clamp(L, 0, material.pbrMaxLod))
      .gb;
}

void fetchNormal(Material material, vec2 uv, vec4 dst, inout MaterialInfo mat) {
  if(material.normalTex == 0) {
    mat.hasNormal = false;
    return;
  }
  mat.hasNormal = true;
  vec2 normalUV =
    uvTransform(uv, vec4(material.nX, material.nY, material.nW, material.nH));
  float L = material.normalMaxLod > 0 ?
              level(material.normalTex, dst, material.normalLodBias) :
              0;
  mat.normal = textureLod(
                 samplers[nonuniformEXT(material.normalTex)], normalUV,
                 clamp(L, 0, material.normalMaxLod))
                 .rgb;
}

void fetchOcclusionTex(Material material, vec2 uv, vec4 dst, inout MaterialInfo mat) {
  if(material.occlusionTex == 0) {
    mat.isOccluded = false;
    mat.ao = 0;
    return;
  }
  mat.isOccluded = true;
  vec2 occlusionUV =
    uvTransform(uv, vec4(material.oX, material.oY, material.oW, material.oH));
  float L = material.occlusionMaxLod > 0 ?
              level(material.occlusionTex, dst, material.occlusionLodBias) :
              0;
  mat.ao = textureLod(
             samplers[nonuniformEXT(material.occlusionTex)], occlusionUV,
             clamp(L, 0, material.occlusionMaxLod))
             .r;
}

void fetchEmissive(Material material, vec2 uv, vec4 dst, inout MaterialInfo mat) {
  if(material.emissiveTex == 0) {
    mat.isEmissive = false;
    mat.emissive = vec4(0);
    return;
  }
  mat.isEmissive = true;
  vec2 emissiveUV =
    uvTransform(uv, vec4(material.emX, material.emY, material.emW, material.emH));
  float L = material.emissiveMaxLod > 0 ?
              level(material.emissiveTex, dst, material.emissiveLodBias) :
              0;
  mat.emissive.rgb = vec3(material.emR, material.emG, material.emB) *
                     SRGBtoLINEAR(textureLod(
                                    samplers[nonuniformEXT(material.emissiveTex)],
                                    emissiveUV, clamp(L, 0, material.emissiveMaxLod)))
                       .rgb;
}

void fetchReflective(Material material, vec2 uv, vec4 dst, inout MaterialInfo mat) {
  vec2 reflectiveUV =
    uvTransform(uv, vec4(material.rlX, material.rlY, material.rlW, material.rlH));
  mat.isReflective = (material.flag & (eReflective | eRefractive)) != 0;
  float L = material.reflectMaxLod > 0 ?
              level(material.reflectiveTex, dst, material.reflectLodBias) :
              0;
  mat.reflective = textureLod(
    samplers[nonuniformEXT(material.reflectiveTex)], reflectiveUV,
    clamp(L, 0, material.reflectMaxLod));
}

void fetchRefractive(Material material, vec2 uv, vec4 dst, inout MaterialInfo mat) {
  vec2 refractiveUV =
    uvTransform(uv, vec4(material.rrX, material.rrY, material.rrW, material.rrH));
  mat.isRefractive = (material.flag & eRefractive) != 0;
  float L = material.refractMaxLod > 0 ?
              level(material.refractiveTex, dst, material.refractLodBias) :
              0;
  mat.refractive = textureLod(
    samplers[nonuniformEXT(material.refractiveTex)], refractiveUV,
    clamp(L, 0, material.refractMaxLod));
}

#endif