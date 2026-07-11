#pragma once

#include "app/iggy3d/creative/document/Object.hpp"
#include "core/math/Vec3.hpp"

#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeAxis3 : std::uint8_t {
  X,
  Y,
  Z,
  Count,
};

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

struct CreativeObjectWorldExtent {
  CreativeVec3 min{};
  CreativeVec3 max{};
  bool valid = false;
};

[[nodiscard]] bool isFiniteCreativeVec3(CreativeVec3 value) noexcept;
[[nodiscard]] bool isValidCreativeAxis3(CreativeAxis3 axis) noexcept;
[[nodiscard]] std::string_view toString(CreativeAxis3 axis) noexcept;
[[nodiscard]] CreativeVec3 rotateCreativeVectorAxisAngle(
    CreativeVec3 vector,
    CreativeAxis3 axis,
    double radians) noexcept;
[[nodiscard]] CreativeVec3 composeCreativeWorldAxisRotation(
    CreativeVec3 eulerRadians,
    CreativeAxis3 axis,
    double radians) noexcept;
[[nodiscard]] double creativeSquaredDistanceFromAxis(
    CreativeVec3 point,
    CreativeVec3 axisPoint,
    CreativeAxis3 axis) noexcept;
[[nodiscard]] bool creativeVec3ExactlyEqual(CreativeVec3 lhs,
                                            CreativeVec3 rhs) noexcept;
[[nodiscard]] bool creativeBoundsExactlyEqual(CreativeBounds lhs,
                                              CreativeBounds rhs) noexcept;
[[nodiscard]] bool isPositiveCreativeVec3(CreativeVec3 value) noexcept;

// Zero-sized bounds are valid. Callers that require volume must separately
// require positive size components.
[[nodiscard]] CreativeBoundsMetrics measureCreativeBounds(
    CreativeBounds bounds) noexcept;
// Resolves the same world-space extent used by clipboard placement anchors:
// transformed bounds, path points, or the transform position in that order.
[[nodiscard]] CreativeObjectWorldExtent resolveCreativeObjectWorldExtent(
    const CreativeObject& object) noexcept;

[[nodiscard]] CreativeVec3 creativeVec3FromCore(iggy3d::Vec3 value) noexcept;
[[nodiscard]] CreativeCoreVec3Conversion creativeVec3ToCoreChecked(
    CreativeVec3 value) noexcept;

}  // namespace iggy3d::creative
