#include "app/iggy3d/map_maker/Presentation.hpp"

#include <cmath>
#include <iomanip>
#include <sstream>

namespace iggy3d {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kCubePreviewDistanceMeters = 3.0F;

float snapToPitch(float value, float pitchMeters) {
  return std::round(value / pitchMeters) * pitchMeters;
}

std::string formatMeters(float value) {
  std::ostringstream out;
  // branch-gate: BG-1205
  out << std::fixed << std::setprecision(value < 1.0F ? 2 : 1) << value;
  std::string text = out.str();
  while (text.size() > 1U && text.back() == '0') {
    text.pop_back();
  }
  // branch-gate: BG-1205
  if (!text.empty() && text.back() == '.') {
    text.pop_back();
  }
  return text;
}

void appendLine(ProductMapMakerHud& hud, std::string text) {
  hud.lines.push_back(ProductMapMakerHudLine{std::move(text), hud.visible});
  hud.lineCount = hud.lines.size();
}

}  // namespace

ProductMapMakerGridOverlay buildProductMapMakerGridOverlay(
    const ProductMapMakerGridSnapshot& snapshot) {
  ProductMapMakerGridOverlay overlay;
  overlay.visible = snapshot.visible;
  overlay.status = std::string(productMapMakerGridStatusName(snapshot.status));
  overlay.reasonCode = snapshot.reasonCode;
  overlay.pitchMeters = snapshot.pitchMeters;
  overlay.majorStepMeters = snapshot.majorStepMeters;
  overlay.planeY = snapshot.planeY;
  overlay.layerCount = snapshot.layerCount;
  overlay.dotCount = snapshot.dotCount;
  overlay.majorDotCount = snapshot.majorDotCount;
  // branch-gate: BG-1205
  if (overlay.visible) {
    overlay.dots = snapshot.dots;
  }
  return overlay;
}

ProductMapMakerCubePreview buildProductMapMakerCubePreview(
    bool mapMakerActive,
    Vec3 anchorWorld,
    float cameraYawDegrees,
    const ProductMapMakerGridSnapshot& grid) {
  ProductMapMakerCubePreview cube;
  // branch-gate: BG-1206
  if (!mapMakerActive || !grid.ok) {
    cube.status = mapMakerActive ? "map_maker_cube_missing_grid"
                                 : "map_maker_cube_disabled";
    cube.reasonCode = cube.status;
    return cube;
  }

  const float yaw = cameraYawDegrees * kPi / 180.0F;
  const Vec3 forward{std::sin(yaw), 0.0F, -std::cos(yaw)};
  const Vec3 rawCenter = anchorWorld + forward * kCubePreviewDistanceMeters;
  cube.centerWorld = {
      snapToPitch(rawCenter.x, grid.pitchMeters),
      grid.planeY + cube.sizeMeters.y * 0.5F,
      snapToPitch(rawCenter.z, grid.pitchMeters),
  };
  cube.visible = true;
  cube.status = "map_maker_cube_ready";
  cube.reasonCode = cube.status;
  return cube;
}

ProductMapMakerHud buildProductMapMakerHud(
    bool mapMakerActive,
    const ProductMapMakerGridSnapshot& grid,
    const ProductMapMakerCubePreview& cube) {
  ProductMapMakerHud hud;
  hud.visible = mapMakerActive;
  // branch-gate: BG-1205
  if (!mapMakerActive) {
    return hud;
  }

  std::ostringstream line;
  line << "MAP grid=" << formatMeters(grid.pitchMeters)
       << "m major=" << formatMeters(grid.majorStepMeters)
       << "m layers=" << grid.layerCount;
  // branch-gate: BG-1206
  if (cube.visible) {
    line << " cube=1m";
  }
  appendLine(hud, line.str());
  return hud;
}

}  // namespace iggy3d
