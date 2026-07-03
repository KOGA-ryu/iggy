#include "app/iggy3d/creative/SpatialProjection.hpp"
#include "app/iggy3d/creative/DocumentMutation.hpp"

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

cr::CreativeObject makePointObject(cr::CreativeObjectId objectId,
                                   cr::CreativeVec3 position) {
  cr::CreativeObject object;
  object.id = objectId;
  object.kind = cr::CreativeObjectKind::SpawnPoint;
  object.name = "Spawn Point";
  object.transform.position = position;
  return object;
}

cr::CreativeObject makeVolumeObject(cr::CreativeObjectId objectId,
                                    cr::CreativeBounds bounds) {
  cr::CreativeObject object;
  object.id = objectId;
  object.kind = cr::CreativeObjectKind::WaterVolume;
  object.name = "Water Volume";
  object.bounds = bounds;
  return object;
}

cr::CreativeObject makeLineObject(cr::CreativeObjectId objectId,
                                  cr::CreativeBounds bounds) {
  cr::CreativeObject object;
  object.id = objectId;
  object.kind = cr::CreativeObjectKind::CameraRail;
  object.name = "Camera Rail";
  object.bounds = bounds;
  return object;
}

cr::CreativeObject makeLinkObject(cr::CreativeObjectId objectId) {
  cr::CreativeObject object;
  object.id = objectId;
  object.kind = cr::CreativeObjectKind::NavLink;
  object.name = "Nav Link";
  object.bounds = cr::CreativeBounds{cr::CreativeVec3{-1.0, 0.0, 0.0},
                                    cr::CreativeVec3{3.0, 0.0, 0.0}};
  return object;
}

cr::CreativeSpatialProjectionRequest makeRequest() {
  cr::CreativeSpatialProjectionRequest request;
  request.gridSize = {8, 8, 2};
  return request;
}

cr::CreativeSpatialProjectionRequest makeDeepRequest() {
  cr::CreativeSpatialProjectionRequest request;
  request.gridSize = {8, 8, 8};
  return request;
}

bool hasCellAt(const cr::CreativeSpatialProjectionReceipt& receipt,
               cr::CreativeObjectId objectId,
               cr::CreativeGridCoord3 coord,
               cr::CreativeGridSize3 gridSize) {
  const cr::CreativeGridIndex index = cr::toGridIndex(coord, gridSize);
  for (const cr::CreativeSpatialCell& cell : receipt.cells) {
    if (cell.objectId == objectId && cell.coord.x == coord.x &&
        cell.coord.y == coord.y && cell.coord.z == coord.z &&
        cell.index == index) {
      return true;
    }
  }
  return false;
}

bool rowMajorIndexRoundTripIsStable() {
  const cr::CreativeGridSize3 size{8, 8, 2};
  const cr::CreativeGridCoord3 coord{3, 4, 1};
  const cr::CreativeGridIndex index = cr::toGridIndex(coord, size);
  const cr::CreativeGridCoord3 roundTrip = cr::toGridCoord(index, size);

  return expect(index == 99U, "row-major index formula") &&
         expect(roundTrip.x == coord.x, "row-major round trip x") &&
         expect(roundTrip.y == coord.y, "row-major round trip y") &&
         expect(roundTrip.z == coord.z, "row-major round trip z");
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

bool offGridPointDoesNotClampToBorderWhenClampEnabled() {
  const cr::CreativeObject object =
      makePointObject(41, cr::CreativeVec3{-1.0, 2.0, 0.0});
  cr::CreativeSpatialProjectionRequest request = makeRequest();
  request.clampToGrid = true;

  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectToGrid(object, request);

  return expect(receipt.status == cr::CreativeSpatialProjectionStatus::OutOfBounds,
                "off-grid point clamp true out of bounds") &&
         expect(receipt.message == "out_of_bounds",
                "off-grid point clamp true message") &&
         expect(receipt.cells.empty(), "off-grid point clamp true no cells") &&
         expect(!hasCellAt(receipt, object.id, cr::CreativeGridCoord3{0, 2, 0},
                           request.gridSize),
                "off-grid point clamp true no border cell");
}

bool offGridPointDoesNotProjectWhenClampDisabled() {
  const cr::CreativeObject object =
      makePointObject(42, cr::CreativeVec3{8.0, 2.0, 0.0});
  cr::CreativeSpatialProjectionRequest request = makeRequest();
  request.clampToGrid = false;

  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectToGrid(object, request);

  return expect(receipt.status == cr::CreativeSpatialProjectionStatus::OutOfBounds,
                "off-grid point clamp false out of bounds") &&
         expect(receipt.message == "out_of_bounds",
                "off-grid point clamp false message") &&
         expect(receipt.cells.empty(), "off-grid point clamp false no cells") &&
         expect(!hasCellAt(receipt, object.id, cr::CreativeGridCoord3{7, 2, 0},
                           request.gridSize),
                "off-grid point clamp false no border cell");
}

bool inGridPointProjectsAtExactRowMajorCell() {
  const cr::CreativeObject object =
      makePointObject(43, cr::CreativeVec3{3.0, 4.0, 1.0});
  const cr::CreativeSpatialProjectionRequest request = makeRequest();

  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectToGrid(object, request);

  return expect(receipt.status == cr::CreativeSpatialProjectionStatus::Projected,
                "in-grid point projected") &&
         expect(receipt.cells.size() == 1U, "in-grid point one cell") &&
         expect(receipt.cells[0].coord.x == 3, "in-grid point x") &&
         expect(receipt.cells[0].coord.y == 4, "in-grid point y") &&
         expect(receipt.cells[0].coord.z == 1, "in-grid point z") &&
         expect(receipt.cells[0].index == 99U, "in-grid point row-major index");
}

bool disjointVolumeClampIntersectsToEmptyProjection() {
  const cr::CreativeObject object = makeVolumeObject(
      44,
      cr::CreativeBounds{cr::CreativeVec3{10.0, 0.0, 0.0},
                         cr::CreativeVec3{11.0, 1.0, 1.0}});
  cr::CreativeSpatialProjectionRequest request = makeRequest();
  request.clampToGrid = true;

  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectToGrid(object, request);

  return expect(receipt.status ==
                    cr::CreativeSpatialProjectionStatus::EmptyProjection,
                "disjoint volume empty") &&
         expect(receipt.message == "empty_projection",
                "disjoint volume message") &&
         expect(receipt.cells.empty(), "disjoint volume no cells") &&
         expect(receipt.projectedBounds.min.x == 8,
                "disjoint volume intersection min x") &&
         expect(receipt.projectedBounds.max.x == 8,
                "disjoint volume intersection max x");
}

bool partialVolumeClampEmitsOnlyIntersectingCells() {
  const cr::CreativeObject object = makeVolumeObject(
      45,
      cr::CreativeBounds{cr::CreativeVec3{-1.0, 1.0, 0.0},
                         cr::CreativeVec3{2.0, 3.0, 1.0}});
  cr::CreativeSpatialProjectionRequest request = makeRequest();
  request.clampToGrid = true;

  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectToGrid(object, request);

  return expect(receipt.status == cr::CreativeSpatialProjectionStatus::Projected,
                "partial volume projected") &&
         expect(receipt.cells.size() == 4U,
                "partial volume intersecting cell count") &&
         expect(hasCellAt(receipt, object.id, cr::CreativeGridCoord3{0, 1, 0},
                          request.gridSize),
                "partial volume cell 0 1 0") &&
         expect(hasCellAt(receipt, object.id, cr::CreativeGridCoord3{1, 1, 0},
                          request.gridSize),
                "partial volume cell 1 1 0") &&
         expect(hasCellAt(receipt, object.id, cr::CreativeGridCoord3{0, 2, 0},
                          request.gridSize),
                "partial volume cell 0 2 0") &&
         expect(hasCellAt(receipt, object.id, cr::CreativeGridCoord3{1, 2, 0},
                          request.gridSize),
                "partial volume cell 1 2 0") &&
         expect(!hasCellAt(receipt, object.id, cr::CreativeGridCoord3{7, 1, 0},
                           request.gridSize),
                "partial volume no border-relocated cell");
}

bool offGridLineDoesNotClampEndpointsToBorder() {
  const cr::CreativeObject object = makeLineObject(
      46,
      cr::CreativeBounds{cr::CreativeVec3{-1.0, 0.0, 0.0},
                         cr::CreativeVec3{3.0, 0.0, 0.0}});
  cr::CreativeSpatialProjectionRequest request = makeRequest();
  request.clampToGrid = true;

  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectToGrid(object, request);

  return expect(receipt.status == cr::CreativeSpatialProjectionStatus::OutOfBounds,
                "off-grid line out of bounds") &&
         expect(receipt.message == "out_of_bounds",
                "off-grid line message") &&
         expect(receipt.cells.empty(), "off-grid line no cells") &&
         expect(!hasCellAt(receipt, object.id, cr::CreativeGridCoord3{0, 0, 0},
                           request.gridSize),
                "off-grid line no clamped border cell");
}

bool linkProjectionDoesNotEmitBorderCells() {
  const cr::CreativeObject object = makeLinkObject(49);
  cr::CreativeSpatialProjectionRequest request = makeRequest();
  request.clampToGrid = true;

  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectLinkObjectToGrid(object, request);

  return expect(receipt.status == cr::CreativeSpatialProjectionStatus::NoProjection,
                "link projection no projection") &&
         expect(receipt.message == "no_projection",
                "link projection message") &&
         expect(receipt.cells.empty(), "link projection no cells") &&
         expect(!hasCellAt(receipt, object.id, cr::CreativeGridCoord3{0, 0, 0},
                           request.gridSize),
                "link projection no border cell");
}

bool aggregateIgnoresOffGridPointBorderArtifact() {
  const cr::CreativeObject offGridPoint =
      makePointObject(47, cr::CreativeVec3{-1.0, 2.0, 0.0});
  const cr::CreativeObject room = makeRoomObject(48);
  const std::vector<cr::CreativeObject> objects{offGridPoint, room};
  cr::CreativeSpatialProjectionRequest request = makeRequest();
  request.clampToGrid = true;

  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectsToGrid(objects, request);

  return expect(receipt.status == cr::CreativeSpatialProjectionStatus::Projected,
                "aggregate off-grid point projected from room") &&
         expect(receipt.cells.size() == 1U,
                "aggregate off-grid point only room cell") &&
         expect(receipt.cells[0].objectId == room.id,
                "aggregate off-grid point room id") &&
         expect(!hasCellAt(receipt, offGridPoint.id,
                           cr::CreativeGridCoord3{0, 2, 0},
                           request.gridSize),
                "aggregate off-grid point no border artifact");
}

bool boundedMoveShiftsCrateProjectionCells() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Document");
  cr::CreativeDocumentCreateRequest createRequest;
  createRequest.kind = cr::CreativeObjectKind::Crate;
  createRequest.name = "Crate";
  createRequest.transform = cr::CreativeTransform{};
  createRequest.hasTransformOverride = true;
  createRequest.bounds = cr::CreativeBounds{cr::CreativeVec3{0.0, 0.0, 0.0},
                                           cr::CreativeVec3{1.0, 1.0, 1.0}};
  createRequest.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt createReceipt =
      document.createObject(createRequest);
  const cr::CreativeObjectId crateId = createReceipt.objectId;
  const cr::CreativeSpatialProjectionRequest request = makeDeepRequest();

  const cr::CreativeSpatialProjectionReceipt before =
      cr::projectObjectsToGrid(document.objects(), request);
  const cr::CreativeDocumentMutationReceipt moveReceipt =
      cr::moveDocumentObject(document, crateId,
                             cr::CreativeVec3{2.0, 0.0, 3.0});
  const cr::CreativeSpatialProjectionReceipt after =
      cr::projectObjectsToGrid(document.objects(), request);
  const cr::CreativeGridCoord3 oldCoord{0, 0, 0};
  const cr::CreativeGridCoord3 newCoord{2, 0, 3};

  return expect(createReceipt.status == cr::CreativeDocumentCreateStatus::Created,
                "crate projection setup created") &&
         expect(before.status == cr::CreativeSpatialProjectionStatus::Projected,
                "crate projection before projected") &&
         expect(hasCellAt(before, crateId, oldCoord, request.gridSize),
                "crate projection before old cell") &&
         expect(moveReceipt.status == cr::CreativeDocumentMutationStatus::Applied,
                "crate projection move applied") &&
         expect(moveReceipt.changed, "crate projection move changed") &&
         expect(moveReceipt.mutationKind == cr::CreativeMutationKind::Move,
                "crate projection move kind") &&
         expect(after.status == cr::CreativeSpatialProjectionStatus::Projected,
                "crate projection after projected") &&
         expect(hasCellAt(after, crateId, newCoord, request.gridSize),
                "crate projection after new cell") &&
         expect(!hasCellAt(after, crateId, oldCoord, request.gridSize),
                "crate projection after old cell absent");
}

}  // namespace

int main() {
  const bool ok = rowMajorIndexRoundTripIsStable() &&
                  defaultRequestExcludesAuthoringOnlyObjects() &&
                  explicitRequestIncludesAuthoringOnlyObjects() &&
                  visibleRoomStillProjects() &&
                  hiddenRoomDoesNotProject() &&
                  aggregateVisibleAndHiddenRoomsProjectsOnlyVisibleCells() &&
                  aggregateAllHiddenProjectableObjectsDoesNotProject() &&
                  hiddenAuthoringObjectReportsHiddenBeforeAuthoringExclusion() &&
                  offGridPointDoesNotClampToBorderWhenClampEnabled() &&
                  offGridPointDoesNotProjectWhenClampDisabled() &&
                  inGridPointProjectsAtExactRowMajorCell() &&
                  disjointVolumeClampIntersectsToEmptyProjection() &&
                  partialVolumeClampEmitsOnlyIntersectingCells() &&
                  offGridLineDoesNotClampEndpointsToBorder() &&
                  linkProjectionDoesNotEmitBorderCells() &&
                  aggregateIgnoresOffGridPointBorderArtifact() &&
                  boundedMoveShiftsCrateProjectionCells();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
