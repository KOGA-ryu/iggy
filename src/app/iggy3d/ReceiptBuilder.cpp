#include "app/iggy3d/ReceiptBuilder.hpp"

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
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/window/RendererLifecycle.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"
#include "app/iggy3d/receipt/ReceiptFields.hpp"

namespace iggy3d {

const creative::CreativeActiveIdentity& defaultProductReceiptCreativeIdentity() {
  static const creative::CreativeActiveIdentity identity;
  return identity;
}

RenderReceipt buildProductAppReceipt(const ProductAppOptions& options,
                                     const ProductWorldTemplate& world,
                                     const FrontendState& frontend,
                                     const FrontendSettings& settings,
                                     const ProductAppWindowState& window,
                                     const ProductSaveBridgeResult& saves,
                                     const creative::CreativeActiveIdentity&
                                         creativeIdentity) {
  RenderReceipt receipt;
  const ProductActiveSurfaceFrame activeSurface = resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(frontend, window));
  const bool sourceCreativeDocument =
      window.inputDevice.interactionMode == ProductInteractionMode::Creative &&
      productCreativeWorldActiveForIdentity(creativeIdentity);
  const ProductCreativeSurfaceKind creativeSurface =
      sourceCreativeDocument
          ? ProductCreativeSurfaceKind::CreativeDocument
          : productCreativeSurfaceKindForWindow(frontend, window);
  const bool mapMakerLive =
      creativeSurface == ProductCreativeSurfaceKind::LegacyMapMaker;
  const GameplayFeedback feedback = buildGameplayFeedback(window);
  const ProductMovementProofPacket movementProof =
      buildProductMovementProofPacket(window);
  const ProductVulkanGameplayReadiness vulkanGameplayReadiness =
      evaluateProductVulkanGameplayReadiness(window);
  const MovementDebugHud movementHud =
      buildMovementDebugHud(movementProof,
                            window.gameplay.gameplayActive,
                            settings.devToolsEnabled,
                            settings.debugOverlayEnabled);
  const NpcBehaviorDebugHud npcBehaviorHud{
      window.debugHud.npcBehaviorDebugHud.visible,
      settings.devToolsEnabled,
      settings.debugOverlayEnabled,
      window.debugHud.npcBehaviorDebugHud.debugAvailable,
      static_cast<std::size_t>(
          window.debugHud.npcBehaviorDebugHud.lineCount),
      window.debugHud.npcBehaviorDebugHud.status,
      window.debugHud.npcBehaviorDebugHud.reasonCode,
      {}};
  const PhysicsDebugHud physicsHud{
      window.debugHud.physicsDebugHud.visible,
      settings.devToolsEnabled,
      settings.debugOverlayEnabled,
      window.debugHud.physicsDebugHud.debugAvailable,
      window.debugHud.physicsDebugHud.lineCount,
      window.debugHud.physicsDebugHud.status,
      window.debugHud.physicsDebugHud.reasonCode,
      window.debugHud.physicsDebugHud.hasWarnings,
      {}};
  appendProductFrontendSettingsWindowFields(receipt, options, frontend, settings, window, creativeSurface, mapMakerLive);
  appendProductStartupWorldBuildoutFields(receipt, frontend, window, saves);
  appendProductSaveStateFields(receipt, frontend, window, creativeIdentity);
  appendProductGameplayRuntimeMovementFields(receipt, window, movementProof);
  appendProductDebugHudFields(receipt, window, movementHud, npcBehaviorHud, physicsHud);
  appendProductGameplaySceneStateFields(receipt, window);
  appendProductFeedbackSurfaceAutomationVulkanFields(receipt, window, feedback, activeSurface, vulkanGameplayReadiness);
  appendProductCreativeUiFields(receipt, window);
  appendProductCreativePickWireframeFields(receipt, window, vulkanGameplayReadiness);
  appendProductTailFields(receipt, options, world, window, saves);
  const bool windowFailed = window.requested && !window.created;
  appendReceiptField(receipt, "result", windowFailed ? "skip" : "pass");
  appendReceiptField(receipt, "reason_code",
                     windowFailed ? "sdl3_unavailable"
                                  : (window.gameplay.gameplayActive ? "product_gameplay_ready"
                                                           : "opening_menu_ready"));
  return receipt;
}

}  // namespace iggy3d
