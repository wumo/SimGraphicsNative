#ifndef BRDF_DIRECT_H
#define BRDF_DIRECT_H
#include "common.h"
#define Disney
//#define Lambert

const float MinRoughness = 0.04;

// Basic Lambertian diffuse
// Implementation from Lambert's Photometria
// https://archive.org/details/lambertsphotome00lambgoog See also [1],
// Equation 1
#if defined(Lambert)
vec3 diffuse(PBRInfo pbrInputs) { return pbrInputs.diffuseColor / PI; }
#elif defined(Disney)
vec3 diffuse(PBRInfo pbrInputs) {
  float f90 = 2.0 * pbrInputs.LdotH * pbrInputs.LdotH * pbrInputs.alphaRoughness - 0.5;

  return (pbrInputs.diffuseColor / PI) * (1.0 + f90 * pow((1.0 - pbrInputs.NdotL), 5.0)) *
         (1.0 + f90 * pow((1.0 - pbrInputs.NdotV), 5.0));
}
#endif

// The following equation models the Fresnel reflectance term of the spec
// equation (aka F()) Implementation of fresnel from [4], Equation 15
vec3 specularReflection(PBRInfo pbrInputs) {
  return pbrInputs.reflectance0 + (pbrInputs.reflectance90 - pbrInputs.reflectance0) *
                                    pow(clamp(1.0 - pbrInputs.VdotH, 0.0, 1.0), 5.0);
}

// This calculates the specular geometric attenuation (aka G()),
// where rougher material will reflect less light back to the viewer.
// This implementation is based on [1] Equation 4, and we adopt their
// modifications to alphaRoughness as input as originally proposed in [2].
float geometricOcclusion(PBRInfo pbrInputs) {
  float NdotL = pbrInputs.NdotL;
  float NdotV = pbrInputs.NdotV;
  float r = pbrInputs.alphaRoughness;

  float attenuationL =
    2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
  float attenuationV =
    2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));
  return attenuationL * attenuationV;
}

// The following equation(s) model the distribution of microfacet normals
// across the area being drawn (aka D()) Implementation from "Average
// Irregularity Representation of a Roughened Surface for Ray Reflection" by T.
// S. Trowbridge, and K. P. Reitz Follows the distribution function recommended
// in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(PBRInfo pbrInputs) {
  float roughnessSq = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;
  float f = (pbrInputs.NdotH * roughnessSq - pbrInputs.NdotH) * pbrInputs.NdotH + 1.0;
  return roughnessSq / (PI * f * f);
}

vec3 blinnPhong(vec3 fragPos, vec3 N, vec3 V, MaterialInfo mat, Light light) {
  vec3 fragcolor = vec3(0.0);
  vec3 L = normalize(light.position.xyz - fragPos);
  float diffuse = max(dot(N, L), 0.0);
  vec3 H = normalize(L + V);
  float specular = pow(max(dot(N, H), 0.0), 16.0);
  fragcolor += diffuse * mat.albedo + specular * vec3(1.0, 1.0, 1.0);
  return fragcolor;
}

bool shadowTrace(vec3 origin, vec3 lightPos);
vec3 getIBLContribution(PBRInfo pbrInputs, vec3 n, vec3 reflection);

vec3 brdf_direct(vec3 worldPos, vec3 rayDir, vec3 N, MaterialInfo mat, Light light) {
  float perceptualRoughness = mat.pbr.r;
  float metallic = mat.pbr.g;

  perceptualRoughness = clamp(perceptualRoughness, MinRoughness, 1.0);
  metallic = clamp(metallic, 0.0, 1.0);
  float alphaRoughness = perceptualRoughness * perceptualRoughness;

  vec3 f0 = vec3(0.04);
  vec3 diffuseColor = mat.albedo.rgb * (vec3(1.0) - f0);
  diffuseColor *= 1.0 - metallic;
  vec3 specularColor = mix(f0, mat.albedo.rgb, metallic);

  // Compute reflectance.
  float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
  // For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
  // For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
  float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
  vec3 specularEnvironmentR0 = specularColor.rgb;
  vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;

  vec3 V = normalize(-rayDir);
  vec3 L = normalize(light.position.xyz - worldPos);
  vec3 H = normalize(V + L);
  vec3 reflection = -normalize(reflect(V, N));
  reflection.y *= -1.0f;

  float NdotL = clamp(dot(N, L), 0.001, 1.0);
  float NdotV = clamp(abs(dot(N, V)), 0.001, 1.0);
  float NdotH = clamp(dot(N, H), 0.0, 1.0);
  float LdotH = clamp(dot(L, H), 0.0, 1.0);
  float VdotH = clamp(dot(V, H), 0.0, 1.0);

  PBRInfo pbrInputs = PBRInfo(
    NdotL, NdotV, NdotH, LdotH, VdotH, perceptualRoughness, metallic,
    specularEnvironmentR0, specularEnvironmentR90, alphaRoughness, diffuseColor,
    specularColor);

  vec3 Lo = vec3(0.0);

  vec3 F = specularReflection(pbrInputs);
  float G = geometricOcclusion(pbrInputs);
  float D = microfacetDistribution(pbrInputs);

  vec3 lightColor = vec3(1);
  vec3 transmitted = NdotL * lightColor * diffuse(pbrInputs);
  vec3 reflected = NdotL * lightColor * G * D / (4.0 * NdotL * NdotV);
  vec3 indirect = getIBLContribution(pbrInputs, N, reflection);
  float shadowed = shadowTrace(worldPos, light.position.xyz) ? 0.1 : 1;
  Lo += (transmitted * (1 - F) + reflected * F + indirect) * shadowed;
  return Lo;
}

#endif