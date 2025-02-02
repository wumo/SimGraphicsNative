#include "primitive_builder.h"
#include "sim/util/syntactic_sugar.h"
#include "sim/graphics/renderer/basic/basic_scene_manager.h"

namespace sim::graphics::renderer::basic {
using namespace glm;

PrimitiveBuilder::PrimitiveBuilder(BasicSceneManager &mm): mm(mm) {}

const std::vector<Vertex::Position> &PrimitiveBuilder::positions() const {
  return _positions;
}
const std::vector<Vertex::Normal> &PrimitiveBuilder::normals() const { return _normals; }
const std::vector<Vertex::UV> &PrimitiveBuilder::uvs() const { return _uvs; }
const std::vector<uint32_t> &PrimitiveBuilder::indices() const { return _indices; }
const std::vector<Primitive::UBO> &PrimitiveBuilder::primitives() const {
  return _primitives;
}

PrimitiveBuilder &PrimitiveBuilder::newPrimitive(
  PrimitiveTopology topology, DynamicType type) {
  Range indexRange;
  Range positionRange, normalRange, uvRange;
  if(_primitives.empty()) {
    indexRange = {0, uint32_t(_indices.size())};
    positionRange = {0, uint32_t(_positions.size())};
    normalRange = {0, uint32_t(_normals.size())};
    uvRange = {0, uint32_t(_uvs.size())};
  } else {
    auto &last = _primitives.back();
    indexRange = {last._index.endOffset(),
                  uint32_t(_indices.size()) - last._index.endOffset()};
    positionRange = {last._position.endOffset(),
                     uint32_t(_positions.size()) - last._position.endOffset()};
    normalRange = {last._normal.endOffset(),
                   uint32_t(_normals.size()) - last._normal.endOffset()};
    uvRange = {last._uv.endOffset(), uint32_t(_uvs.size()) - last._uv.endOffset()};
  }
  Primitive::UBO primitive{indexRange, positionRange, normalRange, uvRange};
  primitive._aabb = aabb;
  primitive._topology = topology;
  primitive._type = type;
  _primitives.push_back(primitive);
  aabb = {};
  return *this;
}

uint32_t PrimitiveBuilder::currentVertexID() const {
  return uint32_t(_positions.size()) -
         (_primitives.empty() ? 0u : _primitives.back()._position.endOffset());
}

PrimitiveBuilder &PrimitiveBuilder::from(
  const std::vector<Vertex::Position> &positions,
  const std::vector<Vertex::Normal> &normals, const std::vector<Vertex::UV> &uvs,
  std::vector<uint32_t> &indices, PrimitiveTopology primitive) {

  const auto vertexID = currentVertexID();
  if(vertexID > 0)
    for(auto index: indices)
      this->_indices.push_back(index + vertexID);
  else
    append(this->_indices, indices);

  append(this->_positions, positions);
  append(this->_normals, normals);
  append(this->_uvs, uvs);

  for(const auto &position: positions)
    aabb.merge(position);
  return *this;
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

PrimitiveBuilder &PrimitiveBuilder::triangle(
  const vec3 p1, const vec3 p2, const vec3 p3) {
  const auto normal = normalize(cross((p2 - p1), (p3 - p1)));
  const auto vertexID = currentVertexID();
  append(_positions, {p1, p2, p3});
  append(_normals, {normal, normal, normal});
  append(_uvs, {{0.f, 0.f}, {1.0f, 0.f}, {0.f, 1.0f}});
  append(_indices, {vertexID, vertexID + 1, vertexID + 2});
  aabb.merge(p1, p2, p3);
  return *this;
}

PrimitiveBuilder &PrimitiveBuilder::rectangle(
  const vec3 center, const vec3 x, const vec3 y) {
  const auto normal = normalize(cross(x, y));
  const auto p1 = center + x + y;
  const auto p2 = center - x + y;
  const auto p3 = center - x - y;
  const auto p4 = center + x - y;
  const auto vertexID = currentVertexID();
  append(_positions, {p1, p2, p3, p4});
  append(_normals, {normal, normal, normal, normal});
  append(_uvs, {{0.f, 0.f}, {1.0f, 0.f}, {1.f, 1.0f}, {0.0f, 1.f}});
  append(
    _indices,
    {vertexID, vertexID + 1, vertexID + 2, vertexID, vertexID + 2, vertexID + 3});
  aabb.merge(p1, p2, p3, p4);
  return *this;
}

PrimitiveBuilder &PrimitiveBuilder::box(vec3 center, vec3 x, vec3 y, float half_z) {
  auto z = normalize(cross(x, y)) * half_z;
  rectangle(center + x, y, z);  // front
  rectangle(center - x, -y, z); // back
  rectangle(center + z, y, -x); // top
  rectangle(center - z, y, x);  // bottom
  rectangle(center + y, z, x);  // right
  rectangle(center - y, z, -x); // left
  return *this;
}

PrimitiveBuilder &PrimitiveBuilder::circle(vec3 center, vec3 z, float R, int segments) {
  auto right = cross(z, {0, 0, 1});
  if(epsilonEqual(length(right), 0.f, epsilon<float>())) right = {1, 0, 0};
  right = normalize(right) * R;
  auto angle = radians(360.f / segments);
  z = normalize(z);
  auto rotation = angleAxis(angle, z);
  auto vertexID = currentVertexID();
  append(_positions, {center, center + right});
  append(_normals, {z, z});
  append(_uvs, {{0.5f, 0.5f}, {0.5f + 0.5f * sin(0.f), 0.5f + 0.5f * cos(0.f)}});
  aabb.merge(center, center + right);
  for(int i = 2; i <= segments; ++i) {
    right = rotation * right;
    append(_positions, {right + center});
    append(_normals, {z});
    append(
      _uvs, {{0.5f + 0.5f * sin((i - 1) * angle), 0.5f + 0.5f * cos((i - 1) * angle)}});
    aabb.merge(right + center);
    append(_indices, {vertexID, vertexID + i - 1, vertexID + i});
  }
  append(_indices, {vertexID, vertexID + segments, vertexID + 1});
  return *this;
}

PrimitiveBuilder &PrimitiveBuilder::cone(
  const vec3 center, const vec3 z, const float R, const int segments) {
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
    append(_positions, {center + z, center + right});
    append(_normals, {uintZ, right});
    append(
      _uvs, {{0.5f, 0.5f}, {0.5f + 0.5f * sin(i * angle), 0.5f + 0.5f * cos(i * angle)}});
    aabb.merge(center + z, center + right);
    right = rotation * right;

    append(_positions, {right + center});
    append(_normals, {right});
    append(
      _uvs, {{0.5f + 0.5f * sin((i + 1) * angle), 0.5f + 0.5f * cos((i + 1) * angle)}});
    aabb.merge(right + center);
    append(_indices, {vertexID + 3 * i, vertexID + 3 * i + 1, vertexID + 3 * i + 2});
  }
  return *this;
}

PrimitiveBuilder &PrimitiveBuilder::cylinder(
  const vec3 center, const vec3 z, const float R, const bool cap, const int segments) {
  auto right = cross(z, {0, 0, 1});
  if(epsilonEqual(length(right), 0.f, epsilon<float>())) right = {1, 0, 0};
  right = normalize(right) * R;
  auto angle = radians(360.f / segments);
  auto rotation = angleAxis(angle, normalize(z));
  auto vertexID = currentVertexID();

  for(int i = 0; i < segments; ++i) {
    //    auto N = normalize(right + rotation * right);
    append(_positions, {center + right + z, center + right});
    append(_normals, {right, right});
    append(_uvs, {{1.f / segments * i, 0.f}, {1.f / segments * i, 0.f}});
    aabb.merge(center + right + z, center + right);
    right = rotation * right;
    append(_positions, {center + right, center + right + z});
    append(_normals, {right, right});
    append(_uvs, {{1.f / segments * (i + 1), 0.f}, {1.f / segments * (i + 1), 0.f}});
    aabb.merge(center + right, center + right + z);
    append(
      _indices, {vertexID + 4 * i, vertexID + 4 * i + 1, vertexID + 4 * i + 2,
                 vertexID + 4 * i, vertexID + 4 * i + 2, vertexID + 4 * i + 3});
  }
  if(cap) {
    circle(center, -z, R, segments);
    circle(center + z, z, R, segments);
  }
  return *this;
}

PrimitiveBuilder &PrimitiveBuilder::sphere(vec3 center, float R, int nsubd) {
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
    append(_positions, {{points[vid], points[vid + 1], points[vid + 2]}});
    append(_normals, {{normals[vid], normals[vid + 1], normals[vid + 2]}});
    append(_uvs, {uvs ? vec3{uvs[vid], uvs[vid + 1], 0.f} : vec3{0.0f, 0.0f, 0.f}});

    aabb.merge(_positions.back());
  }
  auto triangles = mesh->triangles;
  vid = 0;
  for(auto i = 0; i < mesh->ntriangles; ++i, vid += 3)
    append(
      _indices, {triangles[vid] + vertexID, triangles[vid + 1] + vertexID,
                 triangles[vid + 2] + vertexID});
  par_shapes_free_mesh(mesh);
  return *this;
}

PrimitiveBuilder &PrimitiveBuilder::axis(
  const vec3 center, const float length, const float R, float capLength,
  const int segments) {
  sphere(center, R, 1);
  newPrimitive();
  // axis X: Red
  const auto offset = 0;
  auto pos = vec3{-offset, 0, 0};
  auto z = vec3{length + offset, 0, 0};
  cylinder(pos + center, z, R / 2, false, segments);
  pos = {length, 0, 0};
  z = {capLength, 0, 0};
  cone(pos + center, z, R, segments);
  newPrimitive();
  // axis Y: Green
  pos = {0, -offset, 0};
  z = {0, length + offset, 0};
  cylinder(pos + center, z, R / 2, false, segments);
  pos = {0, length, 0};
  z = {0, capLength, 0};
  cone(pos + center, z, R, segments);
  newPrimitive();
  // axis Z: Blue
  pos = {0, 0, -offset};
  z = {0, 0, length + offset};
  cylinder(pos + center, z, R / 2, false, segments);
  pos = {0, 0, length};
  z = {0, 0, capLength};
  cone(pos + center, z, R, segments);
  return *this;
}

PrimitiveBuilder &PrimitiveBuilder::grid(
  const uint32_t nx, const uint32_t ny, const vec3 center, const vec3 x, const vec3 y,
  const float wx, const float wy) {
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
    for(uint32_t column = 0; column <= nx; ++column) {
      auto p = origin + float(column) * _x + float(row) * _y;
      append(_positions, {p});
      append(_normals, {normal});
      append(_uvs, {{column * u, row * v}});
      aabb.merge(p);
    }
  for(uint32_t row = 0; row < ny; ++row)
    for(uint32_t column = 0; column < nx; ++column) {
      auto s0 = (nx + 1) * row;
      auto s1 = (nx + 1) * (row + 1);
      append(
        _indices, {
                    s0 + column + vertexID,
                    s1 + column + vertexID,
                    s1 + column + 1 + vertexID,
                    s0 + column + vertexID,
                    s1 + column + 1 + vertexID,
                    s0 + column + 1 + vertexID,
                  });
    }
  return *this;
}

PrimitiveBuilder &PrimitiveBuilder::gridPatch(
  const uint32_t nx, const uint32_t ny, const vec3 center, const vec3 x, const vec3 y,
  const float wx, const float wy) {
  auto _x = normalize(x);
  auto _y = normalize(y);
  auto normal = cross(_x, _y);
  auto origin = center + _x * (nx * wx / 2.f) - _y * (ny * wy / 2.f);
  _x *= -wx;
  _y *= wy;
  auto u = 1.f / nx;
  auto v = 1.f / ny;
  const auto vertexID = currentVertexID();
  auto i = 0u;
  for(uint32_t row = 0; row <= ny; ++row)
    for(uint32_t column = 0; column <= nx; ++column) {
      auto p = origin + float(column) * _x + float(row) * _y;
      _positions.push_back(p);
      _normals.push_back(normal);
      _uvs.emplace_back(column * u, row * v);
      aabb.merge(p);
    }
  for(uint32_t row = 0; row < ny; ++row)
    for(uint32_t column = 0; column < nx; ++column) {
      auto s0 = (nx + 1) * row;
      auto s1 = (nx + 1) * (row + 1);
      append(
        _indices, {
                    s0 + column + vertexID,
                    s1 + column + vertexID,
                    s1 + column + 1 + vertexID,
                    s0 + column + 1 + vertexID,
                  });
    }
  return *this;
}

PrimitiveBuilder &PrimitiveBuilder::line(vec3 p1, vec3 p2) {
  const auto vertexID = currentVertexID();
  append(_positions, {p1, p2});
  append(_normals, {{}, {}});
  append(_uvs, {{}, {}});
  aabb.merge(p1, p2);
  append(_indices, {vertexID, vertexID + 1});
  return *this;
}

PrimitiveBuilder &PrimitiveBuilder::rectangleLine(vec3 center, vec3 x, vec3 y) {
  const auto p1 = center + x + y;
  const auto p2 = center - x + y;
  const auto p3 = center - x - y;
  const auto p4 = center + x - y;
  const auto vertexID = currentVertexID();
  append(_positions, {p1, p2, p3, p4});
  append(_normals, {{}, {}, {}, {}});
  append(_uvs, {{}, {}, {}, {}});
  aabb.merge(p1, p2, p3, p4);
  append(
    _indices, {vertexID, vertexID + 1, vertexID + 1, vertexID + 2, vertexID + 2,
               vertexID + 3, vertexID + 3, vertexID});
  return *this;
}

PrimitiveBuilder &PrimitiveBuilder::boxLine(vec3 center, vec3 x, vec3 y, float half_z) {
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
  append(_positions, {f1, f2, f3, f4, b1, b2, b3, b4});
  append(_normals, {{}, {}, {}, {}, {}, {}, {}, {}});
  append(_uvs, {{}, {}, {}, {}, {}, {}, {}, {}});
  aabb.merge(f1, f2, f3, f4, b1, b2, b3, b4);
  append(_indices, {vertexID,     vertexID + 1, vertexID + 1, vertexID + 2, vertexID + 2,
                    vertexID + 3, vertexID + 3, vertexID,     vertexID + 4, vertexID + 5,
                    vertexID + 5, vertexID + 6, vertexID + 6, vertexID + 7, vertexID + 7,
                    vertexID + 4, vertexID,     vertexID + 4, vertexID + 1, vertexID + 5,
                    vertexID + 2, vertexID + 6, vertexID + 3, vertexID + 7});
  return *this;
}

}
