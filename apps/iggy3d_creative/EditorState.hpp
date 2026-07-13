#pragma once

#include <cstdint>
#include <vector>

#include "app/iggy3d/creative/camera/Fly.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "core/math/Vec3.hpp"

#include "EditorCatalog.hpp"
#include "EditorAssetReplacement.hpp"
#include "EditorCapture.hpp"
#include "EditorControls.hpp"
#include "EditorEdits.hpp"
#include "EditorGroup.hpp"
#include "EditorInteraction.hpp"
#include "EditorPattern.hpp"
#include "EditorTerrain.hpp"
#include "EditorTerrainPaint.hpp"
#include "EditorToolOptions.hpp"
#include "EditorTransform.hpp"
#include "EditorVolume.hpp"

namespace iggy3d_creative_app {

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
  CreativeEditorInteractionState interaction;
  CreativeEditorCatalogState catalog;
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
