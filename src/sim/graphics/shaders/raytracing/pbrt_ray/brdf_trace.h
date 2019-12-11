#ifndef BRDF_TRACE_H
#define BRDF_TRACE_H

#include "trace.h"
#include "../../brdf_direct.h"

vec3 lighting(vec3 worldPos, vec3 rayDir, vec3 N, MaterialInfo mat, Light light) {
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

  float reflectance90 = clamp(reflectance * 50.0, 0.0, 1.0);
  vec3 specularEnvironmentR0 = specularColor.rgb;
  vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;

  vec3 V = normalize(-rayDir);
  vec3 L = normalize(light.position.xyz - worldPos);
  vec3 H = normalize(V + L);

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
  vec3 reflected = reflectTrace(worldPos, rayDir, N);
  float shadowed = shadowTrace(worldPos, light.position.xyz) ? 0.1 : 1;
  Lo += (transmitted * (1 - F) + reflected * F) * shadowed;
  return Lo;
}

#endif