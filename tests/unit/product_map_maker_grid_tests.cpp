#include "app/iggy3d/map_maker/Grid.hpp"

#include <cmath>
#include <iostream>

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    return false;
  }
  return true;
}

bool near(float lhs, float rhs) {
  return std::fabs(lhs - rhs) <= 0.0001F;
}

bool vecNear(iggy3d::Vec3 lhs, iggy3d::Vec3 rhs) {
  return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
}

bool disabledAndInvalidStatus() {
  iggy3d::ProductMapMakerGridConfig config;
  const iggy3d::ProductMapMakerGridSnapshot disabled =
      iggy3d::buildProductMapMakerGridSnapshot(config);
  config.enabled = true;
  config.pitchMeters = 0.0F;
  const iggy3d::ProductMapMakerGridSnapshot invalid =
      iggy3d::buildProductMapMakerGridSnapshot(config);

  return expect(iggy3d::productMapMakerGridStatusName(
                    iggy3d::ProductMapMakerGridStatus::Ready) ==
                    "map_maker_grid_ready",
                "ready status name") &&
         expect(!disabled.ok, "disabled not ok") &&
         expect(!disabled.visible, "disabled hidden") &&
         expect(disabled.reasonCode == "map_maker_grid_disabled",
                "disabled reason") &&
         expect(!invalid.ok, "invalid not ok") &&
         expect(invalid.reasonCode == "map_maker_grid_invalid_config",
                "invalid reason");
}

bool readyGridBuildsStableDots() {
  iggy3d::ProductMapMakerGridConfig config;
  config.enabled = true;
  config.pitchMeters = 1.0F;
  config.majorStepMeters = 2.0F;
  config.extentXMeters = 2.0F;
  config.extentYMeters = 2.0F;
  config.extentZMeters = 2.0F;
  config.planeY = 4.0F;
  config.anchorWorld = {0.0F, 0.0F, 0.0F};

  const iggy3d::ProductMapMakerGridSnapshot grid =
      iggy3d::buildProductMapMakerGridSnapshot(config);

  return expect(grid.ok, "grid ok") &&
         expect(grid.visible, "grid visible") &&
         expect(grid.reasonCode == "map_maker_grid_ready", "grid ready") &&
         expect(grid.layerCount == 3U, "three layer count") &&
         expect(grid.dotCount == 27U, "3x3x3 dot count") &&
         expect(grid.majorDotCount == 1U, "major dot count") &&
         expect(grid.dots.size() == 27U, "dot vector count") &&
         expect(vecNear(grid.dots.front().worldPosition, {-1.0F, -1.0F, -1.0F}),
                "first dot layer row-major") &&
         expect(vecNear(grid.dots[13].worldPosition, {0.0F, 0.0F, 0.0F}),
                "center dot") &&
         expect(grid.dots[13].major, "center is major") &&
         expect(vecNear(grid.dots.back().worldPosition, {1.0F, 1.0F, 1.0F}),
                "last dot row-major");
}

bool cyclesKnownPitches() {
  return expect(near(iggy3d::nextProductMapMakerGridPitch(1.0F), 0.5F),
                "next 1 to half") &&
         expect(near(iggy3d::nextProductMapMakerGridPitch(0.5F), 0.25F),
                "next half to quarter") &&
         expect(near(iggy3d::previousProductMapMakerGridPitch(0.25F), 0.5F),
                "previous quarter to half") &&
         expect(near(iggy3d::previousProductMapMakerGridPitch(1.0F), 0.1F),
                "previous wraps");
}

}  // namespace

int main() {
  const bool ok = disabledAndInvalidStatus() && readyGridBuildsStableDots() &&
                  cyclesKnownPitches();
  if (!ok) {
    return 1;
  }
  std::cout << "product_map_maker_grid_tests=pass\n";
  return 0;
}
