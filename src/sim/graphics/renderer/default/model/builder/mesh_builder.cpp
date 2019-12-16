#include "mesh_builder.h"
#include "sim/util/syntactic_sugar.h"
#include "sim/graphics/renderer/default/model/model_manager.h"

namespace sim::graphics::renderer::defaultRenderer {
using namespace glm;

MeshBuilder::MeshBuilder(
  uint32_t baseVertexOffset, uint32_t baseIndexOffset, uint32_t baseMeshID)
  : baseVertexOffset{baseVertexOffset},
    baseIndexOffset{baseIndexOffset},
    baseMeshID{baseMeshID} {
  newMesh();
}

std::vector<Vertex> &MeshBuilder::getVertices() { return vertices; }
std::vector<uint32_t> &MeshBuilder::getIndices() { return indices; }
std::vector<Mesh> &MeshBuilder::getMeshes() { return meshes; }

uint32_t MeshBuilder::newMesh(Primitive primitive) {
  meshes.push_back({uint32_t(baseIndexOffset + indices.size()), 0,
                    uint32_t(baseVertexOffset + vertices.size()), 0, primitive});
  return currentMeshID();
}

uint32_t MeshBuilder::currentMeshID() { return uint32_t(baseMeshID + meshes.size() - 1); }

uint32_t MeshBuilder::currentVertexID() const {
  return uint32_t(vertices.size()) - (meshes.back().vertexOffset - baseVertexOffset);
}

MeshBuilder &MeshBuilder::from(
  std::vector<Vertex> &vertices, std::vector<uint32_t> &indices, Primitive primitive,
  bool checkIndexInRange) {
  auto maxVertexID = vertices.size() - 1;
  if(checkIndexInRange)
    for(auto index: indices)
      errorIf(index > maxVertexID, "index out of range");

  ensurePrimitive(primitive);
  const auto vertexID = currentVertexID();
  if(vertexID > 0)
    for(auto index: indices)
      this->indices.push_back(index + vertexID);
  else
    append(this->indices, indices);
  append(this->vertices, vertices);
  updateCurrentMesh();
  return *this;
}

MeshBuilder &MeshBuilder::from(
  std::vector<glm::vec3> &vertexPositions, std::vector<uint32_t> &indices,
  Primitive primitive, bool checkIndexInRange) {

  std::vector<Vertex> vertices;
  vertices.reserve(vertexPositions.size());
  for(auto &pos: vertexPositions)
    vertices.push_back({pos});
  return from(vertices, indices, primitive, checkIndexInRange);
}

mat3 transform(const mat3 &X, const mat3 &Y) { return Y * inverse(X); }

void applyTransform(
  par_shapes_mesh *m, const mat3 &transform, const vec3 translate = {}) {
  auto points = m->points;
  for(auto i = 0; i < m->npoints; i++, points += 3) {
    const auto p = transform * vec3(points[0], points[1], points[2]) + translate;
    points[0] = p.x;
    points[1] = p.y;
    points[2] = p.z;
  }

  points = m->normals;
  if(points) {
    for(auto i = 0; i < m->npoints; i++, points += 3) {
      const auto p = normalize(transform * vec3(points[0], points[1], points[2]));
      points[0] = p.x;
      points[1] = p.y;
      points[2] = p.z;
    }
  }
}

void MeshBuilder::updateCurrentMesh() {
  auto &currentMesh = meshes.back();
  currentMesh.indexCount =
    baseIndexOffset + uint32_t(indices.size()) - currentMesh.indexOffset;
  currentMesh.vertexCount = uint32_t(vertices.size());
}

MeshBuilder &MeshBuilder::triangle(const vec3 p1, const vec3 p2, const vec3 p3) {
  ensurePrimitive(Primitive ::Triangles);
  const auto normal = normalize(cross((p2 - p1), (p3 - p1)));
  const auto vertexID = currentVertexID();
  append(
    vertices,
    {{p1, normal, {0.f, 0.f}}, {p2, normal, {1.0f, 0.f}}, {p3, normal, {0.f, 1.0f}}});
  append(indices, {vertexID, vertexID + 1, vertexID + 2});
  updateCurrentMesh();
  return *this;
}

MeshBuilder &MeshBuilder::rectangle(const vec3 center, const vec3 x, const vec3 y) {
  ensurePrimitive(Primitive ::Triangles);
  const auto normal = normalize(cross(x, y));
  const auto p1 = center + x + y;
  const auto p2 = center - x + y;
  const auto p3 = center - x - y;
  const auto p4 = center + x - y;
  const auto vertexID = currentVertexID();
  append(
    vertices, {{p1, normal, {0.f, 0.f}},
               {p2, normal, {1.0f, 0.f}},
               {p3, normal, {1.0f, 1.f}},
               {p4, normal, {0.0f, 1.f}}});
  append(
    indices,
    {vertexID, vertexID + 1, vertexID + 2, vertexID, vertexID + 2, vertexID + 3});
  updateCurrentMesh();
  return *this;
}

MeshBuilder &MeshBuilder::gridMesh(
  const uint32_t nx, const uint32_t ny, const vec3 center, const vec3 x, const vec3 y,
  const float wx, const float wy) {
  ensurePrimitive(Primitive ::Triangles);
  auto _x = normalize(x);
  auto _y = normalize(y);
  auto normal = cross(_x, _y);
  auto origin = center + _x * (nx * wx / 2.f) - _y * (ny * wy / 2.f);
  _x *= -wx;
  _y *= wy;
  auto u = 1.f / nx;
  auto v = 1.f / ny;
  const auto vertexID = currentVertexID();
  for(uint32_t row = 0; row <= ny; ++row)
    for(uint32_t column = 0; column <= nx; ++column)
      append(
        vertices,
        {{origin + float(column) * _x + float(row) * _y, normal, {column * u, row * v}}});
  for(uint32_t row = 0; row < ny; ++row)
    for(uint32_t column = 0; column < nx; ++column) {
      auto s0 = (nx + 1) * row;
      auto s1 = (nx + 1) * (row + 1);
      append(
        indices,
        {s0 + column + vertexID, s1 + column + vertexID, s1 + column + 1 + vertexID,
         s0 + column + vertexID, s1 + column + 1 + vertexID, s0 + column + 1 + vertexID});
    }
  updateCurrentMesh();
  return *this;
}

MeshBuilder &MeshBuilder::box(vec3 center, vec3 x, vec3 y, float half_z) {
  ensurePrimitive(Primitive ::Triangles);
  auto z = normalize(cross(x, y)) * half_z;
  rectangle(center + x, y, z);  // front
  rectangle(center - x, -y, z); // back
  rectangle(center + z, y, -x); // top
  rectangle(center - z, y, x);  // bottom
  rectangle(center + y, z, x);  // right
  rectangle(center - y, z, -x); // left
  return *this;
}

MeshBuilder &MeshBuilder::circle(vec3 center, vec3 z, float R, int segments) {
  ensurePrimitive(Primitive ::Triangles);
  auto right = cross(z, {0, 0, 1});
  if(epsilonEqual(length(right), 0.f, epsilon<float>())) right = {1, 0, 0};
  right = normalize(right) * R;
  auto angle = radians(360.f / segments);
  z = normalize(z);
  auto rotation = angleAxis(angle, z);
  auto vertexID = currentVertexID();
  append(
    vertices, {{center, z, {0.5f, 0.5f}},
               {center + right, z, {0.5f + 0.5f * sin(0.f), 0.5f + 0.5f * cos(0.f)}}});
  for(int i = 2; i <= segments; ++i) {
    right = rotation * right;
    vertices.push_back(
      {right + center,
       z,
       {0.5f + 0.5f * sin((i - 1) * angle), 0.5f + 0.5f * cos((i - 1) * angle)}});
    append(indices, {vertexID, vertexID + i - 1, vertexID + i});
  }
  append(indices, {vertexID, vertexID + segments, vertexID + 1});
  updateCurrentMesh();
  return *this;
}

MeshBuilder &MeshBuilder::cone(
  const vec3 center, const vec3 z, const float R, const int segments) {
  ensurePrimitive(Primitive ::Triangles);
  circle(center, -z, R, segments);
  auto right = cross(z, {0, 0, 1});
  if(epsilonEqual(length(right), 0.f, epsilon<float>())) right = {1, 0, 0};
  right = normalize(right) * R;
  auto angle = radians(360.f / segments);
  auto rotation = angleAxis(angle, normalize(z));
  auto vertexID = currentVertexID();

  auto uintZ = normalize(z);
  for(int i = 0; i < segments; ++i) {
    //    auto N = normalize(cross(rotation * right - right, z - right));
    vertices.push_back({center + z, uintZ, {0.5f, 0.5f}});
    vertices.push_back({center + right,
                        right,
                        {0.5f + 0.5f * sin(i * angle), 0.5f + 0.5f * cos(i * angle)}});
    right = rotation * right;
    vertices.push_back(
      {right + center,
       right,
       {0.5f + 0.5f * sin((i + 1) * angle), 0.5f + 0.5f * cos((i + 1) * angle)}});
    append(indices, {vertexID + 3 * i, vertexID + 3 * i + 1, vertexID + 3 * i + 2});
  }
  updateCurrentMesh();
  return *this;
}

MeshBuilder &MeshBuilder::cylinder(
  const vec3 center, const vec3 z, const float R, const bool cap, const int segments) {
  ensurePrimitive(Primitive ::Triangles);
  auto right = cross(z, {0, 0, 1});
  if(epsilonEqual(length(right), 0.f, epsilon<float>())) right = {1, 0, 0};
  right = normalize(right) * R;
  auto angle = radians(360.f / segments);
  auto rotation = angleAxis(angle, normalize(z));
  auto vertexID = currentVertexID();

  for(int i = 0; i < segments; ++i) {
    //    auto N = normalize(right + rotation * right);
    vertices.push_back({center + right + z, right, {1.f / segments * i, 0.f}});
    vertices.push_back({center + right, right, {1.f / segments * i, 0.f}});
    right = rotation * right;
    vertices.push_back({center + right, right, {1.f / segments * (i + 1), 0.f}});
    vertices.push_back({center + right + z, right, {1.f / segments * (i + 1), 0.f}});
    append(
      indices, {vertexID + 4 * i, vertexID + 4 * i + 1, vertexID + 4 * i + 2,
                vertexID + 4 * i, vertexID + 4 * i + 2, vertexID + 4 * i + 3});
  }
  if(cap) {
    circle(center, -z, R, segments);
    circle(center + z, z, R, segments);
  }
  updateCurrentMesh();
  return *this;
}

MeshBuilder &MeshBuilder::sphere(vec3 center, float R, int nsubd) {
  ensurePrimitive(Primitive ::Triangles);
  auto mesh = par_shapes_create_subdivided_sphere(nsubd);
  par_shapes_scale(mesh, R, R, R);
  par_shapes_translate(mesh, center.x, center.y, center.z);
  if(mesh->normals == nullptr) throw std::runtime_error("mesh normals is null!");
  auto vid = 0;
  auto points = mesh->points;
  auto normals = mesh->normals;
  auto uvs = mesh->tcoords;
  auto vertexID = currentVertexID();
  for(auto i = 0; i < mesh->npoints; ++i, vid += 3) {
    vertices.push_back(
      Vertex{{points[vid], points[vid + 1], points[vid + 2]},
             {normals[vid], normals[vid + 1], normals[vid + 2]},
             uvs ? vec3{uvs[vid], uvs[vid + 1], 0.f} : vec3{0.0f, 0.0f, 0.f}});
  }
  auto triangles = mesh->triangles;
  vid = 0;
  for(auto i = 0; i < mesh->ntriangles; ++i, vid += 3)
    append(
      indices, {triangles[vid] + vertexID, triangles[vid + 1] + vertexID,
                triangles[vid + 2] + vertexID});
  updateCurrentMesh();
  par_shapes_free_mesh(mesh);
  return *this;
}

MeshBuilder &MeshBuilder::axis(
  const vec3 center, const float length, const float R, float capLength,
  const int segments) {
  ensurePrimitive(Primitive ::Triangles);
  sphere(center, R, 1);
  newMesh();
  // axis X: Red
  const auto offset = 0;
  auto pos = vec3{-offset, 0, 0};
  auto z = vec3{length + offset, 0, 0};
  cylinder(pos + center, z, R / 2, false, segments);
  pos = {length, 0, 0};
  z = {capLength, 0, 0};
  cone(pos + center, z, R, segments);
  newMesh();
  // axis Y: Green
  pos = {0, -offset, 0};
  z = {0, length + offset, 0};
  cylinder(pos + center, z, R / 2, false, segments);
  pos = {0, length, 0};
  z = {0, capLength, 0};
  cone(pos + center, z, R, segments);
  newMesh();
  // axis Z: Blue
  pos = {0, 0, -offset};
  z = {0, 0, length + offset};
  cylinder(pos + center, z, R / 2, false, segments);
  pos = {0, 0, length};
  z = {0, 0, capLength};
  cone(pos + center, z, R, segments);
  return *this;
}

MeshBuilder &MeshBuilder::grid(
  const uint32_t nx, const uint32_t ny, const vec3 center, const vec3 x, const vec3 y,
  const float wx, const float wy) {
  ensurePrimitive(Primitive ::Lines);
  auto _x = normalize(x);
  auto _y = normalize(y);
  auto normal = cross(_x, _y);
  auto origin = center + _x * (nx * wx / 2.f) - _y * (ny * wy / 2.f);
  _x *= -wx;
  _y *= wy;
  auto u = 1.f / nx;
  auto v = 1.f / ny;
  const auto vertexID = currentVertexID();
  for(uint32_t row = 0; row <= ny; ++row)
    for(uint32_t column = 0; column <= nx; ++column)
      append(
        vertices,
        {{origin + float(column) * _x + float(row) * _y, normal, {column * u, row * v}}});
  for(uint32_t row = 0; row < ny; ++row)
    for(uint32_t column = 0; column < nx; ++column) {
      auto s0 = (nx + 1) * row;
      auto s1 = (nx + 1) * (row + 1);
      append(
        indices,
        {s0 + column + vertexID, s1 + column + vertexID, s1 + column + vertexID,
         s1 + column + 1 + vertexID, s0 + column + vertexID, s0 + column + 1 + vertexID,
         s1 + column + 1 + vertexID, s0 + column + 1 + vertexID});
    }
  updateCurrentMesh();
  return *this;
}

MeshBuilder &MeshBuilder::line(vec3 p1, vec3 p2) {
  ensurePrimitive(Primitive ::Lines);
  const auto vertexID = currentVertexID();
  append(vertices, {{p1}, {p2}});
  append(indices, {vertexID, vertexID + 1});
  updateCurrentMesh();
  return *this;
}

MeshBuilder &MeshBuilder::rectangleLine(vec3 center, vec3 x, vec3 y) {
  ensurePrimitive(Primitive ::Lines);
  const auto p1 = center + x + y;
  const auto p2 = center - x + y;
  const auto p3 = center - x - y;
  const auto p4 = center + x - y;
  const auto vertexID = currentVertexID();
  append(vertices, {{p1}, {p2}, {p3}, {p4}});
  append(
    indices, {vertexID, vertexID + 1, vertexID + 1, vertexID + 2, vertexID + 2,
              vertexID + 3, vertexID + 3, vertexID});
  updateCurrentMesh();
  return *this;
}

MeshBuilder &MeshBuilder::boxLine(vec3 center, vec3 x, vec3 y, float half_z) {
  ensurePrimitive(Primitive ::Lines);
  const auto z = normalize(cross(x, y)) * half_z;
  const auto f1 = center + x + y - z;
  const auto f2 = center - x + y - z;
  const auto f3 = center - x - y - z;
  const auto f4 = center + x - y - z;
  const auto b1 = center + x + y + z;
  const auto b2 = center - x + y + z;
  const auto b3 = center - x - y + z;
  const auto b4 = center + x - y + z;

  const auto vertexID = currentVertexID();
  append(vertices, {{f1}, {f2}, {f3}, {f4}, {b1}, {b2}, {b3}, {b4}});
  append(indices, {vertexID,     vertexID + 1, vertexID + 1, vertexID + 2, vertexID + 2,
                   vertexID + 3, vertexID + 3, vertexID,     vertexID + 4, vertexID + 5,
                   vertexID + 5, vertexID + 6, vertexID + 6, vertexID + 7, vertexID + 7,
                   vertexID + 4, vertexID,     vertexID + 4, vertexID + 1, vertexID + 5,
                   vertexID + 2, vertexID + 6, vertexID + 3, vertexID + 7});
  updateCurrentMesh();
  return *this;
}

void MeshBuilder::ensurePrimitive(Primitive type) {
  auto &mesh = meshes.back();
  if(mesh.primitive != type) {
    if(mesh.indexCount > 0) newMesh(type);
    else
      mesh.primitive = type;
  }
}

}
