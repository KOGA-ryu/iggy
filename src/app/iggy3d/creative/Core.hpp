#pragma once

#include <cstdint>

namespace iggy3d::creative {

using Id = std::uint32_t;

inline constexpr Id kInvalidId = 0;

enum class Tool : std::uint8_t {
  Select,
  Move,
  Measure,
  Navigate,
};

struct TargetRef {
  Id value = kInvalidId;
};

}  // namespace iggy3d::creative
