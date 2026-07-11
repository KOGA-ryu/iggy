#pragma once

#include <cstdint>
#include <vector>

#include "app/iggy3d/creative/camera/Fly.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "core/math/Vec3.hpp"

#include "EditorCatalog.hpp"
#include "EditorCapture.hpp"
#include "EditorEdits.hpp"
#include "EditorInteraction.hpp"
#include "EditorPattern.hpp"
#include "EditorToolOptions.hpp"
#include "EditorVolume.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorState {
  iggy3d::ProductCreativeFlyConfig flyConfig{};
  iggy3d::Vec3 flyPos{0.0F, 6.0F, 12.0F};
  float yawDegrees = 0.0F;
  float pitchDegrees = -25.0F;

  bool loggedSelection = false;
  iggy3d::creative::CreativeInputRouterState inputRouterState;
  iggy3d::creative::CreativeControllerState controllerInputState;
  CreativeEditorInteractionState interaction;
  CreativeEditorCatalogState catalog;
  iggy3d::creative::CreativeToolSettings toolSettings =
      iggy3d::creative::makeDefaultCreativeToolSettings();
  CreativeEditorToolOptionsState toolOptions;
  CreativeEditorPatternState pattern;
  CreativeEditorClipboardPasteState clipboardPaste;

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
