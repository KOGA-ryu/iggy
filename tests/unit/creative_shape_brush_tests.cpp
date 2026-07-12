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

bool containsStampCell(const cr::CreativeMaterialBrushStampPlan& plan,
                       cr::CreativeGridCoord3 cell) {
  return std::any_of(plan.generatedCells().begin(), plan.generatedCells().end(),
                     [cell](cr::CreativeGridCoord3 candidate) {
                       return sameCell(candidate, cell);
                     });
}

bool containsSymmetryCell(
    const cr::CreativeMaterialBrushSymmetryPlan& plan,
    cr::CreativeGridCoord3 cell) {
  return std::any_of(plan.generatedCells().begin(), plan.generatedCells().end(),
                     [cell](cr::CreativeGridCoord3 candidate) {
                       return sameCell(candidate, cell);
                     });
}

cr::CreativeMaterialBrushStampPlan stamp(
    cr::CreativeMaterialBrushShape shape,
    cr::CreativeMaterialBrushSize size,
    cr::CreativeGridCoord3 center = {},
    cr::CreativeAxis3 axis = cr::CreativeAxis3::Y,
    cr::CreativeMaterialBrushGuide guide =
        cr::CreativeMaterialBrushGuide::Free,
    cr::CreativeMaterialBrushFill fill =
        cr::CreativeMaterialBrushFill::Solid) {
  cr::CreativeMaterialBrushStampRequest request;
  request.shape = shape;
  request.size = size;
  request.centerCell = center;
  request.axis = axis;
  request.guide = guide;
  request.fill = fill;
  return cr::planCreativeMaterialBrushStamp(request);
}

bool materialBrushStampsAreBoundedAndCanonical() {
  const auto one = stamp(cr::CreativeMaterialBrushShape::Cube,
                         cr::CreativeMaterialBrushSize::OneCell,
                         {4, -2, 7});
  const auto cube = stamp(cr::CreativeMaterialBrushShape::Cube,
                          cr::CreativeMaterialBrushSize::FiveCells);
  const auto sphere = stamp(cr::CreativeMaterialBrushShape::Sphere,
                            cr::CreativeMaterialBrushSize::FiveCells);
  const auto cylinderX = stamp(cr::CreativeMaterialBrushShape::Cylinder,
                               cr::CreativeMaterialBrushSize::FiveCells, {},
                               cr::CreativeAxis3::X);
  const auto cylinderY = stamp(cr::CreativeMaterialBrushShape::Cylinder,
                               cr::CreativeMaterialBrushSize::FiveCells);
  const auto cylinderZ = stamp(cr::CreativeMaterialBrushShape::Cylinder,
                               cr::CreativeMaterialBrushSize::FiveCells, {},
                               cr::CreativeAxis3::Z);
  return expect(one.accepted && one.cellCount == 1U &&
                    sameCell(one.cells[0], {4, -2, 7}),
                "one-cell material brush contains only its center") &&
         expect(cube.accepted && cube.cellCount == 125U &&
                    sameCell(cube.minCell, {-2, -2, -2}) &&
                    sameCell(cube.maxCell, {2, 2, 2}) &&
                    sameCell(cube.cells.front(), {-2, -2, -2}) &&
                    sameCell(cube.cells[cube.cellCount - 1U], {2, 2, 2}),
                "largest cube fills bounded canonical z-y-x storage") &&
         expect(sphere.accepted && sphere.cellCount == 33U &&
                    containsStampCell(sphere, {}) &&
                    !containsStampCell(sphere, {2, 2, 2}),
                "five-cell sphere uses integer radial inclusion") &&
         expect(cylinderX.accepted && cylinderX.cellCount == 65U &&
                    containsStampCell(cylinderX, {2, 0, 0}) &&
                    containsStampCell(cylinderX, {2, 0, 2}) &&
                    !containsStampCell(cylinderX, {0, 2, 2}),
                "five-cell X cylinder extrudes its YZ disk") &&
         expect(cylinderY.accepted && cylinderY.cellCount == 65U &&
                    containsStampCell(cylinderY, {0, 2, 0}) &&
                    containsStampCell(cylinderY, {2, 2, 0}) &&
                    !containsStampCell(cylinderY, {2, 0, 2}),
                "five-cell Y cylinder extrudes its XZ disk") &&
         expect(cylinderZ.accepted && cylinderZ.cellCount == 65U &&
                    containsStampCell(cylinderZ, {0, 0, 2}) &&
                    containsStampCell(cylinderZ, {2, 0, 2}) &&
                    !containsStampCell(cylinderZ, {2, 2, 0}),
                "five-cell Z cylinder extrudes its XY disk");
}

bool materialBrushInvalidInputsFailClosed() {
  const auto invalidShape = stamp(
      static_cast<cr::CreativeMaterialBrushShape>(255U),
      cr::CreativeMaterialBrushSize::OneCell);
  const auto invalidSize = stamp(
      cr::CreativeMaterialBrushShape::Cube,
      static_cast<cr::CreativeMaterialBrushSize>(255U));
  const auto invalidAxis = stamp(
      cr::CreativeMaterialBrushShape::Cylinder,
      cr::CreativeMaterialBrushSize::ThreeCells, {},
      cr::CreativeAxis3::Count);
  const auto invalidGuide = stamp(
      cr::CreativeMaterialBrushShape::Cube,
      cr::CreativeMaterialBrushSize::ThreeCells, {}, cr::CreativeAxis3::Y,
      cr::CreativeMaterialBrushGuide::Count);
  const auto invalidFill = stamp(
      cr::CreativeMaterialBrushShape::Cube,
      cr::CreativeMaterialBrushSize::ThreeCells, {}, cr::CreativeAxis3::Y,
      cr::CreativeMaterialBrushGuide::Free,
      cr::CreativeMaterialBrushFill::Count);
  const auto overflow = stamp(
      cr::CreativeMaterialBrushShape::Cube,
      cr::CreativeMaterialBrushSize::FiveCells,
      {std::numeric_limits<std::int32_t>::max(), 0, 0});
  return expect(!invalidShape.accepted && invalidShape.cellCount == 0U &&
                    invalidShape.status ==
                        cr::CreativeMaterialBrushStampStatus::InvalidShape,
                "invalid material brush shape fails closed") &&
         expect(!invalidSize.accepted && invalidSize.cellCount == 0U &&
                    invalidSize.status ==
                        cr::CreativeMaterialBrushStampStatus::InvalidSize,
                "invalid material brush size fails closed") &&
         expect(!invalidAxis.accepted && invalidAxis.cellCount == 0U &&
                    invalidAxis.status ==
                        cr::CreativeMaterialBrushStampStatus::InvalidAxis,
                "invalid material brush axis fails closed") &&
         expect(!invalidGuide.accepted && invalidGuide.cellCount == 0U &&
                    invalidGuide.status ==
                        cr::CreativeMaterialBrushStampStatus::InvalidGuide,
                "invalid material brush guide fails closed") &&
         expect(!invalidFill.accepted && invalidFill.cellCount == 0U &&
                    invalidFill.status ==
                        cr::CreativeMaterialBrushStampStatus::InvalidFill,
                "invalid material brush fill fails closed") &&
         expect(!overflow.accepted && overflow.cellCount == 0U &&
                    overflow.status ==
                        cr::CreativeMaterialBrushStampStatus::CoordinateOverflow,
                "material brush coordinate overflow rejects before enumeration");
}

bool materialBrushShellsRetainSixNeighborBoundaries() {
  const auto one = stamp(
      cr::CreativeMaterialBrushShape::Sphere,
      cr::CreativeMaterialBrushSize::OneCell, {}, cr::CreativeAxis3::Y,
      cr::CreativeMaterialBrushGuide::Free,
      cr::CreativeMaterialBrushFill::Shell);
  const auto cube = stamp(
      cr::CreativeMaterialBrushShape::Cube,
      cr::CreativeMaterialBrushSize::FiveCells, {}, cr::CreativeAxis3::Y,
      cr::CreativeMaterialBrushGuide::Free,
      cr::CreativeMaterialBrushFill::Shell);
  const auto sphere = stamp(
      cr::CreativeMaterialBrushShape::Sphere,
      cr::CreativeMaterialBrushSize::FiveCells, {}, cr::CreativeAxis3::Y,
      cr::CreativeMaterialBrushGuide::Free,
      cr::CreativeMaterialBrushFill::Shell);
  const auto cylinder = stamp(
      cr::CreativeMaterialBrushShape::Cylinder,
      cr::CreativeMaterialBrushSize::FiveCells, {}, cr::CreativeAxis3::Y,
      cr::CreativeMaterialBrushGuide::Free,
      cr::CreativeMaterialBrushFill::Shell);
  const auto plane = stamp(
      cr::CreativeMaterialBrushShape::Cube,
      cr::CreativeMaterialBrushSize::FiveCells, {}, cr::CreativeAxis3::Y,
      cr::CreativeMaterialBrushGuide::PlaneY,
      cr::CreativeMaterialBrushFill::Shell);

  return expect(one.accepted && one.cellCount == 1U &&
                    containsStampCell(one, {}),
                "one-cell shell retains its only voxel") &&
         expect(cube.accepted && cube.cellCount == 98U &&
                    !containsStampCell(cube, {}) &&
                    containsStampCell(cube, {2, 2, 2}) &&
                    sameCell(cube.cells.front(), {-2, -2, -2}) &&
                    sameCell(cube.cells[cube.cellCount - 1U], {2, 2, 2}),
                "cube shell removes its canonical three-cubed interior") &&
         expect(sphere.accepted && sphere.cellCount == 26U &&
                    !containsStampCell(sphere, {}) &&
                    !containsStampCell(sphere, {1, 0, 0}) &&
                    containsStampCell(sphere, {2, 0, 0}),
                "sphere shell retains only six-neighbor boundary cells") &&
         expect(cylinder.accepted && cylinder.cellCount == 50U &&
                    !containsStampCell(cylinder, {}) &&
                    !containsStampCell(cylinder, {0, 1, 0}) &&
                    containsStampCell(cylinder, {0, 2, 0}) &&
                    containsStampCell(cylinder, {2, 0, 0}),
                "cylinder shell keeps end caps and radial boundary") &&
         expect(plane.accepted && plane.cellCount == 25U &&
                    containsStampCell(plane, {}) &&
                    cr::toString(cr::CreativeMaterialBrushFill::Solid) ==
                        "SOLID" &&
                    cr::toString(cr::CreativeMaterialBrushFill::Shell) ==
                        "SHELL",
                "one-cell-thick guide planes remain complete shells");
}

bool materialBrushGuidesFlattenAndConstrain() {
  const auto cubeY = stamp(
      cr::CreativeMaterialBrushShape::Cube,
      cr::CreativeMaterialBrushSize::ThreeCells, {}, cr::CreativeAxis3::Y,
      cr::CreativeMaterialBrushGuide::PlaneY);
  const auto sphereZ = stamp(
      cr::CreativeMaterialBrushShape::Sphere,
      cr::CreativeMaterialBrushSize::ThreeCells, {}, cr::CreativeAxis3::Y,
      cr::CreativeMaterialBrushGuide::PlaneZ);
  const auto cylinderDisk = stamp(
      cr::CreativeMaterialBrushShape::Cylinder,
      cr::CreativeMaterialBrushSize::ThreeCells, {}, cr::CreativeAxis3::X,
      cr::CreativeMaterialBrushGuide::PlaneX);
  const auto lineStamp = stamp(
      cr::CreativeMaterialBrushShape::Cube,
      cr::CreativeMaterialBrushSize::ThreeCells, {}, cr::CreativeAxis3::Y,
      cr::CreativeMaterialBrushGuide::LineX);
  const auto boundaryPlane = stamp(
      cr::CreativeMaterialBrushShape::Cube,
      cr::CreativeMaterialBrushSize::ThreeCells,
      {std::numeric_limits<std::int32_t>::max(), 0, 0},
      cr::CreativeAxis3::Y, cr::CreativeMaterialBrushGuide::PlaneX);

  cr::CreativeGridCoord3 constrained{99, 99, 99};
  const bool constrainedY = cr::guideCreativeMaterialBrushCenter(
      cr::CreativeMaterialBrushGuide::PlaneY, {3, 4, 5}, {8, 9, 10},
      constrained);
  cr::CreativeGridCoord3 lineX{};
  const bool constrainedLineX = cr::guideCreativeMaterialBrushCenter(
      cr::CreativeMaterialBrushGuide::LineX, {3, 4, 5}, {8, 9, 10},
      lineX);
  cr::CreativeGridCoord3 lineY{};
  const bool constrainedLineY = cr::guideCreativeMaterialBrushCenter(
      cr::CreativeMaterialBrushGuide::LineY, {3, 4, 5}, {8, 9, 10},
      lineY);
  cr::CreativeGridCoord3 lineZ{};
  const bool constrainedLineZ = cr::guideCreativeMaterialBrushCenter(
      cr::CreativeMaterialBrushGuide::LineZ, {3, 4, 5}, {8, 9, 10},
      lineZ);
  cr::CreativeGridCoord3 unchanged{7, 8, 9};
  const bool invalid = cr::guideCreativeMaterialBrushCenter(
      cr::CreativeMaterialBrushGuide::Count, {}, {}, unchanged);
  cr::CreativeAxis3 lineAxis = cr::CreativeAxis3::Z;
  const bool hasLineAxis = cr::creativeMaterialBrushLineAxis(
      cr::CreativeMaterialBrushGuide::LineX, lineAxis);
  cr::CreativeAxis3 planeAxis = cr::CreativeAxis3::Z;
  const bool planeHasLineAxis = cr::creativeMaterialBrushLineAxis(
      cr::CreativeMaterialBrushGuide::PlaneX, planeAxis);

  return expect(cubeY.accepted && cubeY.cellCount == 9U &&
                    cubeY.minCell.y == 0 && cubeY.maxCell.y == 0,
                "Y plane flattens a cube to one exact XZ layer") &&
         expect(sphereZ.accepted && sphereZ.cellCount == 5U &&
                    sphereZ.minCell.z == 0 && sphereZ.maxCell.z == 0,
                "Z plane flattens a sphere to its XY disk") &&
         expect(cylinderDisk.accepted && cylinderDisk.cellCount == 5U &&
                    cylinderDisk.minCell.x == 0 &&
                    cylinderDisk.maxCell.x == 0,
                "matching cylinder and plane axes produce one disk") &&
         expect(lineStamp.accepted && lineStamp.cellCount == 27U,
                "line guide retains the complete stamp geometry") &&
         expect(boundaryPlane.accepted && boundaryPlane.cellCount == 9U &&
                    boundaryPlane.minCell.x ==
                        std::numeric_limits<std::int32_t>::max() &&
                    boundaryPlane.maxCell.x ==
                        std::numeric_limits<std::int32_t>::max(),
                "flattened axis does not fabricate coordinate overflow") &&
         expect(constrainedY && sameCell(constrained, {8, 4, 10}),
                "gesture center keeps the anchored Y coordinate") &&
         expect(constrainedLineX && sameCell(lineX, {8, 4, 5}) &&
                    hasLineAxis && lineAxis == cr::CreativeAxis3::X,
                "X line keeps anchored YZ and reports its guide axis") &&
         expect(constrainedLineY && sameCell(lineY, {3, 9, 5}),
                "Y line keeps anchored XZ") &&
         expect(constrainedLineZ && sameCell(lineZ, {3, 4, 10}),
                "Z line keeps anchored XY") &&
         expect(!planeHasLineAxis && planeAxis == cr::CreativeAxis3::Z,
                "plane guides do not fabricate a line axis") &&
         expect(!invalid && sameCell(unchanged, {7, 8, 9}),
                "invalid plane fails without modifying output") &&
         expect(cr::toString(cr::CreativeMaterialBrushGuide::Free) == "FREE" &&
                    cr::toString(cr::CreativeMaterialBrushGuide::LineX) ==
                        "LINE X" &&
                    cr::toString(cr::CreativeMaterialBrushGuide::LineY) ==
                        "LINE Y" &&
                    cr::toString(cr::CreativeMaterialBrushGuide::LineZ) ==
                        "LINE Z" &&
                    cr::toString(cr::CreativeMaterialBrushGuide::PlaneX) ==
                        "PLANE X" &&
                    cr::toString(cr::CreativeMaterialBrushGuide::PlaneY) ==
                        "PLANE Y" &&
                    cr::toString(cr::CreativeMaterialBrushGuide::PlaneZ) ==
                        "PLANE Z",
                "guide labels state their world-space constraint");
}

bool materialBrushMasksPartitionOccupancy() {
  return expect(cr::creativeMaterialBrushMaskAllows(
                    cr::CreativeMaterialBrushMask::AddOnly, false) &&
                    !cr::creativeMaterialBrushMaskAllows(
                        cr::CreativeMaterialBrushMask::AddOnly, true),
                "add-only mask admits only empty cells") &&
         expect(!cr::creativeMaterialBrushMaskAllows(
                    cr::CreativeMaterialBrushMask::Replace, false) &&
                    cr::creativeMaterialBrushMaskAllows(
                        cr::CreativeMaterialBrushMask::Replace, true),
                "replace mask admits only occupied cells") &&
         expect(cr::creativeMaterialBrushMaskAllows(
                    cr::CreativeMaterialBrushMask::Overwrite, false) &&
                    cr::creativeMaterialBrushMaskAllows(
                        cr::CreativeMaterialBrushMask::Overwrite, true),
                "overwrite mask admits both occupancy states") &&
         expect(!cr::creativeMaterialBrushMaskAllows(
                    cr::CreativeMaterialBrushMask::Count, false) &&
                    cr::toString(cr::CreativeMaterialBrushMask::AddOnly) ==
                        "ADD ONLY" &&
                    cr::toString(cr::CreativeMaterialBrushMask::Replace) ==
                        "REPLACE" &&
                    cr::toString(cr::CreativeMaterialBrushMask::Overwrite) ==
                        "OVERWRITE",
                "invalid mask fails closed and labels remain explicit");
}

bool materialBrushPathUsesExactBoundedSupercover() {
  const cr::CreativeMaterialBrushPathPlan degenerate =
      cr::planCreativeMaterialBrushPath({{4, -2, 7}, {4, -2, 7}});
  const cr::CreativeMaterialBrushPathPlan axial =
      cr::planCreativeMaterialBrushPath({{0, 0, 0}, {4, 0, 0}});
  const cr::CreativeMaterialBrushPathPlan diagonal =
      cr::planCreativeMaterialBrushPath({{0, 0, 0}, {2, 2, 0}});
  const cr::CreativeMaterialBrushPathPlan corner =
      cr::planCreativeMaterialBrushPath({{0, 0, 0}, {1, 1, 1}});

  constexpr std::array expectedDiagonal{
      cr::CreativeGridCoord3{0, 0, 0}, cr::CreativeGridCoord3{1, 0, 0},
      cr::CreativeGridCoord3{0, 1, 0}, cr::CreativeGridCoord3{1, 1, 0},
      cr::CreativeGridCoord3{2, 1, 0}, cr::CreativeGridCoord3{1, 2, 0},
      cr::CreativeGridCoord3{2, 2, 0}};
  bool diagonalOrder = diagonal.centerCount == expectedDiagonal.size();
  for (std::size_t index = 0U;
       diagonalOrder && index < expectedDiagonal.size(); ++index) {
    diagonalOrder = sameCell(diagonal.centers[index], expectedDiagonal[index]);
  }
  return expect(degenerate.accepted && degenerate.centerCount == 1U &&
                    sameCell(degenerate.centers[0], {4, -2, 7}),
                "degenerate material brush path emits one center") &&
         expect(axial.accepted && axial.centerCount == 5U &&
                    sameCell(axial.centers.front(), {0, 0, 0}) &&
                    sameCell(axial.centers[axial.centerCount - 1U], {4, 0, 0}),
                "axial material brush path includes every center") &&
         expect(diagonal.accepted && diagonalOrder,
                "diagonal path includes deterministic edge-crossing neighbors") &&
         expect(corner.accepted && corner.centerCount == 8U &&
                    sameCell(corner.centers[corner.centerCount - 1U],
                             {1, 1, 1}),
                "corner crossing includes all eight touched cells");
}

bool materialBrushPathLimitsFailBeforePartialOutput() {
  cr::CreativeMaterialBrushPathRequest exactRequest;
  exactRequest.fromCell = {0, 0, 0};
  exactRequest.toCell = {255, 0, 0};
  const cr::CreativeMaterialBrushPathPlan exact =
      cr::planCreativeMaterialBrushPath(exactRequest);

  cr::CreativeMaterialBrushPathRequest oversizedRequest = exactRequest;
  oversizedRequest.toCell = {256, 0, 0};
  const cr::CreativeMaterialBrushPathPlan oversized =
      cr::planCreativeMaterialBrushPath(oversizedRequest);

  cr::CreativeMaterialBrushPathRequest smallLimitRequest;
  smallLimitRequest.fromCell = {0, 0, 0};
  smallLimitRequest.toCell = {4, 0, 0};
  smallLimitRequest.maxCenterCount = 4U;
  const cr::CreativeMaterialBrushPathPlan smallLimit =
      cr::planCreativeMaterialBrushPath(smallLimitRequest);

  cr::CreativeMaterialBrushPathRequest invalidLimitRequest;
  invalidLimitRequest.maxCenterCount = 0U;
  const cr::CreativeMaterialBrushPathPlan invalidLimit =
      cr::planCreativeMaterialBrushPath(invalidLimitRequest);

  cr::CreativeMaterialBrushPathRequest extremeRequest;
  extremeRequest.fromCell = {std::numeric_limits<std::int32_t>::min(), 0, 0};
  extremeRequest.toCell = {std::numeric_limits<std::int32_t>::max(), 0, 0};
  const cr::CreativeMaterialBrushPathPlan extreme =
      cr::planCreativeMaterialBrushPath(extremeRequest);

  return expect(exact.accepted && exact.centerCount == 256U,
                "path accepts the exact fixed center capacity") &&
         expect(!oversized.accepted && oversized.centerCount == 0U &&
                    oversized.status ==
                        cr::CreativeMaterialBrushPathStatus::CapacityExceeded,
                "path rejects one center beyond fixed capacity") &&
         expect(!smallLimit.accepted && smallLimit.centerCount == 0U &&
                    smallLimit.status ==
                        cr::CreativeMaterialBrushPathStatus::CapacityExceeded,
                "caller path limit clears partial output") &&
         expect(!invalidLimit.accepted && invalidLimit.centerCount == 0U &&
                    invalidLimit.status ==
                        cr::CreativeMaterialBrushPathStatus::InvalidLimit,
                "zero path limit fails closed") &&
         expect(!extreme.accepted && extreme.centerCount == 0U,
                "extreme path rejects before coordinate traversal");
}

bool materialBrushSymmetryIsBoundedAndDeterministic() {
  constexpr std::array source{cr::CreativeGridCoord3{1, 0, 0},
                              cr::CreativeGridCoord3{2, 0, 0}};
  const cr::CreativeMaterialBrushSymmetryPlan mirrorX =
      cr::planCreativeMaterialBrushSymmetry(
          {cr::CreativeMaterialBrushSymmetry::MirrorX, {}, source});
  constexpr std::array oneCell{cr::CreativeGridCoord3{1, 0, 2}};
  const cr::CreativeMaterialBrushSymmetryPlan mirrorXZ =
      cr::planCreativeMaterialBrushSymmetry(
          {cr::CreativeMaterialBrushSymmetry::MirrorXZ, {}, oneCell});
  constexpr std::array onPlane{cr::CreativeGridCoord3{0, 3, 0}};
  const cr::CreativeMaterialBrushSymmetryPlan deduplicated =
      cr::planCreativeMaterialBrushSymmetry(
          {cr::CreativeMaterialBrushSymmetry::MirrorX, {}, onPlane});

  return expect(mirrorX.accepted && mirrorX.cellCount == 4U &&
                    sameCell(mirrorX.cells[0], {1, 0, 0}) &&
                    sameCell(mirrorX.cells[1], {2, 0, 0}) &&
                    sameCell(mirrorX.cells[2], {-1, 0, 0}) &&
                    sameCell(mirrorX.cells[3], {-2, 0, 0}) &&
                    !mirrorX.cellIsMirrored(0U) &&
                    !mirrorX.cellIsMirrored(1U) &&
                    mirrorX.cellIsMirrored(2U) &&
                    mirrorX.cellIsMirrored(3U) &&
                    sameCell(mirrorX.minCell, {-2, 0, 0}) &&
                    sameCell(mirrorX.maxCell, {2, 0, 0}),
                "mirror X emits originals first and mirrored cells second") &&
         expect(mirrorXZ.accepted && mirrorXZ.cellCount == 4U &&
                    containsSymmetryCell(mirrorXZ, {1, 0, 2}) &&
                    containsSymmetryCell(mirrorXZ, {-1, 0, 2}) &&
                    containsSymmetryCell(mirrorXZ, {1, 0, -2}) &&
                    containsSymmetryCell(mirrorXZ, {-1, 0, -2}),
                "mirror XZ emits a bounded four-way set") &&
         expect(deduplicated.accepted && deduplicated.cellCount == 1U &&
                    !deduplicated.cellIsMirrored(0U),
                "pivot-plane overlap keeps the direct cell once");
}

bool materialBrushSymmetryFailuresClearPartialOutput() {
  constexpr std::array oneCell{cr::CreativeGridCoord3{1, 0, 1}};
  cr::CreativeMaterialBrushSymmetryRequest invalidRequest;
  invalidRequest.symmetry =
      static_cast<cr::CreativeMaterialBrushSymmetry>(255U);
  invalidRequest.sourceCells = oneCell;
  const auto invalid = cr::planCreativeMaterialBrushSymmetry(invalidRequest);

  cr::CreativeMaterialBrushSymmetryRequest emptyRequest;
  const auto empty = cr::planCreativeMaterialBrushSymmetry(emptyRequest);

  cr::CreativeMaterialBrushSymmetryRequest limitRequest;
  limitRequest.sourceCells = oneCell;
  limitRequest.maxCellCount = 0U;
  const auto invalidLimit =
      cr::planCreativeMaterialBrushSymmetry(limitRequest);

  cr::CreativeMaterialBrushSymmetryRequest capacityRequest;
  capacityRequest.symmetry = cr::CreativeMaterialBrushSymmetry::MirrorXZ;
  capacityRequest.sourceCells = oneCell;
  capacityRequest.maxCellCount = 3U;
  const auto capacity =
      cr::planCreativeMaterialBrushSymmetry(capacityRequest);

  cr::CreativeMaterialBrushStampRequest largeStampRequest;
  largeStampRequest.shape = cr::CreativeMaterialBrushShape::Cube;
  largeStampRequest.size = cr::CreativeMaterialBrushSize::FiveCells;
  largeStampRequest.centerCell = {10, 0, 10};
  const auto largeStamp =
      cr::planCreativeMaterialBrushStamp(largeStampRequest);
  cr::CreativeMaterialBrushSymmetryRequest largeSymmetryRequest;
  largeSymmetryRequest.symmetry =
      cr::CreativeMaterialBrushSymmetry::MirrorXZ;
  largeSymmetryRequest.sourceCells = largeStamp.generatedCells();
  const auto largeSymmetry =
      cr::planCreativeMaterialBrushSymmetry(largeSymmetryRequest);

  constexpr std::array overflowSource{
      cr::CreativeGridCoord3{std::numeric_limits<std::int32_t>::min(), 0, 0}};
  cr::CreativeMaterialBrushSymmetryRequest overflowRequest;
  overflowRequest.symmetry = cr::CreativeMaterialBrushSymmetry::MirrorX;
  overflowRequest.pivot = {std::numeric_limits<std::int32_t>::max(), 0, 0};
  overflowRequest.sourceCells = overflowSource;
  const auto overflow =
      cr::planCreativeMaterialBrushSymmetry(overflowRequest);

  return expect(!invalid.accepted && invalid.cellCount == 0U &&
                    invalid.status ==
                        cr::CreativeMaterialBrushSymmetryStatus::
                            InvalidSymmetry,
                "invalid symmetry fails closed") &&
         expect(!empty.accepted && empty.cellCount == 0U &&
                    empty.status ==
                        cr::CreativeMaterialBrushSymmetryStatus::EmptyInput,
                "empty symmetry input fails closed") &&
         expect(!invalidLimit.accepted && invalidLimit.cellCount == 0U &&
                    invalidLimit.status ==
                        cr::CreativeMaterialBrushSymmetryStatus::InvalidLimit,
                "invalid symmetry limit fails closed") &&
         expect(!capacity.accepted && capacity.cellCount == 0U &&
                    capacity.status ==
                        cr::CreativeMaterialBrushSymmetryStatus::
                            CapacityExceeded,
                "symmetry capacity clears partial output") &&
         expect(largeStamp.accepted && largeStamp.cellCount == 125U &&
                    !largeSymmetry.accepted &&
                    largeSymmetry.cellCount == 0U &&
                    largeSymmetry.status ==
                        cr::CreativeMaterialBrushSymmetryStatus::
                            CapacityExceeded,
                "five-cell four-way symmetry rejects before partial output") &&
         expect(!overflow.accepted && overflow.cellCount == 0U &&
                    overflow.status ==
                        cr::CreativeMaterialBrushSymmetryStatus::
                            CoordinateOverflow,
                "symmetry coordinate overflow clears partial output") &&
         expect(cr::toString(cr::CreativeMaterialBrushSymmetry::Off) ==
                        "OFF" &&
                    cr::toString(
                        cr::CreativeMaterialBrushSymmetry::MirrorX) ==
                        "MIRROR X" &&
                    cr::toString(
                        cr::CreativeMaterialBrushSymmetry::MirrorY) ==
                        "MIRROR Y" &&
                    cr::toString(
                        cr::CreativeMaterialBrushSymmetry::MirrorZ) ==
                        "MIRROR Z" &&
                    cr::toString(
                        cr::CreativeMaterialBrushSymmetry::MirrorXZ) ==
                        "MIRROR XZ",
                "symmetry labels expose their world axes");
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
  const bool ok = materialBrushStampsAreBoundedAndCanonical() &&
                  materialBrushInvalidInputsFailClosed() &&
                  materialBrushShellsRetainSixNeighborBoundaries() &&
                  materialBrushGuidesFlattenAndConstrain() &&
                  materialBrushMasksPartitionOccupancy() &&
                  materialBrushPathUsesExactBoundedSupercover() &&
                  materialBrushPathLimitsFailBeforePartialOutput() &&
                  materialBrushSymmetryIsBoundedAndDeterministic() &&
                  materialBrushSymmetryFailuresClearPartialOutput() &&
                  boxParityAndHollowBoundary() &&
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
