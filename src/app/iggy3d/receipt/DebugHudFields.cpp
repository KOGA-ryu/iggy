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

namespace {

struct DebugHudReceiptContext {
  const ProductAppWindowState& window;
  const MovementDebugHud& movementHud;
  const NpcBehaviorDebugHud& npcBehaviorHud;
  const PhysicsDebugHud& physicsHud;
};

struct DebugHudReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const DebugHudReceiptContext& context,
                 std::string_view key);
};

const std::array<DebugHudReceiptFieldRow, 27> kDebugHudReceiptFields{{
    {"movement_debug_hud_visible",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.movementHud.visible);
     }},
    {"movement_debug_hud_line_count",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           static_cast<std::uint64_t>(
               context.movementHud.lines.size()));
     }},
    {"movement_debug_hud_dev_tools_enabled",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.movementHud.developerToolsEnabled);
     }},
    {"movement_debug_hud_debug_overlay_enabled",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.movementHud.debugOverlayEnabled);
     }},
    {"movement_debug_hud_debug_available",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.movementHud.debugAvailable);
     }},
    {"movement_debug_hud_status",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.movementHud.status);
     }},
    {"movement_debug_hud_blocked",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.movementHud.blocked);
     }},
    {"movement_debug_hud_reason_code",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.movementHud.reasonCode);
     }},
    {"movement_debug_hud_hit_surface_id",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.movementHud.hitSurfaceId);
     }},
    {"movement_debug_hud_policy_band",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.movementHud.policyBand);
     }},
    {"movement_debug_hud_speed_multiplier",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, floatReceiptValue(context.movementHud.speedMultiplier));
     }},
    {"npc_behavior_debug_hud_visible",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.npcBehaviorHud.visible);
     }},
    {"npc_behavior_debug_hud_line_count",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           static_cast<std::uint64_t>(
               context.npcBehaviorHud.lineCount));
     }},
    {"npc_behavior_debug_hud_dev_tools_enabled",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.npcBehaviorHud.developerToolsEnabled);
     }},
    {"npc_behavior_debug_hud_debug_overlay_enabled",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.npcBehaviorHud.debugOverlayEnabled);
     }},
    {"npc_behavior_debug_hud_debug_available",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.npcBehaviorHud.debugAvailable);
     }},
    {"npc_behavior_debug_hud_status",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.npcBehaviorHud.status);
     }},
    {"npc_behavior_debug_hud_reason_code",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.npcBehaviorHud.reasonCode);
     }},
    {"npc_behavior_debug_hud_has_unresolved_profile",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.window.debugHud.npcBehaviorDebugHud.hasUnresolvedProfile);
     }},
    {"physics_debug_hud_visible",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.physicsHud.visible);
     }},
    {"physics_debug_hud_line_count",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           static_cast<std::uint64_t>(
               context.physicsHud.lineCount));
     }},
    {"physics_debug_hud_dev_tools_enabled",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.physicsHud.developerToolsEnabled);
     }},
    {"physics_debug_hud_debug_overlay_enabled",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.physicsHud.debugOverlayEnabled);
     }},
    {"physics_debug_hud_debug_available",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.physicsHud.debugAvailable);
     }},
    {"physics_debug_hud_status",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.physicsHud.status);
     }},
    {"physics_debug_hud_reason_code",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.physicsHud.reasonCode);
     }},
    {"physics_debug_hud_has_warnings",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.physicsHud.hasWarnings);
     }},
}};

}  // namespace

void appendProductDebugHudFields(RenderReceipt& receipt, const ProductAppWindowState& window, const MovementDebugHud& movementHud, const NpcBehaviorDebugHud& npcBehaviorHud, const PhysicsDebugHud& physicsHud) {
  const DebugHudReceiptContext context{
      window,
      movementHud,
      npcBehaviorHud,
      physicsHud,
  };

  for (const DebugHudReceiptFieldRow& row : kDebugHudReceiptFields) {
    row.append(receipt, context, row.key);
  }
}

}  // namespace iggy3d
