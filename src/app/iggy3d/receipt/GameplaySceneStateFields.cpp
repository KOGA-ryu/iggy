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
#include "app/iggy3d/view/CreativeFlyAnchorStore.hpp"
#include "app/iggy3d/window/RendererLifecycle.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"

namespace iggy3d {

namespace {

struct GameplaySceneStateReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const ProductAppWindowState& window,
                 std::string_view key);
};

const std::array<GameplaySceneStateReceiptFieldRow, 178>
    kGameplaySceneStateReceiptFields{{
        {"position_hud_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.debugHud.positionHud.visible);
         }},
        {"position_hud_line_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, static_cast<std::uint64_t>(window.debugHud.positionHud.lineCount));
         }},
        {"position_hud_debug_available",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.debugHud.positionHud.debugAvailable);
         }},
        {"position_hud_status",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.debugHud.positionHud.status);
         }},
        {"position_hud_reason_code",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.debugHud.positionHud.reasonCode);
         }},
        {"position_hud_player_position_available",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.debugHud.positionHud.playerPositionAvailable);
         }},
        {"position_hud_world_x",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(window.debugHud.positionHud.worldX));
         }},
        {"position_hud_world_y",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(window.debugHud.positionHud.worldY));
         }},
        {"position_hud_world_z",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(window.debugHud.positionHud.worldZ));
         }},
        {"position_hud_grid_x",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, std::to_string(window.debugHud.positionHud.gridX));
         }},
        {"position_hud_grid_y",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, std::to_string(window.debugHud.positionHud.gridY));
         }},
        {"position_hud_grid_z",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, std::to_string(window.debugHud.positionHud.gridZ));
         }},
        {"position_hud_layer_index",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, std::to_string(window.debugHud.positionHud.layerIndex));
         }},
        {"position_hud_facing",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.debugHud.positionHud.facing);
         }},
        {"position_hud_yaw_degrees",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(window.debugHud.positionHud.yawDegrees));
         }},
        {"position_hud_pitch_degrees",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(window.debugHud.positionHud.pitchDegrees));
         }},
        {"gameplay_collision_surfaces_used",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayCollision.surfacesUsed);
         }},
        {"gameplay_collision_surface_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayCollision.surfaceCount);
         }},
        {"physics_movement_planner_enabled",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.physicsMovementPlanner.enabled);
         }},
        {"physics_movement_planner_requested",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.physicsMovementPlanner.requested);
         }},
        {"physics_movement_planner_used",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.physicsMovementPlanner.used);
         }},
        {"physics_movement_planner_status",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.physicsMovementPlanner.status);
         }},
        {"physics_movement_planner_reason_code",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.physicsMovementPlanner.reasonCode);
         }},
        {"target_discovered",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.targetDiscovered);
         }},
        {"gameplay_target_status",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTarget.status);
         }},
        {"gameplay_target_action",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTarget.action);
         }},
        {"gameplay_target_entity_id",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTarget.entityId);
         }},
        {"gameplay_target_stable_name",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTarget.stableName);
         }},
        {"gameplay_target_kind",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTarget.kind);
         }},
        {"gameplay_target_distance_meters",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(window.gameplay.gameplayTarget.distanceMeters));
         }},
        {"gameplay_target_supports_command",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTarget.supportsCommand);
         }},
        {"gameplay_outcome_status",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayOutcome.status);
         }},
        {"gameplay_outcome_target_active_after",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayOutcome.targetActiveAfter);
         }},
        {"gameplay_outcome_inventory_changed",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayOutcome.inventoryChanged);
         }},
        {"gameplay_outcome_item_id",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayOutcome.itemId);
         }},
        {"gameplay_outcome_item_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayOutcome.itemCount);
         }},
        {"gameplay_outcome_objective_changed",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayOutcome.objectiveChanged);
         }},
        {"gameplay_outcome_event_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayOutcome.eventCount);
         }},
        {"session_outcome",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.sessionOutcome);
         }},
        {"gameplay_tape_requested",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.requested);
         }},
        {"gameplay_tape_loaded",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.loaded);
         }},
        {"gameplay_tape_path",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.path);
         }},
        {"gameplay_tape_status",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.status);
         }},
        {"gameplay_tape_reason_code",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.reasonCode);
         }},
        {"gameplay_tape_line_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.lineCount);
         }},
        {"gameplay_tape_step_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.stepCount);
         }},
        {"gameplay_tape_executed_step_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.executedStepCount);
         }},
        {"gameplay_tape_expected_rejected_step_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.expectedRejectedStepCount);
         }},
        {"gameplay_tape_expected_blocked_step_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.expectedBlockedStepCount);
         }},
        {"gameplay_tape_failed_step",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.failedStep);
         }},
        {"gameplay_tape_failed_source_line",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.failedSourceLine);
         }},
        {"gameplay_tape_failed_action",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.failedAction);
         }},
        {"gameplay_tape_failed_target",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.failedTarget);
         }},
        {"gameplay_tape_failed_rejection",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.failedRejection);
         }},
        {"gameplay_tape_failed_movement_block",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.failedMovementBlock);
         }},
        {"gameplay_tape_last_action",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.lastAction);
         }},
        {"gameplay_tape_last_target",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.lastTarget);
         }},
        {"gameplay_tape_last_movement_block",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.lastMovementBlock);
         }},
        {"gameplay_tape_key_collected",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.keyCollected);
         }},
        {"gameplay_tape_secret_door_opened",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.secretDoorOpened);
         }},
        {"gameplay_tape_treasure_collected",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.treasureCollected);
         }},
        {"gameplay_tape_npc_targetable",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.npcTargetable);
         }},
        {"gameplay_tape_npc_defeated",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.npcDefeated);
         }},
        {"gameplay_tape_exit_objective_complete",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.exitObjectiveComplete);
         }},
        {"gameplay_tape_loop_complete",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.loopComplete);
         }},
        {"gameplay_tape_ai_command_logged",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.aiCommandLogged);
         }},
        {"gameplay_tape_ai_attack_logged",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.aiAttackLogged);
         }},
        {"gameplay_tape_ai_wait_logged",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.aiWaitLogged);
         }},
        {"gameplay_tape_ai_player_damaged",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.aiPlayerDamaged);
         }},
        {"gameplay_tape_ai_player_hp_before",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, static_cast<std::uint64_t>(window.gameplay.gameplayTape.aiPlayerHpBefore));
         }},
        {"gameplay_tape_ai_player_hp_after",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, static_cast<std::uint64_t>(window.gameplay.gameplayTape.aiPlayerHpAfter));
         }},
        {"gameplay_tape_ai_actor_id",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.aiActorId);
         }},
        {"gameplay_tape_ai_target_id",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.aiTargetId);
         }},
        {"gameplay_tape_ai_behavior",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.aiBehavior);
         }},
        {"gameplay_tape_ai_intent",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayTape.aiIntent);
         }},
        {"gameplay_reach_gate",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayReachGate);
         }},
        {"gameplay_last_rejection",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.gameplayLastRejection);
         }},
        {"interaction_executed",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.interactionExecuted);
         }},
        {"attack_executed",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.attackExecuted);
         }},
        {"product_transition_last_action",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.productTransition.lastAction);
         }},
        {"product_transition_status",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.productTransition.status);
         }},
        {"product_transition_returned_to_gameplay",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.productTransition.returnedToGameplay);
         }},
        {"product_transition_returned_to_title",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.productTransition.returnedToTitle);
         }},
        {"product_transition_session_preserved",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.gameplay.productTransition.sessionPreserved);
         }},
        {"camera_controller",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.cameraController);
         }},
        {"camera_mode",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.cameraMode);
         }},
        {"camera_controller_active",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.cameraControllerActive);
         }},
        {"look_input_used",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.lookInputUsed);
         }},
        {"camera_input_source",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.cameraInputSource);
         }},
        {"camera_yaw_degrees",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(window.viewport.cameraYawDegrees));
         }},
        {"camera_pitch_degrees",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(window.viewport.cameraPitchDegrees));
         }},
        {"camera_heading_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.cameraHeadingVisible);
         }},
        {"creative_navigate_active",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.creativeNavigateActive);
         }},
        {"creative_fly_active",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.creativeFlyActive);
         }},
        {"creative_fly_status",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.creativeFlyStatus);
         }},
        {"creative_fly_reason_code",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.creativeFlyReasonCode);
         }},
        {"creative_fly_speed_mps",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(window.viewport.creativeFlySpeedMetersPerSecond));
         }},
        {"creative_fly_anchor_provenance",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, std::string(productCreativeFlyAnchorProvenanceName(window.viewport.creativeFlyAnchor.provenance)));
         }},
        {"creative_fly_anchor_world_epoch",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.creativeFlyAnchor.seededFromWorldEpoch);
         }},
        {"creative_fly_world_x",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(window.viewport.creativeFlyAnchor.positionMeters.x));
         }},
        {"creative_fly_world_y",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(window.viewport.creativeFlyAnchor.positionMeters.y));
         }},
        {"creative_fly_world_z",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, floatReceiptValue(window.viewport.creativeFlyAnchor.positionMeters.z));
         }},
        {"product_draw_item_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawItemCount);
         }},
        {"product_draw_grid_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawGridVisible);
         }},
        {"product_draw_player_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawPlayerVisible);
         }},
        {"product_draw_room_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawRoomVisible);
         }},
        {"product_draw_objective_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawObjectiveVisible);
         }},
        {"product_draw_target_indicator_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawTargetIndicatorVisible);
         }},
        {"product_draw_door_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawDoorVisible);
         }},
        {"product_draw_open_door_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawOpenDoorVisible);
         }},
        {"product_draw_closed_door_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawClosedDoorVisible);
         }},
        {"product_draw_debug_marker_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawDebugMarkerCount);
         }},
        {"product_draw_door_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawDoorCount);
         }},
        {"product_draw_open_door_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawOpenDoorCount);
         }},
        {"product_draw_closed_door_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawClosedDoorCount);
         }},
        {"product_draw_room_geometry_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawRoomGeometryCount);
         }},
        {"product_draw_floor_tile_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawFloorTileCount);
         }},
        {"product_draw_elevated_floor_tile_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawElevatedFloorTileCount);
         }},
        {"product_draw_ramp_tile_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawRampTileCount);
         }},
        {"product_draw_blocked_slope_tile_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawBlockedSlopeTileCount);
         }},
        {"product_draw_wall_tile_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawWallTileCount);
         }},
        {"product_draw_prop_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawPropVisible);
         }},
        {"product_draw_prop_tile_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawPropTileCount);
         }},
        {"product_draw_room_editor_cursor_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawRoomEditorCursorVisible);
         }},
        {"product_draw_room_editor_cursor_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawRoomEditorCursorCount);
         }},
        {"product_draw_room_editor_preview_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawRoomEditorPreviewVisible);
         }},
        {"product_draw_room_editor_preview_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawRoomEditorPreviewCount);
         }},
        {"product_draw_physics_debug_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawPhysicsDebugVisible);
         }},
        {"product_draw_physics_debug_item_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawPhysicsDebugItemCount);
         }},
        {"product_draw_physics_aabb_debug_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawPhysicsAabbDebugCount);
         }},
        {"product_draw_physics_contact_normal_debug_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawPhysicsContactNormalDebugCount);
         }},
        {"product_draw_map_maker_grid_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawMapMakerGridVisible);
         }},
        {"product_draw_map_maker_grid_dot_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawMapMakerGridDotCount);
         }},
        {"product_draw_map_maker_major_grid_dot_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawMapMakerMajorGridDotCount);
         }},
        {"product_draw_map_maker_cube_preview_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawMapMakerCubePreviewVisible);
         }},
        {"product_draw_map_maker_cube_preview_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productDrawMapMakerCubePreviewCount);
         }},
        {"product_view_projection",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productViewProjection);
         }},
        {"product_view_yaw_applied",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productViewYawApplied);
         }},
        {"product_view_pitch_applied",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productViewPitchApplied);
         }},
        {"product_view_player_anchor_found",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productViewPlayerAnchorFound);
         }},
        {"product_render_bridge_ready",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productRenderBridgeReady);
         }},
        {"product_view_frame_ready",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productViewFrameReady);
         }},
        {"product_view_frame_item_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productViewFrameItemCount);
         }},
        {"product_view_frame_on_screen_item_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productViewFrameOnScreenItemCount);
         }},
        {"product_view_frame_target_item_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productViewFrameTargetItemCount);
         }},
        {"product_render_bridge_room_editor_cursor_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productRenderBridgeRoomEditorCursorVisible);
         }},
        {"product_render_bridge_room_editor_cursor_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productRenderBridgeRoomEditorCursorCount);
         }},
        {"product_render_bridge_room_editor_preview_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productRenderBridgeRoomEditorPreviewVisible);
         }},
        {"product_render_bridge_room_editor_preview_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productRenderBridgeRoomEditorPreviewCount);
         }},
        {"product_render_bridge_prop_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productRenderBridgePropVisible);
         }},
        {"product_render_bridge_prop_tile_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productRenderBridgePropTileCount);
         }},
        {"product_render_bridge_physics_debug_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productRenderBridgePhysicsDebugVisible);
         }},
        {"product_render_bridge_physics_debug_item_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productRenderBridgePhysicsDebugItemCount);
         }},
        {"product_render_bridge_physics_aabb_debug_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productRenderBridgePhysicsAabbDebugCount);
         }},
        {"product_render_bridge_physics_contact_normal_debug_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productRenderBridgePhysicsContactNormalDebugCount);
         }},
        {"product_render_bridge_map_maker_grid_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productRenderBridgeMapMakerGridVisible);
         }},
        {"product_render_bridge_map_maker_grid_dot_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productRenderBridgeMapMakerGridDotCount);
         }},
        {"product_render_bridge_map_maker_major_grid_dot_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productRenderBridgeMapMakerMajorGridDotCount);
         }},
        {"product_render_bridge_map_maker_cube_preview_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productRenderBridgeMapMakerCubePreviewVisible);
         }},
        {"product_render_bridge_map_maker_cube_preview_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productRenderBridgeMapMakerCubePreviewCount);
         }},
        {"product_feedback_bridge_ready",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productFeedbackBridgeReady);
         }},
        {"product_feedback_bridge_line_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productFeedbackBridgeLineCount);
         }},
        {"product_vulkan_room_mesh_cpu_ready",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productVulkanRoomMeshCpuReady);
         }},
        {"product_vulkan_room_mesh_backend_presented",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productVulkanRoomMeshBackendPresented);
         }},
        {"product_vulkan_room_mesh_source",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productVulkanRoomMeshSource);
         }},
        {"product_vulkan_room_asset_id",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productVulkanRoomAssetId);
         }},
        {"product_vulkan_room_floor_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productVulkanRoomFloorVisible);
         }},
        {"product_vulkan_room_wall_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productVulkanRoomWallVisible);
         }},
        {"product_vulkan_room_grid_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productVulkanRoomGridVisible);
         }},
        {"product_vulkan_room_source_mesh_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productVulkanRoomSourceMeshCount);
         }},
        {"product_vulkan_room_vertex_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productVulkanRoomVertexCount);
         }},
        {"product_vulkan_room_index_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productVulkanRoomIndexCount);
         }},
        {"product_vulkan_room_draw_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productVulkanRoomDrawCount);
         }},
        {"product_vulkan_room_floor_draw_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productVulkanRoomFloorDrawCount);
         }},
        {"product_vulkan_room_wall_draw_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productVulkanRoomWallDrawCount);
         }},
        {"product_vulkan_room_grid_line_draw_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productVulkanRoomGridLineDrawCount);
         }},
        {"product_vulkan_room_grid_truncated",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productVulkanRoomGridTruncated);
         }},
        {"product_vulkan_room_geometry_signature",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.viewport.productVulkanRoomGeometrySignature);
         }},
    }};

}  // namespace

void appendProductGameplaySceneStateFields(RenderReceipt& receipt, const ProductAppWindowState& window) {
  for (const GameplaySceneStateReceiptFieldRow& row :
       kGameplaySceneStateReceiptFields) {
    row.append(receipt, window, row.key);
  }
}

}  // namespace iggy3d
