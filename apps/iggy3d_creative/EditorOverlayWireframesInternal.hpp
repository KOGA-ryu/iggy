#pragma once

#include "EditorPreviewFrameInternal.hpp"

namespace iggy3d_creative_app {

void appendCreativeEditorMaterialBrushWireframe(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output);
void appendCreativeEditorConnectedFillWireframe(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output);
void appendCreativeEditorSurfaceExtrudeWireframe(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output);
void appendCreativeEditorArchitectureScaleGuide(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output);
[[nodiscard]] CreativeEditorVolumePreviewFacts
appendCreativeEditorVolumeAndToolWireframes(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output);

}  // namespace iggy3d_creative_app
