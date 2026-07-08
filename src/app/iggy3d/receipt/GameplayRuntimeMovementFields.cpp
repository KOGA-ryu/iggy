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

void appendProductGameplayRuntimeMovementFields(RenderReceipt& receipt, const ProductAppWindowState& window, const ProductMovementProofPacket& movementProof, std::uint64_t runtimeStateHash) {
  appendReceiptField(receipt, "runtime_session_created", window.gameplay.runtimeSessionCreated);
  appendReceiptField(receipt, "gameplay_active", window.gameplay.gameplayActive);
  appendReceiptField(receipt, "runtime_state_hash", runtimeStateHash);
  appendReceiptField(receipt, "gameplay_view_visible",
                     window.viewport.gameplayViewVisible);
  appendReceiptField(receipt, "scene_item_count", window.gameplay.sceneItemCount);
  appendReceiptField(receipt, "debug_item_count", window.gameplay.debugItemCount);
  appendReceiptField(receipt, "player_visible", window.gameplay.playerVisible);
  appendReceiptField(receipt, "room_visible", window.gameplay.roomVisible);
  appendReceiptField(receipt, "objective_visible", window.gameplay.objectiveVisible);
  appendReceiptField(receipt, "renderer_mutated_runtime",
                     window.gameplay.rendererMutatedRuntime);
  appendReceiptField(receipt, "scripted_gameplay_smoke",
                     window.gameplay.scriptedGameplaySmoke);
  appendReceiptField(receipt, "gameplay_input_used", window.gameplay.gameplayInputUsed);
  appendReceiptField(receipt, "gameplay_input_source", window.gameplay.gameplayInputSource);
  appendReceiptField(receipt, "gameplay_command_submitted", window.gameplay.gameplayCommand.submitted);
  appendReceiptField(receipt, "gameplay_command_kind", window.gameplay.gameplayCommand.kind);
  appendReceiptField(receipt, "gameplay_command_status", window.gameplay.gameplayCommand.status);
  appendReceiptField(receipt, "gameplay_command_accepted", window.gameplay.gameplayCommand.accepted);
  appendReceiptField(receipt, "gameplay_tick_advanced", window.gameplay.gameplayTickAdvanced);
  appendReceiptField(receipt, "gameplay_tick_reason_code",
                     window.gameplay.gameplayTickReasonCode);
  appendReceiptField(receipt, "player_position_changed", window.gameplay.playerPositionChanged);
  appendReceiptField(receipt, "gameplay_movement_attempted",
                     movementProof.attempted);
  appendReceiptField(receipt, "gameplay_movement_blocked",
                     movementProof.blocked);
  appendReceiptField(receipt, "gameplay_movement_status",
                     movementProof.status);
  appendReceiptField(receipt, "gameplay_movement_debug_available",
                     movementProof.debugAvailable);
  appendReceiptField(receipt, "gameplay_movement_reason_code",
                     movementProof.reasonCode);
  appendReceiptField(receipt, "gameplay_movement_blocked_reason",
                     movementProof.blockedReason);
  appendReceiptField(receipt, "gameplay_movement_hit_surface_id",
                     movementProof.hitSurfaceId);
  appendReceiptField(receipt, "gameplay_movement_ground_snap_applied",
                     movementProof.groundSnapApplied);
  appendReceiptField(receipt, "gameplay_movement_clamped",
                     movementProof.movementClamped);
  appendReceiptField(receipt, "gameplay_movement_slid",
                     movementProof.movementSlid);
  appendReceiptField(receipt, "gameplay_movement_collision_sweep_count",
                     movementProof.collisionSweepCount);
  appendReceiptField(receipt, "gameplay_movement_policy_band",
                     movementProof.policyBand);
  appendReceiptField(receipt, "gameplay_movement_slope_travel_direction",
                     movementProof.slopeTravelDirection);
  appendReceiptField(receipt, "gameplay_movement_slope_angle_degrees",
                     floatReceiptValue(movementProof.slopeAngleDegrees));
  appendReceiptField(receipt, "gameplay_movement_speed_multiplier",
                     floatReceiptValue(movementProof.speedMultiplier));
  appendReceiptField(receipt, "gameplay_movement_start_x",
                     floatReceiptValue(movementProof.startX));
  appendReceiptField(receipt, "gameplay_movement_start_y",
                     floatReceiptValue(movementProof.startY));
  appendReceiptField(receipt, "gameplay_movement_start_z",
                     floatReceiptValue(movementProof.startZ));
  appendReceiptField(receipt, "gameplay_movement_final_x",
                     floatReceiptValue(movementProof.finalX));
  appendReceiptField(receipt, "gameplay_movement_final_y",
                     floatReceiptValue(movementProof.finalY));
  appendReceiptField(receipt, "gameplay_movement_final_z",
                     floatReceiptValue(movementProof.finalZ));
  appendReceiptField(receipt, "gameplay_movement_horizontal_distance_meters",
                     floatReceiptValue(movementProof.horizontalDistanceMeters));
  appendReceiptField(receipt, "gameplay_movement_vertical_delta_meters",
                     floatReceiptValue(movementProof.verticalDeltaMeters));
  appendReceiptField(receipt, "gameplay_movement_ground_velocity_x",
                     floatReceiptValue(movementProof.groundVelocityX));
  appendReceiptField(receipt, "gameplay_movement_ground_velocity_z",
                     floatReceiptValue(movementProof.groundVelocityZ));
  appendReceiptField(receipt, "gameplay_movement_state",
                     movementProof.stateName);
  appendReceiptField(receipt, "movement_state",
                     movementProof.stateName);
  appendReceiptField(receipt, "movement_grounded",
                     movementProof.grounded);
  appendReceiptField(receipt, "movement_vertical_velocity_mps",
                     floatReceiptValue(movementProof.verticalVelocityMetersPerSecond));
  appendReceiptField(receipt, "movement_horizontal_speed_mps",
                     floatReceiptValue(movementProof.horizontalSpeedMetersPerSecond));
  appendReceiptField(receipt, "movement_hit_surface_id",
                     movementProof.hitSurfaceId);
  appendReceiptField(receipt, "wall_run_candidate_available",
                     movementProof.wallRunCandidateAvailable);
  appendReceiptField(receipt, "wall_run_candidate_status",
                     movementProof.wallRunCandidateStatus);
  appendReceiptField(receipt, "wall_run_candidate_reason_code",
                     movementProof.wallRunCandidateReasonCode);
  appendReceiptField(receipt, "wall_run_side", movementProof.wallRunSide);
  appendReceiptField(receipt, "wall_run_surface_id",
                     movementProof.wallRunSurfaceId);
  appendReceiptField(receipt, "wall_run_normal_x",
                     floatReceiptValue(movementProof.wallRunNormalX));
  appendReceiptField(receipt, "wall_run_normal_y",
                     floatReceiptValue(movementProof.wallRunNormalY));
  appendReceiptField(receipt, "wall_run_normal_z",
                     floatReceiptValue(movementProof.wallRunNormalZ));
  appendReceiptField(receipt, "wall_run_approach_speed_mps",
                     floatReceiptValue(
                         movementProof.wallRunApproachSpeedMetersPerSecond));
  appendReceiptField(receipt, "wall_run_active",
                     movementProof.wallRunActive);
  appendReceiptField(receipt, "wall_run_status",
                     movementProof.wallRunStatus);
  appendReceiptField(receipt, "wall_run_reason_code",
                     movementProof.wallRunReasonCode);
  appendReceiptField(receipt, "wall_run_remaining_s",
                     floatReceiptValue(movementProof.wallRunRemainingSeconds));
  appendReceiptField(receipt, "wall_run_duration_s",
                     floatReceiptValue(movementProof.wallRunDurationSeconds));
  appendReceiptField(receipt, "wall_run_gravity_multiplier",
                     floatReceiptValue(movementProof.wallRunGravityMultiplier));
  appendReceiptField(receipt, "wall_run_speed_multiplier",
                     floatReceiptValue(movementProof.wallRunSpeedMultiplier));
  appendReceiptField(receipt, "gameplay_movement_grade_percent",
                     floatReceiptValue(movementProof.gradePercent));
  appendReceiptField(receipt, "gameplay_movement_profile",
                     movementProof.profile);
  appendReceiptField(receipt, "gameplay_movement_max_speed_mps",
                     floatReceiptValue(movementProof.maxSpeedMetersPerSecond));
  appendReceiptField(receipt, "gameplay_jump_requested",
                     window.gameplay.gameplayJump.requested);
  appendReceiptField(receipt, "gameplay_jump_accepted",
                     window.gameplay.gameplayJump.accepted);
  appendReceiptField(receipt, "gameplay_jump_active", window.gameplay.gameplayJump.active);
  appendReceiptField(receipt, "gameplay_jump_status", window.gameplay.gameplayJump.status);
  appendReceiptField(receipt, "gameplay_jump_reason_code",
                     window.gameplay.gameplayJump.reasonCode);
  appendReceiptField(receipt, "gameplay_jump_velocity_mps",
                     floatReceiptValue(window.gameplay.gameplayJump.velocityMetersPerSecond));
  appendReceiptField(receipt, "gameplay_jump_coyote_seconds_remaining",
                     floatReceiptValue(window.gameplay.gameplayJump.coyoteSecondsRemaining));
  appendReceiptField(receipt, "gameplay_jump_buffer_seconds_remaining",
                     floatReceiptValue(window.gameplay.gameplayJump.bufferSecondsRemaining));
  appendReceiptField(receipt, "gameplay_jump_held", window.gameplay.gameplayJump.held);
  appendReceiptField(receipt, "gameplay_jump_cut_applied",
                     window.gameplay.gameplayJump.cutApplied);
  appendReceiptField(receipt, "gameplay_jump_ground_y",
                     floatReceiptValue(window.gameplay.gameplayJump.groundY));
  appendReceiptField(receipt, "gameplay_jump_start_y",
                     floatReceiptValue(window.gameplay.gameplayJump.startY));
  appendReceiptField(receipt, "gameplay_jump_final_y",
                     floatReceiptValue(window.gameplay.gameplayJump.finalY));
  appendReceiptField(receipt, "gameplay_jump_height_meters",
                     floatReceiptValue(window.gameplay.gameplayJump.heightMeters));
  appendReceiptField(receipt, "gameplay_reset_triggered",
                     window.gameplay.gameplayReset.triggered);
  appendReceiptField(receipt, "gameplay_reset_status",
                     window.gameplay.gameplayReset.status);
  appendReceiptField(receipt, "gameplay_reset_reason_code",
                     window.gameplay.gameplayReset.reasonCode);
  appendReceiptField(receipt, "gameplay_reset_spawn_anchor_id",
                     window.gameplay.gameplayReset.spawnAnchorId);
  appendReceiptField(receipt, "gameplay_reset_source_anchor_id",
                     window.gameplay.gameplayReset.sourceAnchorId);
  appendReceiptField(receipt, "gameplay_reset_start_y",
                     floatReceiptValue(window.gameplay.gameplayReset.startY));
  appendReceiptField(receipt, "gameplay_reset_final_y",
                     floatReceiptValue(window.gameplay.gameplayReset.finalY));
  appendReceiptField(receipt, "gameplay_traversal_requested",
                     window.gameplay.gameplayTraversal.requested);
  appendReceiptField(receipt, "gameplay_traversal_consumed",
                     window.gameplay.gameplayTraversal.consumed);
  appendReceiptField(receipt, "gameplay_traversal_accepted",
                     window.gameplay.gameplayTraversal.accepted);
  appendReceiptField(receipt, "gameplay_traversal_fallback_jump_allowed",
                     window.gameplay.gameplayTraversal.fallbackJumpAllowed);
  appendReceiptField(receipt, "gameplay_traversal_status",
                     window.gameplay.gameplayTraversal.status);
  appendReceiptField(receipt, "gameplay_traversal_reason_code",
                     window.gameplay.gameplayTraversal.reasonCode);
  appendReceiptField(receipt, "gameplay_traversal_mechanic",
                     window.gameplay.gameplayTraversal.mechanic);
  appendReceiptField(receipt, "gameplay_traversal_slot_id",
                     window.gameplay.gameplayTraversal.slotId);
  appendReceiptField(receipt, "gameplay_traversal_target_id",
                     window.gameplay.gameplayTraversal.targetId);
  appendReceiptField(receipt, "gameplay_traversal_landing_surface_id",
                     window.gameplay.gameplayTraversal.landingSurfaceId);
  appendReceiptField(receipt, "gameplay_traversal_start_x",
                     floatReceiptValue(window.gameplay.gameplayTraversal.startX));
  appendReceiptField(receipt, "gameplay_traversal_start_y",
                     floatReceiptValue(window.gameplay.gameplayTraversal.startY));
  appendReceiptField(receipt, "gameplay_traversal_start_z",
                     floatReceiptValue(window.gameplay.gameplayTraversal.startZ));
  appendReceiptField(receipt, "gameplay_traversal_final_x",
                     floatReceiptValue(window.gameplay.gameplayTraversal.finalX));
  appendReceiptField(receipt, "gameplay_traversal_final_y",
                     floatReceiptValue(window.gameplay.gameplayTraversal.finalY));
  appendReceiptField(receipt, "gameplay_traversal_final_z",
                     floatReceiptValue(window.gameplay.gameplayTraversal.finalZ));
  appendReceiptField(receipt, "gameplay_dash_requested",
                     window.gameplay.gameplayDash.requested);
  appendReceiptField(receipt, "gameplay_dash_accepted",
                     window.gameplay.gameplayDash.accepted);
  appendReceiptField(receipt, "gameplay_dash_status", window.gameplay.gameplayDash.status);
  appendReceiptField(receipt, "gameplay_dash_reason_code",
                     window.gameplay.gameplayDash.reasonCode);
  appendReceiptField(receipt, "gameplay_dash_speed_mps",
                     floatReceiptValue(window.gameplay.gameplayDash.speedMetersPerSecond));
  appendReceiptField(receipt, "gameplay_dash_distance_meters",
                     floatReceiptValue(window.gameplay.gameplayDash.distanceMeters));
  appendReceiptField(receipt, "gameplay_dash_cooldown_remaining_seconds",
                     floatReceiptValue(window.gameplay.gameplayDash.cooldownRemainingSeconds));
  appendReceiptField(receipt, "gameplay_dash_direction_x",
                     floatReceiptValue(window.gameplay.gameplayDash.directionX));
  appendReceiptField(receipt, "gameplay_dash_direction_z",
                     floatReceiptValue(window.gameplay.gameplayDash.directionZ));
}

}  // namespace iggy3d
