#include "app/iggy3d/map_maker/Presentation.hpp"

#include <iomanip>
#include <sstream>

namespace iggy3d {
namespace {

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
  overlay.dotCount = snapshot.dotCount;
  overlay.majorDotCount = snapshot.majorDotCount;
  // branch-gate: BG-1205
  if (overlay.visible) {
    overlay.dots = snapshot.dots;
  }
  return overlay;
}

ProductMapMakerHud buildProductMapMakerHud(
    bool mapMakerActive,
    const ProductMapMakerGridSnapshot& grid) {
  ProductMapMakerHud hud;
  hud.visible = mapMakerActive;
  // branch-gate: BG-1205
  if (!mapMakerActive) {
    return hud;
  }

  std::ostringstream line;
  line << "MAP grid=" << formatMeters(grid.pitchMeters)
       << "m major=" << formatMeters(grid.majorStepMeters)
       << "m y=" << formatMeters(grid.planeY);
  appendLine(hud, line.str());
  return hud;
}

}  // namespace iggy3d
