#include "EditorFrame.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <string>
#include <thread>

#include "app/iggy3d/creative/camera/Fly.hpp"
#include "app/iggy3d/creative/input/UiInput.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "render/vulkan/VulkanBackend.hpp"

#include "EditorPlacement.hpp"
#include "EditorEdits.hpp"
#include "EditorGamepad.hpp"
#include "EditorInteraction.hpp"
#include "EditorPersistence.hpp"
#include "EditorPicking.hpp"
#include "EditorFrustumCull.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorTransform.hpp"
#include "EditorVolume.hpp"

namespace iggy3d_creative_app {

[[nodiscard]] iggy3d::creative::CreativeToolWorldPoint
resolveCreativeEditorGroundPoint(const iggy3d::RenderCameraFrame& camera) {
  const iggy3d::Vec3 eye = camera.worldEye;
  const iggy3d::Vec3 fwd = camera.worldForward;
  iggy3d::creative::CreativeToolWorldPoint ground{eye.x, 0.0, eye.z};
  if (std::fabs(fwd.y) > 1.0e-4F) {
    const float t = -eye.y / fwd.y;  // eye.y + t*fwd.y == 0
    if (t > 0.0F) {
      ground.x = static_cast<double>(eye.x + fwd.x * t);
      ground.z = static_cast<double>(eye.z + fwd.z * t);
    }
  }
  return ground;
}

namespace creative = iggy3d::creative;
namespace {

static_assert(
    static_cast<std::size_t>(creative::CreativeInputActionId::HotbarSlot9) -
            static_cast<std::size_t>(
                creative::CreativeInputActionId::HotbarSlot1) +
        1U ==
    creative::kCreativeHotbarSlotCount);

void setSdlKey(creative::CreativeInputFrame& frame,
               creative::CreativeInputKey key,
               const bool* keys,
               SDL_Scancode scancode) {
  creative::setCreativeInputKey(frame, key,
                                keys != nullptr && keys[scancode] != 0);
}

[[nodiscard]] creative::CreativeInputFrame makeCreativeInputFrame(
    const bool* keys,
    SDL_Keymod modifiers,
    creative::CreativeInputContext context,
    const creative::CreativeControllerFrame& controller) {
  creative::CreativeInputFrame frame;
  frame.context = context;
  if ((modifiers & SDL_KMOD_SHIFT) != 0U) {
    frame.modifiers |= creative::kCreativeInputModifierShift;
  }
  if ((modifiers & SDL_KMOD_CTRL) != 0U) {
    frame.modifiers |= creative::kCreativeInputModifierControl;
  }
  if ((modifiers & SDL_KMOD_ALT) != 0U) {
    frame.modifiers |= creative::kCreativeInputModifierAlt;
  }
  if ((modifiers & SDL_KMOD_GUI) != 0U) {
    frame.modifiers |= creative::kCreativeInputModifierCommand;
  }

  setSdlKey(frame, creative::CreativeInputKey::Digit1, keys, SDL_SCANCODE_1);
  setSdlKey(frame, creative::CreativeInputKey::Digit2, keys, SDL_SCANCODE_2);
  setSdlKey(frame, creative::CreativeInputKey::Digit3, keys, SDL_SCANCODE_3);
  setSdlKey(frame, creative::CreativeInputKey::Digit4, keys, SDL_SCANCODE_4);
  setSdlKey(frame, creative::CreativeInputKey::Digit5, keys, SDL_SCANCODE_5);
  setSdlKey(frame, creative::CreativeInputKey::Digit6, keys, SDL_SCANCODE_6);
  setSdlKey(frame, creative::CreativeInputKey::Digit7, keys, SDL_SCANCODE_7);
  setSdlKey(frame, creative::CreativeInputKey::Digit8, keys, SDL_SCANCODE_8);
  setSdlKey(frame, creative::CreativeInputKey::Digit9, keys, SDL_SCANCODE_9);
  setSdlKey(frame, creative::CreativeInputKey::C, keys, SDL_SCANCODE_C);
  setSdlKey(frame, creative::CreativeInputKey::D, keys, SDL_SCANCODE_D);
  setSdlKey(frame, creative::CreativeInputKey::E, keys, SDL_SCANCODE_E);
  setSdlKey(frame, creative::CreativeInputKey::N, keys, SDL_SCANCODE_N);
  setSdlKey(frame, creative::CreativeInputKey::O, keys, SDL_SCANCODE_O);
  setSdlKey(frame, creative::CreativeInputKey::R, keys, SDL_SCANCODE_R);
  setSdlKey(frame, creative::CreativeInputKey::S, keys, SDL_SCANCODE_S);
  setSdlKey(frame, creative::CreativeInputKey::V, keys, SDL_SCANCODE_V);
  setSdlKey(frame, creative::CreativeInputKey::X, keys, SDL_SCANCODE_X);
  setSdlKey(frame, creative::CreativeInputKey::Y, keys, SDL_SCANCODE_Y);
  setSdlKey(frame, creative::CreativeInputKey::Z, keys, SDL_SCANCODE_Z);
  setSdlKey(frame, creative::CreativeInputKey::LeftBracket, keys,
            SDL_SCANCODE_LEFTBRACKET);
  setSdlKey(frame, creative::CreativeInputKey::RightBracket, keys,
            SDL_SCANCODE_RIGHTBRACKET);
  setSdlKey(frame, creative::CreativeInputKey::Minus, keys,
            SDL_SCANCODE_MINUS);
  setSdlKey(frame, creative::CreativeInputKey::Equals, keys,
            SDL_SCANCODE_EQUALS);
  setSdlKey(frame, creative::CreativeInputKey::Delete, keys,
            SDL_SCANCODE_DELETE);
  setSdlKey(frame, creative::CreativeInputKey::Backspace, keys,
            SDL_SCANCODE_BACKSPACE);
  setSdlKey(frame, creative::CreativeInputKey::Enter, keys,
            SDL_SCANCODE_RETURN);
  setSdlKey(frame, creative::CreativeInputKey::Escape, keys,
            SDL_SCANCODE_ESCAPE);
  setSdlKey(frame, creative::CreativeInputKey::ArrowUp, keys,
            SDL_SCANCODE_UP);
  setSdlKey(frame, creative::CreativeInputKey::ArrowDown, keys,
            SDL_SCANCODE_DOWN);
  setSdlKey(frame, creative::CreativeInputKey::ArrowLeft, keys,
            SDL_SCANCODE_LEFT);
  setSdlKey(frame, creative::CreativeInputKey::ArrowRight, keys,
            SDL_SCANCODE_RIGHT);
  setSdlKey(frame, creative::CreativeInputKey::W, keys, SDL_SCANCODE_W);
  setSdlKey(frame, creative::CreativeInputKey::A, keys, SDL_SCANCODE_A);
  setSdlKey(frame, creative::CreativeInputKey::Space, keys,
            SDL_SCANCODE_SPACE);
  setSdlKey(frame, creative::CreativeInputKey::LeftControl, keys,
            SDL_SCANCODE_LCTRL);
  setSdlKey(frame, creative::CreativeInputKey::RightControl, keys,
            SDL_SCANCODE_RCTRL);
  setSdlKey(frame, creative::CreativeInputKey::LeftShift, keys,
            SDL_SCANCODE_LSHIFT);
  setSdlKey(frame, creative::CreativeInputKey::RightShift, keys,
            SDL_SCANCODE_RSHIFT);
  setSdlKey(frame, creative::CreativeInputKey::LeftAlt, keys,
            SDL_SCANCODE_LALT);
  setSdlKey(frame, creative::CreativeInputKey::RightAlt, keys,
            SDL_SCANCODE_RALT);
  setSdlKey(frame, creative::CreativeInputKey::LeftCommand, keys,
            SDL_SCANCODE_LGUI);
  setSdlKey(frame, creative::CreativeInputKey::RightCommand, keys,
            SDL_SCANCODE_RGUI);
  const SDL_MouseButtonFlags mouseButtons =
      SDL_GetMouseState(nullptr, nullptr);
  creative::setCreativeInputKey(
      frame, creative::CreativeInputKey::MousePrimary,
      (mouseButtons & SDL_BUTTON_LMASK) != 0U);
  creative::setCreativeInputKey(
      frame, creative::CreativeInputKey::MouseSecondary,
      (mouseButtons & SDL_BUTTON_RMASK) != 0U);
  creative::setCreativeInputKey(
      frame, creative::CreativeInputKey::MouseMiddle,
      (mouseButtons & SDL_BUTTON_MMASK) != 0U);
  creative::setCreativeInputKey(frame,
                                creative::CreativeInputKey::GamepadInventory,
                                creative::creativeControllerButtonDown(
                                    controller,
                                    creative::CreativeControllerButton::North));
  creative::setCreativeInputKey(frame,
                                creative::CreativeInputKey::GamepadDpadUp,
                                creative::creativeControllerButtonDown(
                                    controller,
                                    creative::CreativeControllerButton::DpadUp));
  creative::setCreativeInputKey(frame,
                                creative::CreativeInputKey::GamepadDpadDown,
                                creative::creativeControllerButtonDown(
                                    controller,
                                    creative::CreativeControllerButton::DpadDown));
  creative::setCreativeInputKey(frame,
                                creative::CreativeInputKey::GamepadConfirm,
                                creative::creativeControllerButtonDown(
                                    controller,
                                    creative::CreativeControllerButton::South));
  creative::setCreativeInputKey(frame,
                                creative::CreativeInputKey::GamepadCancel,
                                creative::creativeControllerButtonDown(
                                    controller,
                                    creative::CreativeControllerButton::East));
  creative::setCreativeInputKey(frame,
                                creative::CreativeInputKey::GamepadDpadLeft,
                                creative::creativeControllerButtonDown(
                                    controller,
                                    creative::CreativeControllerButton::DpadLeft));
  creative::setCreativeInputKey(frame,
                                creative::CreativeInputKey::GamepadDpadRight,
                                creative::creativeControllerButtonDown(
                                    controller,
                                    creative::CreativeControllerButton::DpadRight));
  creative::setCreativeInputKey(
      frame, creative::CreativeInputKey::GamepadLeftShoulder,
      creative::creativeControllerButtonDown(
          controller, creative::CreativeControllerButton::LeftShoulder));
  creative::setCreativeInputKey(
      frame, creative::CreativeInputKey::GamepadRightShoulder,
      creative::creativeControllerButtonDown(
          controller, creative::CreativeControllerButton::RightShoulder));
  creative::setCreativeInputKey(
      frame, creative::CreativeInputKey::GamepadWest,
      creative::creativeControllerButtonDown(
          controller, creative::CreativeControllerButton::West));
  creative::setCreativeInputKey(
      frame, creative::CreativeInputKey::GamepadBack,
      creative::creativeControllerButtonDown(
          controller, creative::CreativeControllerButton::Back));
  creative::setCreativeInputKey(
      frame, creative::CreativeInputKey::GamepadStart,
      creative::creativeControllerButtonDown(
          controller, creative::CreativeControllerButton::Start));
  creative::setCreativeInputKey(
      frame, creative::CreativeInputKey::GamepadLeftStick,
      creative::creativeControllerButtonDown(
          controller, creative::CreativeControllerButton::LeftStick));
  creative::setCreativeInputKey(
      frame, creative::CreativeInputKey::GamepadRightStick,
      creative::creativeControllerButtonDown(
          controller, creative::CreativeControllerButton::RightStick));
  creative::setCreativeInputKey(
      frame, creative::CreativeInputKey::GamepadLeftTrigger,
      creative::creativeControllerButtonDown(
          controller, creative::CreativeControllerButton::LeftTrigger));
  creative::setCreativeInputKey(
      frame, creative::CreativeInputKey::GamepadRightTrigger,
      creative::creativeControllerButtonDown(
          controller, creative::CreativeControllerButton::RightTrigger));
  return frame;
}

[[nodiscard]] bool routedActionPresent(
    const creative::CreativeInputRouteResult& routedInput,
    creative::CreativeInputActionId action) noexcept {
  return std::any_of(
      routedInput.actionEvents().begin(), routedInput.actionEvents().end(),
      [action](const creative::CreativeInputActionEvent& event) {
        return event.action == action;
      });
}

[[nodiscard]] creative::CreativeInputContext resolveInputContext(
    bool captureMode,
    const CreativeEditorState& editor) noexcept {
  struct Candidate {
    bool active = false;
    creative::CreativeInputContext context =
        creative::CreativeInputContext::EditorViewport;
  };
  const std::array candidates{
      Candidate{captureMode, creative::CreativeInputContext::Capture},
      Candidate{editor.controls.open,
                creative::CreativeInputContext::Controls},
      Candidate{editor.transform.active,
                editor.transform.controlsOpen
                    ? creative::CreativeInputContext::TransformControls
                    : creative::CreativeInputContext::TransformPreview},
      Candidate{editor.catalog.model.open,
                creative::CreativeInputContext::Catalog},
      Candidate{editor.catalog.toolWheel.open,
                creative::CreativeInputContext::ToolWheel},
      Candidate{editor.toolOptions.open,
                creative::CreativeInputContext::ToolOptions},
  };
  const auto active = std::find_if(
      candidates.begin(), candidates.end(),
      [](const Candidate& candidate) { return candidate.active; });
  return active == candidates.end()
             ? creative::CreativeInputContext::EditorViewport
             : active->context;
}

}  // namespace

void applyCreativeEditorCommandInput(
    const creative::CreativeInputRouteResult& routedInput,
    creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const std::filesystem::path& saveRoot,
    const std::string& saveId) {
  if (!routedInput.actionEvents().empty()) {
    finalizeCreativeMaterialStroke(appState, editor,
                                   "creative_material_stroke_command");
  }
  if (routedInput.context ==
          creative::CreativeInputContext::TransformPreview ||
      routedInput.context ==
          creative::CreativeInputContext::TransformControls) {
    return;
  }
  if (routedInput.context != creative::CreativeInputContext::EditorViewport) {
    return;
  }
  for (const creative::CreativeInputActionEvent& event :
       routedInput.actionEvents()) {
    switch (event.action) {
      case creative::CreativeInputActionId::HotbarSlot1:
      case creative::CreativeInputActionId::HotbarSlot2:
      case creative::CreativeInputActionId::HotbarSlot3:
      case creative::CreativeInputActionId::HotbarSlot4:
      case creative::CreativeInputActionId::HotbarSlot5:
      case creative::CreativeInputActionId::HotbarSlot6:
      case creative::CreativeInputActionId::HotbarSlot7:
      case creative::CreativeInputActionId::HotbarSlot8:
      case creative::CreativeInputActionId::HotbarSlot9: {
        const std::size_t slot =
            static_cast<std::size_t>(event.action) -
            static_cast<std::size_t>(
                creative::CreativeInputActionId::HotbarSlot1);
        static_cast<void>(
            selectCreativeEditorHotbarSlot(appState, editor, slot));
        break;
      }
      case creative::CreativeInputActionId::ToggleCatalog:
      case creative::CreativeInputActionId::CatalogPrevious:
      case creative::CreativeInputActionId::CatalogNext:
      case creative::CreativeInputActionId::CatalogPreviousVariant:
      case creative::CreativeInputActionId::CatalogNextVariant:
      case creative::CreativeInputActionId::CatalogPreviousPage:
      case creative::CreativeInputActionId::CatalogNextPage:
      case creative::CreativeInputActionId::CatalogConfirm:
      case creative::CreativeInputActionId::CatalogClose:
      case creative::CreativeInputActionId::ToggleToolWheel:
      case creative::CreativeInputActionId::ToolWheelPrevious:
      case creative::CreativeInputActionId::ToolWheelNext:
      case creative::CreativeInputActionId::ToolWheelConfirm:
      case creative::CreativeInputActionId::ToolWheelOptions:
      case creative::CreativeInputActionId::ToolWheelClose:
      case creative::CreativeInputActionId::ToolOptionsPrevious:
      case creative::CreativeInputActionId::ToolOptionsNext:
      case creative::CreativeInputActionId::ToolOptionsDecrease:
      case creative::CreativeInputActionId::ToolOptionsIncrease:
      case creative::CreativeInputActionId::ToolOptionsConfirm:
      case creative::CreativeInputActionId::ToolOptionsClose:
      case creative::CreativeInputActionId::ToggleTransformControls:
      case creative::CreativeInputActionId::TransformControlPrevious:
      case creative::CreativeInputActionId::TransformControlNext:
      case creative::CreativeInputActionId::TransformConstraintX:
      case creative::CreativeInputActionId::TransformConstraintY:
      case creative::CreativeInputActionId::TransformConstraintZ:
      case creative::CreativeInputActionId::TransformNudgeNegative:
      case creative::CreativeInputActionId::TransformNudgePositive:
      case creative::CreativeInputActionId::MoveForward:
      case creative::CreativeInputActionId::MoveBackward:
      case creative::CreativeInputActionId::MoveLeft:
      case creative::CreativeInputActionId::MoveRight:
      case creative::CreativeInputActionId::FlyUp:
      case creative::CreativeInputActionId::FlyDown:
      case creative::CreativeInputActionId::Sprint:
      case creative::CreativeInputActionId::PrimaryAction:
      case creative::CreativeInputActionId::SecondaryAction:
      case creative::CreativeInputActionId::PickAction:
      case creative::CreativeInputActionId::HotbarPrevious:
      case creative::CreativeInputActionId::HotbarNext:
      case creative::CreativeInputActionId::ToggleControls:
      case creative::CreativeInputActionId::ControlsPrevious:
      case creative::CreativeInputActionId::ControlsNext:
      case creative::CreativeInputActionId::ControlsDecrease:
      case creative::CreativeInputActionId::ControlsIncrease:
      case creative::CreativeInputActionId::ControlsActivate:
      case creative::CreativeInputActionId::ControlsClose:
      case creative::CreativeInputActionId::ControlsResetDefaults:
      case creative::CreativeInputActionId::Count:
        break;
      case creative::CreativeInputActionId::ConfirmActiveTool:
        static_cast<void>(confirmCreativeEditorHeldItem(
            appState, editor, "keyboard_confirm"));
        break;
      case creative::CreativeInputActionId::CancelActiveTool:
        static_cast<void>(cancelCreativeEditorHeldItem(appState, editor));
        break;
      case creative::CreativeInputActionId::DeleteSelection:
        if (editor.volume.active) {
          static_cast<void>(applyCreativeEditorVolumeOperationWithHistory(
              appState, editor.volume, editor.placeBrush,
              creative::CreativeVolumeOperationKind::Erase,
              editor.toolSettings,
              event.trigger == creative::CreativeInputKey::Backspace
                  ? "volume_backspace_erase"
                  : "volume_delete_erase"));
        } else {
          (void)deleteSelectedObject(
              appState,
              event.trigger == creative::CreativeInputKey::Backspace
                  ? "backspace_key"
                  : "delete_key",
              &appState.history);
        }
        break;
      case creative::CreativeInputActionId::Undo:
        (void)undoLastEdit(appState, "keyboard_undo");
        break;
      case creative::CreativeInputActionId::Redo:
        (void)redoLastEdit(appState, "keyboard_redo");
        break;
      case creative::CreativeInputActionId::CopySelection:
        (void)copySelectionToClipboard(appState, "keyboard_copy");
        break;
      case creative::CreativeInputActionId::CutSelection:
        (void)cutSelectionToClipboardWithHistory(appState, "keyboard_cut");
        break;
      case creative::CreativeInputActionId::PasteClipboard:
        static_cast<void>(beginCreativeEditorClipboardTransformPreview(
            appState, appState.clipboard, editor.transform, "keyboard_paste"));
        break;
      case creative::CreativeInputActionId::DuplicateSelection:
        (void)duplicateSelectedObjectsWithUndo(
            appState, appState.history,
            creative::CreativeDuplicateCommandRequest{}, "keyboard_duplicate");
        break;
      case creative::CreativeInputActionId::RotateYawNegative:
      case creative::CreativeInputActionId::RotateYawPositive: {
        creative::CreativeTransformCommandRequest request;
        request.kind = creative::CreativeTransformCommandKind::RotateYaw;
        const double rotationStep =
            creative::creativeRotationStepDegrees(
                editor.toolSettings.rotationStep);
        request.yawDegrees =
            event.action == creative::CreativeInputActionId::RotateYawNegative
                ? -rotationStep
                : rotationStep;
        (void)transformSelectedObjectsWithUndo(
            appState, appState.history, request,
            request.yawDegrees < 0.0 ? "keyboard_rotate_yaw_negative"
                                     : "keyboard_rotate_yaw_positive");
        break;
      }
      case creative::CreativeInputActionId::ScaleDown:
      case creative::CreativeInputActionId::ScaleUp: {
        creative::CreativeTransformCommandRequest request;
        request.kind = creative::CreativeTransformCommandKind::Scale;
        const double factor =
            event.action == creative::CreativeInputActionId::ScaleDown ? 0.9
                                                                        : 1.1;
        request.scaleFactor = {factor, factor, factor};
        (void)transformSelectedObjectsWithUndo(
            appState, appState.history, request,
            factor < 1.0 ? "keyboard_scale_down" : "keyboard_scale_up");
        break;
      }
      case creative::CreativeInputActionId::Save: {
        const iggy3d::CreativeWorldSaveResult saveResult =
            saveStandaloneScene(appState.facade, saveRoot, saveId);
        if (saveResult.accepted && saveResult.saved) {
          clearEditHistory(appState.history, "save_success");
        }
        break;
      }
      case creative::CreativeInputActionId::NewDocument:
        clearToBlankScene(appState);
        clearEditHistory(appState.history, "new_clear");
        creative::clearCreativeVolumeSelection(editor.volume.selection);
        break;
      case creative::CreativeInputActionId::Load: {
        const bool loaded = loadStandaloneScene(appState, saveRoot, saveId);
        if (loaded) {
          clearEditHistory(appState.history, "load_success");
          creative::clearCreativeVolumeSelection(editor.volume.selection);
        }
        break;
      }
    }
  }
}

namespace {

constexpr creative::CreativeStickProfile kRadialStickProfile{
    0.0F, 1.0F, false, false};
constexpr creative::CreativeWheelProfile kFrameWheelProfile{};

}  // namespace

CreativeEditorFrameInputResult beginCreativeEditorFrameInput(
    iggy3d::SdlWindow& window,
    iggy3d::VulkanBackend& backend,
    CreativeEditorGamepad& gamepad,
    CreativeEditorState& editor,
    bool captureMode) {
  CreativeEditorFrameInputResult result;

  window.pollEvents();
  result.monotonicTimeNanoseconds = SDL_GetTicksNS();
  result.windowFocused = window.eventState().focused;
  if (window.eventState().quitRequested) {
    result.keepRunning = false;
    return result;
  }
  if (!window.isDrawable()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    result.skipFrame = true;
    return result;
  }

  const iggy3d::SdlDrawableExtent extent = window.drawableExtent();
  if (extent.width == 0U || extent.height == 0U) {
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    result.skipFrame = true;
    return result;
  }
  result.extent = extent;

  if (extent.width != editor.lastWidth || extent.height != editor.lastHeight) {
    iggy3d::RenderViewport viewport;
    viewport.width = extent.width;
    viewport.height = extent.height;
    viewport.aspectRatio =
        static_cast<float>(extent.width) / static_cast<float>(extent.height);
    backend.resize(viewport);
    editor.lastWidth = extent.width;
    editor.lastHeight = extent.height;
  }

  const bool* keys = SDL_GetKeyboardState(nullptr);
  const creative::CreativeControllerSample controllerSample = gamepad.sample();
  const creative::CreativeControllerFrame controller =
      creative::stepCreativeControllerInput(editor.controllerInputState,
                                            controllerSample);
  editor.controllerInputState = controller.next;
  const creative::CreativeInputContext inputContext =
      resolveInputContext(captureMode, editor);
  const creative::CreativeInputFrame inputFrame = makeCreativeInputFrame(
      keys, SDL_GetModState(), inputContext, controller);
  result.inputFrame = inputFrame;
  result.routedInput =
      creative::routeCreativeInput(editor.inputRouterState, inputFrame,
                                   editor.controlProfile.bindingSpan());
  const auto actionDown = [&inputFrame, &result, &editor](
                              creative::CreativeInputActionId action) {
    return creative::creativeInputActionDown(
        inputFrame, action, editor.controlProfile.bindingSpan(),
        &result.routedInput);
  };
  const creative::CreativeStickSignal moveStick =
      creative::creativeControllerStick(
          controller, creative::CreativeControllerStick::Left,
          editor.controlProfile.movementStick);
  const creative::CreativeStickSignal lookStick =
      creative::creativeControllerStick(
          controller, creative::CreativeControllerStick::Right,
          editor.controlProfile.lookStick);
  const creative::CreativeStickSignal radialStick =
      creative::creativeControllerStick(
          controller, creative::CreativeControllerStick::Right,
          kRadialStickProfile);
  const creative::CreativeControllerLookDelta gamepadLook =
      creative::creativeControllerLookDelta(lookStick,
                                            editor.controlProfile
                                                .gamepadLookSensitivity);
  result.modifiers = inputFrame.modifiers;
  result.transformNudgeWheelSteps =
      inputContext == creative::CreativeInputContext::TransformPreview
          ? creative::quantizeCreativeWheelSteps(
                window.eventState().mouseWheelY, kFrameWheelProfile)
          : 0;
  result.transformFineNudge =
      (inputFrame.modifiers & creative::kCreativeInputModifierShift) != 0U ||
      creative::creativeControllerButtonDown(
          controller, creative::CreativeControllerButton::LeftShoulder);
  result.toolWheelDirectionX = radialStick.x;
  result.toolWheelDirectionY = radialStick.y;
  creative::CreativeWorldInputSample worldInput;
  if (!captureMode && result.windowFocused) {
    creative::setCreativeWorldAction(
        worldInput, creative::CreativeWorldActionId::Primary,
        actionDown(creative::CreativeInputActionId::PrimaryAction));
    creative::setCreativeWorldAction(
        worldInput, creative::CreativeWorldActionId::Secondary,
        actionDown(creative::CreativeInputActionId::SecondaryAction));
    creative::setCreativeWorldAction(
        worldInput, creative::CreativeWorldActionId::Pick,
        actionDown(creative::CreativeInputActionId::PickAction));
    creative::setCreativeWorldAction(
        worldInput, creative::CreativeWorldActionId::HotbarPrevious,
        actionDown(creative::CreativeInputActionId::HotbarPrevious));
    creative::setCreativeWorldAction(
        worldInput, creative::CreativeWorldActionId::HotbarNext,
        actionDown(creative::CreativeInputActionId::HotbarNext));
    worldInput.hotbarWheelSteps = creative::quantizeCreativeWheelSteps(
        window.eventState().mouseWheelY, kFrameWheelProfile);
  }
  result.worldActions = creative::routeCreativeWorldActions(
      editor.interaction.actionRouter, worldInput);
  iggy3d::ProductCreativeFlyInput flyInput;
  const bool viewportNavigationContext =
      inputContext == creative::CreativeInputContext::EditorViewport ||
      inputContext == creative::CreativeInputContext::TransformPreview;
  const bool viewportNavigationActive =
      viewportNavigationContext && !editor.transform.controlsOpen &&
      !routedActionPresent(result.routedInput,
                           creative::CreativeInputActionId::ToggleCatalog) &&
      !routedActionPresent(result.routedInput,
                           creative::CreativeInputActionId::ToggleToolWheel);
  if (viewportNavigationActive) {
    const bool precisionNudgeRequested =
        inputContext == creative::CreativeInputContext::TransformPreview &&
        (result.transformNudgeWheelSteps != 0 ||
         routedActionPresent(
             result.routedInput,
             creative::CreativeInputActionId::TransformNudgeNegative) ||
         routedActionPresent(
             result.routedInput,
             creative::CreativeInputActionId::TransformNudgePositive));
    const float keyboardMoveX =
        (actionDown(creative::CreativeInputActionId::MoveRight)
             ? 1.0F
             : 0.0F) -
        (actionDown(creative::CreativeInputActionId::MoveLeft)
             ? 1.0F
             : 0.0F);
    const float keyboardMoveY =
        (actionDown(creative::CreativeInputActionId::MoveForward)
             ? 1.0F
             : 0.0F) -
        (actionDown(creative::CreativeInputActionId::MoveBackward)
             ? 1.0F
             : 0.0F);
    const bool ascend = actionDown(creative::CreativeInputActionId::FlyUp);
    const bool descendRequested =
        actionDown(creative::CreativeInputActionId::FlyDown);
    const bool descend =
        descendRequested &&
        !(precisionNudgeRequested && result.transformFineNudge);
    const float moveZ =
        (ascend ? 1.0F : 0.0F) - (descend ? 1.0F : 0.0F);
    flyInput.moveX =
        std::clamp(keyboardMoveX + moveStick.x, -1.0F, 1.0F);
    flyInput.moveY =
        std::clamp(keyboardMoveY + moveStick.y, -1.0F, 1.0F);
    flyInput.moveZ = moveZ;
    flyInput.sprinting = actionDown(creative::CreativeInputActionId::Sprint);
  }

  float mouseDx = 0.0F;
  float mouseDy = 0.0F;
  SDL_GetRelativeMouseState(&mouseDx, &mouseDy);
  if (viewportNavigationActive) {
    editor.yawDegrees += mouseDx * editor.controlProfile.mouseLookSensitivity +
                         gamepadLook.yawDegrees;
    editor.pitchDegrees = std::clamp(
        editor.pitchDegrees -
            mouseDy * editor.controlProfile.mouseLookSensitivity +
            gamepadLook.pitchDegrees,
        -80.0F, 80.0F);
  }
  flyInput.cameraYawDegrees = editor.yawDegrees;
  flyInput.cameraPitchDegrees = editor.pitchDegrees;

  const iggy3d::ProductCreativeFlyResult flyResult =
      applyProductCreativeFlyInput(editor.flyConfig, flyInput, editor.flyPos);
  if (flyResult.applied) {
    editor.flyPos = flyResult.finalPositionMeters;
  }

  return result;
}

CreativeEditorPickFrame buildCreativeEditorPickFrame(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    iggy3d::creative::CreativeObjectId floorObjectId,
    StandaloneCaptureScript& captureScript,
    bool captureMode) {
  CreativeEditorPickFrame frame;
  for (const iggy3d::creative::CreativeObject& obj : document.objects()) {
    if (!obj.visible) {
      continue;
    }
    const ObjectVisualPickBounds hit = buildObjectVisualPickBounds(
        obj, camera.clipFromWorld, drawableWidth, drawableHeight);
    const iggy3d::Vec3 boxMin = hit.bounds.min;
    const iggy3d::Vec3 boxMax = hit.bounds.max;
    frame.objectPickCandidates.push_back(hit);
    if (obj.id == floorObjectId) {
      frame.haveFloorBounds = true;
      frame.floorBoxMin = boxMin;
      frame.floorBoxMax = boxMax;
    }
    if (captureMode && !captureScript.pointHitProxyLogged &&
        obj.id == captureScript.pointTargetId) {
      SDL_Log("iggy3d_creative: POINT hit proxy objectId=%llu "
              "aabbValid=%d marker=[(%.3f, %.3f, %.3f).."
              "(%.3f, %.3f, %.3f)] screen=[%.1f, %.1f..%.1f, %.1f]",
              static_cast<unsigned long long>(captureScript.pointTargetId),
              hit.screenAabb.valid ? 1 : 0, boxMin.x, boxMin.y, boxMin.z,
              boxMax.x, boxMax.y, boxMax.z, hit.screenAabb.minX,
              hit.screenAabb.minY, hit.screenAabb.maxX, hit.screenAabb.maxY);
      captureScript.pointHitProxyLogged = true;
    }
    if (captureMode && !captureScript.lineHitProxyLogged &&
        obj.id == captureScript.lineTargetId) {
      SDL_Log("iggy3d_creative: LINE hit proxy objectId=%llu "
              "aabbValid=%d visual=[(%.3f, %.3f, %.3f).."
              "(%.3f, %.3f, %.3f)] screen=[%.1f, %.1f..%.1f, %.1f]",
              static_cast<unsigned long long>(captureScript.lineTargetId),
              hit.screenAabb.valid ? 1 : 0, boxMin.x, boxMin.y, boxMin.z,
              boxMax.x, boxMax.y, boxMax.z, hit.screenAabb.minX,
              hit.screenAabb.minY, hit.screenAabb.maxX, hit.screenAabb.maxY);
      captureScript.lineHitProxyLogged = true;
    }
    if (captureMode && !captureScript.pathHitProxyLogged &&
        obj.id == captureScript.pathTargetId) {
      SDL_Log("iggy3d_creative: PATH hit proxy objectId=%llu "
              "aabbValid=%d visual=[(%.3f, %.3f, %.3f).."
              "(%.3f, %.3f, %.3f)] screen=[%.1f, %.1f..%.1f, %.1f] "
              "pathPointCount=%zu pathPoints='%s'",
              static_cast<unsigned long long>(captureScript.pathTargetId),
              hit.screenAabb.valid ? 1 : 0, boxMin.x, boxMin.y, boxMin.z,
              boxMax.x, boxMax.y, boxMax.z, hit.screenAabb.minX,
              hit.screenAabb.minY, hit.screenAabb.maxX, hit.screenAabb.maxY,
              obj.pathPoints.size(), pathPointsSummary(obj.pathPoints).c_str());
      captureScript.pathHitProxyLogged = true;
    }
  }
  return frame;
}

CreativeEditorSelectionFrame resolveCreativeEditorSelectionFrame(
    const iggy3d::creative::Facade& facade) {
  CreativeEditorSelectionFrame selection;
  const iggy3d::creative::CreativeSelectionState& selectionState =
      facade.selectionState();
  selection.selectedId = selectionState.selectedTarget.value;
  selection.selected =
      selection.selectedId != 0
          ? facade.findObject(static_cast<iggy3d::creative::CreativeObjectId>(
                selection.selectedId))
          : nullptr;
  const std::span<const iggy3d::creative::TargetRef> targets =
      iggy3d::creative::selectedTargetList(selectionState);
  if (targets.empty() && selection.selected != nullptr) {
    selection.selectedObjectIds.push_back(selection.selected->id);
  } else {
    selection.selectedObjectIds.reserve(targets.size());
    for (iggy3d::creative::TargetRef target : targets) {
      if (target.value != iggy3d::creative::kInvalidId) {
        selection.selectedObjectIds.push_back(
            static_cast<iggy3d::creative::CreativeObjectId>(target.value));
      }
    }
  }
  selection.selectionCount = selection.selectedObjectIds.size();

  bool haveBounds = false;
  for (iggy3d::creative::CreativeObjectId objectId :
       selection.selectedObjectIds) {
    const iggy3d::creative::CreativeObject* object = facade.findObject(objectId);
    if (object == nullptr || !object->visible) {
      continue;
    }
    const VisualBounds bounds = visualBoundsForObject(*object);
    if (!haveBounds) {
      selection.boxMin = bounds.min;
      selection.boxMax = bounds.max;
      haveBounds = true;
      continue;
    }
    selection.boxMin.x = std::min(selection.boxMin.x, bounds.min.x);
    selection.boxMin.y = std::min(selection.boxMin.y, bounds.min.y);
    selection.boxMin.z = std::min(selection.boxMin.z, bounds.min.z);
    selection.boxMax.x = std::max(selection.boxMax.x, bounds.max.x);
    selection.boxMax.y = std::max(selection.boxMax.y, bounds.max.y);
    selection.boxMax.z = std::max(selection.boxMax.z, bounds.max.z);
  }
  selection.hasSelection = haveBounds;
  return selection;
}

void logCreativeEditorWorldPickProofFrame(
    const iggy3d::creative::Facade& facade,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    const CreativeEditorPickFrame& pickFrame,
    iggy3d::creative::CreativeObjectId floorObjectId,
    CreativeEditorState& editor,
    bool captureMode) {
  if (!captureMode) {
    return;
  }

  auto logWorldPickProof =
      [&](const char* label, iggy3d::creative::CreativeObjectId expectedId,
          iggy3d::Vec3 worldPoint, bool& logged) {
        if (logged || expectedId == iggy3d::creative::kInvalidObjectId) {
          return;
        }
        const creative::CreativeScreenPoint screenPoint =
            creative::projectCreativeWorldPointToScreen(
                camera.clipFromWorld, worldPoint, drawableWidth,
                drawableHeight);
        if (!screenPoint.valid) {
          return;
        }
        const WorldRay ray =
            worldRayFromPixel(camera, screenPoint.x, screenPoint.y,
                              drawableWidth, drawableHeight);
        const ObjectVisualPickResult pick = pickNearestVisualBoundsObject(
            pickFrame.objectPickCandidates, ray);
        SDL_Log("iggy3d_creative: WORLD_PICK_PROOF label='%s' "
                "click=(%.1f, %.1f) rayValid=%d expectedObjectId=%llu "
                "pickedObjectId=%llu matched=%d entryDistance=%.3f "
                "tested=%llu hits=%llu",
                label, screenPoint.x, screenPoint.y, pick.rayValid ? 1 : 0,
                static_cast<unsigned long long>(expectedId),
                static_cast<unsigned long long>(pick.objectId),
                pick.objectId == expectedId ? 1 : 0, pick.entryDistance,
                static_cast<unsigned long long>(pick.testedCount),
                static_cast<unsigned long long>(pick.hitCount));
        logged = true;
      };

  if (!editor.captureWorldPickFloorLogged && pickFrame.haveFloorBounds) {
    const iggy3d::Vec3 floorTopCorner{
        pickFrame.floorBoxMin.x +
            (pickFrame.floorBoxMax.x - pickFrame.floorBoxMin.x) * 0.85F,
        pickFrame.floorBoxMax.y,
        pickFrame.floorBoxMin.z +
            (pickFrame.floorBoxMax.z - pickFrame.floorBoxMin.z) * 0.85F};
    logWorldPickProof("floor_overlap", floorObjectId, floorTopCorner,
                      editor.captureWorldPickFloorLogged);
  }

  auto logObjectCenterPick =
      [&](const char* label, iggy3d::creative::CreativeObjectId expectedId,
          bool& logged) {
        const iggy3d::creative::CreativeObject* object =
            facade.findObject(expectedId);
        if (object == nullptr) {
          return;
        }
        logWorldPickProof(label,
                          expectedId,
                          visualBoundsCenter(visualBoundsForObject(*object)),
                          logged);
      };
  logObjectCenterPick("point_proxy", editor.captureScript.pointTargetId,
                     editor.captureWorldPickPointLogged);
  logObjectCenterPick("line_proxy", editor.captureScript.lineTargetId,
                      editor.captureWorldPickLineLogged);
  logObjectCenterPick("path_proxy", editor.captureScript.pathTargetId,
                     editor.captureWorldPickPathLogged);
}

[[nodiscard]] bool submitCreativeEditorFrame(
    const CreativeEditorSubmitFrameRequest& request) {
  iggy3d::FrameInput& frame = request.frame;
  const iggy3d::creative::CreativeAppState& appState = request.appState;
  CreativeEditorState& editor = request.editor;
  const CreativeEditorSelectionFrame& selection = request.selection;
  const CreativeEditorOverlayFrame& overlayFrame = request.overlayFrame;
  const StandaloneRoomBakePreviewScene& roomBakePreview =
      request.roomBakePreview;
  const iggy3d::creative::Id selectedId = selection.selectedId;
  const iggy3d::creative::CreativeObject* selected = selection.selected;
  const bool hasSelection = selection.hasSelection;

  const StandaloneFrustumCullResult frustumCull =
      cullStandaloneSceneRoomMeshesByFrustum(
          *frame.projections.scene, frame.camera.clipFromWorld);
  frame.projections.scene = &frustumCull.scene;

  const iggy3d::RenderSubmitResult submit =
      request.backend.submitFrame(frame);
  if (!editor.loggedSelection) {
    editor.loggedSelection = true;
    SDL_Log("iggy3d_creative: frame %llu submit outcome=%d reason='%s' "
            "meshes=%zu frustumInputMeshes=%zu frustumKeptMeshes=%zu "
            "frustumCulledMeshes=%zu frustumConservativeMeshes=%zu "
            "selectedTarget=%u hasSelection=%d selBoxLines=%zu "
            "pointMarkerLines=%zu lineMarkerLines=%zu pathHandleLines=%zu "
            "gizmoLines=%zu combinedWireLines=%zu uiRects=%zu glyphs=%zu",
            static_cast<unsigned long long>(editor.frameIndex),
            static_cast<int>(submit.outcome),
            std::string(submit.reason.code).c_str(),
            frustumCull.scene.room.meshes.size(),
            frustumCull.receipt.inputRoomMeshCount,
            frustumCull.receipt.keptRoomMeshCount,
            frustumCull.receipt.culledRoomMeshCount,
            frustumCull.receipt.conservativelyKeptMeshCount, selectedId,
            hasSelection ? 1 : 0,
            overlayFrame.documentWireLineCount,
            overlayFrame.pointMarkerEdgeCount,
            overlayFrame.lineMarkerEdgeCount,
            overlayFrame.pathPointHandleEdgeCount,
            overlayFrame.combinedWireLines.size() -
                overlayFrame.documentWireLineCount -
                overlayFrame.pointMarkerEdgeCount -
                overlayFrame.lineMarkerEdgeCount -
                overlayFrame.pathPointHandleEdgeCount,
            overlayFrame.combinedWireLines.size(),
            overlayFrame.uiRects.size(), overlayFrame.glyphs.size());
  }

  if (request.maxFrames != 0U && editor.frameIndex >= request.maxFrames) {
    logStandaloneRoomBakeFinal(roomBakePreview);
    // Name the SELECTED object + kind so the capture is self-documenting; the
    // capture proof expects this target to be the FLOOR.
    const char* selKind =
        hasSelection
            ? creative::toString(selected->kind).data()
            : "<none>";
    SDL_Log("iggy3d_creative: FINAL frame %llu submit outcome=%d reason='%s' "
            "selectedTarget=%u selectedKind='%s' hasSelection=%d selBoxLines=%zu "
            "pointMarkerLines=%zu lineMarkerLines=%zu pathHandleLines=%zu "
            "gizmoLines=%zu combinedWireLines=%zu placeMode=%d brush='%s' "
            "ghostEdges=%zu placed=%llu objectCount=%llu "
            "frustumInputMeshes=%zu frustumKeptMeshes=%zu "
            "frustumCulledMeshes=%zu frustumConservativeMeshes=%zu",
            static_cast<unsigned long long>(editor.frameIndex),
            static_cast<int>(submit.outcome),
            std::string(submit.reason.code).c_str(), selectedId, selKind,
            hasSelection ? 1 : 0, overlayFrame.documentWireLineCount,
            overlayFrame.pointMarkerEdgeCount,
            overlayFrame.lineMarkerEdgeCount,
            overlayFrame.pathPointHandleEdgeCount,
            overlayFrame.combinedWireLines.size() -
                overlayFrame.documentWireLineCount -
                overlayFrame.pointMarkerEdgeCount -
                overlayFrame.lineMarkerEdgeCount -
                overlayFrame.pathPointHandleEdgeCount,
            overlayFrame.combinedWireLines.size(), editor.placeMode ? 1 : 0,
            std::string(creative::toString(editor.placeBrush)).c_str(),
            overlayFrame.ghostEdgeCount,
            static_cast<unsigned long long>(editor.placedCount),
            static_cast<unsigned long long>(
                appState.facade.document().objectCount()),
            frustumCull.receipt.inputRoomMeshCount,
            frustumCull.receipt.keptRoomMeshCount,
            frustumCull.receipt.culledRoomMeshCount,
            frustumCull.receipt.conservativelyKeptMeshCount);
    return true;
  }
  return false;
}

}  // namespace iggy3d_creative_app
