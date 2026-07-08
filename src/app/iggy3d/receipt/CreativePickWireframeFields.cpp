#include "app/iggy3d/receipt/ReceiptFields.hpp"

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

void appendProductCreativePickWireframeFields(RenderReceipt& receipt, const ProductAppWindowState& window, const ProductVulkanGameplayReadiness& vulkanGameplayReadiness) {
  const CreativeAuthoringStore& authoring = window.creativeAuthoring;
  appendReceiptField(receipt, "creative_viewport_pick_requested",
                     authoring.creativeViewportPickRequested);
  appendReceiptField(receipt, "creative_viewport_pick_active",
                     authoring.creativeViewportPickActive);
  appendReceiptField(receipt, "creative_viewport_pick_click_present",
                     authoring.creativeViewportPickClickPresent);
  appendReceiptField(receipt, "creative_viewport_pick_click_suppressed",
                     authoring.creativeViewportPickClickSuppressed);
  appendReceiptField(receipt, "creative_viewport_pick_facade_available",
                     authoring.creativeViewportPickFacadeAvailable);
  appendReceiptField(receipt, "creative_viewport_pick_source_available",
                     authoring.creativeViewportPickSourceAvailable);
  appendReceiptField(receipt, "creative_viewport_pick_projected",
                     authoring.creativeViewportPickProjected);
  appendReceiptField(receipt, "creative_viewport_pick_picked",
                     authoring.creativeViewportPickPicked);
  appendReceiptField(receipt, "creative_viewport_pick_object_count",
                     authoring.creativeViewportPickObjectCount);
  appendReceiptField(receipt, "creative_viewport_pick_projection_cell_count",
                     authoring.creativeViewportPickProjectionCellCount);
  appendReceiptField(receipt, "creative_viewport_pick_status",
                     authoring.creativeViewportPickStatus);
  appendReceiptField(receipt, "creative_viewport_pick_reason_code",
                     authoring.creativeViewportPickReasonCode);
  appendReceiptField(receipt, "creative_viewport_pick_pick_status",
                     authoring.creativeViewportPickPickStatus);
  appendReceiptField(receipt, "creative_viewport_pick_message",
                     authoring.creativeViewportPickMessage);
  appendReceiptField(receipt, "creative_viewport_pick_coord_x",
                     std::to_string(authoring.creativeViewportPickCoordX));
  appendReceiptField(receipt, "creative_viewport_pick_coord_y",
                     std::to_string(authoring.creativeViewportPickCoordY));
  appendReceiptField(receipt, "creative_viewport_pick_coord_z",
                     std::to_string(authoring.creativeViewportPickCoordZ));
  appendReceiptField(receipt, "creative_viewport_pick_grid_index",
                     authoring.creativeViewportPickGridIndex);
  appendReceiptField(receipt, "creative_viewport_pick_object_id",
                     authoring.creativeViewportPickObjectId);
  appendReceiptField(receipt, "creative_viewport_pick_object_kind",
                     authoring.creativeViewportPickObjectKind);
  appendReceiptField(receipt, "creative_viewport_pick_occupancy_kind",
                     authoring.creativeViewportPickOccupancyKind);
  appendReceiptField(receipt, "creative_viewport_pick_target",
                     authoring.creativeViewportPickTarget);
  appendReceiptField(receipt, "creative_viewport_pick_cell_index",
                     authoring.creativeViewportPickCellIndex);
  appendReceiptField(receipt, "creative_wireframe_requested",
                     authoring.creativeWireframeRequested);
  appendReceiptField(receipt, "creative_wireframe_active",
                     authoring.creativeWireframeActive);
  appendReceiptField(receipt, "creative_wireframe_facade_available",
                     authoring.creativeWireframeFacadeAvailable);
  appendReceiptField(receipt, "creative_wireframe_document_available",
                     authoring.creativeWireframeDocumentAvailable);
  appendReceiptField(receipt, "creative_wireframe_source_available",
                     authoring.creativeWireframeSourceAvailable);
  appendReceiptField(receipt, "creative_wireframe_object_count",
                     authoring.creativeWireframeObjectCount);
  appendReceiptField(receipt, "creative_wireframe_visible_object_count",
                     authoring.creativeWireframeVisibleObjectCount);
  appendReceiptField(receipt, "creative_wireframe_item_count",
                     authoring.creativeWireframeItemCount);
  appendReceiptField(receipt, "creative_wireframe_segment_count",
                     authoring.creativeWireframeSegmentCount);
  appendReceiptField(receipt, "creative_wireframe_box_item_count",
                     authoring.creativeWireframeBoxItemCount);
  appendReceiptField(receipt, "creative_wireframe_line_item_count",
                     authoring.creativeWireframeLineItemCount);
  appendReceiptField(receipt, "creative_wireframe_point_item_count",
                     authoring.creativeWireframePointItemCount);
  appendReceiptField(receipt,
                     "creative_wireframe_skipped_degenerate_count",
                     authoring.creativeWireframeSkippedDegenerateCount);
  appendReceiptField(receipt, "creative_wireframe_status",
                     authoring.creativeWireframeStatus);
  appendReceiptField(receipt, "creative_wireframe_reason_code",
                     authoring.creativeWireframeReasonCode);
  appendReceiptField(receipt, "creative_wireframe_wireframe_status",
                     authoring.creativeWireframeWireframeStatus);
  appendReceiptField(receipt, "creative_wireframe_wireframe_reason_code",
                     authoring.creativeWireframeWireframeReasonCode);
  appendReceiptField(receipt, "creative_wireframe_segment_status",
                     authoring.creativeWireframeSegmentStatus);
  appendReceiptField(receipt, "creative_wireframe_segment_reason_code",
                     authoring.creativeWireframeSegmentReasonCode);
  appendReceiptField(receipt, "creative_wireframe_debug_line_requested",
                     authoring.creativeWireframeDebugLineRequested);
  appendReceiptField(receipt, "creative_wireframe_debug_line_source_available",
                     authoring.creativeWireframeDebugLineSourceAvailable);
  appendReceiptField(receipt,
                     "creative_wireframe_debug_line_input_segment_count",
                     authoring.creativeWireframeDebugLineInputSegmentCount);
  appendReceiptField(receipt, "creative_wireframe_debug_line_count",
                     authoring.creativeWireframeDebugLineCount);
  appendReceiptField(receipt,
                     "creative_wireframe_debug_line_skipped_degenerate_count",
                     authoring.creativeWireframeDebugLineSkippedDegenerateCount);
  appendReceiptField(receipt, "creative_wireframe_debug_line_status",
                     authoring.creativeWireframeDebugLineStatus);
  appendReceiptField(receipt, "creative_wireframe_debug_line_reason_code",
                     authoring.creativeWireframeDebugLineReasonCode);
  appendReceiptField(receipt, "product_vulkan_gameplay_ready",
                     vulkanGameplayReadiness.ready);
  appendReceiptField(receipt, "product_vulkan_gameplay_status",
                     vulkanGameplayReadiness.status);
  appendReceiptField(receipt, "product_vulkan_gameplay_reason_code",
                     vulkanGameplayReadiness.reasonCode);
}

}  // namespace iggy3d
