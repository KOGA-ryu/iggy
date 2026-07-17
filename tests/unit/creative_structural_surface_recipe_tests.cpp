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

bool cutoutPartitionsSurfaceWithoutOverlap() {
  cr::CreativeStructuralSurfaceCutoutRequest cutout;
  cutout.surface = request(cr::CreativeObjectKind::Floor, 4.0, 2U);
  cutout.cutoutMinimumX = 0.0;
  cutout.cutoutMaximumX = 2.0;
  cutout.cutoutMinimumZ = 4.0;
  cutout.cutoutMaximumZ = 6.0;
  const auto result = cr::planCreativeStructuralSurfaceCutout(cutout);

  return expect(result.accepted && result.pieceCount == 4U,
                "interior cutout produces four pieces") &&
         expect(result.pieces[0].bounds.min.x == -2.0 &&
                    result.pieces[0].bounds.max.x == 0.0 &&
                    result.pieces[0].bounds.min.z == 3.0 &&
                    result.pieces[0].bounds.max.z == 8.0,
                "west piece spans the full outer depth") &&
         expect(result.pieces[1].bounds.min.x == 2.0 &&
                    result.pieces[1].bounds.max.x == 6.0,
                "east piece starts at the cutout edge") &&
         expect(result.pieces[2].bounds.min.x == 0.0 &&
                    result.pieces[2].bounds.max.x == 2.0 &&
                    result.pieces[2].bounds.min.z == 3.0 &&
                    result.pieces[2].bounds.max.z == 4.0,
                "north piece only fills between side strips") &&
         expect(result.pieces[3].bounds.min.z == 6.0 &&
                    result.pieces[3].bounds.max.z == 8.0 &&
                    result.pieces[3].bounds.min.y == 3.9 &&
                    result.pieces[3].bounds.max.y == 4.0,
                "south piece preserves the structural anchor and thickness");
}

bool edgeCutoutsOmitDegeneratePiecesAndRejectEscapes() {
  cr::CreativeStructuralSurfaceCutoutRequest edge;
  edge.surface = request(cr::CreativeObjectKind::Ceiling, 7.0);
  edge.cutoutMinimumX = -2.0;
  edge.cutoutMaximumX = 1.0;
  edge.cutoutMinimumZ = 3.0;
  edge.cutoutMaximumZ = 5.0;
  const auto edgeResult = cr::planCreativeStructuralSurfaceCutout(edge);

  edge.cutoutMaximumX = 7.0;
  const auto escaped = cr::planCreativeStructuralSurfaceCutout(edge);
  edge.cutoutMaximumX = edge.cutoutMinimumX;
  const auto degenerate = cr::planCreativeStructuralSurfaceCutout(edge);
  edge.cutoutMinimumX = edge.surface.minimumX;
  edge.cutoutMaximumX = edge.surface.maximumX;
  edge.cutoutMinimumZ = edge.surface.minimumZ;
  edge.cutoutMaximumZ = edge.surface.maximumZ;
  const auto consumed = cr::planCreativeStructuralSurfaceCutout(edge);

  return expect(edgeResult.accepted && edgeResult.pieceCount == 2U,
                "corner cutout omits two zero-area pieces") &&
         expect(!escaped.accepted &&
                    escaped.status ==
                        cr::CreativeStructuralSurfaceCutoutStatus::
                            CutoutOutsideSurface,
                "cutout outside the structural footprint rejects") &&
         expect(
             !degenerate.accepted &&
                 degenerate.status ==
                     cr::CreativeStructuralSurfaceCutoutStatus::InvalidCutout,
             "degenerate cutout rejects") &&
         expect(!consumed.accepted && consumed.pieceCount == 0U &&
                    consumed.status ==
                        cr::CreativeStructuralSurfaceCutoutStatus::
                            CutoutConsumesSurface,
                "cutout cannot consume the entire structural surface");
}

}  // namespace

int main() {
  const bool ok = descriptorAnchorsProduceExactBounds() &&
                  invalidRequestsFailClosed() &&
                  cutoutPartitionsSurfaceWithoutOverlap() &&
                  edgeCutoutsOmitDegeneratePiecesAndRejectEscapes();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
