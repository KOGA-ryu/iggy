#include "app/iggy3d/creative/SpatialProjection.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeObject makeAuthoringPointObject() {
  cr::CreativeObject object;
  object.id = 7;
  object.kind = cr::CreativeObjectKind::Note;
  object.name = "Authoring note";
  object.transform.position = {2.0, 3.0, 0.0};
  return object;
}

cr::CreativeObject makeRoomObject(cr::CreativeObjectId objectId) {
  cr::CreativeObject object;
  object.id = objectId;
  object.kind = cr::CreativeObjectKind::Room;
  object.name = "Room";
  object.bounds.min = {1.0, 2.0, 0.0};
  object.bounds.max = {2.0, 3.0, 1.0};
  return object;
}

cr::CreativeSpatialProjectionRequest makeRequest() {
  cr::CreativeSpatialProjectionRequest request;
  request.gridSize = {8, 8, 2};
  return request;
}

bool defaultRequestExcludesAuthoringOnlyObjects() {
  const cr::CreativeObject object = makeAuthoringPointObject();
  const cr::CreativeSpatialProjectionRequest request = makeRequest();
  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectToGrid(object, request);

  return expect(!request.includeAuthoringOnly,
                "default request excludes authoring") &&
         expect(receipt.status == cr::CreativeSpatialProjectionStatus::NoProjection,
                "authoring default no projection") &&
         expect(receipt.profile ==
                    cr::CreativeSpatialProjectionProfile::PointProjection,
                "authoring default keeps point profile") &&
         expect(receipt.occupancyKind ==
                    cr::CreativeSpatialOccupancyKind::Authoring,
                "authoring default occupancy") &&
         expect(receipt.cells.empty(), "authoring default has no cells") &&
         expect(receipt.message == "authoring_excluded",
                "authoring default message");
}

bool explicitRequestIncludesAuthoringOnlyObjects() {
  const cr::CreativeObject object = makeAuthoringPointObject();
  cr::CreativeSpatialProjectionRequest request = makeRequest();
  request.includeAuthoringOnly = true;

  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectToGrid(object, request);

  return expect(receipt.status == cr::CreativeSpatialProjectionStatus::Projected,
                "authoring included projected") &&
         expect(receipt.cells.size() == 1U, "authoring included one cell") &&
         expect(receipt.cells[0].objectId == object.id,
                "authoring included object id") &&
         expect(receipt.cells[0].objectKind == object.kind,
                "authoring included object kind") &&
         expect(receipt.cells[0].occupancyKind ==
                    cr::CreativeSpatialOccupancyKind::Authoring,
                "authoring included occupancy") &&
         expect(receipt.cells[0].coord.x == 2, "authoring included x") &&
         expect(receipt.cells[0].coord.y == 3, "authoring included y") &&
         expect(receipt.cells[0].coord.z == 0, "authoring included z") &&
         expect(receipt.cells[0].index == 26U, "authoring included index");
}

bool visibleRoomStillProjects() {
  const cr::CreativeObject object = makeRoomObject(11);
  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectToGrid(object, makeRequest());

  return expect(receipt.status == cr::CreativeSpatialProjectionStatus::Projected,
                "visible room projected") &&
         expect(receipt.message == "projected", "visible room message") &&
         expect(receipt.objectId == object.id, "visible room object id") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Room,
                "visible room kind") &&
         expect(receipt.profile ==
                    cr::CreativeSpatialProjectionProfile::BoxProjection,
                "visible room profile") &&
         expect(receipt.occupancyKind ==
                    cr::CreativeSpatialOccupancyKind::Structural,
                "visible room occupancy") &&
         expect(receipt.cells.size() == 1U, "visible room one cell") &&
         expect(receipt.cells[0].objectId == object.id,
                "visible room cell object id") &&
         expect(receipt.cells[0].coord.x == 1, "visible room x") &&
         expect(receipt.cells[0].coord.y == 2, "visible room y") &&
         expect(receipt.cells[0].coord.z == 0, "visible room z") &&
         expect(receipt.cells[0].index == 17U, "visible room index");
}

bool hiddenRoomDoesNotProject() {
  cr::CreativeObject object = makeRoomObject(12);
  object.visible = false;

  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectToGrid(object, makeRequest());

  return expect(receipt.status ==
                    cr::CreativeSpatialProjectionStatus::NoProjection,
                "hidden room no projection") &&
         expect(receipt.message == "object_hidden", "hidden room message") &&
         expect(receipt.objectId == object.id, "hidden room object id") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Room,
                "hidden room kind") &&
         expect(receipt.profile ==
                    cr::CreativeSpatialProjectionProfile::BoxProjection,
                "hidden room profile") &&
         expect(receipt.occupancyKind ==
                    cr::CreativeSpatialOccupancyKind::Structural,
                "hidden room occupancy") &&
         expect(receipt.cells.empty(), "hidden room no cells");
}

bool aggregateVisibleAndHiddenRoomsProjectsOnlyVisibleCells() {
  cr::CreativeObject visible = makeRoomObject(21);
  cr::CreativeObject hidden = makeRoomObject(22);
  hidden.visible = false;
  hidden.bounds.min = {3.0, 4.0, 0.0};
  hidden.bounds.max = {4.0, 5.0, 1.0};
  const std::vector<cr::CreativeObject> objects{visible, hidden};

  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectsToGrid(objects, makeRequest());

  bool hiddenObjectAppears = false;
  for (const cr::CreativeSpatialCell& cell : receipt.cells) {
    hiddenObjectAppears = hiddenObjectAppears || cell.objectId == hidden.id;
  }

  return expect(receipt.status == cr::CreativeSpatialProjectionStatus::Projected,
                "mixed aggregate projected") &&
         expect(receipt.cells.size() == 1U, "mixed aggregate visible cell only") &&
         expect(receipt.cells[0].objectId == visible.id,
                "mixed aggregate visible object id") &&
         expect(!hiddenObjectAppears, "mixed aggregate hidden object absent");
}

bool aggregateAllHiddenProjectableObjectsDoesNotProject() {
  cr::CreativeObject hidden = makeRoomObject(31);
  hidden.visible = false;
  const std::vector<cr::CreativeObject> objects{hidden};

  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectsToGrid(objects, makeRequest());

  return expect(receipt.status != cr::CreativeSpatialProjectionStatus::Projected,
                "all hidden aggregate not projected") &&
         expect(receipt.status ==
                    cr::CreativeSpatialProjectionStatus::NoProjection,
                "all hidden aggregate no projection") &&
         expect(receipt.cells.empty(), "all hidden aggregate no cells");
}

bool hiddenAuthoringObjectReportsHiddenBeforeAuthoringExclusion() {
  cr::CreativeObject object = makeAuthoringPointObject();
  object.visible = false;
  const cr::CreativeSpatialProjectionRequest request = makeRequest();

  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectToGrid(object, request);

  return expect(receipt.status ==
                    cr::CreativeSpatialProjectionStatus::NoProjection,
                "hidden authoring no projection") &&
         expect(receipt.message == "object_hidden",
                "hidden authoring message") &&
         expect(receipt.profile ==
                    cr::CreativeSpatialProjectionProfile::PointProjection,
                "hidden authoring profile") &&
         expect(receipt.occupancyKind ==
                    cr::CreativeSpatialOccupancyKind::Authoring,
                "hidden authoring occupancy") &&
         expect(receipt.cells.empty(), "hidden authoring no cells");
}

}  // namespace

int main() {
  const bool ok = defaultRequestExcludesAuthoringOnlyObjects() &&
                  explicitRequestIncludesAuthoringOnlyObjects() &&
                  visibleRoomStillProjects() &&
                  hiddenRoomDoesNotProject() &&
                  aggregateVisibleAndHiddenRoomsProjectsOnlyVisibleCells() &&
                  aggregateAllHiddenProjectableObjectsDoesNotProject() &&
                  hiddenAuthoringObjectReportsHiddenBeforeAuthoringExclusion();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
