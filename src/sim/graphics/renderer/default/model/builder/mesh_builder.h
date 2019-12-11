#pragma once
#include "sim/graphics/renderer/default/model/model.h"
#include "par_shapes.h"

namespace sim::graphics::renderer::defaultRenderer {
class MeshBuilder {
public:
  explicit MeshBuilder(
    uint32_t baseVertexOffset, uint32_t baseIndexOffset, uint32_t baseMeshID);
  uint32_t newMesh(Primitive primitive = Primitive::Triangles);
  uint32_t currentMeshID();

  /**
   * construct the mesh from predefined vertices and indices
   * @param vertices
   * @param indices
   * @param primitive
   * @param checkIndexInRange check if index is out of the range of vertices
   */
  MeshBuilder &from(
    std::vector<Vertex> &vertices, std::vector<uint32_t> &indices,
    Primitive primitive = Primitive::Triangles, bool checkIndexInRange = true);
  /**
   * construct the mesh from predefined vertex positions and indices. Vertex normal and uv
   * will be zero.
   * @param vertices
   * @param indices
   * @param primitive
   * @param checkIndexInRange check if index is out of the range of vertices
   */
  MeshBuilder &from(
    std::vector<glm::vec3> &vertexPositions, std::vector<uint32_t> &indices,
    Primitive primitive = Primitive::Triangles, bool checkIndexInRange = true);

  /**generate a triangle from 3 nodes. Nodes should be in the Counter-Clock-Wise order.*/
  MeshBuilder &triangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);
  /**
  * generate a rectangle
  * @param center the center of the rectangle
  * @param x the 'width' direction of the rectangle from the center.
  * @param y the 'height' direction of the rectangle from the center.
  */
  MeshBuilder &rectangle(glm::vec3 center, glm::vec3 x, glm::vec3 y);
  MeshBuilder &gridMesh(
    uint32_t nx, uint32_t ny, glm::vec3 center = {0.f, 0.f, 0.f}, glm::vec3 x = {0, 0, 1},
    glm::vec3 y = {1, 0, 0}, float wx = 1, float wy = 1);
  MeshBuilder &grid(
    uint32_t nx, uint32_t ny, glm::vec3 center = {0.f, 0.f, 0.f}, glm::vec3 x = {0, 0, 1},
    glm::vec3 y = {1, 0, 0}, float wx = 1, float wy = 1);
  /**
  * generate a circle
  * @param center the center of the circle
  * @param z the normal vector of the circle surface.
  * @param R the radius of the circle
  * @param segments number of the edges to simulate circle.
  */
  MeshBuilder &circle(glm::vec3 center, glm::vec3 z, float R, int segments = 50);
  /**
  * generate a sphere
  * @param center the center of the sphere
  * @param R  the radius of the sphere
  * @param nsubd the number of sub-divisions. At most 3, the higher,the smoother.
  */
  MeshBuilder &sphere(glm::vec3 center, float R, int nsubd = 3);
  /**
  * generate a box
  * @param center the center of the box
  * @param x the 'width' direction of the box from the center.
  * @param y the 'height' direction of the box from the center.
  * @param z the 'depth' direction of the box from the center.
  */
  MeshBuilder &box(glm::vec3 center, glm::vec3 x, glm::vec3 y, float z);
  /**
  * generate a cone
  * @param center the center of the cone
  * @param z  the normal vector of the bottom circle surface of the cone.
  * @param R  the radius of the bottom circle surface of the cone.
  * @param segments  number of the edges of a polygon to simulate the circle.
  * @return
  */
  MeshBuilder &cone(glm::vec3 center, glm::vec3 z, float R, int segments = 50);
  /**
  * generate a cylinder
  * @param center the center of the cylinder
  * @param z the normal vector of the cylinder
  * @param R the radius of the bottom circle of the cylinder
  * @param cap the length between top and bottom circle of the cylinder.
  * @param segments  number of the edges of a polygon to simulate the circle.
  */
  MeshBuilder &cylinder(
    glm::vec3 center, glm::vec3 z, float R, bool cap = true, int segments = 50);
  /**
  * generate an axis model.
  * @param center the center of the center of the axis
  * @param length the length of each axis bar.
  * @param R  the radius of the cap of each axis bar.
  * @param capLength the length of the cap of each axis bar.
  * @param segments number of the edges of a polygon to simulate the circle.
  */
  MeshBuilder &axis(
    glm::vec3 center, float length, float R, float capLength, int segments = 10);
  /**
  * generate a line from p1 to p2.
  * @param p1
  * @param p2
  */
  MeshBuilder &line(glm::vec3 p1, glm::vec3 p2);
  /**
  * generate a line by a rectangle located in center and from x to y.
  * @param center the center of the line
  * @param x
  * @param y
  */
  MeshBuilder &rectangleLine(glm::vec3 center, glm::vec3 x, glm::vec3 y);
  /**
  * generate a line by a box located in center with (x,y,half_z) direction.
  * @param center
  * @param x
  * @param y
  * @param half_z
  */
  MeshBuilder &boxLine(glm::vec3 center, glm::vec3 x, glm::vec3 y, float half_z);

  std::vector<Vertex> &getVertices();
  std::vector<uint32_t> &getIndices();
  std::vector<Mesh> &getMeshes();

private:
  uint32_t currentVertexID() const;
  void updateCurrentMesh();
  void ensurePrimitive(Primitive type);

private:
  const uint32_t baseVertexOffset, baseIndexOffset, baseMeshID;
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  std::vector<Mesh> meshes;
};
}
