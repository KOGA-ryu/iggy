#include "app/iggy3d/receipt/ReceiptFields.hpp"

#include <array>
#include <charconv>
#include <string>
#include <string_view>
#include <utility>

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/GameplayFeedback.hpp"
#include "app/iggy3d/gameplay/MovementProof.hpp"
#include "app/iggy3d/debug/MovementDebugHud.hpp"
#include "app/iggy3d/debug/DebugHudState.hpp"
#include "app/iggy3d/creative/ui/UiProjection.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/creative/bridge/UiCommandFrame.hpp"
#include "app/iggy3d/creative/bridge/UiInputFrame.hpp"
#include "app/iggy3d/creative/bridge/ViewportPickFrame.hpp"
#include "app/iggy3d/creative/bridge/WireframeFrame.hpp"
#include "app/iggy3d/window/RendererLifecycle.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"

namespace iggy3d {

namespace {

struct GameplayRuntimeMovementReceiptContext {
  const ProductAppWindowState& window;
  const ProductMovementProofPacket& movementProof;
  std::uint64_t runtimeStateHash;
};

struct GameplayRuntimeMovementReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const GameplayRuntimeMovementReceiptContext& context,
                 std::string_view key);
};

const std::array<GameplayRuntimeMovementReceiptFieldRow, 116>
    kGameplayRuntimeMovementReceiptFields{{
        {"runtime_session_created",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.runtimeSessionCreated);
         }},
        {"gameplay_active",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayActive);
         }},
        {"runtime_state_hash",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.runtimeStateHash);
         }},
        {"gameplay_view_visible",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.viewport.gameplayViewVisible);
         }},
        {"scene_item_count",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.sceneItemCount);
         }},
        {"debug_item_count",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.debugItemCount);
         }},
        {"player_visible",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.playerVisible);
         }},
        {"room_visible",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.roomVisible);
         }},
        {"objective_visible",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.objectiveVisible);
         }},
        {"renderer_mutated_runtime",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.rendererMutatedRuntime);
         }},
        {"scripted_gameplay_smoke",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.scriptedGameplaySmoke);
         }},
        {"gameplay_input_used",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayInputUsed);
         }},
        {"gameplay_input_source",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayInputSource);
         }},
        {"gameplay_command_submitted",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayCommand.submitted);
         }},
        {"gameplay_command_kind",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayCommand.kind);
         }},
        {"gameplay_command_status",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayCommand.status);
         }},
        {"gameplay_command_accepted",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayCommand.accepted);
         }},
        {"gameplay_tick_advanced",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayTickAdvanced);
         }},
        {"gameplay_tick_reason_code",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayTickReasonCode);
         }},
        {"player_position_changed",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.playerPositionChanged);
         }},
        {"gameplay_movement_attempted",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.attempted);
         }},
        {"gameplay_movement_blocked",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.blocked);
         }},
        {"gameplay_movement_status",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.status);
         }},
        {"gameplay_movement_debug_available",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.debugAvailable);
         }},
        {"gameplay_movement_reason_code",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.reasonCode);
         }},
        {"gameplay_movement_blocked_reason",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.blockedReason);
         }},
        {"gameplay_movement_hit_surface_id",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.hitSurfaceId);
         }},
        {"gameplay_movement_ground_snap_applied",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.groundSnapApplied);
         }},
        {"gameplay_movement_clamped",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.movementClamped);
         }},
        {"gameplay_movement_slid",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.movementSlid);
         }},
        {"gameplay_movement_collision_sweep_count",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.collisionSweepCount);
         }},
        {"gameplay_movement_policy_band",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.policyBand);
         }},
        {"gameplay_movement_slope_travel_direction",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.slopeTravelDirection);
         }},
        {"gameplay_movement_slope_angle_degrees",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.slopeAngleDegrees));
         }},
        {"gameplay_movement_speed_multiplier",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.speedMultiplier));
         }},
        {"gameplay_movement_start_x",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.startX));
         }},
        {"gameplay_movement_start_y",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.startY));
         }},
        {"gameplay_movement_start_z",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.startZ));
         }},
        {"gameplay_movement_final_x",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.finalX));
         }},
        {"gameplay_movement_final_y",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.finalY));
         }},
        {"gameplay_movement_final_z",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.finalZ));
         }},
        {"gameplay_movement_horizontal_distance_meters",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.horizontalDistanceMeters));
         }},
        {"gameplay_movement_vertical_delta_meters",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.verticalDeltaMeters));
         }},
        {"gameplay_movement_ground_velocity_x",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.groundVelocityX));
         }},
        {"gameplay_movement_ground_velocity_z",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.groundVelocityZ));
         }},
        {"gameplay_movement_state",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.stateName);
         }},
        {"movement_state",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.stateName);
         }},
        {"movement_grounded",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.grounded);
         }},
        {"movement_vertical_velocity_mps",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.verticalVelocityMetersPerSecond));
         }},
        {"movement_horizontal_speed_mps",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.horizontalSpeedMetersPerSecond));
         }},
        {"movement_hit_surface_id",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.hitSurfaceId);
         }},
        {"wall_run_candidate_available",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.wallRunCandidateAvailable);
         }},
        {"wall_run_candidate_status",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.wallRunCandidateStatus);
         }},
        {"wall_run_candidate_reason_code",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.wallRunCandidateReasonCode);
         }},
        {"wall_run_side",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.wallRunSide);
         }},
        {"wall_run_surface_id",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.wallRunSurfaceId);
         }},
        {"wall_run_normal_x",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.wallRunNormalX));
         }},
        {"wall_run_normal_y",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.wallRunNormalY));
         }},
        {"wall_run_normal_z",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.wallRunNormalZ));
         }},
        {"wall_run_approach_speed_mps",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.wallRunApproachSpeedMetersPerSecond));
         }},
        {"wall_run_active",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.wallRunActive);
         }},
        {"wall_run_status",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.wallRunStatus);
         }},
        {"wall_run_reason_code",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.wallRunReasonCode);
         }},
        {"wall_run_remaining_s",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.wallRunRemainingSeconds));
         }},
        {"wall_run_duration_s",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.wallRunDurationSeconds));
         }},
        {"wall_run_gravity_multiplier",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.wallRunGravityMultiplier));
         }},
        {"wall_run_speed_multiplier",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.wallRunSpeedMultiplier));
         }},
        {"gameplay_movement_grade_percent",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.gradePercent));
         }},
        {"gameplay_movement_profile",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.movementProof.profile);
         }},
        {"gameplay_movement_max_speed_mps",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.movementProof.maxSpeedMetersPerSecond));
         }},
        {"gameplay_jump_requested",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayJump.requested);
         }},
        {"gameplay_jump_accepted",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayJump.accepted);
         }},
        {"gameplay_jump_active",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayJump.active);
         }},
        {"gameplay_jump_status",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayJump.status);
         }},
        {"gameplay_jump_reason_code",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayJump.reasonCode);
         }},
        {"gameplay_jump_velocity_mps",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.window.gameplay.gameplayJump.velocityMetersPerSecond));
         }},
        {"gameplay_jump_coyote_seconds_remaining",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.window.gameplay.gameplayJump.coyoteSecondsRemaining));
         }},
        {"gameplay_jump_buffer_seconds_remaining",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.window.gameplay.gameplayJump.bufferSecondsRemaining));
         }},
        {"gameplay_jump_held",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayJump.held);
         }},
        {"gameplay_jump_cut_applied",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayJump.cutApplied);
         }},
        {"gameplay_jump_ground_y",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.window.gameplay.gameplayJump.groundY));
         }},
        {"gameplay_jump_start_y",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.window.gameplay.gameplayJump.startY));
         }},
        {"gameplay_jump_final_y",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.window.gameplay.gameplayJump.finalY));
         }},
        {"gameplay_jump_height_meters",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.window.gameplay.gameplayJump.heightMeters));
         }},
        {"gameplay_reset_triggered",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayReset.triggered);
         }},
        {"gameplay_reset_status",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayReset.status);
         }},
        {"gameplay_reset_reason_code",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayReset.reasonCode);
         }},
        {"gameplay_reset_spawn_anchor_id",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayReset.spawnAnchorId);
         }},
        {"gameplay_reset_source_anchor_id",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayReset.sourceAnchorId);
         }},
        {"gameplay_reset_start_y",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.window.gameplay.gameplayReset.startY));
         }},
        {"gameplay_reset_final_y",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.window.gameplay.gameplayReset.finalY));
         }},
        {"gameplay_traversal_requested",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayTraversal.requested);
         }},
        {"gameplay_traversal_consumed",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayTraversal.consumed);
         }},
        {"gameplay_traversal_accepted",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayTraversal.accepted);
         }},
        {"gameplay_traversal_fallback_jump_allowed",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayTraversal.fallbackJumpAllowed);
         }},
        {"gameplay_traversal_status",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayTraversal.status);
         }},
        {"gameplay_traversal_reason_code",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayTraversal.reasonCode);
         }},
        {"gameplay_traversal_mechanic",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayTraversal.mechanic);
         }},
        {"gameplay_traversal_slot_id",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayTraversal.slotId);
         }},
        {"gameplay_traversal_target_id",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayTraversal.targetId);
         }},
        {"gameplay_traversal_landing_surface_id",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayTraversal.landingSurfaceId);
         }},
        {"gameplay_traversal_start_x",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.window.gameplay.gameplayTraversal.startX));
         }},
        {"gameplay_traversal_start_y",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.window.gameplay.gameplayTraversal.startY));
         }},
        {"gameplay_traversal_start_z",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.window.gameplay.gameplayTraversal.startZ));
         }},
        {"gameplay_traversal_final_x",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.window.gameplay.gameplayTraversal.finalX));
         }},
        {"gameplay_traversal_final_y",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.window.gameplay.gameplayTraversal.finalY));
         }},
        {"gameplay_traversal_final_z",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.window.gameplay.gameplayTraversal.finalZ));
         }},
        {"gameplay_dash_requested",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayDash.requested);
         }},
        {"gameplay_dash_accepted",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayDash.accepted);
         }},
        {"gameplay_dash_status",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayDash.status);
         }},
        {"gameplay_dash_reason_code",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.gameplay.gameplayDash.reasonCode);
         }},
        {"gameplay_dash_speed_mps",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.window.gameplay.gameplayDash.speedMetersPerSecond));
         }},
        {"gameplay_dash_distance_meters",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.window.gameplay.gameplayDash.distanceMeters));
         }},
        {"gameplay_dash_cooldown_remaining_seconds",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.window.gameplay.gameplayDash.cooldownRemainingSeconds));
         }},
        {"gameplay_dash_direction_x",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.window.gameplay.gameplayDash.directionX));
         }},
        {"gameplay_dash_direction_z",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(context.window.gameplay.gameplayDash.directionZ));
         }},
    }};

}  // namespace

void appendProductGameplayRuntimeMovementFields(RenderReceipt& receipt, const ProductAppWindowState& window, const ProductMovementProofPacket& movementProof, std::uint64_t runtimeStateHash) {
  const GameplayRuntimeMovementReceiptContext context{
      window, movementProof, runtimeStateHash};
  for (const GameplayRuntimeMovementReceiptFieldRow& row :
       kGameplayRuntimeMovementReceiptFields) {
    row.append(receipt, context, row.key);
  }
}

}  // namespace iggy3d
