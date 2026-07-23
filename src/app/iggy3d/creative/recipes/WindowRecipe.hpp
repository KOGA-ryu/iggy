#pragma once

#include "app/iggy3d/creative/recipes/StructuralWallRecipe.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeWindowPartCapacity = 6U;

enum class CreativeWindowRecipeStatus : std::uint8_t {
  NotRequested,
  InvalidWallFrame,
  InvalidCutout,
  InvalidSettings,
  InvalidDimensions,
  CapacityExceeded,
  Ready,
};

enum class CreativeWindowPartKind : std::uint8_t {
  MinimumJamb,
  MaximumJamb,
  FrameSill,
  FrameHeader,
  PrimaryInsert,
  SecondaryShutter,
  Count,
};

struct CreativeWindowRecipeRequest {
  CreativeStructuralWallFrame wallFrame;
  CreativeBounds cutoutBounds;
  CreativeWindowSettings settings;
  // Asset-backed inserts represent the whole insert assembly. Procedural
  // paired shutters are emitted as two independently visible panels.
  bool proceduralInsert{true};
  double frameWidthMeters{0.05};
  double insertGapMeters{0.01};
  // Zero derives a bounded insert thickness from the host wall.
  double insertThicknessMeters{0.0};
};

struct CreativeWindowPartPlan {
  CreativeWindowPartKind kind{CreativeWindowPartKind::Count};
  CreativeObjectKind objectKind{CreativeObjectKind::Unknown};
  CreativeBounds bounds;
};

struct CreativeWindowRecipeResult {
  bool accepted{false};
  CreativeWindowRecipeStatus status{CreativeWindowRecipeStatus::NotRequested};
  std::array<CreativeWindowPartPlan, kCreativeWindowPartCapacity> parts{};
  std::size_t partCount{0U};
  std::size_t primaryInsertPartIndex{kCreativeWindowPartCapacity};
  CreativeBounds framedOpeningBounds;
  std::string_view reasonCode{"creative_window_not_requested"};
};

static_assert(std::is_trivially_copyable_v<CreativeWindowRecipeRequest>);
static_assert(std::is_trivially_copyable_v<CreativeWindowPartPlan>);
static_assert(std::is_trivially_copyable_v<CreativeWindowRecipeResult>);

[[nodiscard]] CreativeWindowRecipeResult planCreativeWindow(
    const CreativeWindowRecipeRequest& request) noexcept;

}  // namespace iggy3d::creative
