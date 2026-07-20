#include "EditorPreviewFrame.hpp"
#include "EditorPreviewFrameInternal.hpp"

#include "EditorInteraction.hpp"
#include "EditorState.hpp"

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

  const CreativeEditorPlacementVisualizationReceipt placementVisualization =
      attachCreativeEditorPlacementPreviews(
          editor, request.captureMode, frame,
          &request.appState.facade.document(), request.assetCatalog,
          request.placementClearanceCache);

  const CreativeEditorWorldOverlayFacts worldFacts =
      buildCreativeEditorWorldWireframes(request, output,
                                         &placementVisualization);

  // DD-15: with the desktop shell on, the legacy controller HUD (hotbar,
  // catalog, tool wheel, action hints, held-item labels) renders only for a
  // gamepad; on keyboard/mouse the ImGui panels are primary, so it is
  // suppressed to stop it colliding with the status bar. The center crosshair
  // and the selection wireframes are always kept. Under --capture the shell is
  // off (shellEnabled == false), so the legacy HUD builds exactly as before and
  // image output is unchanged.
  const bool suppressLegacyHud =
      editor.desktopUi.shellEnabled && !request.captureMode &&
      editor.activeControlDevice ==
          iggy3d::creative::CreativeControlDevice::KeyboardMouse;
  const iggy3d::RenderContentViewport crosshairRegion =
      iggy3d::effectiveContentViewport(request.frame);
  appendCreativeEditorCrosshairOverlay(editor, crosshairRegion,
                                       output.uiRects);
  appendCreativeEditorArchitectureScaleLabel(request, output);
  if (!suppressLegacyHud) {
    appendCreativeEditorHudOverlays(
        request, output, worldFacts.volume, worldFacts.hasSelection);
  }

  // Attach only after every backing vector has reached its final size.
  attachCreativeEditorOverlayFrame(frame, output);
}

}  // namespace iggy3d_creative_app
