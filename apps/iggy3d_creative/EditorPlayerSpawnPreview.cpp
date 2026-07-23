#include "EditorPlayerSpawnPreview.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include "EditorFrame.hpp"
#include "EditorPreviewFrame.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/render/CreativeScreenProjection.hpp"
#include "render/debug/DebugHudText.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

constexpr std::size_t kGroundRingSegmentCount = 16U;
constexpr float kGroundLineLiftMeters = 0.0125F;
constexpr float kMinimumFacingLengthMeters = 1.0F;
constexpr float kTau = 6.28318530717958647692F;

[[nodiscard]] bool statusHasAuthoredPosition(
    cr::CreativePlayerSpawnStatus status) noexcept {
  switch (status) {
    case cr::CreativePlayerSpawnStatus::UnsupportedFloor:
    case cr::CreativePlayerSpawnStatus::OutsideWorldBounds:
    case cr::CreativePlayerSpawnStatus::Obstructed:
    case cr::CreativePlayerSpawnStatus::Unreachable:
    case cr::CreativePlayerSpawnStatus::Ready:
      return true;
    case cr::CreativePlayerSpawnStatus::NotRequested:
    case cr::CreativePlayerSpawnStatus::MissingDocument:
    case cr::CreativePlayerSpawnStatus::InvalidDocument:
    case cr::CreativePlayerSpawnStatus::MissingRoomBake:
    case cr::CreativePlayerSpawnStatus::InvalidObject:
    case cr::CreativePlayerSpawnStatus::InvalidSettings:
    case cr::CreativePlayerSpawnStatus::UnsupportedProfile:
    case cr::CreativePlayerSpawnStatus::GroupUnavailable:
      return false;
  }
  return false;
}

[[nodiscard]] bool statusHasGroundedPosition(
    cr::CreativePlayerSpawnStatus status) noexcept {
  switch (status) {
    case cr::CreativePlayerSpawnStatus::OutsideWorldBounds:
    case cr::CreativePlayerSpawnStatus::Obstructed:
    case cr::CreativePlayerSpawnStatus::Unreachable:
    case cr::CreativePlayerSpawnStatus::Ready:
      return true;
    case cr::CreativePlayerSpawnStatus::NotRequested:
    case cr::CreativePlayerSpawnStatus::MissingDocument:
    case cr::CreativePlayerSpawnStatus::InvalidDocument:
    case cr::CreativePlayerSpawnStatus::MissingRoomBake:
    case cr::CreativePlayerSpawnStatus::InvalidObject:
    case cr::CreativePlayerSpawnStatus::InvalidSettings:
    case cr::CreativePlayerSpawnStatus::UnsupportedProfile:
    case cr::CreativePlayerSpawnStatus::UnsupportedFloor:
    case cr::CreativePlayerSpawnStatus::GroupUnavailable:
      return false;
  }
  return false;
}

void appendLine(CreativePlayerSpawnPreviewGeometry& geometry,
                iggy3d::Vec3 start,
                iggy3d::Vec3 end,
                CreativePlayerSpawnPreviewLineRole role) noexcept {
  if (!iggy3d::isFinite(start) || !iggy3d::isFinite(end) ||
      iggy3d::nearlyEqual(start, end)) {
    return;
  }
  if (geometry.lineCount >= geometry.lines.size()) {
    geometry.capacityExceeded = true;
    return;
  }
  geometry.lines[geometry.lineCount++] = {start, end, role};
}

void appendPhysicalEnvelope(CreativePlayerSpawnPreviewGeometry& geometry,
                            iggy3d::Vec3 foot,
                            float radius,
                            float height) noexcept {
  const float skin = static_cast<float>(iggy3d::kDefaultPlayerSkinMeters);
  const iggy3d::Vec3 minimum{foot.x - radius, foot.y + skin,
                             foot.z - radius};
  const iggy3d::Vec3 maximum{foot.x + radius, foot.y + height,
                             foot.z + radius};
  const auto corner = [&](std::uint32_t index) noexcept {
    return iggy3d::Vec3{(index & 1U) != 0U ? maximum.x : minimum.x,
                        (index & 2U) != 0U ? maximum.y : minimum.y,
                        (index & 4U) != 0U ? maximum.z : minimum.z};
  };
  constexpr std::array<std::array<std::uint32_t, 2U>, 12U> kEdges{{
      {{0U, 1U}}, {{2U, 3U}}, {{4U, 5U}}, {{6U, 7U}},
      {{0U, 2U}}, {{1U, 3U}}, {{4U, 6U}}, {{5U, 7U}},
      {{0U, 4U}}, {{1U, 5U}}, {{2U, 6U}}, {{3U, 7U}},
  }};
  for (const auto& edge : kEdges) {
    appendLine(geometry, corner(edge[0]), corner(edge[1]),
               CreativePlayerSpawnPreviewLineRole::PhysicalEnvelope);
  }
}

void appendGroundClearance(CreativePlayerSpawnPreviewGeometry& geometry,
                           iggy3d::Vec3 foot,
                           float radius) noexcept {
  const float y = foot.y + kGroundLineLiftMeters;
  for (std::size_t index = 0U; index < kGroundRingSegmentCount; ++index) {
    const float angleA =
        kTau * static_cast<float>(index) /
        static_cast<float>(kGroundRingSegmentCount);
    const float angleB =
        kTau * static_cast<float>(index + 1U) /
        static_cast<float>(kGroundRingSegmentCount);
    appendLine(geometry,
               {foot.x + std::cos(angleA) * radius, y,
                foot.z + std::sin(angleA) * radius},
               {foot.x + std::cos(angleB) * radius, y,
                foot.z + std::sin(angleB) * radius},
               CreativePlayerSpawnPreviewLineRole::GroundClearance);
  }
  const float crossRadius = std::max(radius * 0.35F, 0.10F);
  appendLine(geometry, {foot.x - crossRadius, y, foot.z},
             {foot.x + crossRadius, y, foot.z},
             CreativePlayerSpawnPreviewLineRole::FloorContact);
  appendLine(geometry, {foot.x, y, foot.z - crossRadius},
             {foot.x, y, foot.z + crossRadius},
             CreativePlayerSpawnPreviewLineRole::FloorContact);
}

[[nodiscard]] iggy3d::Vec3 authoredPositionFor(
    const cr::CreativeObject& object) noexcept {
  const cr::CreativeCoreVec3Conversion converted =
      cr::creativeVec3ToCoreChecked(object.transform.position);
  return converted.converted ? converted.value : iggy3d::Vec3{};
}

[[nodiscard]] iggy3d::Vec3 facingFor(const cr::CreativeObject& object) noexcept {
  const double yaw = object.transform.rotationEulerRadians.y;
  if (!std::isfinite(yaw)) {
    return {0.0F, 0.0F, -1.0F};
  }
  return {static_cast<float>(std::sin(yaw)), 0.0F,
          static_cast<float>(-std::cos(yaw))};
}

[[nodiscard]] iggy3d::RenderLineColor previewColor(bool accepted) noexcept {
  return accepted ? iggy3d::RenderLineColor{0.20F, 1.0F, 0.35F, 1.0F}
                  : iggy3d::RenderLineColor{1.0F, 0.20F, 0.20F, 1.0F};
}

[[nodiscard]] float previewLineThickness(
    CreativePlayerSpawnPreviewLineRole role,
    float baseThickness) noexcept {
  switch (role) {
    case CreativePlayerSpawnPreviewLineRole::CameraHeight:
    case CreativePlayerSpawnPreviewLineRole::Facing:
      return baseThickness * 1.25F;
    case CreativePlayerSpawnPreviewLineRole::FloorContact:
    case CreativePlayerSpawnPreviewLineRole::GroundingOffset:
      return baseThickness * 0.90F;
    case CreativePlayerSpawnPreviewLineRole::PhysicalEnvelope:
    case CreativePlayerSpawnPreviewLineRole::GroundClearance:
    case CreativePlayerSpawnPreviewLineRole::Count:
      return baseThickness;
  }
  return baseThickness;
}

void appendStatusLabel(const CreativeEditorOverlayFrameRequest& request,
                       const CreativePlayerSpawnPreviewGeometry& geometry,
                       iggy3d::RenderLineColor color,
                       CreativeEditorOverlayFrame& output) {
  constexpr std::size_t kLabelQuadCapacity = 128U;
  const iggy3d::RenderContentViewport content =
      iggy3d::effectiveContentViewport(request.frame);
  if (content.width == 0U || content.height == 0U) {
    return;
  }
  const iggy3d::Vec3 labelPosition =
      geometry.cameraPositionMeters + iggy3d::Vec3{0.0F, 0.15F, 0.0F};
  const cr::CreativeScreenPoint projected =
      cr::projectCreativeWorldPointToScreen(request.frame.camera.clipFromWorld,
                                            labelPosition, content.width,
                                            content.height);
  if (!projected.valid || !projected.insideViewport) {
    return;
  }
  std::array<iggy3d::DebugHudGlyphQuad, kLabelQuadCapacity> quads{};
  const iggy3d::DebugHudFixedLayoutResult layout =
      iggy3d::layoutDebugHudTextAtInto(
          creativePlayerSpawnPreviewStatusLabel(geometry.status),
          content.x + static_cast<std::int32_t>(projected.x) + 6,
          content.y + static_cast<std::int32_t>(projected.y) - 6,
          request.drawableWidth, request.drawableHeight, quads);
  if (layout.capacityExceeded) {
    return;
  }
  for (std::size_t index = 0U; index < layout.quadCount; ++index) {
    quads[index].r = color.r;
    quads[index].g = color.g;
    quads[index].b = color.b;
    quads[index].a = color.a;
  }
  output.playerSpawnPreviewLabelGlyphCount = layout.glyphCount;
  output.glyphs.insert(output.glyphs.end(), quads.begin(),
                       quads.begin() + layout.quadCount);
}

}  // namespace

CreativePlayerSpawnPreviewGeometry planCreativePlayerSpawnPreview(
    const cr::CreativePlayerSpawnPlanRequest& request) {
  CreativePlayerSpawnPreviewGeometry geometry;
  if (request.spawnObject == nullptr ||
      request.spawnObject->kind != cr::CreativeObjectKind::SpawnPoint ||
      request.spawnObject->id == cr::kInvalidObjectId) {
    return geometry;
  }

  const cr::CreativeObject& object = *request.spawnObject;
  const cr::CreativePlayerSpawnPlan plan =
      cr::planCreativePlayerSpawn(request);
  geometry.active = true;
  geometry.accepted = plan.accepted;
  geometry.status = plan.status;
  geometry.reasonCode = plan.reasonCode;
  geometry.objectId = object.id;

  const iggy3d::Vec3 authoredFallback = authoredPositionFor(object);
  geometry.authoredPositionMeters =
      statusHasAuthoredPosition(plan.status) &&
              iggy3d::isFinite(plan.authoredPositionMeters)
          ? plan.authoredPositionMeters
          : authoredFallback;
  geometry.groundedPositionMeters =
      statusHasGroundedPosition(plan.status) &&
              iggy3d::isFinite(plan.groundedPositionMeters)
          ? plan.groundedPositionMeters
          : geometry.authoredPositionMeters;
  const float fallbackRadius =
      cr::isValidCreativePlayerSpawnSettings(object.playerSpawn)
          ? static_cast<float>(std::max(
                object.playerSpawn.validationRadiusMeters,
                iggy3d::kDefaultPlayerBodyRadiusMeters))
          : static_cast<float>(iggy3d::kDefaultPlayerBodyRadiusMeters);
  geometry.clearanceRadiusMeters =
      std::isfinite(plan.clearanceRadiusMeters) &&
              plan.clearanceRadiusMeters > 0.0F
          ? plan.clearanceRadiusMeters
          : fallbackRadius;
  geometry.bodyHeightMeters =
      std::isfinite(plan.bodyHeightMeters) && plan.bodyHeightMeters > 0.0F
          ? plan.bodyHeightMeters
          : static_cast<float>(iggy3d::kDefaultPlayerStandingHeightMeters);
  geometry.cameraPositionMeters =
      geometry.groundedPositionMeters +
      iggy3d::Vec3{0.0F, cr::kCreativeDefaultPlayerEyeHeightMeters, 0.0F};
  geometry.facingDirection =
      iggy3d::isFinite(plan.facingDirection) &&
              iggy3d::lengthSquared(plan.facingDirection) > 0.0F
          ? plan.facingDirection
          : facingFor(object);

  appendPhysicalEnvelope(geometry, geometry.groundedPositionMeters,
                         geometry.clearanceRadiusMeters,
                         geometry.bodyHeightMeters);
  appendGroundClearance(geometry, geometry.groundedPositionMeters,
                        geometry.clearanceRadiusMeters);
  appendLine(geometry, geometry.groundedPositionMeters,
             geometry.cameraPositionMeters,
             CreativePlayerSpawnPreviewLineRole::CameraHeight);
  const float facingLength = std::max(
      kMinimumFacingLengthMeters, geometry.clearanceRadiusMeters * 2.0F);
  appendLine(geometry, geometry.cameraPositionMeters,
             geometry.cameraPositionMeters +
                 geometry.facingDirection * facingLength,
             CreativePlayerSpawnPreviewLineRole::Facing);
  appendLine(geometry, geometry.authoredPositionMeters,
             geometry.groundedPositionMeters,
             CreativePlayerSpawnPreviewLineRole::GroundingOffset);
  return geometry;
}

bool refreshCreativePlayerSpawnPreviewCache(
    CreativePlayerSpawnPreviewCache& cache,
    const cr::CreativeDocument& document,
    const cr::CreativeRoomBakeResult* roomBake,
    const cr::CreativeObject* selectedObject,
    std::uint64_t roomBakeRevision) {
  const cr::CreativeObjectId objectId =
      selectedObject != nullptr ? selectedObject->id : cr::kInvalidObjectId;
  if (cache.valid && cache.documentId == document.id() &&
      cache.documentRevision == document.revision() &&
      cache.objectId == objectId && cache.roomBakeIdentity == roomBake &&
      cache.roomBakeRevision == roomBakeRevision) {
    return false;
  }
  cache.geometry =
      planCreativePlayerSpawnPreview({&document, roomBake, selectedObject});
  cache.documentId = document.id();
  cache.documentRevision = document.revision();
  cache.objectId = objectId;
  cache.roomBakeIdentity = roomBake;
  cache.roomBakeRevision = roomBakeRevision;
  ++cache.refreshCount;
  cache.valid = true;
  return true;
}

void invalidateCreativePlayerSpawnPreviewCache(
    CreativePlayerSpawnPreviewCache& cache) noexcept {
  cache = {};
}

CreativePlayerSpawnPlanSymbol planCreativePlayerSpawnPlanSymbol(
    cr::CreativeVec3 pointCells,
    double yawRadians,
    const cr::CreativePlayerSpawnSettings& settings,
    double cellSizeMeters) noexcept {
  CreativePlayerSpawnPlanSymbol symbol;
  if (!cr::isFiniteCreativeVec3(pointCells) ||
      !std::isfinite(cellSizeMeters) || cellSizeMeters <= 0.0) {
    return symbol;
  }
  const bool settingsValid = cr::isValidCreativePlayerSpawnSettings(settings);
  const bool yawValid = std::isfinite(yawRadians);
  const double bodyRadius = iggy3d::kDefaultPlayerBodyRadiusMeters;
  const double clearanceRadius =
      settingsValid
          ? std::max(settings.validationRadiusMeters, bodyRadius)
          : bodyRadius;
  const double safeYaw = yawValid ? yawRadians : 0.0;
  const double facingLengthMeters = std::max(1.25, clearanceRadius);

  symbol.drawable = true;
  symbol.runtimeReady =
      settingsValid && yawValid &&
      cr::isSupportedCreativePlayerProfileId(settings.playerProfileId);
  symbol.centerCells = pointCells;
  symbol.bodyRadiusCells = bodyRadius / cellSizeMeters;
  symbol.clearanceRadiusCells = clearanceRadius / cellSizeMeters;
  symbol.facingEndCells =
      {pointCells.x + std::sin(safeYaw) * facingLengthMeters / cellSizeMeters,
       pointCells.y,
       pointCells.z - std::cos(safeYaw) * facingLengthMeters / cellSizeMeters};
  return symbol;
}

std::string_view creativePlayerSpawnPreviewStatusLabel(
    cr::CreativePlayerSpawnStatus status) noexcept {
  switch (status) {
    case cr::CreativePlayerSpawnStatus::NotRequested:
      return "SPAWN: NOT REQUESTED";
    case cr::CreativePlayerSpawnStatus::MissingDocument:
      return "SPAWN: DOCUMENT MISSING";
    case cr::CreativePlayerSpawnStatus::InvalidDocument:
      return "SPAWN: REPAIR DOCUMENT";
    case cr::CreativePlayerSpawnStatus::MissingRoomBake:
      return "SPAWN: PREVIEW UNAVAILABLE";
    case cr::CreativePlayerSpawnStatus::InvalidObject:
      return "SPAWN: RECREATE MARKER";
    case cr::CreativePlayerSpawnStatus::InvalidSettings:
      return "SPAWN: FIX PROFILE GROUP OR RADIUS";
    case cr::CreativePlayerSpawnStatus::UnsupportedProfile:
      return "SPAWN: PROFILE UNAVAILABLE";
    case cr::CreativePlayerSpawnStatus::OutsideWorldBounds:
      return "SPAWN: MOVE INSIDE MAP";
    case cr::CreativePlayerSpawnStatus::UnsupportedFloor:
      return "SPAWN: MOVE ONTO SUPPORTED FLOOR";
    case cr::CreativePlayerSpawnStatus::Obstructed:
      return "SPAWN: CLEAR BODY SPACE";
    case cr::CreativePlayerSpawnStatus::Unreachable:
      return "SPAWN: CONNECT WALKABLE FLOOR";
    case cr::CreativePlayerSpawnStatus::GroupUnavailable:
      return "SPAWN: GROUP HAS NO VALID MARKER";
    case cr::CreativePlayerSpawnStatus::Ready:
      return "SPAWN: READY";
  }
  return "SPAWN: INVALID";
}

void appendCreativeEditorPlayerSpawnPreview(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  if (request.captureMode || request.selection.selected == nullptr) {
    return;
  }
  if (request.playerSpawnPreview == nullptr) {
    return;
  }
  const CreativePlayerSpawnPreviewGeometry& geometry =
      *request.playerSpawnPreview;
  output.playerSpawnPreviewActive = geometry.active;
  output.playerSpawnPreviewAccepted = geometry.accepted;
  output.playerSpawnPreviewStatus = geometry.status;
  output.playerSpawnPreviewReasonCode = geometry.reasonCode;
  if (!geometry.active) {
    return;
  }

  const iggy3d::RenderLineColor color = previewColor(geometry.accepted);
  const float baseThickness = std::max(0.035F, request.gizmoThickness);
  output.combinedWireLines.reserve(output.combinedWireLines.size() +
                                   geometry.lineCount);
  for (std::size_t index = 0U; index < geometry.lineCount; ++index) {
    const CreativePlayerSpawnPreviewLine& previewLine = geometry.lines[index];
    iggy3d::RenderCreativeWireframeDebugLine line;
    line.start = previewLine.start;
    line.end = previewLine.end;
    line.color = color;
    line.objectId = geometry.objectId;
    line.objectKind =
        static_cast<std::uint32_t>(cr::CreativeObjectKind::SpawnPoint);
    line.style = static_cast<std::uint32_t>(previewLine.role);
    line.thickness = previewLineThickness(previewLine.role, baseThickness);
    output.combinedWireLines.push_back(line);
  }
  output.playerSpawnPreviewEdgeCount = geometry.lineCount;
  appendStatusLabel(request, geometry, color, output);
}

}  // namespace iggy3d_creative_app
