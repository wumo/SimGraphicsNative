#pragma once

#include <cstddef>
namespace sim { namespace graphics {
struct FPSMeter {
  static const size_t kFPSHistorySize = 128;

  float fpsHistory[kFPSHistorySize] = {0.0f};
  size_t historyPointer = 0;
  float fpsAccumulator = 0.0f;
  float fps = 0.0f;

  void update(float dt);
  float FPS() const;
  float FrameTime() const;
};
}}
