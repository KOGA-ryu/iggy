#pragma once

#include <cstdint>
#include <vector>

#include "app/iggy3d/creative/camera/Fly.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "core/math/Vec3.hpp"

#include "EditorAssetLibrary.hpp"
#include "EditorCatalog.hpp"
#include "EditorAssetReplacement.hpp"
#include "EditorCapture.hpp"
#include "EditorControls.hpp"
#include "EditorDesktopUi.hpp"
#include "EditorEdits.hpp"
#include "EditorGroup.hpp"
#include "EditorInteraction.hpp"
#include "EditorLogicLinks.hpp"
#include "EditorMovingPlatformPreview.hpp"
#include "EditorPattern.hpp"
#include "EditorTerrain.hpp"
#include "EditorTerrainPaint.hpp"
#include "EditorToolOptions.hpp"
#include "EditorTransform.hpp"
#include "EditorVolume.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorDocumentTransientState {
  iggy3d::creative::CreativeHeldItemKind synchronizedHeldItemKind =
      iggy3d::creative::CreativeHeldItemKind::Count;
  CreativeEditorWorldTarget target;
  CreativeEditorPlacementFeedback placementFeedback;
  CreativeMaterialBrushPivotState materialBrushPivot;
  CreativeMaterialStrokeState materialStroke;
  CreativeAssetScatterStrokeState assetScatter;
  CreativeAuthoredAssetStrokeState authoredAssetStroke;
  CreativeEditorConnectedFillCache connectedFill;
  CreativeEditorSurfaceExtrudeCache surfaceExtrude;
  iggy3d::creative::CreativeObjectId moveTargetId =
      iggy3d::creative::kInvalidObjectId;
  CreativeEditorGroupFocusState groupFocus;
  CreativeEditorPatternState pattern;
  CreativeEditorAssetReplacementState assetReplacement;
  CreativeEditorSelectionTransformState transform;
  CreativeEditorTerrainState terrain;
  CreativeEditorTerrainPaintState terrainPaint;
  CreativeEditorVolumeState volume;
  CreativeEditorLogicLinkState logicLinks;
  CreativeMovingPlatformPreviewState movingPlatformPreview;
};

struct CreativeEditorAuthoredAssetEditSession {
  bool active = false;
  bool menuOpen = false;
  std::string assetId;
  std::string label;
  iggy3d::creative::CreativeAppState workspace;
  iggy3d::creative::CreativeAuthoredAssetFingerprint initialFingerprint;
  std::uint64_t observedDocumentRevision = 0U;
  bool dirty = false;
  CreativeEditorAssetEditMenuAction menuAction =
      CreativeEditorAssetEditMenuAction::ContinueEditing;
  CreativeEditorDocumentTransientState mapDocumentState;
  iggy3d::Vec3 mapFlyPosition{};
  float mapYawDegrees = 0.0F;
  float mapPitchDegrees = 0.0F;
  std::string statusLabel;
};

struct CreativeEditorState {
  iggy3d::ProductCreativeFlyConfig flyConfig{};
  iggy3d::Vec3 flyPos{0.0F, 6.0F, 12.0F};
  float yawDegrees = 0.0F;
  float pitchDegrees = -25.0F;
  bool rightStickLookRearmRequired = false;
  iggy3d::creative::CreativeControlDevice activeControlDevice =
      iggy3d::creative::CreativeControlDevice::KeyboardMouse;

  bool loggedSelection = false;
  iggy3d::creative::CreativeControlProfile controlProfile =
      iggy3d::creative::makeDefaultCreativeControlProfile();
  iggy3d::creative::CreativeInputRouterState inputRouterState;
  iggy3d::creative::CreativeControllerState controllerInputState;
  CreativeEditorControlsState controls;
  CreativeEditorDesktopUiState desktopUi;
  CreativeEditorInteractionState interaction;
  CreativeEditorCatalogState catalog;
  CreativeEditorAuthoredAssetLibrary authoredAssets;
  CreativeEditorAssetLibraryState assetLibrary;
  CreativeEditorAuthoredAssetEditSession assetEdit;
  iggy3d::creative::CreativeToolSettings toolSettings =
      iggy3d::creative::makeDefaultCreativeToolSettings();
  CreativeEditorToolOptionsState toolOptions;
  CreativeEditorQuickEditState quickEdit;
  CreativeEditorGroupFocusState groupFocus;
  CreativeEditorPatternState pattern;
  CreativeEditorAssetReplacementState assetReplacement;
  CreativeEditorSelectionTransformState transform;
  CreativeEditorTerrainState terrain;
  CreativeEditorTerrainPaintState terrainPaint;
  CreativeEditorLogicLinkState logicLinks;
  CreativeMovingPlatformPreviewState movingPlatformPreview;

  bool placeMode = false;
  std::vector<iggy3d::creative::CreativeObjectKind> brushPalette;
  iggy3d::creative::CreativeObjectKind placeBrush =
      iggy3d::creative::CreativeObjectKind::Unknown;
  double placeCellSize = 1.0;
  std::uint64_t placedCount = 0;

  CreativeEditorVolumeState volume;

  StandaloneCaptureScript captureScript;
  bool captureWorldPickFloorLogged = false;
  bool captureWorldPickPointLogged = false;
  bool captureWorldPickLineLogged = false;
  bool captureWorldPickPathLogged = false;

  std::uint64_t frameIndex = 0;
  std::uint32_t lastWidth = 0;
  std::uint32_t lastHeight = 0;
};

}  // namespace iggy3d_creative_app
