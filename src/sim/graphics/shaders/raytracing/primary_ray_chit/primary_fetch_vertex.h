#ifndef PRIMARY_FETCH_VERTEX_H
#define PRIMARY_FETCH_VERTEX_H

hitAttributeNV vec2 hit;
void fetchVertexInfo(
  in MeshInstance instance, out vec3 worldPos, out vec3 normal, out vec2 uv,
  out vec4 dst) {
  worldPos = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV;

  const vec3 barycentrics = vec3(1.0f - hit.x - hit.y, hit.x, hit.y);

  Mesh mesh = meshes[instance.mesh];

  const uint offset = mesh.indexOffset + 3 * gl_PrimitiveID;
  const uvec3 face = uvec3(
    indices[offset] + mesh.vertexOffset, indices[offset + 1] + mesh.vertexOffset,
    indices[offset + 2] + mesh.vertexOffset);
  Vertex v0 = unpack(vertices[int(face.x)]);
  Vertex v1 = unpack(vertices[int(face.y)]);
  Vertex v2 = unpack(vertices[int(face.z)]);
  if(v0.normal.x != 0 || v0.normal.y != 0 || v0.normal.z != 0)
    normal = BaryLerp(v0.normal.xyz, v1.normal.xyz, v2.normal.xyz, barycentrics);
  else
    normal = cross(v1.position - v0.position, v2.position - v0.position);

  mat3 mNormal = transpose(inverse(mat3(gl_ObjectToWorldNV)));
  normal = mNormal * normal;
  normal = normalize(normal);

  uv = BaryLerp(v0.uv.xy, v1.uv.xy, v2.uv.xy, barycentrics);

  dst = vec4(0, 0, 0, 0);
}

#endif