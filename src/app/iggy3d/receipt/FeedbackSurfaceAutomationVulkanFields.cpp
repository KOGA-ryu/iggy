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

struct FeedbackSurfaceAutomationVulkanReceiptContext {
  const ProductAppWindowState& window;
  const GameplayFeedback& feedback;
  const ProductActiveSurfaceFrame& activeSurface;
  const ProductVulkanGameplayReadiness& vulkanGameplayReadiness;
};

struct FeedbackSurfaceAutomationVulkanReceiptFieldRow {
  std::string_view key;
  void (*append)(
      RenderReceipt& receipt,
      const FeedbackSurfaceAutomationVulkanReceiptContext& context,
      std::string_view key);
};

const std::array<FeedbackSurfaceAutomationVulkanReceiptFieldRow, 54>
    kFeedbackSurfaceAutomationVulkanReceiptFields{{
        {"product_feedback_visible",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.feedback.visible);
         }},
        {"product_feedback_target_status",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.feedback.targetStatus);
         }},
        {"product_feedback_reach_status",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.feedback.reachStatus);
         }},
        {"product_feedback_command_kind",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.feedback.commandKind);
         }},
        {"product_feedback_command_status",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.feedback.commandStatus);
         }},
        {"product_feedback_rejection_reason",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.feedback.rejectionReason);
         }},
        {"product_feedback_attack_visible",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.feedback.combatFeedbackVisible);
         }},
        {"product_feedback_interaction_visible",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.feedback.interactionFeedbackVisible);
         }},
        {"active_surface",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, productFrontendSurfaceName(context.activeSurface.activeSurface));
         }},
        {"active_parent_surface",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, productFrontendSurfaceName(context.activeSurface.parentSurface));
         }},
        {"input_surface",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, productInputSurfaceName(context.activeSurface.inputSurface));
         }},
        {"input_owner",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, menuOwnerName(context.activeSurface.inputOwner));
         }},
        {"active_surface_status",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.activeSurface.status);
         }},
        {"active_surface_mouse_capture_policy",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, productActiveMouseCapturePolicyName(context.activeSurface.mouseCapturePolicy));
         }},
        {"input_action_last",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, inputActionName(context.window.inputDevice.lastInputAction));
         }},
        {"input_action_accepted",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.inputDevice.lastInputAccepted);
         }},
        {"gameplay_input_suppressed",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.activeSurface.gameplayInputSuppressed);
         }},
        {"automation_control_requested",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.automationControl.requested);
         }},
        {"automation_control_loaded",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.automationControl.loaded);
         }},
        {"automation_control_path",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.automationControl.path);
         }},
        {"automation_control_status",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.automationControl.status);
         }},
        {"automation_control_scope",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.automationControl.scope);
         }},
        {"automation_control_line_count",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.automationControl.lineCount);
         }},
        {"automation_control_applied_count",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.automationControl.appliedCount);
         }},
        {"automation_control_last_key",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.automationControl.lastKey);
         }},
        {"automation_control_last_action",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.automationControl.lastAction);
         }},
        {"automation_control_last_owner",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, menuOwnerName(context.window.automationControl.lastOwner));
         }},
        {"automation_control_last_result",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.automationControl.lastResult);
         }},
        {"product_vulkan_renderer_requested",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.presentPath.productVulkanRenderer.requested);
         }},
        {"product_vulkan_backend_built",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.vulkanGameplayReadiness.backendBuilt);
         }},
        {"product_vulkan_renderer_created",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.presentPath.productVulkanRenderer.created);
         }},
        {"product_vulkan_renderer_ready",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.presentPath.productVulkanRenderer.ready);
         }},
        {"product_vulkan_surface_created",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.presentPath.productVulkanSurfaceCreated);
         }},
        {"product_vulkan_swapchain_ready",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.presentPath.productVulkanSwapchainReady);
         }},
        {"product_vulkan_frame_submitted",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.presentPath.productVulkanFrameSubmitted);
         }},
        {"product_vulkan_frame_submitted_count",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.presentPath.productVulkanFrameSubmittedCount);
         }},
        {"product_vulkan_status",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.presentPath.productVulkanStatus);
         }},
        {"product_vulkan_reason_code",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.presentPath.productVulkanReasonCode);
         }},
        {"product_vulkan_rendering_path",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.presentPath.productVulkanRenderingPath);
         }},
        {"product_vulkan_record_mode",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.presentPath.productVulkanRecordMode);
         }},
        {"product_vulkan_menu_requested",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.productVulkanMenu.requested);
         }},
        {"product_vulkan_menu_visible",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.productVulkanMenu.visible);
         }},
        {"product_vulkan_menu_status",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.productVulkanMenu.status);
         }},
        {"product_vulkan_menu_reason_code",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.productVulkanMenu.reasonCode);
         }},
        {"product_vulkan_menu_surface",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.productVulkanMenu.surface);
         }},
        {"product_vulkan_menu_ui_ready",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.productVulkanMenu.uiReady);
         }},
        {"product_vulkan_menu_ui_partial",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.productVulkanMenu.uiPartial);
         }},
        {"product_vulkan_menu_ui_status",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.productVulkanMenu.uiStatus);
         }},
        {"product_vulkan_menu_ui_reason_code",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.productVulkanMenu.uiReasonCode);
         }},
        {"product_vulkan_menu_ui_primitive_count",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.productVulkanMenu.uiPrimitiveCount);
         }},
        {"product_vulkan_menu_ui_text_count",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.productVulkanMenu.uiTextCount);
         }},
        {"product_vulkan_menu_ui_rect_count",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.productVulkanMenu.uiRectCount);
         }},
        {"product_vulkan_menu_ui_row_count",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.productVulkanMenu.uiRowCount);
         }},
        {"product_vulkan_menu_ui_selected_action",
         [](RenderReceipt& receipt,
            const FeedbackSurfaceAutomationVulkanReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.productVulkanMenu.uiSelectedAction);
         }},
    }};

}  // namespace

void appendProductFeedbackSurfaceAutomationVulkanFields(RenderReceipt& receipt, const ProductAppWindowState& window, const GameplayFeedback& feedback, const ProductActiveSurfaceFrame& activeSurface, const ProductVulkanGameplayReadiness& vulkanGameplayReadiness) {
  const FeedbackSurfaceAutomationVulkanReceiptContext context{
      window,
      feedback,
      activeSurface,
      vulkanGameplayReadiness,
  };

  for (const FeedbackSurfaceAutomationVulkanReceiptFieldRow& row :
       kFeedbackSurfaceAutomationVulkanReceiptFields) {
    row.append(receipt, context, row.key);
  }
}

}  // namespace iggy3d
