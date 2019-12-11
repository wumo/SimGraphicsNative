#ifndef TRACE_H
#define TRACE_H

void trace(vec3 origin, vec3 dir, float tmin, float tmax) {
  const uint rayFlags = gl_RayFlagsOpaqueNV;
  const uint cullMask = 0xFF;

  prd.recursion++;
  traceNV(Scene, rayFlags, cullMask, 0, 1, 0, origin, tmin, dir, tmax, 0);
}

bool shadowTrace(vec3 origin, vec3 lightPos) {
  if(prd.recursion >= prd.maxRecursion) return false;
  const uint shadowRayFlags = gl_RayFlagsTerminateOnFirstHitNV | gl_RayFlagsOpaqueNV |
                              gl_RayFlagsSkipClosestHitShaderNV;
  const uint cullMask = 0xFF;
  const float tmin = 1e-3;
  const float tmax = length(lightPos - origin);
  vec3 dir = normalize(lightPos - origin);
  prdShadow.shadowed = true;
  prd.recursion++;
  traceNV(Scene, shadowRayFlags, cullMask, 1, 1, 1, origin, tmin, dir, tmax, 2);
  return prdShadow.shadowed;
}
vec3 reflectTrace(vec3 origin, vec3 rayDir, vec3 N) {
  if(prd.recursion >= prd.maxRecursion) return vec3(0);
  vec3 dir = reflect(rayDir, N);
  const float tmin = 1e-3;
  const float tmax = cam.zFar;
  trace(origin, dir, tmin, tmax);
  return prd.color.xyz;
}

vec3 refractTrace(vec3 origin, vec3 rayDir, vec3 N, float eta, vec3 failColor) {
  if(prd.recursion >= prd.maxRecursion) return failColor;
  const float NdotD = dot(N, rayDir);

  vec3 refrNormal = N;
  float refrEta;
  if(NdotD > 0.0f) {
    refrNormal = -N;
    refrEta = 1.0f / eta;
  } else {
    refrNormal = N;
    refrEta = eta;
  }
  vec3 dir = refract(rayDir, refrNormal, refrEta);
  const float tmin = 1e-3;
  const float tmax = cam.zFar;
  trace(origin, dir, tmin, tmax);
  return prd.color.xyz / (refrEta * refrEta);
}

#endif