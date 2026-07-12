#pragma once

#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/document/VoxelField.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeTerrainControlCapacity = 256U;
inline constexpr std::uint16_t kCreativeTerrainMinimumHeightCells = 1U;
inline constexpr std::uint16_t kCreativeTerrainMaximumHeightCells = 64U;
inline constexpr std::uint16_t kCreativeTerrainMinimumRadiusCells = 1U;
inline constexpr std::uint16_t kCreativeTerrainMaximumRadiusCells = 16U;

struct CreativeTerrainCoord2 {
  std::int32_t x = 0;
  std::int32_t z = 0;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainCoord2,
      CreativeTerrainCoord2) noexcept = default;
};

struct CreativeTerrainControlPoint {
  CreativeTerrainCoord2 coord{};
  std::uint16_t heightCells = 4U;
  std::uint16_t radiusCells = 4U;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainControlPoint,
      CreativeTerrainControlPoint) noexcept = default;
};

enum class CreativeTerrainEditKind : std::uint8_t {
  Upsert,
  Remove,
  Count,
};

struct CreativeTerrainControlEdit {
  CreativeTerrainEditKind kind = CreativeTerrainEditKind::Upsert;
  CreativeTerrainControlPoint control{};
};

enum class CreativeTerrainMutationStatus : std::uint8_t {
  NotRequested,
  InvalidField,
  InvalidEdit,
  DuplicateCoordinate,
  CapacityExceeded,
  NoChange,
  Applied,
};

struct CreativeTerrainMutationReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeTerrainMutationStatus status =
      CreativeTerrainMutationStatus::NotRequested;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::uint64_t attemptedEditCount = 0;
  std::uint64_t controlCountBefore = 0;
  std::uint64_t controlCountAfter = 0;
  std::uint64_t changedControlCount = 0;
  std::string_view reasonCode = "creative_terrain_mutation_not_requested";
};

class CreativeTerrainField {
 public:
  [[nodiscard]] bool isValid() const noexcept;
  [[nodiscard]] bool validateInvariants() const noexcept;
  [[nodiscard]] std::uint64_t revision() const noexcept;
  [[nodiscard]] std::uint64_t controlCount() const noexcept;
  [[nodiscard]] std::span<const CreativeTerrainControlPoint> controls()
      const noexcept;
  [[nodiscard]] const CreativeTerrainControlPoint* controlAt(
      CreativeTerrainCoord2 coord) const noexcept;

  [[nodiscard]] CreativeTerrainMutationReceipt apply(
      std::span<const CreativeTerrainControlEdit> edits);
  void clear() noexcept;

 private:
  std::vector<CreativeTerrainControlPoint> controls_;
  std::uint64_t revision_ = 0;
  bool valid_ = true;
};

struct CreativeTerrainColumn {
  CreativeTerrainCoord2 coord{};
  std::uint16_t heightCells = 0;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainColumn,
      CreativeTerrainColumn) noexcept = default;
};

enum class CreativeTerrainSurfacePlanStatus : std::uint8_t {
  NotRequested,
  InvalidField,
  Empty,
  Ready,
};

struct CreativeTerrainSurfacePlan {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainSurfacePlanStatus status =
      CreativeTerrainSurfacePlanStatus::NotRequested;
  std::uint64_t sourceRevision = 0;
  std::uint64_t contributionCount = 0;
  std::vector<CreativeTerrainColumn> columns;
  std::vector<CreativeVoxelCuboid> cuboids;
  std::string_view reasonCode = "creative_terrain_surface_not_requested";
};

[[nodiscard]] bool isValidCreativeTerrainControlPoint(
    CreativeTerrainControlPoint control) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainMutationStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainSurfacePlanStatus status) noexcept;

// Each control contributes inside a compact circular influence disk. Overlap is
// blended with deterministic integer weights, so save/load and cross-platform
// rebuilds produce identical column heights. The generated cuboids are derived
// cache data; controls remain the authored truth.
[[nodiscard]] CreativeTerrainSurfacePlan buildCreativeTerrainSurfacePlan(
    const CreativeTerrainField& field);

}  // namespace iggy3d::creative
