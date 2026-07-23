#include "EditorFrame.hpp"
#include "EditorGizmo.hpp"
#include "EditorPlayerSpawnPreview.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace {
namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

constexpr double kPi = 3.14159265358979323846;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float left, float right, float epsilon = 0.0001F) {
  return std::fabs(left - right) <= epsilon;
}

cr::CreativeDocumentCreateReceipt addFloor(cr::CreativeDocument& document) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Floor;
  request.name = "Spawn Preview Floor";
  request.transform.position = {-4.0, 0.0, -4.0};
  request.hasTransformOverride = true;
  request.bounds = {{-4.0, 0.0, -4.0}, {4.0, 0.25, 4.0}};
  request.hasBoundsOverride = true;
  return document.createObject(request);
}

cr::CreativeDocumentCreateReceipt addSpawn(
    cr::CreativeDocument& document,
    cr::CreativeVec3 position,
    cr::CreativePlayerSpawnSettings settings = {},
    double yawRadians = kPi * 0.5) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::SpawnPoint;
  request.name = "Preview Spawn";
  request.transform.position = position;
  request.transform.rotationEulerRadians.y = yawRadians;
  request.hasTransformOverride = true;
  request.playerSpawn = std::move(settings);
  request.hasPlayerSpawnSettingsOverride = true;
  return document.createObject(request);
}

cr::CreativeDocumentCreateReceipt addBlocker(cr::CreativeDocument& document,
                                             cr::CreativeVec3 position) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Wall;
  request.name = "Spawn Preview Blocker";
  request.transform.position = position;
  request.hasTransformOverride = true;
  request.bounds = {{position.x - 0.4, position.y, position.z - 0.4},
                    {position.x + 0.4, position.y + 2.0,
                     position.z + 0.4}};
  request.hasBoundsOverride = true;
  return document.createObject(request);
}

cr::CreativeRoomBakeResult bake(const cr::CreativeDocument& document) {
  cr::CreativeRoomBakeRequest request;
  request.document = &document;
  request.roomId = "player_spawn_preview_test";
  request.sourceName = "Player Spawn Preview Test";
  request.validateReachability = false;
  return cr::buildRoomAssetFromCreativeDocument(request);
}

cr::CreativeDocument makeDocument() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Player Spawn Preview");
  static_cast<void>(document.assignId(991U));
  static_cast<void>(document.setWorldBounds(
      {{-5.0, 0.0, -5.0}, {5.0, 3.0, 5.0}}));
  static_cast<void>(addFloor(document));
  return document;
}

std::size_t roleCount(const app::CreativePlayerSpawnPreviewGeometry& geometry,
                      app::CreativePlayerSpawnPreviewLineRole role) {
  return static_cast<std::size_t>(std::count_if(
      geometry.lines.begin(), geometry.lines.begin() + geometry.lineCount,
      [role](const app::CreativePlayerSpawnPreviewLine& line) {
        return line.role == role;
      }));
}

bool readyGeometryMatchesTheExactPhysicalPlan() {
  cr::CreativeDocument document = makeDocument();
  cr::CreativePlayerSpawnSettings settings;
  settings.validationRadiusMeters = 0.55;
  const cr::CreativeDocumentCreateReceipt spawn =
      addSpawn(document, {1.0, 0.25, -1.0}, settings);
  const cr::CreativeRoomBakeResult roomBake = bake(document);
  const app::CreativePlayerSpawnPreviewGeometry geometry =
      app::planCreativePlayerSpawnPreview(
          {&document, &roomBake, document.findObject(spawn.objectId), 0.5F});

  float minimumEnvelopeX = std::numeric_limits<float>::max();
  float maximumEnvelopeX = std::numeric_limits<float>::lowest();
  float minimumEnvelopeY = std::numeric_limits<float>::max();
  float maximumEnvelopeY = std::numeric_limits<float>::lowest();
  bool allFiniteAndNondegenerate = true;
  const app::CreativePlayerSpawnPreviewLine* facing = nullptr;
  for (std::size_t index = 0U; index < geometry.lineCount; ++index) {
    const app::CreativePlayerSpawnPreviewLine& line = geometry.lines[index];
    allFiniteAndNondegenerate =
        allFiniteAndNondegenerate && iggy3d::isFinite(line.start) &&
        iggy3d::isFinite(line.end) &&
        !iggy3d::nearlyEqual(line.start, line.end);
    if (line.role ==
        app::CreativePlayerSpawnPreviewLineRole::PhysicalEnvelope) {
      minimumEnvelopeX =
          std::min({minimumEnvelopeX, line.start.x, line.end.x});
      maximumEnvelopeX =
          std::max({maximumEnvelopeX, line.start.x, line.end.x});
      minimumEnvelopeY =
          std::min({minimumEnvelopeY, line.start.y, line.end.y});
      maximumEnvelopeY =
          std::max({maximumEnvelopeY, line.start.y, line.end.y});
    }
    if (line.role == app::CreativePlayerSpawnPreviewLineRole::Facing) {
      facing = &line;
    }
  }

  return expect(spawn.accepted && roomBake.receipt.accepted,
                "ready preview fixture bakes") &&
         expect(geometry.active && geometry.accepted &&
                    geometry.status == cr::CreativePlayerSpawnStatus::Ready &&
                    !geometry.capacityExceeded && geometry.lineCount == 32U,
                "ready spawn produces one bounded exact preview") &&
         expect(roleCount(
                    geometry,
                    app::CreativePlayerSpawnPreviewLineRole::PhysicalEnvelope) ==
                    12U &&
                    roleCount(
                        geometry,
                        app::CreativePlayerSpawnPreviewLineRole::GroundClearance) ==
                        16U &&
                    roleCount(
                        geometry,
                        app::CreativePlayerSpawnPreviewLineRole::FloorContact) ==
                        2U &&
                    roleCount(
                        geometry,
                        app::CreativePlayerSpawnPreviewLineRole::CameraHeight) ==
                        1U &&
                    roleCount(geometry,
                              app::CreativePlayerSpawnPreviewLineRole::Facing) ==
                        1U,
                "preview roles expose envelope clearance contact camera and facing") &&
         expect(allFiniteAndNondegenerate,
                "every preview line is finite and nondegenerate") &&
         expect(near(minimumEnvelopeX, 0.45F) &&
                    near(maximumEnvelopeX, 1.55F) &&
                    near(minimumEnvelopeY,
                         0.25F +
                             static_cast<float>(
                                 iggy3d::kDefaultPlayerSkinMeters)) &&
                    near(maximumEnvelopeY, 2.05F),
                "wire box matches the planner standing overlap volume") &&
         expect(facing != nullptr && near(facing->start.x, 1.0F) &&
                    near(facing->start.y, 1.95F) &&
                    near(facing->start.z, -1.0F) &&
                    near(facing->end.x, 2.1F) &&
                    near(facing->end.z, -1.0F),
                "camera ray uses authored yaw and effective clearance length") &&
         expect(app::creativePlayerSpawnPreviewStatusLabel(
                    geometry.status) == "SPAWN: READY",
                "ready preview has a stable creator-facing label");
}

bool rejectedGeometryStaysVisibleAtTheAuthoredPoint() {
  cr::CreativeDocument obstructed = makeDocument();
  const cr::CreativeDocumentCreateReceipt blockedSpawn =
      addSpawn(obstructed, {-1.0, 0.25, 0.5});
  static_cast<void>(addBlocker(obstructed, {-1.0, 0.25, 0.5}));
  const cr::CreativeRoomBakeResult obstructedBake = bake(obstructed);
  const app::CreativePlayerSpawnPreviewGeometry blocked =
      app::planCreativePlayerSpawnPreview(
          {&obstructed, &obstructedBake,
           obstructed.findObject(blockedSpawn.objectId)});

  cr::CreativeDocument unsupported = makeDocument();
  cr::CreativePlayerSpawnSettings unsupportedSettings;
  unsupportedSettings.playerProfileId = "future_profile";
  const cr::CreativeDocumentCreateReceipt unsupportedSpawn =
      addSpawn(unsupported, {2.0, 0.25, 3.0}, unsupportedSettings, 0.0);
  const cr::CreativeRoomBakeResult unsupportedBake = bake(unsupported);
  const app::CreativePlayerSpawnPreviewGeometry unsupportedGeometry =
      app::planCreativePlayerSpawnPreview(
          {&unsupported, &unsupportedBake,
           unsupported.findObject(unsupportedSpawn.objectId)});
  const app::CreativePlayerSpawnPreviewGeometry missingBake =
      app::planCreativePlayerSpawnPreview(
          {&unsupported, nullptr,
           unsupported.findObject(unsupportedSpawn.objectId)});

  return expect(blocked.active && !blocked.accepted &&
                    blocked.status ==
                        cr::CreativePlayerSpawnStatus::Obstructed &&
                    blocked.lineCount == 32U,
                "obstructed spawn remains visible with physical failure status") &&
         expect(unsupportedGeometry.active &&
                    !unsupportedGeometry.accepted &&
                    unsupportedGeometry.status ==
                        cr::CreativePlayerSpawnStatus::UnsupportedProfile &&
                    near(unsupportedGeometry.groundedPositionMeters.x, 2.0F) &&
                    near(unsupportedGeometry.groundedPositionMeters.y, 0.25F) &&
                    near(unsupportedGeometry.groundedPositionMeters.z, 3.0F) &&
                    near(unsupportedGeometry.clearanceRadiusMeters, 0.45F),
                "unsupported profile preview retains authored position and settings") &&
         expect(missingBake.active && !missingBake.accepted &&
                    missingBake.status ==
                        cr::CreativePlayerSpawnStatus::MissingRoomBake &&
                    missingBake.lineCount == 32U,
                "missing bake is visible and fails closed") &&
         expect(app::creativePlayerSpawnPreviewStatusLabel(blocked.status) ==
                        "SPAWN: CLEAR BODY SPACE" &&
                    app::creativePlayerSpawnPreviewStatusLabel(
                        cr::CreativePlayerSpawnStatus::UnsupportedFloor) ==
                        "SPAWN: MOVE ONTO SUPPORTED FLOOR" &&
                    app::creativePlayerSpawnPreviewStatusLabel(
                        cr::CreativePlayerSpawnStatus::Unreachable) ==
                        "SPAWN: CONNECT WALKABLE FLOOR",
                "physical failures explain their repair action");
}

bool overlayUsesSelectionGateAndSemanticColor() {
  cr::CreativeDocument document = makeDocument();
  const cr::CreativeDocumentCreateReceipt spawn =
      addSpawn(document, {0.0, 0.25, 0.0});
  cr::CreativeAppState appState;
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeRoomBakeResult roomBake = bake(appState.facade.document());
  const cr::CreativeObject* spawnObject =
      appState.facade.findObject(spawn.objectId);

  app::CreativeEditorState editor;
  app::CreativeEditorSelectionFrame selection;
  selection.selectedId = static_cast<cr::Id>(spawn.objectId);
  selection.selected = spawnObject;
  selection.hasSelection = true;
  selection.selectionCount = 1U;
  app::CreativeEditorGizmoFrame gizmo;
  iggy3d::FrameInput frame;
  cr::CreativeSpatialProjectionRequest projection;
  app::CreativeEditorOverlayFrame output;
  app::CreativeEditorOverlayFrameRequest request{
      appState, editor, selection, gizmo, frame, projection};
  app::CreativePlayerSpawnPreviewCache cache;
  static_cast<void>(app::refreshCreativePlayerSpawnPreviewCache(
      cache, appState.facade.document(), &roomBake, spawnObject, 1U));
  request.playerSpawnPreview = &cache.geometry;
  request.gizmoThickness = 0.05F;
  app::appendCreativeEditorPlayerSpawnPreview(request, output);

  app::CreativeEditorOverlayFrame captureOutput;
  request.captureMode = true;
  app::appendCreativeEditorPlayerSpawnPreview(request, captureOutput);

  return expect(output.playerSpawnPreviewActive &&
                    output.playerSpawnPreviewAccepted &&
                    output.playerSpawnPreviewStatus ==
                        cr::CreativePlayerSpawnStatus::Ready &&
                    output.playerSpawnPreviewEdgeCount == 32U &&
                    output.combinedWireLines.size() == 32U,
                "selected spawn attaches one semantic overlay") &&
         expect(std::all_of(
                    output.combinedWireLines.begin(),
                    output.combinedWireLines.end(),
                    [spawn](
                        const iggy3d::RenderCreativeWireframeDebugLine& line) {
                      return line.objectId == spawn.objectId &&
                             line.objectKind == static_cast<std::uint32_t>(
                                                    cr::CreativeObjectKind::
                                                        SpawnPoint) &&
                             near(line.color.r, 0.20F) &&
                             near(line.color.g, 1.0F) &&
                             near(line.color.b, 0.35F);
                    }),
                "accepted spawn overlay uses one green object-owned role") &&
         expect(!captureOutput.playerSpawnPreviewActive &&
                    captureOutput.combinedWireLines.empty(),
                "scripted capture remains spawn-preview inert");
}

bool previewCacheMakesIdleInspectionFree() {
  cr::CreativeDocument document = makeDocument();
  const cr::CreativeDocumentCreateReceipt spawn =
      addSpawn(document, {0.0, 0.25, 0.0});
  const cr::CreativeRoomBakeResult roomBake = bake(document);
  const cr::CreativeObject* spawnObject = document.findObject(spawn.objectId);
  app::CreativePlayerSpawnPreviewCache cache;
  const bool first = app::refreshCreativePlayerSpawnPreviewCache(
      cache, document, &roomBake, spawnObject, 7U);
  bool idleChanged = false;
  for (std::size_t frame = 0U; frame < 300U; ++frame) {
    idleChanged =
        app::refreshCreativePlayerSpawnPreviewCache(
            cache, document, &roomBake, spawnObject, 7U) ||
        idleChanged;
  }
  const bool bakeChanged = app::refreshCreativePlayerSpawnPreviewCache(
      cache, document, &roomBake, spawnObject, 8U);
  const std::uint64_t refreshCount = cache.refreshCount;
  app::invalidateCreativePlayerSpawnPreviewCache(cache);

  return expect(first && !idleChanged && bakeChanged && refreshCount == 2U,
                "only source-key changes rebuild spawn geometry") &&
         expect(cache.refreshCount == 0U && !cache.valid,
                "explicit invalidation clears transient cache identity");
}

bool nonSpawnSelectionIsInert() {
  cr::CreativeDocument document = makeDocument();
  const cr::CreativeObject* floor = document.objects().empty()
                                        ? nullptr
                                        : &document.objects().front();
  const cr::CreativeRoomBakeResult roomBake = bake(document);
  const app::CreativePlayerSpawnPreviewGeometry geometry =
      app::planCreativePlayerSpawnPreview({&document, &roomBake, floor});
  return expect(!geometry.active && !geometry.accepted &&
                    geometry.status ==
                        cr::CreativePlayerSpawnStatus::NotRequested &&
                    geometry.lineCount == 0U,
                "non-spawn selection emits no spawn geometry");
}

bool planSymbolIsFiniteAndUsesTheSameYawConvention() {
  cr::CreativePlayerSpawnSettings settings;
  settings.validationRadiusMeters = 0.75;
  const app::CreativePlayerSpawnPlanSymbol ready =
      app::planCreativePlayerSpawnPlanSymbol(
          {2.0, 0.25, -3.0}, kPi * 0.5, settings, 0.5);
  settings.validationRadiusMeters =
      std::numeric_limits<double>::quiet_NaN();
  const app::CreativePlayerSpawnPlanSymbol invalid =
      app::planCreativePlayerSpawnPlanSymbol(
          {2.0, 0.25, -3.0}, std::numeric_limits<double>::quiet_NaN(),
          settings, 0.5);
  const app::CreativePlayerSpawnPlanSymbol badGrid =
      app::planCreativePlayerSpawnPlanSymbol(
          {2.0, 0.25, -3.0}, 0.0, {}, 0.0);

  return expect(ready.drawable && ready.runtimeReady &&
                    near(static_cast<float>(ready.bodyRadiusCells), 0.60F) &&
                    near(static_cast<float>(ready.clearanceRadiusCells), 1.50F) &&
                    near(static_cast<float>(ready.facingEndCells.x), 4.5F) &&
                    near(static_cast<float>(ready.facingEndCells.z), -3.0F),
                "plan symbol uses exact scale radius and yaw convention") &&
         expect(invalid.drawable && !invalid.runtimeReady &&
                    std::isfinite(invalid.clearanceRadiusCells) &&
                    cr::isFiniteCreativeVec3(invalid.facingEndCells),
                "invalid settings retain finite repair geometry") &&
         expect(!badGrid.drawable,
                "invalid grid suppresses unsafe plan geometry");
}

}  // namespace

int main() {
  const bool ok = readyGeometryMatchesTheExactPhysicalPlan() &&
                  rejectedGeometryStaysVisibleAtTheAuthoredPoint() &&
                  overlayUsesSelectionGateAndSemanticColor() &&
                  previewCacheMakesIdleInspectionFree() &&
                  nonSpawnSelectionIsInert() &&
                  planSymbolIsFiniteAndUsesTheSameYawConvention();
  if (ok) {
    std::cout << "creative_editor_player_spawn_preview_tests: PASS\n";
  }
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
