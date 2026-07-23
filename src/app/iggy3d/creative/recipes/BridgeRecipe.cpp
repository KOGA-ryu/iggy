#include "app/iggy3d/creative/recipes/BridgeRecipe.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/recipes/TerrainGradeAdapters.hpp"
#include "core/hash/StableHash.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

namespace iggy3d::creative {
namespace {

constexpr double kBridgeGeometryEpsilon = 1.0e-9;

void reject(CreativeBridgeRecipeReceipt& receipt,
            CreativeBridgeRecipeStatus status,
            std::string_view reasonCode) {
  receipt.status = status;
  receipt.reasonCode = reasonCode;
}

[[nodiscard]] bool positiveFinite(double value) noexcept {
  return std::isfinite(value) && value > 0.0;
}

[[nodiscard]] bool nonnegativeFinite(double value) noexcept {
  return std::isfinite(value) && value >= 0.0;
}

[[nodiscard]] bool validMaterial(CreativeStructuralMaterial value) noexcept {
  return value < CreativeStructuralMaterial::Count;
}

void appendTagOnce(std::vector<std::string>& tags, std::string tag) {
  if (!tag.empty() &&
      std::find(tags.begin(), tags.end(), tag) == tags.end()) {
    tags.push_back(std::move(tag));
  }
}

[[nodiscard]] bool validCrossingFrame(
    const CreativeWatercourseCrossingFrame& frame) noexcept {
  const double axisLength =
      std::hypot(frame.crossingAxis.x, frame.crossingAxis.z);
  return frame.id != kInvalidCreativeTerrainWatercourseCrossingId &&
         frame.sourcePointId != kInvalidCreativeTerrainPathSourcePointId &&
         isFiniteCreativeVec3(frame.centerMeters) &&
         isFiniteCreativeVec3(frame.crossingAxis) &&
         isFiniteCreativeVec3(frame.leftBankMeters) &&
         isFiniteCreativeVec3(frame.rightBankMeters) &&
         isFiniteCreativeVec3(frame.leftApproachMeters) &&
         isFiniteCreativeVec3(frame.rightApproachMeters) &&
         isFiniteCreativeVec3(frame.centerGrid) &&
         isFiniteCreativeVec3(frame.leftBankGrid) &&
         isFiniteCreativeVec3(frame.rightBankGrid) &&
         isFiniteCreativeVec3(frame.leftApproachGrid) &&
         isFiniteCreativeVec3(frame.rightApproachGrid) &&
         isFiniteCreativeVec3(frame.channelBedGrid) &&
         isFiniteCreativeVec3(frame.clearanceReferenceGrid) &&
         isFiniteCreativeVec3(frame.channelBedMeters) &&
         isFiniteCreativeVec3(frame.clearanceReferenceMeters) &&
         positiveFinite(frame.spanMeters) && positiveFinite(axisLength);
}

[[nodiscard]] bool appendOrientedBox(
    CreativeRecipePlan& plan,
    const CreativeBridgeRecipeRequest& request,
    CreativeObjectKind kind,
    CreativeVec3 center,
    CreativeVec3 size,
    double yaw,
    CreativeStructuralMaterial material,
    std::string stableKey,
    std::string name,
    std::string roleTag) {
  if (!isFiniteCreativeVec3(center) || !isPositiveCreativeVec3(size) ||
      !std::isfinite(yaw) || !validMaterial(material) || stableKey.empty() ||
      plan.objects.size() >= kCreativeBridgeGeneratedObjectCapacity) {
    return false;
  }
  CreativeDocumentCreateRequest create;
  create.kind = kind;
  create.name = std::move(name);
  const CreativeVec3 half{size.x * 0.5, size.y * 0.5, size.z * 0.5};
  create.bounds = {{center.x - half.x, center.y - half.y, center.z - half.z},
                   {center.x + half.x, center.y + half.y, center.z + half.z}};
  create.hasBoundsOverride = true;
  create.transform.position = center;
  create.transform.rotationEulerRadians.y = yaw;
  create.hasTransformOverride = true;
  create.visible = true;
  create.hasVisibleOverride = true;
  create.tags = request.tags;
  appendTagOnce(create.tags, "creative_bridge:generated");
  appendTagOnce(create.tags, std::move(roleTag));
  appendTagOnce(create.tags, creativeStructuralMaterialTag(material));

  CreativeRecipeObjectPlan object;
  object.createRequest = std::move(create);
  object.role = CreativeRecipeObjectRole::Generated;
  object.stableKey = std::move(stableKey);
  plan.objects.push_back(std::move(object));
  return true;
}

[[nodiscard]] bool roundedCoord(double value, std::int32_t& output) noexcept {
  if (!std::isfinite(value) ||
      value < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
      value > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
    return false;
  }
  const long long rounded = std::llround(value);
  if (rounded < std::numeric_limits<std::int32_t>::min() ||
      rounded > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  output = static_cast<std::int32_t>(rounded);
  return true;
}

[[nodiscard]] bool roundedHeight(double value,
                                 std::uint16_t& output) noexcept {
  if (!std::isfinite(value) ||
      value < static_cast<double>(kCreativeTerrainMinimumHeightCells) ||
      value > static_cast<double>(kCreativeTerrainMaximumHeightCells)) {
    return false;
  }
  const long long rounded = std::llround(value);
  if (rounded < kCreativeTerrainMinimumHeightCells ||
      rounded > kCreativeTerrainMaximumHeightCells) {
    return false;
  }
  output = static_cast<std::uint16_t>(rounded);
  return true;
}

[[nodiscard]] bool gradeEndpoint(CreativeVec3 value,
                                 CreativeTerrainCoord2& coord,
                                 std::uint16_t& height) noexcept {
  return roundedCoord(value.x, coord.x) && roundedCoord(value.z, coord.z) &&
         roundedHeight(value.y, height);
}

[[nodiscard]] double gradePermille(CreativeTerrainCoord2 from,
                                   CreativeTerrainCoord2 to,
                                   std::uint16_t fromHeight,
                                   std::uint16_t toHeight) noexcept {
  const double run = std::hypot(static_cast<double>(to.x) - from.x,
                                static_cast<double>(to.z) - from.z);
  if (!positiveFinite(run)) {
    return std::numeric_limits<double>::infinity();
  }
  return std::fabs(static_cast<double>(toHeight) - fromHeight) / run * 1000.0;
}

[[nodiscard]] bool planApproaches(
    CreativeBridgeRecipeResult& result,
    const CreativeBridgeRecipeRequest& request,
    double deckTopGrid) {
  CreativeTerrainCoord2 leftTerrain;
  CreativeTerrainCoord2 leftDeck;
  CreativeTerrainCoord2 rightDeck;
  CreativeTerrainCoord2 rightTerrain;
  std::uint16_t leftTerrainHeight = 0U;
  std::uint16_t leftDeckHeight = 0U;
  std::uint16_t rightDeckHeight = 0U;
  std::uint16_t rightTerrainHeight = 0U;
  CreativeVec3 leftDeckGrid = request.crossing.leftBankGrid;
  CreativeVec3 rightDeckGrid = request.crossing.rightBankGrid;
  leftDeckGrid.y = deckTopGrid;
  rightDeckGrid.y = deckTopGrid;
  if (!gradeEndpoint(request.crossing.leftApproachGrid, leftTerrain,
                     leftTerrainHeight) ||
      !gradeEndpoint(leftDeckGrid, leftDeck, leftDeckHeight) ||
      !gradeEndpoint(rightDeckGrid, rightDeck, rightDeckHeight) ||
      !gradeEndpoint(request.crossing.rightApproachGrid, rightTerrain,
                     rightTerrainHeight)) {
    return false;
  }

  const double leftGrade = gradePermille(
      leftTerrain, leftDeck, leftTerrainHeight, leftDeckHeight);
  const double rightGrade = gradePermille(
      rightDeck, rightTerrain, rightDeckHeight, rightTerrainHeight);
  result.receipt.maximumApproachGradePermille =
      std::max(leftGrade, rightGrade);
  if (!std::isfinite(result.receipt.maximumApproachGradePermille) ||
      result.receipt.maximumApproachGradePermille >
          request.source.settings.maximumApproachGradePermille +
              kBridgeGeometryEpsilon) {
    reject(result.receipt,
           CreativeBridgeRecipeStatus::ApproachGradeExceeded,
           "creative_bridge_recipe_approach_grade_exceeded");
    return false;
  }

  const double halfWidthEstimate =
      request.source.settings.deckWidthMeters /
      (request.gridCellSizeMeters * 2.0);
  if (!std::isfinite(halfWidthEstimate) || halfWidthEstimate < 0.5 ||
      halfWidthEstimate >=
          static_cast<double>(kCreativeTerrainGradeMaximumHalfWidthCells) +
              0.5) {
    return false;
  }
  const long long roundedHalfWidth = std::llround(halfWidthEstimate);
  CreativeTerrainGradePathSegmentRequest left;
  left.start = leftTerrain;
  left.end = leftDeck;
  left.startHeightCells = leftTerrainHeight;
  left.endHeightCells = leftDeckHeight;
  left.halfWidthCells = static_cast<std::uint16_t>(roundedHalfWidth);
  left.falloffCells = request.source.settings.approachFalloffCells;
  CreativeTerrainGradePathSegmentRequest right;
  right.start = rightDeck;
  right.end = rightTerrain;
  right.startHeightCells = rightDeckHeight;
  right.endHeightCells = rightTerrainHeight;
  right.halfWidthCells = left.halfWidthCells;
  right.falloffCells = request.source.settings.approachFalloffCells;
  const CreativeTerrainGradeAdapterPlan leftPlan =
      planCreativeTerrainGradePathSegment(left);
  const CreativeTerrainGradeAdapterPlan rightPlan =
      planCreativeTerrainGradePathSegment(right);
  if (!leftPlan.accepted || leftPlan.recipeCount != 1U ||
      !rightPlan.accepted || rightPlan.recipeCount != 1U) {
    return false;
  }
  result.approachGrades[0] = leftPlan.recipes[0];
  result.approachGrades[1] = rightPlan.recipes[0];
  result.approachGradeCount = 2U;
  result.receipt.approachGradeCount = 2U;
  return true;
}

[[nodiscard]] std::uint64_t fingerprintBridgeOutput(
    const CreativeBridgeRecipeResult& result) noexcept {
  StableHasher hasher;
  hasher.addString("creative_bridge_output_v1");
  hasher.addU64(result.structure.definitionFingerprint);
  hasher.addU64(result.approachGradeCount);
  for (std::size_t index = 0U; index < result.approachGradeCount; ++index) {
    const CreativeTerrainGradeRecipe& grade = result.approachGrades[index];
    hasher.addU64(grade.version);
    hasher.addI64(grade.start.x);
    hasher.addI64(grade.start.z);
    hasher.addI64(grade.end.x);
    hasher.addI64(grade.end.z);
    hasher.addU64(grade.startHeightCells);
    hasher.addU64(grade.endHeightCells);
    hasher.addU64(grade.halfWidthCells);
    hasher.addI64(grade.crossSlopePermille);
    hasher.addU64(grade.falloffCells);
  }
  return hasher.value();
}

}  // namespace

bool isValidCreativeBridgeSettings(
    const CreativeBridgeSettings& settings) noexcept {
  return positiveFinite(settings.deckWidthMeters) &&
         positiveFinite(settings.deckThicknessMeters) &&
         std::isfinite(settings.deckElevationOffsetMeters) &&
         positiveFinite(settings.maximumSpanMeters) &&
         nonnegativeFinite(settings.minimumClearanceMeters) &&
         settings.supportStyle < CreativeBridgeSupportStyle::Count &&
         positiveFinite(settings.supportSpacingMeters) &&
         positiveFinite(settings.supportWidthMeters) &&
         positiveFinite(settings.supportDepthMeters) &&
         settings.supportDepthMeters <= settings.deckWidthMeters &&
         positiveFinite(settings.railHeightMeters) &&
         positiveFinite(settings.railThicknessMeters) &&
         settings.railThicknessMeters < settings.deckWidthMeters * 0.5 &&
         settings.maximumApproachGradePermille <= 1000U &&
         settings.approachFalloffCells <=
             kCreativeTerrainGradeMaximumFalloffCells &&
         validMaterial(settings.materials.deck) &&
         validMaterial(settings.materials.supports) &&
         validMaterial(settings.materials.rails);
}

bool isValidCreativeBridgeSourceRecipe(
    const CreativeBridgeSourceRecipe& source) noexcept {
  return source.version == kCreativeBridgeRecipeVersion &&
         source.attachment ==
             CreativeBridgeAttachmentKind::WatercourseCrossing &&
         !source.watercoursePathKey.empty() &&
         source.crossingId != kInvalidCreativeTerrainWatercourseCrossingId &&
         isValidCreativeBridgeSettings(source.settings);
}

std::string_view toString(CreativeBridgeRecipeStatus status) noexcept {
  switch (status) {
    case CreativeBridgeRecipeStatus::NotRequested: return "NotRequested";
    case CreativeBridgeRecipeStatus::UnsupportedVersion:
      return "UnsupportedVersion";
    case CreativeBridgeRecipeStatus::InvalidRequest: return "InvalidRequest";
    case CreativeBridgeRecipeStatus::InvalidAttachment:
      return "InvalidAttachment";
    case CreativeBridgeRecipeStatus::SpanExceeded: return "SpanExceeded";
    case CreativeBridgeRecipeStatus::ClearanceInsufficient:
      return "ClearanceInsufficient";
    case CreativeBridgeRecipeStatus::ApproachGradeExceeded:
      return "ApproachGradeExceeded";
    case CreativeBridgeRecipeStatus::ApproachRejected:
      return "ApproachRejected";
    case CreativeBridgeRecipeStatus::StructureCapacityExceeded:
      return "StructureCapacityExceeded";
    case CreativeBridgeRecipeStatus::InvalidStructure:
      return "InvalidStructure";
    case CreativeBridgeRecipeStatus::Ready: return "Ready";
  }
  return "Unknown";
}

CreativeBridgeRecipeResult planCreativeBridge(
    const CreativeBridgeRecipeRequest& request) {
  CreativeBridgeRecipeResult result;
  CreativeBridgeRecipeReceipt& receipt = result.receipt;
  receipt.requested = true;
  if (request.version != kCreativeBridgeRecipeVersion ||
      request.source.version != kCreativeBridgeRecipeVersion) {
    reject(receipt, CreativeBridgeRecipeStatus::UnsupportedVersion,
           "creative_bridge_recipe_version_unsupported");
    return result;
  }
  if (request.instanceKey.empty() || request.name.empty() ||
      !positiveFinite(request.gridCellSizeMeters) ||
      !isValidCreativeBridgeSourceRecipe(request.source) ||
      !validCrossingFrame(request.crossing)) {
    reject(receipt, CreativeBridgeRecipeStatus::InvalidRequest,
           "creative_bridge_recipe_request_invalid");
    return result;
  }
  if (request.crossing.id != request.source.crossingId) {
    reject(receipt, CreativeBridgeRecipeStatus::InvalidAttachment,
           "creative_bridge_recipe_crossing_mismatch");
    return result;
  }

  const CreativeBridgeSettings& settings = request.source.settings;
  receipt.spanMeters = request.crossing.spanMeters;
  if (receipt.spanMeters > settings.maximumSpanMeters) {
    reject(receipt, CreativeBridgeRecipeStatus::SpanExceeded,
           "creative_bridge_recipe_span_exceeded");
    return result;
  }
  const double axisLength = std::hypot(request.crossing.crossingAxis.x,
                                       request.crossing.crossingAxis.z);
  const CreativeVec3 axis{request.crossing.crossingAxis.x / axisLength, 0.0,
                          request.crossing.crossingAxis.z / axisLength};
  const CreativeVec3 normal{-axis.z, 0.0, axis.x};
  const double yaw = -std::atan2(axis.z, axis.x);
  const double deckTop = request.crossing.centerMeters.y +
                         settings.deckElevationOffsetMeters;
  const double deckBottom = deckTop - settings.deckThicknessMeters;
  receipt.underDeckClearanceMeters =
      deckBottom - request.crossing.clearanceReferenceMeters.y;
  if (!std::isfinite(receipt.underDeckClearanceMeters) ||
      receipt.underDeckClearanceMeters + kBridgeGeometryEpsilon <
          settings.minimumClearanceMeters) {
    reject(receipt, CreativeBridgeRecipeStatus::ClearanceInsufficient,
           "creative_bridge_recipe_clearance_insufficient");
    return result;
  }

  result.structure.kind = CreativeRecipeKind::Bridge;
  result.structure.instanceKey = request.instanceKey;
  result.structure.instanceName = request.name;
  const CreativeVec3 deckCenter{request.crossing.centerMeters.x,
                                deckTop - settings.deckThicknessMeters * 0.5,
                                request.crossing.centerMeters.z};
  if (!appendOrientedBox(result.structure, request, CreativeObjectKind::Bridge,
                         deckCenter,
                         {receipt.spanMeters, settings.deckThicknessMeters,
                          settings.deckWidthMeters},
                         yaw, settings.materials.deck, "deck",
                         request.name + " Deck", "creative_bridge:deck")) {
    reject(receipt, CreativeBridgeRecipeStatus::InvalidStructure,
           "creative_bridge_recipe_deck_invalid");
    return result;
  }
  receipt.deckCount = 1U;

  if (settings.supportStyle == CreativeBridgeSupportStyle::PierPairs) {
    const double stationEstimate =
        std::ceil(receipt.spanMeters / settings.supportSpacingMeters) - 1.0;
    if (!std::isfinite(stationEstimate) ||
        stationEstimate >
            static_cast<double>(kCreativeBridgeSupportStationCapacity)) {
      result.structure = {};
      reject(receipt, CreativeBridgeRecipeStatus::StructureCapacityExceeded,
             "creative_bridge_recipe_support_capacity_exceeded");
      return result;
    }
    const std::size_t stationCount = stationEstimate > 0.0
                                         ? static_cast<std::size_t>(stationEstimate)
                                         : 0U;
    const double supportTop = deckBottom;
    const double supportBottom = request.crossing.channelBedMeters.y;
    const double supportHeight = supportTop - supportBottom;
    if (stationCount > 0U && !positiveFinite(supportHeight)) {
      result.structure = {};
      reject(receipt, CreativeBridgeRecipeStatus::ClearanceInsufficient,
             "creative_bridge_recipe_support_height_invalid");
      return result;
    }
    const double sideOffset =
        settings.deckWidthMeters * 0.5 - settings.supportDepthMeters * 0.5;
    for (std::size_t station = 0U; station < stationCount; ++station) {
      const double along =
          -receipt.spanMeters * 0.5 +
          receipt.spanMeters * static_cast<double>(station + 1U) /
              static_cast<double>(stationCount + 1U);
      for (std::size_t side = 0U; side < 2U; ++side) {
        const double signedSide = side == 0U ? 1.0 : -1.0;
        const CreativeVec3 center{
            request.crossing.centerMeters.x + axis.x * along +
                normal.x * sideOffset * signedSide,
            supportBottom + supportHeight * 0.5,
            request.crossing.centerMeters.z + axis.z * along +
                normal.z * sideOffset * signedSide};
        const std::string suffix = std::to_string(station + 1U) + "." +
                                   (side == 0U ? "left" : "right");
        if (!appendOrientedBox(
                result.structure, request, CreativeObjectKind::Column, center,
                {settings.supportWidthMeters, supportHeight,
                 settings.supportDepthMeters},
                yaw, settings.materials.supports, "support." + suffix,
                request.name + " Support " + suffix,
                "creative_bridge:support")) {
          result.structure = {};
          reject(receipt,
                 CreativeBridgeRecipeStatus::StructureCapacityExceeded,
                 "creative_bridge_recipe_support_generation_failed");
          return result;
        }
      }
    }
    receipt.supportStationCount = stationCount;
    receipt.supportObjectCount = stationCount * 2U;
  }

  if (settings.rails) {
    const double railOffset =
        settings.deckWidthMeters * 0.5 - settings.railThicknessMeters * 0.5;
    for (std::size_t side = 0U; side < 2U; ++side) {
      const double signedSide = side == 0U ? 1.0 : -1.0;
      const CreativeVec3 center{
          request.crossing.centerMeters.x +
              normal.x * railOffset * signedSide,
          deckTop + settings.railHeightMeters * 0.5,
          request.crossing.centerMeters.z +
              normal.z * railOffset * signedSide};
      const std::string sideName = side == 0U ? "left" : "right";
      if (!appendOrientedBox(
              result.structure, request, CreativeObjectKind::Railing, center,
              {receipt.spanMeters, settings.railHeightMeters,
               settings.railThicknessMeters},
              yaw, settings.materials.rails, "rail." + sideName,
              request.name + " " + sideName + " Rail",
              "creative_bridge:rail")) {
        result.structure = {};
        reject(receipt, CreativeBridgeRecipeStatus::StructureCapacityExceeded,
               "creative_bridge_recipe_rail_generation_failed");
        return result;
      }
    }
    receipt.railCount = 2U;
  }

  const double deckTopGrid =
      request.crossing.centerGrid.y +
      settings.deckElevationOffsetMeters / request.gridCellSizeMeters;
  if (!planApproaches(result, request, deckTopGrid)) {
    result.structure = {};
    result.approachGradeCount = 0U;
    if (receipt.status != CreativeBridgeRecipeStatus::ApproachGradeExceeded) {
      reject(receipt, CreativeBridgeRecipeStatus::ApproachRejected,
             "creative_bridge_recipe_approach_rejected");
    }
    return result;
  }

  receipt.generatedObjectCount = result.structure.objects.size();
  result.structure.definitionFingerprint =
      fingerprintCreativeRecipePlan(result.structure);
  if (result.structure.definitionFingerprint == 0U ||
      !materializeCreativeRecipe(result.structure, 1U).receipt.accepted) {
    result.structure = {};
    result.approachGradeCount = 0U;
    reject(receipt, CreativeBridgeRecipeStatus::InvalidStructure,
           "creative_bridge_recipe_structure_invalid");
    return result;
  }
  receipt.definitionFingerprint = fingerprintBridgeOutput(result);
  if (receipt.definitionFingerprint == 0U) {
    result.structure = {};
    result.approachGradeCount = 0U;
    reject(receipt, CreativeBridgeRecipeStatus::InvalidStructure,
           "creative_bridge_recipe_fingerprint_invalid");
    return result;
  }
  receipt.accepted = true;
  receipt.status = CreativeBridgeRecipeStatus::Ready;
  receipt.reasonCode = "creative_bridge_recipe_ready";
  return result;
}

}  // namespace iggy3d::creative
