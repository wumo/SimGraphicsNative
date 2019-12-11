#ifndef RAY_CHIT_H
#define RAY_CHIT_H

#include "../common.h"

layout(set = 0, binding = 0) uniform accelerationStructureNV Scene;
layout(set = 0, binding = 3, std430) readonly buffer VertexBuffer { vec4[2] vertices[]; };
layout(set = 0, binding = 4, std430) readonly buffer IndexBuffer { uint indices[]; };
layout(set = 0, binding = 5, std430) readonly buffer MeshBuffer { Mesh meshes[]; };

layout(set = 1, binding = 0) uniform CameraSettingUniform { CameraSetting cam; };
layout(set = 1, binding = 1, std430) readonly buffer LightBuffer {
  uint numLights;
  Light lights[];
};
layout(set = 1, binding = 3, std430) readonly buffer MeshInstanceTriBuffer {
  MeshInstance instances[];
};
layout(set = 1, binding = 4, std430) readonly buffer MaterialBuffer {
  Material materials[];
};
layout(set = 1, binding = 5) uniform sampler2D samplers[];

layout(location = 0) rayPayloadInNV RayPayload prd;
layout(location = 2) rayPayloadInNV ShadowRayPayload prdShadow;

layout(constant_id = 0) const int NUM_LIGHTS = 1;

#include "../fetch_materials.h"

void fetchVertexInfo(
  in MeshInstance instance, out vec3 worldPos, out vec3 normal, out vec2 uv,
  out vec4 dst);

vec3 lighting(vec3 worldPos, vec3 rayDir, vec3 N, MaterialInfo mat, Light light);

void main() {
  MeshInstance instance = instances[gl_InstanceCustomIndexNV];
  vec3 worldPos, normal;
  vec2 uv;
  vec4 dst;
  fetchVertexInfo(instance, worldPos, normal, uv, dst);

  Material material = materials[instance.material];
  MaterialInfo mat;
  fetchAlbedo(material, uv, dst, mat);
  fetchPBR(material, uv, dst, mat);
#ifdef FETCH_REFRACTIVE
  fetchRefractive(material, uv, dst, mat);
#endif

  vec3 fragColor = vec3(0, 0, 0);
  for(int i = 0; i < numLights; i++) {
    Light light = lights[i];
    fragColor += lighting(worldPos, gl_WorldRayDirectionNV, normal, mat, light);
  }
  prd.hitT = gl_HitTNV;
  prd.color = fragColor;
}

#endif