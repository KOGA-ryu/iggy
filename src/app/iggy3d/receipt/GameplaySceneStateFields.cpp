#include "app/iggy3d/receipt/ReceiptFields.hpp"

#include <charconv>
#include <string>
#include <string_view>
#include <utility>

#include "app/iggy3d/gameplay/GameplayFeedback.hpp"
#include "app/iggy3d/gameplay/MovementProof.hpp"
#include "app/iggy3d/debug/MovementDebugHud.hpp"
#include "app/iggy3d/debug/NpcBehaviorDebugHud.hpp"
#include "app/iggy3d/debug/PhysicsDebugHud.hpp"
#include "app/iggy3d/creative/ui/UiProjection.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/creative/bridge/UiCommandFrame.hpp"
#include "app/iggy3d/creative/bridge/UiInputFrame.hpp"
#include "app/iggy3d/creative/bridge/ViewportPickFrame.hpp"
#include "app/iggy3d/creative/bridge/WireframeFrame.hpp"
#include "app/iggy3d/window/RendererLifecycle.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"

namespace iggy3d {

void appendProductGameplaySceneStateFields(RenderReceipt& receipt, const ProductAppWindowState& window) {
  appendReceiptField(receipt, "position_hud_visible",
                     window.positionHud.visible);
  appendReceiptField(receipt, "position_hud_line_count",
                     static_cast<std::uint64_t>(window.positionHud.lineCount));
  appendReceiptField(receipt, "position_hud_debug_available",
                     window.positionHud.debugAvailable);
  appendReceiptField(receipt, "position_hud_status",
                     window.positionHud.status);
  appendReceiptField(receipt, "position_hud_reason_code",
                     window.positionHud.reasonCode);
  appendReceiptField(receipt, "position_hud_player_position_available",
                     window.positionHud.playerPositionAvailable);
  appendReceiptField(receipt, "position_hud_world_x",
                     floatReceiptValue(window.positionHud.worldX));
  appendReceiptField(receipt, "position_hud_world_y",
                     floatReceiptValue(window.positionHud.worldY));
  appendReceiptField(receipt, "position_hud_world_z",
                     floatReceiptValue(window.positionHud.worldZ));
  appendReceiptField(receipt, "position_hud_grid_x",
                     std::to_string(window.positionHud.gridX));
  appendReceiptField(receipt, "position_hud_grid_y",
                     std::to_string(window.positionHud.gridY));
  appendReceiptField(receipt, "position_hud_grid_z",
                     std::to_string(window.positionHud.gridZ));
  appendReceiptField(receipt, "position_hud_layer_index",
                     std::to_string(window.positionHud.layerIndex));
  appendReceiptField(receipt, "position_hud_facing",
                     window.positionHud.facing);
  appendReceiptField(receipt, "position_hud_yaw_degrees",
                     floatReceiptValue(window.positionHud.yawDegrees));
  appendReceiptField(receipt, "position_hud_pitch_degrees",
                     floatReceiptValue(window.positionHud.pitchDegrees));
  appendReceiptField(receipt, "gameplay_collision_surfaces_used",
                     window.gameplayCollision.surfacesUsed);
  appendReceiptField(receipt, "gameplay_collision_surface_count",
                     window.gameplayCollision.surfaceCount);
  appendReceiptField(receipt, "physics_movement_planner_enabled",
                     window.physicsMovementPlanner.enabled);
  appendReceiptField(receipt, "physics_movement_planner_requested",
                     window.physicsMovementPlanner.requested);
  appendReceiptField(receipt, "physics_movement_planner_used",
                     window.physicsMovementPlanner.used);
  appendReceiptField(receipt, "physics_movement_planner_status",
                     window.physicsMovementPlanner.status);
  appendReceiptField(receipt, "physics_movement_planner_reason_code",
                     window.physicsMovementPlanner.reasonCode);
  appendReceiptField(receipt, "target_discovered", window.targetDiscovered);
  appendReceiptField(receipt, "gameplay_target_status",
                     window.gameplayTarget.status);
  appendReceiptField(receipt, "gameplay_target_action",
                     window.gameplayTarget.action);
  appendReceiptField(receipt, "gameplay_target_entity_id",
                     window.gameplayTarget.entityId);
  appendReceiptField(receipt, "gameplay_target_stable_name",
                     window.gameplayTarget.stableName);
  appendReceiptField(receipt, "gameplay_target_kind", window.gameplayTarget.kind);
  appendReceiptField(receipt, "gameplay_target_distance_meters",
                     floatReceiptValue(window.gameplayTarget.distanceMeters));
  appendReceiptField(receipt, "gameplay_target_supports_command",
                     window.gameplayTarget.supportsCommand);
  appendReceiptField(receipt, "gameplay_outcome_status",
                     window.gameplayOutcome.status);
  appendReceiptField(receipt, "gameplay_outcome_target_active_after",
                     window.gameplayOutcome.targetActiveAfter);
  appendReceiptField(receipt, "gameplay_outcome_inventory_changed",
                     window.gameplayOutcome.inventoryChanged);
  appendReceiptField(receipt, "gameplay_outcome_item_id",
                     window.gameplayOutcome.itemId);
  appendReceiptField(receipt, "gameplay_outcome_item_count",
                     window.gameplayOutcome.itemCount);
  appendReceiptField(receipt, "gameplay_outcome_objective_changed",
                     window.gameplayOutcome.objectiveChanged);
  appendReceiptField(receipt, "gameplay_outcome_event_count",
                     window.gameplayOutcome.eventCount);
  appendReceiptField(receipt, "session_outcome", window.sessionOutcome);
  appendReceiptField(receipt, "gameplay_tape_requested",
                     window.gameplayTape.requested);
  appendReceiptField(receipt, "gameplay_tape_loaded", window.gameplayTape.loaded);
  appendReceiptField(receipt, "gameplay_tape_path", window.gameplayTape.path);
  appendReceiptField(receipt, "gameplay_tape_status", window.gameplayTape.status);
  appendReceiptField(receipt, "gameplay_tape_reason_code",
                     window.gameplayTape.reasonCode);
  appendReceiptField(receipt, "gameplay_tape_line_count",
                     window.gameplayTape.lineCount);
  appendReceiptField(receipt, "gameplay_tape_step_count",
                     window.gameplayTape.stepCount);
  appendReceiptField(receipt, "gameplay_tape_executed_step_count",
                     window.gameplayTape.executedStepCount);
  appendReceiptField(receipt, "gameplay_tape_expected_rejected_step_count",
                     window.gameplayTape.expectedRejectedStepCount);
  appendReceiptField(receipt, "gameplay_tape_expected_blocked_step_count",
                     window.gameplayTape.expectedBlockedStepCount);
  appendReceiptField(receipt, "gameplay_tape_failed_step",
                     window.gameplayTape.failedStep);
  appendReceiptField(receipt, "gameplay_tape_failed_source_line",
                     window.gameplayTape.failedSourceLine);
  appendReceiptField(receipt, "gameplay_tape_failed_action",
                     window.gameplayTape.failedAction);
  appendReceiptField(receipt, "gameplay_tape_failed_target",
                     window.gameplayTape.failedTarget);
  appendReceiptField(receipt, "gameplay_tape_failed_rejection",
                     window.gameplayTape.failedRejection);
  appendReceiptField(receipt, "gameplay_tape_failed_movement_block",
                     window.gameplayTape.failedMovementBlock);
  appendReceiptField(receipt, "gameplay_tape_last_action",
                     window.gameplayTape.lastAction);
  appendReceiptField(receipt, "gameplay_tape_last_target",
                     window.gameplayTape.lastTarget);
  appendReceiptField(receipt, "gameplay_tape_last_movement_block",
                     window.gameplayTape.lastMovementBlock);
  appendReceiptField(receipt, "gameplay_tape_key_collected",
                     window.gameplayTape.keyCollected);
  appendReceiptField(receipt, "gameplay_tape_secret_door_opened",
                     window.gameplayTape.secretDoorOpened);
  appendReceiptField(receipt, "gameplay_tape_treasure_collected",
                     window.gameplayTape.treasureCollected);
  appendReceiptField(receipt, "gameplay_tape_npc_targetable",
                     window.gameplayTape.npcTargetable);
  appendReceiptField(receipt, "gameplay_tape_npc_defeated",
                     window.gameplayTape.npcDefeated);
  appendReceiptField(receipt, "gameplay_tape_exit_objective_complete",
                     window.gameplayTape.exitObjectiveComplete);
  appendReceiptField(receipt, "gameplay_tape_loop_complete",
                     window.gameplayTape.loopComplete);
  appendReceiptField(receipt, "gameplay_tape_ai_command_logged",
                     window.gameplayTape.aiCommandLogged);
  appendReceiptField(receipt, "gameplay_tape_ai_attack_logged",
                     window.gameplayTape.aiAttackLogged);
  appendReceiptField(receipt, "gameplay_tape_ai_wait_logged",
                     window.gameplayTape.aiWaitLogged);
  appendReceiptField(receipt, "gameplay_tape_ai_player_damaged",
                     window.gameplayTape.aiPlayerDamaged);
  appendReceiptField(receipt, "gameplay_tape_ai_player_hp_before",
                     static_cast<std::uint64_t>(window.gameplayTape.aiPlayerHpBefore));
  appendReceiptField(receipt, "gameplay_tape_ai_player_hp_after",
                     static_cast<std::uint64_t>(window.gameplayTape.aiPlayerHpAfter));
  appendReceiptField(receipt, "gameplay_tape_ai_actor_id",
                     window.gameplayTape.aiActorId);
  appendReceiptField(receipt, "gameplay_tape_ai_target_id",
                     window.gameplayTape.aiTargetId);
  appendReceiptField(receipt, "gameplay_tape_ai_behavior",
                     window.gameplayTape.aiBehavior);
  appendReceiptField(receipt, "gameplay_tape_ai_intent",
                     window.gameplayTape.aiIntent);
  appendReceiptField(receipt, "gameplay_reach_gate", window.gameplayReachGate);
  appendReceiptField(receipt, "gameplay_last_rejection", window.gameplayLastRejection);
  appendReceiptField(receipt, "interaction_executed", window.interactionExecuted);
  appendReceiptField(receipt, "attack_executed", window.attackExecuted);
  appendReceiptField(receipt, "product_transition_last_action",
                     window.productTransition.lastAction);
  appendReceiptField(receipt, "product_transition_status",
                     window.productTransition.status);
  appendReceiptField(receipt, "product_transition_returned_to_gameplay",
                     window.productTransition.returnedToGameplay);
  appendReceiptField(receipt, "product_transition_returned_to_title",
                     window.productTransition.returnedToTitle);
  appendReceiptField(receipt, "product_transition_session_preserved",
                     window.productTransition.sessionPreserved);
  appendReceiptField(receipt, "camera_controller", window.viewport.cameraController);
  appendReceiptField(receipt, "camera_mode", window.viewport.cameraMode);
  appendReceiptField(receipt, "camera_controller_active",
                     window.viewport.cameraControllerActive);
  appendReceiptField(receipt, "look_input_used", window.viewport.lookInputUsed);
  appendReceiptField(receipt, "camera_input_source", window.viewport.cameraInputSource);
  appendReceiptField(receipt, "camera_yaw_degrees",
                     floatReceiptValue(window.viewport.cameraYawDegrees));
  appendReceiptField(receipt, "camera_pitch_degrees",
                     floatReceiptValue(window.viewport.cameraPitchDegrees));
  appendReceiptField(receipt, "camera_heading_visible",
                     window.viewport.cameraHeadingVisible);
  appendReceiptField(receipt, "creative_navigate_active",
                     window.creativeNavigateActive);
  appendReceiptField(receipt, "creative_fly_active",
                     window.viewport.creativeFlyActive);
  appendReceiptField(receipt, "creative_fly_status",
                     window.viewport.creativeFlyStatus);
  appendReceiptField(receipt, "creative_fly_reason_code",
                     window.viewport.creativeFlyReasonCode);
  appendReceiptField(receipt, "creative_fly_speed_mps",
                     floatReceiptValue(
                         window.viewport.creativeFlySpeedMetersPerSecond));
  appendReceiptField(receipt, "creative_fly_anchor_valid",
                     window.viewport.creativeFlyAnchorValid);
  appendReceiptField(receipt, "creative_fly_world_x",
                     floatReceiptValue(window.viewport.creativeFlyPositionMeters.x));
  appendReceiptField(receipt, "creative_fly_world_y",
                     floatReceiptValue(window.viewport.creativeFlyPositionMeters.y));
  appendReceiptField(receipt, "creative_fly_world_z",
                     floatReceiptValue(window.viewport.creativeFlyPositionMeters.z));
  appendReceiptField(receipt, "product_draw_item_count",
                     window.viewport.productDrawItemCount);
  appendReceiptField(receipt, "product_draw_grid_visible",
                     window.viewport.productDrawGridVisible);
  appendReceiptField(receipt, "product_draw_player_visible",
                     window.viewport.productDrawPlayerVisible);
  appendReceiptField(receipt, "product_draw_room_visible",
                     window.viewport.productDrawRoomVisible);
  appendReceiptField(receipt, "product_draw_objective_visible",
                     window.viewport.productDrawObjectiveVisible);
  appendReceiptField(receipt, "product_draw_target_indicator_visible",
                     window.viewport.productDrawTargetIndicatorVisible);
  appendReceiptField(receipt, "product_draw_door_visible",
                     window.viewport.productDrawDoorVisible);
  appendReceiptField(receipt, "product_draw_open_door_visible",
                     window.viewport.productDrawOpenDoorVisible);
  appendReceiptField(receipt, "product_draw_closed_door_visible",
                     window.viewport.productDrawClosedDoorVisible);
  appendReceiptField(receipt, "product_draw_debug_marker_count",
                     window.viewport.productDrawDebugMarkerCount);
  appendReceiptField(receipt, "product_draw_door_count",
                     window.viewport.productDrawDoorCount);
  appendReceiptField(receipt, "product_draw_open_door_count",
                     window.viewport.productDrawOpenDoorCount);
  appendReceiptField(receipt, "product_draw_closed_door_count",
                     window.viewport.productDrawClosedDoorCount);
  appendReceiptField(receipt, "product_draw_room_geometry_count",
                     window.viewport.productDrawRoomGeometryCount);
  appendReceiptField(receipt, "product_draw_floor_tile_count",
                     window.viewport.productDrawFloorTileCount);
  appendReceiptField(receipt, "product_draw_elevated_floor_tile_count",
                     window.viewport.productDrawElevatedFloorTileCount);
  appendReceiptField(receipt, "product_draw_ramp_tile_count",
                     window.viewport.productDrawRampTileCount);
  appendReceiptField(receipt, "product_draw_blocked_slope_tile_count",
                     window.viewport.productDrawBlockedSlopeTileCount);
  appendReceiptField(receipt, "product_draw_wall_tile_count",
                     window.viewport.productDrawWallTileCount);
  appendReceiptField(receipt, "product_draw_prop_visible",
                     window.viewport.productDrawPropVisible);
  appendReceiptField(receipt, "product_draw_prop_tile_count",
                     window.viewport.productDrawPropTileCount);
  appendReceiptField(receipt, "product_draw_room_editor_cursor_visible",
                     window.viewport.productDrawRoomEditorCursorVisible);
  appendReceiptField(receipt, "product_draw_room_editor_cursor_count",
                     window.viewport.productDrawRoomEditorCursorCount);
  appendReceiptField(receipt, "product_draw_room_editor_preview_visible",
                     window.viewport.productDrawRoomEditorPreviewVisible);
  appendReceiptField(receipt, "product_draw_room_editor_preview_count",
                     window.viewport.productDrawRoomEditorPreviewCount);
  appendReceiptField(receipt, "product_draw_physics_debug_visible",
                     window.viewport.productDrawPhysicsDebugVisible);
  appendReceiptField(receipt, "product_draw_physics_debug_item_count",
                     window.viewport.productDrawPhysicsDebugItemCount);
  appendReceiptField(receipt, "product_draw_physics_aabb_debug_count",
                     window.viewport.productDrawPhysicsAabbDebugCount);
  appendReceiptField(receipt,
                     "product_draw_physics_contact_normal_debug_count",
                     window.viewport.productDrawPhysicsContactNormalDebugCount);
  appendReceiptField(receipt, "product_draw_map_maker_grid_visible",
                     window.viewport.productDrawMapMakerGridVisible);
  appendReceiptField(receipt, "product_draw_map_maker_grid_dot_count",
                     window.viewport.productDrawMapMakerGridDotCount);
  appendReceiptField(receipt, "product_draw_map_maker_major_grid_dot_count",
                     window.viewport.productDrawMapMakerMajorGridDotCount);
  appendReceiptField(receipt, "product_draw_map_maker_cube_preview_visible",
                     window.viewport.productDrawMapMakerCubePreviewVisible);
  appendReceiptField(receipt, "product_draw_map_maker_cube_preview_count",
                     window.viewport.productDrawMapMakerCubePreviewCount);
  appendReceiptField(receipt, "product_view_projection",
                     window.viewport.productViewProjection);
  appendReceiptField(receipt, "product_view_yaw_applied",
                     window.viewport.productViewYawApplied);
  appendReceiptField(receipt, "product_view_pitch_applied",
                     window.viewport.productViewPitchApplied);
  appendReceiptField(receipt, "product_view_player_anchor_found",
                     window.viewport.productViewPlayerAnchorFound);
  appendReceiptField(receipt, "product_render_bridge_ready",
                     window.viewport.productRenderBridgeReady);
  appendReceiptField(receipt, "product_view_frame_ready",
                     window.viewport.productViewFrameReady);
  appendReceiptField(receipt, "product_view_frame_item_count",
                     window.viewport.productViewFrameItemCount);
  appendReceiptField(receipt, "product_view_frame_on_screen_item_count",
                     window.viewport.productViewFrameOnScreenItemCount);
  appendReceiptField(receipt, "product_view_frame_target_item_count",
                     window.viewport.productViewFrameTargetItemCount);
  appendReceiptField(receipt, "product_render_bridge_room_editor_cursor_visible",
                     window.viewport.productRenderBridgeRoomEditorCursorVisible);
  appendReceiptField(receipt, "product_render_bridge_room_editor_cursor_count",
                     window.viewport.productRenderBridgeRoomEditorCursorCount);
  appendReceiptField(receipt, "product_render_bridge_room_editor_preview_visible",
                     window.viewport.productRenderBridgeRoomEditorPreviewVisible);
  appendReceiptField(receipt, "product_render_bridge_room_editor_preview_count",
                     window.viewport.productRenderBridgeRoomEditorPreviewCount);
  appendReceiptField(receipt, "product_render_bridge_prop_visible",
                     window.viewport.productRenderBridgePropVisible);
  appendReceiptField(receipt, "product_render_bridge_prop_tile_count",
                     window.viewport.productRenderBridgePropTileCount);
  appendReceiptField(receipt, "product_render_bridge_physics_debug_visible",
                     window.viewport.productRenderBridgePhysicsDebugVisible);
  appendReceiptField(receipt, "product_render_bridge_physics_debug_item_count",
                     window.viewport.productRenderBridgePhysicsDebugItemCount);
  appendReceiptField(receipt,
                     "product_render_bridge_physics_aabb_debug_count",
                     window.viewport.productRenderBridgePhysicsAabbDebugCount);
  appendReceiptField(
      receipt,
      "product_render_bridge_physics_contact_normal_debug_count",
      window.viewport.productRenderBridgePhysicsContactNormalDebugCount);
  appendReceiptField(receipt, "product_render_bridge_map_maker_grid_visible",
                     window.viewport.productRenderBridgeMapMakerGridVisible);
  appendReceiptField(receipt, "product_render_bridge_map_maker_grid_dot_count",
                     window.viewport.productRenderBridgeMapMakerGridDotCount);
  appendReceiptField(receipt,
                     "product_render_bridge_map_maker_major_grid_dot_count",
                     window.viewport.productRenderBridgeMapMakerMajorGridDotCount);
  appendReceiptField(
      receipt,
      "product_render_bridge_map_maker_cube_preview_visible",
      window.viewport.productRenderBridgeMapMakerCubePreviewVisible);
  appendReceiptField(
      receipt,
      "product_render_bridge_map_maker_cube_preview_count",
      window.viewport.productRenderBridgeMapMakerCubePreviewCount);
  appendReceiptField(receipt, "product_feedback_bridge_ready",
                     window.viewport.productFeedbackBridgeReady);
  appendReceiptField(receipt, "product_feedback_bridge_line_count",
                     window.viewport.productFeedbackBridgeLineCount);
  appendReceiptField(receipt, "product_vulkan_room_mesh_cpu_ready",
                     window.viewport.productVulkanRoomMeshCpuReady);
  appendReceiptField(receipt, "product_vulkan_room_mesh_backend_presented",
                     window.viewport.productVulkanRoomMeshBackendPresented);
  appendReceiptField(receipt, "product_vulkan_room_mesh_source",
                     window.viewport.productVulkanRoomMeshSource);
  appendReceiptField(receipt, "product_vulkan_room_asset_id",
                     window.viewport.productVulkanRoomAssetId);
  appendReceiptField(receipt, "product_vulkan_room_floor_visible",
                     window.viewport.productVulkanRoomFloorVisible);
  appendReceiptField(receipt, "product_vulkan_room_wall_visible",
                     window.viewport.productVulkanRoomWallVisible);
  appendReceiptField(receipt, "product_vulkan_room_grid_visible",
                     window.viewport.productVulkanRoomGridVisible);
  appendReceiptField(receipt, "product_vulkan_room_source_mesh_count",
                     window.viewport.productVulkanRoomSourceMeshCount);
  appendReceiptField(receipt, "product_vulkan_room_vertex_count",
                     window.viewport.productVulkanRoomVertexCount);
  appendReceiptField(receipt, "product_vulkan_room_index_count",
                     window.viewport.productVulkanRoomIndexCount);
  appendReceiptField(receipt, "product_vulkan_room_draw_count",
                     window.viewport.productVulkanRoomDrawCount);
  appendReceiptField(receipt, "product_vulkan_room_floor_draw_count",
                     window.viewport.productVulkanRoomFloorDrawCount);
  appendReceiptField(receipt, "product_vulkan_room_wall_draw_count",
                     window.viewport.productVulkanRoomWallDrawCount);
  appendReceiptField(receipt, "product_vulkan_room_grid_line_draw_count",
                     window.viewport.productVulkanRoomGridLineDrawCount);
  appendReceiptField(receipt, "product_vulkan_room_grid_truncated",
                     window.viewport.productVulkanRoomGridTruncated);
  appendReceiptField(receipt, "product_vulkan_room_geometry_signature",
                     window.viewport.productVulkanRoomGeometrySignature);
}

}  // namespace iggy3d
