#include "EditorFrame.hpp"
#include "EditorGizmo.hpp"
#include "EditorPreviewFrameInternal.hpp"
#include "EditorState.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

namespace {
namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float lhs, float rhs) {
  return std::fabs(lhs - rhs) <= 1.0e-6F;
}

cr::CreativeObjectId create(
    cr::Facade& facade,
    cr::CreativeObjectKind kind,
    std::string name,
    cr::CreativeVec3 position) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::move(name);
  request.transform.position = position;
  request.hasTransformOverride = true;
  return facade.createDocumentObject(request).objectId;
}

bool representativeDomainsKeepOneOrderedLineStream() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("overlay assembler parity");
  static_cast<void>(document.assignId(880U));
  if (!appState.facade.installDocument(std::move(document)).accepted) {
    return expect(false, "representative overlay document installed");
  }
  const cr::CreativeObjectId source =
      create(appState.facade, cr::CreativeObjectKind::Switch, "Switch",
             {-1.0, 0.0, 0.0});
  const cr::CreativeObjectId target =
      create(appState.facade, cr::CreativeObjectKind::Door, "Door",
             {1.0, 0.0, 0.0});
  const cr::CreativeObjectId placed =
      create(appState.facade, cr::CreativeObjectKind::Crate, "Crate",
             {0.0, 0.0, 2.0});
  static_cast<void>(appState.facade.setLogicLink(
      {source, target, cr::CreativeLogicLinkAction::Toggle}));
  static_cast<void>(appState.facade.configureMeasurement(
      cr::CreativeMeasurementMode::Distance,
      cr::CreativeMeasurementAxis::X, false));
  static_cast<void>(appState.facade.appendMeasurementPoint(
      {-2.0, 0.0, -2.0, {}, cr::CreativeMeasurementSnapKind::Grid}));
  static_cast<void>(appState.facade.appendMeasurementPoint(
      {2.0, 0.0, -2.0, {}, cr::CreativeMeasurementSnapKind::Surface}));

  app::CreativeEditorState editor;
  editor.frameIndex = 7U;
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::LogicLink;
  editor.logicLinks.documentId = appState.facade.document().id();
  editor.logicLinks.sourceObjectId = source;
  editor.interaction.target.objectHit = true;
  editor.interaction.target.objectId = target;
  editor.interaction.placementFeedback.status =
      app::CreativeEditorPlacementFeedbackStatus::Placed;
  editor.interaction.placementFeedback.objectId = placed;
  editor.interaction.placementFeedback.frameIndex = editor.frameIndex;

  app::CreativeEditorSelectionFrame selection;
  app::CreativeEditorGizmoFrame gizmo;
  iggy3d::FrameInput frame;
  frame.viewport.width = 800U;
  frame.viewport.height = 600U;
  cr::CreativeSpatialProjectionRequest projection;
  projection.gridSize = {32U, 16U, 32U};
  app::CreativeEditorOverlayFrame output;
  const app::CreativeEditorOverlayFrameRequest request{
      appState, editor, selection, gizmo, frame, projection,
      800U, 600U, 0.05F};
  const app::CreativeEditorWorldOverlayFacts facts =
      app::buildCreativeEditorWorldWireframes(request, output);

  const std::size_t measurementBegin =
      output.documentWireLineCount + output.placementGridLineCount;
  const std::size_t placementBegin =
      measurementBegin + output.measurementEdgeCount;
  const std::size_t logicBegin =
      placementBegin + output.placementFeedbackEdgeCount;
  if (output.documentWireLineCount != 24U ||
      output.measurementEdgeCount != 7U ||
      output.placementFeedbackEdgeCount != 12U ||
      output.logicLinkEdgeCount != 27U) {
    std::cerr << "INFO: overlay counts document="
              << output.documentWireLineCount
              << " measurement=" << output.measurementEdgeCount
              << " grid=" << output.placementGridLineCount
              << " placement=" << output.placementFeedbackEdgeCount
              << " logic=" << output.logicLinkEdgeCount
              << " total=" << output.combinedWireLines.size() << '\n';
  }
  if (!expect(output.documentWireLineCount == 24U,
              "base document keeps two box bodies after line filtering") ||
      !expect(output.measurementEdgeCount == 7U,
              "measurement contributes one segment and two point crosses") ||
      !expect(output.placementFeedbackEdgeCount == 12U,
              "placement feedback contributes one box") ||
      !expect(output.logicLinkEdgeCount == 27U &&
                  output.logicLinkLabelGlyphCount == 6U,
              "logic contributes arrow, endpoint boxes, and label") ||
      !expect(output.combinedWireLines.size() ==
                  logicBegin + output.logicLinkEdgeCount,
              "representative domains share one contiguous line stream")) {
    return false;
  }

  const iggy3d::RenderCreativeWireframeDebugLine& documentLine =
      output.combinedWireLines.front();
  const iggy3d::RenderCreativeWireframeDebugLine& measurementLine =
      output.combinedWireLines[measurementBegin];
  const iggy3d::RenderCreativeWireframeDebugLine& placementLine =
      output.combinedWireLines[placementBegin];
  const iggy3d::RenderCreativeWireframeDebugLine& logicLine =
      output.combinedWireLines[logicBegin];
  return expect(documentLine.objectId != cr::kInvalidObjectId &&
                    near(documentLine.thickness, 0.03F),
                "document signature stays first") &&
         expect(measurementLine.objectId == cr::kInvalidObjectId &&
                    near(measurementLine.color.r, 0.18F) &&
                    near(measurementLine.color.g, 0.90F) &&
                    near(measurementLine.thickness, 0.035F),
                "measurement signature follows document interactions") &&
         expect(placementLine.objectId == placed &&
                    near(placementLine.color.r, 0.25F) &&
                    near(placementLine.color.g, 1.0F),
                "placement feedback signature follows measurements") &&
         expect(logicLine.objectId == source &&
                    logicLine.segmentKind == 0U &&
                    near(logicLine.thickness, 0.05F),
                "logic signature follows placement feedback") &&
         expect(!output.generatedScopeActive &&
                    output.generatedScopeEdgeCount == 0U &&
                    !facts.hasSelection &&
                    !facts.volume.selectionVisible &&
                    !facts.volume.hasOperationPreview &&
                    !facts.volume.terrainRegion &&
                    !facts.volume.terrainStamp,
                "compatibility counts and final facts remain exact");
}

std::string readFile(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  return {std::istreambuf_iterator<char>{input},
          std::istreambuf_iterator<char>{}};
}

bool leafAssemblersStayNarrowAndAllocationNeutral() {
  constexpr std::array sources{
      std::string_view{
          "apps/iggy3d_creative/EditorOverlayDocumentAssembler.cpp"},
      std::string_view{
          "apps/iggy3d_creative/EditorOverlayPlacementAssembler.cpp"},
      std::string_view{
          "apps/iggy3d_creative/EditorOverlayRelationshipsAssembler.cpp"},
      std::string_view{
          "apps/iggy3d_creative/EditorOverlayWorldAssembler.cpp"},
  };
  bool clean = true;
  for (std::string_view source : sources) {
    const std::string text = readFile(source);
    clean = expect(
                !text.empty() &&
                    text.find("EditorState.hpp") == std::string::npos &&
                    text.find("CreativeEditorState") == std::string::npos,
                "leaf assembler excludes broad editor state") &&
            clean;
    clean = expect(text.find(".reserve(") == std::string::npos,
                   "leaf assembler never owns output capacity") &&
            clean;
  }
  const std::string coordinator = readFile(
      "apps/iggy3d_creative/EditorOverlayWireframes.cpp");
  return clean &&
         expect(coordinator.find("combinedWireLines.reserve(") !=
                    std::string::npos,
                "coordinator owns the single output reserve");
}

bool coordinatorKeepsEstablishedDomainOrder() {
  const std::string source = readFile(
      "apps/iggy3d_creative/EditorOverlayWireframes.cpp");
  constexpr std::array tokens{
      std::string_view{"appendCreativeEditorOverlayDocumentBase"},
      std::string_view{"appendCreativeEditorArchitectureScaleGuide"},
      std::string_view{"appendCreativeEditorPlacementGridOverlay"},
      std::string_view{
          "appendCreativeEditorOverlayWorldLayoutRoofHandles"},
      std::string_view{
          "appendCreativeEditorOverlayWorldLayoutVerticalConnectorHandles"},
      std::string_view{"appendCreativeEditorOverlayAssetCollision"},
      std::string_view{
          "appendCreativeEditorOverlayDocumentInteraction"},
      std::string_view{"appendCreativeEditorOverlayMeasurement"},
      std::string_view{
          "appendCreativeEditorOverlayMovingPlatformPath"},
      std::string_view{
          "appendCreativeEditorStructuralSpanEditWireframe"},
      std::string_view{
          "appendCreativeEditorOverlayAttachmentSockets"},
      std::string_view{"appendCreativeEditorOverlayPlacementFeedback"},
      std::string_view{
          "appendCreativeEditorPlacementClearanceWireframes"},
      std::string_view{"appendCreativeEditorOverlayLogicLinks"},
      std::string_view{
          "appendCreativeEditorAssetReplacementWireframes"},
      std::string_view{"appendCreativeEditorAssetScatterWireframes"},
      std::string_view{"appendCreativeEditorMaterialBrushWireframe"},
      std::string_view{"appendCreativeEditorConnectedFillWireframe"},
      std::string_view{"appendCreativeEditorSurfaceExtrudeWireframe"},
      std::string_view{
          "appendCreativeEditorVolumeAndToolWireframes"},
  };
  std::size_t previous = 0U;
  for (std::string_view token : tokens) {
    const std::size_t position = source.find(token, previous);
    if (!expect(position != std::string::npos,
                "coordinator retains every ordered domain stage")) {
      return false;
    }
    previous = position + token.size();
  }
  return true;
}

}  // namespace

int main() {
  const bool passed =
      representativeDomainsKeepOneOrderedLineStream() &&
      leafAssemblersStayNarrowAndAllocationNeutral() &&
      coordinatorKeepsEstablishedDomainOrder();
  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
