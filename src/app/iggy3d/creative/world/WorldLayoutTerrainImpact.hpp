#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

enum class CreativeWorldLayoutTerrainImpactStatus : std::uint8_t {
  NoEffect,
  Current,
  Drifted,
  Overridden,
  Count,
};

enum class CreativeWorldLayoutTerrainImpactPlanStatus : std::uint8_t {
  NotRequested,
  InvalidDocument,
  InvalidLayout,
  KernelRejected,
  MutationRejected,
  Empty,
  Ready,
};

struct CreativeWorldLayoutTerrainMaterialImpact {
  CreativeTerrainCoord2 coord{};
  CreativeTerrainMaterial material = CreativeTerrainMaterial::Grass;
};

// Transient attribution for one World Layout terrain symbol. This is derived
// from the same recipe kernels as generation and is intentionally not saved.
struct CreativeWorldLayoutTerrainSourceImpact {
  CreativeWorldLayoutTable table = CreativeWorldLayoutTable::None;
  std::size_t index = kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeWorldLayoutTerrainImpactStatus status =
      CreativeWorldLayoutTerrainImpactStatus::NoEffect;
  bool hasGridBounds = false;
  CreativeTerrainCoord2 minimumCoord{};
  CreativeTerrainCoord2 maximumCoord{};
  bool hasHeightRange = false;
  std::uint16_t minimumHeightCells = 0U;
  std::uint16_t maximumHeightCells = 0U;
  std::uint64_t authoredControlCount = 0U;
  std::uint64_t authoredMaterialCount = 0U;
  std::uint64_t effectiveControlEditCount = 0U;
  std::uint64_t effectiveMaterialEditCount = 0U;
  std::vector<CreativeTerrainControlPoint> controls;
  std::vector<CreativeWorldLayoutTerrainMaterialImpact> materials;
  std::vector<CreativeTerrainCoord2> influenceCells;
  bool influenceCellsClipped = false;
};

struct CreativeWorldLayoutTerrainImpactPlan {
  bool requested = false;
  bool accepted = false;
  CreativeWorldLayoutTerrainImpactPlanStatus status =
      CreativeWorldLayoutTerrainImpactPlanStatus::NotRequested;
  CreativeDocumentId sourceDocumentId = kInvalidDocumentId;
  std::uint64_t sourceDocumentRevision = 0U;
  std::uint64_t sourceTerrainRevision = 0U;
  std::uint64_t sourceMaterialRevision = 0U;
  CreativeWorldLayoutTable failedTable = CreativeWorldLayoutTable::None;
  std::size_t failedIndex = kInvalidCreativeWorldLayoutIndex;
  std::vector<CreativeWorldLayoutTerrainSourceImpact> sources;
  std::string reasonCode = "creative_world_layout_terrain_impact_not_requested";
  std::string kernelReasonCode =
      "creative_world_layout_terrain_impact_kernel_not_requested";
};

[[nodiscard]] std::string_view
toString(CreativeWorldLayoutTerrainImpactStatus status) noexcept;
[[nodiscard]] std::string_view
toString(CreativeWorldLayoutTerrainImpactPlanStatus status) noexcept;

[[nodiscard]] CreativeWorldLayoutTerrainImpactPlan
buildCreativeWorldLayoutTerrainImpactPlan(const CreativeDocument& document,
                                          const CreativeWorldLayout& layout);

[[nodiscard]] const CreativeWorldLayoutTerrainSourceImpact*
findCreativeWorldLayoutTerrainSourceImpact(
    const CreativeWorldLayoutTerrainImpactPlan& plan,
    CreativeWorldLayoutTable table,
    std::size_t index) noexcept;

[[nodiscard]] bool creativeWorldLayoutTerrainImpactWorldBounds(
    const CreativeWorldLayoutTerrainSourceImpact& impact,
    CreativeGridSettings grid,
    CreativeBounds& output) noexcept;

}  // namespace iggy3d::creative
