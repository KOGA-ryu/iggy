#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "app/iggy3d/map_maker/Grid.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d {

// TODO(map-maker): Replace the hardcoded cube preview with a visible toolbelt
// model that exposes the selected creative asset/tool and preview parameters.
// See docs/build_packets/product_map_maker_grid_authoring_v1.md.
struct ProductMapMakerGridOverlay {
  bool visible = false;
  std::string status = "map_maker_grid_disabled";
  std::string reasonCode = "map_maker_grid_disabled";
  float pitchMeters = 1.0F;
  float majorStepMeters = 5.0F;
  float planeY = 0.0F;
  std::uint64_t layerCount = 0;
  std::uint64_t dotCount = 0;
  std::uint64_t majorDotCount = 0;
  std::vector<ProductMapMakerGridDot> dots;
};

struct ProductMapMakerCubePreview {
  bool visible = false;
  std::string status = "map_maker_cube_disabled";
  std::string reasonCode = "map_maker_cube_disabled";
  std::string stableName = "map_maker.unit_cube_preview";
  Vec3 centerWorld;
  Vec3 sizeMeters{1.0F, 1.0F, 1.0F};
};

struct ProductMapMakerHudLine {
  std::string text;
  bool visible = false;
};

struct ProductMapMakerHud {
  bool visible = false;
  std::uint64_t lineCount = 0;
  std::vector<ProductMapMakerHudLine> lines;
};

ProductMapMakerGridOverlay buildProductMapMakerGridOverlay(
    const ProductMapMakerGridSnapshot& snapshot);
ProductMapMakerCubePreview buildProductMapMakerCubePreview(
    bool mapMakerActive,
    Vec3 anchorWorld,
    float cameraYawDegrees,
    const ProductMapMakerGridSnapshot& grid);
ProductMapMakerHud buildProductMapMakerHud(
    bool mapMakerActive,
    const ProductMapMakerGridSnapshot& grid,
    const ProductMapMakerCubePreview& cube = {});

}  // namespace iggy3d
