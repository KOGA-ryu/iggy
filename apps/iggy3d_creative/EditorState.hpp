#pragma once

#include <cstdint>
#include <vector>

#include "EditorPlaytestLaunch.hpp"
#include "app/iggy3d/creative/camera/Fly.hpp"
#include "app/iggy3d/creative/camera/ViewportNavigation.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "core/math/Vec3.hpp"

#include "EditorAssetLibrary.hpp"
#include "EditorCatalog.hpp"
#include "EditorAssetReplacement.hpp"
#include "EditorCapture.hpp"
#include "EditorControls.hpp"
#include "EditorDesktopModel.hpp"
#include "EditorDesktopUi.hpp"
#include "EditorEdits.hpp"
#include "EditorGroup.hpp"
#include "EditorInteraction.hpp"
#include "EditorLogicLinks.hpp"
#include "EditorMovingPlatformPreview.hpp"
#include "EditorPattern.hpp"
#include "EditorTerrain.hpp"
#include "EditorTerrainGeneration.hpp"
#include "EditorTerrainPaint.hpp"
#include "EditorTerrainStampLibrary.hpp"
#include "EditorToolOptions.hpp"
#include "EditorTransform.hpp"
#include "EditorVolume.hpp"
#include "EditorWorldLayoutState.hpp"
#include "EditorWorldLayoutPlanView.hpp"
#include "EditorWorldLayoutTopography.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorDocumentTransientState {
  iggy3d::creative::CreativeHeldItemKind synchronizedHeldItemKind =
      iggy3d::creative::CreativeHeldItemKind::Count;
  CreativeEditorWorldTarget target;
  CreativeEditorPlacementFeedback placementFeedback;
  CreativeMaterialBrushPivotState materialBrushPivot;
  CreativeMaterialStrokeState materialStroke;
  CreativeEditorStructuralSpanState structuralSpan;
  CreativeEditorStructuralSpanEditState structuralSpanEdit;
  CreativeAssetScatterStrokeState assetScatter;
  CreativeAuthoredAssetStrokeState authoredAssetStroke;
  CreativeEditorConnectedFillCache connectedFill;
  CreativeEditorSurfaceExtrudeCache surfaceExtrude;
  CreativeEditorRoomPlacementState roomPlacement;
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
  iggy3d::ProductCreativeViewportFocus mapViewportFocus;
  std::string statusLabel;
};

struct CreativeEditorPersistenceState {
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  std::uint64_t savedRevision = 0U;
  std::uint64_t savedFingerprint = 0U;
  bool hasSavePoint = false;
  mutable iggy3d::creative::CreativeDocumentId cachedDocumentId =
      iggy3d::creative::kInvalidDocumentId;
  mutable std::uint64_t cachedRevision = 0U;
  mutable std::uint64_t cachedFingerprint = 0U;
  mutable bool cachedFingerprintValid = false;
};

struct CreativeEditorState {
  iggy3d::ProductCreativeFlyConfig flyConfig{};
  iggy3d::Vec3 flyPos{0.0F, 6.0F, 12.0F};
  float yawDegrees = 0.0F;
  float pitchDegrees = -25.0F;
  iggy3d::ProductCreativeViewportNavigationConfig viewportNavigationConfig;
  iggy3d::ProductCreativeViewportFocus viewportFocus;
  bool rightStickLookRearmRequired = false;
  iggy3d::creative::CreativeControlDevice activeControlDevice =
      iggy3d::creative::CreativeControlDevice::KeyboardMouse;

  bool loggedSelection = false;
  iggy3d::creative::CreativeControlProfile controlProfile =
      iggy3d::creative::makeDefaultCreativeControlProfile();
  // The optional [playtest] window preferences from the control profile
  // file (loaded at boot, preserved on control saves, read at Play).
  PlaytestWindowPreferences playtestWindowPreferences;
  iggy3d::creative::CreativeInputRouterState inputRouterState;
  iggy3d::creative::CreativeControllerState controllerInputState;
  CreativeEditorControlsState controls;
  CreativeEditorDesktopUiState desktopUi;
  CreativeEditorPersistenceState persistence;
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
  CreativeEditorTerrainGenerationState terrainGeneration;
  CreativeEditorTerrainPaintState terrainPaint;
  CreativeEditorTerrainStampLibraryState terrainStamps;
  CreativeEditorLogicLinkState logicLinks;
  CreativeMovingPlatformPreviewState movingPlatformPreview;
  CreativeEditorWorldLayoutState worldLayout;
  CreativeEditorWorldLayoutTopographyState worldLayoutTopography;
  CreativeEditorWorldLayoutPlanViewCache worldLayoutPlanView;
  CreativeDesktopGeneratedSourceScopeCache generatedSourceScopeCache;

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
