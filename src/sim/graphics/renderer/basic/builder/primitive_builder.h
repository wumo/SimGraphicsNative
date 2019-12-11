#pragma once
#include "../basic_model.h"
#include "par_shapes.h"

namespace sim ::graphics::renderer::basic {
class PrimitiveBuilder {
public:
  PrimitiveBuilder &newPrimitive(
    PrimitiveTopology topology = PrimitiveTopology::Triangles);

  /**
   * construct the mesh from predefined vertices and indices
   * @param vertices
   * @param indices
   * @param primitive
   * @param checkIndexInRange check if index is out of the range of vertices
   */
  PrimitiveBuilder &from(
    std::vector<Vertex> &vertices, std::vector<uint32_t> &indices,
    PrimitiveTopology primitive = PrimitiveTopology::Triangles);
  /**
   * construct the mesh from predefined vertex positions and indices. Vertex normal and uv
   * will be zero.
   * @param vertices
   * @param indices
   * @param primitive
   * @param checkIndexInRange check if index is out of the range of vertices
   */
  PrimitiveBuilder &from(
    std::vector<glm::vec3> &vertexPositions, std::vector<uint32_t> &indices,
    PrimitiveTopology primitive = PrimitiveTopology::Triangles);

  /**generate a triangle from 3 nodes. Nodes should be in the Counter-Clock-Wise order.*/
  PrimitiveBuilder &triangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);
  /**
  * generate a rectangle
  * @param center the center of the rectangle
  * @param x the 'width' direction of the rectangle from the center.
  * @param y the 'height' direction of the rectangle from the center.
  */
  PrimitiveBuilder &rectangle(glm::vec3 center, glm::vec3 x, glm::vec3 y);
  PrimitiveBuilder &gridMesh(
    uint32_t nx, uint32_t ny, glm::vec3 center = {0.f, 0.f, 0.f}, glm::vec3 x = {0, 0, 1},
    glm::vec3 y = {1, 0, 0}, float wx = 1, float wy = 1);
  PrimitiveBuilder &grid(
    uint32_t nx, uint32_t ny, glm::vec3 center = {0.f, 0.f, 0.f}, glm::vec3 x = {0, 0, 1},
    glm::vec3 y = {1, 0, 0}, float wx = 1, float wy = 1);
  /**
  * generate a circle
  * @param center the center of the circle
  * @param z the normal vector of the circle surface.
  * @param R the radius of the circle
  * @param segments number of the edges to simulate circle.
  */
  PrimitiveBuilder &circle(glm::vec3 center, glm::vec3 z, float R, int segments = 50);
  /**
  * generate a sphere
  * @param center the center of the sphere
  * @param R  the radius of the sphere
  * @param nsubd the number of sub-divisions. At most 3, the higher,the smoother.
  */
  PrimitiveBuilder &sphere(glm::vec3 center, float R, int nsubd = 3);
  /**
  * generate a box
  * @param center the center of the box
  * @param x the 'width' direction of the box from the center.
  * @param y the 'height' direction of the box from the center.
  * @param z the 'depth' direction of the box from the center.
  */
  PrimitiveBuilder &box(glm::vec3 center, glm::vec3 x, glm::vec3 y, float z);
  /**
  * generate a cone
  * @param center the center of the cone
  * @param z  the normal vector of the bottom circle surface of the cone.
  * @param R  the radius of the bottom circle surface of the cone.
  * @param segments  number of the edges of a polygon to simulate the circle.
  * @return
  */
  PrimitiveBuilder &cone(glm::vec3 center, glm::vec3 z, float R, int segments = 50);
  /**
  * generate a cylinder
  * @param center the center of the cylinder
  * @param z the normal vector of the cylinder
  * @param R the radius of the bottom circle of the cylinder
  * @param cap the length between top and bottom circle of the cylinder.
  * @param segments  number of the edges of a polygon to simulate the circle.
  */
  PrimitiveBuilder &cylinder(
    glm::vec3 center, glm::vec3 z, float R, bool cap = true, int segments = 50);
  /**
  * generate an axis model.
  * @param center the center of the center of the axis
  * @param length the length of each axis bar.
  * @param R  the radius of the cap of each axis bar.
  * @param capLength the length of the cap of each axis bar.
  * @param segments number of the edges of a polygon to simulate the circle.
  */
  PrimitiveBuilder &axis(
    glm::vec3 center, float length, float R, float capLength, int segments = 10);
  /**
  * generate a line from p1 to p2.
  * @param p1
  * @param p2
  */
  PrimitiveBuilder &line(glm::vec3 p1, glm::vec3 p2);
  /**
  * generate a line by a rectangle located in center and from x to y.
  * @param center the center of the line
  * @param x
  * @param y
  */
  PrimitiveBuilder &rectangleLine(glm::vec3 center, glm::vec3 x, glm::vec3 y);
  /**
  * generate a line by a box located in center with (x,y,half_z) direction.
  * @param center
  * @param x
  * @param y
  * @param half_z
  */
  PrimitiveBuilder &boxLine(glm::vec3 center, glm::vec3 x, glm::vec3 y, float half_z);

  const std::vector<Vertex> &vertices() const;
  const std::vector<uint32_t> &indices() const;
  const std::vector<Primitive> &primitives() const;

private:
  uint32_t currentVertexID() const;

private:
  std::vector<Vertex> _vertices;
  std::vector<uint32_t> _indices;
  AABB aabb;
  std::vector<Primitive> _primitives;
};
}
