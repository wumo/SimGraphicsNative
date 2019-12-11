
#include <cfloat>
#include "fps_meter.h"

namespace sim::graphics {
auto FPSMeter::update(const float dt) -> void {
  fpsAccumulator += dt - fpsHistory[historyPointer];
  fpsHistory[historyPointer] = dt;
  historyPointer = (historyPointer + 1) % kFPSHistorySize;
  fps = (fpsAccumulator > 0.0f) ?
          (1.0f / (fpsAccumulator / static_cast<float>(kFPSHistorySize))) :
          FLT_MAX;
}

auto FPSMeter::FPS() const -> float { return this->fps; }

auto FPSMeter::FrameTime() const -> float { return 1000.0f / this->fps; }
}