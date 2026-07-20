#include "EditorDraftingStyle.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>

namespace {

using iggy3d_creative_app::CreativeEditorDraftingFillPattern;
using iggy3d_creative_app::CreativeEditorDraftingRole;
using iggy3d_creative_app::CreativeEditorDraftingStrokeClass;
using iggy3d_creative_app::CreativeEditorDraftingStyle;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float actual, float expected) {
  return std::fabs(actual - expected) <= 1.0e-3F;
}

constexpr std::size_t kRoleCount =
    static_cast<std::size_t>(CreativeEditorDraftingRole::Count);

const CreativeEditorDraftingStyle& style(CreativeEditorDraftingRole role) {
  return iggy3d_creative_app::creativeEditorDraftingStyle(role);
}

float thickness(CreativeEditorDraftingRole role, float pixelsPerCell) {
  return iggy3d_creative_app::creativeEditorDraftingStrokeThicknessPixels(
      style(role), pixelsPerCell);
}

float luminance(const CreativeEditorDraftingStyle& value) {
  return 0.299F * static_cast<float>(value.tint.r) +
         0.587F * static_cast<float>(value.tint.g) +
         0.114F * static_cast<float>(value.tint.b);
}

bool everyRoleHasAStyleAndAUniqueName() {
  bool allNamed = true;
  bool allUnique = true;
  bool reservedSeparate = true;
  for (std::size_t index = 0U; index < kRoleCount; ++index) {
    const auto role = static_cast<CreativeEditorDraftingRole>(index);
    const std::string_view name = iggy3d_creative_app::toString(role);
    if (name.empty() || name == "unknown") {
      std::cerr << "unnamed role at index " << index << '\n';
      allNamed = false;
    }
    for (std::size_t other = index + 1U; other < kRoleCount; ++other) {
      if (name == iggy3d_creative_app::toString(
                      static_cast<CreativeEditorDraftingRole>(other))) {
        std::cerr << "duplicate role name: " << name << '\n';
        allUnique = false;
      }
    }
    for (const std::string_view reserved :
         iggy3d_creative_app::creativeEditorDraftingReservedSymbolNames()) {
      if (name == reserved) {
        std::cerr << "reserved name in production enum: " << name << '\n';
        reservedSeparate = false;
      }
    }
  }
  const bool reservedListed =
      !iggy3d_creative_app::creativeEditorDraftingReservedSymbolNames()
           .empty();
  return expect(allNamed, "every role owns a real name") &&
         expect(allUnique, "role names never collide") &&
         expect(reservedSeparate,
                "reserved sheet symbols never enter the production enum") &&
         expect(reservedListed, "the reserved symbol list is declared") &&
         expect(iggy3d_creative_app::toString(
                    CreativeEditorDraftingRole::Count) == "unknown",
                "out-of-range roles answer unknown");
}

bool stylesAreStructurallyValid() {
  bool allValid = true;
  for (std::size_t index = 0U; index < kRoleCount; ++index) {
    const auto role = static_cast<CreativeEditorDraftingRole>(index);
    const CreativeEditorDraftingStyle& value = style(role);
    const bool alphaSane = value.fillAlpha >= 0.0F && value.fillAlpha <= 1.0F;
    const bool dashSane =
        (value.dashCells <= 0.0F) == (value.gapCells <= 0.0F) &&
        value.dashCells >= 0.0F && value.gapCells >= 0.0F;
    const bool floorSane = value.minimumThicknessPixels >= 1.0F;
    const bool layered =
        value.normalLayer || value.overheadLayer || value.contextLayer;
    const bool fillSane =
        value.fillPattern != CreativeEditorDraftingFillPattern::None ||
        value.fillAlpha == 0.0F;
    const bool drawsSomething =
        value.strokeClass != CreativeEditorDraftingStrokeClass::None ||
        value.fillPattern != CreativeEditorDraftingFillPattern::None;
    if (!alphaSane || !dashSane || !floorSane || !layered || !fillSane ||
        !drawsSomething) {
      std::cerr << "structurally invalid style: "
                << iggy3d_creative_app::toString(role) << '\n';
      allValid = false;
    }
  }
  return expect(allValid,
                "every style carries sane alpha, dash, floor, layer, and "
                "ink parameters");
}

bool visualLawsHold() {
  using Role = CreativeEditorDraftingRole;
  bool exteriorHeavier = true;
  for (const float pixelsPerCell : {6.0F, 12.0F, 24.0F, 48.0F}) {
    if (thickness(Role::ExteriorWall, pixelsPerCell) <
        thickness(Role::InteriorPartition, pixelsPerCell)) {
      exteriorHeavier = false;
    }
  }
  const bool exteriorStrictlyHeavierWhenZoomed =
      thickness(Role::ExteriorWall, 24.0F) >
      thickness(Role::InteriorPartition, 24.0F);
  const CreativeEditorDraftingStyle& roofOutline = style(Role::RoofOutline);
  const CreativeEditorDraftingStyle& roofRidge = style(Role::RoofRidge);
  const bool roofOverheadOnly =
      roofOutline.overheadLayer && !roofOutline.normalLayer &&
      roofRidge.overheadLayer && !roofRidge.normalLayer;
  const bool roofDashedAndLighter =
      roofOutline.dashCells > 0.0F && roofRidge.dashCells > 0.0F &&
      thickness(Role::RoofOutline, 24.0F) <
          thickness(Role::ExteriorWall, 24.0F);
  const CreativeEditorDraftingStyle& ghost =
      style(Role::LowerLevelGhostOverlay);
  const bool ghostIsContext = ghost.contextLayer && !ghost.normalLayer &&
                              ghost.tint.a < 160U &&
                              ghost.drawOrder < style(Role::RoomFloor).drawOrder;
  const CreativeEditorDraftingStyle& invalid =
      style(Role::PreviewInvalidOverlay);
  const bool invalidHasShapeCue =
      invalid.fillPattern == CreativeEditorDraftingFillPattern::Hatched &&
      invalid.dashCells > 0.0F;
  bool interactiveOverlaysPaintLast = true;
  for (const Role overlayRole :
       {Role::HoverOverlay, Role::SelectedOverlay, Role::PreviewValidOverlay,
        Role::PreviewInvalidOverlay, Role::LockedOverlay,
        Role::GeneratedOverlay}) {
    if (style(overlayRole).drawOrder <= style(Role::RegionMask).drawOrder) {
      interactiveOverlaysPaintLast = false;
    }
  }
  const bool layeringSane =
      style(Role::RoomFloor).drawOrder <
          style(Role::InteriorPartition).drawOrder &&
      style(Role::InteriorPartition).drawOrder <
          style(Role::ExteriorWall).drawOrder &&
      style(Role::ElevationBand).drawOrder <
          style(Role::ContourMinor).drawOrder;
  return expect(exteriorHeavier,
                "exterior walls never read lighter than partitions") &&
         expect(exteriorStrictlyHeavierWhenZoomed,
                "exterior walls read strictly heavier at working zoom") &&
         expect(roofOverheadOnly, "roof geometry lives only overhead") &&
         expect(roofDashedAndLighter,
                "overhead roof reads dashed and lighter than walls") &&
         expect(ghostIsContext,
                "lower-storey ghost is context-only, faded, painted under") &&
         expect(invalidHasShapeCue,
                "invalid previews carry a hatch-and-dash shape cue") &&
         expect(interactiveOverlaysPaintLast,
                "interactive state overlays paint above all geometry") &&
         expect(layeringSane,
                "fills paint under lines and partitions under exteriors");
}

bool terrainSemanticsRemainDistinct() {
  using Role = CreativeEditorDraftingRole;
  constexpr Role kTerrainRoles[] = {
      Role::Hill,      Role::Valley, Role::Crater,
      Role::Ridge,     Role::Plateau, Role::Road,
      Role::River,     Role::Ditch, Role::RidgeLine,
  };
  bool allDistinct = true;
  for (std::size_t first = 0U; first < std::size(kTerrainRoles); ++first) {
    for (std::size_t second = first + 1U; second < std::size(kTerrainRoles);
         ++second) {
      const CreativeEditorDraftingStyle& a = style(kTerrainRoles[first]);
      const CreativeEditorDraftingStyle& b = style(kTerrainRoles[second]);
      const bool colorDiffers = a.tint.r != b.tint.r || a.tint.g != b.tint.g ||
                                a.tint.b != b.tint.b;
      const bool structureDiffers =
          a.strokeClass != b.strokeClass || !near(a.dashCells, b.dashCells) ||
          !near(a.gapCells, b.gapCells) || !near(a.fillAlpha, b.fillAlpha);
      if (!colorDiffers && !structureDiffers) {
        std::cerr << "indistinguishable terrain styles: "
                  << iggy3d_creative_app::toString(kTerrainRoles[first])
                  << " vs "
                  << iggy3d_creative_app::toString(kTerrainRoles[second])
                  << '\n';
        allDistinct = false;
      }
    }
  }
  return expect(allDistinct,
                "every authored terrain recipe remains visually distinct");
}

bool overlaysStayDistinguishableInGrayscale() {
  using Role = CreativeEditorDraftingRole;
  constexpr Role kOverlays[] = {
      Role::HoverOverlay,         Role::SelectedOverlay,
      Role::PreviewValidOverlay,  Role::PreviewInvalidOverlay,
      Role::LockedOverlay,        Role::GeneratedOverlay,
      Role::OverheadOverlay,      Role::LowerLevelGhostOverlay};
  bool allDistinguishable = true;
  for (std::size_t first = 0U; first < std::size(kOverlays); ++first) {
    for (std::size_t second = first + 1U; second < std::size(kOverlays);
         ++second) {
      const CreativeEditorDraftingStyle& a = style(kOverlays[first]);
      const CreativeEditorDraftingStyle& b = style(kOverlays[second]);
      const bool structureDiffers =
          a.strokeClass != b.strokeClass ||
          std::lround(a.dashCells * 10.0F) !=
              std::lround(b.dashCells * 10.0F) ||
          std::lround(a.gapCells * 10.0F) != std::lround(b.gapCells * 10.0F) ||
          a.fillPattern != b.fillPattern;
      const bool luminanceDiffers =
          std::fabs(luminance(a) - luminance(b)) >= 32.0F;
      if (!structureDiffers && !luminanceDiffers) {
        std::cerr << "grayscale-ambiguous overlays: "
                  << iggy3d_creative_app::toString(kOverlays[first]) << " vs "
                  << iggy3d_creative_app::toString(kOverlays[second]) << '\n';
        allDistinguishable = false;
      }
    }
  }
  return expect(allDistinguishable,
                "every overlay pair differs in structure or luminance");
}

bool strokeThicknessFollowsClassAndFloor() {
  using Role = CreativeEditorDraftingRole;
  const bool hairlineFixed = near(thickness(Role::ContourMinor, 6.0F), 1.0F) &&
                             near(thickness(Role::ContourMinor, 48.0F), 1.0F);
  const bool lightFloored = near(thickness(Role::Stair, 6.0F), 1.0F);
  const bool lightScales = near(thickness(Role::Stair, 48.0F), 3.36F);
  const bool heavyScales = near(thickness(Role::ExteriorWall, 24.0F), 4.32F);
  const bool mediumScales =
      near(thickness(Role::InteriorPartition, 24.0F), 2.64F);
  const bool fillDrawsNoStroke = near(thickness(Role::RoomFloor, 48.0F), 0.0F);
  return expect(hairlineFixed, "hairlines stay at the one-pixel floor") &&
         expect(lightFloored,
                "light strokes clamp to the floor at far zoom-out") &&
         expect(lightScales && mediumScales && heavyScales,
                "stroke classes scale linearly with pixels per cell") &&
         expect(fillDrawsNoStroke, "fill-only roles draw no stroke");
}

}  // namespace

int main() {
  bool ok = true;
  ok = everyRoleHasAStyleAndAUniqueName() && ok;
  ok = stylesAreStructurallyValid() && ok;
  ok = visualLawsHold() && ok;
  ok = terrainSemanticsRemainDistinct() && ok;
  ok = overlaysStayDistinguishableInGrayscale() && ok;
  ok = strokeThicknessFollowsClassAndFloor() && ok;
  return ok ? 0 : 1;
}
