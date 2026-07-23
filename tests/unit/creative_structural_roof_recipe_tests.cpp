#include "app/iggy3d/creative/recipes/StructuralRoofRecipe.hpp"
#include "app/iggy3d/creative/Geometry.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numbers>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs, double tolerance = 1.0e-9) {
  return std::abs(lhs - rhs) <= tolerance;
}

bool near(cr::CreativeVec3 lhs,
          cr::CreativeVec3 rhs,
          double tolerance = 1.0e-9) {
  return near(lhs.x, rhs.x, tolerance) &&
         near(lhs.y, rhs.y, tolerance) &&
         near(lhs.z, rhs.z, tolerance);
}

bool near(cr::CreativeBounds lhs,
          cr::CreativeBounds rhs,
          double tolerance = 1.0e-9) {
  return near(lhs.min, rhs.min, tolerance) &&
         near(lhs.max, rhs.max, tolerance);
}

cr::CreativeTransformedBounds resolvedPart(
    const cr::CreativeStructuralRoofPart& part) {
  const cr::CreativeBoundsMetrics metrics = cr::measureCreativeBounds(part.bounds);
  cr::CreativeTransform transform;
  transform.position = metrics.center;
  transform.rotationEulerRadians = part.rotationEulerRadians;
  return cr::resolveCreativeTransformedBounds(part.bounds, transform);
}

cr::CreativeStructuralRoofRecipeRequest request() {
  cr::CreativeStructuralRoofRecipeRequest value;
  value.minimumX = 0.0;
  value.maximumX = 8.0;
  value.minimumZ = 0.0;
  value.maximumZ = 6.0;
  value.supportPlaneMeters = 3.0;
  return value;
}

bool enumValuesPreserveExistingWireContract() {
  return expect(static_cast<std::uint8_t>(cr::CreativeStructuralRoofStyle::Flat) ==
                        0U &&
                    static_cast<std::uint8_t>(
                        cr::CreativeStructuralRoofStyle::Gable) == 1U &&
                    static_cast<std::uint8_t>(
                        cr::CreativeStructuralRoofStyle::Shed) == 2U &&
                    static_cast<std::uint8_t>(
                        cr::CreativeStructuralRoofStyle::Hip) == 3U,
                "new roof styles append after stable flat and gable values");
}

bool flatRoofOwnsThicknessMaterialEdgesAndDrainage() {
  auto value = request();
  value.layerCount = 2U;
  value.overhangMeters = 0.5;
  value.material = cr::CreativeStructuralMaterial::Stone;
  const auto plan = cr::planCreativeStructuralRoof(value);
  return expect(plan.accepted && plan.partCount == 1U &&
                    plan.parts[0].partKind ==
                        cr::CreativeStructuralRoofPartKind::FlatPanel &&
                    plan.parts[0].kind == cr::CreativeObjectKind::Roof,
                "flat roof emits one typed structural panel") &&
         expect(plan.parts[0].bounds.min.x == -0.5 &&
                    plan.parts[0].bounds.max.x == 8.5 &&
                    plan.parts[0].bounds.min.z == -0.5 &&
                    plan.parts[0].bounds.max.z == 6.5 &&
                    plan.parts[0].bounds.min.y == 3.0 &&
                    plan.parts[0].bounds.max.y == 3.5 &&
                    near(plan.thicknessMeters, 0.5) &&
                    plan.material == cr::CreativeStructuralMaterial::Stone,
                "flat roof preserves exact overhang thickness and material") &&
         expect(plan.edgeCount == 4U && plan.drainageSocketCount == 4U &&
                    plan.edges[0].drainageEligible &&
                    plan.edges[1].drainageEligible &&
                    plan.edges[2].drainageEligible &&
                    plan.edges[3].drainageEligible &&
                    plan.drainageSockets[0].positionMeters.y == 3.5,
                "flat roof exposes four typed eaves and drainage sockets");
}

bool shedRoofOwnsDirectionAndOneLowEave() {
  auto value = request();
  value.style = cr::CreativeStructuralRoofStyle::Shed;
  value.slopeDirection =
      cr::CreativeStructuralRoofSlopeDirection::PositiveZ;
  value.pitchDegrees = 30.0;
  const auto south = cr::planCreativeStructuralRoof(value);

  value.slopeDirection =
      cr::CreativeStructuralRoofSlopeDirection::NegativeX;
  const auto west = cr::planCreativeStructuralRoof(value);
  return expect(south.accepted && south.partCount == 1U &&
                    south.parts[0].partKind ==
                        cr::CreativeStructuralRoofPartKind::ShedPanel &&
                    south.parts[0].kind == cr::CreativeObjectKind::RoofSlope &&
                    near(south.riseMeters, std::tan(std::numbers::pi / 6.0) *
                                                     6.0),
                "shed roof emits one thin panel with exact full-span rise") &&
         expect(south.drainageSocketCount == 1U &&
                    south.drainageSockets[0].edge ==
                        cr::CreativeStructuralRoofPerimeterEdge::South &&
                    near(south.ridgeStart.z, 0.0) &&
                    near(south.ridgeEnd.z, 0.0),
                "positive-z shed drains south and crests on north edge") &&
         expect(west.accepted && west.drainageSocketCount == 1U &&
                    west.drainageSockets[0].edge ==
                        cr::CreativeStructuralRoofPerimeterEdge::West &&
                    near(west.riseMeters,
                         std::tan(std::numbers::pi / 6.0) * 8.0),
                "negative-x shed rotates dimensions and drains west");
}

bool gableRoofUsesTwoThinPanelsAndTypedVerges() {
  auto value = request();
  value.style = cr::CreativeStructuralRoofStyle::Gable;
  value.ridgeAxis = cr::CreativeStructuralRoofRidgeAxis::X;
  value.pitchDegrees = 45.0;
  value.overhangMeters = 1.0;
  const auto xPlan = cr::planCreativeStructuralRoof(value);

  value.ridgeAxis = cr::CreativeStructuralRoofRidgeAxis::Z;
  const auto zPlan = cr::planCreativeStructuralRoof(value);
  const cr::CreativeTransformedBounds first = resolvedPart(xPlan.parts[0]);
  const cr::CreativeTransformedBounds second = resolvedPart(xPlan.parts[1]);
  const cr::CreativeBoundsMetrics firstAuthored =
      cr::measureCreativeBounds(xPlan.parts[0].bounds);
  const cr::CreativeBoundsMetrics secondAuthored =
      cr::measureCreativeBounds(xPlan.parts[1].bounds);
  return expect(xPlan.accepted && xPlan.partCount == 2U &&
                    near(xPlan.riseMeters, 4.0) &&
                    xPlan.parts[0].partKind ==
                        cr::CreativeStructuralRoofPartKind::GableFirst &&
                    xPlan.parts[1].partKind ==
                        cr::CreativeStructuralRoofPartKind::GableSecond &&
                    xPlan.parts[0].kind == cr::CreativeObjectKind::RoofSlope &&
                    xPlan.parts[1].kind == cr::CreativeObjectKind::RoofSlope,
                "x-ridge gable emits two thin semantic panels") &&
         expect(first.valid && second.valid && firstAuthored.valid &&
                    secondAuthored.valid && near(firstAuthored.size.x, 10.0) &&
                    near(secondAuthored.size.x, 10.0) &&
                    near(firstAuthored.size.y, 0.25) &&
                    near(secondAuthored.size.y, 0.25) &&
                    near(firstAuthored.size.z, std::sqrt(32.0)) &&
                    near(secondAuthored.size.z, std::sqrt(32.0)) &&
                    first.worldBounds.min.y < 3.0 &&
                    second.worldBounds.min.y < 3.0 &&
                    near(first.worldBounds.max.y, 7.0) &&
                    near(second.worldBounds.max.y, 7.0),
                "gable weather faces meet while thickness extends inward") &&
         expect(near(xPlan.ridgeStart.x, -1.0) &&
                    near(xPlan.ridgeStart.y, 7.0) &&
                    near(xPlan.ridgeStart.z, 3.0) &&
                    near(xPlan.ridgeEnd.x, 9.0) &&
                    near(xPlan.ridgeEnd.y, 7.0) &&
                    xPlan.drainageSocketCount == 2U,
                "gable ridge and two drainage eaves are exact") &&
         expect(zPlan.accepted && zPlan.partCount == 2U &&
                    near(zPlan.riseMeters, 5.0) &&
                    zPlan.drainageSocketCount == 2U &&
                    zPlan.drainageSockets[0].edge ==
                        cr::CreativeStructuralRoofPerimeterEdge::East &&
                    zPlan.drainageSockets[1].edge ==
                        cr::CreativeStructuralRoofPerimeterEdge::West,
                "z-ridge swaps span and eave ownership");
}

bool hipRoofOwnsFourTaperedPanelsAndRidge() {
  auto value = request();
  value.style = cr::CreativeStructuralRoofStyle::Hip;
  value.ridgeAxis = cr::CreativeStructuralRoofRidgeAxis::X;
  value.pitchDegrees = 45.0;
  value.overhangMeters = 1.0;
  value.material = cr::CreativeStructuralMaterial::Timber;
  const auto plan = cr::planCreativeStructuralRoof(value);
  return expect(plan.accepted && plan.partCount == 4U &&
                    plan.parts[0].kind == cr::CreativeObjectKind::HipRoof &&
                    plan.parts[1].kind == cr::CreativeObjectKind::HipRoof &&
                    plan.parts[2].kind == cr::CreativeObjectKind::HipRoof &&
                    plan.parts[3].kind == cr::CreativeObjectKind::HipRoof,
                "hip roof emits four tapered panel owners") &&
         expect(near(plan.riseMeters, 4.0) &&
                    near(plan.ridgeStart.x, 3.0) &&
                    near(plan.ridgeEnd.x, 5.0) &&
                    near(plan.ridgeStart.y, 7.0) &&
                    near(plan.ridgeStart.z, 3.0) &&
                    plan.material == cr::CreativeStructuralMaterial::Timber,
                "hip roof derives equal-pitch ridge and material") &&
         expect(plan.edgeCount == 4U && plan.drainageSocketCount == 4U,
                "hip roof exposes drainage along all four eaves");
}

bool generatedPartsPublishCanonicalDrainageFrames() {
  const cr::CreativeBounds flatBounds{{-4.0, 3.0, -3.0},
                                      {4.0, 3.25, 3.0}};
  cr::CreativeTransform flatTransform;
  flatTransform.position = cr::measureCreativeBounds(flatBounds).center;
  const auto flat = cr::planCreativeStructuralRoofPartSockets(
      cr::CreativeObjectKind::Roof, flatBounds, flatTransform);

  auto value = request();
  value.style = cr::CreativeStructuralRoofStyle::Gable;
  value.ridgeAxis = cr::CreativeStructuralRoofRidgeAxis::X;
  value.pitchDegrees = 45.0;
  const auto roof = cr::planCreativeStructuralRoof(value);
  const cr::CreativeStructuralRoofPart& first = roof.parts[0];
  cr::CreativeTransform slopeTransform;
  slopeTransform.position = cr::measureCreativeBounds(first.bounds).center;
  slopeTransform.rotationEulerRadians = first.rotationEulerRadians;
  const auto slope = cr::planCreativeStructuralRoofPartSockets(
      first.kind, first.bounds, slopeTransform);

  return expect(flat.accepted && flat.socketCount == 4U &&
                    flat.sockets[0].edge ==
                        cr::CreativeStructuralRoofPerimeterEdge::North &&
                    flat.sockets[1].edge ==
                        cr::CreativeStructuralRoofPerimeterEdge::East &&
                    flat.sockets[2].edge ==
                        cr::CreativeStructuralRoofPerimeterEdge::South &&
                    flat.sockets[3].edge ==
                        cr::CreativeStructuralRoofPerimeterEdge::West &&
                    near(flat.sockets[0].localPosition.y, 0.125),
                "flat generated panel publishes four top-edge receivers") &&
         expect(roof.accepted && slope.accepted && slope.socketCount == 1U &&
                    slope.sockets[0].edge ==
                        cr::CreativeStructuralRoofPerimeterEdge::North &&
                    near(slope.sockets[0].localPosition.y, 0.125) &&
                    near(slope.sockets[0].localPosition.z,
                         -std::sqrt(18.0) * 0.5),
                "sloped panel publishes one receiver on its visible low eave") &&
         expect(cr::creativeStructuralRoofDrainageSocketName(
                    slope.sockets[0].edge) == "roof_drainage_north" &&
                    cr::creativeStructuralRoofDrainageCompatibility() ==
                        "roof.drainage",
                "drainage receivers use one stable name and compatibility");
}

double aperturePieceArea(
    const cr::CreativeStructuralRoofApertureResult& plan) {
  double area = 0.0;
  for (std::size_t index = 0U; index < plan.pieceCount; ++index) {
    const cr::CreativeBoundsMetrics metrics =
        cr::measureCreativeBounds(plan.pieces[index].part.bounds);
    if (!metrics.valid) {
      return -1.0;
    }
    area += metrics.size.x * metrics.size.z;
  }
  return area;
}

bool flatAndShedAperturesPartitionExactPanels() {
  cr::CreativeStructuralRoofApertureRequest flat;
  flat.roof = request();
  flat.apertures[0] = {cr::CreativeStructuralRoofApertureKind::Skylight,
                       2.0, 4.0, 2.0, 4.0};
  flat.apertureCount = 1U;
  const auto flatPlan = cr::planCreativeStructuralRoofApertures(flat);
  const cr::CreativeBoundsMetrics flatInsert =
      cr::measureCreativeBounds(flatPlan.inserts[0].bounds);

  cr::CreativeStructuralRoofApertureRequest shed = flat;
  shed.roof.style = cr::CreativeStructuralRoofStyle::Shed;
  shed.roof.slopeDirection =
      cr::CreativeStructuralRoofSlopeDirection::PositiveZ;
  shed.apertures[0] = {cr::CreativeStructuralRoofApertureKind::Skylight,
                       2.0, 4.0, 3.5, 4.5};
  const auto shedPlan = cr::planCreativeStructuralRoofApertures(shed);
  const cr::CreativeBoundsMetrics shedInsert =
      cr::measureCreativeBounds(shedPlan.inserts[0].bounds);
  const cr::CreativeTransformedBounds resolvedShedInsert =
      cr::resolveCreativeTransformedBounds(
          shedPlan.inserts[0].bounds,
          {shedInsert.center, shedPlan.inserts[0].rotationEulerRadians,
           {1.0, 1.0, 1.0}});
  const cr::CreativeVec3 shedInsertNormal = cr::rotateCreativeVectorEulerXyz(
      {0.0, 1.0, 0.0}, shedPlan.inserts[0].rotationEulerRadians);
  const cr::CreativeVec3 shedInsertWeatherCenter{
      resolvedShedInsert.center.x + shedInsertNormal.x * shedInsert.size.y * 0.5,
      resolvedShedInsert.center.y + shedInsertNormal.y * shedInsert.size.y * 0.5,
      resolvedShedInsert.center.z + shedInsertNormal.z * shedInsert.size.y * 0.5};

  return expect(flatPlan.accepted && flatPlan.pieceCount == 4U &&
                    flatPlan.insertCount == 1U && flatInsert.valid &&
                    near(aperturePieceArea(flatPlan), 44.0) &&
                    near(flatInsert.center.x, 3.0) &&
                    near(flatInsert.center.z, 3.0) &&
                    near(flatInsert.size.x, 2.0) &&
                    near(flatInsert.size.y, 0.25) &&
                    near(flatInsert.size.z, 2.0),
                "flat skylight partitions four panels around one exact insert") &&
         expect(shedPlan.accepted && shedPlan.pieceCount == 4U &&
                    shedPlan.insertCount == 1U && shedInsert.valid &&
                    resolvedShedInsert.valid &&
                    near(shedInsert.size.x, 2.0) &&
                    near(shedInsert.size.z,
                         1.0 / std::cos(std::numbers::pi / 6.0)) &&
                    near(shedInsertWeatherCenter.x, 3.0) &&
                    near(shedInsertWeatherCenter.z, 4.0) &&
                    near(shedPlan.pieces[0].part.rotationEulerRadians,
                         shedPlan.roof.parts[0].rotationEulerRadians),
                "shed skylight maps plan footprint onto the sloped panel");
}

bool gableAperturesRespectPanelAndInsertSemantics() {
  cr::CreativeStructuralRoofApertureRequest value;
  value.roof = request();
  value.roof.style = cr::CreativeStructuralRoofStyle::Gable;
  value.roof.ridgeAxis = cr::CreativeStructuralRoofRidgeAxis::X;
  value.apertures[0] = {
      cr::CreativeStructuralRoofApertureKind::Skylight,
      1.0, 2.0, 0.75, 1.75};
  value.apertures[1] = {
      cr::CreativeStructuralRoofApertureKind::ChimneyClearance,
      5.0, 6.0, 4.25, 5.25};
  value.apertureCount = 2U;
  const auto plan = cr::planCreativeStructuralRoofApertures(value);
  const auto repeated = cr::planCreativeStructuralRoofApertures(value);

  cr::CreativeStructuralRoofApertureRequest ridge = value;
  ridge.apertureCount = 1U;
  ridge.apertures[0] = {
      cr::CreativeStructuralRoofApertureKind::Skylight,
      2.0, 4.0, 2.75, 3.25};
  const auto ridgePlan = cr::planCreativeStructuralRoofApertures(ridge);

  bool deterministic = plan.accepted && repeated.accepted &&
                       plan.pieceCount == repeated.pieceCount &&
                       plan.insertCount == repeated.insertCount;
  for (std::size_t index = 0U; deterministic && index < plan.pieceCount;
       ++index) {
    deterministic =
        plan.pieces[index].sourcePartIndex ==
            repeated.pieces[index].sourcePartIndex &&
        plan.pieces[index].part.partKind ==
            repeated.pieces[index].part.partKind &&
        near(plan.pieces[index].part.bounds,
             repeated.pieces[index].part.bounds) &&
        near(plan.pieces[index].part.rotationEulerRadians,
             repeated.pieces[index].part.rotationEulerRadians);
  }

  return expect(plan.accepted && plan.pieceCount == 8U &&
                    plan.insertCount == 1U &&
                    plan.inserts[0].apertureIndex == 0U &&
                    plan.inserts[0].kind == cr::CreativeObjectKind::Window &&
                    deterministic,
                "gable openings partition both panels in stable source order") &&
         expect(ridgePlan.status ==
                    cr::CreativeStructuralRoofApertureStatus::
                        ApertureCrossesPanelBoundary &&
                    ridgePlan.failedApertureIndex == 0U &&
                    ridgePlan.pieceCount == 0U &&
                    ridgePlan.insertCount == 0U,
                "ridge-crossing aperture rejects atomically");
}

bool invalidAperturesFailClosedWithExplicitLimits() {
  cr::CreativeStructuralRoofApertureRequest outside;
  outside.roof = request();
  outside.apertures[0] = {
      cr::CreativeStructuralRoofApertureKind::Skylight,
      -0.05, 1.0, 1.0, 2.0};
  outside.apertureCount = 1U;

  cr::CreativeStructuralRoofApertureRequest overlap = outside;
  overlap.apertures[0] = {
      cr::CreativeStructuralRoofApertureKind::Skylight,
      1.0, 2.0, 1.0, 2.0};
  overlap.apertures[1] = {
      cr::CreativeStructuralRoofApertureKind::ChimneyClearance,
      1.5, 2.5, 1.5, 2.5};
  overlap.apertureCount = 2U;

  cr::CreativeStructuralRoofApertureRequest hip = outside;
  hip.roof.style = cr::CreativeStructuralRoofStyle::Hip;
  hip.apertures[0] = {
      cr::CreativeStructuralRoofApertureKind::Skylight,
      1.0, 2.0, 1.0, 2.0};

  cr::CreativeStructuralRoofApertureRequest invalidCount = outside;
  invalidCount.apertureCount =
      cr::kCreativeStructuralRoofApertureCapacity + 1U;
  cr::CreativeStructuralRoofApertureRequest invalidClearance = outside;
  invalidClearance.minimumClearanceMeters =
      std::numeric_limits<double>::quiet_NaN();
  cr::CreativeStructuralRoofApertureRequest invalidKind = outside;
  invalidKind.apertures[0].kind =
      cr::CreativeStructuralRoofApertureKind::Count;

  cr::CreativeStructuralRoofApertureRequest noApertures;
  noApertures.roof = request();
  noApertures.roof.style = cr::CreativeStructuralRoofStyle::Hip;
  const auto unchangedHip =
      cr::planCreativeStructuralRoofApertures(noApertures);

  const auto outsidePlan =
      cr::planCreativeStructuralRoofApertures(outside);
  const auto overlapPlan =
      cr::planCreativeStructuralRoofApertures(overlap);
  const auto hipPlan = cr::planCreativeStructuralRoofApertures(hip);

  return expect(outsidePlan.status ==
                        cr::CreativeStructuralRoofApertureStatus::
                            ApertureOutsideRoof &&
                    outsidePlan.failedApertureIndex == 0U,
                "roof aperture preserves perimeter material clearance") &&
         expect(overlapPlan.status ==
                        cr::CreativeStructuralRoofApertureStatus::
                            AperturesTooClose &&
                    overlapPlan.failedApertureIndex == 1U,
                "overlapping roof apertures reject") &&
         expect(hipPlan.status ==
                        cr::CreativeStructuralRoofApertureStatus::
                            UnsupportedRoofStyle &&
                    hipPlan.failedApertureIndex == 0U,
                "hip aperture reports polygon-storage requirement") &&
         expect(cr::planCreativeStructuralRoofApertures(invalidCount).status ==
                    cr::CreativeStructuralRoofApertureStatus::InvalidCount &&
                    cr::planCreativeStructuralRoofApertures(invalidClearance)
                            .status ==
                        cr::CreativeStructuralRoofApertureStatus::
                            InvalidClearance &&
                    cr::planCreativeStructuralRoofApertures(invalidKind).status ==
                        cr::CreativeStructuralRoofApertureStatus::
                            InvalidAperture,
                "capacity non-finite and invalid enum inputs fail closed") &&
         expect(unchangedHip.accepted && unchangedHip.pieceCount == 4U &&
                    unchangedHip.insertCount == 0U,
                "hip roof remains valid when no aperture is requested");
}

bool invalidRoofInputsFailClosed() {
  auto invalidPitch = request();
  invalidPitch.style = cr::CreativeStructuralRoofStyle::Gable;
  invalidPitch.pitchDegrees = 90.0;
  auto invalidOverhang = request();
  invalidOverhang.overhangMeters = -0.1;
  auto nonFinite = request();
  nonFinite.maximumX = std::numeric_limits<double>::infinity();
  auto zeroLayers = request();
  zeroLayers.layerCount = 0U;
  auto invalidDirection = request();
  invalidDirection.slopeDirection =
      cr::CreativeStructuralRoofSlopeDirection::Count;
  auto invalidMaterial = request();
  invalidMaterial.material = cr::CreativeStructuralMaterial::Count;
  auto shortHipRidge = request();
  shortHipRidge.style = cr::CreativeStructuralRoofStyle::Hip;
  shortHipRidge.ridgeAxis = cr::CreativeStructuralRoofRidgeAxis::Z;
  return expect(!cr::planCreativeStructuralRoof(invalidPitch).accepted,
                "unsafe roof pitch rejects") &&
         expect(!cr::planCreativeStructuralRoof(invalidOverhang).accepted,
                "negative roof overhang rejects") &&
         expect(!cr::planCreativeStructuralRoof(nonFinite).accepted,
                "non-finite roof footprint rejects") &&
         expect(!cr::planCreativeStructuralRoof(zeroLayers).accepted,
                "zero roof layers reject") &&
         expect(!cr::planCreativeStructuralRoof(invalidDirection).accepted,
                "invalid shed direction rejects even for flat defaults") &&
         expect(!cr::planCreativeStructuralRoof(invalidMaterial).accepted,
                "invalid roof material rejects") &&
         expect(cr::planCreativeStructuralRoof(shortHipRidge).status ==
                    cr::CreativeStructuralRoofRecipeStatus::InvalidRidgeSpan,
                "hip rejects a selected ridge axis shorter than its cross span");
}

}  // namespace

int main() {
  const bool ok = enumValuesPreserveExistingWireContract() &&
                  flatRoofOwnsThicknessMaterialEdgesAndDrainage() &&
                  shedRoofOwnsDirectionAndOneLowEave() &&
                  gableRoofUsesTwoThinPanelsAndTypedVerges() &&
                  hipRoofOwnsFourTaperedPanelsAndRidge() &&
                  generatedPartsPublishCanonicalDrainageFrames() &&
                  flatAndShedAperturesPartitionExactPanels() &&
                  gableAperturesRespectPanelAndInsertSemantics() &&
                  invalidAperturesFailClosedWithExplicitLimits() &&
                  invalidRoofInputsFailClosed();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
