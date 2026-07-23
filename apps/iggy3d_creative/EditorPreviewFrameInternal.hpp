#pragma once

#include "EditorPreviewFrame.hpp"
#include "EditorVolume.hpp"
#include "app/iggy3d/creative/tools/ShapeBrush.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorVolumePreviewFacts {
  iggy3d::creative::CreativeVolumeSelection selection{};
  iggy3d::creative::CreativeShapeBrushPlanReceipt shapePlan{};
  iggy3d::creative::CreativeVolumeOperationReceipt operationPreview{};
  bool selectionVisible = false;
  bool usesShapePlan = false;
  bool hasOperationPreview = false;
  bool terrainRegion = false;
  bool terrainStamp = false;
};

struct CreativeEditorWorldOverlayFacts {
  CreativeEditorVolumePreviewFacts volume{};
  bool hasSelection = false;
};

[[nodiscard]] bool creativePreviewBoundsTransform(
    const iggy3d::creative::CreativeBounds& bounds,
    float insetScale,
    iggy3d::Vec3& center,
    iggy3d::Vec3& size) noexcept;

[[nodiscard]] CreativeEditorWorldOverlayFacts
buildCreativeEditorWorldWireframes(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output,
    const CreativeEditorPlacementVisualizationReceipt*
        placementVisualization = nullptr);

void appendCreativeEditorHudOverlays(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output,
    const CreativeEditorVolumePreviewFacts& volumeFacts,
    bool hasSelection);

void appendCreativeEditorArchitectureScaleLabel(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output);

}  // namespace iggy3d_creative_app
