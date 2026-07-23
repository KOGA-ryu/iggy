#include "app/iggy3d/creative/recipes/RetainingEdgeRecipe.hpp"

#include "app/iggy3d/creative/Geometry.hpp"

#include <algorithm>
#include <cmath>
#include <compare>
#include <cstdint>
#include <map>
#include <numbers>
#include <optional>
#include <string>
#include <utility>

namespace iggy3d::creative {
namespace {

constexpr double kRetainingEdgeEpsilon = 1.0e-9;
constexpr std::string_view kInfrastructureWallAsset =
    "infrastructure/retaining_wall_2x1p2";
constexpr std::string_view kInfrastructureStairAsset =
    "infrastructure/terrain_steps_2x3x1p2";

void reject(CreativeRetainingEdgeRecipeReceipt& receipt,
            CreativeRetainingEdgeRecipeStatus status,
            std::string_view reasonCode) {
  receipt.status = status;
  receipt.reasonCode = reasonCode;
}

[[nodiscard]] bool positiveFinite(double value) noexcept {
  return std::isfinite(value) && value > 0.0;
}

[[nodiscard]] bool validGrid(CreativeGridSettings grid) noexcept {
  return isFiniteCreativeVec3(grid.origin) &&
         positiveFinite(grid.cellSizeMeters) && grid.size.width > 0 &&
         grid.size.height > 0 && grid.size.depth > 0;
}

[[nodiscard]] bool coordInside(
    CreativeTerrainCoord2 coord,
    CreativeTerrainHeightFieldBounds bounds) noexcept {
  const std::int64_t offsetX =
      static_cast<std::int64_t>(coord.x) - bounds.minimum.x;
  const std::int64_t offsetZ =
      static_cast<std::int64_t>(coord.z) - bounds.minimum.z;
  return offsetX >= 0 && offsetZ >= 0 && offsetX < bounds.widthCells &&
         offsetZ < bounds.depthCells;
}

[[nodiscard]] bool selectsEdge(
    CreativeRetainingEdgeSelection selection,
    CreativeTerrainHeightFieldBounds bounds,
    CreativeTerrainHardEdge edge) noexcept {
  const bool firstInside = coordInside(edge.first, bounds);
  const bool secondInside = coordInside(edge.second, bounds);
  switch (selection) {
    case CreativeRetainingEdgeSelection::All:
      return firstInside || secondInside;
    case CreativeRetainingEdgeSelection::Internal:
      return firstInside && secondInside;
    case CreativeRetainingEdgeSelection::Perimeter:
      return firstInside != secondInside;
    case CreativeRetainingEdgeSelection::Count:
      break;
  }
  return false;
}

[[nodiscard]] std::uint16_t terrainHeightOrZero(
    const CreativeTerrainHeightField& terrain,
    CreativeTerrainCoord2 coord) noexcept {
  const std::optional<std::uint16_t> height = terrain.heightAt(coord);
  return height.has_value() && *height != kCreativeTerrainEmptyHeightCells
             ? *height
             : 0U;
}

struct HalfGridPoint {
  std::int64_t x2 = 0;
  std::int64_t z2 = 0;

  [[nodiscard]] friend constexpr auto operator<=>(
      HalfGridPoint,
      HalfGridPoint) noexcept = default;
};

struct EdgeGeometry {
  CreativeTerrainHardEdge edge{};
  HalfGridPoint firstEndpoint{};
  HalfGridPoint secondEndpoint{};
  CreativeVec3 centerMeters{};
  double lengthMeters = 0.0;
  double baseMeters = 0.0;
  double topMeters = 0.0;
  double yawRadians = 0.0;
  bool horizontal = false;
};

[[nodiscard]] bool edgeGeometry(
    const CreativeRetainingEdgeRecipeRequest& request,
    CreativeTerrainHardEdge edge,
    EdgeGeometry& output) noexcept {
  const std::int64_t deltaX =
      static_cast<std::int64_t>(edge.second.x) - edge.first.x;
  const std::int64_t deltaZ =
      static_cast<std::int64_t>(edge.second.z) - edge.first.z;
  const std::uint16_t firstHeight =
      terrainHeightOrZero(*request.terrain, edge.first);
  const std::uint16_t secondHeight =
      terrainHeightOrZero(*request.terrain, edge.second);
  if (firstHeight == secondHeight) {
    return false;
  }

  output.edge = edge;
  output.lengthMeters = request.grid.cellSizeMeters;
  output.baseMeters = request.grid.origin.y +
                      std::min(firstHeight, secondHeight) *
                          request.grid.cellSizeMeters;
  output.topMeters = request.grid.origin.y +
                     std::max(firstHeight, secondHeight) *
                         request.grid.cellSizeMeters;
  if (deltaX != 0) {
    const std::int64_t boundaryX2 =
        static_cast<std::int64_t>(edge.first.x) + edge.second.x;
    const std::int64_t centerZ2 =
        static_cast<std::int64_t>(edge.first.z) * 2;
    output.firstEndpoint = {boundaryX2, centerZ2 - 1};
    output.secondEndpoint = {boundaryX2, centerZ2 + 1};
    output.yawRadians = -std::numbers::pi * 0.5;
    output.horizontal = false;
  } else if (deltaZ != 0) {
    const std::int64_t centerX2 =
        static_cast<std::int64_t>(edge.first.x) * 2;
    const std::int64_t boundaryZ2 =
        static_cast<std::int64_t>(edge.first.z) + edge.second.z;
    output.firstEndpoint = {centerX2 - 1, boundaryZ2};
    output.secondEndpoint = {centerX2 + 1, boundaryZ2};
    output.yawRadians = 0.0;
    output.horizontal = true;
  } else {
    return false;
  }
  const double centerX =
      (static_cast<double>(output.firstEndpoint.x2) +
       static_cast<double>(output.secondEndpoint.x2)) *
      0.25;
  const double centerZ =
      (static_cast<double>(output.firstEndpoint.z2) +
       static_cast<double>(output.secondEndpoint.z2)) *
      0.25;
  output.centerMeters = {
      request.grid.origin.x + centerX * request.grid.cellSizeMeters,
      (output.baseMeters + output.topMeters) * 0.5,
      request.grid.origin.z + centerZ * request.grid.cellSizeMeters,
  };
  return isFiniteCreativeVec3(output.centerMeters) &&
         positiveFinite(output.topMeters - output.baseMeters);
}

void appendTagOnce(std::vector<std::string>& tags, std::string tag) {
  if (!tag.empty() &&
      std::find(tags.begin(), tags.end(), tag) == tags.end()) {
    tags.push_back(std::move(tag));
  }
}

[[nodiscard]] std::string edgeKey(CreativeTerrainHardEdge edge) {
  return std::to_string(edge.first.x) + "." +
         std::to_string(edge.first.z) + "." +
         std::to_string(edge.second.x) + "." +
         std::to_string(edge.second.z);
}

[[nodiscard]] std::string pointKey(HalfGridPoint point) {
  return std::to_string(point.x2) + "." + std::to_string(point.z2);
}

[[nodiscard]] bool appendBox(
    CreativeRecipePlan& plan,
    const CreativeRetainingEdgeRecipeRequest& request,
    CreativeObjectKind kind,
    CreativeVec3 center,
    CreativeVec3 size,
    double yawRadians,
    std::string stableKey,
    std::string name,
    std::string roleTag,
    std::string assetId = {}) {
  if (plan.objects.size() >= kCreativeRetainingEdgeGeneratedObjectCapacity ||
      !isFiniteCreativeVec3(center) || !isPositiveCreativeVec3(size) ||
      !std::isfinite(yawRadians) || stableKey.empty()) {
    return false;
  }
  CreativeDocumentCreateRequest create;
  create.kind = kind;
  create.name = std::move(name);
  create.assetId = std::move(assetId);
  const CreativeVec3 half{size.x * 0.5, size.y * 0.5, size.z * 0.5};
  create.bounds = {{center.x - half.x, center.y - half.y, center.z - half.z},
                   {center.x + half.x, center.y + half.y, center.z + half.z}};
  create.hasBoundsOverride = true;
  create.transform.position = center;
  create.transform.rotationEulerRadians.y = yawRadians;
  create.hasTransformOverride = true;
  create.visible = true;
  create.hasVisibleOverride = true;
  create.tags = request.tags;
  appendTagOnce(create.tags, "creative_retaining_edge:generated");
  appendTagOnce(create.tags, std::move(roleTag));
  appendTagOnce(create.tags, creativeStructuralMaterialTag(
                                 request.source.settings.material));

  CreativeRecipeObjectPlan object;
  object.createRequest = std::move(create);
  object.role = CreativeRecipeObjectRole::Generated;
  object.stableKey = std::move(stableKey);
  plan.objects.push_back(std::move(object));
  return true;
}

struct VertexFacts {
  std::size_t degree = 0U;
  bool horizontal = false;
  bool vertical = false;
  double baseMeters = 0.0;
  double topMeters = 0.0;
};

void addVertexFact(std::map<HalfGridPoint, VertexFacts>& vertices,
                   HalfGridPoint point,
                   const EdgeGeometry& edge) {
  VertexFacts& facts = vertices[point];
  if (facts.degree == 0U) {
    facts.baseMeters = edge.baseMeters;
    facts.topMeters = edge.topMeters;
  } else {
    facts.baseMeters = std::min(facts.baseMeters, edge.baseMeters);
    facts.topMeters = std::max(facts.topMeters, edge.topMeters);
  }
  ++facts.degree;
  facts.horizontal = facts.horizontal || edge.horizontal;
  facts.vertical = facts.vertical || !edge.horizontal;
}

[[nodiscard]] bool hasTransition(
    const CreativeRetainingEdgeSettings& settings,
    CreativeTerrainHardEdge edge,
    std::size_t& transitionIndex) noexcept {
  for (std::size_t index = 0U; index < settings.transitionCount; ++index) {
    if (settings.transitions[index].edge == edge) {
      transitionIndex = index;
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool appendTransition(
    CreativeRecipePlan& plan,
    CreativeRetainingEdgeRecipeReceipt& receipt,
    const CreativeRetainingEdgeRecipeRequest& request,
    const EdgeGeometry& edge,
    const CreativeRetainingEdgeTransition& transition,
    std::size_t transitionIndex) {
  const std::uint16_t firstHeight =
      terrainHeightOrZero(*request.terrain, edge.edge.first);
  const std::uint16_t secondHeight =
      terrainHeightOrZero(*request.terrain, edge.edge.second);
  if (firstHeight == 0U || secondHeight == 0U ||
      firstHeight == secondHeight) {
    return false;
  }
  const CreativeTerrainCoord2 low =
      firstHeight < secondHeight ? edge.edge.first : edge.edge.second;
  const CreativeTerrainCoord2 high =
      firstHeight < secondHeight ? edge.edge.second : edge.edge.first;
  const double runMeters = transition.runCells * request.grid.cellSizeMeters;
  const double riseMeters = edge.topMeters - edge.baseMeters;
  const double centerX = request.grid.origin.x +
                         (static_cast<double>(low.x) + high.x) * 0.5 *
                             request.grid.cellSizeMeters;
  const double centerZ = request.grid.origin.z +
                         (static_cast<double>(low.z) + high.z) * 0.5 *
                             request.grid.cellSizeMeters;
  const CreativeVec3 center{centerX, (edge.baseMeters + edge.topMeters) * 0.5,
                            centerZ};
  const double directionX = static_cast<double>(high.x) - low.x;
  const double directionZ = static_cast<double>(high.z) - low.z;
  const double yaw = std::atan2(directionX, directionZ);
  const CreativeVec3 size{request.grid.cellSizeMeters, riseMeters, runMeters};
  const CreativeBounds bounds{
      {center.x - size.x * 0.5, center.y - size.y * 0.5,
       center.z - size.z * 0.5},
      {center.x + size.x * 0.5, center.y + size.y * 0.5,
       center.z + size.z * 0.5},
  };
  CreativeTransform transform;
  transform.position = center;
  transform.rotationEulerRadians.y = yaw;
  const std::string key = "transition." + std::to_string(transitionIndex + 1U) +
                          "." + edgeKey(edge.edge);
  if (transition.kind == CreativeRetainingEdgeTransitionKind::Stair) {
    CreativeStairRecipeRequest stair;
    stair.authoredBounds = bounds;
    stair.transform = transform;
    stair.landingDepthMeters = request.grid.cellSizeMeters;
    stair.availableHeadroomMeters = request.source.settings.maximumHeightMeters;
    const CreativeStairRecipeResult planned = planCreativeStair(stair);
    if (!planned.accepted ||
        !appendBox(plan, request, CreativeObjectKind::Stair, center, size, yaw,
                   key, request.name + " Stair", "creative_retaining_edge:stair",
                   request.source.settings.kit ==
                           CreativeRetainingEdgeKit::InfrastructureStone
                       ? std::string{kInfrastructureStairAsset}
                       : std::string{})) {
      return false;
    }
    ++receipt.stairCount;
    return true;
  }
  if (transition.kind == CreativeRetainingEdgeTransitionKind::Ramp) {
    CreativeRampRecipeRequest ramp;
    ramp.authoredBounds = bounds;
    ramp.transform = transform;
    ramp.landingDepthMeters = request.grid.cellSizeMeters;
    ramp.availableHeadroomMeters = request.source.settings.maximumHeightMeters;
    ramp.material = request.source.settings.material;
    const CreativeRampRecipeResult planned = planCreativeRamp(ramp);
    if (!planned.accepted || !planned.walkable ||
        !appendBox(plan, request, CreativeObjectKind::Ramp, center, size, yaw,
                   key, request.name + " Ramp", "creative_retaining_edge:ramp")) {
      return false;
    }
    ++receipt.rampCount;
    return true;
  }
  return false;
}

}  // namespace

bool isValidCreativeRetainingEdgeSettings(
    const CreativeRetainingEdgeSettings& settings) noexcept {
  if (settings.selection >= CreativeRetainingEdgeSelection::Count ||
      settings.kit >= CreativeRetainingEdgeKit::Count ||
      !positiveFinite(settings.thicknessMeters) ||
      !positiveFinite(settings.maximumHeightMeters) ||
      settings.material >= CreativeStructuralMaterial::Count ||
      settings.transitionCount > kCreativeRetainingEdgeTransitionCapacity) {
    return false;
  }
  for (std::size_t index = 0U; index < settings.transitionCount; ++index) {
    const CreativeRetainingEdgeTransition& transition =
        settings.transitions[index];
    if (!isValidCreativeTerrainHardEdge(transition.edge) ||
        transition.kind >= CreativeRetainingEdgeTransitionKind::Count ||
        transition.runCells == 0U) {
      return false;
    }
    for (std::size_t prior = 0U; prior < index; ++prior) {
      if (settings.transitions[prior].edge == transition.edge) {
        return false;
      }
    }
  }
  return true;
}

bool isValidCreativeRetainingEdgeSourceRecipe(
    const CreativeRetainingEdgeSourceRecipe& source) noexcept {
  return source.version == kCreativeRetainingEdgeRecipeVersion &&
         !source.terrainProfileKey.empty() &&
         isValidCreativeRetainingEdgeSettings(source.settings);
}

std::string_view toString(CreativeRetainingEdgeRecipeStatus status) noexcept {
  switch (status) {
    case CreativeRetainingEdgeRecipeStatus::NotRequested:
      return "NotRequested";
    case CreativeRetainingEdgeRecipeStatus::UnsupportedVersion:
      return "UnsupportedVersion";
    case CreativeRetainingEdgeRecipeStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeRetainingEdgeRecipeStatus::NoMatchingEdges:
      return "NoMatchingEdges";
    case CreativeRetainingEdgeRecipeStatus::TransitionInvalid:
      return "TransitionInvalid";
    case CreativeRetainingEdgeRecipeStatus::HeightExceeded:
      return "HeightExceeded";
    case CreativeRetainingEdgeRecipeStatus::StructureCapacityExceeded:
      return "StructureCapacityExceeded";
    case CreativeRetainingEdgeRecipeStatus::InvalidStructure:
      return "InvalidStructure";
    case CreativeRetainingEdgeRecipeStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

CreativeRetainingEdgeRecipeResult planCreativeRetainingEdge(
    const CreativeRetainingEdgeRecipeRequest& request) {
  CreativeRetainingEdgeRecipeResult result;
  CreativeRetainingEdgeRecipeReceipt& receipt = result.receipt;
  receipt.requested = true;
  if (request.version != kCreativeRetainingEdgeRecipeVersion ||
      request.source.version != kCreativeRetainingEdgeRecipeVersion) {
    reject(receipt, CreativeRetainingEdgeRecipeStatus::UnsupportedVersion,
           "creative_retaining_edge_recipe_version_unsupported");
    return result;
  }
  if (request.instanceKey.empty() || request.name.empty() ||
      !validGrid(request.grid) ||
      !isValidCreativeTerrainHeightFieldBounds(request.profileBounds) ||
      !isValidCreativeRetainingEdgeSourceRecipe(request.source) ||
      request.terrain == nullptr || !request.terrain->validateInvariants() ||
      !validateCreativeTerrainHardEdges(request.hardEdges)) {
    reject(receipt, CreativeRetainingEdgeRecipeStatus::InvalidRequest,
           "creative_retaining_edge_recipe_request_invalid");
    return result;
  }

  std::vector<EdgeGeometry> selected;
  selected.reserve(request.hardEdges.size());
  for (const CreativeTerrainHardEdge edge : request.hardEdges) {
    if (!selectsEdge(request.source.settings.selection, request.profileBounds,
                     edge)) {
      continue;
    }
    EdgeGeometry geometry;
    if (!edgeGeometry(request, edge, geometry)) {
      reject(receipt, CreativeRetainingEdgeRecipeStatus::InvalidRequest,
             "creative_retaining_edge_recipe_edge_invalid");
      return result;
    }
    const double height = geometry.topMeters - geometry.baseMeters;
    receipt.maximumWallHeightMeters =
        std::max(receipt.maximumWallHeightMeters, height);
    if (height > request.source.settings.maximumHeightMeters +
                     kRetainingEdgeEpsilon) {
      reject(receipt, CreativeRetainingEdgeRecipeStatus::HeightExceeded,
             "creative_retaining_edge_recipe_height_exceeded");
      return result;
    }
    selected.push_back(geometry);
  }
  receipt.selectedEdgeCount = selected.size();
  if (selected.empty()) {
    reject(receipt, CreativeRetainingEdgeRecipeStatus::NoMatchingEdges,
           "creative_retaining_edge_recipe_no_matching_edges");
    return result;
  }

  for (std::size_t index = 0U;
       index < request.source.settings.transitionCount; ++index) {
    const CreativeTerrainHardEdge edge =
        request.source.settings.transitions[index].edge;
    if (std::none_of(selected.begin(), selected.end(), [&](const auto& value) {
          return value.edge == edge;
        })) {
      reject(receipt, CreativeRetainingEdgeRecipeStatus::TransitionInvalid,
             "creative_retaining_edge_recipe_transition_unmatched");
      return result;
    }
  }

  result.structure.kind = CreativeRecipeKind::RetainingEdge;
  result.structure.instanceKey = request.instanceKey;
  result.structure.instanceName = request.name;
  std::map<HalfGridPoint, VertexFacts> vertices;
  for (const EdgeGeometry& edge : selected) {
    std::size_t transitionIndex = 0U;
    if (hasTransition(request.source.settings, edge.edge, transitionIndex)) {
      if (!appendTransition(
              result.structure, receipt, request, edge,
              request.source.settings.transitions[transitionIndex],
              transitionIndex)) {
        result.structure = {};
        reject(receipt, CreativeRetainingEdgeRecipeStatus::TransitionInvalid,
               "creative_retaining_edge_recipe_transition_rejected");
        return result;
      }
      continue;
    }
    addVertexFact(vertices, edge.firstEndpoint, edge);
    addVertexFact(vertices, edge.secondEndpoint, edge);
    const double height = edge.topMeters - edge.baseMeters;
    if (!appendBox(
            result.structure, request, CreativeObjectKind::Wall,
            edge.centerMeters,
            {edge.lengthMeters + request.source.settings.thicknessMeters,
             height, request.source.settings.thicknessMeters},
            edge.yawRadians, "segment." + edgeKey(edge.edge),
            request.name + " Segment",
            "creative_retaining_edge:segment",
            request.source.settings.kit ==
                    CreativeRetainingEdgeKit::InfrastructureStone
                ? std::string{kInfrastructureWallAsset}
                : std::string{})) {
      result.structure = {};
      reject(receipt,
             CreativeRetainingEdgeRecipeStatus::StructureCapacityExceeded,
             "creative_retaining_edge_recipe_segment_capacity_exceeded");
      return result;
    }
    ++receipt.wallSegmentCount;
  }

  for (const auto& [point, facts] : vertices) {
    const bool cap = facts.degree == 1U && request.source.settings.capEnds;
    const bool corner = facts.degree >= 2U && facts.horizontal &&
                        facts.vertical &&
                        request.source.settings.closeCorners;
    if (!cap && !corner) {
      continue;
    }
    const double height = facts.topMeters - facts.baseMeters;
    const CreativeVec3 center{
        request.grid.origin.x + static_cast<double>(point.x2) * 0.5 *
                                    request.grid.cellSizeMeters,
        (facts.baseMeters + facts.topMeters) * 0.5,
        request.grid.origin.z + static_cast<double>(point.z2) * 0.5 *
                                    request.grid.cellSizeMeters,
    };
    const std::string role = cap ? "cap" : "corner";
    if (!appendBox(
            result.structure, request, CreativeObjectKind::Column, center,
            {request.source.settings.thicknessMeters, height,
             request.source.settings.thicknessMeters},
            0.0, role + "." + pointKey(point),
            request.name + (cap ? " End Cap" : " Corner"),
            "creative_retaining_edge:" + role)) {
      result.structure = {};
      reject(receipt,
             CreativeRetainingEdgeRecipeStatus::StructureCapacityExceeded,
             "creative_retaining_edge_recipe_joint_capacity_exceeded");
      return result;
    }
    if (cap) {
      ++receipt.capCount;
    } else {
      ++receipt.cornerCount;
    }
  }

  receipt.generatedObjectCount = result.structure.objects.size();
  result.structure.definitionFingerprint =
      fingerprintCreativeRecipePlan(result.structure);
  if (result.structure.definitionFingerprint == 0U ||
      !materializeCreativeRecipe(result.structure, 1U).receipt.accepted) {
    result.structure = {};
    reject(receipt, CreativeRetainingEdgeRecipeStatus::InvalidStructure,
           "creative_retaining_edge_recipe_structure_invalid");
    return result;
  }
  receipt.definitionFingerprint = result.structure.definitionFingerprint;
  receipt.accepted = true;
  receipt.status = CreativeRetainingEdgeRecipeStatus::Ready;
  receipt.reasonCode = "creative_retaining_edge_recipe_ready";
  return result;
}

}  // namespace iggy3d::creative
