#include "app/iggy3d/creative/world/WorldLayoutPlanHitTest.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeWorldLayoutPlanPrimitive primitive(
    cr::CreativeWorldLayoutPlanPrimitiveKind kind) {
  cr::CreativeWorldLayoutPlanPrimitive value;
  value.kind = kind;
  value.layer = cr::CreativeWorldLayoutPlanLayer::Active;
  value.role = cr::CreativeWorldLayoutPlanRole::Object;
  return value;
}

bool invalidInputFailsClosed() {
  auto point = primitive(cr::CreativeWorldLayoutPlanPrimitiveKind::Point);
  point.pointCount = 1U;
  point.points[0] = {0.0, 0.0};
  const auto invalidPoint = cr::hitTestCreativeWorldLayoutPlanPrimitive(
      point, {std::numeric_limits<double>::quiet_NaN(), 0.0}, 0.1);
  const auto invalidTolerance = cr::hitTestCreativeWorldLayoutPlanPrimitive(
      point, {0.0, 0.0}, -0.1);
  point.pointCount = 2U;
  const auto invalidPrimitive = cr::hitTestCreativeWorldLayoutPlanPrimitive(
      point, {0.0, 0.0}, 0.1);
  return expect(
      !invalidPoint.hit &&
          invalidPoint.status ==
              cr::CreativeWorldLayoutPlanHitTestStatus::InvalidRequest &&
          !invalidTolerance.hit &&
          invalidTolerance.status ==
              cr::CreativeWorldLayoutPlanHitTestStatus::InvalidRequest &&
          !invalidPrimitive.hit &&
          invalidPrimitive.status ==
              cr::CreativeWorldLayoutPlanHitTestStatus::InvalidPrimitive,
      "invalid requests and primitive layouts fail closed");
}

bool segmentsHonorWidthAndTolerance() {
  auto segment = primitive(cr::CreativeWorldLayoutPlanPrimitiveKind::Segment);
  segment.pointCount = 2U;
  segment.points[0] = {0.0, 0.0};
  segment.points[1] = {4.0, 0.0};
  const auto center = cr::hitTestCreativeWorldLayoutPlanPrimitive(
      segment, {2.0, 0.0}, 0.0);
  const auto tolerance = cr::hitTestCreativeWorldLayoutPlanPrimitive(
      segment, {2.0, 0.2}, 0.2);
  const auto miss = cr::hitTestCreativeWorldLayoutPlanPrimitive(
      segment, {2.0, 0.21}, 0.2);
  segment.widthCells = 2.0;
  const auto width = cr::hitTestCreativeWorldLayoutPlanPrimitive(
      segment, {2.0, 0.9}, 0.0);
  return expect(center.hit && tolerance.hit && !miss.hit && width.hit,
                "segments include authored width plus caller tolerance");
}

bool polygonsSelectInteriorAndBoundary() {
  auto polygon = primitive(cr::CreativeWorldLayoutPlanPrimitiveKind::Polygon);
  polygon.pointCount = 4U;
  polygon.points = {{{0.0, 0.0}, {4.0, 0.0}, {4.0, 3.0}, {0.0, 3.0}}};
  const auto interior = cr::hitTestCreativeWorldLayoutPlanPrimitive(
      polygon, {2.0, 1.0}, 0.0);
  const auto boundary = cr::hitTestCreativeWorldLayoutPlanPrimitive(
      polygon, {4.1, 1.0}, 0.1);
  const auto miss = cr::hitTestCreativeWorldLayoutPlanPrimitive(
      polygon, {4.11, 1.0}, 0.1);
  return expect(interior.hit && interior.distanceCells == 0.0 && boundary.hit &&
                    !miss.hit,
                "polygon interiors and tolerance-expanded boundaries hit");
}

bool circlesSelectTheirFilledFootprint() {
  auto circle = primitive(cr::CreativeWorldLayoutPlanPrimitiveKind::Circle);
  circle.pointCount = 1U;
  circle.points[0] = {2.0, 2.0};
  circle.radiusCells = 2.0;
  const auto interior = cr::hitTestCreativeWorldLayoutPlanPrimitive(
      circle, {2.0, 2.0}, 0.0);
  const auto boundary = cr::hitTestCreativeWorldLayoutPlanPrimitive(
      circle, {4.1, 2.0}, 0.1);
  const auto miss = cr::hitTestCreativeWorldLayoutPlanPrimitive(
      circle, {4.11, 2.0}, 0.1);
  return expect(interior.hit && boundary.hit && !miss.hit,
                "circle interiors and tolerance-expanded rims hit");
}

bool arcsRespectDirectedSweepAndEndpoints() {
  constexpr double kPi = 3.14159265358979323846;
  auto arc = primitive(cr::CreativeWorldLayoutPlanPrimitiveKind::Arc);
  arc.pointCount = 1U;
  arc.points[0] = {0.0, 0.0};
  arc.radiusCells = 2.0;
  arc.startRadians = 0.0;
  arc.sweepRadians = kPi * 0.5;
  const auto positive = cr::hitTestCreativeWorldLayoutPlanPrimitive(
      arc, {std::sqrt(2.0), std::sqrt(2.0)}, 0.01);
  const auto opposite = cr::hitTestCreativeWorldLayoutPlanPrimitive(
      arc, {-std::sqrt(2.0), std::sqrt(2.0)}, 0.01);
  const auto endpoint = cr::hitTestCreativeWorldLayoutPlanPrimitive(
      arc, {2.08, 0.0}, 0.1);
  arc.sweepRadians = -kPi * 0.5;
  const auto negative = cr::hitTestCreativeWorldLayoutPlanPrimitive(
      arc, {std::sqrt(2.0), -std::sqrt(2.0)}, 0.01);
  return expect(positive.hit && !opposite.hit && endpoint.hit && negative.hit,
                "arcs honor signed sweeps and endpoint tolerance");
}

bool pointsUseCallerTolerance() {
  auto point = primitive(cr::CreativeWorldLayoutPlanPrimitiveKind::Point);
  point.pointCount = 1U;
  point.points[0] = {3.0, 4.0};
  const auto hit = cr::hitTestCreativeWorldLayoutPlanPrimitive(
      point, {3.3, 4.4}, 0.5);
  const auto miss = cr::hitTestCreativeWorldLayoutPlanPrimitive(
      point, {3.3, 4.41}, 0.5);
  return expect(hit.hit && !miss.hit,
                "point symbols use zoom-derived caller tolerance");
}

bool regionSelectionDistinguishesWindowAndCrossing() {
  auto segment = primitive(cr::CreativeWorldLayoutPlanPrimitiveKind::Segment);
  segment.pointCount = 2U;
  segment.points[0] = {0.0, 0.0};
  segment.points[1] = {4.0, 0.0};
  segment.widthCells = 0.5;
  const auto segmentWindow =
      cr::selectCreativeWorldLayoutPlanPrimitiveInRegion(
          segment, {-1.0, -1.0}, {5.0, 1.0},
          cr::CreativeWorldLayoutPlanRegionMode::Window);
  const auto segmentPartialWindow =
      cr::selectCreativeWorldLayoutPlanPrimitiveInRegion(
          segment, {1.0, -1.0}, {3.0, 1.0},
          cr::CreativeWorldLayoutPlanRegionMode::Window);
  const auto segmentCrossing =
      cr::selectCreativeWorldLayoutPlanPrimitiveInRegion(
          segment, {1.0, -1.0}, {3.0, 1.0},
          cr::CreativeWorldLayoutPlanRegionMode::Crossing);

  auto polygon = primitive(cr::CreativeWorldLayoutPlanPrimitiveKind::Polygon);
  polygon.pointCount = 4U;
  polygon.points = {{{0.0, 0.0}, {4.0, 0.0}, {4.0, 4.0}, {0.0, 4.0}}};
  const auto regionInsidePolygon =
      cr::selectCreativeWorldLayoutPlanPrimitiveInRegion(
          polygon, {1.0, 1.0}, {2.0, 2.0},
          cr::CreativeWorldLayoutPlanRegionMode::Crossing);
  const auto polygonNotEnclosed =
      cr::selectCreativeWorldLayoutPlanPrimitiveInRegion(
          polygon, {1.0, 1.0}, {2.0, 2.0},
          cr::CreativeWorldLayoutPlanRegionMode::Window);

  auto circle = primitive(cr::CreativeWorldLayoutPlanPrimitiveKind::Circle);
  circle.pointCount = 1U;
  circle.points[0] = {2.0, 2.0};
  circle.radiusCells = 1.0;
  const auto circleWindow =
      cr::selectCreativeWorldLayoutPlanPrimitiveInRegion(
          circle, {0.5, 0.5}, {3.5, 3.5},
          cr::CreativeWorldLayoutPlanRegionMode::Window);
  const auto circleCrossing =
      cr::selectCreativeWorldLayoutPlanPrimitiveInRegion(
          circle, {2.9, 1.9}, {3.2, 2.1},
          cr::CreativeWorldLayoutPlanRegionMode::Crossing);

  constexpr double kPi = 3.14159265358979323846;
  auto arc = primitive(cr::CreativeWorldLayoutPlanPrimitiveKind::Arc);
  arc.pointCount = 1U;
  arc.points[0] = {0.0, 0.0};
  arc.radiusCells = 2.0;
  arc.startRadians = 0.0;
  arc.sweepRadians = kPi * 0.5;
  const auto arcWindow = cr::selectCreativeWorldLayoutPlanPrimitiveInRegion(
      arc, {-0.1, -0.1}, {2.1, 2.1},
      cr::CreativeWorldLayoutPlanRegionMode::Window);
  const auto arcCrossing = cr::selectCreativeWorldLayoutPlanPrimitiveInRegion(
      arc, {1.3, 1.3}, {1.6, 1.6},
      cr::CreativeWorldLayoutPlanRegionMode::Crossing);
  const auto arcOpposite = cr::selectCreativeWorldLayoutPlanPrimitiveInRegion(
      arc, {-1.6, 1.3}, {-1.3, 1.6},
      cr::CreativeWorldLayoutPlanRegionMode::Crossing);

  auto point = primitive(cr::CreativeWorldLayoutPlanPrimitiveKind::Point);
  point.pointCount = 1U;
  point.points[0] = {7.0, 8.0};
  const auto pointCrossing =
      cr::selectCreativeWorldLayoutPlanPrimitiveInRegion(
          point, {6.9, 7.9}, {7.1, 8.1},
          cr::CreativeWorldLayoutPlanRegionMode::Crossing);
  const auto invalidMode =
      cr::selectCreativeWorldLayoutPlanPrimitiveInRegion(
          point, {6.9, 7.9}, {7.1, 8.1},
          cr::CreativeWorldLayoutPlanRegionMode::Count);

  return expect(
      segmentWindow.hit && !segmentPartialWindow.hit && segmentCrossing.hit &&
          regionInsidePolygon.hit && !polygonNotEnclosed.hit &&
          circleWindow.hit && circleCrossing.hit && arcWindow.hit &&
          arcCrossing.hit && !arcOpposite.hit && pointCrossing.hit &&
          !invalidMode.hit &&
          invalidMode.status ==
              cr::CreativeWorldLayoutPlanHitTestStatus::InvalidRequest,
      "window encloses complete geometry while crossing accepts intersection");
}

}  // namespace

int main() {
  bool ok = true;
  ok = invalidInputFailsClosed() && ok;
  ok = segmentsHonorWidthAndTolerance() && ok;
  ok = polygonsSelectInteriorAndBoundary() && ok;
  ok = circlesSelectTheirFilledFootprint() && ok;
  ok = arcsRespectDirectedSweepAndEndpoints() && ok;
  ok = pointsUseCallerTolerance() && ok;
  ok = regionSelectionDistinguishesWindowAndCrossing() && ok;
  return ok ? 0 : 1;
}
