#version 450
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(set = 0, binding = 1) uniform LayerUniform { int layer; };

void main() {
  gl_Position = gl_in[0].gl_Position;
  gl_Layer = layer;
  EmitVertex();
  gl_Position = gl_in[1].gl_Position;
  gl_Layer = layer;
  EmitVertex();
  gl_Position = gl_in[2].gl_Position;
  gl_Layer = layer;
  EmitVertex();
  EndPrimitive();
}