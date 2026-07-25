#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "EditorDesktopModel.hpp"
#include "EditorGizmo.hpp"
#include "EditorGroup.hpp"
#include "EditorInteraction.hpp"
#include "EditorLogicLinks.hpp"
#include "EditorPathEditing.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorWorldLayoutState.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/input/ControlProfile.hpp"
#include "app/iggy3d/creative/render/CreativeOverlayFrame.hpp"
#include "app/iggy3d/creative/tools/Measure.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorOverlaySelectionSnapshot {
  iggy3d::creative::Id selectedId = 0;
  const iggy3d::creative::CreativeObject* selected = nullptr;
  std::span<const iggy3d::creative::CreativeObjectId> selectedObjectIds;
  std::uint64_t selectionCount = 0;
  bool hasSelection = false;
};

struct CreativeEditorOverlayDocumentAssemblyRequest {
  const iggy3d::creative::CreativeDocument& document;
  const iggy3d::creative::CreativeSpatialProjectionRequest&
      wireProjectionRequest;
  CreativeEditorWorldLayoutState& worldLayout;
  CreativeDesktopGeneratedSourceScopeCache& generatedSourceScopeCache;
  const CreativeEditorGroupFocusState& groupFocus;
  const CreativeMovingPlatformPathEditState& movingPlatformPathEdit;
  const CreativeEditorGizmoFrame& gizmoFrame;
  CreativeEditorOverlaySelectionSnapshot selection;
  float gizmoThickness = 0.0F;
  bool volumeActive = false;
};

struct CreativeEditorOverlayDocumentPlan {
  iggy3d::CreativeWireframeDebugRenderFrame wireframe;
  CreativeDesktopGeneratedSourceScopeSummary generatedScopeSummary;
  iggy3d::RenderLineColor generatedScopeColor{1.0F, 0.82F, 0.22F, 1.0F};
  bool hasSelection = false;
  bool generatedScopeBroad = false;
};

[[nodiscard]] CreativeEditorOverlayDocumentPlan
prepareCreativeEditorOverlayDocument(
    const CreativeEditorOverlayDocumentAssemblyRequest& request,
    CreativeEditorOverlayFrame& output);

void appendCreativeEditorOverlayDocumentBase(
    const CreativeEditorOverlayDocumentAssemblyRequest& request,
    const CreativeEditorOverlayDocumentPlan& plan,
    CreativeEditorOverlayFrame& output);

void appendCreativeEditorOverlayDocumentInteraction(
    const CreativeEditorOverlayDocumentAssemblyRequest& request,
    const CreativeEditorOverlayDocumentPlan& plan,
    CreativeEditorOverlayFrame& output);

struct CreativeEditorOverlayWorldAssemblyRequest {
  const iggy3d::creative::CreativeDocument& document;
  const iggy3d::creative::CreativeMeasurementState& measurementState;
  const CreativeEditorWorldLayoutState& worldLayout;
  const CreativeMovingPlatformPathEditState& movingPlatformPathEdit;
  const CreativeEditorWorldTarget& target;
  const iggy3d::creative::CreativeToolSettings& toolSettings;
  const iggy3d::creative::CreativeHotbarEntry& held;
  const iggy3d::creative::CreativeObject* selected = nullptr;
  iggy3d::creative::CreativeInputContext inputContext =
      iggy3d::creative::CreativeInputContext::EditorViewport;
  float gizmoThickness = 0.0F;
  bool captureMode = false;
  bool modalOpen = false;
};

void appendCreativeEditorOverlayWorldLayoutRoofHandles(
    const CreativeEditorOverlayWorldAssemblyRequest& request,
    CreativeEditorOverlayFrame& output);

void appendCreativeEditorOverlayWorldLayoutVerticalConnectorHandles(
    const CreativeEditorOverlayWorldAssemblyRequest& request,
    CreativeEditorOverlayFrame& output);

void appendCreativeEditorOverlayMeasurement(
    const CreativeEditorOverlayWorldAssemblyRequest& request,
    CreativeEditorOverlayFrame& output);

void appendCreativeEditorOverlayMovingPlatformPath(
    const CreativeEditorOverlayWorldAssemblyRequest& request,
    CreativeEditorOverlayFrame& output);

struct CreativeEditorOverlayPlacementAssemblyRequest {
  const iggy3d::creative::CreativeDocument& document;
  const iggy3d::creative::CreativeHotbarEntry& held;
  const CreativeEditorPlacementFeedback& placementFeedback;
  const CreativeEditorPlacementVisualizationReceipt* visualization = nullptr;
  const iggy3d::StaticMeshAssetCatalog* assetCatalog = nullptr;
  std::uint64_t frameIndex = 0;
  float gizmoThickness = 0.0F;
};

void appendCreativeEditorOverlayAssetCollision(
    const CreativeEditorOverlayPlacementAssemblyRequest& request,
    CreativeEditorOverlayFrame& output);

void appendCreativeEditorOverlayPlacementFeedback(
    const CreativeEditorOverlayPlacementAssemblyRequest& request,
    CreativeEditorOverlayFrame& output);

struct CreativeEditorOverlayRelationshipsAssemblyRequest {
  const iggy3d::creative::CreativeDocument& document;
  const CreativeEditorOverlaySelectionSnapshot& selection;
  const CreativeEditorWorldTarget& target;
  const CreativeEditorLogicLinkState& logicLinks;
  const iggy3d::creative::CreativeHotbarEntry& held;
  const iggy3d::creative::CreativeToolSettings& toolSettings;
  const iggy3d::FrameInput& frame;
  const iggy3d::StaticMeshAssetCatalog* assetCatalog = nullptr;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
  float gizmoThickness = 0.0F;
  bool captureMode = false;
  bool modalOpen = false;
};

void appendCreativeEditorOverlayAttachmentSockets(
    const CreativeEditorOverlayRelationshipsAssemblyRequest& request,
    CreativeEditorOverlayFrame& output);

void appendCreativeEditorOverlayLogicLinks(
    const CreativeEditorOverlayRelationshipsAssemblyRequest& request,
    CreativeEditorOverlayFrame& output);

}  // namespace iggy3d_creative_app
