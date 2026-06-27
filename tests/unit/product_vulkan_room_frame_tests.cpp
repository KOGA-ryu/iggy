#include "app/iggy3d/ProductAsciiRoomActivation.hpp"
#include "app/iggy3d/ProductGameplayProjectionRefresh.hpp"
#include "app/iggy3d/window/ProductWindowRendererLifecycle.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "core/math/Mat4.hpp"
#include "core/math/Vec3.hpp"
#include "render/FrameInput.hpp"
#include "render/vulkan/BufferImageResources.hpp"

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
  return ok;
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
  return ok;
}

}  // namespace

int main() {
  const bool ok = productGameplayBuildsFirstPersonRoomFrame() &&
                  productReceiptCarriesFirstPersonRoomPathProof();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "product_vulkan_room_frame_tests=pass\n";
  return EXIT_SUCCESS;
}
