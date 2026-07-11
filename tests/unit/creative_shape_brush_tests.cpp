#include "app/iggy3d/creative/tools/ShapeBrush.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
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

cr::CreativeShapeBrushPlanReceipt plan(
    cr::CreativeShapeBrushKind kind,
    cr::CreativeGridCoord3 first,
    cr::CreativeGridCoord3 second,
    bool hollow = false,
    cr::CreativeShapeBrushAxis axis = cr::CreativeShapeBrushAxis::Y) {
  cr::CreativeShapeBrushPlanRequest request;
  request.kind = kind;
  request.axis = axis;
  request.firstCell = first;
  request.secondCell = second;
  request.hollow = hollow;
  return cr::planCreativeShapeBrush(request);
}

bool sameCell(cr::CreativeGridCoord3 lhs,
              cr::CreativeGridCoord3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool containsCell(const cr::CreativeShapeBrushPlanReceipt& receipt,
                  cr::CreativeGridCoord3 cell) {
  return std::any_of(receipt.cells.begin(), receipt.cells.end(),
                     [cell](cr::CreativeGridCoord3 candidate) {
                       return sameCell(candidate, cell);
                     });
}

bool boxParityAndHollowBoundary() {
  const auto filled = plan(cr::CreativeShapeBrushKind::Box,
                           {0, 0, 0}, {2, 2, 2});
  const auto hollow = plan(cr::CreativeShapeBrushKind::Box,
                           {0, 0, 0}, {2, 2, 2}, true);
  return expect(filled.accepted && filled.generatedCellCount == 27U,
                "box fills canonical 3 cube") &&
         expect(filled.candidateCellCount == 27U,
                "box candidate count exact") &&
         expect(hollow.accepted && hollow.generatedCellCount == 26U,
                "box hollow removes center") &&
         expect(!containsCell(hollow, {1, 1, 1}),
                "box hollow excludes center") &&
         expect(sameCell(filled.cells.front(), {0, 0, 0}) &&
                    sameCell(filled.cells.back(), {2, 2, 2}),
                "box ordering is canonical");
}

bool lineIsDeterministicAndEndpointInclusive() {
  const auto forward = plan(cr::CreativeShapeBrushKind::Line,
                            {-2, 1, 0}, {3, 3, 1});
  const auto reversed = plan(cr::CreativeShapeBrushKind::Line,
                             {3, 3, 1}, {-2, 1, 0});
  bool same = forward.cells.size() == reversed.cells.size();
  for (std::size_t index = 0; same && index < forward.cells.size(); ++index) {
    same = sameCell(forward.cells[index], reversed.cells[index]);
  }
  return expect(forward.accepted && forward.generatedCellCount == 6U,
                "line uses longest-axis step count") &&
         expect(sameCell(forward.cells.front(), {-2, 1, 0}) &&
                    sameCell(forward.cells.back(), {3, 3, 1}),
                "line includes canonical endpoints") &&
         expect(same, "reversed line has identical ordered plan") &&
         expect(plan(cr::CreativeShapeBrushKind::Line,
                     {4, -3, 2}, {4, -3, 2}, true)
                        .generatedCellCount == 1U,
                "degenerate hollow line remains one cell");
}

bool ellipsoidUsesSymmetricCellCenters() {
  const auto filled = plan(cr::CreativeShapeBrushKind::Ellipsoid,
                           {0, 0, 0}, {2, 2, 2});
  const auto hollow = plan(cr::CreativeShapeBrushKind::Ellipsoid,
                           {0, 0, 0}, {2, 2, 2}, true);
  return expect(filled.accepted && filled.generatedCellCount == 19U,
                "three-cell ellipsoid has symmetric lattice count") &&
         expect(!containsCell(filled, {0, 0, 0}) &&
                    !containsCell(filled, {2, 2, 2}),
                "ellipsoid excludes cube corners") &&
         expect(containsCell(filled, {1, 1, 1}),
                "ellipsoid includes center") &&
         expect(hollow.generatedCellCount == 18U &&
                    !containsCell(hollow, {1, 1, 1}),
                "ellipsoid hollow removes six-neighbor interior");
}

bool cylinderAxisAndDiskBehavior() {
  const auto alongY = plan(cr::CreativeShapeBrushKind::Cylinder,
                           {0, 0, 0}, {4, 2, 4}, false,
                           cr::CreativeShapeBrushAxis::Y);
  const auto alongX = plan(cr::CreativeShapeBrushKind::Cylinder,
                           {0, 0, 0}, {2, 4, 4}, false,
                           cr::CreativeShapeBrushAxis::X);
  const auto disk = plan(cr::CreativeShapeBrushKind::Cylinder,
                         {0, 2, 0}, {4, 2, 4}, false,
                         cr::CreativeShapeBrushAxis::Y);
  return expect(alongY.accepted && alongY.generatedCellCount == 63U,
                "vertical cylinder extrudes 21-cell disk") &&
         expect(alongX.accepted && alongX.generatedCellCount == 63U,
                "x cylinder rotates radial plane") &&
         expect(disk.accepted && disk.generatedCellCount == 21U,
                "one-cell cylinder is a disk") &&
         expect(!containsCell(disk, {0, 2, 0}) &&
                    containsCell(disk, {2, 2, 2}),
                "disk follows ellipse boundary");
}

bool limitsAndInvalidEnumsFailClosed() {
  cr::CreativeShapeBrushPlanRequest request;
  request.kind = cr::CreativeShapeBrushKind::Ellipsoid;
  request.firstCell = {0, 0, 0};
  request.secondCell = {20, 20, 20};
  request.maxCandidateCellCount = 1'000U;
  const auto candidateRejected = cr::planCreativeShapeBrush(request);

  request.kind = cr::CreativeShapeBrushKind::Box;
  request.secondCell = {2, 2, 2};
  request.maxCandidateCellCount = 27U;
  request.maxGeneratedCellCount = 26U;
  const auto generatedRejected = cr::planCreativeShapeBrush(request);

  request = {};
  request.kind = static_cast<cr::CreativeShapeBrushKind>(255U);
  const auto kindRejected = cr::planCreativeShapeBrush(request);
  request = {};
  request.axis = static_cast<cr::CreativeShapeBrushAxis>(255U);
  const auto axisRejected = cr::planCreativeShapeBrush(request);

  cr::CreativeShapeBrushPlanRequest extreme;
  extreme.kind = cr::CreativeShapeBrushKind::Box;
  extreme.firstCell = {std::numeric_limits<std::int32_t>::min(), 0, 0};
  extreme.secondCell = {std::numeric_limits<std::int32_t>::max(), 0, 0};
  const auto extremeRejected = cr::planCreativeShapeBrush(extreme);

  return expect(!candidateRejected.accepted &&
                    candidateRejected.status ==
                        cr::CreativeShapeBrushPlanStatus::CandidateLimitExceeded,
                "candidate limit rejects before enumeration") &&
         expect(!generatedRejected.accepted && generatedRejected.cells.empty() &&
                    generatedRejected.status ==
                        cr::CreativeShapeBrushPlanStatus::GeneratedLimitExceeded,
                "generated limit clears partial plan") &&
         expect(!kindRejected.accepted && !axisRejected.accepted,
                "invalid enums fail closed") &&
         expect(!extremeRejected.accepted && extremeRejected.cells.empty(),
                "extreme bounds reject without iteration");
}

}  // namespace

int main() {
  const bool ok = boxParityAndHollowBoundary() &&
                  lineIsDeterministicAndEndpointInclusive() &&
                  ellipsoidUsesSymmetricCellCenters() &&
                  cylinderAxisAndDiskBehavior() &&
                  limitsAndInvalidEnumsFailClosed();
  if (!ok) {
    return 1;
  }
  std::cout << "creative_shape_brush_tests: PASS\n";
  return 0;
}
