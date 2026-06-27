#include "app/iggy3d/room/ProductRoomGeometryOptimization.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::EditableRoomFloor floor(std::string id,
                                float x,
                                float z,
                                std::string material = "debug_floor") {
  iggy3d::EditableRoomFloor out;
  out.id = std::move(id);
  out.storyIndex = 0;
  out.centerMeters = {x, -0.05F, z};
  out.sizeMeters = {1.0F, 0.10F, 1.0F};
  out.semantics = iggy3d::defaultFloorSemantics(std::move(material));
  return out;
}

iggy3d::EditableRoomWall wall(std::string id,
                              float startX,
                              float startZ,
                              float endX,
                              float endZ,
                              std::string material = "debug_wall") {
  iggy3d::EditableRoomWall out;
  out.id = std::move(id);
  out.storyIndex = 0;
  out.startMeters = {startX, 0.0F, startZ};
  out.endMeters = {endX, 0.0F, endZ};
  out.bottomY = 0.0F;
  out.heightMeters = 2.5F;
  out.thicknessMeters = 1.0F;
  out.semantics = iggy3d::defaultWallSemantics(std::move(material));
  return out;
}

}  // namespace

int main() {
  bool ok = true;

  {
    const auto report = iggy3d::buildProductRoomGeometryOptimizationReport(nullptr);
    ok = expect(!report.ok, "missing document is not ok") && ok;
    ok = expect(report.status == "product_room_geometry_document_missing",
                "missing document status") &&
         ok;
    ok = expect(report.naiveDrawCount == 0, "missing document zero draw count") && ok;
    ok = expect(report.optimizedTriangleCount == 0,
                "missing document zero triangle count") &&
         ok;
  }

  {
    iggy3d::EditableRoomDocument document;
    document.floors.push_back(floor("floor_1", 0.0F, 0.0F));
    document.walls.push_back(wall("wall_1", 0.0F, 0.0F, 1.0F, 0.0F));
    const auto report = iggy3d::buildProductRoomGeometryOptimizationReport(&document);
    ok = expect(report.ok, "single document report ok") && ok;
    ok = expect(report.status == "product_room_geometry_optimization_ready",
                "ready status") &&
         ok;
    ok = expect(report.sourceFloorCount == 1, "single source floor count") && ok;
    ok = expect(report.sourceWallCount == 1, "single source wall count") && ok;
    ok = expect(report.naiveDrawCount == 2, "single naive draw count") && ok;
    ok = expect(report.naiveTriangleCount == 14, "single naive triangle count") &&
         ok;
    ok = expect(report.optimizedFloorRectCount == 1,
                "single optimized floor rect count") &&
         ok;
    ok = expect(report.optimizedWallRunCount == 1,
                "single optimized wall run count") &&
         ok;
    ok = expect(report.drawCountAvoided == 0, "single draw avoided") && ok;
    ok = expect(report.triangleCountAvoided == 0, "single triangle avoided") && ok;
  }

  {
    iggy3d::EditableRoomDocument document;
    document.floors.push_back(floor("floor_1", 0.0F, 0.0F));
    document.floors.push_back(floor("floor_2", 1.0F, 0.0F));
    document.floors.push_back(floor("floor_3", 0.0F, 1.0F));
    document.floors.push_back(floor("floor_4", 1.0F, 1.0F));
    const auto report = iggy3d::buildProductRoomGeometryOptimizationReport(&document);
    ok = expect(report.sourceFloorCount == 4, "2x2 source floor count") && ok;
    ok = expect(report.naiveDrawCount == 4, "2x2 naive draw count") && ok;
    ok = expect(report.naiveTriangleCount == 8, "2x2 naive triangle count") && ok;
    ok = expect(report.optimizedFloorRectCount == 1,
                "2x2 optimized floor rect count") &&
         ok;
    ok = expect(report.optimizedDrawCount == 1, "2x2 optimized draw count") && ok;
    ok = expect(report.optimizedTriangleCount == 2,
                "2x2 optimized triangle count") &&
         ok;
    ok = expect(report.drawCountAvoided == 3, "2x2 draw avoided") && ok;
    ok = expect(report.triangleCountAvoided == 6, "2x2 triangle avoided") && ok;
    ok = expect(report.floorRectMergeCandidateCount == 3,
                "2x2 floor merge candidate count") &&
         ok;
  }

  {
    iggy3d::EditableRoomDocument document;
    document.floors.push_back(floor("floor_1", 0.0F, 0.0F));
    document.floors.push_back(floor("floor_2", 2.0F, 0.0F));
    const auto report = iggy3d::buildProductRoomGeometryOptimizationReport(&document);
    ok = expect(report.optimizedFloorRectCount == 2,
                "non-adjacent floors do not merge") &&
         ok;
    ok = expect(report.drawCountAvoided == 0, "non-adjacent draw avoided") && ok;
  }

  {
    iggy3d::EditableRoomDocument document;
    document.floors.push_back(floor("floor_1", 0.0F, 0.0F));
    document.floors.push_back(floor("floor_2", 1.0F, 0.0F, "painted_floor"));
    const auto report = iggy3d::buildProductRoomGeometryOptimizationReport(&document);
    ok = expect(report.optimizedFloorRectCount == 2,
                "mismatched floor material does not merge") &&
         ok;
    ok = expect(report.triangleCountAvoided == 0,
                "mismatched floor triangle avoided") &&
         ok;
  }

  {
    iggy3d::EditableRoomDocument document;
    document.walls.push_back(wall("wall_1", 0.0F, 0.0F, 1.0F, 0.0F));
    document.walls.push_back(wall("wall_2", 1.0F, 0.0F, 2.0F, 0.0F));
    document.walls.push_back(wall("wall_3", 2.0F, 0.0F, 3.0F, 0.0F));
    const auto report = iggy3d::buildProductRoomGeometryOptimizationReport(&document);
    ok = expect(report.sourceWallCount == 3, "wall run source count") && ok;
    ok = expect(report.naiveDrawCount == 3, "wall run naive draw count") && ok;
    ok = expect(report.naiveTriangleCount == 36,
                "wall run naive triangle count") &&
         ok;
    ok = expect(report.optimizedWallRunCount == 1,
                "wall run optimized count") &&
         ok;
    ok = expect(report.optimizedDrawCount == 1,
                "wall run optimized draw count") &&
         ok;
    ok = expect(report.optimizedTriangleCount == 12,
                "wall run optimized triangle count") &&
         ok;
    ok = expect(report.wallRunMergeCandidateCount == 2,
                "wall run merge candidate count") &&
         ok;
    ok = expect(report.drawCountAvoided == 2, "wall run draw avoided") && ok;
    ok = expect(report.triangleCountAvoided == 24,
                "wall run triangle avoided") &&
         ok;
  }

  {
    iggy3d::EditableRoomDocument document;
    document.walls.push_back(wall("wall_1", 0.0F, 0.0F, 1.0F, 0.0F));
    document.walls.push_back(wall("wall_2", 1.0F, 0.0F, 1.0F, 1.0F));
    const auto report = iggy3d::buildProductRoomGeometryOptimizationReport(&document);
    ok = expect(report.optimizedWallRunCount == 2,
                "perpendicular walls do not merge") &&
         ok;
    ok = expect(report.drawCountAvoided == 0, "perpendicular draw avoided") && ok;
  }

  {
    iggy3d::EditableRoomDocument document;
    document.walls.push_back(wall("wall_1", 0.0F, 0.0F, 1.0F, 0.0F));
    document.walls.push_back(wall("wall_2", 1.0F, 0.0F, 2.0F, 0.0F, "painted_wall"));
    const auto report = iggy3d::buildProductRoomGeometryOptimizationReport(&document);
    ok = expect(report.optimizedWallRunCount == 2,
                "mismatched wall material does not merge") &&
         ok;
    ok = expect(report.triangleCountAvoided == 0,
                "mismatched wall triangle avoided") &&
         ok;
  }

  {
    iggy3d::EditableRoomDocument document;
    document.floors.push_back(floor("floor_1", 0.25F, 0.0F));
    document.walls.push_back(wall("wall_1", 0.0F, 0.0F, 1.0F, 1.0F));
    const auto report = iggy3d::buildProductRoomGeometryOptimizationReport(&document);
    ok = expect(report.optimizedFloorRectCount == 1,
                "invalid grid floor remains unmerged") &&
         ok;
    ok = expect(report.optimizedWallRunCount == 1,
                "non-axis wall remains unmerged") &&
         ok;
    ok = expect(report.drawCountAvoided == 0,
                "invalid geometry does not produce negative draw savings") &&
         ok;
    ok = expect(report.triangleCountAvoided == 0,
                "invalid geometry does not produce negative triangle savings") &&
         ok;
  }

  {
    iggy3d::EditableRoomDocument document;
    document.floors.push_back(floor("floor_1", 0.0F, 0.0F));
    document.floors.push_back(floor("floor_2", 1.0F, 0.0F));
    const std::size_t floorCountBefore = document.floors.size();
    const std::string firstId = document.floors.front().id;
    const float firstX = document.floors.front().centerMeters.x;
    (void)iggy3d::buildProductRoomGeometryOptimizationReport(&document);
    ok = expect(document.floors.size() == floorCountBefore,
                "report does not mutate floor count") &&
         ok;
    ok = expect(document.floors.front().id == firstId,
                "report does not mutate floor id") &&
         ok;
    ok = expect(document.floors.front().centerMeters.x == firstX,
                "report does not mutate floor position") &&
         ok;
  }

  return ok ? 0 : 1;
}
