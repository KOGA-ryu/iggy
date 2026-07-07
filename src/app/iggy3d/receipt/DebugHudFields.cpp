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

void appendProductDebugHudFields(RenderReceipt& receipt, const ProductAppWindowState& window, const MovementDebugHud& movementHud, const NpcBehaviorDebugHud& npcBehaviorHud, const PhysicsDebugHud& physicsHud) {
  appendReceiptField(receipt, "movement_debug_hud_visible", movementHud.visible);
  appendReceiptField(receipt, "movement_debug_hud_line_count",
                     static_cast<std::uint64_t>(movementHud.lines.size()));
  appendReceiptField(receipt, "movement_debug_hud_dev_tools_enabled",
                     movementHud.developerToolsEnabled);
  appendReceiptField(receipt, "movement_debug_hud_debug_overlay_enabled",
                     movementHud.debugOverlayEnabled);
  appendReceiptField(receipt, "movement_debug_hud_debug_available",
                     movementHud.debugAvailable);
  appendReceiptField(receipt, "movement_debug_hud_status", movementHud.status);
  appendReceiptField(receipt, "movement_debug_hud_blocked", movementHud.blocked);
  appendReceiptField(receipt, "movement_debug_hud_reason_code",
                     movementHud.reasonCode);
  appendReceiptField(receipt, "movement_debug_hud_hit_surface_id",
                     movementHud.hitSurfaceId);
  appendReceiptField(receipt, "movement_debug_hud_policy_band",
                     movementHud.policyBand);
  appendReceiptField(receipt, "movement_debug_hud_speed_multiplier",
                     floatReceiptValue(movementHud.speedMultiplier));
  appendReceiptField(receipt,
                     "npc_behavior_debug_hud_visible",
                     npcBehaviorHud.visible);
  appendReceiptField(receipt,
                     "npc_behavior_debug_hud_line_count",
                     static_cast<std::uint64_t>(npcBehaviorHud.lineCount));
  appendReceiptField(receipt,
                     "npc_behavior_debug_hud_dev_tools_enabled",
                     npcBehaviorHud.developerToolsEnabled);
  appendReceiptField(receipt,
                     "npc_behavior_debug_hud_debug_overlay_enabled",
                     npcBehaviorHud.debugOverlayEnabled);
  appendReceiptField(receipt,
                     "npc_behavior_debug_hud_debug_available",
                     npcBehaviorHud.debugAvailable);
  appendReceiptField(receipt,
                     "npc_behavior_debug_hud_status",
                     npcBehaviorHud.status);
  appendReceiptField(receipt,
                     "npc_behavior_debug_hud_reason_code",
                     npcBehaviorHud.reasonCode);
  appendReceiptField(receipt,
                     "npc_behavior_debug_hud_has_unresolved_profile",
                     window.debugHud.npcBehaviorDebugHud.hasUnresolvedProfile);
  appendReceiptField(receipt, "physics_debug_hud_visible",
                     physicsHud.visible);
  appendReceiptField(receipt, "physics_debug_hud_line_count",
                     static_cast<std::uint64_t>(physicsHud.lineCount));
  appendReceiptField(receipt, "physics_debug_hud_dev_tools_enabled",
                     physicsHud.developerToolsEnabled);
  appendReceiptField(receipt, "physics_debug_hud_debug_overlay_enabled",
                     physicsHud.debugOverlayEnabled);
  appendReceiptField(receipt, "physics_debug_hud_debug_available",
                     physicsHud.debugAvailable);
  appendReceiptField(receipt, "physics_debug_hud_status",
                     physicsHud.status);
  appendReceiptField(receipt, "physics_debug_hud_reason_code",
                     physicsHud.reasonCode);
  appendReceiptField(receipt, "physics_debug_hud_has_warnings",
                     physicsHud.hasWarnings);
}

}  // namespace iggy3d
