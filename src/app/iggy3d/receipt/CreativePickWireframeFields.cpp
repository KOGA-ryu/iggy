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

struct CreativePickWireframeReceiptContext {
  const ProductAppWindowState& window;
  const CreativeAuthoringStore& authoring;
  const ProductVulkanGameplayReadiness& vulkanGameplayReadiness;
};

struct CreativePickWireframeReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const CreativePickWireframeReceiptContext& context,
                 std::string_view key);
};

const std::array<CreativePickWireframeReceiptFieldRow, 52>
    kCreativePickWireframeReceiptFields{{
        {"creative_viewport_pick_requested",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeViewportPickRequested);
         }},
        {"creative_viewport_pick_active",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeViewportPickActive);
         }},
        {"creative_viewport_pick_click_present",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeViewportPickClickPresent);
         }},
        {"creative_viewport_pick_click_suppressed",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeViewportPickClickSuppressed);
         }},
        {"creative_viewport_pick_facade_available",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeViewportPickFacadeAvailable);
         }},
        {"creative_viewport_pick_source_available",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeViewportPickSourceAvailable);
         }},
        {"creative_viewport_pick_projected",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeViewportPickProjected);
         }},
        {"creative_viewport_pick_picked",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeViewportPickPicked);
         }},
        {"creative_viewport_pick_object_count",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeViewportPickObjectCount);
         }},
        {"creative_viewport_pick_projection_cell_count",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeViewportPickProjectionCellCount);
         }},
        {"creative_viewport_pick_status",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeViewportPickStatus);
         }},
        {"creative_viewport_pick_reason_code",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeViewportPickReasonCode);
         }},
        {"creative_viewport_pick_pick_status",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeViewportPickPickStatus);
         }},
        {"creative_viewport_pick_message",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeViewportPickMessage);
         }},
        {"creative_viewport_pick_coord_x",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, std::to_string(context.authoring.creativeViewportPickCoordX));
         }},
        {"creative_viewport_pick_coord_y",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, std::to_string(context.authoring.creativeViewportPickCoordY));
         }},
        {"creative_viewport_pick_coord_z",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, std::to_string(context.authoring.creativeViewportPickCoordZ));
         }},
        {"creative_viewport_pick_grid_index",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeViewportPickGridIndex);
         }},
        {"creative_viewport_pick_object_id",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeViewportPickObjectId);
         }},
        {"creative_viewport_pick_object_kind",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeViewportPickObjectKind);
         }},
        {"creative_viewport_pick_occupancy_kind",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeViewportPickOccupancyKind);
         }},
        {"creative_viewport_pick_target",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeViewportPickTarget);
         }},
        {"creative_viewport_pick_cell_index",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeViewportPickCellIndex);
         }},
        {"creative_wireframe_requested",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeRequested);
         }},
        {"creative_wireframe_active",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeActive);
         }},
        {"creative_wireframe_facade_available",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeFacadeAvailable);
         }},
        {"creative_wireframe_document_available",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeDocumentAvailable);
         }},
        {"creative_wireframe_source_available",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeSourceAvailable);
         }},
        {"creative_wireframe_object_count",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeObjectCount);
         }},
        {"creative_wireframe_visible_object_count",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeVisibleObjectCount);
         }},
        {"creative_wireframe_item_count",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeItemCount);
         }},
        {"creative_wireframe_segment_count",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeSegmentCount);
         }},
        {"creative_wireframe_box_item_count",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeBoxItemCount);
         }},
        {"creative_wireframe_line_item_count",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeLineItemCount);
         }},
        {"creative_wireframe_point_item_count",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframePointItemCount);
         }},
        {"creative_wireframe_skipped_degenerate_count",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeSkippedDegenerateCount);
         }},
        {"creative_wireframe_status",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeStatus);
         }},
        {"creative_wireframe_reason_code",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeReasonCode);
         }},
        {"creative_wireframe_wireframe_status",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeWireframeStatus);
         }},
        {"creative_wireframe_wireframe_reason_code",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeWireframeReasonCode);
         }},
        {"creative_wireframe_segment_status",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeSegmentStatus);
         }},
        {"creative_wireframe_segment_reason_code",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeSegmentReasonCode);
         }},
        {"creative_wireframe_debug_line_requested",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeDebugLineRequested);
         }},
        {"creative_wireframe_debug_line_source_available",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeDebugLineSourceAvailable);
         }},
        {"creative_wireframe_debug_line_input_segment_count",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeDebugLineInputSegmentCount);
         }},
        {"creative_wireframe_debug_line_count",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeDebugLineCount);
         }},
        {"creative_wireframe_debug_line_skipped_degenerate_count",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeDebugLineSkippedDegenerateCount);
         }},
        {"creative_wireframe_debug_line_status",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeDebugLineStatus);
         }},
        {"creative_wireframe_debug_line_reason_code",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.authoring.creativeWireframeDebugLineReasonCode);
         }},
        {"product_vulkan_gameplay_ready",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.vulkanGameplayReadiness.ready);
         }},
        {"product_vulkan_gameplay_status",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.vulkanGameplayReadiness.status);
         }},
        {"product_vulkan_gameplay_reason_code",
         [](RenderReceipt& receipt,
            const CreativePickWireframeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.vulkanGameplayReadiness.reasonCode);
         }},
    }};

}  // namespace

void appendProductCreativePickWireframeFields(RenderReceipt& receipt, const ProductAppWindowState& window, const ProductVulkanGameplayReadiness& vulkanGameplayReadiness) {
  const CreativePickWireframeReceiptContext context{
      window,
      window.creativeAuthoring,
      vulkanGameplayReadiness,
  };

  for (const CreativePickWireframeReceiptFieldRow& row :
       kCreativePickWireframeReceiptFields) {
    row.append(receipt, context, row.key);
  }
}

}  // namespace iggy3d
