#ifndef SIM_WAVE_FFT_COMMON_H
#define SIM_WAVE_FFT_COMMON_H

const int h0Idx = 0, htIdx = 1, hDxIdx = 2, hDzIdx = 3, slopeXIdx = 4, slopeZIdx = 5;
struct Data {
  vec2 data[6];
};

layout(push_constant) uniform ComputeUniform {
  int positionOffset;
  int normalOffset;
  int dataOffset;
  float patchSize;
  float choppyScale;
  float timeScale;
  float time;
};

layout(set = 0, binding = 0, std430) buffer BitReversalBuffer { int rev[]; };
layout(set = 0, binding = 1, std430) buffer OceanBuffer { Data datum[]; };
layout(set = 0, binding = 2, std430) buffer Positions { float positions[]; };
layout(set = 0, binding = 3, std430) buffer Normals { float normals[]; };

layout(constant_id = 0) const uint lx = 128;
layout(constant_id = 1) const uint ly = 1;
layout(constant_id = 2) const int N = 128;
layout(local_size_x_id = 0, local_size_y_id = 1) in;

const float PI = 3.141592653589793;

vec2 polar(float rho, float theta) { return vec2(rho * cos(theta), rho * sin(theta)); }

vec2 mul(vec2 a, vec2 b) { return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x); }

vec2 cpow(vec2 a, int p) {
  vec2 result = {1, 0};
  for(int i = 0; i < p; i++)
    result = mul(result, a);
  return result;
}

shared vec2 sharedData[N];

void fft1D(int id, int category) {
  int totalStages = int(log2(N));
  int groupSize = 1;
  float sign = 1;
  for(uint stage = 0; stage < totalStages; stage++) {
    groupSize *= 2;
    vec2 w_m = polar(1, sign * 2 * PI / groupSize);
    int group = id / groupSize;
    int groupIdx = id % groupSize;
    int halfSize = groupSize / 2;
    int k = groupIdx % halfSize;
    vec2 w = cpow(w_m, k);
    int j = group * groupSize + k;
    vec2 u = sharedData[j];
    vec2 t = mul(w, sharedData[j + groupSize / 2]);
    vec2 temp;
    if(groupIdx < halfSize) temp = u + t;
    else
      temp = u - t;
    barrier();
    sharedData[id] = temp;
  }
  sharedData[id] /= N;
}

#endif // SIM_WAVE_FFT_COMMON_H
