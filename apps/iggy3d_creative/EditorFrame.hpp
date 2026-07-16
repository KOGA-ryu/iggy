#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "app/platform/SdlWindow.hpp"
#include "core/math/Vec3.hpp"
#include "render/FrameInput.hpp"

#include "EditorState.hpp"
#include "EditorCapture.hpp"
#include "EditorPicking.hpp"

namespace iggy3d {

class VulkanBackend;

}  // namespace iggy3d

namespace iggy3d_creative_app {

struct CreativeEditorOverlayFrame;
class CreativeEditorGamepad;
struct StandaloneRoomBakePreviewScene;

struct CreativeEditorFrameInputResult {
  bool keepRunning = true;
  bool skipFrame = false;
  iggy3d::SdlDrawableExtent extent{};
  iggy3d::creative::CreativeInputFrame inputFrame;
  iggy3d::creative::CreativeInputRouteResult routedInput;
  iggy3d::creative::CreativeWorldActionFrame worldActions;
  iggy3d::creative::CreativeInputModifierMask modifiers =
      iggy3d::creative::kCreativeInputModifierNone;
  float toolWheelDirectionX = 0.0F;
  float toolWheelDirectionY = 0.0F;
  float navigationMoveRight = 0.0F;
  float navigationMoveForward = 0.0F;
  float navigationYawDeltaDegrees = 0.0F;
  float navigationPitchDeltaDegrees = 0.0F;
  bool navigationActive = false;
  bool navigationSprinting = false;
  std::int32_t transformNudgeWheelSteps = 0;
  bool transformFineNudge = false;
  std::uint64_t monotonicTimeNanoseconds = 0;
  bool windowFocused = true;
  iggy3d::creative::CreativeControlDevice activeControlDevice =
      iggy3d::creative::CreativeControlDevice::KeyboardMouse;
};

struct CreativeEditorNavigationAdmission {
  bool navigationActive = false;
  bool rightStickLookActive = false;
  bool clearRightStickLookRearm = false;
};

[[nodiscard]] CreativeEditorNavigationAdmission
admitCreativeEditorNavigation(
    iggy3d::creative::CreativeInputContext inputContext,
    bool transformControlsOpen,
    bool rightStickLookRearmRequired,
    iggy3d::creative::CreativeStickSignal rightStickLook,
    bool catalogToggleRouted,
    bool toolWheelToggleRouted) noexcept;

CreativeEditorFrameInputResult beginCreativeEditorFrameInput(
    iggy3d::SdlWindow& window,
    iggy3d::VulkanBackend& backend,
    CreativeEditorGamepad& gamepad,
    CreativeEditorState& editor,
    bool captureMode,
    bool applyEditorNavigation);

void applyCreativeEditorCommandInput(
    const iggy3d::creative::CreativeInputRouteResult& routedInput,
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const std::filesystem::path& saveRoot,
    const std::string& saveId);

// Resets editor transient state (volume/terrain/brush/fill caches) after the
// live document is swapped (New / Open / tab switch). Shared by the keyboard
// dispatcher and the desktop command dispatcher so both replace documents the
// same way.
void resetCreativeEditorForDocumentReplacement(
    CreativeEditorState& editor,
    iggy3d::creative::CreativeDocumentId documentId) noexcept;

[[nodiscard]] iggy3d::creative::CreativeToolWorldPoint
resolveCreativeEditorGroundPoint(const iggy3d::RenderCameraFrame& camera);

struct CreativeEditorPickFrame {
  std::vector<ObjectVisualPickBounds> objectPickCandidates;
  std::vector<PathPointHandleHit> pathPointHandleHits;
  CreativeEditorStructuralSpanEndpointHandleFrame
      structuralSpanEndpointHandles;
  bool haveFloorBounds = false;
  iggy3d::Vec3 floorBoxMin{};
  iggy3d::Vec3 floorBoxMax{};
};

struct CreativeEditorSelectionFrame {
  iggy3d::creative::Id selectedId = 0;
  const iggy3d::creative::CreativeObject* selected = nullptr;
  std::vector<iggy3d::creative::CreativeObjectId> selectedObjectIds;
  std::uint64_t selectionCount = 0;
  bool hasSelection = false;
  iggy3d::Vec3 boxMin{-0.5F, 0.0F, -0.5F};
  iggy3d::Vec3 boxMax{0.5F, 1.0F, 0.5F};
};

struct CreativeEditorSubmitFrameRequest {
  iggy3d::VulkanBackend& backend;
  iggy3d::FrameInput& frame;
  const iggy3d::creative::CreativeAppState& appState;
  CreativeEditorState& editor;
  const CreativeEditorSelectionFrame& selection;
  const CreativeEditorOverlayFrame& overlayFrame;
  const StandaloneRoomBakePreviewScene& roomBakePreview;
  std::uint64_t maxFrames = 0;
};

[[nodiscard]] bool submitCreativeEditorFrame(
    const CreativeEditorSubmitFrameRequest& request);

[[nodiscard]] CreativeEditorSelectionFrame resolveCreativeEditorSelectionFrame(
    const iggy3d::creative::Facade& facade);

[[nodiscard]] CreativeEditorPickFrame buildCreativeEditorPickFrame(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    iggy3d::creative::CreativeObjectId pathHandleObjectId,
    iggy3d::creative::CreativeObjectId structuralSpanHandleObjectId,
    iggy3d::creative::CreativeObjectId floorObjectId,
    StandaloneCaptureScript& captureScript,
    bool captureMode);

void logCreativeEditorWorldPickProofFrame(
    const iggy3d::creative::Facade& facade,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    const CreativeEditorPickFrame& pickFrame,
    iggy3d::creative::CreativeObjectId floorObjectId,
    CreativeEditorState& editor,
    bool captureMode);

}  // namespace iggy3d_creative_app
