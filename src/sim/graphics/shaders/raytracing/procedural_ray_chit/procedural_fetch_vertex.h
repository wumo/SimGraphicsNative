#ifndef PROCEDURAL_FETCH_VERTEX
#define PROCEDURAL_FETCH_VERTEX

hitAttributeNV ProcedurePayload hit;

void fetchVertexInfo(
  in MeshInstance instance, out vec3 worldPos, out vec3 normal, out vec2 uv,
  out vec4 dst) {
  worldPos = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV;
  const vec3 barycentrics = vec3(1.0f - hit.uv.x - hit.uv.y, hit.uv.x, hit.uv.y);
  Mesh mesh = meshes[instance.mesh];
  normal = BaryLerp(hit.v0.normal, hit.v1.normal, hit.v2.normal, barycentrics);
  mat3 mNormal = transpose(inverse(mat3(gl_ObjectToWorldNV)));
  normal = mNormal * normal;
  normal = normalize(normal);

  uv = BaryLerp(hit.v0.uv, hit.v1.uv, hit.v2.uv, barycentrics);

  vec3 d = gl_WorldRayDirectionNV;
  float t = gl_HitTNV;

  vec3 e1 = normalize(hit.v1.position - hit.v0.position);
  vec3 e2 = normalize(hit.v2.position - hit.v0.position);

  vec3 cu = cross(e2, d);
  vec3 cv = cross(d, e1);
  vec3 q = prd.dOdx + t * prd.dDdx;
  vec3 r = prd.dOdy + t * prd.dDdy;
  float k = dot(cross(e1, e2), d);

  float dudx = dot(cu, q) / k;
  float dudy = dot(cu, r) / k;
  float dvdx = dot(cv, q) / k;
  float dvdy = dot(cv, r) / k;

  vec2 g1 = hit.v1.uv - hit.v0.uv;
  vec2 g2 = hit.v2.uv - hit.v0.uv;
  dst.x = dudx * g1.x + dvdx * g2.x;
  dst.y = dudy * g1.x + dvdy * g2.x;
  dst.z = dudx * g1.y + dvdx * g2.y;
  dst.w = dudy * g1.y + dvdy * g2.y;
}

#endif