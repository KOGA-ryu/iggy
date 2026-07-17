#pragma once

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeStructuralSurfaceRecipeStatus : std::uint8_t {
  NotRequested,
  InvalidFootprint,
  InvalidAnchorPlane,
  InvalidLayerCount,
  UnsupportedKind,
  UnrepresentableBounds,
  Ready,
};

struct CreativeStructuralSurfaceRecipeRequest {
  CreativeObjectKind kind = CreativeObjectKind::Unknown;
  double minimumX = 0.0;
  double maximumX = 0.0;
  double minimumZ = 0.0;
  double maximumZ = 0.0;
  double anchorPlaneMeters = 0.0;
  std::uint16_t layerCount = 1U;
};

struct CreativeStructuralSurfaceRecipeResult {
  bool accepted = false;
  CreativeStructuralSurfaceRecipeStatus status =
      CreativeStructuralSurfaceRecipeStatus::NotRequested;
  CreativeStructuralSurfaceAnchor anchor =
      CreativeStructuralSurfaceAnchor::None;
  CreativeBounds bounds;
  double totalThicknessMeters = 0.0;
  std::string_view reasonCode =
      "creative_structural_surface_not_requested";
};

[[nodiscard]] CreativeStructuralSurfaceRecipeResult
planCreativeStructuralSurface(
    const CreativeStructuralSurfaceRecipeRequest& request) noexcept;

enum class CreativeStructuralSurfaceCutoutStatus : std::uint8_t {
  NotRequested,
  InvalidSurface,
  InvalidCutout,
  CutoutOutsideSurface,
  CutoutConsumesSurface,
  Ready,
};

struct CreativeStructuralSurfaceCutoutRequest {
  CreativeStructuralSurfaceRecipeRequest surface;
  double cutoutMinimumX = 0.0;
  double cutoutMaximumX = 0.0;
  double cutoutMinimumZ = 0.0;
  double cutoutMaximumZ = 0.0;
};

inline constexpr std::size_t kCreativeStructuralSurfaceCutoutPieceCapacity = 4U;

struct CreativeStructuralSurfaceCutoutResult {
  bool accepted = false;
  CreativeStructuralSurfaceCutoutStatus status =
      CreativeStructuralSurfaceCutoutStatus::NotRequested;
  std::array<CreativeStructuralSurfaceRecipeResult,
             kCreativeStructuralSurfaceCutoutPieceCapacity>
      pieces{};
  std::uint8_t pieceCount = 0U;
  std::string_view reasonCode =
      "creative_structural_surface_cutout_not_requested";
};

// Partitions one horizontal structural surface around one rectangular opening.
// Output order is stable: west, east, north, south. Zero-area pieces are
// omitted, allowing openings to touch an outer edge without overlap.
[[nodiscard]] CreativeStructuralSurfaceCutoutResult
planCreativeStructuralSurfaceCutout(
    const CreativeStructuralSurfaceCutoutRequest& request) noexcept;

}  // namespace iggy3d::creative
