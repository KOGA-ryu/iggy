#pragma once

#include "app/iggy3d/creative/document/Object.hpp"
#include "core/math/Vec3.hpp"

#include <cstdint>

namespace iggy3d::creative {

enum class CreativeGeometryStatus : std::uint8_t {
  Valid,
  NonFiniteVector,
  NonFiniteBounds,
  ReversedBounds,
  ArithmeticOverflow,
  OutsideCoreFloatRange,
  Count,
};

struct CreativeBoundsMetrics {
  CreativeVec3 center{};
  CreativeVec3 size{};
  CreativeGeometryStatus status{CreativeGeometryStatus::NonFiniteBounds};
  bool valid{false};
};

struct CreativeCoreVec3Conversion {
  iggy3d::Vec3 value{};
  CreativeGeometryStatus status{CreativeGeometryStatus::NonFiniteVector};
  bool converted{false};
};

[[nodiscard]] bool isFiniteCreativeVec3(CreativeVec3 value) noexcept;
[[nodiscard]] bool creativeVec3ExactlyEqual(CreativeVec3 lhs,
                                            CreativeVec3 rhs) noexcept;
[[nodiscard]] bool creativeBoundsExactlyEqual(CreativeBounds lhs,
                                              CreativeBounds rhs) noexcept;
[[nodiscard]] bool isPositiveCreativeVec3(CreativeVec3 value) noexcept;

// Zero-sized bounds are valid. Callers that require volume must separately
// require positive size components.
[[nodiscard]] CreativeBoundsMetrics measureCreativeBounds(
    CreativeBounds bounds) noexcept;

[[nodiscard]] CreativeVec3 creativeVec3FromCore(iggy3d::Vec3 value) noexcept;
[[nodiscard]] CreativeCoreVec3Conversion creativeVec3ToCoreChecked(
    CreativeVec3 value) noexcept;

}  // namespace iggy3d::creative
