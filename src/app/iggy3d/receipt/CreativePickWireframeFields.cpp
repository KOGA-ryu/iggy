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

void appendProductCreativePickWireframeFields(RenderReceipt& receipt, const ProductAppWindowState& window, const ProductVulkanGameplayReadiness& vulkanGameplayReadiness) {
  appendReceiptField(receipt, "creative_viewport_pick_requested",
                     window.creativeViewportPickRequested);
  appendReceiptField(receipt, "creative_viewport_pick_active",
                     window.creativeViewportPickActive);
  appendReceiptField(receipt, "creative_viewport_pick_click_present",
                     window.creativeViewportPickClickPresent);
  appendReceiptField(receipt, "creative_viewport_pick_click_suppressed",
                     window.creativeViewportPickClickSuppressed);
  appendReceiptField(receipt, "creative_viewport_pick_facade_available",
                     window.creativeViewportPickFacadeAvailable);
  appendReceiptField(receipt, "creative_viewport_pick_source_available",
                     window.creativeViewportPickSourceAvailable);
  appendReceiptField(receipt, "creative_viewport_pick_projected",
                     window.creativeViewportPickProjected);
  appendReceiptField(receipt, "creative_viewport_pick_picked",
                     window.creativeViewportPickPicked);
  appendReceiptField(receipt, "creative_viewport_pick_object_count",
                     window.creativeViewportPickObjectCount);
  appendReceiptField(receipt, "creative_viewport_pick_projection_cell_count",
                     window.creativeViewportPickProjectionCellCount);
  appendReceiptField(receipt, "creative_viewport_pick_status",
                     window.creativeViewportPickStatus);
  appendReceiptField(receipt, "creative_viewport_pick_reason_code",
                     window.creativeViewportPickReasonCode);
  appendReceiptField(receipt, "creative_viewport_pick_pick_status",
                     window.creativeViewportPickPickStatus);
  appendReceiptField(receipt, "creative_viewport_pick_message",
                     window.creativeViewportPickMessage);
  appendReceiptField(receipt, "creative_viewport_pick_coord_x",
                     std::to_string(window.creativeViewportPickCoordX));
  appendReceiptField(receipt, "creative_viewport_pick_coord_y",
                     std::to_string(window.creativeViewportPickCoordY));
  appendReceiptField(receipt, "creative_viewport_pick_coord_z",
                     std::to_string(window.creativeViewportPickCoordZ));
  appendReceiptField(receipt, "creative_viewport_pick_grid_index",
                     window.creativeViewportPickGridIndex);
  appendReceiptField(receipt, "creative_viewport_pick_object_id",
                     window.creativeViewportPickObjectId);
  appendReceiptField(receipt, "creative_viewport_pick_object_kind",
                     window.creativeViewportPickObjectKind);
  appendReceiptField(receipt, "creative_viewport_pick_occupancy_kind",
                     window.creativeViewportPickOccupancyKind);
  appendReceiptField(receipt, "creative_viewport_pick_target",
                     window.creativeViewportPickTarget);
  appendReceiptField(receipt, "creative_viewport_pick_cell_index",
                     window.creativeViewportPickCellIndex);
  appendReceiptField(receipt, "creative_wireframe_requested",
                     window.creativeWireframeRequested);
  appendReceiptField(receipt, "creative_wireframe_active",
                     window.creativeWireframeActive);
  appendReceiptField(receipt, "creative_wireframe_facade_available",
                     window.creativeWireframeFacadeAvailable);
  appendReceiptField(receipt, "creative_wireframe_document_available",
                     window.creativeWireframeDocumentAvailable);
  appendReceiptField(receipt, "creative_wireframe_source_available",
                     window.creativeWireframeSourceAvailable);
  appendReceiptField(receipt, "creative_wireframe_object_count",
                     window.creativeWireframeObjectCount);
  appendReceiptField(receipt, "creative_wireframe_visible_object_count",
                     window.creativeWireframeVisibleObjectCount);
  appendReceiptField(receipt, "creative_wireframe_item_count",
                     window.creativeWireframeItemCount);
  appendReceiptField(receipt, "creative_wireframe_segment_count",
                     window.creativeWireframeSegmentCount);
  appendReceiptField(receipt, "creative_wireframe_box_item_count",
                     window.creativeWireframeBoxItemCount);
  appendReceiptField(receipt, "creative_wireframe_line_item_count",
                     window.creativeWireframeLineItemCount);
  appendReceiptField(receipt, "creative_wireframe_point_item_count",
                     window.creativeWireframePointItemCount);
  appendReceiptField(receipt,
                     "creative_wireframe_skipped_degenerate_count",
                     window.creativeWireframeSkippedDegenerateCount);
  appendReceiptField(receipt, "creative_wireframe_status",
                     window.creativeWireframeStatus);
  appendReceiptField(receipt, "creative_wireframe_reason_code",
                     window.creativeWireframeReasonCode);
  appendReceiptField(receipt, "creative_wireframe_wireframe_status",
                     window.creativeWireframeWireframeStatus);
  appendReceiptField(receipt, "creative_wireframe_wireframe_reason_code",
                     window.creativeWireframeWireframeReasonCode);
  appendReceiptField(receipt, "creative_wireframe_segment_status",
                     window.creativeWireframeSegmentStatus);
  appendReceiptField(receipt, "creative_wireframe_segment_reason_code",
                     window.creativeWireframeSegmentReasonCode);
  appendReceiptField(receipt, "creative_wireframe_debug_line_requested",
                     window.creativeWireframeDebugLineRequested);
  appendReceiptField(receipt, "creative_wireframe_debug_line_source_available",
                     window.creativeWireframeDebugLineSourceAvailable);
  appendReceiptField(receipt,
                     "creative_wireframe_debug_line_input_segment_count",
                     window.creativeWireframeDebugLineInputSegmentCount);
  appendReceiptField(receipt, "creative_wireframe_debug_line_count",
                     window.creativeWireframeDebugLineCount);
  appendReceiptField(receipt,
                     "creative_wireframe_debug_line_skipped_degenerate_count",
                     window.creativeWireframeDebugLineSkippedDegenerateCount);
  appendReceiptField(receipt, "creative_wireframe_debug_line_status",
                     window.creativeWireframeDebugLineStatus);
  appendReceiptField(receipt, "creative_wireframe_debug_line_reason_code",
                     window.creativeWireframeDebugLineReasonCode);
  appendReceiptField(receipt, "product_vulkan_gameplay_ready",
                     vulkanGameplayReadiness.ready);
  appendReceiptField(receipt, "product_vulkan_gameplay_status",
                     vulkanGameplayReadiness.status);
  appendReceiptField(receipt, "product_vulkan_gameplay_reason_code",
                     vulkanGameplayReadiness.reasonCode);
}

}  // namespace iggy3d
