#include "app/iggy3d/ascii_room/Activation.hpp"
#include "app/iggy3d/gameplay/ProjectionRefresh.hpp"
#include "app/iggy3d/window/FramePresenter.hpp"
#include "app/iggy3d/window/RendererLifecycle.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "core/math/Mat4.hpp"
#include "core/math/Vec3.hpp"
#include "render/FrameInput.hpp"
#include "render/vulkan/BufferImageResources.hpp"
#include "runtime/physics/PhysicsAabbCollider.hpp"
#include "runtime/physics/PhysicsFrameStats.hpp"
#include "runtime/physics/PhysicsKinematicMotor.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool nearlyEqual(float lhs, float rhs, float epsilon = 0.0001F) {
  return std::fabs(lhs - rhs) <= epsilon;
}

bool nearlyEqual(const iggy3d::Mat4& lhs,
                 const iggy3d::Mat4& rhs,
                 float epsilon = 0.0001F) {
  for (std::size_t i = 0; i < lhs.m.size(); ++i) {
    if (!nearlyEqual(lhs.m[i], rhs.m[i], epsilon)) {
      return false;
    }
  }
  return true;
}

std::size_t countDebugKind(const iggy3d::DebugProjectionResult& debug,
                           iggy3d::DebugProjectionKind kind) {
  std::size_t count = 0U;
  for (const iggy3d::DebugProjectionItem& item : debug.items) {
    if (item.kind == kind) {
      ++count;
    }
  }
  return count;
}

iggy3d::PhysicsAabbCollider debugCollider(iggy3d::PhysicsBodyId bodyId,
                                          iggy3d::Vec3 center,
                                          iggy3d::Vec3 halfExtents) {
  iggy3d::PhysicsAabbCollider collider;
  collider.bodyId = bodyId;
  collider.worldCenterMeters = center;
  collider.halfExtentsMeters = halfExtents;
  collider.bounds = iggy3d::aabbFromCenterExtents(center, halfExtents);
  return collider;
}

iggy3d::ProductAppWindowState makeGameplayWindow(
    std::optional<iggy3d::Session>& session) {
  iggy3d::ProductAppWindowState window;
  window.asciiRoomDraftText =
      "#######\n"
      "#P..$.#\n"
      "#..E..#\n"
      "#######\n";
  window.asciiRoomDraftRoomId = "vulkan_product_room_frame";
  window.asciiRoomDraftSourceName = "unit/vulkan_product_room_frame.iggyroom.txt";
  window.viewport.cameraYawDegrees = 18.0F;
  window.viewport.cameraPitchDegrees = -3.0F;
  const iggy3d::ProductAsciiRoomActivationResult activation =
      iggy3d::activateProductAsciiRoomPreview(session, window);
  expect(activation.ok, "ascii room activation ok");
  return window;
}

iggy3d::PhysicsFrameStats makeReadyPlayerPhysicsStats() {
  iggy3d::PhysicsFrameStats stats = iggy3d::buildPhysicsFrameStats();
  stats.sourcePacketCount = 1U;
  stats.playerBakedSurfaceCount = 6U;
  stats.playerBakedColliderCount = 4U;
  stats.playerSkippedSurfaceCount = 1U;
  stats.playerIterationCount = 2U;
  return stats;
}

iggy3d::PhysicsFrameStats makeWarningPlayerPhysicsStats() {
  iggy3d::PhysicsFrameStats stats = makeReadyPlayerPhysicsStats();
  stats.ok = false;
  stats.status = iggy3d::PhysicsFrameStatsStatus::PacketFailed;
  stats.reasonCode = iggy3d::physicsFrameStatsStatusName(stats.status);
  stats.upstreamReasonCode = "physics_frame_stats_packet_failed";
  stats.failedPacketCount = 1U;
  return stats;
}

void seedPhysicsMovementStats(std::optional<iggy3d::Session>& session,
                              const iggy3d::PhysicsFrameStats& stats) {
  iggy3d::MovementResult movement;
  movement.physicsFrameStatsAvailable = true;
  movement.physicsFrameStats = stats;
  iggy3d::SessionState& state = session->mutableStateForOwnedSystems();
  state.transient.lastMovementResultAvailable = true;
  state.transient.lastMovementResult = movement;
}

void seedMovementDebugFacts(iggy3d::ProductAppWindowState& window) {
  window.gameplayMovementDebugAvailable = true;
  window.gameplayMovementStatus = "moved";
  window.gameplayMovementReasonCode = "movement_ok";
  window.gameplayMovementBlockedReason = "movement_ok";
  window.gameplayMovementHitSurfaceId = "none";
  window.gameplayMovementGroundSnapApplied = true;
  window.gameplayMovementPolicyBand = "flat";
  window.gameplayMovementSpeedMultiplier = 1.0F;
  window.gameplayMovementFinalX = -1.0F;
  window.gameplayMovementFinalY = 0.0F;
  window.gameplayMovementFinalZ = -1.0F;
}

void seedPhysicsMovementStatsAndGeometry(std::optional<iggy3d::Session>& session) {
  iggy3d::MovementResult movement;
  movement.physicsFrameStatsAvailable = true;
  movement.physicsFrameStats = makeReadyPlayerPhysicsStats();
  movement.physicsDebugGeometryAvailable = true;
  movement.physicsDebugAabbColliders.push_back(
      debugCollider({101}, {0.0F, 0.0F, 0.0F}, {2.0F, 0.05F, 2.0F}));
  movement.physicsDebugAabbColliders.push_back(
      debugCollider({102}, {1.5F, 0.8F, 0.0F}, {0.1F, 0.8F, 2.0F}));
  movement.physicsDebugAabbSourceSurfaceIds = {"floor", "wall"};
  iggy3d::PhysicsKinematicMotorHit hit;
  hit.bodyId = {102};
  hit.colliderIndex = 1U;
  hit.centerMeters = {1.2F, 0.9F, 0.0F};
  movement.physicsDebugHits.push_back(hit);
  movement.physicsDebugHitSourceSurfaceIds = {"wall"};

  iggy3d::SessionState& state = session->mutableStateForOwnedSystems();
  state.transient.lastMovementResultAvailable = true;
  state.transient.lastMovementResult = movement;
}

bool productGameplayBuildsFirstPersonRoomFrame() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "session created")) {
    return false;
  }

  const iggy3d::ProductGameplayProjectionFrame frame =
      iggy3d::buildProductGameplayProjectionFrame(
          iggy3d::ProductGameplayProjectionFrameRequest{
              session, window, false, false, iggy3d::ProductRendererRequest::Vulkan});
  iggy3d::applyGameplayProjectionMetrics(window, frame.scenePtr(), frame.debugPtr(),
                                         frame.drawListPtr(), frame.viewportFramePtr(),
                                         frame.renderBridgePtr(), frame.viewVisible);
  const iggy3d::FrameInput renderFrame =
      iggy3d::makeProductVulkanFrame(frame.scene, frame.debug, 9U, 1280U, 720U,
                                     window.viewport.cameraYawDegrees,
                                     window.viewport.cameraPitchDegrees);
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(renderFrame.projections.scene->room);
  const iggy3d::Mat4 expectedClipFromWorld =
      renderFrame.camera.clipFromView * renderFrame.camera.viewFromWorld;

  bool ok = true;
  ok = expect(window.gameplayActive, "gameplay active") && ok;
  ok = expect(window.activeRoom.loaded, "active room loaded") && ok;
  ok = expect(frame.hasGameplayProjection, "projection frame built") && ok;
  ok = expect(frame.viewVisible, "gameplay view visible") && ok;
  ok = expect(frame.scene.room.loaded, "scene room loaded") && ok;
  ok = expect(frame.scene.room.assetId == window.activeRoom.roomId,
              "scene room matches active room") &&
       ok;
  ok = expect(renderFrame.projections.scene == &frame.scene,
              "frame input carries scene projection") &&
       ok;
  ok = expect(iggy3d::renderCameraModeName(renderFrame.camera.mode) == "first_person",
              "first-person render camera mode") &&
       ok;
  ok = expect(iggy3d::validateFrameInput(renderFrame) ==
                  iggy3d::FrameInputStatus::Valid,
              "frame input validates") &&
       ok;
  ok = expect(iggy3d::isFinite(renderFrame.camera.worldEye), "finite eye") && ok;
  ok = expect(iggy3d::isFinite(renderFrame.camera.worldForward), "finite forward") &&
       ok;
  ok = expect(iggy3d::isFinite(renderFrame.camera.viewFromWorld),
              "finite view matrix") &&
       ok;
  ok = expect(iggy3d::isFinite(renderFrame.camera.clipFromView),
              "finite projection matrix") &&
       ok;
  ok = expect(iggy3d::isFinite(renderFrame.camera.clipFromWorld),
              "finite view-projection matrix") &&
       ok;
  ok = expect(nearlyEqual(renderFrame.camera.clipFromWorld, expectedClipFromWorld),
              "clipFromWorld composition") &&
       ok;
  ok = expect(geometry.ready, "room mesh CPU geometry ready") && ok;
  ok = expect(geometry.sourceRoomAssetId == window.activeRoom.roomId,
              "geometry asset id") &&
       ok;
  ok = expect(!geometry.vertices.empty(), "room mesh vertices present") && ok;
  ok = expect(!geometry.indices.empty(), "room mesh indices present") && ok;
  ok = expect(!geometry.indexedDraws.empty(), "room mesh draws present") && ok;
  ok = expect(geometry.roomFloorDrawCount > 0U, "floor draw count positive") && ok;
  ok = expect(geometry.roomWallDrawCount > 0U, "wall draw count positive") && ok;
  ok = expect(window.viewport.productVulkanRoomMeshCpuReady,
              "window CPU mesh proof ready") &&
       ok;
  ok = expect(window.viewport.productVulkanRoomMeshSource == "scene_room_projection",
              "window CPU mesh source") &&
       ok;
  ok = expect(window.viewport.productVulkanRoomAssetId == window.activeRoom.roomId,
              "window CPU mesh room id") &&
       ok;
  ok = expect(window.viewport.productVulkanRoomVertexCount == geometry.vertices.size(),
              "window vertex count") &&
       ok;
  ok = expect(window.viewport.productVulkanRoomIndexCount == geometry.indices.size(),
              "window index count") &&
       ok;
  ok = expect(window.viewport.productVulkanRoomDrawCount ==
                  geometry.indexedDraws.size(),
              "window draw count") &&
       ok;
  ok = expect(frame.topDownMapOverlay.purpose == "minimap",
              "Vulkan player map is minimap") &&
       ok;
  ok = expect(frame.topDownMapOverlay.size == "compact",
              "Vulkan player map compact") &&
       ok;
  ok = expect(!frame.movementHud.visible, "movement debug HUD hidden") && ok;
  ok = expect(frame.movementHud.status == "not_requested",
              "movement debug HUD not requested") &&
       ok;
  ok = expect(!frame.npcBehaviorHud.visible, "NPC debug HUD hidden") && ok;
  ok = expect(frame.npcBehaviorHud.status == "not_requested",
              "NPC debug HUD not requested") &&
       ok;
  ok = expect(!frame.physicsHud.visible, "physics debug HUD hidden") && ok;
  ok = expect(frame.physicsHud.status == "not_requested",
              "physics debug HUD not requested") &&
       ok;
  ok = expect(!frame.positionHud.visible, "position HUD hidden by default") && ok;
  ok = expect(frame.positionHud.status == "not_requested",
              "position HUD default not requested") &&
       ok;
  return ok;
}

bool productGameplayDefaultOverlayKeepsDebugHudsCleanWithData() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "clean overlay session created")) {
    return false;
  }
  seedMovementDebugFacts(window);
  seedPhysicsMovementStats(session, makeReadyPlayerPhysicsStats());

  const iggy3d::ProductGameplayProjectionFrame frame =
      iggy3d::buildProductGameplayProjectionFrame(
          iggy3d::ProductGameplayProjectionFrameRequest{
              session, window, true, false, iggy3d::ProductRendererRequest::Vulkan});

  iggy3d::ProductAppOptions options;
  iggy3d::ProductWorldTemplate world;
  iggy3d::FrontendState frontend;
  iggy3d::FrontendSettings settings;
  settings.devToolsEnabled = true;
  settings.debugOverlayEnabled = false;
  iggy3d::ProductSaveBridgeResult saves;
  const iggy3d::RenderReceipt receipt =
      iggy3d::buildProductAppReceipt(options, world, frontend, settings, window, saves);

  bool ok = true;
  ok = expect(!frame.movementHud.visible,
              "default overlay hides movement debug HUD with data") &&
       ok;
  ok = expect(frame.movementHud.debugAvailable,
              "hidden movement HUD keeps data availability") &&
       ok;
  ok = expect(frame.movementHud.status == "not_requested",
              "hidden movement HUD status is gated") &&
       ok;
  ok = expect(frame.movementHud.reasonCode == "not_requested",
              "hidden movement HUD reason is gated") &&
       ok;
  ok = expect(frame.movementHud.lines.empty(), "hidden movement HUD has no lines") &&
       ok;
  ok = expect(!frame.npcBehaviorHud.visible, "default overlay hides NPC HUD") &&
       ok;
  ok = expect(frame.npcBehaviorHud.status == "not_requested",
              "default overlay NPC status") &&
       ok;
  ok = expect(!frame.physicsHud.visible, "default overlay hides physics HUD") &&
       ok;
  ok = expect(frame.physicsHud.status == "not_requested",
              "default overlay physics status") &&
       ok;
  ok = expect(frame.physicsHud.lineCount == 4U,
              "hidden physics HUD keeps line count") &&
       ok;
  ok = expect(frame.debug.physicsDebugHudLines.size() == 4U,
              "default overlay preserves physics debug lines") &&
       ok;
  ok = expect(!frame.drawList.physicsDebugVisible,
              "default overlay hides physics debug geometry") &&
       ok;
  ok = expect(!frame.positionHud.visible,
              "default overlay hides position HUD") &&
       ok;
  ok = expect(frame.positionHud.debugAvailable,
              "hidden position HUD keeps projection availability") &&
       ok;
  ok = expect(frame.positionHud.status == "not_requested",
              "hidden position HUD status is gated") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "movement_debug_hud_visible",
                                      "false"),
              "receipt clean movement HUD hidden") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "movement_debug_hud_debug_overlay_enabled",
                                      "false"),
              "receipt clean movement overlay disabled") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "movement_debug_hud_debug_available",
                                      "true"),
              "receipt clean movement data available") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "movement_debug_hud_status",
                                      "not_requested"),
              "receipt clean movement HUD status") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "movement_debug_hud_reason_code",
                                      "not_requested"),
              "receipt clean movement HUD reason") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "physics_debug_hud_visible",
                                      "false"),
              "receipt clean physics HUD hidden") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "physics_debug_hud_status",
                                      "not_requested"),
              "receipt clean physics HUD status") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "position_hud_visible",
                                      "false"),
              "receipt position HUD hidden") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "position_hud_status",
                                      "not_requested"),
              "receipt position HUD not requested") &&
       ok;
  return ok;
}

bool productPhysicsDebugHudUnavailableWithoutMovementStats() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "physics unavailable session created")) {
    return false;
  }

  const iggy3d::ProductGameplayProjectionFrame frame =
      iggy3d::buildProductGameplayProjectionFrame(
          iggy3d::ProductGameplayProjectionFrameRequest{
              session, window, true, true, iggy3d::ProductRendererRequest::Vulkan});

  iggy3d::ProductAppOptions options;
  iggy3d::ProductWorldTemplate world;
  iggy3d::FrontendState frontend;
  iggy3d::FrontendSettings settings;
  settings.devToolsEnabled = true;
  settings.debugOverlayEnabled = true;
  iggy3d::ProductSaveBridgeResult saves;
  const iggy3d::RenderReceipt receipt =
      iggy3d::buildProductAppReceipt(options, world, frontend, settings, window, saves);

  bool ok = true;
  ok = expect(frame.debug.physicsDebugHudLines.empty(),
              "no movement stats means no physics projection lines") &&
       ok;
  ok = expect(!frame.physicsHud.visible, "unavailable physics HUD hidden") && ok;
  ok = expect(frame.physicsHud.status == "physics_debug_unavailable",
              "unavailable physics HUD status") &&
       ok;
  ok = expect(frame.physicsHud.reasonCode == "physics_debug_unavailable",
              "unavailable physics HUD reason") &&
       ok;
  ok = expect(window.physicsDebugHudDebugAvailable,
              "window physics debug projection available") &&
       ok;
  ok = expect(window.physicsDebugHudLineCount == 0U,
              "window physics debug line count zero") &&
       ok;
  ok = expect(window.physicsDebugHudStatus == "physics_debug_unavailable",
              "window physics debug unavailable status") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "physics_debug_hud_visible",
                                      "false"),
              "receipt physics debug hidden") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "physics_debug_hud_line_count",
                                      "0"),
              "receipt physics debug line count") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "physics_debug_hud_dev_tools_enabled",
                                      "true"),
              "receipt physics debug dev tools enabled") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "physics_debug_hud_debug_overlay_enabled",
                                      "true"),
              "receipt physics debug overlay enabled") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "physics_debug_hud_debug_available",
                                      "true"),
              "receipt physics debug projection available") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "physics_debug_hud_status",
                                      "physics_debug_unavailable"),
              "receipt physics debug unavailable status") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "physics_debug_hud_reason_code",
                                      "physics_debug_unavailable"),
              "receipt physics debug unavailable reason") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "physics_debug_hud_has_warnings",
                                      "false"),
              "receipt physics debug warnings false") &&
       ok;
  return ok;
}

bool productPhysicsDebugHudReadyFromMovementStats() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "physics ready session created")) {
    return false;
  }
  seedPhysicsMovementStats(session, makeReadyPlayerPhysicsStats());

  const iggy3d::ProductGameplayProjectionFrame frame =
      iggy3d::buildProductGameplayProjectionFrame(
          iggy3d::ProductGameplayProjectionFrameRequest{
              session, window, true, true, iggy3d::ProductRendererRequest::Vulkan});

  bool ok = true;
  ok = expect(frame.debug.physicsDebugHudLines.size() == 4U,
              "ready physics projection line count") &&
       ok;
  ok = expect(frame.debug.physicsDebugHudLines[0] ==
                  "PHYS packets=1 failed=0 bodies=0 colliders=0 contacts=0 sensors=0",
              "ready physics summary line") &&
       ok;
  ok = expect(frame.debug.physicsDebugHudLines[3] ==
                  "PHYS MOVE kin_iter=0 kin_hits=0 player_iter=2 "
                  "player_hits=0 baked=4 skipped=1",
              "ready physics movement line") &&
       ok;
  ok = expect(frame.physicsHud.visible, "ready physics HUD visible") && ok;
  ok = expect(frame.physicsHud.status == "physics_debug_ready",
              "ready physics HUD status") &&
       ok;
  ok = expect(frame.physicsHud.reasonCode == "physics_debug_ready",
              "ready physics HUD reason") &&
       ok;
  ok = expect(frame.physicsHud.lineCount == 4U, "ready physics HUD line count") &&
       ok;
  ok = expect(!frame.physicsHud.hasWarnings, "ready physics HUD no warnings") &&
       ok;
  ok = expect(window.physicsDebugHudVisible, "window physics HUD visible") && ok;
  ok = expect(window.physicsDebugHudLineCount == 4U,
              "window physics HUD line count") &&
       ok;
  ok = expect(window.physicsDebugHudStatus == "physics_debug_ready",
              "window physics HUD ready status") &&
       ok;
  ok = expect(!window.physicsDebugHudHasWarnings,
              "window physics HUD no warnings") &&
       ok;
  ok = expect(frame.positionHud.visible, "ready position HUD visible") && ok;
  ok = expect(frame.positionHud.status == "position_hud_ready",
              "ready position HUD status") &&
       ok;
  ok = expect(frame.positionHud.playerPositionAvailable,
              "ready position HUD has player position") &&
       ok;
  ok = expect(frame.positionHud.lines.size() == 3U,
              "ready position HUD line count") &&
       ok;
  ok = expect(window.positionHudVisible, "window position HUD visible") && ok;
  ok = expect(window.positionHudLineCount == 3U,
              "window position HUD line count") &&
       ok;
  ok = expect(window.positionHudFacing == "north",
              "window position HUD facing") &&
       ok;
  return ok;
}

bool productPhysicsDebugHudWarningFromMovementStats() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "physics warning session created")) {
    return false;
  }
  seedPhysicsMovementStats(session, makeWarningPlayerPhysicsStats());

  const iggy3d::ProductGameplayProjectionFrame frame =
      iggy3d::buildProductGameplayProjectionFrame(
          iggy3d::ProductGameplayProjectionFrameRequest{
              session, window, true, true, iggy3d::ProductRendererRequest::Vulkan});

  bool ok = true;
  ok = expect(frame.debug.physicsDebugHudLines.size() == 5U,
              "warning physics projection line count") &&
       ok;
  ok = expect(frame.debug.physicsDebugHudLines[4] ==
                  "PHYS WARN status=physics_debug_snapshot_stats_failed "
                  "upstream=physics_frame_stats_packet_failed bp=0 pen=0 impulse=0",
              "warning physics line") &&
       ok;
  ok = expect(frame.physicsHud.visible, "warning physics HUD visible") && ok;
  ok = expect(frame.physicsHud.hasWarnings, "warning physics HUD has warnings") &&
       ok;
  ok = expect(window.physicsDebugHudHasWarnings,
              "window physics HUD has warnings") &&
       ok;
  return ok;
}

bool productPhysicsDebugGeometryFromMovementStatsWhenGatedOn() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "physics geometry session created")) {
    return false;
  }
  seedPhysicsMovementStatsAndGeometry(session);

  const iggy3d::ProductGameplayProjectionFrame frame =
      iggy3d::buildProductGameplayProjectionFrame(
          iggy3d::ProductGameplayProjectionFrameRequest{
              session, window, true, true, iggy3d::ProductRendererRequest::Vulkan});
  iggy3d::applyGameplayProjectionMetrics(window, frame.scenePtr(), frame.debugPtr(),
                                         frame.drawListPtr(), frame.viewportFramePtr(),
                                         frame.renderBridgePtr(), frame.viewVisible);

  iggy3d::ProductAppOptions options;
  iggy3d::ProductWorldTemplate world;
  iggy3d::FrontendState frontend;
  iggy3d::FrontendSettings settings;
  settings.devToolsEnabled = true;
  settings.debugOverlayEnabled = true;
  iggy3d::ProductSaveBridgeResult saves;
  const iggy3d::RenderReceipt receipt =
      iggy3d::buildProductAppReceipt(options, world, frontend, settings, window, saves);

  bool ok = true;
  ok = expect(frame.debug.physicsDebugHudLines.size() == 4U,
              "geometry stats still produce physics HUD lines") &&
       ok;
  ok = expect(countDebugKind(frame.debug, iggy3d::DebugProjectionKind::PhysicsAabb) ==
                  2U,
              "geometry projection AABB count") &&
       ok;
  ok = expect(countDebugKind(frame.debug,
                            iggy3d::DebugProjectionKind::PhysicsContactNormal) ==
                  1U,
              "geometry projection hit count") &&
       ok;
  ok = expect(frame.physicsHud.visible, "geometry physics HUD visible") && ok;
  ok = expect(frame.physicsHud.status == "physics_debug_ready",
              "geometry physics HUD ready") &&
       ok;
  ok = expect(frame.drawList.physicsDebugVisible,
              "geometry draw-list physics debug visible") &&
       ok;
  ok = expect(frame.drawList.physicsDebugItemCount == 3U,
              "geometry draw-list physics debug count") &&
       ok;
  ok = expect(frame.drawList.physicsAabbDebugCount == 2U,
              "geometry draw-list AABB count") &&
       ok;
  ok = expect(frame.drawList.physicsContactNormalDebugCount == 1U,
              "geometry draw-list hit count") &&
       ok;
  ok = expect(frame.renderBridge.physicsDebugVisible,
              "geometry render bridge physics debug visible") &&
       ok;
  ok = expect(frame.renderBridge.physicsDebugItemCount == 3U,
              "geometry render bridge physics debug count") &&
       ok;
  ok = expect(frame.renderBridge.physicsAabbDebugCount == 2U,
              "geometry render bridge AABB count") &&
       ok;
  ok = expect(frame.renderBridge.physicsContactNormalDebugCount == 1U,
              "geometry render bridge hit count") &&
       ok;
  ok = expect(window.viewport.productDrawPhysicsDebugVisible,
              "window draw physics debug visible") &&
       ok;
  ok = expect(window.viewport.productDrawPhysicsDebugItemCount == 3U,
              "window draw physics debug count") &&
       ok;
  ok = expect(window.viewport.productDrawPhysicsAabbDebugCount == 2U,
              "window draw physics AABB count") &&
       ok;
  ok = expect(window.viewport.productDrawPhysicsContactNormalDebugCount == 1U,
              "window draw physics contact count") &&
       ok;
  ok = expect(window.viewport.productRenderBridgePhysicsDebugVisible,
              "window render physics debug visible") &&
       ok;
  ok = expect(window.viewport.productRenderBridgePhysicsDebugItemCount == 3U,
              "window render physics debug count") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(
                  receipt, "product_draw_physics_debug_visible", "true"),
              "receipt draw physics debug visible") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(
                  receipt, "product_draw_physics_debug_item_count", "3"),
              "receipt draw physics debug count") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(
                  receipt, "product_draw_physics_aabb_debug_count", "2"),
              "receipt draw physics AABB count") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(
                  receipt,
                  "product_draw_physics_contact_normal_debug_count",
                  "1"),
              "receipt draw physics contact count") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(
                  receipt, "product_render_bridge_physics_debug_visible", "true"),
              "receipt render physics debug visible") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(
                  receipt,
                  "product_render_bridge_physics_debug_item_count",
                  "3"),
              "receipt render physics debug count") &&
       ok;
  return ok;
}

bool productPhysicsDebugGeometryRequiresDebugOverlayGate() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "physics geometry gated session created")) {
    return false;
  }
  seedPhysicsMovementStatsAndGeometry(session);

  const iggy3d::ProductGameplayProjectionFrame frame =
      iggy3d::buildProductGameplayProjectionFrame(
          iggy3d::ProductGameplayProjectionFrameRequest{
              session, window, true, false, iggy3d::ProductRendererRequest::Vulkan});

  return expect(frame.debug.physicsDebugHudLines.size() == 4U,
                "gated geometry keeps stats HUD packet") &&
         expect(countDebugKind(frame.debug, iggy3d::DebugProjectionKind::PhysicsAabb) ==
                    0U,
                "gated geometry omits AABB projection") &&
         expect(countDebugKind(frame.debug,
                              iggy3d::DebugProjectionKind::PhysicsContactNormal) ==
                    0U,
                "gated geometry omits hit projection") &&
         expect(!frame.drawList.physicsDebugVisible,
                "gated geometry draw-list hidden") &&
         expect(frame.drawList.physicsDebugItemCount == 0U,
                "gated geometry draw-list count zero");
}

bool productReceiptCarriesFirstPersonRoomPathProof() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "receipt session created")) {
    return false;
  }

  const iggy3d::ProductGameplayProjectionFrame frame =
      iggy3d::buildProductGameplayProjectionFrame(
          iggy3d::ProductGameplayProjectionFrameRequest{
              session, window, false, false, iggy3d::ProductRendererRequest::Vulkan});
  iggy3d::applyGameplayProjectionMetrics(window, frame.scenePtr(), frame.debugPtr(),
                                         frame.drawListPtr(), frame.viewportFramePtr(),
                                         frame.renderBridgePtr(), frame.viewVisible);
  window.productVulkanRendererRequested = true;
  window.productVulkanRendererCreated = true;
  window.productVulkanRendererReady = true;
  window.productVulkanFrameSubmitted = true;
  window.productVulkanFrameSubmittedCount = 1U;
  window.productVulkanRenderingPath = "package_room_meshes";
  window.productVulkanRecordMode = "room_mesh_draws";
  window.viewport.productVulkanRoomMeshBackendPresented = true;

  iggy3d::ProductAppOptions options;
  iggy3d::ProductWorldTemplate world;
  iggy3d::FrontendState frontend;
  iggy3d::FrontendSettings settings;
  iggy3d::ProductSaveBridgeResult saves;
  const iggy3d::RenderReceipt receipt =
      iggy3d::buildProductAppReceipt(options, world, frontend, settings, window, saves);
  const bool expectedReady = iggy3d::productWindowVulkanBackendBuilt();

  bool ok = true;
  ok = expect(iggy3d::hasReceiptField(receipt, "gameplay_active", "true"),
              "receipt gameplay active") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "product_vulkan_room_mesh_cpu_ready",
                                      "true"),
              "receipt CPU mesh ready") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "product_vulkan_room_mesh_backend_presented",
                                      "true"),
              "receipt backend room path presented") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "product_vulkan_room_mesh_source",
                                      "scene_room_projection"),
              "receipt room mesh source") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "product_vulkan_room_asset_id",
                                      "vulkan_product_room_frame"),
              "receipt room asset id") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "product_vulkan_rendering_path",
                                      "package_room_meshes"),
              "receipt rendering path") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "product_vulkan_record_mode",
                                      "room_mesh_draws"),
              "receipt record mode") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "product_vulkan_gameplay_ready",
                                      expectedReady ? "true" : "false"),
              "receipt gameplay readiness") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(
                  receipt,
                  "product_vulkan_gameplay_status",
                  expectedReady ? "product_vulkan_gameplay_ready"
                                : "product_vulkan_backend_unavailable"),
              "receipt gameplay readiness status") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt, "top_down_map_purpose", "minimap"),
              "receipt minimap purpose") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt, "top_down_map_size", "compact"),
              "receipt compact minimap") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "movement_debug_hud_visible",
                                      "false"),
              "receipt movement debug hidden") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "npc_behavior_debug_hud_visible",
                                      "false"),
              "receipt NPC debug hidden") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "physics_debug_hud_visible",
                                      "false"),
              "receipt physics debug hidden") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt,
                                      "physics_debug_hud_status",
                                      "not_requested"),
              "receipt physics debug not requested") &&
       ok;
  return ok;
}

bool vulkanGameplayFrameCarriesPositionHudUiOverlay() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "position hud vulkan session created")) {
    return false;
  }

  const iggy3d::ProductGameplayProjectionFrame visibleProjection =
      iggy3d::buildProductGameplayProjectionFrame(
          iggy3d::ProductGameplayProjectionFrameRequest{
              session, window, true, true, iggy3d::ProductRendererRequest::Vulkan});
  iggy3d::ProductVulkanGameplayFrame visible =
      iggy3d::buildProductVulkanGameplayFrame(visibleProjection, 12U, 1280U,
                                              720U, 15.0F, -2.0F);
  const iggy3d::FrameInput& visibleFrame =
      iggy3d::refreshProductVulkanGameplayFrameInput(visible);

  const iggy3d::ProductGameplayProjectionFrame hiddenProjection =
      iggy3d::buildProductGameplayProjectionFrame(
          iggy3d::ProductGameplayProjectionFrameRequest{
              session, window, true, false, iggy3d::ProductRendererRequest::Vulkan});
  iggy3d::ProductVulkanGameplayFrame hidden =
      iggy3d::buildProductVulkanGameplayFrame(hiddenProjection, 13U, 1280U,
                                              720U, 15.0F, -2.0F);
  const iggy3d::FrameInput& hiddenFrame =
      iggy3d::refreshProductVulkanGameplayFrameInput(hidden);

  return expect(visibleProjection.positionHud.visible,
                "position hud projection visible") &&
         expect(visibleFrame.ui.visible, "position hud ui visible") &&
         expect(visibleFrame.ui.rectCount == 1U,
                "position hud ui background rect") &&
         expect(visibleFrame.ui.textGlyphCount > 0U,
                "position hud ui text glyph count") &&
         expect(visibleFrame.ui.textGlyphQuadCount > visibleFrame.ui.textGlyphCount,
                "position hud ui glyph quads") &&
         expect(visibleFrame.ui.primitiveCount > visibleFrame.ui.rectCount,
                "position hud ui primitive count") &&
         expect(!hiddenProjection.positionHud.visible,
                "hidden position hud projection hidden") &&
         expect(!hiddenFrame.ui.visible, "hidden position hud has no vulkan ui") &&
         expect(hiddenFrame.ui.rectCount == 0U, "hidden position hud no rects") &&
         expect(hiddenFrame.ui.textGlyphQuadCount == 0U,
                "hidden position hud no glyph quads");
}

bool vulkanGameplayFrameCarriesMovementTuningUiOverlay() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "movement tuning vulkan session created")) {
    return false;
  }

  const iggy3d::ProductGameplayProjectionFrame projection =
      iggy3d::buildProductGameplayProjectionFrame(
          iggy3d::ProductGameplayProjectionFrameRequest{
              session, window, false, false, iggy3d::ProductRendererRequest::Vulkan});

  iggy3d::ProductVulkanGameplayFrame hidden =
      iggy3d::buildProductVulkanGameplayFrame(
          projection,
          14U,
          1280U,
          720U,
          0.0F,
          0.0F,
          window.gameplayMovementTuning,
          window.gameplayMovementTuningSelectedField,
          false);
  const iggy3d::FrameInput& hiddenFrame =
      iggy3d::refreshProductVulkanGameplayFrameInput(hidden);

  window.gameplayMovementTuningVisible = true;
  window.gameplayMovementTuningSelectedField =
      iggy3d::ProductGameplayMovementTuningField::JumpImpulse;
  iggy3d::ProductVulkanGameplayFrame visible =
      iggy3d::buildProductVulkanGameplayFrame(
          projection,
          15U,
          1280U,
          720U,
          0.0F,
          0.0F,
          window.gameplayMovementTuning,
          window.gameplayMovementTuningSelectedField,
          window.gameplayMovementTuningVisible);
  const iggy3d::FrameInput& visibleFrame =
      iggy3d::refreshProductVulkanGameplayFrameInput(visible);

  return expect(!hiddenFrame.ui.visible,
                "hidden movement tuning has no vulkan ui") &&
         expect(hiddenFrame.ui.rectCount == 0U,
                "hidden movement tuning no rects") &&
         expect(hiddenFrame.ui.textGlyphQuadCount == 0U,
                "hidden movement tuning no glyph quads") &&
         expect(visibleFrame.ui.visible,
                "movement tuning vulkan ui visible") &&
         expect(visibleFrame.ui.rectCount >= 2U,
                "movement tuning background and selected row rects") &&
         expect(visibleFrame.ui.textGlyphCount > 0U,
                "movement tuning text glyph count") &&
         expect(visibleFrame.ui.textGlyphQuadCount > visibleFrame.ui.textGlyphCount,
                "movement tuning glyph quads") &&
         expect(visibleFrame.ui.primitiveCount > visibleFrame.ui.rectCount,
                "movement tuning primitive count includes text");
}

bool vulkanGameplayFrameCarriesDevToolsOverlay() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "dev tools vulkan session created")) {
    return false;
  }

  const iggy3d::ProductGameplayProjectionFrame projection =
      iggy3d::buildProductGameplayProjectionFrame(
          iggy3d::ProductGameplayProjectionFrameRequest{
              session, window, false, false, iggy3d::ProductRendererRequest::Vulkan});

  iggy3d::ProductVulkanGameplayFrame hidden =
      iggy3d::buildProductVulkanGameplayFrame(
          projection,
          16U,
          1280U,
          720U,
          0.0F,
          0.0F,
          window.gameplayMovementTuning,
          window.gameplayMovementTuningSelectedField,
          false,
          false,
          iggy3d::FrontendDevToolsCategory::Movement);
  const iggy3d::FrameInput& hiddenFrame =
      iggy3d::refreshProductVulkanGameplayFrameInput(hidden);

  iggy3d::ProductVulkanGameplayFrame visible =
      iggy3d::buildProductVulkanGameplayFrame(
          projection,
          17U,
          1280U,
          720U,
          0.0F,
          0.0F,
          window.gameplayMovementTuning,
          window.gameplayMovementTuningSelectedField,
          false,
          true,
          iggy3d::FrontendDevToolsCategory::Movement);
  const iggy3d::FrameInput& visibleFrame =
      iggy3d::refreshProductVulkanGameplayFrameInput(visible);

  return expect(!hiddenFrame.ui.visible,
                "hidden dev tools has no vulkan ui") &&
         expect(hiddenFrame.ui.rectCount == 0U,
                "hidden dev tools no rects") &&
         expect(hiddenFrame.ui.textGlyphQuadCount == 0U,
                "hidden dev tools no glyph quads") &&
         expect(visibleFrame.ui.visible,
                "dev tools vulkan ui visible") &&
         expect(visibleFrame.ui.rectCount >= 2U,
                "dev tools panel and selected row rects") &&
         expect(visibleFrame.ui.textGlyphCount > 0U,
                "dev tools text glyph count") &&
         expect(visibleFrame.ui.textGlyphQuadCount > visibleFrame.ui.textGlyphCount,
                "dev tools glyph quads") &&
         expect(visibleFrame.ui.primitiveCount > visibleFrame.ui.rectCount,
                "dev tools primitive count includes text");
}

bool creativeMapMakerFrameCarriesGridOverlay() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "map maker frame session created")) {
    return false;
  }
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  window.viewport.creativeFlyAnchorValid = true;
  window.viewport.creativeFlyPositionMeters = {0.0F, 2.0F, 0.0F};

  const iggy3d::ProductGameplayProjectionFrame projection =
      iggy3d::buildProductGameplayProjectionFrame(
          iggy3d::ProductGameplayProjectionFrameRequest{
              session, window, false, false, iggy3d::ProductRendererRequest::Vulkan});
  iggy3d::applyGameplayProjectionMetrics(window,
                                         projection.scenePtr(),
                                         projection.debugPtr(),
                                         projection.drawListPtr(),
                                         projection.viewportFramePtr(),
                                         projection.renderBridgePtr(),
                                         projection.viewVisible);
  iggy3d::ProductVulkanGameplayFrame vulkanFrame =
      iggy3d::buildProductVulkanGameplayFrame(
          projection,
          14U,
          1280U,
          720U,
          window.viewport.cameraYawDegrees,
          window.viewport.cameraPitchDegrees);
  const iggy3d::FrameInput& frameInput =
      iggy3d::refreshProductVulkanGameplayFrameInput(vulkanFrame);
  bool cubeMeshProjected = false;
  std::uint64_t gridMeshCount = 0;
  for (const iggy3d::SceneRoomMeshItem& mesh : projection.scene.room.meshes) {
    if (mesh.role == "grid") {
      ++gridMeshCount;
    }
    if (mesh.id == "map_maker.unit_cube_preview" &&
        mesh.role == "prop" &&
        nearlyEqual(mesh.size.x, 1.0F) &&
        nearlyEqual(mesh.size.y, 1.0F) &&
        nearlyEqual(mesh.size.z, 1.0F)) {
      cubeMeshProjected = true;
    }
  }

  return expect(projection.mapMakerGrid.visible, "map maker grid visible") &&
         expect(projection.mapMakerGrid.dotCount > 0U,
                "map maker grid dot count positive") &&
         expect(projection.mapMakerGrid.layerCount > 1U,
                "map maker grid has vertical layers") &&
         expect(projection.mapMakerGridOverlay.visible,
                "map maker overlay visible") &&
         expect(gridMeshCount == projection.mapMakerGrid.dotCount,
                "map maker grid projected as 3D dot meshes") &&
         expect(projection.mapMakerCubePreview.visible,
                "map maker cube preview visible") &&
         expect(cubeMeshProjected, "map maker cube projected as prop mesh") &&
         expect(projection.mapMakerHud.visible, "map maker hud visible") &&
         expect(projection.cameraAnchorOverrideAvailable,
                "map maker camera override available") &&
         expect(projection.drawList.mapMakerGridVisible,
                "map maker draw-list visible") &&
         expect(projection.drawList.mapMakerGridDotCount ==
                    projection.mapMakerGrid.dotCount,
                "map maker draw-list dot count") &&
         expect(projection.drawList.mapMakerCubePreviewVisible,
                "map maker draw-list cube visible") &&
         expect(projection.drawList.mapMakerCubePreviewCount == 1U,
                "map maker draw-list cube count") &&
         expect(projection.renderBridge.mapMakerGridVisible,
                "map maker render bridge visible") &&
         expect(projection.renderBridge.mapMakerCubePreviewVisible,
                "map maker render bridge cube visible") &&
         expect(projection.renderBridge.mapMakerCubePreviewCount == 1U,
                "map maker render bridge cube count") &&
         expect(window.mapMakerActive, "map maker window active") &&
         expect(window.mapMakerGridVisible, "map maker window grid visible") &&
         expect(window.mapMakerGridLayerCount == projection.mapMakerGrid.layerCount,
                "map maker window layer count") &&
         expect(window.viewport.productDrawMapMakerGridVisible,
                "map maker draw receipt visible") &&
         expect(window.viewport.productDrawMapMakerCubePreviewVisible,
                "map maker cube draw receipt visible") &&
         expect(window.viewport.productRenderBridgeMapMakerGridVisible,
                "map maker bridge receipt visible") &&
         expect(window.viewport.productRenderBridgeMapMakerCubePreviewVisible,
                "map maker cube bridge receipt visible") &&
         expect(frameInput.ui.visible, "map maker vulkan UI visible") &&
         expect(frameInput.ui.rectCount > 0U, "map maker vulkan rects") &&
         expect(frameInput.ui.textGlyphCount > 0U,
                "map maker hud vulkan text");
}

}  // namespace

int main() {
  const bool ok = productGameplayBuildsFirstPersonRoomFrame() &&
                  productGameplayDefaultOverlayKeepsDebugHudsCleanWithData() &&
                  productPhysicsDebugHudUnavailableWithoutMovementStats() &&
                  productPhysicsDebugHudReadyFromMovementStats() &&
                  productPhysicsDebugHudWarningFromMovementStats() &&
                  productPhysicsDebugGeometryFromMovementStatsWhenGatedOn() &&
                  productPhysicsDebugGeometryRequiresDebugOverlayGate() &&
                  productReceiptCarriesFirstPersonRoomPathProof() &&
                  vulkanGameplayFrameCarriesPositionHudUiOverlay() &&
                  vulkanGameplayFrameCarriesMovementTuningUiOverlay() &&
                  vulkanGameplayFrameCarriesDevToolsOverlay() &&
                  creativeMapMakerFrameCarriesGridOverlay();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "product_vulkan_room_frame_tests=pass\n";
  return EXIT_SUCCESS;
}
