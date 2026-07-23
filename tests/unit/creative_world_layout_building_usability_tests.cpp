#include "app/iggy3d/creative/world/WorldLayoutBlockoutMaterialization.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingUsability.hpp"
#include "runtime/movement/MovementParams.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs) {
  return std::abs(lhs - rhs) <= 1.0e-6;
}

cr::CreativeWorldLayoutBuildingBlockoutRecipe blockoutRecipe(
    cr::CreativeWorldLayoutBuildingBlockoutPattern pattern,
    std::uint16_t storeyCount = 1U) {
  cr::CreativeWorldLayoutBuildingBlockoutRecipe recipe;
  recipe.request.footprint = {{0, 0}, {12, 10}};
  recipe.request.pattern = pattern;
  recipe.request.wallThicknessCells = 0.25;
  recipe.request.floorToFloorCells = 3U;
  recipe.request.storeys.count = storeyCount;
  recipe.request.storeys.connectStoreys = storeyCount > 1U;
  recipe.request.facade.includeEntrance = true;
  recipe.request.facade.includeExteriorWindows = false;
  recipe.floorTopLayer = 0.0;
  return recipe;
}

cr::CreativeWorldLayout materialize(
    const cr::CreativeWorldLayoutBuildingBlockoutRecipe& recipe) {
  cr::CreativeWorldLayout source;
  source.stableKey = "building_usability";
  const cr::CreativeWorldLayoutBuildingEditResult result =
      cr::materializeCreativeWorldLayoutBuildingBlockout(source, recipe, 1U);
  if (!result.accepted) {
    std::cerr << "FAIL: usability blockout fixture materialized\n";
    return {};
  }
  return result.edited;
}

bool hasIssue(const cr::CreativeWorldLayoutBuildingUsabilityReceipt& receipt,
              cr::CreativeWorldLayoutBuildingUsabilityIssueKind kind,
              cr::CreativeWorldLayoutTable table,
              std::size_t index) {
  return std::any_of(
      receipt.issues.begin(), receipt.issues.begin() + receipt.issueCount,
      [kind, table, index](const auto& issue) {
        return issue.kind == kind && issue.table == table &&
               issue.index == index;
      });
}

bool validTwoStoreyBlockoutIsUsableAndGenerated() {
  const cr::CreativeWorldLayout layout = materialize(blockoutRecipe(
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom, 2U));
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Building Usability Valid");
  static_cast<void>(document.assignId(9951U));
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  const cr::CreativeWorldLayoutPreviewResult preview =
      compiled.receipt.accepted
          ? cr::previewCreativeWorldLayoutPlan(document, compiled.plan)
          : cr::CreativeWorldLayoutPreviewResult{};
  cr::CreativeWorldLayoutBuildingUsabilityConfig config;
  config.gridCellSizeMeters = document.gridSettings().cellSizeMeters;
  const cr::CreativeWorldLayoutBuildingUsabilityReceipt receipt =
      cr::validateCreativeWorldLayoutBuildingUsability(
          {&layout, preview.accepted ? &preview.document : nullptr, config});

  return expect(compiled.receipt.accepted && preview.accepted,
                "valid usability fixture compiles exact generated geometry") &&
         expect(receipt.accepted && receipt.usable &&
                    receipt.status ==
                        cr::CreativeWorldLayoutBuildingUsabilityStatus::Ready &&
                    receipt.buildingCount == 1U &&
                    receipt.usableBuildingCount == 1U &&
                    receipt.roomCount == 2U &&
                    receipt.reachableRoomCount == 2U &&
                    receipt.issueCount == 0U,
                "exterior door and stair reach every generated room");
}

bool buildingWithoutRoomsIsNeverCountedUsable() {
  cr::CreativeWorldLayout layout;
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "empty_building";
  building.name = "Empty Building";
  layout.buildings.push_back(building);

  const cr::CreativeWorldLayoutBuildingUsabilityReceipt receipt =
      cr::validateCreativeWorldLayoutBuildingUsability({&layout, nullptr, {}});
  return expect(receipt.accepted && !receipt.usable &&
                    receipt.buildingCount == 1U &&
                    receipt.usableBuildingCount == 0U &&
                    hasIssue(
                        receipt,
                        cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
                            BuildingWithoutRooms,
                        cr::CreativeWorldLayoutTable::Building, 0U),
                "roomless building is reported and never counted usable");
}

bool missingEntranceIdentifiesTheBuilding() {
  cr::CreativeWorldLayoutBuildingBlockoutRecipe recipe = blockoutRecipe(
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom);
  recipe.request.facade.includeEntrance = false;
  const cr::CreativeWorldLayout layout = materialize(recipe);
  const cr::CreativeWorldLayoutBuildingUsabilityReceipt receipt =
      cr::validateCreativeWorldLayoutBuildingUsability({&layout, nullptr, {}});

  return expect(receipt.accepted && !receipt.usable &&
                    receipt.issueCount == 1U,
                "room without an exterior door is not usable") &&
         expect(hasIssue(
                    receipt,
                    cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
                        MissingExteriorEntrance,
                    cr::CreativeWorldLayoutTable::Building, 0U),
                "missing entrance points to its building source");
}

bool missingInteriorDoorsIdentifyDisconnectedRooms() {
  cr::CreativeWorldLayoutBuildingBlockoutRecipe recipe = blockoutRecipe(
      cr::CreativeWorldLayoutBuildingBlockoutPattern::Grid2x2);
  recipe.request.connectRooms = false;
  const cr::CreativeWorldLayout layout = materialize(recipe);
  const cr::CreativeWorldLayoutBuildingUsabilityReceipt receipt =
      cr::validateCreativeWorldLayoutBuildingUsability({&layout, nullptr, {}});
  const std::size_t disconnected = static_cast<std::size_t>(std::count_if(
      receipt.issues.begin(), receipt.issues.begin() + receipt.issueCount,
      [](const auto& issue) {
        return issue.kind ==
               cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
                   DisconnectedRoom;
      }));

  return expect(receipt.accepted && !receipt.usable &&
                    receipt.roomCount == 4U &&
                    receipt.reachableRoomCount == 1U && disconnected == 3U,
                "one entrance cannot reach three unconnected rooms");
}

bool sharedInteriorDoorsConnectEveryRoom() {
  cr::CreativeWorldLayout layout = materialize(blockoutRecipe(
      cr::CreativeWorldLayoutBuildingBlockoutPattern::Grid2x2));
  for (cr::CreativeWorldLayoutOpening& opening : layout.openings) {
    if (opening.hostKind ==
            cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
        opening.roomTopologyEdgeIndex !=
            cr::kInvalidCreativeWorldLayoutIndex) {
      opening.roomEdge = cr::CreativeWorldLayoutRoomEdge::Count;
    }
  }
  const cr::CreativeWorldLayoutBuildingUsabilityReceipt receipt =
      cr::validateCreativeWorldLayoutBuildingUsability({&layout, nullptr, {}});

  return expect(receipt.accepted && receipt.usable &&
                    receipt.roomCount == 4U &&
                    receipt.reachableRoomCount == 4U &&
                    receipt.issueCount == 0U,
                "direct topology-edge doors connect the complete floor plan");
}

bool undersizedEntranceReportsClearanceAndNoUsableEntrance() {
  cr::CreativeWorldLayout layout = materialize(blockoutRecipe(
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom));
  if (layout.openings.empty()) {
    return expect(false, "clearance fixture owns an exterior entrance");
  }
  layout.openings[0].widthCells = 0.4;
  const cr::CreativeWorldLayoutBuildingUsabilityReceipt receipt =
      cr::validateCreativeWorldLayoutBuildingUsability({&layout, nullptr, {}});

  return expect(hasIssue(
                    receipt,
                    cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
                        OpeningClearanceTooSmall,
                    cr::CreativeWorldLayoutTable::Opening, 0U),
                "undersized door identifies its opening source") &&
         expect(hasIssue(
                    receipt,
                    cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
                        MissingExteriorEntrance,
                    cr::CreativeWorldLayoutTable::Building, 0U),
                "an impassable door does not count as an entrance");
}

bool missingStoreyConnectorIdentifiesLevelAndRoom() {
  cr::CreativeWorldLayoutBuildingBlockoutRecipe recipe = blockoutRecipe(
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom, 2U);
  recipe.request.storeys.connectStoreys = false;
  const cr::CreativeWorldLayout layout = materialize(recipe);
  const cr::CreativeWorldLayoutBuildingUsabilityReceipt receipt =
      cr::validateCreativeWorldLayoutBuildingUsability({&layout, nullptr, {}});

  return expect(hasIssue(
                    receipt,
                    cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
                        MissingVerticalConnection,
                    cr::CreativeWorldLayoutTable::Level, 1U),
                "upper storey without stairs identifies its level") &&
         expect(hasIssue(
                    receipt,
                    cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
                        DisconnectedRoom,
                    cr::CreativeWorldLayoutTable::Room, 1U),
                "upper room remains unreachable without a connector");
}

bool invalidConnectorOwnershipIsNotMisreportedAsClearance() {
  cr::CreativeWorldLayout layout = materialize(blockoutRecipe(
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom, 2U));
  if (layout.verticalConnectors.empty() || layout.rooms.size() < 2U ||
      layout.rooms[1].levelIndex >= layout.levels.size()) {
    return expect(false, "connector ownership fixture is complete");
  }
  layout.levels[layout.rooms[1].levelIndex].buildingIndex =
      cr::kInvalidCreativeWorldLayoutIndex;
  const cr::CreativeWorldLayoutBuildingUsabilityReceipt receipt =
      cr::validateCreativeWorldLayoutBuildingUsability({&layout, nullptr, {}});

  return expect(hasIssue(
                    receipt,
                    cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
                        InvalidConnector,
                    cr::CreativeWorldLayoutTable::VerticalConnector, 0U),
                "mismatched level ownership identifies an invalid connector") &&
         expect(!hasIssue(
                    receipt,
                    cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
                        ConnectorClearanceTooSmall,
                    cr::CreativeWorldLayoutTable::VerticalConnector, 0U),
                "ownership defects are not mislabeled as player clearance");
}

bool steepRampDoesNotCountAsAUsableStoreyConnection() {
  cr::CreativeWorldLayout layout = materialize(blockoutRecipe(
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom, 2U));
  if (layout.verticalConnectors.empty()) {
    return expect(false, "ramp clearance fixture owns a connector");
  }
  layout.verticalConnectors[0].kind =
      cr::CreativeWorldLayoutVerticalConnectorKind::Ramp;
  const cr::CreativeWorldLayoutBuildingUsabilityReceipt receipt =
      cr::validateCreativeWorldLayoutBuildingUsability({&layout, nullptr, {}});

  return expect(hasIssue(
                    receipt,
                    cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
                        ConnectorClearanceTooSmall,
                    cr::CreativeWorldLayoutTable::VerticalConnector, 0U),
                "ramp steeper than the movement envelope is rejected") &&
         expect(hasIssue(
                    receipt,
                    cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
                        MissingVerticalConnection,
                    cr::CreativeWorldLayoutTable::Level, 1U),
                "unusable ramp does not satisfy upper-storey connectivity");
}

bool generatedObjectGapsRemainSourceAddressable() {
  const cr::CreativeWorldLayout layout = materialize(blockoutRecipe(
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom, 2U));
  cr::CreativeDocument emptyGenerated =
      cr::CreativeDocument::create("Missing Generated Building");
  static_cast<void>(emptyGenerated.assignId(9952U));
  const cr::CreativeWorldLayoutBuildingUsabilityReceipt receipt =
      cr::validateCreativeWorldLayoutBuildingUsability(
          {&layout, &emptyGenerated, {}});

  return expect(receipt.accepted && !receipt.usable &&
                    hasIssue(
                        receipt,
                        cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
                            MissingGeneratedFloor,
                        cr::CreativeWorldLayoutTable::Room, 0U) &&
                    hasIssue(
                        receipt,
                        cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
                            MissingGeneratedOpening,
                        cr::CreativeWorldLayoutTable::Opening, 0U) &&
                    hasIssue(
                        receipt,
                        cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
                            MissingGeneratedConnector,
                        cr::CreativeWorldLayoutTable::VerticalConnector, 0U),
                "missing generated facts point back to semantic sources");
}

bool generatedFailuresPrecedeBoundedAdvisoryIssues() {
  cr::CreativeWorldLayout layout = materialize(blockoutRecipe(
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom));
  if (layout.openings.empty()) {
    return expect(false, "bounded issue fixture owns an entrance");
  }
  cr::CreativeWorldLayoutOpening narrow = layout.openings[0];
  narrow.includeInsert = false;
  narrow.widthCells = 0.1;
  for (std::size_t index = 0U; index < 40U; ++index) {
    narrow.stableKey = "narrow_" + std::to_string(index);
    layout.openings.push_back(narrow);
  }
  cr::CreativeDocument emptyGenerated =
      cr::CreativeDocument::create("Bounded Building Issues");
  static_cast<void>(emptyGenerated.assignId(9953U));
  const cr::CreativeWorldLayoutBuildingUsabilityReceipt receipt =
      cr::validateCreativeWorldLayoutBuildingUsability(
          {&layout, &emptyGenerated, {}});

  return expect(receipt.capacityExceeded &&
                    receipt.issueCount == receipt.issues.size() &&
                    receipt.droppedIssueCount > 0U,
                "advisory overflow is explicit and bounded") &&
         expect(receipt.issues[0].kind ==
                        cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
                            MissingGeneratedFloor &&
                    receipt.issues[1].kind ==
                        cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
                            MissingGeneratedOpening,
                "generated output failures retain priority over warnings");
}

bool defaultEnvelopeMatchesRuntimePlayer() {
  const cr::CreativeWorldLayoutBuildingUsabilityConfig config;
  const iggy3d::MovementParams movement;
  return expect(near(config.actorRadiusMeters, movement.radiusMeters) &&
                    near(config.actorHeightMeters, movement.heightMeters) &&
                    near(config.maximumStepMeters,
                         movement.stepHeightMeters) &&
                    near(config.skinMeters, movement.skinMeters) &&
                    near(config.maximumRampSlopeDegrees,
                         movement.maxWalkableSlopeDegrees),
                "usability defaults stay pinned to runtime player movement");
}

bool invalidRequestsFailClosed() {
  cr::CreativeWorldLayout layout;
  cr::CreativeWorldLayoutBuildingUsabilityConfig invalid;
  invalid.actorRadiusMeters = -1.0;
  const auto missing =
      cr::validateCreativeWorldLayoutBuildingUsability({});
  const auto malformed = cr::validateCreativeWorldLayoutBuildingUsability(
      {&layout, nullptr, invalid});
  return expect(!missing.accepted && !missing.usable &&
                    missing.status ==
                        cr::CreativeWorldLayoutBuildingUsabilityStatus::
                            InvalidRequest &&
                    !malformed.accepted && !malformed.usable,
                "missing and malformed requests fail closed");
}

}  // namespace

int main() {
  const bool ok = validTwoStoreyBlockoutIsUsableAndGenerated() &&
                  buildingWithoutRoomsIsNeverCountedUsable() &&
                  missingEntranceIdentifiesTheBuilding() &&
                  missingInteriorDoorsIdentifyDisconnectedRooms() &&
                  sharedInteriorDoorsConnectEveryRoom() &&
                  undersizedEntranceReportsClearanceAndNoUsableEntrance() &&
                  missingStoreyConnectorIdentifiesLevelAndRoom() &&
                  invalidConnectorOwnershipIsNotMisreportedAsClearance() &&
                  steepRampDoesNotCountAsAUsableStoreyConnection() &&
                  generatedObjectGapsRemainSourceAddressable() &&
                  generatedFailuresPrecedeBoundedAdvisoryIssues() &&
                  defaultEnvelopeMatchesRuntimePlayer() &&
                  invalidRequestsFailClosed();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
