#pragma once

#include "app/iggy3d/creative/document/TerrainField.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeTerrainMaterialOverrideCapacity =
    kCreativeTerrainRenderPatchCapacity;

struct CreativeTerrainMaterialOverride {
  CreativeTerrainCoord2 coord{};
  CreativeTerrainMaterial material = CreativeTerrainMaterial::Dirt;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainMaterialOverride,
      CreativeTerrainMaterialOverride) noexcept = default;
};

enum class CreativeTerrainMaterialEditKind : std::uint8_t {
  Set,
  Clear,
  Count,
};

struct CreativeTerrainMaterialEdit {
  CreativeTerrainMaterialEditKind kind = CreativeTerrainMaterialEditKind::Set;
  CreativeTerrainCoord2 coord{};
  CreativeTerrainMaterial material = CreativeTerrainMaterial::Dirt;
};

enum class CreativeTerrainMaterialMutationStatus : std::uint8_t {
  NotRequested,
  InvalidField,
  InvalidEdit,
  DuplicateCoordinate,
  CapacityExceeded,
  NoChange,
  Applied,
};

struct CreativeTerrainMaterialMutationReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeTerrainMaterialMutationStatus status =
      CreativeTerrainMaterialMutationStatus::NotRequested;
  std::uint64_t revisionBefore = 0U;
  std::uint64_t revisionAfter = 0U;
  std::uint64_t attemptedEditCount = 0U;
  std::uint64_t overrideCountBefore = 0U;
  std::uint64_t overrideCountAfter = 0U;
  std::uint64_t changedOverrideCount = 0U;
  std::string_view reasonCode =
      "creative_terrain_material_mutation_not_requested";
};

// Sparse authored surface-material overrides. Grass is the canonical default
// and is represented by absence, keeping untouched terrain compact.
class CreativeTerrainMaterialField {
 public:
  [[nodiscard]] bool isValid() const noexcept;
  [[nodiscard]] bool validateInvariants() const noexcept;
  [[nodiscard]] std::uint64_t revision() const noexcept;
  [[nodiscard]] std::uint64_t overrideCount() const noexcept;
  [[nodiscard]] std::span<const CreativeTerrainMaterialOverride> overrides()
      const noexcept;
  [[nodiscard]] const CreativeTerrainMaterialOverride* overrideAt(
      CreativeTerrainCoord2 coord) const noexcept;
  [[nodiscard]] CreativeTerrainMaterial materialAt(
      CreativeTerrainCoord2 coord) const noexcept;

  [[nodiscard]] CreativeTerrainMaterialMutationReceipt apply(
      std::span<const CreativeTerrainMaterialEdit> edits);
  void clear() noexcept;

 private:
  std::vector<CreativeTerrainMaterialOverride> overrides_;
  std::uint64_t revision_ = 0U;
  bool valid_ = true;
};

[[nodiscard]] bool isValidCreativeTerrainMaterial(
    CreativeTerrainMaterial material) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainMaterial material) noexcept;
[[nodiscard]] bool parseCreativeTerrainMaterial(
    std::string_view value,
    CreativeTerrainMaterial& output) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainMaterialMutationStatus status) noexcept;
[[nodiscard]] std::string_view creativeTerrainMaterialRenderRole(
    CreativeTerrainMaterial material) noexcept;

}  // namespace iggy3d::creative
