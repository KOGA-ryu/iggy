#pragma once

#include <cstddef>
#include <optional>

#include "app/iggy3d/ProductGameplayFeedback.hpp"
#include "app/iggy3d/ProductMovementDebugHud.hpp"
#include "app/iggy3d/ProductNpcBehaviorDebugHud.hpp"
#include "app/iggy3d/ProductPrimitiveDrawList.hpp"
#include "app/iggy3d/ProductRenderBridge.hpp"
#include "app/iggy3d/ProductRoomEditorHud.hpp"
#include "app/iggy3d/ProductRoomEditorOverlay.hpp"
#include "app/iggy3d/ProductViewportFraming.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

struct ProductGameplayProjectionRefreshRequest {
  const std::optional<Session>& activeSession;
  ProductAppWindowState& window;
  bool developerToolsEnabled = false;
  bool debugOverlayEnabled = false;
};

struct ProductGameplayProjectionFrameRequest {
  const std::optional<Session>& activeSession;
  ProductAppWindowState& window;
  bool developerToolsEnabled = false;
  bool debugOverlayEnabled = false;
};

struct ProductGameplayProjectionFrame {
  SceneProjectionResult scene;
  DebugProjectionResult debug;
  ProductPrimitiveDrawList drawList;
  ProductViewportFrame viewportFrame;
  ProductGameplayFeedback feedback;
  ProductMovementDebugHud movementHud;
  ProductNpcBehaviorDebugHud npcBehaviorHud;
  ProductRoomEditorHud roomEditorHud;
  ProductRoomEditorOverlay roomEditorOverlay;
  ProductRenderBridgeFrame renderBridge;
  bool hasGameplayProjection = false;
  bool viewVisible = false;
  std::size_t sceneItemCount = 0;

  const SceneProjectionResult* scenePtr() const;
  const DebugProjectionResult* debugPtr() const;
  const ProductPrimitiveDrawList* drawListPtr() const;
  const ProductViewportFrame* viewportFramePtr() const;
  const ProductRenderBridgeFrame* renderBridgePtr() const;
};

DebugProjectionResult buildProductDebugProjectionWithNpcBehavior(
    const SessionState& state);

void copyNpcBehaviorDebugHud(ProductAppWindowState& window,
                             const ProductNpcBehaviorDebugHud& hud);

void copyProductRoomEditorOverlay(ProductAppWindowState& window,
                                  const ProductRoomEditorOverlay& overlay);

void copyProductRoomEditorHud(ProductAppWindowState& window,
                              const ProductRoomEditorHud& hud);

void applyGameplayProjectionMetrics(ProductAppWindowState& window,
                                    const SceneProjectionResult* scene,
                                    const DebugProjectionResult* debug,
                                    const ProductPrimitiveDrawList* drawList,
                                    const ProductViewportFrame* frame,
                                    const ProductRenderBridgeFrame* bridge,
                                    bool viewVisible);

ProductGameplayProjectionFrame buildProductGameplayProjectionFrame(
    const ProductGameplayProjectionFrameRequest& request);

FrameInput makeProductVulkanFrame(const SceneProjectionResult& scene,
                                  const DebugProjectionResult& debug,
                                  std::uint64_t frameIndex,
                                  std::uint32_t viewportWidth,
                                  std::uint32_t viewportHeight,
                                  float cameraYawDegrees,
                                  float cameraPitchDegrees);

void refreshProductGameplayProjectionMetrics(
    const ProductGameplayProjectionRefreshRequest& request);

}  // namespace iggy3d
