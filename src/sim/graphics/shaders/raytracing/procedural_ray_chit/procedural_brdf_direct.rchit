#version 460
#extension GL_NV_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_control_flow_attributes : require

#include "../ray_chit.h"
#include "procedural_fetch_vertex.h"
bool shadowTrace(vec3 origin, vec3 dir) { return false; }
vec3 getIBLContribution(PBRInfo pbrInputs, vec3 n, vec3 reflection) { return vec3(0); }
#include "../../brdf_direct.h"
vec3 lighting(vec3 worldPos, vec3 rayDir, vec3 N, MaterialInfo mat, Light light) {
  return brdf_direct(worldPos, rayDir, N, mat, light);
}
