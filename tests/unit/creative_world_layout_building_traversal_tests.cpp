#include "app/iggy3d/creative/world/WorldLayoutBlockoutMaterialization.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingTraversal.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOpenings.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"
#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

struct BuildingFixture {
  cr::CreativeWorldLayout layout;
  cr::CreativeDocument generated;
  bool ready = false;
};

BuildingFixture twoStoreyFixture(
    cr::CreativeWorldLayoutVerticalConnectorKind connectorKind =
        cr::CreativeWorldLayoutVerticalConnectorKind::Stair) {
  cr::CreativeWorldLayoutBuildingBlockoutRecipe recipe;
  recipe.request.footprint =
      connectorKind == cr::CreativeWorldLayoutVerticalConnectorKind::Ramp
          ? cr::CreativeWorldLayoutRect{{0, 0}, {24, 20}}
          : cr::CreativeWorldLayoutRect{{0, 0}, {12, 10}};
  recipe.request.pattern =
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom;
  recipe.request.wallThicknessCells = 0.25;
  recipe.request.wallHeightCells = 3U;
  recipe.request.storeys.count = 2U;
  recipe.request.storeys.connectStoreys = true;
  recipe.request.storeys.connectorKind = connectorKind;
  recipe.request.facade.includeEntrance = true;
  recipe.request.facade.includeExteriorWindows = false;

  BuildingFixture fixture;
  fixture.layout.stableKey = "building_traversal";
  const cr::CreativeWorldLayoutBuildingEditResult materialized =
      cr::materializeCreativeWorldLayoutBuildingBlockout(
          fixture.layout, recipe, 1U);
  if (!materialized.accepted) {
    std::cerr << "fixture materialization failed: "
              << materialized.reasonCode << '\n';
    return fixture;
  }
  fixture.layout = materialized.edited;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Building Traversal Fixture");
  static_cast<void>(document.assignId(9961U));
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, fixture.layout);
  const cr::CreativeWorldLayoutPreviewResult preview =
      compiled.receipt.accepted
          ? cr::previewCreativeWorldLayoutPlan(document, compiled.plan)
          : cr::CreativeWorldLayoutPreviewResult{};
  if (!preview.accepted) {
    return fixture;
  }
  fixture.generated = preview.document;
  fixture.ready = true;
  return fixture;
}

bool hasIssue(const cr::CreativeWorldLayoutBuildingTraversalReceipt& receipt,
              cr::CreativeWorldLayoutBuildingTraversalIssueKind kind,
              cr::CreativeWorldLayoutTable table,
              std::size_t index) {
  return std::any_of(
      receipt.issues.begin(), receipt.issues.begin() + receipt.issueCount,
      [kind, table, index](const auto& issue) {
        return issue.kind == kind && issue.table == table &&
               issue.index == index;
      });
}

const cr::CreativeObject* firstStructuralWall(
    const BuildingFixture& fixture) {
  for (const cr::CreativeObject& object : fixture.generated.objects()) {
    if (object.kind != cr::CreativeObjectKind::Wall) {
      continue;
    }
    const cr::CreativeWorldLayoutObjectProvenance provenance =
        cr::resolveCreativeWorldLayoutObjectProvenance(fixture.layout,
                                                       object);
    if (provenance.owned) {
      return &object;
    }
  }
  return nullptr;
}

bool addStructuralBlocker(BuildingFixture& fixture,
                          cr::CreativeVec3 position,
                          cr::CreativeBounds localBounds,
                          std::string name) {
  const cr::CreativeObject* source = firstStructuralWall(fixture);
  if (source == nullptr) {
    return false;
  }
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Wall;
  request.name = std::move(name);
  request.bounds = {
      {localBounds.min.x + position.x, localBounds.min.y + position.y,
       localBounds.min.z + position.z},
      {localBounds.max.x + position.x, localBounds.max.y + position.y,
       localBounds.max.z + position.z}};
  request.hasBoundsOverride = true;
  request.tags = source->tags;
  return fixture.generated.createObject(request).accepted;
}

bool validGeneratedBuildingPassesRuntimeTraversal() {
  const BuildingFixture fixture = twoStoreyFixture();
  const cr::CreativeWorldLayoutBuildingTraversalReceipt receipt =
      cr::validateCreativeWorldLayoutBuildingTraversal(
          {fixture.ready ? &fixture.layout : nullptr,
           fixture.ready ? &fixture.generated : nullptr,
           {}});
  if (!receipt.traversable) {
    std::cerr << "traversal receipt: accepted=" << receipt.accepted
              << " status=" << static_cast<int>(receipt.status)
              << " rooms=" << receipt.roomFloorContactCount << '/'
              << receipt.roomStandingClearanceCount << '/'
              << receipt.roomCount << " passages="
              << receipt.traversablePassageCount << '/'
              << receipt.passageCount << " connectors="
              << receipt.traversableConnectorCount << '/'
              << receipt.connectorCount << " issues=" << receipt.issueCount
              << '\n';
    for (std::size_t index = 0U; index < receipt.issueCount; ++index) {
      std::cerr << "  issue " << index << " kind="
                << static_cast<int>(receipt.issues[index].kind)
                << " table=" << static_cast<int>(receipt.issues[index].table)
                << " index=" << receipt.issues[index].index << '\n';
    }
  }

  return expect(fixture.ready, "two-storey traversal fixture compiles") &&
         expect(receipt.accepted && receipt.traversable &&
                    receipt.status ==
                        cr::CreativeWorldLayoutBuildingTraversalStatus::Ready &&
                    receipt.roomCount == 2U &&
                    receipt.roomFloorContactCount == 2U &&
                    receipt.roomStandingClearanceCount == 2U &&
                    receipt.passageCount == 1U &&
                    receipt.traversablePassageCount == 1U &&
                    receipt.connectorCount == 1U &&
                    receipt.traversableConnectorCount == 1U &&
                    receipt.issueCount == 0U,
                "runtime body occupies rooms and crosses doors and stairs");
}

bool validGeneratedRampPassesRuntimeTraversal() {
  const BuildingFixture fixture = twoStoreyFixture(
      cr::CreativeWorldLayoutVerticalConnectorKind::Ramp);
  const cr::CreativeWorldLayoutBuildingTraversalReceipt receipt =
      cr::validateCreativeWorldLayoutBuildingTraversal(
          {fixture.ready ? &fixture.layout : nullptr,
           fixture.ready ? &fixture.generated : nullptr,
           {}});

  return expect(fixture.ready, "two-storey ramp fixture compiles") &&
         expect(receipt.accepted && receipt.traversable &&
                    receipt.connectorCount == 1U &&
                    receipt.traversableConnectorCount == 1U,
                "runtime motor traverses the generated ramp both ways");
}

bool removingGeneratedFloorReportsExactRoom() {
  BuildingFixture fixture = twoStoreyFixture();
  cr::CreativeObjectId floorId = cr::kInvalidObjectId;
  for (const cr::CreativeObject& object : fixture.generated.objects()) {
    const cr::CreativeWorldLayoutObjectProvenance provenance =
        cr::resolveCreativeWorldLayoutObjectProvenance(fixture.layout,
                                                       object);
    if (object.kind == cr::CreativeObjectKind::Floor && provenance.owned &&
        provenance.table == cr::CreativeWorldLayoutTable::Room &&
        provenance.index == 0U) {
      floorId = object.id;
      break;
    }
  }
  const cr::CreativeDocumentRemoveReceipt removed =
      fixture.generated.removeDocumentObject(
          cr::CreativeDocumentRemoveRequest{floorId});
  const cr::CreativeWorldLayoutBuildingTraversalReceipt receipt =
      cr::validateCreativeWorldLayoutBuildingTraversal(
          {&fixture.layout, &fixture.generated, {}});

  return expect(fixture.ready && removed.accepted,
                "generated lower room floor can be removed for sabotage") &&
         expect(receipt.accepted && !receipt.traversable &&
                    hasIssue(
                        receipt,
                        cr::CreativeWorldLayoutBuildingTraversalIssueKind::
                            MissingRoomFloorContact,
                        cr::CreativeWorldLayoutTable::Room, 0U),
                "missing physical floor contact identifies its room source");
}

bool lowStructuralCeilingBlocksStandingClearance() {
  BuildingFixture fixture = twoStoreyFixture();
  const bool blockerAdded = addStructuralBlocker(
      fixture, {6.0, 0.0, 5.0}, {{-6.0, 1.60, -5.0}, {6.0, 1.70, 5.0}},
      "Low Structural Ceiling");
  const cr::CreativeWorldLayoutBuildingTraversalReceipt receipt =
      cr::validateCreativeWorldLayoutBuildingTraversal(
          {&fixture.layout, &fixture.generated, {}});

  return expect(fixture.ready && blockerAdded,
                "low structural ceiling sabotage is generated") &&
         expect(receipt.roomFloorContactCount == receipt.roomCount,
                "low ceiling does not erase floor contact") &&
         expect(hasIssue(
                    receipt,
                    cr::CreativeWorldLayoutBuildingTraversalIssueKind::
                        RoomStandingClearanceBlocked,
                    cr::CreativeWorldLayoutTable::Room, 0U),
                "player-height obstruction identifies lower room clearance");
}

bool structuralBlockerBehindOpenDoorFailsPassage() {
  BuildingFixture fixture = twoStoreyFixture();
  if (!fixture.ready || fixture.layout.openings.empty()) {
    return expect(false, "door passage fixture is complete");
  }
  const cr::CreativeWorldLayoutOpening& opening = fixture.layout.openings[0];
  const cr::CreativeWorldLayoutOpeningHostFrame host =
      cr::resolveCreativeWorldLayoutOpeningHost(fixture.layout, opening);
  const cr::CreativeWorldLayoutOpeningHostPoint point =
      cr::creativeWorldLayoutOpeningHostPoint(host, opening.centerOffsetCells);
  const cr::CreativeGridSettings grid = fixture.generated.gridSettings();
  const bool blockerAdded = addStructuralBlocker(
      fixture,
      {grid.origin.x + point.x * grid.cellSizeMeters,
       grid.origin.y + host.baseLayer * grid.cellSizeMeters,
       grid.origin.z + point.z * grid.cellSizeMeters},
      {{-0.75, 0.0, -0.75}, {0.75, 2.2, 0.75}},
      "Blocked Door Passage");
  const cr::CreativeWorldLayoutBuildingTraversalReceipt receipt =
      cr::validateCreativeWorldLayoutBuildingTraversal(
          {&fixture.layout, &fixture.generated, {}});

  return expect(host.accepted && blockerAdded,
                "door blocker uses exact authored host point") &&
         expect(hasIssue(
                    receipt,
                    cr::CreativeWorldLayoutBuildingTraversalIssueKind::
                        OpeningPassageBlocked,
                    cr::CreativeWorldLayoutTable::Opening, 0U),
                "open-door motor sweep catches generated collision blockage");
}

bool structuralBlockerOnStairFailsBidirectionalTraversal() {
  BuildingFixture fixture = twoStoreyFixture();
  if (!fixture.ready || fixture.layout.verticalConnectors.empty()) {
    return expect(false, "connector traversal fixture is complete");
  }
  const cr::CreativeWorldLayoutVerticalConnectorPlan connector =
      cr::planCreativeWorldLayoutVerticalConnector(
          fixture.generated.gridSettings(), fixture.layout, 0U);
  const cr::CreativeVec3 lower = connector.stair.lowerLanding.centerMeters;
  const cr::CreativeVec3 upper = connector.stair.upperLanding.centerMeters;
  const bool blockerAdded = addStructuralBlocker(
      fixture,
      {(lower.x + upper.x) * 0.5, (lower.y + upper.y) * 0.5,
       (lower.z + upper.z) * 0.5},
      {{-0.70, -0.90, -0.70}, {0.70, 0.90, 0.70}},
      "Blocked Stair Passage");
  const cr::CreativeWorldLayoutBuildingTraversalReceipt receipt =
      cr::validateCreativeWorldLayoutBuildingTraversal(
          {&fixture.layout, &fixture.generated, {}});

  return expect(connector.accepted && blockerAdded,
                "stair blocker uses exact connector path") &&
         expect(hasIssue(
                    receipt,
                    cr::CreativeWorldLayoutBuildingTraversalIssueKind::
                        ConnectorTraversalBlocked,
                    cr::CreativeWorldLayoutTable::VerticalConnector, 0U),
                "runtime motor rejects a physically blocked stair");
}

bool motorStepBoundFailsExplicitly() {
  const BuildingFixture fixture = twoStoreyFixture();
  cr::CreativeWorldLayoutBuildingTraversalConfig config;
  config.maximumMotorStepsPerPath = 1U;
  const cr::CreativeWorldLayoutBuildingTraversalReceipt receipt =
      cr::validateCreativeWorldLayoutBuildingTraversal(
          {&fixture.layout, &fixture.generated, config});

  return expect(hasIssue(
                    receipt,
                    cr::CreativeWorldLayoutBuildingTraversalIssueKind::
                        TraversalCapacityExceeded,
                    cr::CreativeWorldLayoutTable::VerticalConnector, 0U),
                "bounded motor proof reports capacity instead of guessing");
}

bool defaultEnvelopeMatchesRuntimePlayer() {
  const cr::CreativeWorldLayoutBuildingTraversalConfig config;
  const iggy3d::MovementParams movement;
  return expect(std::fabs(config.actorRadiusMeters - movement.radiusMeters) <=
                        1.0e-6 &&
                    std::fabs(config.actorHeightMeters - movement.heightMeters) <=
                        1.0e-6 &&
                    std::fabs(config.maximumStepMeters -
                              movement.stepHeightMeters) <= 1.0e-6 &&
                    std::fabs(config.groundSnapMeters -
                              movement.groundSnapMeters) <= 1.0e-6 &&
                    std::fabs(config.skinMeters - movement.skinMeters) <=
                        1.0e-6,
                "traversal defaults stay pinned to the runtime player body");
}

bool invalidRequestsFailClosed() {
  const BuildingFixture fixture = twoStoreyFixture();
  cr::CreativeWorldLayoutBuildingTraversalConfig invalid;
  invalid.actorHeightMeters = -1.0;
  const auto missing =
      cr::validateCreativeWorldLayoutBuildingTraversal({});
  const auto malformed = cr::validateCreativeWorldLayoutBuildingTraversal(
      {&fixture.layout, &fixture.generated, invalid});
  return expect(!missing.accepted && !missing.traversable &&
                    missing.status ==
                        cr::CreativeWorldLayoutBuildingTraversalStatus::
                            InvalidRequest,
                "missing traversal request fails closed") &&
         expect(!malformed.accepted && !malformed.traversable &&
                    malformed.status ==
                        cr::CreativeWorldLayoutBuildingTraversalStatus::
                            InvalidRequest,
                "invalid player envelope fails closed");
}

}  // namespace

int main() {
  return validGeneratedBuildingPassesRuntimeTraversal() &&
                 validGeneratedRampPassesRuntimeTraversal() &&
                 removingGeneratedFloorReportsExactRoom() &&
                 lowStructuralCeilingBlocksStandingClearance() &&
                 structuralBlockerBehindOpenDoorFailsPassage() &&
                 structuralBlockerOnStairFailsBidirectionalTraversal() &&
                 motorStepBoundFailsExplicitly() &&
                 defaultEnvelopeMatchesRuntimePlayer() &&
                 invalidRequestsFailClosed()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
