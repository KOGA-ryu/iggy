#include "render/vulkan/RenderLoopReceipt.hpp"

#include <string>

#include "render/vulkan/VulkanResult.hpp"

namespace iggy3d::vulkan {

void appendRenderLoopFrameReceipt(
    RenderReceipt& receipt, const VulkanFrameResult& result,
    const RenderLoopCreateInfo& createInfo, const FrameInput& frame,
    const SwapchainInfo& readySwapchain,
    const RenderLoopFramePlan& framePlan,
    const SwapchainAcquireResult& acquire,
    const RenderLoopSubmissionResult& submission) {
  const bool drawPackageRoom = framePlan.drawPackageRoom;
  const bool drawUiFrame = framePlan.drawUiFrame;
  const bool drawProxyPrimitives = framePlan.drawProxyPrimitives;
  const bool drawFirstRoom = framePlan.drawFirstRoom;
  const RenderLoopProxySceneFacts& proxyFacts = framePlan.proxyFacts;
  const DebugHudLayoutResult& debugHud = framePlan.debugHud;
  const ProjectileOverlayLayout& projectileOverlay =
      framePlan.projectileOverlay;
  const VkResult submitResult = submission.submitResult;
  const VkResult presentVkResult = submission.presentResult;
  const SwapchainPresentResult& presentResult = submission.presentation;

  appendReceiptField(receipt, "frame_status", vulkanFrameStatusName(result.status));
  appendReceiptField(receipt, "acquire_result", "VK_SUCCESS");
  appendReceiptField(receipt, "acquire_action", "submit");
  appendReceiptField(receipt, "acquired_image_index",
                     static_cast<std::uint64_t>(acquire.imageIndex));
  appendReceiptField(receipt, "command_recorded", result.commandRecorded);
  appendReceiptField(receipt, "submit_result", vkResultName(submitResult));
  appendReceiptField(receipt, "present_result", vkResultName(presentVkResult));
  appendReceiptField(receipt, "present_action",
                     presentResult.presented
                         ? (presentResult.recreateRequested ? "presented_then_recreate"
                                                            : "presented")
                         : (presentResult.recreateRequested ? "recreate" : "fail"));
  appendReceiptField(receipt, "presented", presentResult.presented);
  // branch-gate: BG-1078
  appendReceiptField(receipt, "record_mode",
                     drawPackageRoom ? "room_mesh_draws"
                     : drawProxyPrimitives ? "draw_primitives"
                     // branch-gate: BG-1078
                     : drawUiFrame ? "ui_primitives"
                                         : (drawFirstRoom ? "first_room" : "empty_frame"));
  // branch-gate: BG-1078
  appendReceiptField(receipt, "draw_count",
                     static_cast<std::uint64_t>(
                         drawPackageRoom
                             ? createInfo.firstRoomResources->geometry().indexedDraws.size() +
                                   createInfo.firstRoomResources->geometry()
                                       .staticMeshInstanceBatches.size()
                         : drawProxyPrimitives
                               ? renderLoopProxyDrawCount(proxyFacts)
                         // branch-gate: BG-1078
                         : drawUiFrame ? frame.ui.primitiveCount
                                             : (drawFirstRoom ? 1U : 0U)));
  appendReceiptField(receipt, "first_room_visible",
                     (drawPackageRoom || drawProxyPrimitives || drawFirstRoom) &&
                         presentResult.presented);
  appendReceiptField(receipt, "proxy_floor_visible", drawPackageRoom || drawProxyPrimitives);
  appendReceiptField(receipt, "proxy_room_bounds_visible",
                     drawPackageRoom || drawProxyPrimitives);
  appendReceiptField(receipt, "proxy_player_marker_visible",
                     drawPackageRoom || drawProxyPrimitives);
  appendReceiptField(receipt, "proxy_target_marker_visible",
                     (drawPackageRoom || drawProxyPrimitives) && proxyFacts.targetMarkerVisible);
  appendReceiptField(receipt, "proxy_objective_marker_visible",
                     (drawPackageRoom || drawProxyPrimitives) && proxyFacts.objectiveMarkerVisible);
  appendReceiptField(receipt, "fallback_room_proxy", drawProxyPrimitives);
  appendReceiptField(receipt, "fallback_reason",
                     drawProxyPrimitives ? "no_projected_room_geometry" : "not_applicable");
  if (drawPackageRoom) {
    const SceneRoomProjection& room = frame.projections.scene->room;
    const FirstRoomGeometryResources& geometry =
        createInfo.firstRoomResources->geometry();
    appendReceiptField(receipt, "room_asset_loaded", true);
    appendReceiptField(receipt, "room_asset_id", room.assetId);
    appendReceiptField(receipt, "room_asset_version",
                       static_cast<std::uint64_t>(room.version));
    appendReceiptField(receipt, "source_toml", room.sourceToml);
    appendReceiptField(receipt, "source_subset", room.sourceSubset);
    appendReceiptField(receipt, "room_static_mesh_count",
                       static_cast<std::uint64_t>(room.staticMeshCount));
    appendReceiptField(receipt, "room_material_count",
                       static_cast<std::uint64_t>(room.materialCount));
    appendReceiptField(receipt, "room_anchor_count",
                       static_cast<std::uint64_t>(room.anchorCount));
    appendReceiptField(receipt, "mesh_draw_count",
                       static_cast<std::uint64_t>(
                           geometry.indexedDraws.size() +
                           geometry.staticMeshInstanceBatches.size()));
    appendReceiptField(receipt, "indexed_draw_count",
                       static_cast<std::uint64_t>(
                           geometry.indexedDraws.size()));
    appendReceiptField(
        receipt, "static_mesh_instance_draw_count",
        static_cast<std::uint64_t>(geometry.staticMeshInstanceBatches.size()));
    appendReceiptField(receipt, "static_mesh_instance_count",
                       static_cast<std::uint64_t>(
                           geometry.staticMeshInstanceCount));
    appendReceiptField(receipt, "room_floor_draw_count",
                       static_cast<std::uint64_t>(geometry.roomFloorDrawCount));
    appendReceiptField(receipt, "room_wall_draw_count",
                       static_cast<std::uint64_t>(geometry.roomWallDrawCount));
    appendReceiptField(receipt, "room_grid_line_draw_count",
                       static_cast<std::uint64_t>(geometry.roomGridLineDrawCount));
    appendReceiptField(receipt, "room_grid_visible", geometry.roomGridVisible);
    appendReceiptField(receipt, "room_grid_truncated", geometry.roomGridTruncated);
    appendReceiptField(receipt, "vertex_buffer_uploaded",
                       createInfo.firstRoomResources->geometry().vertexBuffer.allocation.buffer !=
                           VK_NULL_HANDLE);
    appendReceiptField(receipt, "index_buffer_uploaded",
                       createInfo.firstRoomResources->geometry().indexBuffer.allocation.buffer !=
                           VK_NULL_HANDLE);
    appendReceiptField(receipt, "depth_enabled", true);
    appendReceiptField(receipt, "camera_projection", "perspective");
    appendReceiptField(receipt, "drawable_aspect",
                       std::to_string(frame.viewport.aspectRatio));
    appendReceiptField(receipt, "projection_application", "single");
    appendReceiptField(receipt, "floor_visible", room.floorVisible);
    appendReceiptField(receipt, "wall_visible", room.wallVisible);
    appendReceiptField(receipt, "opening_visible", room.openingVisible);
    appendReceiptField(receipt, "prop_visible", room.propVisible);
    appendReceiptField(receipt, "key_marker_visible",
                       proxyFacts.keyMarkerVisible || room.keyAnchorVisible);
    appendReceiptField(receipt, "dummy_marker_visible",
                       proxyFacts.dummyMarkerVisible || room.dummyAnchorVisible);
  }
  if (drawPackageRoom) {
    const FirstRoomGeometryResources& geometry =
        createInfo.firstRoomResources->geometry();
    appendReceiptField(receipt,
                       "creative_wireframe_debug_geometry_input_line_count",
                       static_cast<std::uint64_t>(
                           geometry.creativeWireframeDebugLineInputCount));
    appendReceiptField(receipt,
                       "creative_wireframe_debug_geometry_draw_count",
                       static_cast<std::uint64_t>(
                           geometry.creativeWireframeDebugGeometryDrawCount));
    appendReceiptField(receipt,
                       "creative_wireframe_debug_geometry_box_count",
                       static_cast<std::uint64_t>(
                           geometry.creativeWireframeDebugGeometryDrawCount));
    appendReceiptField(receipt,
                       "creative_wireframe_debug_geometry_skipped_count",
                       static_cast<std::uint64_t>(
                           geometry.creativeWireframeDebugGeometrySkippedCount));
    appendReceiptField(receipt,
                       "creative_wireframe_debug_geometry_status",
                       geometry.creativeWireframeDebugGeometryStatus);
    appendReceiptField(receipt,
                       "creative_wireframe_debug_geometry_reason_code",
                       geometry.creativeWireframeDebugGeometryReasonCode);
  } else {
    appendReceiptField(receipt,
                       "creative_wireframe_debug_geometry_input_line_count",
                       static_cast<std::uint64_t>(
                           frame.creativeWireframeDebug.lineCount));
    appendReceiptField(receipt,
                       "creative_wireframe_debug_geometry_draw_count",
                       static_cast<std::uint64_t>(0));
    appendReceiptField(receipt,
                       "creative_wireframe_debug_geometry_box_count",
                       static_cast<std::uint64_t>(0));
    appendReceiptField(receipt,
                       "creative_wireframe_debug_geometry_skipped_count",
                       static_cast<std::uint64_t>(0));
    const char* reasonCode =
        frame.creativeWireframeDebug.available
            ? (frame.creativeWireframeDebug.lineCount == 0U
                   ? "vulkan_creative_wireframe_debug_geometry_no_lines"
                   : "vulkan_creative_wireframe_debug_geometry_not_drawn")
            : "vulkan_creative_wireframe_debug_geometry_not_requested";
    appendReceiptField(receipt,
                       "creative_wireframe_debug_geometry_status",
                       reasonCode);
    appendReceiptField(receipt,
                       "creative_wireframe_debug_geometry_reason_code",
                       reasonCode);
  }
  appendReceiptField(receipt, "screenshot_capture",
                     drawFirstRoom && readySwapchain.transferSourceSupported &&
                             createInfo.frameCapture != nullptr &&
                             createInfo.frameCapture->ready()
                         ? "enabled"
                         : "unavailable");
  appendReceiptField(receipt, "room_proxy_visible",
                     drawPackageRoom || drawProxyPrimitives || (drawFirstRoom && presentResult.presented)
                         ? "true"
                         : "unavailable");
  appendReceiptField(receipt, "player_marker_visible",
                     drawPackageRoom || drawProxyPrimitives ? "true" : "unavailable");
  appendReceiptField(receipt, "marker_count",
                     static_cast<std::uint64_t>(
                         drawPackageRoom || drawProxyPrimitives ? proxyFacts.markerCount : 0U));
  appendReceiptField(receipt, "debug_hud_projected", debugHud.projected);
  appendReceiptField(receipt, "debug_hud_line_count",
                     static_cast<std::uint64_t>(debugHud.lineCount));
  appendReceiptField(receipt, "debug_hud_glyph_count",
                     static_cast<std::uint64_t>(debugHud.glyphCount));
  appendReceiptField(receipt, "debug_hud_rendered",
                     debugHud.projected && !debugHud.quads.empty() &&
                         result.commandRecorded && presentResult.presented);
  appendReceiptField(receipt, "debug_hud_record_mode",
                     debugHud.projected && !debugHud.quads.empty() ? "glyph_quads"
                                                                   : "unavailable");
  appendReceiptField(receipt, "ui_visible", frame.ui.visible);
  appendReceiptField(receipt, "ui_rendered",
                     drawUiFrame && result.commandRecorded && presentResult.presented);
  appendReceiptField(receipt, "ui_overlay_rect_count",
                     static_cast<std::uint64_t>(frame.ui.rectCount));
  appendReceiptField(receipt, "ui_text_glyph_count",
                     static_cast<std::uint64_t>(frame.ui.textGlyphCount));
  appendReceiptField(receipt, "ui_text_glyph_quad_count",
                     static_cast<std::uint64_t>(frame.ui.textGlyphQuadCount));
  appendReceiptField(receipt, "ui_primitive_count",
                     static_cast<std::uint64_t>(frame.ui.primitiveCount));
  appendReceiptField(receipt, "projectile_visual_projected",
                     projectileOverlay.projected);
  appendReceiptField(receipt, "projectile_visual_count",
                     static_cast<std::uint64_t>(projectileOverlay.projectileCount));
  appendReceiptField(receipt, "projectile_marker_count",
                     static_cast<std::uint64_t>(projectileOverlay.markerCount));
  appendReceiptField(receipt, "projectile_trail_rect_count",
                     static_cast<std::uint64_t>(projectileOverlay.trailRectCount));
  appendReceiptField(receipt, "projectile_overlay_rect_count",
                     static_cast<std::uint64_t>(projectileOverlay.rects.size()));
  appendReceiptField(receipt, "projectile_impact_visible",
                     projectileOverlay.impactVisible);
  appendReceiptField(receipt, "projectile_rendered",
                     projectileOverlay.projected && !projectileOverlay.rects.empty() &&
                         result.commandRecorded && presentResult.presented);
  appendReceiptField(receipt, "projectile_record_mode",
                     projectileOverlay.projected && !projectileOverlay.rects.empty()
                         ? "overlay_rects"
                         : "unavailable");
}

}  // namespace iggy3d::vulkan
