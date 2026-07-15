#include "app/iggy3d/creative/input/UiInput.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <type_traits>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool validWheelProfile(
    const CreativeWheelProfile& profile) noexcept {
  return profile.polarity != CreativeWheelPolarity::Count &&
         profile.stepMode != CreativeWheelStepMode::Count &&
         std::isfinite(profile.epsilon) && profile.epsilon >= 0.0F;
}

[[nodiscard]] std::size_t positiveStepDistance(std::int64_t steps,
                                               std::size_t count) noexcept {
  const std::uint64_t magnitude =
      steps < 0
          ? static_cast<std::uint64_t>(-(steps + 1)) + 1U
          : static_cast<std::uint64_t>(steps);
  return static_cast<std::size_t>(
      magnitude % static_cast<std::uint64_t>(count));
}

}  // namespace

static_assert(std::is_trivially_copyable_v<CreativeWheelProfile>);
static_assert(std::is_trivially_copyable_v<CreativePointerSample>);
static_assert(std::is_trivially_copyable_v<CreativeDrawablePointer>);
static_assert(std::is_trivially_copyable_v<CreativeWrappedIndexResult>);
static_assert(std::is_trivially_copyable_v<CreativeRadialSectorResult>);

std::int32_t quantizeCreativeWheelSteps(
    float wheelDelta,
    CreativeWheelProfile profile) noexcept {
  if (!validWheelProfile(profile) || !std::isfinite(wheelDelta) ||
      std::fabs(wheelDelta) <= profile.epsilon) {
    return 0;
  }

  double magnitude = 1.0;
  if (profile.stepMode == CreativeWheelStepMode::RoundedMagnitude) {
    magnitude =
        std::max(1.0, std::round(std::fabs(static_cast<double>(wheelDelta))));
  }
  const double maximum =
      static_cast<double>(std::numeric_limits<std::int32_t>::max());
  const std::int32_t steps =
      magnitude >= maximum
          ? std::numeric_limits<std::int32_t>::max()
          : static_cast<std::int32_t>(magnitude);
  const bool positive = wheelDelta > 0.0F;
  const bool reverse = profile.polarity == CreativeWheelPolarity::Reversed;
  return positive != reverse ? steps : -steps;
}

CreativeDrawablePointer resolveCreativeDrawablePointer(
    CreativePointerSample sample) noexcept {
  CreativeDrawablePointer result;
  result.moved = sample.moved;
  result.primaryPressed = sample.primaryPressed;
  if (!std::isfinite(sample.logicalX) || !std::isfinite(sample.logicalY) ||
      sample.drawableWidth == 0U || sample.drawableHeight == 0U) {
    return result;
  }

  const float scaleX =
      sample.logicalWidth > 0U
          ? static_cast<float>(sample.drawableWidth) /
                static_cast<float>(sample.logicalWidth)
          : 1.0F;
  const float scaleY =
      sample.logicalHeight > 0U
          ? static_cast<float>(sample.drawableHeight) /
                static_cast<float>(sample.logicalHeight)
          : 1.0F;
  result.x = sample.logicalX * scaleX;
  result.y = sample.logicalY * scaleY;
  result.centeredX =
      result.x - static_cast<float>(sample.drawableWidth) * 0.5F;
  result.centeredY =
      static_cast<float>(sample.drawableHeight) * 0.5F - result.y;
  result.valid = std::isfinite(result.x) && std::isfinite(result.y) &&
                 std::isfinite(result.centeredX) &&
                 std::isfinite(result.centeredY);
  return result;
}

CreativeContentRect resolveCreativeContentViewport(
    float logicalMinX,
    float logicalMinY,
    float logicalMaxX,
    float logicalMaxY,
    float scaleX,
    float scaleY,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight) noexcept {
  CreativeContentRect result;
  if (drawableWidth == 0U || drawableHeight == 0U ||
      !std::isfinite(logicalMinX) || !std::isfinite(logicalMinY) ||
      !std::isfinite(logicalMaxX) || !std::isfinite(logicalMaxY) ||
      !std::isfinite(scaleX) || !std::isfinite(scaleY) || scaleX <= 0.0F ||
      scaleY <= 0.0F) {
    return result;
  }
  const float drawableW = static_cast<float>(drawableWidth);
  const float drawableH = static_cast<float>(drawableHeight);
  const float minX = std::clamp(std::round(logicalMinX * scaleX), 0.0F, drawableW);
  const float minY = std::clamp(std::round(logicalMinY * scaleY), 0.0F, drawableH);
  const float maxX = std::clamp(std::round(logicalMaxX * scaleX), 0.0F, drawableW);
  const float maxY = std::clamp(std::round(logicalMaxY * scaleY), 0.0F, drawableH);
  if (maxX <= minX || maxY <= minY) {
    return result;
  }
  result.x = static_cast<std::int32_t>(minX);
  result.y = static_cast<std::int32_t>(minY);
  result.width = static_cast<std::uint32_t>(maxX - minX);
  result.height = static_cast<std::uint32_t>(maxY - minY);
  result.valid = true;
  return result;
}

CreativeWrappedIndexResult stepCreativeWrappedIndex(
    std::size_t current,
    std::size_t count,
    std::int64_t steps) noexcept {
  CreativeWrappedIndexResult result;
  if (count == 0U) {
    return result;
  }

  const std::size_t start = current % count;
  const std::size_t distance = positiveStepDistance(steps, count);
  std::size_t next = start;
  if (steps > 0 && distance > 0U) {
    next = start >= count - distance ? start - (count - distance)
                                     : start + distance;
  } else if (steps < 0 && distance > 0U) {
    next = start < distance ? count - (distance - start) : start - distance;
  }
  result.index = next;
  result.valid = true;
  result.changed = next != current;
  return result;
}

CreativeRadialSectorResult resolveCreativeRadialSector(
    float x,
    float y,
    std::size_t sectorCount,
    float deadzone) noexcept {
  CreativeRadialSectorResult result;
  if (sectorCount == 0U || sectorCount > kCreativeRadialSectorCapacity ||
      !std::isfinite(x) || !std::isfinite(y) ||
      !std::isfinite(deadzone) || deadzone < 0.0F ||
      std::hypot(x, y) <= deadzone) {
    return result;
  }

  constexpr double kTau = 2.0 * std::numbers::pi_v<double>;
  double angle = std::atan2(static_cast<double>(x), static_cast<double>(y));
  if (angle < 0.0) {
    angle += kTau;
  }
  const double sector = kTau / static_cast<double>(sectorCount);
  result.index = static_cast<std::size_t>(
                     std::floor((angle + sector * 0.5) / sector)) %
                 sectorCount;
  result.valid = true;
  return result;
}

}  // namespace iggy3d::creative
