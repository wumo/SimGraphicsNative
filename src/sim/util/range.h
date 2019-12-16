#pragma once
#include <cstdint>

namespace sim::util {
struct Range {
  uint32_t offset{0};
  uint32_t size{0};

  uint32_t endOffset() const { return offset + size; }
};
}