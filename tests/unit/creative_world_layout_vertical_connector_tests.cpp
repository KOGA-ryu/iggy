#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
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

cr::CreativeWorldLayout twoStoreyLayout() {
  cr::CreativeWorldLayout layout;
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "house";
  building.name = "House";
  building.rootMode = cr::CreativeBuildingRootMode::None;
  layout.buildings.push_back(building);
  layout.levels.push_back({0U, "ground", "Ground", 0.0, 3U, 1U, 1U, 1U});
  layout.levels.push_back({0U, "upper", "Upper", 3.0, 3U, 1U, 1U, 1U});
  layout.rooms.push_back(
      {0U, 0U, "ground_room", "Ground Room", {{0, 0}, {8, 8}}, 0.25});
  layout.rooms.push_back(
      {0U, 1U, "upper_room", "Upper Room", {{0, 0}, {8, 8}}, 0.25});
  layout.verticalConnectors.push_back(
      {0U,
       0U,
       1U,
       cr::CreativeWorldLayoutVerticalConnectorKind::Stair,
       cr::CreativeWorldLayoutVerticalDirection::PositiveX,
       "main_stair",
       "Main Stair",
       {{1, 2}, {5, 4}}});
  return layout;
}

bool stairPlanOwnsRiseDirectionAndStepParity() {
  const cr::CreativeGridSettings grid{{10.0, 1.0, -5.0}, 1.0, {32, 16, 32}};
  const cr::CreativeWorldLayout layout = twoStoreyLayout();
  const auto plan =
      cr::planCreativeWorldLayoutVerticalConnector(grid, layout, 0U);
  return expect(plan.accepted &&
                    plan.objectKind == cr::CreativeObjectKind::Stair,
                "valid two-storey stair is accepted") &&
         expect(plan.riseMeters == 3.0 && plan.runMeters == 4.0 &&
                    plan.widthMeters == 2.0 && plan.stepCount == 12U,
                "stair dimensions and descriptor-owned step count match") &&
         expect(
             std::abs(plan.rotationEulerRadians.y - std::numbers::pi * 0.5) <
                 1.0e-12,
             "positive X stair rotates local positive Z toward positive X") &&
         expect(plan.authoredBounds.min.x == 12.0 &&
                    plan.authoredBounds.max.x == 14.0 &&
                    plan.authoredBounds.min.z == -4.0 &&
                    plan.authoredBounds.max.z == 0.0 &&
                    plan.authoredBounds.min.y == 1.0 &&
                    plan.authoredBounds.max.y == 4.0,
                "authored local bounds preserve world center and swapped axes");
}

bool invalidStoriesFootprintsAndLandingsFailClosed() {
  const cr::CreativeGridSettings grid{{}, 1.0, {32, 16, 32}};
  cr::CreativeWorldLayout wrongRise = twoStoreyLayout();
  wrongRise.levels[1].floorTopLayer = 2.5;
  cr::CreativeWorldLayout outside = twoStoreyLayout();
  outside.verticalConnectors[0].footprint = {{-1, 2}, {5, 4}};
  cr::CreativeWorldLayout noLanding = twoStoreyLayout();
  noLanding.verticalConnectors[0].footprint = {{0, 2}, {4, 4}};
  cr::CreativeWorldLayout steep = twoStoreyLayout();
  steep.verticalConnectors[0].footprint = {{1, 2}, {3, 4}};

  return expect(
             cr::planCreativeWorldLayoutVerticalConnector(grid, wrongRise, 0U)
                     .status ==
                 cr::CreativeWorldLayoutVerticalConnectorStatus::InvalidLevels,
             "non-adjacent story elevations reject") &&
         expect(cr::planCreativeWorldLayoutVerticalConnector(grid, outside, 0U)
                        .status ==
                    cr::CreativeWorldLayoutVerticalConnectorStatus::
                        InvalidFootprint,
                "opening outside either room rejects") &&
         expect(
             cr::planCreativeWorldLayoutVerticalConnector(grid, noLanding, 0U)
                     .status ==
                 cr::CreativeWorldLayoutVerticalConnectorStatus::InvalidLanding,
             "missing low landing rejects") &&
         expect(
             cr::planCreativeWorldLayoutVerticalConnector(grid, steep, 0U)
                     .status ==
                 cr::CreativeWorldLayoutVerticalConnectorStatus::InvalidSlope,
             "run shorter than rise rejects");
}

bool oneConnectorOwnsEachAffectedSlab() {
  const cr::CreativeGridSettings grid{{}, 1.0, {32, 16, 32}};
  cr::CreativeWorldLayout layout = twoStoreyLayout();
  layout.verticalConnectors.push_back(layout.verticalConnectors.front());
  layout.verticalConnectors.back().stableKey = "second_stair";
  layout.verticalConnectors.back().footprint = {{2, 5}, {6, 7}};
  return expect(
      cr::planCreativeWorldLayoutVerticalConnector(grid, layout, 0U).status ==
          cr::CreativeWorldLayoutVerticalConnectorStatus::SurfaceAlreadyCut,
      "a room surface cannot publish two competing cutout owners");
}

bool compilerCutsBothSlabsAndEmitsOneStair() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Vertical Connector Compile");
  static_cast<void>(document.assignId(71U));
  static_cast<void>(document.setGridSettings({{}, 1.0, {32, 16, 32}}));
  const cr::CreativeWorldLayout layout = twoStoreyLayout();
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  if (!compiled.receipt.accepted || compiled.plan.objectRecipes.size() != 1U) {
    return expect(false, "connector layout compiles as one building recipe");
  }

  const auto& objects = compiled.plan.objectRecipes.front().objects;
  const auto stair = std::find_if(
      objects.begin(), objects.end(),
      [](const cr::CreativeRecipeObjectPlan& value) {
        return value.createRequest.kind == cr::CreativeObjectKind::Stair;
      });
  const std::size_t lowerCeilingParts = static_cast<std::size_t>(std::count_if(
      objects.begin(), objects.end(),
      [](const cr::CreativeRecipeObjectPlan& value) {
        return value.stableKey.find("ground_room.ceiling.part.") !=
               std::string::npos;
      }));
  const std::size_t upperFloorParts = static_cast<std::size_t>(
      std::count_if(objects.begin(), objects.end(),
                    [](const cr::CreativeRecipeObjectPlan& value) {
                      return value.stableKey.find("upper_room.floor.part.") !=
                             std::string::npos;
                    }));

  return expect(stair != objects.end(), "compiler emits the semantic stair") &&
         expect(stair->stableKey == "house.main_stair" &&
                    stair->createRequest.hasTransformOverride &&
                    std::abs(
                        stair->createRequest.transform.rotationEulerRadians.y -
                        std::numbers::pi * 0.5) < 1.0e-12,
                "compiled stair keeps stable identity and direction") &&
         expect(lowerCeilingParts == 4U && upperFloorParts == 4U,
                "one opening partitions both affected structural slabs") &&
         expect(std::none_of(objects.begin(), objects.end(),
                             [](const cr::CreativeRecipeObjectPlan& value) {
                               return value.stableKey ==
                                          "house.ground_room.ceiling" ||
                                      value.stableKey ==
                                          "house.upper_room.floor";
                             }),
                "uncut full slabs are not emitted beneath cutout pieces");
}

} // namespace

int main() {
  return stairPlanOwnsRiseDirectionAndStepParity() &&
                 invalidStoriesFootprintsAndLandingsFailClosed() &&
                 oneConnectorOwnsEachAffectedSlab() &&
                 compilerCutsBothSlabsAndEmitsOneStair()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
