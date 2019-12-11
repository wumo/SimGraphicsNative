#version 450

layout(location = 0) in vec3 inPos;

layout(push_constant) uniform PushConsts { layout(offset = 0) mat4 mvp; };

layout(location = 0) out vec3 outUVW;

out gl_PerVertex { vec4 gl_Position; };

void main() {
  outUVW = inPos;
  gl_Position = mvp * vec4(outUVW, 1.0);
}
