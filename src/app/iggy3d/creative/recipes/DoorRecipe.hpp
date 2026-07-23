#pragma once

#include "app/iggy3d/creative/recipes/StructuralWallRecipe.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeDoorPartCapacity = 11U;
inline constexpr std::size_t kCreativeDoorLeafCapacity = 2U;
inline constexpr std::uint8_t kCreativeDoorNoLeaf = 0xffU;

enum class CreativeDoorRecipeStatus : std::uint8_t {
  NotRequested,
  InvalidWallFrame,
  InvalidCutout,
  InvalidSettings,
  InvalidDimensions,
  CapacityExceeded,
  Ready,
};

enum class CreativeDoorPartKind : std::uint8_t {
  MinimumJamb,
  MaximumJamb,
  Header,
  PrimaryLeaf,
  SecondaryLeaf,
  PrimaryLowerHinge,
  PrimaryUpperHinge,
  SecondaryLowerHinge,
  SecondaryUpperHinge,
  PrimaryHandle,
  SecondaryHandle,
  Count,
};

struct CreativeDoorRecipeRequest {
  CreativeStructuralWallFrame wallFrame;
  CreativeBounds cutoutBounds;
  CreativeDoorSettings settings;
  double frameWidthMeters{0.07};
  double floorGapMeters{0.015};
  double leafGapMeters{0.01};
  // Zero derives a bounded thickness from the host wall.
  double leafThicknessMeters{0.0};
};

struct CreativeDoorPartPlan {
  CreativeDoorPartKind kind{CreativeDoorPartKind::Count};
  CreativeObjectKind objectKind{CreativeObjectKind::Unknown};
  CreativeBounds closedBounds;
  CreativeTransform closedTransform;
  std::uint8_t leafIndex{kCreativeDoorNoLeaf};
};

struct CreativeDoorLeafPlan {
  CreativeBounds closedAssemblyBounds;
  CreativeBounds sweepBounds;
  CreativeVec3 hingePivotMeters;
  double openAngleRadians{0.0};
  std::size_t firstPartIndex{0U};
  std::size_t partCount{0U};
};

struct CreativeDoorRecipeResult {
  bool accepted{false};
  CreativeDoorRecipeStatus status{CreativeDoorRecipeStatus::NotRequested};
  std::array<CreativeDoorPartPlan, kCreativeDoorPartCapacity> parts{};
  std::size_t partCount{0U};
  std::array<CreativeDoorLeafPlan, kCreativeDoorLeafCapacity> leaves{};
  std::size_t leafCount{0U};
  CreativeBounds fullSweepBounds;
  std::string_view reasonCode{"creative_door_not_requested"};
};

static_assert(std::is_trivially_copyable_v<CreativeDoorRecipeRequest>);
static_assert(std::is_trivially_copyable_v<CreativeDoorPartPlan>);
static_assert(std::is_trivially_copyable_v<CreativeDoorLeafPlan>);
static_assert(std::is_trivially_copyable_v<CreativeDoorRecipeResult>);

[[nodiscard]] CreativeDoorRecipeResult planCreativeDoor(
    const CreativeDoorRecipeRequest& request) noexcept;

}  // namespace iggy3d::creative
