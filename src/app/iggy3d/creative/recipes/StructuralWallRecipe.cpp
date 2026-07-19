#include "app/iggy3d/creative/recipes/StructuralWallRecipe.hpp"

#include "app/iggy3d/creative/Geometry.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace iggy3d::creative {
namespace {

constexpr double kGeometryEpsilon = 1.0e-9;

struct OrderedOpening {
  const CreativeStructuralWallOpeningRequest* request = nullptr;
  std::size_t sourceIndex = 0U;
  double minimum = 0.0;
  double maximum = 0.0;
  double insertHeight = 0.0;
  double insertWidth = 0.0;
  double insertThickness = 0.0;
};

[[nodiscard]] bool near(double lhs, double rhs) noexcept {
  return std::abs(lhs - rhs) <= kGeometryEpsilon;
}

[[nodiscard]] bool positiveFinite(double value) noexcept {
  return std::isfinite(value) && value > kGeometryEpsilon;
}

[[nodiscard]] bool nonNegativeFinite(double value) noexcept {
  return std::isfinite(value) && value >= 0.0;
}

[[nodiscard]] bool validBounds(CreativeBounds bounds) noexcept {
  const CreativeBoundsMetrics metrics = measureCreativeBounds(bounds);
  return metrics.valid && isPositiveCreativeVec3(metrics.size);
}

void reject(CreativeStructuralWallRecipeResult& result,
            CreativeStructuralWallRecipeStatus status,
            std::string_view reasonCode, std::size_t failedOpeningIndex = 0U) {
  result.accepted = false;
  result.status = status;
  result.frame = {};
  result.fullHeightSpans.clear();
  result.planarSolidPieces.clear();
  result.openings.clear();
  result.failedOpeningIndex = failedOpeningIndex;
  result.reasonCode = reasonCode;
}

[[nodiscard]] bool makeFrame(const CreativeStructuralWallRecipeRequest& request,
                             CreativeStructuralWallFrame& frame) noexcept {
  frame.start = request.start;
  frame.end = request.end;
  frame.baseYMeters = request.start.y;
  frame.heightMeters = request.heightMeters;
  frame.thicknessMeters = request.thicknessMeters;

  if (near(request.start.z, request.end.z) &&
      !near(request.start.x, request.end.x)) {
    const double delta = request.end.x - request.start.x;
    frame.axis = CreativeStructuralWallAxis::X;
    frame.tangent = {delta > 0.0 ? 1.0 : -1.0, 0.0, 0.0};
    frame.normal = {0.0, 0.0, 1.0};
    frame.lengthMeters = std::abs(delta);
    frame.bounds = {{std::min(request.start.x, request.end.x), request.start.y,
                     request.start.z - request.thicknessMeters * 0.5},
                    {std::max(request.start.x, request.end.x),
                     request.start.y + request.heightMeters,
                     request.start.z + request.thicknessMeters * 0.5}};
    return validBounds(frame.bounds);
  }
  if (near(request.start.x, request.end.x) &&
      !near(request.start.z, request.end.z)) {
    const double delta = request.end.z - request.start.z;
    frame.axis = CreativeStructuralWallAxis::Z;
    frame.tangent = {0.0, 0.0, delta > 0.0 ? 1.0 : -1.0};
    frame.normal = {1.0, 0.0, 0.0};
    frame.lengthMeters = std::abs(delta);
    frame.bounds = {{request.start.x - request.thicknessMeters * 0.5,
                     request.start.y, std::min(request.start.z, request.end.z)},
                    {request.start.x + request.thicknessMeters * 0.5,
                     request.start.y + request.heightMeters,
                     std::max(request.start.z, request.end.z)}};
    return validBounds(frame.bounds);
  }
  return false;
}

[[nodiscard]] CreativeBounds spanBounds(
    const CreativeStructuralWallFrame& frame, double minimumOffset,
    double maximumOffset, double bottom, double top,
    double thickness) noexcept {
  if (frame.axis == CreativeStructuralWallAxis::X) {
    const double first = frame.start.x + frame.tangent.x * minimumOffset;
    const double second = frame.start.x + frame.tangent.x * maximumOffset;
    return {{std::min(first, second), frame.baseYMeters + bottom,
             frame.start.z - thickness * 0.5},
            {std::max(first, second), frame.baseYMeters + top,
             frame.start.z + thickness * 0.5}};
  }
  const double first = frame.start.z + frame.tangent.z * minimumOffset;
  const double second = frame.start.z + frame.tangent.z * maximumOffset;
  return {{frame.start.x - thickness * 0.5, frame.baseYMeters + bottom,
           std::min(first, second)},
          {frame.start.x + thickness * 0.5, frame.baseYMeters + top,
           std::max(first, second)}};
}

[[nodiscard]] bool poseUsesStart(
    CreativeStructuralWallOpeningPose pose) noexcept {
  return pose ==
             CreativeStructuralWallOpeningPose::OpenFromStartNegativeNormal ||
         pose == CreativeStructuralWallOpeningPose::OpenFromStartPositiveNormal;
}

[[nodiscard]] double poseNormalSign(
    CreativeStructuralWallOpeningPose pose) noexcept {
  return pose == CreativeStructuralWallOpeningPose::
                         OpenFromStartNegativeNormal ||
                 pose == CreativeStructuralWallOpeningPose::
                             OpenFromEndNegativeNormal
             ? -1.0
             : 1.0;
}

[[nodiscard]] CreativeBounds openInsertBounds(
    const CreativeStructuralWallFrame& frame,
    const OrderedOpening& opening) noexcept {
  const auto pose = opening.request->pose;
  const double hingeOffset =
      poseUsesStart(pose) ? opening.minimum + opening.insertThickness * 0.5
                          : opening.maximum - opening.insertThickness * 0.5;
  if (frame.axis == CreativeStructuralWallAxis::X) {
    const double hinge = frame.start.x + frame.tangent.x * hingeOffset;
    const double normalEnd =
        frame.start.z + poseNormalSign(pose) * opening.insertWidth;
    return {{hinge - opening.insertThickness * 0.5,
             frame.baseYMeters + opening.request->insertBottomMeters,
             std::min(frame.start.z, normalEnd)},
            {hinge + opening.insertThickness * 0.5,
             frame.baseYMeters + opening.request->insertBottomMeters +
                 opening.insertHeight,
             std::max(frame.start.z, normalEnd)}};
  }
  const double hinge = frame.start.z + frame.tangent.z * hingeOffset;
  const double normalEnd =
      frame.start.x + poseNormalSign(pose) * opening.insertWidth;
  return {{std::min(frame.start.x, normalEnd),
           frame.baseYMeters + opening.request->insertBottomMeters,
           hinge - opening.insertThickness * 0.5},
          {std::max(frame.start.x, normalEnd),
           frame.baseYMeters + opening.request->insertBottomMeters +
               opening.insertHeight,
           hinge + opening.insertThickness * 0.5}};
}

[[nodiscard]] bool resolveOpening(
    const CreativeStructuralWallRecipeRequest& wall,
    const CreativeStructuralWallFrame& frame,
    const CreativeStructuralWallOpeningRequest& request,
    std::size_t sourceIndex, OrderedOpening& opening) noexcept {
  opening.request = &request;
  opening.sourceIndex = sourceIndex;
  if (!isCreativeStructuralWallOpeningPoseValid(request.pose) ||
      !positiveFinite(request.widthMeters) ||
      !nonNegativeFinite(request.centerOffsetMeters) ||
      !nonNegativeFinite(request.cutoutBottomMeters) ||
      !positiveFinite(request.cutoutHeightMeters)) {
    return false;
  }

  opening.minimum = request.centerOffsetMeters - request.widthMeters * 0.5;
  opening.maximum = request.centerOffsetMeters + request.widthMeters * 0.5;
  const double cutoutTop =
      request.cutoutBottomMeters + request.cutoutHeightMeters;
  if (!std::isfinite(opening.minimum) || !std::isfinite(opening.maximum) ||
      !std::isfinite(cutoutTop) ||
      opening.minimum <=
          wall.minimumOpeningEdgeClearanceMeters + kGeometryEpsilon ||
      opening.maximum >= frame.lengthMeters -
                             wall.minimumOpeningEdgeClearanceMeters -
                             kGeometryEpsilon ||
      cutoutTop > frame.heightMeters + kGeometryEpsilon) {
    return false;
  }
  if (!request.includeInsert) {
    return true;
  }

  if (!nonNegativeFinite(request.insertBottomMeters) ||
      !std::isfinite(request.insertHeightMeters) ||
      !std::isfinite(request.insertWidthMeters) ||
      !std::isfinite(request.insertThicknessMeters) ||
      request.insertHeightMeters < 0.0 || request.insertWidthMeters < 0.0 ||
      request.insertThicknessMeters < 0.0) {
    return false;
  }
  opening.insertHeight = request.insertHeightMeters == 0.0
                             ? request.cutoutHeightMeters
                             : request.insertHeightMeters;
  opening.insertWidth = request.insertWidthMeters == 0.0
                            ? request.widthMeters
                            : request.insertWidthMeters;
  opening.insertThickness = request.insertThicknessMeters == 0.0
                                ? frame.thicknessMeters
                                : request.insertThicknessMeters;
  const double insertTop = request.insertBottomMeters + opening.insertHeight;
  return positiveFinite(opening.insertHeight) &&
         positiveFinite(opening.insertWidth) &&
         positiveFinite(opening.insertThickness) && std::isfinite(insertTop) &&
         opening.insertWidth <= request.widthMeters + kGeometryEpsilon &&
         insertTop <= cutoutTop + kGeometryEpsilon &&
         request.insertBottomMeters + kGeometryEpsilon >=
             request.cutoutBottomMeters;
}

[[nodiscard]] double cutoutTop(const OrderedOpening& opening) noexcept {
  return opening.request->cutoutBottomMeters +
         opening.request->cutoutHeightMeters;
}

[[nodiscard]] bool intervalsSeparated(double firstMinimum,
                                      double firstMaximum,
                                      double secondMinimum,
                                      double secondMaximum,
                                      double separation) noexcept {
  return firstMaximum + separation + kGeometryEpsilon < secondMinimum ||
         secondMaximum + separation + kGeometryEpsilon < firstMinimum;
}

[[nodiscard]] bool buildOpeningPlan(
    const CreativeStructuralWallFrame& frame,
    const OrderedOpening& orderedOpening,
    CreativeStructuralWallOpeningPlan& plan) noexcept {
  const CreativeStructuralWallOpeningRequest& opening =
      *orderedOpening.request;
  plan.sourceIndex = orderedOpening.sourceIndex;
  plan.minimumOffsetMeters = orderedOpening.minimum;
  plan.maximumOffsetMeters = orderedOpening.maximum;
  plan.cutoutBounds =
      spanBounds(frame, orderedOpening.minimum, orderedOpening.maximum,
                 opening.cutoutBottomMeters, cutoutTop(orderedOpening),
                 frame.thicknessMeters);
  if (!validBounds(plan.cutoutBounds)) {
    return false;
  }

  if (opening.cutoutBottomMeters > kGeometryEpsilon) {
    plan.hasSill = true;
    plan.sillBounds =
        spanBounds(frame, orderedOpening.minimum, orderedOpening.maximum, 0.0,
                   opening.cutoutBottomMeters, frame.thicknessMeters);
  }
  const double openingTop = cutoutTop(orderedOpening);
  if (openingTop < frame.heightMeters - kGeometryEpsilon) {
    plan.hasLintel = true;
    plan.lintelBounds =
        spanBounds(frame, orderedOpening.minimum, orderedOpening.maximum,
                   openingTop, frame.heightMeters, frame.thicknessMeters);
  }
  if (opening.includeInsert) {
    plan.hasInsert = true;
    plan.insertBounds = spanBounds(
        frame, opening.centerOffsetMeters - orderedOpening.insertWidth * 0.5,
        opening.centerOffsetMeters + orderedOpening.insertWidth * 0.5,
        opening.insertBottomMeters,
        opening.insertBottomMeters + orderedOpening.insertHeight,
        orderedOpening.insertThickness);
    if (opening.pose != CreativeStructuralWallOpeningPose::Closed) {
      plan.insertBounds = openInsertBounds(frame, orderedOpening);
    }
  }
  return (!plan.hasSill || validBounds(plan.sillBounds)) &&
         (!plan.hasLintel || validBounds(plan.lintelBounds)) &&
         (!plan.hasInsert || validBounds(plan.insertBounds));
}

void sortUniqueBreaks(std::vector<double>& breaks) {
  std::sort(breaks.begin(), breaks.end());
  breaks.erase(
      std::unique(breaks.begin(), breaks.end(),
                  [](double lhs, double rhs) { return near(lhs, rhs); }),
      breaks.end());
}

[[nodiscard]] bool appendPlanarSolidPieces(
    const CreativeStructuralWallFrame& frame,
    const std::vector<OrderedOpening>& openings,
    std::vector<CreativeBounds>& pieces) {
  std::vector<double> horizontalBreaks{0.0, frame.lengthMeters};
  std::vector<double> verticalBreaks{0.0, frame.heightMeters};
  horizontalBreaks.reserve(openings.size() * 2U + 2U);
  verticalBreaks.reserve(openings.size() * 2U + 2U);
  for (const OrderedOpening& opening : openings) {
    horizontalBreaks.push_back(opening.minimum);
    horizontalBreaks.push_back(opening.maximum);
    verticalBreaks.push_back(opening.request->cutoutBottomMeters);
    verticalBreaks.push_back(cutoutTop(opening));
  }
  sortUniqueBreaks(horizontalBreaks);
  sortUniqueBreaks(verticalBreaks);

  const auto cellIsCutout = [&](double horizontal, double vertical) {
    return std::any_of(
        openings.begin(), openings.end(), [&](const OrderedOpening& opening) {
          return horizontal > opening.minimum - kGeometryEpsilon &&
                 horizontal < opening.maximum + kGeometryEpsilon &&
                 vertical > opening.request->cutoutBottomMeters -
                                kGeometryEpsilon &&
                 vertical < cutoutTop(opening) + kGeometryEpsilon;
        });
  };

  for (std::size_t verticalIndex = 1U;
       verticalIndex < verticalBreaks.size(); ++verticalIndex) {
    const double bottom = verticalBreaks[verticalIndex - 1U];
    const double top = verticalBreaks[verticalIndex];
    if (top - bottom <= kGeometryEpsilon) {
      continue;
    }
    bool runActive = false;
    double runMinimum = 0.0;
    for (std::size_t horizontalIndex = 1U;
         horizontalIndex < horizontalBreaks.size(); ++horizontalIndex) {
      const double minimum = horizontalBreaks[horizontalIndex - 1U];
      const double maximum = horizontalBreaks[horizontalIndex];
      if (maximum - minimum <= kGeometryEpsilon) {
        continue;
      }
      const bool solid = !cellIsCutout((minimum + maximum) * 0.5,
                                       (bottom + top) * 0.5);
      if (solid && !runActive) {
        runActive = true;
        runMinimum = minimum;
      }
      const bool runEnds = runActive &&
                           (!solid ||
                            horizontalIndex + 1U == horizontalBreaks.size());
      if (!runEnds) {
        continue;
      }
      const double runMaximum = solid ? maximum : minimum;
      const CreativeBounds piece =
          spanBounds(frame, runMinimum, runMaximum, bottom, top,
                     frame.thicknessMeters);
      if (!validBounds(piece)) {
        pieces.clear();
        return false;
      }
      pieces.push_back(piece);
      runActive = false;
    }
  }
  return !pieces.empty();
}

}  // namespace

bool isCreativeStructuralWallOpeningPoseValid(
    CreativeStructuralWallOpeningPose pose) noexcept {
  switch (pose) {
    case CreativeStructuralWallOpeningPose::Closed:
    case CreativeStructuralWallOpeningPose::OpenFromStartNegativeNormal:
    case CreativeStructuralWallOpeningPose::OpenFromStartPositiveNormal:
    case CreativeStructuralWallOpeningPose::OpenFromEndNegativeNormal:
    case CreativeStructuralWallOpeningPose::OpenFromEndPositiveNormal:
      return true;
  }
  return false;
}

CreativeStructuralWallRecipeResult planCreativeStructuralWall(
    const CreativeStructuralWallRecipeRequest& request) {
  CreativeStructuralWallRecipeResult result;
  if (!isFiniteCreativeVec3(request.start) ||
      !isFiniteCreativeVec3(request.end) ||
      !positiveFinite(request.heightMeters) ||
      !positiveFinite(request.thicknessMeters) ||
      !nonNegativeFinite(request.minimumOpeningEdgeClearanceMeters) ||
      !nonNegativeFinite(request.minimumOpeningSeparationMeters)) {
    reject(result, CreativeStructuralWallRecipeStatus::InvalidWall,
           "creative_structural_wall_invalid");
    return result;
  }
  if (!near(request.start.y, request.end.y)) {
    reject(result, CreativeStructuralWallRecipeStatus::UnsupportedOrientation,
           "creative_structural_wall_orientation_unsupported");
    return result;
  }
  if (!makeFrame(request, result.frame)) {
    const bool cardinal = (near(request.start.z, request.end.z) &&
                           !near(request.start.x, request.end.x)) ||
                          (near(request.start.x, request.end.x) &&
                           !near(request.start.z, request.end.z));
    reject(result,
           cardinal
               ? CreativeStructuralWallRecipeStatus::UnrepresentableGeometry
               : CreativeStructuralWallRecipeStatus::UnsupportedOrientation,
           cardinal ? "creative_structural_wall_geometry_unrepresentable"
                    : "creative_structural_wall_orientation_unsupported");
    return result;
  }
  if (request.minimumOpeningEdgeClearanceMeters * 2.0 >=
      result.frame.lengthMeters - kGeometryEpsilon) {
    reject(result, CreativeStructuralWallRecipeStatus::InvalidWall,
           "creative_structural_wall_clearance_invalid");
    return result;
  }

  std::vector<OrderedOpening> ordered;
  ordered.reserve(request.openings.size());
  for (std::size_t index = 0U; index < request.openings.size(); ++index) {
    OrderedOpening opening;
    if (!resolveOpening(request, result.frame, request.openings[index], index,
                        opening)) {
      reject(result, CreativeStructuralWallRecipeStatus::InvalidOpening,
             "creative_structural_wall_opening_invalid", index);
      return result;
    }
    ordered.push_back(opening);
  }
  std::sort(ordered.begin(), ordered.end(),
            [](const OrderedOpening& lhs, const OrderedOpening& rhs) {
              if (!near(lhs.minimum, rhs.minimum)) {
                return lhs.minimum < rhs.minimum;
              }
              if (lhs.request->sortKey != rhs.request->sortKey) {
                return lhs.request->sortKey < rhs.request->sortKey;
              }
              return lhs.sourceIndex < rhs.sourceIndex;
            });
  bool requiresPlanarPartition = false;
  for (std::size_t first = 0U; first < ordered.size(); ++first) {
    for (std::size_t second = first + 1U; second < ordered.size(); ++second) {
      const bool horizontallySeparated = intervalsSeparated(
          ordered[first].minimum, ordered[first].maximum,
          ordered[second].minimum, ordered[second].maximum,
          request.minimumOpeningSeparationMeters);
      if (horizontallySeparated) {
        continue;
      }
      const bool verticallySeparated = intervalsSeparated(
          ordered[first].request->cutoutBottomMeters, cutoutTop(ordered[first]),
          ordered[second].request->cutoutBottomMeters,
          cutoutTop(ordered[second]),
          request.minimumOpeningSeparationMeters);
      if (!verticallySeparated) {
        reject(result,
               CreativeStructuralWallRecipeStatus::OverlappingOpenings,
               "creative_structural_wall_openings_overlap",
               ordered[second].sourceIndex);
        return result;
      }
      requiresPlanarPartition = true;
    }
  }

  result.openings.reserve(ordered.size());
  for (const OrderedOpening& orderedOpening : ordered) {
    CreativeStructuralWallOpeningPlan plan;
    if (!buildOpeningPlan(result.frame, orderedOpening, plan)) {
      reject(result, CreativeStructuralWallRecipeStatus::InvalidOpening,
             "creative_structural_wall_opening_geometry_unrepresentable",
             orderedOpening.sourceIndex);
      return result;
    }
    result.openings.push_back(plan);
  }

  if (requiresPlanarPartition) {
    if (!appendPlanarSolidPieces(result.frame, ordered,
                                 result.planarSolidPieces)) {
      reject(result,
             CreativeStructuralWallRecipeStatus::UnrepresentableGeometry,
             "creative_structural_wall_geometry_unrepresentable");
      return result;
    }
    result.accepted = true;
    result.status = CreativeStructuralWallRecipeStatus::Ready;
    result.failedOpeningIndex = 0U;
    result.reasonCode = "creative_structural_wall_ready";
    return result;
  }

  result.fullHeightSpans.reserve(ordered.size() + 1U);
  double cursor = 0.0;
  for (std::size_t openingIndex = 0U; openingIndex < ordered.size();
       ++openingIndex) {
    const OrderedOpening& orderedOpening = ordered[openingIndex];
    const CreativeBounds span =
        spanBounds(result.frame, cursor, orderedOpening.minimum, 0.0,
                   result.frame.heightMeters, result.frame.thicknessMeters);
    if (!validBounds(span)) {
      reject(result, CreativeStructuralWallRecipeStatus::InvalidOpening,
             "creative_structural_wall_opening_geometry_unrepresentable",
             orderedOpening.sourceIndex);
      return result;
    }
    result.fullHeightSpans.push_back(span);
    cursor = orderedOpening.maximum;
  }

  const CreativeBounds finalSpan =
      spanBounds(result.frame, cursor, result.frame.lengthMeters, 0.0,
                 result.frame.heightMeters, result.frame.thicknessMeters);
  if (!validBounds(finalSpan)) {
    reject(result, CreativeStructuralWallRecipeStatus::UnrepresentableGeometry,
           "creative_structural_wall_geometry_unrepresentable");
    return result;
  }
  result.fullHeightSpans.push_back(finalSpan);
  result.accepted = true;
  result.status = CreativeStructuralWallRecipeStatus::Ready;
  result.failedOpeningIndex = 0U;
  result.reasonCode = "creative_structural_wall_ready";
  return result;
}

}  // namespace iggy3d::creative
