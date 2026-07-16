#pragma once

#include <cstdint>
#include <string_view>

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

namespace iggy3d::creative {

inline constexpr double kCreativeStructuralSpanMinimumMeters = 0.01;
inline constexpr double kCreativeStructuralSpanMaximumMeters = 4096.0;

enum class CreativeStructuralSpanAxis : std::uint8_t {
  LocalX,
  LocalZ,
  Count,
};

enum class CreativeStructuralSpanStatus : std::uint8_t {
  Ready,
  UnsupportedDescriptor,
  InvalidAnchor,
  DegenerateSpan,
  SpanTooLong,
  InvalidGeometry,
  ArithmeticOverflow,
};

struct CreativeStructuralSpanRequest {
  CreativeObjectKind objectKind{CreativeObjectKind::Unknown};
  CreativeVec3 firstAnchor{};
  CreativeVec3 secondAnchor{};
};

struct CreativeStructuralSpanPlan {
  CreativeStructuralSpanStatus status{
      CreativeStructuralSpanStatus::UnsupportedDescriptor};
  CreativeObjectKind objectKind{CreativeObjectKind::Unknown};
  CreativeStructuralSpanAxis spanAxis{CreativeStructuralSpanAxis::Count};
  CreativeTransform transform{};
  CreativeBounds authoredBounds{};
  double spanLengthMeters{0.0};
  bool accepted{false};
};

[[nodiscard]] bool descriptorSupportsCreativeStructuralSpan(
    const CreativeObjectDescriptor& descriptor) noexcept;
[[nodiscard]] CreativeStructuralSpanPlan planCreativeStructuralSpan(
    const CreativeStructuralSpanRequest& request) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeStructuralSpanStatus status) noexcept;

}  // namespace iggy3d::creative
