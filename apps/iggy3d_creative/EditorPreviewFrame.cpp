#include "EditorPreviewFrame.hpp"
#include "EditorPreviewFrameInternal.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace creative = iggy3d::creative;
using namespace iggy3d;
namespace {

void attachCreativeEditorOverlayFrame(FrameInput& frame,
                                      CreativeEditorOverlayFrame& output) {
  frame.ui.visible = true;
  frame.ui.rects = output.uiRects.data();
  frame.ui.rectCount = output.uiRects.size();
  frame.ui.textGlyphQuads = output.glyphs.data();
  frame.ui.textGlyphQuadCount = output.glyphs.size();

  RenderCreativeWireframeDebugFrame combinedWireFrame;
  combinedWireFrame.available = true;
  combinedWireFrame.visible = !output.combinedWireLines.empty();
  combinedWireFrame.lines = output.combinedWireLines.data();
  combinedWireFrame.lineCount = output.combinedWireLines.size();
  frame.creativeWireframeDebug = combinedWireFrame;
}

}  // namespace

void buildAndAttachCreativeEditorOverlayFrame(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  CreativeEditorState& editor = request.editor;
  FrameInput& frame = request.frame;

  attachCreativeEditorPlacementPreviews(
      editor, request.captureMode, frame,
      &request.appState.facade.document());

  const CreativeEditorWorldOverlayFacts worldFacts =
      buildCreativeEditorWorldWireframes(request, output);
  appendCreativeEditorHudOverlays(
      request, output, worldFacts.volume, worldFacts.hasSelection);

  // Attach only after every backing vector has reached its final size.
  attachCreativeEditorOverlayFrame(frame, output);
}

}  // namespace iggy3d_creative_app
