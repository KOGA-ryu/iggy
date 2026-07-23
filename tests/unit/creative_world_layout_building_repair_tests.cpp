#include "app/iggy3d/creative/world/WorldLayoutBlockoutMaterialization.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingRepairs.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOpenings.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeWorldLayoutBuildingBlockoutRecipe blockoutRecipe(
    cr::CreativeWorldLayoutBuildingBlockoutPattern pattern,
    std::uint16_t storeys = 1U) {
  cr::CreativeWorldLayoutBuildingBlockoutRecipe recipe;
  recipe.request.footprint = {{0, 0}, {12, 10}};
  recipe.request.pattern = pattern;
  recipe.request.wallThicknessCells = 0.25;
  recipe.request.floorToFloorCells = 3U;
  recipe.request.storeys.count = storeys;
  recipe.request.storeys.connectStoreys = storeys > 1U;
  recipe.request.facade.includeEntrance = true;
  recipe.request.facade.includeExteriorWindows = false;
  return recipe;
}

cr::CreativeWorldLayout materialize(
    const cr::CreativeWorldLayoutBuildingBlockoutRecipe& recipe,
    std::uint64_t nextOrdinal = 1U) {
  cr::CreativeWorldLayout source;
  source.stableKey = "building_repair";
  const cr::CreativeWorldLayoutBuildingEditResult result =
      cr::materializeCreativeWorldLayoutBuildingBlockout(source, recipe,
                                                         nextOrdinal);
  if (!result.accepted) {
    std::cerr << "FAIL: building repair fixture materialized\n";
    return {};
  }
  return result.edited;
}

const cr::CreativeWorldLayoutBuildingUsabilityIssue* findIssue(
    const cr::CreativeWorldLayoutBuildingUsabilityReceipt& receipt,
    cr::CreativeWorldLayoutBuildingUsabilityIssueKind kind,
    cr::CreativeWorldLayoutTable table,
    std::size_t index = cr::kInvalidCreativeWorldLayoutIndex) {
  const auto found = std::find_if(
      receipt.issues.begin(), receipt.issues.begin() + receipt.issueCount,
      [kind, table, index](const auto& issue) {
        return issue.kind == kind && issue.table == table &&
               (index == cr::kInvalidCreativeWorldLayoutIndex ||
                issue.index == index);
      });
  return found == receipt.issues.begin() + receipt.issueCount ? nullptr
                                                              : &*found;
}

cr::CreativeWorldLayoutBuildingUsabilityReceipt validate(
    const cr::CreativeWorldLayout& layout) {
  return cr::validateCreativeWorldLayoutBuildingUsability(
      {&layout, nullptr, {}});
}

bool missingEntranceGetsOneValidatedExteriorDoor() {
  cr::CreativeWorldLayoutBuildingBlockoutRecipe recipe = blockoutRecipe(
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom);
  recipe.request.facade.includeEntrance = false;
  const cr::CreativeWorldLayout source = materialize(recipe);
  const cr::CreativeWorldLayoutBuildingUsabilityReceipt before =
      validate(source);
  const auto* issue = findIssue(
      before,
      cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
          MissingExteriorEntrance,
      cr::CreativeWorldLayoutTable::Building, 0U);
  if (!expect(issue != nullptr, "missing entrance fixture has exact issue")) {
    return false;
  }
  const cr::CreativeWorldLayoutBuildingRepairResult repaired =
      cr::planCreativeWorldLayoutBuildingRepair({&source, *issue, {}, 91U});
  const cr::CreativeWorldLayoutBuildingUsabilityReceipt after =
      validate(repaired.edited);
  const cr::CreativeWorldLayoutOpening* opening =
      repaired.resultIndex < repaired.edited.openings.size()
          ? &repaired.edited.openings[repaired.resultIndex]
          : nullptr;
  const cr::CreativeWorldLayoutOpeningHostFrame host =
      opening != nullptr
          ? cr::resolveCreativeWorldLayoutOpeningHost(repaired.edited,
                                                      *opening)
          : cr::CreativeWorldLayoutOpeningHostFrame{};

  return expect(
             repaired.accepted && repaired.changed &&
                 repaired.operation ==
                     cr::CreativeWorldLayoutBuildingRepairOperation::
                         AddExteriorEntrance &&
                 repaired.edited.openings.size() ==
                     source.openings.size() + 1U &&
                 repaired.nextStableOrdinal > 91U,
             "missing entrance repair adds one owned door") &&
         expect(opening != nullptr && host.accepted && host.exterior &&
                    opening->kind == cr::CreativeBuildingOpeningKind::Door &&
                    cr::evaluateCreativeWorldLayoutOpeningClearance(
                        {opening})
                        .traversable,
                "new entrance is exterior and matches the player envelope") &&
         expect(findIssue(
                    after,
                    cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
                        MissingExteriorEntrance,
                    cr::CreativeWorldLayoutTable::Building, 0U) == nullptr,
                "repair removes the exact entrance issue");
}

bool disconnectedRoomGetsOneTopologyDoor() {
  cr::CreativeWorldLayoutBuildingBlockoutRecipe recipe = blockoutRecipe(
      cr::CreativeWorldLayoutBuildingBlockoutPattern::Grid2x2);
  recipe.request.connectRooms = false;
  const cr::CreativeWorldLayout source = materialize(recipe);
  const cr::CreativeWorldLayoutBuildingUsabilityReceipt before =
      validate(source);
  const auto* issue = findIssue(
      before,
      cr::CreativeWorldLayoutBuildingUsabilityIssueKind::DisconnectedRoom,
      cr::CreativeWorldLayoutTable::Room);
  if (!expect(issue != nullptr,
              "disconnected room fixture has an exact room issue")) {
    return false;
  }
  const cr::CreativeWorldLayoutBuildingRepairResult repaired =
      cr::planCreativeWorldLayoutBuildingRepair({&source, *issue, {}, 121U});
  const cr::CreativeWorldLayoutBuildingUsabilityReceipt after =
      validate(repaired.edited);
  const cr::CreativeWorldLayoutOpening* opening =
      repaired.resultIndex < repaired.edited.openings.size()
          ? &repaired.edited.openings[repaired.resultIndex]
          : nullptr;
  const cr::CreativeWorldLayoutOpeningHostFrame host =
      opening != nullptr
          ? cr::resolveCreativeWorldLayoutOpeningHost(repaired.edited,
                                                      *opening)
          : cr::CreativeWorldLayoutOpeningHostFrame{};

  return expect(
             repaired.accepted && repaired.operation ==
                                      cr::CreativeWorldLayoutBuildingRepairOperation::
                                          ConnectRoom &&
                 repaired.edited.openings.size() ==
                     source.openings.size() + 1U,
             "disconnected room repair adds one shared-edge door") &&
         expect(opening != nullptr && host.accepted && !host.exterior &&
                    host.owningRoomCount == 2U,
                "repair lands on one shared room boundary") &&
         expect(findIssue(
                    after,
                    cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
                        DisconnectedRoom,
                    cr::CreativeWorldLayoutTable::Room, issue->index) ==
                    nullptr,
                "repair connects the selected room to the entrance graph");
}

bool undersizedDoorExpandsWithoutChangingIdentity() {
  cr::CreativeWorldLayout source = materialize(blockoutRecipe(
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom));
  if (source.openings.empty()) {
    return expect(false, "opening repair fixture has an entrance");
  }
  source.openings[0].widthCells = 0.25;
  source.openings[0].cutoutBottomCells = 0.5;
  source.openings[0].cutoutHeightCells = 1.0;
  source.openings[0].includeInsert = false;
  const std::string stableKey = source.openings[0].stableKey;
  const cr::CreativeWorldLayoutBuildingUsabilityReceipt before =
      validate(source);
  const auto* issue = findIssue(
      before,
      cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
          OpeningClearanceTooSmall,
      cr::CreativeWorldLayoutTable::Opening, 0U);
  if (!expect(issue != nullptr, "undersized opening has clearance issue")) {
    return false;
  }
  const cr::CreativeWorldLayoutBuildingRepairResult repaired =
      cr::planCreativeWorldLayoutBuildingRepair({&source, *issue, {}, 151U});
  const cr::CreativeWorldLayoutOpening& opening = repaired.edited.openings[0];

  return expect(
             repaired.accepted && repaired.operation ==
                                      cr::CreativeWorldLayoutBuildingRepairOperation::
                                          ExpandOpeningClearance &&
                 repaired.nextStableOrdinal == 151U,
             "clearance repair edits the existing door in place") &&
         expect(opening.stableKey == stableKey &&
                    !opening.includeInsert &&
                    opening.cutoutBottomCells == 0.0 &&
                    cr::evaluateCreativeWorldLayoutOpeningClearance(
                        {&opening})
                        .traversable,
                "clearance repair preserves identity and fits the player");
}

bool ambiguousAndImpossibleRepairsFailWithoutMutation() {
  cr::CreativeWorldLayoutBuildingBlockoutRecipe twoStoreys = blockoutRecipe(
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom, 2U);
  twoStoreys.request.storeys.connectStoreys = false;
  const cr::CreativeWorldLayout source = materialize(twoStoreys);
  const cr::CreativeWorldLayoutBuildingUsabilityReceipt sourceIssues =
      validate(source);
  const auto* vertical = findIssue(
      sourceIssues,
      cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
          MissingVerticalConnection,
      cr::CreativeWorldLayoutTable::Level);

  cr::CreativeWorldLayout tiny;
  tiny.stableKey = "tiny";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "tiny_building";
  building.name = "Tiny";
  tiny.buildings.push_back(building);
  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = 0U;
  level.stableKey = "tiny_level";
  level.name = "Ground";
  tiny.levels.push_back(level);
  cr::CreativeWorldLayoutRoom room;
  room.buildingIndex = 0U;
  room.levelIndex = 0U;
  room.stableKey = "tiny_room";
  room.name = "Tiny Room";
  room.footprint = {{0, 0}, {1, 1}};
  tiny.rooms.push_back(room);
  const auto tinyIssues = validate(tiny);
  const auto* entrance = findIssue(
      tinyIssues,
      cr::CreativeWorldLayoutBuildingUsabilityIssueKind::
          MissingExteriorEntrance,
      cr::CreativeWorldLayoutTable::Building, 0U);
  if (!expect(vertical != nullptr && entrance != nullptr,
              "unsupported and impossible fixtures expose exact issues")) {
    return false;
  }
  const auto unsupported = cr::planCreativeWorldLayoutBuildingRepair(
      {&source, *vertical, {}, 181U});
  const auto impossible = cr::planCreativeWorldLayoutBuildingRepair(
      {&tiny, *entrance, {}, 191U});

  return expect(
             !unsupported.accepted && !unsupported.changed &&
                 unsupported.status ==
                     cr::CreativeWorldLayoutBuildingRepairStatus::
                         UnsupportedIssue &&
                 unsupported.edited.buildings.empty(),
             "ambiguous stair repair remains creator-owned") &&
         expect(!impossible.accepted && !impossible.changed &&
                    impossible.status ==
                        cr::CreativeWorldLayoutBuildingRepairStatus::
                            NoValidCandidate &&
                    impossible.edited.buildings.empty(),
                "too-small shell cannot receive a fabricated entrance");
}

bool staleIssueIsRejected() {
  cr::CreativeWorldLayout source = materialize(blockoutRecipe(
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom));
  cr::CreativeWorldLayoutBuildingUsabilityIssue stale;
  stale.kind =
      cr::CreativeWorldLayoutBuildingUsabilityIssueKind::DisconnectedRoom;
  stale.table = cr::CreativeWorldLayoutTable::Room;
  stale.index = 0U;
  stale.buildingIndex = 0U;
  const auto result = cr::planCreativeWorldLayoutBuildingRepair(
      {&source, stale, {}, 201U});
  return expect(!result.accepted &&
                    result.status ==
                        cr::CreativeWorldLayoutBuildingRepairStatus::
                            InvalidRequest,
                "resolved or mismatched diagnostics cannot mutate source");
}

}  // namespace

int main() {
  const bool ok = missingEntranceGetsOneValidatedExteriorDoor() &&
                  disconnectedRoomGetsOneTopologyDoor() &&
                  undersizedDoorExpandsWithoutChangingIdentity() &&
                  ambiguousAndImpossibleRepairsFailWithoutMutation() &&
                  staleIssueIsRejected();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
