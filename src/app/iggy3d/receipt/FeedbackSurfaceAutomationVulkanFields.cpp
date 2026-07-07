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

void appendProductFeedbackSurfaceAutomationVulkanFields(RenderReceipt& receipt, const ProductAppWindowState& window, const GameplayFeedback& feedback, const ProductActiveSurfaceFrame& activeSurface, const ProductVulkanGameplayReadiness& vulkanGameplayReadiness) {
  appendReceiptField(receipt, "product_feedback_visible", feedback.visible);
  appendReceiptField(receipt, "product_feedback_target_status",
                     feedback.targetStatus);
  appendReceiptField(receipt, "product_feedback_reach_status",
                     feedback.reachStatus);
  appendReceiptField(receipt, "product_feedback_command_kind",
                     feedback.commandKind);
  appendReceiptField(receipt, "product_feedback_command_status",
                     feedback.commandStatus);
  appendReceiptField(receipt, "product_feedback_rejection_reason",
                     feedback.rejectionReason);
  appendReceiptField(receipt, "product_feedback_attack_visible",
                     feedback.combatFeedbackVisible);
  appendReceiptField(receipt, "product_feedback_interaction_visible",
                     feedback.interactionFeedbackVisible);
  appendReceiptField(receipt, "active_surface",
                     productFrontendSurfaceName(activeSurface.activeSurface));
  appendReceiptField(receipt, "active_parent_surface",
                     productFrontendSurfaceName(activeSurface.parentSurface));
  appendReceiptField(receipt, "input_surface",
                     productInputSurfaceName(activeSurface.inputSurface));
  appendReceiptField(receipt, "input_owner",
                     menuOwnerName(activeSurface.inputOwner));
  appendReceiptField(receipt, "active_surface_status", activeSurface.status);
  appendReceiptField(receipt,
                     "active_surface_mouse_capture_policy",
                     productActiveMouseCapturePolicyName(
                         activeSurface.mouseCapturePolicy));
  appendReceiptField(receipt, "input_action_last",
                     inputActionName(window.inputDevice.lastInputAction));
  appendReceiptField(receipt, "input_action_accepted",
                     window.inputDevice.lastInputAccepted);
  appendReceiptField(receipt, "gameplay_input_suppressed",
                     activeSurface.gameplayInputSuppressed);
  appendReceiptField(receipt, "automation_control_requested",
                     window.automationControl.requested);
  appendReceiptField(receipt, "automation_control_loaded",
                     window.automationControl.loaded);
  appendReceiptField(receipt, "automation_control_path", window.automationControl.path);
  appendReceiptField(receipt, "automation_control_status",
                     window.automationControl.status);
  appendReceiptField(receipt, "automation_control_scope", window.automationControl.scope);
  appendReceiptField(receipt, "automation_control_line_count",
                     window.automationControl.lineCount);
  appendReceiptField(receipt, "automation_control_applied_count",
                     window.automationControl.appliedCount);
  appendReceiptField(receipt, "automation_control_last_key",
                     window.automationControl.lastKey);
  appendReceiptField(receipt, "automation_control_last_action",
                     window.automationControl.lastAction);
  appendReceiptField(receipt, "automation_control_last_owner",
                     menuOwnerName(window.automationControl.lastOwner));
  appendReceiptField(receipt, "automation_control_last_result",
                     window.automationControl.lastResult);
  appendReceiptField(receipt, "product_vulkan_renderer_requested",
                     window.productVulkanRenderer.requested);
  appendReceiptField(receipt, "product_vulkan_backend_built",
                     vulkanGameplayReadiness.backendBuilt);
  appendReceiptField(receipt, "product_vulkan_renderer_created",
                     window.productVulkanRenderer.created);
  appendReceiptField(receipt, "product_vulkan_renderer_ready",
                     window.productVulkanRenderer.ready);
  appendReceiptField(receipt, "product_vulkan_surface_created",
                     window.productVulkanSurfaceCreated);
  appendReceiptField(receipt, "product_vulkan_swapchain_ready",
                     window.productVulkanSwapchainReady);
  appendReceiptField(receipt, "product_vulkan_frame_submitted",
                     window.productVulkanFrameSubmitted);
  appendReceiptField(receipt, "product_vulkan_frame_submitted_count",
                     window.productVulkanFrameSubmittedCount);
  appendReceiptField(receipt, "product_vulkan_status", window.productVulkanStatus);
  appendReceiptField(receipt, "product_vulkan_reason_code",
                     window.productVulkanReasonCode);
  appendReceiptField(receipt, "product_vulkan_rendering_path",
                     window.productVulkanRenderingPath);
  appendReceiptField(receipt, "product_vulkan_record_mode",
                     window.productVulkanRecordMode);
  appendReceiptField(receipt, "product_vulkan_menu_requested",
                     window.productVulkanMenu.requested);
  appendReceiptField(receipt, "product_vulkan_menu_visible",
                     window.productVulkanMenu.visible);
  appendReceiptField(receipt, "product_vulkan_menu_status",
                     window.productVulkanMenu.status);
  appendReceiptField(receipt, "product_vulkan_menu_reason_code",
                     window.productVulkanMenu.reasonCode);
  appendReceiptField(receipt, "product_vulkan_menu_surface",
                     window.productVulkanMenu.surface);
  appendReceiptField(receipt, "product_vulkan_menu_ui_ready",
                     window.productVulkanMenu.uiReady);
  appendReceiptField(receipt, "product_vulkan_menu_ui_partial",
                     window.productVulkanMenu.uiPartial);
  appendReceiptField(receipt, "product_vulkan_menu_ui_status",
                     window.productVulkanMenu.uiStatus);
  appendReceiptField(receipt, "product_vulkan_menu_ui_reason_code",
                     window.productVulkanMenu.uiReasonCode);
  appendReceiptField(receipt, "product_vulkan_menu_ui_primitive_count",
                     window.productVulkanMenu.uiPrimitiveCount);
  appendReceiptField(receipt, "product_vulkan_menu_ui_text_count",
                     window.productVulkanMenu.uiTextCount);
  appendReceiptField(receipt, "product_vulkan_menu_ui_rect_count",
                     window.productVulkanMenu.uiRectCount);
  appendReceiptField(receipt, "product_vulkan_menu_ui_row_count",
                     window.productVulkanMenu.uiRowCount);
  appendReceiptField(receipt, "product_vulkan_menu_ui_selected_action",
                     window.productVulkanMenu.uiSelectedAction);
}

}  // namespace iggy3d
