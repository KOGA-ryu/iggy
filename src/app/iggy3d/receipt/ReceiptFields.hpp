#pragma once

#include <cstdint>
#include <string>

#include "app/iggy3d/gameplay/GameplayFeedback.hpp"
#include "app/iggy3d/gameplay/MovementProof.hpp"
#include "app/iggy3d/debug/MovementDebugHud.hpp"
#include "app/iggy3d/debug/DebugHudState.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/window/RendererLifecycle.hpp"

namespace iggy3d {

struct ProductAppWindowState;
struct ProductWorldTemplate;

namespace creative {
struct CreativeActiveIdentity;
}  // namespace creative

// Shared receipt-field helper (promoted from ReceiptBuilder.cpp's anon namespace to
// external linkage so the domain appenders below can share one definition).
std::string floatReceiptValue(float value);

// Domain appenders extracted from buildProductAppReceipt (docs/appkernel_build_map, slice 3).
// Each appends a contiguous, order-preserved run of receipt fields for one subsystem.
void appendProductFrontendSettingsWindowFields(RenderReceipt& receipt, const ProductAppOptions& options, const FrontendState& frontend, const FrontendSettings& settings, const ProductAppWindowState& window, ProductCreativeSurfaceKind creativeSurface, bool mapMakerLive);
void appendProductStartupProbeFields(RenderReceipt& receipt, const FrontendState& frontend, const ProductAppWindowState& window, const ProductSaveBridgeResult& saves);
void appendProductWorldAuthoringFields(RenderReceipt& receipt, const ProductAppWindowState& window);
void appendProductActiveRoomFields(RenderReceipt& receipt, const ProductAppWindowState& window);
void appendProductStartupWorldBuildoutFields(RenderReceipt& receipt, const FrontendState& frontend, const ProductAppWindowState& window, const ProductSaveBridgeResult& saves);
void appendProductSaveStateFields(RenderReceipt& receipt, const FrontendState& frontend, const ProductAppWindowState& window, const creative::CreativeActiveIdentity& creativeIdentity);
void appendProductGameplayRuntimeMovementFields(RenderReceipt& receipt, const ProductAppWindowState& window, const ProductMovementProofPacket& movementProof, std::uint64_t runtimeStateHash);
void appendProductDebugHudFields(RenderReceipt& receipt, const ProductAppWindowState& window, const MovementDebugHud& movementHud, const NpcBehaviorDebugHud& npcBehaviorHud, const PhysicsDebugHud& physicsHud);
void appendProductGameplaySceneStateFields(RenderReceipt& receipt, const ProductAppWindowState& window);
void appendProductFeedbackSurfaceAutomationVulkanFields(RenderReceipt& receipt, const ProductAppWindowState& window, const GameplayFeedback& feedback, const ProductActiveSurfaceFrame& activeSurface, const ProductVulkanGameplayReadiness& vulkanGameplayReadiness);
void appendProductCreativeUiFields(RenderReceipt& receipt, const ProductAppWindowState& window);
void appendProductCreativePickWireframeFields(RenderReceipt& receipt, const ProductAppWindowState& window, const ProductVulkanGameplayReadiness& vulkanGameplayReadiness);
void appendProductTailFields(RenderReceipt& receipt, const ProductAppOptions& options, const ProductWorldTemplate& world, const ProductAppWindowState& window, const ProductSaveBridgeResult& saves);

}  // namespace iggy3d
