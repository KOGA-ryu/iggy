#pragma once

#include <optional>

#include "app/iggy3d/ProductNpcBehaviorDebugHud.hpp"
#include "app/iggy3d/ProductPrimitiveDrawList.hpp"
#include "app/iggy3d/ProductRenderBridge.hpp"
#include "app/iggy3d/ProductRoomEditorOverlay.hpp"
#include "app/iggy3d/ProductViewportFraming.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

struct ProductGameplayProjectionRefreshRequest {
  const std::optional<Session>& activeSession;
  ProductAppWindowState& window;
  bool developerToolsEnabled = false;
  bool debugOverlayEnabled = false;
};

DebugProjectionResult buildProductDebugProjectionWithNpcBehavior(
    const SessionState& state);

void copyNpcBehaviorDebugHud(ProductAppWindowState& window,
                             const ProductNpcBehaviorDebugHud& hud);

void copyProductRoomEditorOverlay(ProductAppWindowState& window,
                                  const ProductRoomEditorOverlay& overlay);

void applyGameplayProjectionMetrics(ProductAppWindowState& window,
                                    const SceneProjectionResult* scene,
                                    const DebugProjectionResult* debug,
                                    const ProductPrimitiveDrawList* drawList,
                                    const ProductViewportFrame* frame,
                                    const ProductRenderBridgeFrame* bridge,
                                    bool viewVisible);

void refreshProductGameplayProjectionMetrics(
    const ProductGameplayProjectionRefreshRequest& request);

}  // namespace iggy3d
