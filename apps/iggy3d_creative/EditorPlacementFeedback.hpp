#pragma once

#include <string>

namespace iggy3d::creative {

class CreativeDocument;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorOverlayFrame;
struct CreativeEditorOverlayFrameRequest;
struct CreativeEditorPlacementFeedback;

[[nodiscard]] std::string creativeEditorPlacementFeedbackLabel(
    const CreativeEditorPlacementFeedback& feedback,
    const iggy3d::creative::CreativeDocument& document);

void appendCreativeEditorPlacementClearanceWireframes(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output);

}  // namespace iggy3d_creative_app
