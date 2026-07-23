#pragma once

#include <cstdint>

namespace iggy3d::creative {

enum class CreativeMeasurementMode : std::uint8_t {
  Distance,
  AxisProjected,
  Vertical,
  Slope,
  Perimeter,
  Area,
  Count,
};

enum class CreativeMeasurementAxis : std::uint8_t {
  X,
  Y,
  Z,
  Count,
};

enum class CreativeMeasurementSnapMode : std::uint8_t {
  Auto,
  Grid,
  Surface,
  Vertex,
  Opening,
  Level,
  Count,
};

enum class CreativeMeasurementSnapKind : std::uint8_t {
  None,
  Grid,
  Surface,
  Vertex,
  Opening,
  Level,
  Count,
};

}  // namespace iggy3d::creative
