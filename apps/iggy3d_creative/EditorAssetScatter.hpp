#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "EditorEdits.hpp"
#include "EditorPlacement.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"
#include "app/iggy3d/creative/tools/AssetScatter.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d::creative {

struct CreativeAppState;
class CreativeDocument;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorState;
struct CreativePlacementClearanceCache;

enum class CreativeEditorAssetScatterCandidateStatus : std::uint8_t {
  Ready,
  MissingSurface,
  SlopeRejected,
  InvalidPlacement,
  Obstructed,
  Occupied,
};

struct CreativeEditorAssetScatterCandidate {
  CreativeBrushPlacementPlan placement{};
  iggy3d::creative::CreativeVec3 surfacePosition{};
  std::uint64_t visitedKey = 0;
  CreativeEditorAssetScatterCandidateStatus status =
      CreativeEditorAssetScatterCandidateStatus::InvalidPlacement;
  bool placeable = false;
};

struct CreativeEditorAssetScatterPlan {
  std::array<CreativeEditorAssetScatterCandidate,
             iggy3d::creative::kCreativeAssetScatterCandidateCapacity>
      candidates{};
  std::size_t candidateCount = 0;
  std::size_t placeableCount = 0;
  iggy3d::creative::CreativeAssetScatterStatus kernelStatus =
      iggy3d::creative::CreativeAssetScatterStatus::NotRequested;
  bool requested = false;
  bool accepted = false;
  bool truncated = false;

  [[nodiscard]] std::span<const CreativeEditorAssetScatterCandidate> items()
      const noexcept {
    return {candidates.data(), candidateCount};
  }
};

struct CreativeAssetScatterStrokeState {
  iggy3d::creative::CreativeWorldGestureRepeatState repeat{};
  StandaloneEditTransaction transaction{};
  iggy3d::creative::CreativeWorldGestureVisitedKeys visited{};
  CreativeEditorAssetScatterPlan preview{};
  std::uint16_t acceptedMutationCount = 0;
  bool capacityReached = false;
};

[[nodiscard]] bool creativeEditorUsesAssetScatter(
    const iggy3d::creative::CreativeHotbarEntry& held,
    const iggy3d::creative::CreativeToolSettings& settings) noexcept;

[[nodiscard]] CreativeEditorAssetScatterPlan
buildCreativeEditorAssetScatterPlan(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeEditorState& editor,
    const iggy3d::creative::CreativeHotbarEntry& held,
    const CreativePlacementClearanceCache* clearanceCache = nullptr) noexcept;

void processCreativeAssetScatterFrame(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeWorldActionFrame& actions,
    std::uint64_t monotonicTimeNanoseconds,
    const CreativePlacementClearanceCache* clearanceCache = nullptr);

void finalizeCreativeAssetScatterStroke(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view reasonCode);

[[nodiscard]] std::size_t appendCreativeEditorAssetScatterWireframes(
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);

}  // namespace iggy3d_creative_app
