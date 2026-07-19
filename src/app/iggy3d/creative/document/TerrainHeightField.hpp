#pragma once

#include "app/iggy3d/creative/document/TerrainField.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::uint16_t kCreativeTerrainEmptyHeightCells = 0U;
inline constexpr std::size_t kCreativeTerrainHeightFieldCellCapacity =
    kCreativeTerrainRenderPatchCapacity;

struct CreativeTerrainHeightFieldBounds {
  CreativeTerrainCoord2 minimum{};
  std::uint16_t widthCells = 0U;
  std::uint16_t depthCells = 0U;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainHeightFieldBounds,
      CreativeTerrainHeightFieldBounds) noexcept = default;
};

enum class CreativeTerrainHeightFieldReplaceStatus : std::uint8_t {
  NotRequested,
  InvalidField,
  InvalidBounds,
  InvalidHeights,
  NoChange,
  Applied,
};

struct CreativeTerrainHeightFieldReplaceReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeTerrainHeightFieldReplaceStatus status =
      CreativeTerrainHeightFieldReplaceStatus::NotRequested;
  std::uint64_t revisionBefore = 0U;
  std::uint64_t revisionAfter = 0U;
  std::uint64_t cellCountBefore = 0U;
  std::uint64_t cellCountAfter = 0U;
  std::string_view reasonCode =
      "creative_terrain_height_field_replace_not_requested";
};

// Dense, row-major terrain tile heights. Zero denotes an absent tile; present
// tile heights use the same quantized 1..64 cell range as existing terrain.
// This is intentionally independent from the legacy rod-backed field while
// the replacement storage and generation contracts are being proven.
class CreativeTerrainHeightField {
 public:
  [[nodiscard]] bool isValid() const noexcept;
  [[nodiscard]] bool validateInvariants() const noexcept;
  [[nodiscard]] std::uint64_t revision() const noexcept;
  [[nodiscard]] CreativeTerrainHeightFieldBounds bounds() const noexcept;
  [[nodiscard]] std::uint64_t cellCount() const noexcept;
  [[nodiscard]] std::uint64_t presentCellCount() const noexcept;
  [[nodiscard]] std::span<const std::uint16_t> heights() const noexcept;
  [[nodiscard]] bool contains(CreativeTerrainCoord2 coord) const noexcept;
  [[nodiscard]] std::optional<std::uint16_t> heightAt(
      CreativeTerrainCoord2 coord) const noexcept;

  [[nodiscard]] CreativeTerrainHeightFieldReplaceReceipt replace(
      CreativeTerrainHeightFieldBounds bounds,
      std::span<const std::uint16_t> heights);
  void clear() noexcept;

 private:
  CreativeTerrainHeightFieldBounds bounds_{};
  std::vector<std::uint16_t> heights_;
  std::uint64_t revision_ = 0U;
  bool valid_ = true;
};

[[nodiscard]] bool isValidCreativeTerrainHeightFieldBounds(
    CreativeTerrainHeightFieldBounds bounds) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainHeightFieldReplaceStatus status) noexcept;

// Converts committed tile heights into the same canonical surface contract as
// legacy terrain. Absent cells remain holes; contiguous equal-height cells are
// merged into collision cuboids without changing the heightfield.
[[nodiscard]] CreativeTerrainSurfacePlan
buildCreativeTerrainHeightSurfacePlan(
    const CreativeTerrainHeightField& field);
// Replaces every base column inside the heightfield bounds, including removing
// columns where the replacement height is zero, then rebuilds canonical
// cuboids for the combined surface. This is the transient preview composition
// seam; neither input is mutated.
[[nodiscard]] CreativeTerrainSurfacePlan
replaceCreativeTerrainSurfaceRegion(
    const CreativeTerrainSurfacePlan& base,
    const CreativeTerrainHeightField& replacement);
// Legacy controls remain the fallback outside the authored heightfield bounds.
// Inside those bounds the dense heightfield is authoritative, including zero
// cells that intentionally remove legacy terrain.
[[nodiscard]] CreativeTerrainSurfacePlan
buildCreativeComposedTerrainSurfacePlan(
    const CreativeTerrainField& legacy,
    const CreativeTerrainHeightField& authored);
[[nodiscard]] CreativeTerrainRenderPlan buildCreativeTerrainHeightRenderPlan(
    const CreativeTerrainHeightField& field,
    CreativeVec3 gridOrigin,
    double cellSize,
    std::size_t maxPatchCount = kCreativeTerrainRenderPatchCapacity);

}  // namespace iggy3d::creative
