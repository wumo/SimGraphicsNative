#ifndef BSDF_TRACE_H
#define BSDF_TRACE_H

#include "trace.h"

float evaluateFresnelDielectric(float et, vec3 N, vec3 rayDir) {
  const float NdotD = dot(N, rayDir);
  if(NdotD > 0.0) et = 1.0 / et;
  const float cosi = abs(NdotD);
  float sint = 1.0f - cosi * cosi;
  sint = (0.0f < sint) ? sqrt(sint) / et : 0.0f;
  if(sint > 1.0) return 1.0;

  // Handle total internal reflection.
  if(1.0f < sint) { return 1.0f; }

  float cost = 1.0f - sint * sint;
  cost = (0.0f < cost) ? sqrt(cost) : 0.0f;

  const float et_cosi = et * cosi;
  const float et_cost = et * cost;

  const float rPerpendicular = (cosi - et_cost) / (cosi + et_cost);
  const float rParallel = (et_cosi - cost) / (et_cosi + cost);

  const float result = (rParallel * rParallel + rPerpendicular * rPerpendicular) * 0.5f;

  return (result <= 1.0f) ? result : 1.0f;
}

vec3 lighting(vec3 worldPos, vec3 rayDir, vec3 N, MaterialInfo mat, Light light) {
  vec3 V = normalize(-rayDir);
  vec3 L = normalize(light.position.xyz - worldPos);
  vec3 H = normalize(V + L);

  uint maxRecursion = prd.maxRecursion;

  //  PBRInfo pbrInputs;
  //  float perceptualRoughness = clamp(mat.pbr.r, 0, 1.0);
  //  pbrInputs.alphaRoughness = perceptualRoughness * perceptualRoughness;
  //  pbrInputs.NdotL = clamp(dot(N, L), 0.001, 1.0);
  //  pbrInputs.NdotV = clamp(abs(dot(N, V)), 0.001, 1.0);
  //  pbrInputs.NdotH = clamp(dot(N, H), 0.0, 1.0);
  //
  //  float G = geometricOcclusion(pbrInputs);
  //  float D = microfacetDistribution(pbrInputs);

  prd.maxRecursion =
    min(prd.recursion + (maxRecursion - prd.recursion) / 3, maxRecursion);
  vec3 reflected = reflectTrace(worldPos, rayDir, N);

  float F = evaluateFresnelDielectric(mat.refractive.r, N, rayDir);
  bool noRefract = equal(F, 1.0); //total internal reflection

  vec3 transmitted = mat.albedo;
  [[branch]] if(!noRefract) {
    prd.maxRecursion = maxRecursion - 1;
    transmitted = refractTrace(worldPos, rayDir, N, 1.0 / mat.refractive.r, mat.albedo);
  }

  //  prd.maxRecursion = noRefract ? prd.recursion : maxRecursion - 1;
  //  vec3 transmitted =
  //    refractTrace(worldPos, rayDir, N, 1.0 / mat.refractive.r, mat.albedo);

  vec3 Lo = vec3(0.0);
  vec3 lightColor = vec3(1);

  prd.maxRecursion = maxRecursion;
  float shadowed = shadowTrace(worldPos, light.position.xyz) ? 0.1 : 1;
  Lo += (transmitted * (1.0 - F) * mat.albedo + reflected * F) * shadowed;
  return Lo;
}

#endif