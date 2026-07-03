#pragma once

#include "app/iggy3d/creative/Tools.hpp"

#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeSnapMode : std::uint8_t {
  Disabled,
  Grid,
};

using CreativeSnapAxisMask = std::uint8_t;

inline constexpr CreativeSnapAxisMask kCreativeSnapAxisNone = 0;
inline constexpr CreativeSnapAxisMask kCreativeSnapAxisX = 1u << 0;
inline constexpr CreativeSnapAxisMask kCreativeSnapAxisY = 1u << 1;
inline constexpr CreativeSnapAxisMask kCreativeSnapAxisXY =
    kCreativeSnapAxisX | kCreativeSnapAxisY;

struct CreativeSnapPoint2 {
  double x = 0.0;
  double y = 0.0;
};

struct CreativeSnapSettings {
  CreativeSnapMode mode = CreativeSnapMode::Grid;
  CreativeSnapAxisMask axes = kCreativeSnapAxisXY;
  double stepX = 1.0;
  double stepY = 1.0;
  double originX = 0.0;
  double originY = 0.0;
};

struct CreativeSnapReceipt {
  CreativeSnapPoint2 inputPoint;
  CreativeSnapPoint2 outputPoint;
  bool settingsValid = false;
  bool snapped = false;
  bool changed = false;
  bool accepted = false;
  std::string_view message = "snap_not_applied";
};

[[nodiscard]] CreativeSnapSettings makeDefaultCreativeSnapSettings() noexcept;
[[nodiscard]] bool isValidSnapSettings(
    CreativeSnapSettings settings) noexcept;
[[nodiscard]] double snapScalar(double value,
                                double step,
                                double origin) noexcept;
[[nodiscard]] CreativeSnapReceipt snapPoint(
    CreativeSnapPoint2 point,
    CreativeSnapSettings settings) noexcept;
[[nodiscard]] CreativeToolPointerPacket snapPointerPacket(
    const CreativeToolPointerPacket& pointer,
    CreativeSnapSettings settings) noexcept;

}  // namespace iggy3d::creative
