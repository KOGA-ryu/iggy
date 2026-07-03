#include "app/iggy3d/creative/SpatialProjection.hpp"

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

cr::CreativeObject makeAuthoringPointObject() {
  cr::CreativeObject object;
  object.id = 7;
  object.kind = cr::CreativeObjectKind::Note;
  object.name = "Authoring note";
  object.transform.position = {2.0, 3.0, 0.0};
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

}  // namespace

int main() {
  const bool ok = defaultRequestExcludesAuthoringOnlyObjects() &&
                  explicitRequestIncludesAuthoringOnlyObjects();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
