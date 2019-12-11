#pragma once
#include <string>

namespace sim ::graphics {
struct Config {
  uint32_t width{1280};
  uint32_t height{720};
  std::string title{"Base"};

  bool gui{false};
  bool vsync{true};
  uint32_t sampleCount{1};
  uint32_t numFrame{2};
};

struct DebugConfig {
  bool enableValidationLayer{false};
  bool enableGPUValidation{false};
};

class FeatureConfig {
public:
  enum Value : uint32_t {
    DedicatedAllocation = 0b1u,
    DescriptorIndexing = 0b11u,
    RayTracing = 0b111u
  };

  FeatureConfig() = default;
  constexpr explicit FeatureConfig(Value value): value{value} {}
  operator Value() const { return value; }
  explicit operator bool() = delete;

private:
  Value value;
};
}
