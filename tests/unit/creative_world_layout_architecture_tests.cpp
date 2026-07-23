#include "app/iggy3d/creative/world/WorldLayoutArchitecture.hpp"

#include "app/iggy3d/creative/world/WorldLayoutBlockout.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBlockoutMaterialization.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingTraversal.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingUsability.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOpenings.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"
#include "runtime/movement/MovementParams.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

constexpr double kEpsilon = 1.0e-9;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs) noexcept {
  return std::fabs(lhs - rhs) <= kEpsilon;
}

bool sameExplicitRoomTopology(const cr::CreativeWorldLayout& lhs,
                              const cr::CreativeWorldLayout& rhs) {
  if (lhs.topologyVertices.size() != rhs.topologyVertices.size() ||
      lhs.topologyEdges.size() != rhs.topologyEdges.size() ||
      lhs.roomBoundaries.size() != rhs.roomBoundaries.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < lhs.topologyVertices.size(); ++index) {
    const auto& left = lhs.topologyVertices[index];
    const auto& right = rhs.topologyVertices[index];
    if (left.levelIndex != right.levelIndex ||
        left.stableKey != right.stableKey ||
        left.position.x != right.position.x ||
        left.position.z != right.position.z) {
      return false;
    }
  }
  for (std::size_t index = 0U; index < lhs.topologyEdges.size(); ++index) {
    const auto& left = lhs.topologyEdges[index];
    const auto& right = rhs.topologyEdges[index];
    if (left.levelIndex != right.levelIndex ||
        left.stableKey != right.stableKey ||
        left.startVertexIndex != right.startVertexIndex ||
        left.endVertexIndex != right.endVertexIndex ||
        !near(left.wallThicknessCells, right.wallThicknessCells)) {
      return false;
    }
  }
  for (std::size_t index = 0U; index < lhs.roomBoundaries.size(); ++index) {
    const auto& left = lhs.roomBoundaries[index];
    const auto& right = rhs.roomBoundaries[index];
    if (left.roomIndex != right.roomIndex ||
        left.topologyEdgeIndex != right.topologyEdgeIndex ||
        left.order != right.order || left.reversed != right.reversed) {
      return false;
    }
  }
  return true;
}

struct ArchitectureFixture {
  cr::CreativeGridSettings grid;
  cr::CreativeWorldLayout layout;
  std::size_t primaryBuildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::size_t neighborBuildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
};

cr::CreativeWorldLayoutBuildingBlockoutRecipe blockoutRecipe(
    cr::CreativeWorldLayoutRect footprint,
    std::uint16_t storeyCount,
    std::uint16_t floorToFloorCells = 4U) noexcept {
  cr::CreativeWorldLayoutBuildingBlockoutRecipe recipe;
  recipe.request.footprint = footprint;
  recipe.request.pattern =
      cr::CreativeWorldLayoutBuildingBlockoutPattern::Grid2x2;
  recipe.request.floorToFloorCells = floorToFloorCells;
  recipe.request.storeys.count = storeyCount;
  recipe.request.storeys.connectStoreys = storeyCount > 1U;
  recipe.floorTopLayer = 2.0;
  recipe.floorThicknessLayers = 2U;
  recipe.roofThicknessLayers = 1U;
  return recipe;
}

ArchitectureFixture makeFixture(std::uint16_t floorToFloorCells = 4U) {
  ArchitectureFixture fixture;
  fixture.grid.cellSizeMeters = 0.5;
  fixture.layout.stableKey = "architecture_fixture";

  const cr::CreativeWorldLayoutBuildingEditResult primary =
      cr::materializeCreativeWorldLayoutBuildingBlockout(
          fixture.layout,
          blockoutRecipe({{0, 0}, {24, 24}}, 2U, floorToFloorCells), 1U,
          {"Primary Estate", {}});
  if (!primary.accepted) {
    return fixture;
  }
  fixture.primaryBuildingIndex = primary.resultBuildingIndex;

  const cr::CreativeWorldLayoutBuildingEditResult neighbor =
      cr::materializeCreativeWorldLayoutBuildingBlockout(
          primary.edited,
          blockoutRecipe({{32, 0}, {52, 20}}, 1U, floorToFloorCells),
          primary.nextStableOrdinal, {"Neighbor Estate", {}});
  if (!neighbor.accepted) {
    return fixture;
  }
  fixture.layout = neighbor.edited;
  fixture.neighborBuildingIndex = neighbor.resultBuildingIndex;
  return fixture;
}

const cr::CreativeWorldLayoutOpening* firstOwnedOpening(
    const cr::CreativeWorldLayout& layout,
    std::size_t buildingIndex) noexcept {
  for (const cr::CreativeWorldLayoutOpening& opening : layout.openings) {
    if (opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
        opening.roomIndex < layout.rooms.size() &&
        layout.rooms[opening.roomIndex].buildingIndex == buildingIndex) {
      return &opening;
    }
  }
  return nullptr;
}

bool presetsAreExplicitAndValid() {
  const cr::CreativeWorldLayoutArchitecturalProfile residential =
      cr::defaultCreativeWorldLayoutArchitecturalProfile(
          cr::CreativeWorldLayoutArchitecturalProfileKind::Residential);
  const cr::CreativeWorldLayoutArchitecturalProfile grand =
      cr::defaultCreativeWorldLayoutArchitecturalProfile(
          cr::CreativeWorldLayoutArchitecturalProfileKind::Grand);
  cr::CreativeWorldLayoutArchitecturalProfile invalid = residential;
  invalid.floorToFloorMeters = std::numeric_limits<double>::quiet_NaN();
  cr::CreativeGridSettings halfMeterGrid;
  halfMeterGrid.cellSizeMeters = 0.5;
  std::uint16_t residentialCells = 0U;
  std::uint16_t grandCells = 0U;

  return expect(cr::validCreativeWorldLayoutArchitecturalProfile(residential) &&
                    residential.floorToFloorMeters == 3.0 &&
                    residential.floorThicknessLayers == 4U &&
                    residential.ceilingThicknessLayers == 1U &&
                    residential.roofThicknessLayers == 1U,
                "residential profile owns one complete scale contract") &&
         expect(cr::validCreativeWorldLayoutArchitecturalProfile(grand) &&
                    grand.floorToFloorMeters == 5.0 &&
                    grand.floorThicknessLayers == 6U &&
                    grand.ceilingThicknessLayers == 1U &&
                    grand.roofThicknessLayers == 1U,
                "grand profile owns one complete scale contract") &&
         expect(
             cr::resolveCreativeWorldLayoutArchitecturalProfileFloorToFloorCells(
                 halfMeterGrid, residential, residentialCells) &&
                 cr::resolveCreativeWorldLayoutArchitecturalProfileFloorToFloorCells(
                     halfMeterGrid, grand, grandCells) &&
                 residentialCells == 6U && grandCells == 10U,
             "one kernel resolves architectural profiles onto the document grid") &&
         expect(!cr::validCreativeWorldLayoutArchitecturalProfile(invalid),
                "non-finite custom scale fails closed");
}

bool creativeValidationDefaultsMatchTheRuntimePlayer() {
  const iggy3d::MovementParams runtime;
  const cr::CreativeWorldLayoutOpeningClearanceRequest opening;
  const cr::CreativeWorldLayoutBuildingUsabilityConfig usability;
  const cr::CreativeWorldLayoutBuildingTraversalConfig traversal;

  const bool creativeDefaultsMatch =
      opening.actorRadiusMeters == iggy3d::kDefaultPlayerBodyRadiusMeters &&
      opening.actorHeightMeters ==
          iggy3d::kDefaultPlayerStandingHeightMeters &&
      opening.maximumStepMeters == iggy3d::kDefaultPlayerStepHeightMeters &&
      opening.skinMeters == iggy3d::kDefaultPlayerSkinMeters &&
      usability.actorRadiusMeters == iggy3d::kDefaultPlayerBodyRadiusMeters &&
      usability.actorHeightMeters ==
          iggy3d::kDefaultPlayerStandingHeightMeters &&
      usability.maximumStepMeters == iggy3d::kDefaultPlayerStepHeightMeters &&
      usability.skinMeters == iggy3d::kDefaultPlayerSkinMeters &&
      traversal.actorRadiusMeters == iggy3d::kDefaultPlayerBodyRadiusMeters &&
      traversal.actorHeightMeters ==
          iggy3d::kDefaultPlayerStandingHeightMeters &&
      traversal.maximumStepMeters == iggy3d::kDefaultPlayerStepHeightMeters &&
      traversal.groundSnapMeters == iggy3d::kDefaultPlayerGroundSnapMeters &&
      traversal.skinMeters == iggy3d::kDefaultPlayerSkinMeters;
  const auto runtimeNear = [](double canonical, float value) noexcept {
    return std::fabs(canonical - static_cast<double>(value)) <= 1.0e-6;
  };
  const bool runtimeMatches =
      runtimeNear(iggy3d::kDefaultPlayerBodyRadiusMeters,
                  runtime.radiusMeters) &&
      runtimeNear(iggy3d::kDefaultPlayerStandingHeightMeters,
                  runtime.heightMeters) &&
      runtimeNear(iggy3d::kDefaultPlayerStepHeightMeters,
                  runtime.stepHeightMeters) &&
      runtimeNear(iggy3d::kDefaultPlayerGroundSnapMeters,
                  runtime.groundSnapMeters) &&
      runtimeNear(iggy3d::kDefaultPlayerSkinMeters, runtime.skinMeters);

  return expect(creativeDefaultsMatch && runtimeMatches,
                "every Creative clearance proof uses the runtime player envelope") &&
         expect(cr::kCreativeArchitecturalHumanReferenceHeightMeters ==
                    iggy3d::kDefaultPlayerStandingHeightMeters,
                "architectural scale marker uses the runtime player height") &&
         expect(runtimeNear(iggy3d::kDefaultPlayerStepHeightMeters,
                            runtime.clamberBandBottomMeters) &&
                    runtimeNear(iggy3d::kDefaultPlayerStandingHeightMeters,
                                runtime.clamberBandTopMeters),
                "runtime clamber limits derive from the same player envelope");
}

bool selectedBuildingNormalizesAtomically() {
  const ArchitectureFixture fixture = makeFixture();
  if (!expect(fixture.primaryBuildingIndex !=
                  cr::kInvalidCreativeWorldLayoutIndex &&
                  fixture.neighborBuildingIndex !=
                      cr::kInvalidCreativeWorldLayoutIndex,
              "architecture fixture materializes both estates")) {
    return false;
  }

  const cr::CreativeWorldLayoutBuildingTemplateFingerprint neighborBefore =
      cr::fingerprintCreativeWorldLayoutBuilding(
          fixture.layout, fixture.neighborBuildingIndex);
  const cr::CreativeWorldLayoutOpening* openingBefore =
      firstOwnedOpening(fixture.layout, fixture.primaryBuildingIndex);
  if (!expect(neighborBefore.valid && openingBefore != nullptr &&
                  fixture.layout.verticalConnectors.size() == 1U,
              "fixture exposes neighbor opening and stair evidence")) {
    return false;
  }
  const cr::CreativeWorldLayoutOpening openingSnapshot = *openingBefore;
  const cr::CreativeWorldLayoutVerticalConnector connectorSnapshot =
      fixture.layout.verticalConnectors.front();

  cr::CreativeWorldLayoutArchitectureRequest request;
  request.buildingIndex = fixture.primaryBuildingIndex;
  request.profile = cr::defaultCreativeWorldLayoutArchitecturalProfile(
      cr::CreativeWorldLayoutArchitecturalProfileKind::Residential);
  const cr::CreativeWorldLayoutArchitectureResult normalized =
      cr::normalizeCreativeWorldLayoutBuildingArchitecture(
          fixture.grid, fixture.layout, request);
  if (!expect(normalized.receipt.accepted && normalized.receipt.changed &&
                  normalized.receipt.status ==
                      cr::CreativeWorldLayoutArchitectureStatus::Ready,
              "residential normalization accepts one atomic candidate")) {
    return false;
  }

  const cr::CreativeWorldLayoutBuildingTemplateFingerprint neighborAfter =
      cr::fingerprintCreativeWorldLayoutBuilding(
          normalized.edited, fixture.neighborBuildingIndex);
  const cr::CreativeWorldLayoutOpening* openingAfter =
      firstOwnedOpening(normalized.edited, fixture.primaryBuildingIndex);
  const cr::CreativeWorldLayoutVerticalConnector& connectorAfter =
      normalized.edited.verticalConnectors.front();
  const cr::CreativeWorldLayoutVerticalConnectorPlan connectorPlan =
      cr::planCreativeWorldLayoutVerticalConnector(
          fixture.grid, normalized.edited, 0U);
  const cr::CreativeWorldLayoutBuildingDimensions dimensions =
      cr::measureCreativeWorldLayoutBuildingDimensions(
          fixture.grid, normalized.edited, fixture.primaryBuildingIndex);
  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      cr::expandCreativeWorldLayoutRooms(normalized.edited);
  const cr::CreativeWorldLayoutBuildingBlockoutSyncReceipt sync =
      cr::inspectCreativeWorldLayoutBuildingBlockoutSync(
          normalized.edited, fixture.primaryBuildingIndex);

  std::size_t targetLevelCount = 0U;
  bool levelsMatch = true;
  for (const cr::CreativeWorldLayoutLevel& level : normalized.edited.levels) {
    if (level.buildingIndex != fixture.primaryBuildingIndex) {
      continue;
    }
    const double expectedFloor = targetLevelCount == 0U ? 2.0 : 8.0;
    levelsMatch = levelsMatch && near(level.floorTopLayer, expectedFloor) &&
                  level.wallHeightCells == 6U &&
                  level.floorThicknessLayers == 4U &&
                  level.ceilingThicknessLayers == 1U &&
                  level.roofThicknessLayers == 1U;
    ++targetLevelCount;
  }

  std::size_t facadeCount = 0U;
  for (const cr::CreativeWorldLayoutWall& wall : expanded.expanded.walls) {
    if (wall.buildingIndex == fixture.primaryBuildingIndex &&
        near(wall.baseLayer, 2.0) && wall.heightCells == 12U) {
      ++facadeCount;
    }
  }

  const std::int64_t connectorRun =
      connectorAfter.direction ==
                  cr::CreativeWorldLayoutVerticalDirection::PositiveX ||
              connectorAfter.direction ==
                  cr::CreativeWorldLayoutVerticalDirection::NegativeX
          ? static_cast<std::int64_t>(connectorAfter.footprint.maximum.x) -
                connectorAfter.footprint.minimum.x
          : static_cast<std::int64_t>(connectorAfter.footprint.maximum.z) -
                connectorAfter.footprint.minimum.z;

  return expect(targetLevelCount == 2U && levelsMatch &&
                    normalized.edited
                            .buildings[fixture.primaryBuildingIndex]
                            .rootHeightCells == 6U,
                "selected levels become one coherent three-metre stack") &&
         expect(dimensions.accepted &&
                    dimensions.occupiedLevelCount == 2U &&
                    near(dimensions.minimumFloorToFloorMeters, 3.0) &&
                    near(dimensions.maximumFloorToFloorMeters, 3.0) &&
                    near(dimensions.exteriorFacadeHeightMeters, 6.0) &&
                    near(dimensions.totalHeightMeters, 6.45),
                "measured facade slabs and envelope use the profile") &&
         expect(expanded.accepted && facadeCount == 4U,
                "four exterior facades remain continuous across storeys") &&
         expect(openingAfter != nullptr &&
                    openingAfter->stableKey == openingSnapshot.stableKey &&
                    near(openingAfter->centerOffsetCells,
                         openingSnapshot.centerOffsetCells) &&
                    near(openingAfter->widthCells,
                         openingSnapshot.widthCells) &&
                    near(openingAfter->cutoutBottomCells,
                         openingSnapshot.cutoutBottomCells) &&
                    near(openingAfter->cutoutHeightCells,
                         openingSnapshot.cutoutHeightCells),
                "opening identity and local dimensions remain authored") &&
         expect(connectorAfter.stableKey == connectorSnapshot.stableKey &&
                    connectorRun == 6 && connectorPlan.accepted &&
                    near(connectorPlan.riseMeters, 3.0) &&
                    normalized.receipt.updatedVerticalConnectorCount == 1U,
                "stair keeps identity and resizes to the new rise") &&
         expect(neighborAfter == neighborBefore,
                "neighboring estate fingerprint remains exact") &&
         expect(sync.accepted &&
                    sync.state ==
                        cr::CreativeWorldLayoutBuildingBlockoutSyncState::
                            Current &&
                    sync.provenance.recipe.request.floorToFloorCells == 6U &&
                    sync.provenance.recipe.floorThicknessLayers == 4U &&
                    sync.provenance.recipe.ceilingThicknessLayers == 1U &&
                    sync.provenance.recipe.roofThicknessLayers == 1U &&
                    sync.provenance.recipe.architecturalProfileKind ==
                        cr::CreativeWorldLayoutArchitecturalProfileKind::Residential &&
                    normalized.receipt.preservedBlockoutLink,
                "normalization updates rather than severs blockout truth");
}

bool customGeometryRetainsAuthoredIntent() {
  ArchitectureFixture fixture = makeFixture();
  if (!expect(fixture.layout.verticalConnectors.size() == 1U,
              "custom geometry fixture owns one connector")) {
    return false;
  }
  cr::CreativeWorldLayoutVerticalConnector& connector =
      fixture.layout.verticalConnectors.front();
  const bool alongX =
      connector.direction ==
          cr::CreativeWorldLayoutVerticalDirection::PositiveX ||
      connector.direction ==
          cr::CreativeWorldLayoutVerticalDirection::NegativeX;
  if (alongX) {
    connector.footprint.minimum.x -= 2;
    connector.footprint.maximum.x += 2;
  } else {
    connector.footprint.minimum.z -= 2;
    connector.footprint.maximum.z += 2;
  }

  cr::CreativeWorldLayoutBox floor;
  floor.buildingIndex = fixture.primaryBuildingIndex;
  floor.kind = cr::CreativeObjectKind::Floor;
  floor.stableKey = "custom_upper_floor";
  floor.name = "Custom Upper Floor";
  floor.footprint = {{1, 1}, {3, 3}};
  floor.anchorLayer = 6.0;
  floor.layerCount = 2U;
  fixture.layout.boxes.push_back(floor);

  cr::CreativeWorldLayoutBox prop;
  prop.buildingIndex = fixture.primaryBuildingIndex;
  prop.kind = cr::CreativeObjectKind::Prop;
  prop.stableKey = "authored_prop";
  prop.name = "Authored Prop";
  prop.footprint = {{4, 4}, {5, 5}};
  prop.anchorLayer = 2.5;
  prop.layerCount = 1U;
  fixture.layout.boxes.push_back(prop);

  const cr::CreativeWorldLayoutVerticalConnectorPlan beforePlan =
      cr::planCreativeWorldLayoutVerticalConnector(
          fixture.grid, fixture.layout, 0U);
  cr::CreativeWorldLayoutArchitectureRequest request;
  request.buildingIndex = fixture.primaryBuildingIndex;
  request.profile = cr::defaultCreativeWorldLayoutArchitecturalProfile(
      cr::CreativeWorldLayoutArchitecturalProfileKind::Residential);
  const cr::CreativeWorldLayoutArchitectureResult normalized =
      cr::normalizeCreativeWorldLayoutBuildingArchitecture(
          fixture.grid, fixture.layout, request);
  if (!expect(!beforePlan.accepted &&
                  beforePlan.reasonCode ==
                      "creative_stair_headroom_insufficient" &&
                  normalized.receipt.accepted,
              "normalization repairs short headroom without rejecting custom "
              "geometry")) {
    return false;
  }

  const cr::CreativeWorldLayoutVerticalConnector& resolvedConnector =
      normalized.edited.verticalConnectors.front();
  const std::int64_t run =
      alongX
          ? static_cast<std::int64_t>(
                resolvedConnector.footprint.maximum.x) -
                resolvedConnector.footprint.minimum.x
          : static_cast<std::int64_t>(
                resolvedConnector.footprint.maximum.z) -
                resolvedConnector.footprint.minimum.z;
  const cr::CreativeWorldLayoutBox& resolvedFloor =
      normalized.edited.boxes[normalized.edited.boxes.size() - 2U];
  const cr::CreativeWorldLayoutBox& resolvedProp =
      normalized.edited.boxes.back();

  return expect(run == 8,
                "authored gentle connector run is never shortened") &&
         expect(near(resolvedFloor.anchorLayer, 8.0) &&
                    resolvedFloor.layerCount == 4U &&
                    normalized.receipt.updatedStructuralBoxCount == 1U,
                "structural box follows its normalized level plane") &&
         expect(near(resolvedProp.anchorLayer, 2.5) &&
                    resolvedProp.layerCount == 1U,
                "non-structural prop remains exactly authored");
}

bool explicitRoomTopologySurvivesArchitectureNormalization() {
  ArchitectureFixture fixture = makeFixture();
  const cr::CreativeWorldLayoutRoomGraphMaterializeResult materialized =
      cr::materializeCreativeWorldLayoutRoomGraph(fixture.layout);
  if (!expect(materialized.accepted && materialized.changed,
              "architecture fixture materializes canonical room topology")) {
    return false;
  }
  const cr::CreativeWorldLayoutRoomGraph beforeGraph =
      cr::buildCreativeWorldLayoutRoomGraph(materialized.edited);
  bool directOpeningHosts = !materialized.edited.openings.empty();
  for (const cr::CreativeWorldLayoutOpening& opening :
       materialized.edited.openings) {
    directOpeningHosts &=
        opening.hostKind ==
            cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
        opening.roomTopologyEdgeIndex <
            materialized.edited.topologyEdges.size();
  }

  cr::CreativeWorldLayoutArchitectureRequest request;
  request.buildingIndex = fixture.primaryBuildingIndex;
  request.profile = cr::defaultCreativeWorldLayoutArchitecturalProfile(
      cr::CreativeWorldLayoutArchitecturalProfileKind::Residential);
  const cr::CreativeWorldLayoutArchitectureResult normalized =
      cr::normalizeCreativeWorldLayoutBuildingArchitecture(
          fixture.grid, materialized.edited, request);
  const cr::CreativeWorldLayoutRoomGraph afterGraph =
      normalized.receipt.accepted
          ? cr::buildCreativeWorldLayoutRoomGraph(normalized.edited)
          : cr::CreativeWorldLayoutRoomGraph{};

  bool openingHostsUnchanged =
      normalized.edited.openings.size() == materialized.edited.openings.size();
  for (std::size_t index = 0U;
       openingHostsUnchanged && index < normalized.edited.openings.size();
       ++index) {
    const auto& before = materialized.edited.openings[index];
    const auto& after = normalized.edited.openings[index];
    openingHostsUnchanged =
        before.stableKey == after.stableKey &&
        before.roomIndex == after.roomIndex &&
        before.roomTopologyEdgeIndex == after.roomTopologyEdgeIndex;
  }

  return expect(beforeGraph.accepted && beforeGraph.sourceWasExplicit &&
                    directOpeningHosts,
                "precondition owns explicit graph and direct edge hosts") &&
         expect(normalized.receipt.accepted && normalized.receipt.changed,
                "shared architecture kernel accepts explicit topology") &&
         expect(afterGraph.accepted && afterGraph.sourceWasExplicit &&
                    sameExplicitRoomTopology(materialized.edited,
                                             normalized.edited),
                "vertical normalization preserves every topology identity") &&
         expect(openingHostsUnchanged,
                "direct opening hosts survive floor-to-floor changes") &&
         expect(normalized.receipt.updatedLevelCount == 2U &&
                    normalized.receipt.updatedVerticalConnectorCount == 1U,
                "dependent levels and connector update in one result");
}

bool invalidProfilesPublishNoCandidate() {
  const ArchitectureFixture fixture = makeFixture();
  const ArchitectureFixture validFixture = makeFixture(6U);
  cr::CreativeWorldLayoutArchitectureRequest request;
  request.buildingIndex = fixture.primaryBuildingIndex;
  request.profile = cr::defaultCreativeWorldLayoutArchitecturalProfile(
      cr::CreativeWorldLayoutArchitecturalProfileKind::Custom);

  request.profile.floorToFloorMeters = 3.1;
  const cr::CreativeWorldLayoutArchitectureResult fractional =
      cr::normalizeCreativeWorldLayoutBuildingArchitecture(
          fixture.grid, fixture.layout, request);

  request.profile.floorToFloorMeters = 1.0;
  const cr::CreativeWorldLayoutArchitectureResult tooShort =
      cr::normalizeCreativeWorldLayoutBuildingArchitecture(
          fixture.grid, fixture.layout, request);

  cr::CreativeWorldLayout unaligned = fixture.layout;
  cr::CreativeWorldLayoutWall wall;
  wall.buildingIndex = fixture.primaryBuildingIndex;
  wall.stableKey = "custom_unaligned_wall";
  wall.name = "Custom Unaligned Wall";
  wall.start = {1, 1};
  wall.end = {3, 1};
  wall.baseLayer = 2.5;
  wall.heightCells = 1U;
  unaligned.walls.push_back(wall);
  request.profile = cr::defaultCreativeWorldLayoutArchitecturalProfile(
      cr::CreativeWorldLayoutArchitecturalProfileKind::Residential);
  const cr::CreativeWorldLayoutArchitectureResult customWall =
      cr::normalizeCreativeWorldLayoutBuildingArchitecture(
          fixture.grid, unaligned, request);

  request.profile = cr::defaultCreativeWorldLayoutArchitecturalProfile(
      cr::CreativeWorldLayoutArchitecturalProfileKind::Custom);
  request.profile.floorToFloorMeters = 3.0;
  request.profile.floorThicknessLayers = 2U;
  request.profile.ceilingThicknessLayers = 1U;
  request.profile.roofThicknessLayers = 1U;
  const cr::CreativeWorldLayoutArchitectureResult noChange =
      cr::normalizeCreativeWorldLayoutBuildingArchitecture(
          validFixture.grid, validFixture.layout, request);

  return expect(!fractional.receipt.accepted &&
                    fractional.receipt.status ==
                        cr::CreativeWorldLayoutArchitectureStatus::
                            UnrepresentableProfile &&
                    fractional.edited.buildings.empty(),
                "non-grid-representable profiles publish no candidate") &&
         expect(!tooShort.receipt.accepted &&
                    tooShort.receipt.status ==
                        cr::CreativeWorldLayoutArchitectureStatus::
                            OpeningDoesNotFit &&
                    tooShort.edited.buildings.empty(),
                "profiles shorter than existing openings fail atomically") &&
         expect(!customWall.receipt.accepted &&
                    customWall.receipt.status ==
                        cr::CreativeWorldLayoutArchitectureStatus::
                            UnalignedStructure &&
                    customWall.edited.buildings.empty(),
                "ambiguous custom vertical structure fails closed") &&
         expect(noChange.receipt.accepted && !noChange.receipt.changed &&
                    noChange.receipt.status ==
                        cr::CreativeWorldLayoutArchitectureStatus::NoChange &&
                    noChange.receipt.preservedBlockoutLink,
                "matching custom values report an honest no-op");
}

}  // namespace

int main() {
  const bool ok = presetsAreExplicitAndValid() &&
                  creativeValidationDefaultsMatchTheRuntimePlayer() &&
                  selectedBuildingNormalizesAtomically() &&
                  customGeometryRetainsAuthoredIntent() &&
                  explicitRoomTopologySurvivesArchitectureNormalization() &&
                  invalidProfilesPublishNoCandidate();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
