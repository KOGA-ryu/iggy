#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "app/iggy3d/creative/document/Document.hpp"

namespace iggy3d::creative {

enum class CreativePlacementGridStatus : std::uint8_t {
  Invalid,
  Ready,
};

enum class CreativeGridTargetStatus : std::uint8_t {
  InvalidInput,
  TargetOutOfBounds,
  AdjacentOutOfBounds,
  Ready,
};

using CreativePlacementGridAxisMask = std::uint8_t;

inline constexpr CreativePlacementGridAxisMask kCreativePlacementGridAxisX =
    1U << 0U;
inline constexpr CreativePlacementGridAxisMask kCreativePlacementGridAxisY =
    1U << 1U;
inline constexpr CreativePlacementGridAxisMask kCreativePlacementGridAxisZ =
    1U << 2U;

struct CreativePlacementGridFrameRequest {
  CreativeGridSettings documentGrid{};
  CreativeDocumentSnapSettings documentSnap =
      makeDefaultCreativeDocumentSnapSettings();
  CreativeBounds documentWorldBounds{};
  double stepOverrideMeters = 0.0;
  double activePlaneY = 0.0;
  bool useStepOverride = false;
  bool useActivePlaneOverride = false;
  bool storageAligned = false;
  std::uint8_t depthOffsetSteps = 0U;
  // Zero selects automatic hysteresis; otherwise exactly one axis bit.
  CreativePlacementGridAxisMask depthAxisLock = 0U;
  CreativeVec3 previousDepthAxis{};
};

struct CreativePlacementGridFrame {
  CreativePlacementGridStatus status = CreativePlacementGridStatus::Invalid;
  CreativeVec3 latticeOrigin{};
  CreativeVec3 stepMeters{1.0, 1.0, 1.0};
  CreativeVec3 storageOrigin{};
  double storageCellSizeMeters = 1.0;
  CreativeGridSize3 storageSize{};
  CreativeBounds documentBounds{};
  CreativePlacementGridAxisMask boundedAxes = 0U;
  double activePlaneY = 0.0;
  std::uint32_t majorEvery = 5U;
  std::uint8_t depthOffsetSteps = 0U;
  CreativePlacementGridAxisMask depthAxisLock = 0U;
  CreativeVec3 previousDepthAxis{};
  bool storageAligned = false;
  bool valid = false;
};

struct CreativeGridTarget {
  CreativeGridTargetStatus status = CreativeGridTargetStatus::InvalidInput;
  bool valid = false;
  bool resolved = false;
  bool targetInBounds = false;
  bool adjacentInBounds = false;
  CreativeVec3 hitPoint{};
  CreativeVec3 faceNormal{};
  CreativeVec3 placerForward{0.0, 0.0, -1.0};
  CreativeVec3 viewDepthAxis{0.0, 0.0, -1.0};
  CreativeGridCoord3 targetCell{};
  CreativeGridCoord3 surfaceAdjacentCell{};
  CreativeGridCoord3 adjacentCell{};
  CreativeBounds targetCellBounds{};
  CreativeBounds surfaceAdjacentCellBounds{};
  CreativeBounds adjacentCellBounds{};
  CreativeVec3 surfacePlacementAnchor{};
  CreativeVec3 placementAnchor{};
};

enum class CreativePlacementGridLineRole : std::uint8_t {
  Minor,
  Major,
  Boundary,
};

struct CreativePlacementGridLine {
  CreativeVec3 start{};
  CreativeVec3 end{};
  CreativePlacementGridLineRole role =
      CreativePlacementGridLineRole::Minor;
};

inline constexpr std::size_t kCreativePlacementGridUnboundedLinesPerAxis =
    129U;
inline constexpr std::size_t kCreativePlacementGridMaximumLinesPerAxis = 513U;
inline constexpr std::size_t kCreativePlacementGridMaximumLineCount =
    kCreativePlacementGridMaximumLinesPerAxis * 2U;

struct CreativePlacementGridOverlayRequest {
  CreativePlacementGridFrame frame{};
  CreativeVec3 focus{};
};

struct CreativePlacementGridOverlayPlan {
  std::array<CreativePlacementGridLine,
             kCreativePlacementGridMaximumLineCount>
      lines{};
  std::size_t lineCount = 0U;
  CreativeBounds visibleBounds{};
  bool clippedX = false;
  bool clippedZ = false;
  bool valid = false;
};

struct CreativePlacementGridDot {
  CreativeVec3 position{};
  CreativePlacementGridLineRole role =
      CreativePlacementGridLineRole::Minor;
};

inline constexpr std::size_t kCreativePlacementGridDotsPerAxis = 33U;
inline constexpr std::size_t kCreativePlacementGridMaximumDotCount =
    kCreativePlacementGridDotsPerAxis * kCreativePlacementGridDotsPerAxis;

struct CreativePlacementGridDotLayerRequest {
  CreativePlacementGridFrame frame{};
  CreativeGridTarget target{};
};

struct CreativePlacementGridDotLayerPlan {
  std::array<CreativePlacementGridDot,
             kCreativePlacementGridMaximumDotCount>
      dots{};
  std::size_t dotCount = 0U;
  bool valid = false;
};

[[nodiscard]] CreativePlacementGridFrame makeCreativePlacementGridFrame(
    const CreativePlacementGridFrameRequest& request) noexcept;
[[nodiscard]] CreativeGridTarget resolveCreativeGridTargetFromHit(
    CreativeVec3 hitPoint,
    CreativeVec3 faceNormal,
    const CreativePlacementGridFrame& frame,
    CreativeVec3 placerForward = {0.0, 0.0, -1.0}) noexcept;
[[nodiscard]] CreativeGridTarget resolveCreativeGridTargetFromHit(
    CreativeVec3 hitPoint,
    CreativeVec3 faceNormal,
    double cellSize,
    CreativeVec3 origin = {},
    CreativeVec3 placerForward = {0.0, 0.0, -1.0}) noexcept;
[[nodiscard]] CreativePlacementGridOverlayPlan
buildCreativePlacementGridOverlayPlan(
    const CreativePlacementGridOverlayRequest& request) noexcept;
[[nodiscard]] CreativePlacementGridDotLayerPlan
buildCreativePlacementGridDotLayerPlan(
    const CreativePlacementGridDotLayerRequest& request) noexcept;

}  // namespace iggy3d::creative
