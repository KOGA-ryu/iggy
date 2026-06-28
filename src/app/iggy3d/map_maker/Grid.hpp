#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "core/math/Vec3.hpp"

namespace iggy3d {

enum class ProductMapMakerGridStatus : std::uint8_t {
  Ready,
  Disabled,
  InvalidConfig,
};

struct ProductMapMakerGridConfig {
  bool enabled = false;
  float pitchMeters = 1.0F;
  float majorStepMeters = 5.0F;
  float extentXMeters = 40.0F;
  float extentYMeters = 8.0F;
  float extentZMeters = 40.0F;
  float planeY = 0.0F;
  Vec3 anchorWorld;
};

struct ProductMapMakerGridDot {
  Vec3 worldPosition;
  bool major = false;
};

struct ProductMapMakerGridSnapshot {
  bool ok = false;
  bool visible = false;
  ProductMapMakerGridStatus status = ProductMapMakerGridStatus::Disabled;
  std::string reasonCode = "map_maker_grid_disabled";
  float pitchMeters = 1.0F;
  float majorStepMeters = 5.0F;
  float planeY = 0.0F;
  std::uint64_t layerCount = 0;
  std::uint64_t dotCount = 0;
  std::uint64_t majorDotCount = 0;
  std::vector<ProductMapMakerGridDot> dots;
};

bool isValidProductMapMakerGridConfig(const ProductMapMakerGridConfig& config);
std::string_view productMapMakerGridStatusName(ProductMapMakerGridStatus status);
ProductMapMakerGridSnapshot buildProductMapMakerGridSnapshot(
    const ProductMapMakerGridConfig& config);
float nextProductMapMakerGridPitch(float currentPitchMeters);
float previousProductMapMakerGridPitch(float currentPitchMeters);

}  // namespace iggy3d
