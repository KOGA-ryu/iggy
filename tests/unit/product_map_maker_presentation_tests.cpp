#include "app/iggy3d/map_maker/Presentation.hpp"

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
  return std::fabs(lhs - rhs) <= 0.001F;
}

iggy3d::ProductMapMakerGridSnapshot readyGrid() {
  iggy3d::ProductMapMakerGridConfig config;
  config.enabled = true;
  config.pitchMeters = 0.5F;
  config.majorStepMeters = 5.0F;
  config.extentXMeters = 1.0F;
  config.extentYMeters = 1.0F;
  config.extentZMeters = 1.0F;
  config.planeY = 2.0F;
  return iggy3d::buildProductMapMakerGridSnapshot(config);
}

bool overlayCopiesReadyGridOnly() {
  const iggy3d::ProductMapMakerGridSnapshot grid = readyGrid();
  const iggy3d::ProductMapMakerGridOverlay overlay =
      iggy3d::buildProductMapMakerGridOverlay(grid);
  iggy3d::ProductMapMakerGridSnapshot hidden = grid;
  hidden.visible = false;
  const iggy3d::ProductMapMakerGridOverlay hiddenOverlay =
      iggy3d::buildProductMapMakerGridOverlay(hidden);

  return expect(overlay.visible, "ready overlay visible") &&
         expect(overlay.status == "map_maker_grid_ready", "overlay status") &&
         expect(overlay.pitchMeters == 0.5F, "pitch copied") &&
         expect(overlay.layerCount == grid.layerCount, "layer count copied") &&
         expect(overlay.dotCount == grid.dotCount, "dot count copied") &&
         expect(overlay.dots.size() == grid.dots.size(), "dots copied") &&
         expect(!hiddenOverlay.visible, "hidden overlay not visible") &&
         expect(hiddenOverlay.dots.empty(), "hidden overlay omits dots");
}

bool hudShowsCompactGridLineWhenActive() {
  const iggy3d::ProductMapMakerGridSnapshot grid = readyGrid();
  const iggy3d::ProductMapMakerHud inactive =
      iggy3d::buildProductMapMakerHud(false, grid);
  const iggy3d::ProductMapMakerHud active =
      iggy3d::buildProductMapMakerHud(true, grid);

  return expect(!inactive.visible, "inactive hud hidden") &&
         expect(inactive.lines.empty(), "inactive no lines") &&
         expect(active.visible, "active hud visible") &&
         expect(active.lineCount == 1U, "active line count") &&
         expect(active.lines.size() == 1U, "active vector count") &&
         expect(active.lines[0].visible, "active line visible") &&
         expect(active.lines[0].text == "MAP grid=0.5m major=5m layers=3",
                "active line text");
}

bool cubePreviewSnapsOneMeterCubeInFrontOfCamera() {
  const iggy3d::ProductMapMakerGridSnapshot grid = readyGrid();
  const iggy3d::ProductMapMakerCubePreview inactive =
      iggy3d::buildProductMapMakerCubePreview(false, {0.0F, 2.0F, 0.0F}, 90.0F, grid);
  const iggy3d::ProductMapMakerCubePreview cube =
      iggy3d::buildProductMapMakerCubePreview(true, {0.1F, 2.0F, 0.2F}, 90.0F, grid);
  const iggy3d::ProductMapMakerHud hud =
      iggy3d::buildProductMapMakerHud(true, grid, cube);

  return expect(!inactive.visible, "inactive cube hidden") &&
         expect(inactive.reasonCode == "map_maker_cube_disabled",
                "inactive cube reason") &&
         expect(cube.visible, "cube visible") &&
         expect(cube.status == "map_maker_cube_ready", "cube ready") &&
         expect(cube.stableName == "map_maker.unit_cube_preview",
                "cube stable name") &&
         expect(near(cube.sizeMeters.x, 1.0F) && near(cube.sizeMeters.y, 1.0F) &&
                    near(cube.sizeMeters.z, 1.0F),
                "cube is one meter") &&
         expect(near(cube.centerWorld.x, 3.0F), "cube snapped x") &&
         expect(near(cube.centerWorld.y, 2.5F), "cube sits on grid plane") &&
         expect(near(cube.centerWorld.z, 0.0F), "cube snapped z") &&
         expect(hud.lines.size() == 1U, "hud one line with cube") &&
         expect(hud.lines[0].text ==
                    "MAP grid=0.5m major=5m layers=3 cube=1m",
                "hud cube text");
}

}  // namespace

int main() {
  const bool ok = overlayCopiesReadyGridOnly() &&
                  hudShowsCompactGridLineWhenActive() &&
                  cubePreviewSnapsOneMeterCubeInFrontOfCamera();
  if (!ok) {
    return 1;
  }
  std::cout << "product_map_maker_presentation_tests=pass\n";
  return 0;
}
