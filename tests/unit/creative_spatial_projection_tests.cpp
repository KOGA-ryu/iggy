#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
#include <type_traits>
#include <utility>
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
  object.pathPoints = {
      cr::CreativePathPoint{{1.0, 0.0, 0.0}},
      cr::CreativePathPoint{{3.0, 0.0, 0.0}},
  };
  return object;
}

cr::CreativeObject makePathObject(
    cr::CreativeObjectId objectId,
    std::vector<cr::CreativePathPoint> pathPoints) {
  cr::CreativeObject object;
  object.id = objectId;
  object.kind = cr::CreativeObjectKind::PatrolRoute;
  object.name = "Patrol Route";
  object.pathPoints = std::move(pathPoints);
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

bool sameCoord(cr::CreativeGridCoord3 lhs, cr::CreativeGridCoord3 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool sameBounds(cr::CreativeGridBounds3 lhs, cr::CreativeGridBounds3 rhs) {
  return sameCoord(lhs.min, rhs.min) && sameCoord(lhs.max, rhs.max);
}

bool expectCoord(cr::CreativeGridCoord3 actual,
                 cr::CreativeGridCoord3 expected,
                 std::string_view message) {
  return expect(sameCoord(actual, expected), message);
}

bool expectBounds(cr::CreativeGridBounds3 actual,
                  cr::CreativeGridBounds3 expected,
                  std::string_view message) {
  return expect(sameBounds(actual, expected), message);
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

static_assert(std::is_trivially_copyable_v<cr::CreativeSpatialProjectionSummary>);

bool summaryMatchesFull(const cr::CreativeObject& object,
                        const cr::CreativeSpatialProjectionRequest& request,
                        std::string_view label) {
  const cr::CreativeSpatialProjectionSummary summary =
      cr::projectObjectToGridSummary(object, request);
  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectToGrid(object, request);
  return expect(summary.status == receipt.status, label) &&
         expect(summary.objectId == receipt.objectId, label) &&
         expect(summary.objectKind == receipt.objectKind, label) &&
         expect(summary.profile == receipt.profile, label) &&
         expect(summary.occupancyKind == receipt.occupancyKind, label) &&
         expectBounds(summary.projectedBounds, receipt.projectedBounds, label) &&
         expect(summary.cellCount == receipt.cells.size(), label);
}

bool summaryParityCoversAllProjectionProfiles() {
  const cr::CreativeSpatialProjectionRequest request = makeDeepRequest();
  const cr::CreativeObject point = makePointObject(701, {1.0, 1.0, 1.0});
  const cr::CreativeObject box = makeRoomObject(702);
  const cr::CreativeObject volume = makeVolumeObject(
      703,
      {{1.0, 1.0, 1.0}, {3.0, 3.0, 2.0}});
  const cr::CreativeObject line = makeLineObject(
      704,
      {{1.0, 1.0, 1.0}, {4.0, 1.0, 1.0}});
  const cr::CreativeObject path = makePathObject(
      705,
      {{{1.0, 1.0, 1.0}}, {{3.0, 1.0, 1.0}}, {{3.0, 3.0, 1.0}}});
  const cr::CreativeObject link = makeLinkObject(706);

  return summaryMatchesFull(point, request, "summary point parity") &&
         summaryMatchesFull(box, request, "summary box parity") &&
         summaryMatchesFull(volume, request, "summary volume parity") &&
         summaryMatchesFull(line, request, "summary line parity") &&
         summaryMatchesFull(path, request, "summary path parity") &&
         summaryMatchesFull(link, request, "summary link parity");
}

bool summaryParityCoversRejectedProjectionCases() {
  const cr::CreativeObject valid = makeRoomObject(711);
  cr::CreativeSpatialProjectionRequest invalidGrid = makeRequest();
  invalidGrid.cellSize = 0.0;
  cr::CreativeObject invalidObject = valid;
  invalidObject.id = cr::kInvalidObjectId;
  cr::CreativeObject hidden = valid;
  hidden.visible = false;
  const cr::CreativeObject authoring = makeAuthoringPointObject();
  cr::CreativeSpatialProjectionRequest includeAuthoring = makeRequest();
  includeAuthoring.includeAuthoringOnly = false;
  cr::CreativeObject offGrid = makePointObject(712, {-1.0, 0.0, 0.0});
  cr::CreativeObject invalidPath = makePathObject(713, {{{1.0, 1.0, 1.0}}});
  cr::CreativeObject invalidLink = makeLinkObject(714);
  invalidLink.pathPoints.clear();

  return summaryMatchesFull(valid, invalidGrid, "summary invalid grid parity") &&
         summaryMatchesFull(invalidObject, makeRequest(),
                            "summary invalid object parity") &&
         summaryMatchesFull(hidden, makeRequest(), "summary hidden parity") &&
         summaryMatchesFull(authoring, includeAuthoring,
                            "summary authoring parity") &&
         summaryMatchesFull(offGrid, makeRequest(),
                            "summary out of bounds parity") &&
         summaryMatchesFull(invalidPath, makeRequest(),
                            "summary invalid path parity") &&
         summaryMatchesFull(invalidLink, makeRequest(),
                            "summary invalid link parity");
}

bool summaryPathPreservesGlobalBacktrackingDeduplication() {
  const cr::CreativeObject path = makePathObject(
      721,
      {{{0.0, 0.0, 0.0}},
       {{3.0, 0.0, 0.0}},
       {{1.0, 0.0, 0.0}},
       {{3.0, 0.0, 0.0}}});
  const cr::CreativeSpatialProjectionRequest request = makeDeepRequest();
  const cr::CreativeSpatialProjectionSummary summary =
      cr::projectObjectToGridSummary(path, request);
  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectToGrid(path, request);

  return expect(summary.status == cr::CreativeSpatialProjectionStatus::Projected,
                "summary backtracking projected") &&
         expect(summary.cellCount == 4U,
                "summary backtracking globally deduplicated count") &&
         expect(summary.cellCount == receipt.cells.size(),
                "summary backtracking full count parity") &&
         expect(summaryMatchesFull(path, request,
                                   "summary backtracking parity"),
                "summary backtracking metadata parity");
}

bool largeBoundsSummaryCountsWithoutFullMaterialization() {
  const cr::CreativeObject object = makeVolumeObject(
      731,
      {{0.0, 0.0, 0.0}, {128.0, 128.0, 128.0}});
  cr::CreativeSpatialProjectionRequest request;
  request.gridSize = {128, 128, 128};
  const cr::CreativeSpatialProjectionSummary summary =
      cr::projectObjectToGridSummary(object, request);

  return expect(summary.status == cr::CreativeSpatialProjectionStatus::Projected,
                "large summary projected") &&
         expect(summary.cellCount == 128U * 128U * 128U,
                "large summary arithmetic cell count") &&
         expect(summary.projectedBounds.max.x == 128,
                "large summary max x") &&
         expect(summary.projectedBounds.max.y == 128,
                "large summary max y") &&
         expect(summary.projectedBounds.max.z == 128,
                "large summary max z");
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

bool worldToGridCoordUsesContainingCellBoundaries() {
  return expectCoord(cr::worldToGridCoord({1.25, 2.0, 0.01}, 1.0),
                     {1, 2, 0},
                     "positive containing cells") &&
         expectCoord(cr::worldToGridCoord({1.999, 2.001, -0.001}, 1.0),
                     {1, 2, -1},
                     "fractional values around boundary") &&
         expectCoord(cr::worldToGridCoord({-0.001, -1.0, -1.001}, 1.0),
                     {-1, -1, -2},
                     "negative containing cells");
}

bool worldBoundsToGridBoundsUsesHalfOpenBoundaries() {
  return expectBounds(cr::worldBoundsToGridBounds({{1.0, 2.0, 0.0},
                                                   {2.0, 3.0, 1.0}},
                                                  1.0),
                      {{1, 2, 0}, {2, 3, 1}},
                      "exact max boundary stays half-open") &&
         expectBounds(cr::worldBoundsToGridBounds({{0.25, -0.25, 0.0},
                                                   {2.01, 0.99, 1.01}},
                                                  1.0),
                      {{0, -1, 0}, {3, 1, 2}},
                      "fractional max boundary expands") &&
         expectBounds(cr::worldBoundsToGridBounds({{-2.5, -0.1, -1.0},
                                                   {-1.0, 1.0, 0.0}},
                                                  1.0),
                      {{-3, -1, -1}, {-1, 1, 0}},
                      "negative min boundary floors");
}

bool invalidWorldToGridCoordInputsReturnDefault() {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  const double tooLarge =
      static_cast<double>(std::numeric_limits<std::int32_t>::max()) + 1024.0;
  const cr::CreativeGridCoord3 zero{};
  return expectCoord(cr::worldToGridCoord({1.0, 2.0, 3.0}, 0.0),
                     zero,
                     "zero cell size default") &&
         expectCoord(cr::worldToGridCoord({1.0, 2.0, 3.0}, -1.0),
                     zero,
                     "negative cell size default") &&
         expectCoord(cr::worldToGridCoord({1.0, 2.0, 3.0}, nan),
                     zero,
                     "nan cell size default") &&
         expectCoord(cr::worldToGridCoord({1.0, 2.0, 3.0}, inf),
                     zero,
                     "infinite cell size default") &&
         expectCoord(cr::worldToGridCoord({nan, 2.0, 3.0}, 1.0),
                     zero,
                     "nan coordinate default") &&
         expectCoord(cr::worldToGridCoord({1.0, inf, 3.0}, 1.0),
                     zero,
                     "infinite coordinate default") &&
         expectCoord(cr::worldToGridCoord({tooLarge, 0.0, 0.0}, 1.0),
                     zero,
                     "out-of-range coordinate default");
}

bool invalidWorldBoundsToGridBoundsInputsReturnDefault() {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  const double tooLarge =
      static_cast<double>(std::numeric_limits<std::int32_t>::max()) + 1024.0;
  const cr::CreativeGridBounds3 zero{};
  return expectBounds(cr::worldBoundsToGridBounds({{0.0, 0.0, 0.0},
                                                   {1.0, 1.0, 1.0}},
                                                  nan),
                      zero,
                      "nan cell size bounds default") &&
         expectBounds(cr::worldBoundsToGridBounds({{0.0, 0.0, 0.0},
                                                   {1.0, 1.0, 1.0}},
                                                  inf),
                      zero,
                      "infinite cell size bounds default") &&
         expectBounds(cr::worldBoundsToGridBounds({{nan, 0.0, 0.0},
                                                   {1.0, 1.0, 1.0}},
                                                  1.0),
                      zero,
                      "non-finite min bounds default") &&
         expectBounds(cr::worldBoundsToGridBounds({{0.0, 0.0, 0.0},
                                                   {1.0, inf, 1.0}},
                                                  1.0),
                      zero,
                      "non-finite max bounds default") &&
         expectBounds(cr::worldBoundsToGridBounds({{0.0, 0.0, 0.0},
                                                   {tooLarge, 1.0, 1.0}},
                                                  1.0),
                      zero,
                      "out-of-range max bounds default");
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

bool nonFinitePointDoesNotProjectOriginCell() {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  const cr::CreativeSpatialProjectionRequest request = makeRequest();
  const cr::CreativeObject nanObject = makePointObject(431, {nan, 2.0, 0.0});
  const cr::CreativeObject infObject = makePointObject(432, {1.0, inf, 0.0});
  const cr::CreativeSpatialProjectionReceipt nanReceipt =
      cr::projectObjectToGrid(nanObject, request);
  const cr::CreativeSpatialProjectionReceipt infReceipt =
      cr::projectObjectToGrid(infObject, request);

  return expect(nanReceipt.status == cr::CreativeSpatialProjectionStatus::OutOfBounds,
                "nan point out of bounds") &&
         expect(nanReceipt.message == "out_of_bounds", "nan point message") &&
         expect(nanReceipt.cells.empty(), "nan point no cells") &&
         expect(!hasCellAt(nanReceipt, nanObject.id, {0, 0, 0}, request.gridSize),
                "nan point no origin cell") &&
         expect(infReceipt.status == cr::CreativeSpatialProjectionStatus::OutOfBounds,
                "inf point out of bounds") &&
         expect(infReceipt.message == "out_of_bounds", "inf point message") &&
         expect(infReceipt.cells.empty(), "inf point no cells") &&
         expect(!hasCellAt(infReceipt, infObject.id, {0, 0, 0}, request.gridSize),
                "inf point no origin cell");
}

bool maxIntPointDoesNotOverflowProjectionBounds() {
  const double maxCell =
      static_cast<double>(std::numeric_limits<std::int32_t>::max());
  const cr::CreativeSpatialProjectionRequest request = makeRequest();
  const cr::CreativeObject object = makePointObject(433, {maxCell, 0.0, 0.0});
  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectToGrid(object, request);

  return expect(receipt.status == cr::CreativeSpatialProjectionStatus::OutOfBounds,
                "max int point out of bounds") &&
         expect(receipt.message == "out_of_bounds",
                "max int point message") &&
         expect(receipt.cells.empty(), "max int point no cells") &&
         expectBounds(receipt.projectedBounds,
                      {},
                      "max int point safe empty bounds");
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

bool invalidVolumeBoundsDoNotProjectOriginCell() {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double tooLarge =
      static_cast<double>(std::numeric_limits<std::int32_t>::max()) + 1024.0;
  const cr::CreativeSpatialProjectionRequest request = makeRequest();
  const cr::CreativeObject nanObject =
      makeVolumeObject(451, {{nan, 0.0, 0.0}, {1.0, 1.0, 1.0}});
  const cr::CreativeObject rangeObject =
      makeVolumeObject(452, {{0.0, 0.0, 0.0}, {tooLarge, 1.0, 1.0}});
  const cr::CreativeSpatialProjectionReceipt nanReceipt =
      cr::projectObjectToGrid(nanObject, request);
  const cr::CreativeSpatialProjectionReceipt rangeReceipt =
      cr::projectObjectToGrid(rangeObject, request);

  return expect(nanReceipt.status == cr::CreativeSpatialProjectionStatus::OutOfBounds,
                "nan bounds out of bounds") &&
         expect(nanReceipt.message == "out_of_bounds", "nan bounds message") &&
         expect(nanReceipt.cells.empty(), "nan bounds no cells") &&
         expect(!hasCellAt(nanReceipt, nanObject.id, {0, 0, 0}, request.gridSize),
                "nan bounds no origin cell") &&
         expect(rangeReceipt.status == cr::CreativeSpatialProjectionStatus::OutOfBounds,
                "range bounds out of bounds") &&
         expect(rangeReceipt.message == "out_of_bounds", "range bounds message") &&
         expect(rangeReceipt.cells.empty(), "range bounds no cells") &&
         expect(!hasCellAt(rangeReceipt, rangeObject.id, {0, 0, 0}, request.gridSize),
                "range bounds no origin cell");
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

bool infiniteCellSizeRejectsProjectionRequests() {
  cr::CreativeSpatialProjectionRequest request = makeRequest();
  request.cellSize = std::numeric_limits<double>::infinity();
  const cr::CreativeObject room = makeRoomObject(61);
  const cr::CreativeSpatialProjectionReceipt single =
      cr::projectObjectToGrid(room, request);
  const std::vector<cr::CreativeObject> objects{room};
  const cr::CreativeSpatialProjectionReceipt aggregate =
      cr::projectObjectsToGrid(objects, request);

  return expect(single.status == cr::CreativeSpatialProjectionStatus::InvalidGrid,
                "single infinite cell invalid grid") &&
         expect(single.message == "invalid_grid",
                "single infinite cell message") &&
         expect(single.cells.empty(), "single infinite cell no cells") &&
         expect(aggregate.status == cr::CreativeSpatialProjectionStatus::InvalidGrid,
                "aggregate infinite cell invalid grid") &&
         expect(aggregate.message == "invalid_grid",
                "aggregate infinite cell message") &&
         expect(aggregate.cells.empty(), "aggregate infinite cell no cells");
}

bool pathProjectionSamplesAdjacentSegmentsWithStableDedupe() {
  const cr::CreativeObject object = makePathObject(
      50,
      {
          cr::CreativePathPoint{{1.0, 0.0, 0.0}},
          cr::CreativePathPoint{{3.0, 0.0, 0.0}},
          cr::CreativePathPoint{{3.0, 0.0, 2.0}},
      });
  const cr::CreativeSpatialProjectionRequest request = makeDeepRequest();

  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectToGrid(object, request);
  const std::vector<cr::CreativeGridCoord3> expectedCoords{
      {1, 0, 0},
      {2, 0, 0},
      {3, 0, 0},
      {3, 0, 1},
      {3, 0, 2},
  };

  bool ok = expect(receipt.status ==
                       cr::CreativeSpatialProjectionStatus::Projected,
                   "path projected") &&
            expect(receipt.message == "projected", "path projected message") &&
            expect(receipt.objectId == object.id, "path object id") &&
            expect(receipt.objectKind == cr::CreativeObjectKind::PatrolRoute,
                   "path object kind") &&
            expect(receipt.profile ==
                       cr::CreativeSpatialProjectionProfile::PathProjection,
                   "path projection profile") &&
            expect(receipt.occupancyKind ==
                       cr::CreativeSpatialOccupancyKind::Gameplay,
                   "path occupancy") &&
            expect(receipt.projectedBounds.min.x == 1,
                   "path bounds min x") &&
            expect(receipt.projectedBounds.min.y == 0,
                   "path bounds min y") &&
            expect(receipt.projectedBounds.min.z == 0,
                   "path bounds min z") &&
            expect(receipt.projectedBounds.max.x == 4,
                   "path bounds max x") &&
            expect(receipt.projectedBounds.max.y == 1,
                   "path bounds max y") &&
            expect(receipt.projectedBounds.max.z == 3,
                   "path bounds max z") &&
            expect(receipt.cells.size() == expectedCoords.size(),
                   "path deduped cell count");

  for (std::size_t index = 0; index < expectedCoords.size() &&
                              index < receipt.cells.size();
       ++index) {
    const cr::CreativeSpatialCell& cell = receipt.cells[index];
    ok = expect(cell.objectId == object.id, "path cell object id") &&
         expect(cell.objectKind == cr::CreativeObjectKind::PatrolRoute,
                "path cell object kind") &&
         expect(cell.occupancyKind == cr::CreativeSpatialOccupancyKind::Gameplay,
                "path cell occupancy") &&
         expect(sameCoord(cell.coord, expectedCoords[index]),
                "path cell order") &&
         expect(cell.index == cr::toGridIndex(expectedCoords[index],
                                             request.gridSize),
                "path cell row-major index") &&
         ok;
  }

  return ok;
}

bool hiddenPathProjectsNoCells() {
  cr::CreativeObject object = makePathObject(
      51,
      {
          cr::CreativePathPoint{{1.0, 0.0, 0.0}},
          cr::CreativePathPoint{{3.0, 0.0, 0.0}},
      });
  object.visible = false;

  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectToGrid(object, makeDeepRequest());

  return expect(receipt.status ==
                    cr::CreativeSpatialProjectionStatus::NoProjection,
                "hidden path no projection") &&
         expect(receipt.message == "object_hidden",
                "hidden path message") &&
         expect(receipt.profile ==
                    cr::CreativeSpatialProjectionProfile::PathProjection,
                "hidden path profile") &&
         expect(receipt.occupancyKind ==
                    cr::CreativeSpatialOccupancyKind::Gameplay,
                "hidden path occupancy") &&
         expect(receipt.cells.empty(), "hidden path no cells");
}

bool offGridPathEndpointDoesNotClampToBorder() {
  const cr::CreativeObject object = makePathObject(
      52,
      {
          cr::CreativePathPoint{{-1.0, 0.0, 0.0}},
          cr::CreativePathPoint{{3.0, 0.0, 0.0}},
      });
  cr::CreativeSpatialProjectionRequest request = makeDeepRequest();
  request.clampToGrid = true;

  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectToGrid(object, request);

  return expect(receipt.status == cr::CreativeSpatialProjectionStatus::OutOfBounds,
                "off-grid path out of bounds") &&
         expect(receipt.message == "out_of_bounds",
                "off-grid path message") &&
         expect(receipt.cells.empty(), "off-grid path no cells") &&
         expect(!hasCellAt(receipt, object.id, cr::CreativeGridCoord3{0, 0, 0},
                           request.gridSize),
                "off-grid path no clamped border cell");
}

bool linkProjectionSamplesStoredEndpoints() {
  const cr::CreativeObject object = makeLinkObject(49);
  const cr::CreativeSpatialProjectionRequest request = makeRequest();

  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectToGrid(object, request);

  return expect(receipt.status == cr::CreativeSpatialProjectionStatus::Projected,
                "link projection projected") &&
         expect(receipt.message == "projected",
                "link projection message") &&
         expect(receipt.profile ==
                    cr::CreativeSpatialProjectionProfile::LinkProjection,
                "link projection profile") &&
         expect(receipt.occupancyKind ==
                    cr::CreativeSpatialOccupancyKind::Navigation,
                "link projection occupancy") &&
         expect(receipt.cells.size() == 3U, "link projection cell count") &&
         expect(hasCellAt(receipt, object.id, cr::CreativeGridCoord3{1, 0, 0},
                          request.gridSize),
                "link projection start cell") &&
         expect(hasCellAt(receipt, object.id, cr::CreativeGridCoord3{2, 0, 0},
                          request.gridSize),
                "link projection middle cell") &&
         expect(hasCellAt(receipt, object.id, cr::CreativeGridCoord3{3, 0, 0},
                          request.gridSize),
                "link projection end cell");
}

bool invalidLinkEndpointsDoNotProject() {
  cr::CreativeObject object = makeLinkObject(49);
  object.pathPoints.pop_back();
  const cr::CreativeSpatialProjectionRequest request = makeRequest();

  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectToGrid(object, request);

  return expect(receipt.status ==
                    cr::CreativeSpatialProjectionStatus::EmptyProjection,
                "invalid link endpoint empty") &&
         expect(receipt.message == "invalid_line_endpoints",
                "invalid link endpoint message") &&
         expect(receipt.cells.empty(), "invalid link endpoint no cells");
}

bool offGridLinkEndpointDoesNotClampToBorder() {
  cr::CreativeObject object = makeLinkObject(49);
  object.pathPoints.front().position.x = -1.0;
  cr::CreativeSpatialProjectionRequest request = makeRequest();
  request.clampToGrid = true;

  const cr::CreativeSpatialProjectionReceipt receipt =
      cr::projectObjectToGrid(object, request);

  return expect(receipt.status == cr::CreativeSpatialProjectionStatus::OutOfBounds,
                "off-grid link out of bounds") &&
         expect(receipt.message == "out_of_bounds",
                "off-grid link message") &&
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
                  worldToGridCoordUsesContainingCellBoundaries() &&
                  worldBoundsToGridBoundsUsesHalfOpenBoundaries() &&
                  invalidWorldToGridCoordInputsReturnDefault() &&
                  invalidWorldBoundsToGridBoundsInputsReturnDefault() &&
                  summaryParityCoversAllProjectionProfiles() &&
                  summaryParityCoversRejectedProjectionCases() &&
                  summaryPathPreservesGlobalBacktrackingDeduplication() &&
                  largeBoundsSummaryCountsWithoutFullMaterialization() &&
                  defaultRequestExcludesAuthoringOnlyObjects() &&
                  explicitRequestIncludesAuthoringOnlyObjects() &&
                  visibleRoomStillProjects() &&
                  hiddenRoomDoesNotProject() &&
                  aggregateVisibleAndHiddenRoomsProjectsOnlyVisibleCells() &&
                  aggregateAllHiddenProjectableObjectsDoesNotProject() &&
                  hiddenAuthoringObjectReportsHiddenBeforeAuthoringExclusion() &&
                  offGridPointDoesNotClampToBorderWhenClampEnabled() &&
                  offGridPointDoesNotProjectWhenClampDisabled() &&
                  nonFinitePointDoesNotProjectOriginCell() &&
                  maxIntPointDoesNotOverflowProjectionBounds() &&
                  inGridPointProjectsAtExactRowMajorCell() &&
                  disjointVolumeClampIntersectsToEmptyProjection() &&
                  partialVolumeClampEmitsOnlyIntersectingCells() &&
                  invalidVolumeBoundsDoNotProjectOriginCell() &&
                  offGridLineDoesNotClampEndpointsToBorder() &&
                  infiniteCellSizeRejectsProjectionRequests() &&
                  pathProjectionSamplesAdjacentSegmentsWithStableDedupe() &&
                  hiddenPathProjectsNoCells() &&
                  offGridPathEndpointDoesNotClampToBorder() &&
                  linkProjectionSamplesStoredEndpoints() &&
                  invalidLinkEndpointsDoNotProject() &&
                  offGridLinkEndpointDoesNotClampToBorder() &&
                  aggregateIgnoresOffGridPointBorderArtifact() &&
                  boundedMoveShiftsCrateProjectionCells();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
