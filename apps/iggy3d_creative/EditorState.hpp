#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "app/iggy3d/creative/camera/Fly.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "core/math/Vec3.hpp"

#include "EditorCapture.hpp"
#include "EditorGizmo.hpp"
#include "EditorPicking.hpp"
#include "EditorEdits.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorState {
  iggy3d::ProductCreativeFlyConfig flyConfig{};
  iggy3d::Vec3 flyPos{0.0F, 6.0F, 12.0F};
  float yawDegrees = 0.0F;
  float pitchDegrees = -25.0F;

  bool loggedSelection = false;
  bool selectionButtonDown = false;
  iggy3d::creative::CreativeInputRouterState inputRouterState;

  bool moveDragButtonDown = false;
  bool loggedMoveBefore = false;
  bool loggedMoveAfter = false;

  GizmoAxis interactiveGrabbedAxis = GizmoAxis::None;
  iggy3d::Vec3 interactiveGrabAnchorS{0.0F, 0.0F, 0.0F};
  float interactiveGrabCursorX = 0.0F;
  float interactiveGrabCursorY = 0.0F;
  ScreenPoint interactiveGrabCenterScreen;
  ScreenPoint interactiveGrabTipScreen;
  bool interactivePathMoveActive = false;
  iggy3d::creative::CreativeObjectId interactivePathMoveObjectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeToolWorldPoint interactivePathMoveStartGround{};
  bool interactivePathPointMoveActive = false;
  iggy3d::creative::CreativeObjectId interactivePathPointMoveObjectId =
      iggy3d::creative::kInvalidObjectId;
  std::size_t interactivePathPointMoveIndex = 0U;
  iggy3d::creative::CreativeToolWorldPoint
      interactivePathPointMoveStartGround{};
  bool loggedGizmoGrab = false;

  bool placeMode = false;
  std::vector<iggy3d::creative::CreativeObjectKind> brushPalette;
  iggy3d::creative::CreativeObjectKind placeBrush =
      iggy3d::creative::CreativeObjectKind::Unknown;
  double placeCellSize = 1.0;
  bool placeButtonDown = false;
  std::uint64_t placedCount = 0;

  StandaloneUndoStack undoStack;
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
