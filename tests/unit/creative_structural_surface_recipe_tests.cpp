#include "app/iggy3d/creative/recipes/StructuralSurfaceRecipe.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
#include <type_traits>

namespace {
namespace cr = iggy3d::creative;

static_assert(std::is_standard_layout_v<
              cr::CreativeStructuralSurfaceRecipeRequest>);
static_assert(std::is_trivially_copyable_v<
              cr::CreativeStructuralSurfaceRecipeRequest>);

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeStructuralSurfaceRecipeRequest request(
    cr::CreativeObjectKind kind, double anchorPlaneMeters,
    std::uint16_t layerCount = 1U) {
  return {kind, -2.0, 6.0, 3.0, 8.0, anchorPlaneMeters, layerCount};
}

bool descriptorAnchorsProduceExactBounds() {
  const auto floor = cr::planCreativeStructuralSurface(
      request(cr::CreativeObjectKind::Floor, 4.0, 2U));
  const auto ceiling = cr::planCreativeStructuralSurface(
      request(cr::CreativeObjectKind::Ceiling, 7.0, 2U));
  const auto roof = cr::planCreativeStructuralSurface(
      request(cr::CreativeObjectKind::Roof, 9.0));

  return expect(floor.accepted &&
                    floor.anchor == cr::CreativeStructuralSurfaceAnchor::TopPlane &&
                    floor.totalThicknessMeters == 0.1 &&
                    floor.bounds.min.x == -2.0 && floor.bounds.max.x == 6.0 &&
                    floor.bounds.min.z == 3.0 && floor.bounds.max.z == 8.0 &&
                    floor.bounds.min.y == 3.9 && floor.bounds.max.y == 4.0,
                "floor material extends below its finished top plane") &&
         expect(ceiling.accepted &&
                    ceiling.anchor ==
                        cr::CreativeStructuralSurfaceAnchor::BottomPlane &&
                    ceiling.totalThicknessMeters == 0.5 &&
                    ceiling.bounds.min.y == 7.0 &&
                    ceiling.bounds.max.y == 7.5,
                "ceiling material extends above its support plane") &&
         expect(roof.accepted &&
                    roof.anchor ==
                        cr::CreativeStructuralSurfaceAnchor::BottomPlane &&
                    roof.totalThicknessMeters == 1.0 &&
                    roof.bounds.min.y == 9.0 && roof.bounds.max.y == 10.0,
                "roof material extends above its support plane");
}

bool invalidRequestsFailClosed() {
  auto unsupportedRequest = request(cr::CreativeObjectKind::Wall, 0.0);
  auto zeroLayersRequest = request(cr::CreativeObjectKind::Floor, 0.0, 0U);
  auto nonFiniteFootprint = request(cr::CreativeObjectKind::Floor, 0.0);
  nonFiniteFootprint.maximumX = std::numeric_limits<double>::infinity();
  auto nonFiniteAnchor = request(cr::CreativeObjectKind::Floor, 0.0);
  nonFiniteAnchor.anchorPlaneMeters =
      std::numeric_limits<double>::quiet_NaN();
  auto unrepresentable = request(
      cr::CreativeObjectKind::Roof, std::numeric_limits<double>::max());

  const auto unsupported =
      cr::planCreativeStructuralSurface(unsupportedRequest);
  const auto zeroLayers =
      cr::planCreativeStructuralSurface(zeroLayersRequest);
  const auto badFootprint =
      cr::planCreativeStructuralSurface(nonFiniteFootprint);
  const auto badAnchor = cr::planCreativeStructuralSurface(nonFiniteAnchor);
  const auto overflow = cr::planCreativeStructuralSurface(unrepresentable);

  return expect(!unsupported.accepted &&
                    unsupported.status ==
                        cr::CreativeStructuralSurfaceRecipeStatus::UnsupportedKind,
                "non-horizontal object kind is unsupported") &&
         expect(!zeroLayers.accepted &&
                    zeroLayers.status == cr::CreativeStructuralSurfaceRecipeStatus::
                                             InvalidLayerCount,
                "zero structural layers reject") &&
         expect(!badFootprint.accepted &&
                    badFootprint.status ==
                        cr::CreativeStructuralSurfaceRecipeStatus::InvalidFootprint,
                "non-finite footprint rejects") &&
         expect(!badAnchor.accepted &&
                    badAnchor.status == cr::CreativeStructuralSurfaceRecipeStatus::
                                            InvalidAnchorPlane,
                "non-finite anchor rejects") &&
         expect(!overflow.accepted &&
                    overflow.status == cr::CreativeStructuralSurfaceRecipeStatus::
                                           UnrepresentableBounds,
                "overflowing bounds reject without publishing geometry");
}

}  // namespace

int main() {
  const bool ok = descriptorAnchorsProduceExactBounds() &&
                  invalidRequestsFailClosed();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
