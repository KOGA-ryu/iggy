#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "app/iggy3d/map_maker/Grid.hpp"

namespace iggy3d {

struct ProductMapMakerGridOverlay {
  bool visible = false;
  std::string status = "map_maker_grid_disabled";
  std::string reasonCode = "map_maker_grid_disabled";
  float pitchMeters = 1.0F;
  float majorStepMeters = 5.0F;
  float planeY = 0.0F;
  std::uint64_t dotCount = 0;
  std::uint64_t majorDotCount = 0;
  std::vector<ProductMapMakerGridDot> dots;
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
ProductMapMakerHud buildProductMapMakerHud(
    bool mapMakerActive,
    const ProductMapMakerGridSnapshot& grid);

}  // namespace iggy3d
