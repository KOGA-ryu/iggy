#include "EditorFrame.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <span>
#include <thread>
#include <utility>

#include "app/iggy3d/creative/camera/Fly.hpp"
#include "app/iggy3d/creative/input/ActionHints.hpp"
#include "app/iggy3d/creative/input/UiInput.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "render/vulkan/VulkanBackend.hpp"

#include "EditorGamepad.hpp"

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
  creative::setCreativeInputKey(
      frame, creative::CreativeInputKey::GamepadTouchpad,
      creative::creativeControllerButtonDown(
          controller, creative::CreativeControllerButton::Touchpad));
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
    bool desktopUiWantsInput,
    const CreativeEditorState& editor) noexcept {
  struct Candidate {
    bool active = false;
    creative::CreativeInputContext context =
        creative::CreativeInputContext::EditorViewport;
  };
  const std::array candidates{
      Candidate{captureMode, creative::CreativeInputContext::Capture},
      // When the desktop shell wants the mouse/keyboard (cursor over a panel or
      // a text field focused) it wins over every editor modal, so viewport and
      // panel bindings stay quiet while the user drives ImGui. Capture stays
      // first so scripted --capture runs remain input-inert.
      Candidate{desktopUiWantsInput,
                creative::CreativeInputContext::DesktopUi},
      Candidate{editor.assetLibrary.open,
                creative::CreativeInputContext::AssetLibrary},
      Candidate{editor.assetEdit.active && editor.assetEdit.menuOpen,
                creative::CreativeInputContext::AuthoredAssetEditMenu},
      Candidate{editor.controls.open,
                creative::CreativeInputContext::Controls},
      Candidate{editor.assetReplacement.active,
                creative::CreativeInputContext::AssetReplacementPreview},
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

[[nodiscard]] bool prepareCreativeEditorDrawableFrame(
    iggy3d::SdlWindow& window,
    iggy3d::VulkanBackend& backend,
    CreativeEditorState& editor,
    CreativeEditorFrameInputResult& result) {
  window.pollEvents();
  result.monotonicTimeNanoseconds = SDL_GetTicksNS();
  result.windowFocused = window.eventState().focused;
  if (window.eventState().quitRequested) {
    result.keepRunning = false;
    return false;
  }
  if (!window.isDrawable()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    result.skipFrame = true;
    return false;
  }

  const iggy3d::SdlDrawableExtent extent = window.drawableExtent();
  if (extent.width == 0U || extent.height == 0U) {
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    result.skipFrame = true;
    return false;
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
  return true;
}

[[nodiscard]] creative::CreativeWorldInputSample makeCreativeWorldInputSample(
    const creative::CreativeInputFrame& inputFrame,
    const creative::CreativeInputRouteResult& routedInput,
    std::span<const creative::CreativeInputBinding> bindings,
    bool enabled,
    std::int32_t hotbarWheelSteps) noexcept {
  creative::CreativeWorldInputSample sample;
  if (!enabled) {
    return sample;
  }
  const auto actionDown = [&](creative::CreativeInputActionId action) {
    return creative::creativeInputActionDown(inputFrame, action, bindings,
                                             &routedInput);
  };
  constexpr std::array mappings{
      std::pair{creative::CreativeWorldActionId::Primary,
                creative::CreativeInputActionId::PrimaryAction},
      std::pair{creative::CreativeWorldActionId::Secondary,
                creative::CreativeInputActionId::SecondaryAction},
      std::pair{creative::CreativeWorldActionId::Accept,
                creative::CreativeInputActionId::AcceptAction},
      std::pair{creative::CreativeWorldActionId::Reject,
                creative::CreativeInputActionId::RejectAction},
      std::pair{creative::CreativeWorldActionId::Pick,
                creative::CreativeInputActionId::PickAction},
      std::pair{creative::CreativeWorldActionId::HotbarPrevious,
                creative::CreativeInputActionId::HotbarPrevious},
      std::pair{creative::CreativeWorldActionId::HotbarNext,
                creative::CreativeInputActionId::HotbarNext},
  };
  for (const auto& [worldAction, inputAction] : mappings) {
    creative::setCreativeWorldAction(sample, worldAction,
                                     actionDown(inputAction));
  }
  sample.hotbarWheelSteps = hotbarWheelSteps;
  return sample;
}

[[nodiscard]] iggy3d::ProductCreativeFlyInput makeCreativeEditorFlyInput(
    const creative::CreativeInputFrame& inputFrame,
    const creative::CreativeInputRouteResult& routedInput,
    std::span<const creative::CreativeInputBinding> bindings,
    creative::CreativeStickSignal moveStick,
    bool navigationActive,
    bool transformContext,
    std::int32_t transformNudgeWheelSteps,
    bool transformFineNudge) noexcept {
  iggy3d::ProductCreativeFlyInput flyInput;
  if (!navigationActive) {
    return flyInput;
  }
  const auto actionDown = [&](creative::CreativeInputActionId action) {
    return creative::creativeInputActionDown(inputFrame, action, bindings,
                                             &routedInput);
  };
  const bool precisionNudgeRequested =
      transformContext &&
      (transformNudgeWheelSteps != 0 ||
       routedActionPresent(
           routedInput,
           creative::CreativeInputActionId::TransformNudgeNegative) ||
       routedActionPresent(
           routedInput,
           creative::CreativeInputActionId::TransformNudgePositive));
  const float keyboardMoveX =
      (actionDown(creative::CreativeInputActionId::MoveRight) ? 1.0F : 0.0F) -
      (actionDown(creative::CreativeInputActionId::MoveLeft) ? 1.0F : 0.0F);
  const float keyboardMoveY =
      (actionDown(creative::CreativeInputActionId::MoveForward) ? 1.0F : 0.0F) -
      (actionDown(creative::CreativeInputActionId::MoveBackward) ? 1.0F : 0.0F);
  const bool ascend = actionDown(creative::CreativeInputActionId::FlyUp);
  const bool descendRequested =
      actionDown(creative::CreativeInputActionId::FlyDown);
  const bool descend =
      descendRequested && !(precisionNudgeRequested && transformFineNudge);
  flyInput.moveX = std::clamp(keyboardMoveX + moveStick.x, -1.0F, 1.0F);
  flyInput.moveY = std::clamp(keyboardMoveY + moveStick.y, -1.0F, 1.0F);
  flyInput.moveZ = (ascend ? 1.0F : 0.0F) - (descend ? 1.0F : 0.0F);
  flyInput.sprinting = actionDown(creative::CreativeInputActionId::Sprint);
  return flyInput;
}

}  // namespace

namespace {

constexpr creative::CreativeStickProfile kRadialStickProfile{
    0.0F, 1.0F, false, false};
constexpr creative::CreativeWheelProfile kFrameWheelProfile{};

}  // namespace

CreativeEditorNavigationAdmission admitCreativeEditorNavigation(
    creative::CreativeInputContext inputContext,
    bool transformControlsOpen,
    bool rightStickLookRearmRequired,
    creative::CreativeStickSignal rightStickLook,
    bool catalogToggleRouted,
    bool toolWheelToggleRouted) noexcept {
  const bool viewportContext =
      inputContext == creative::CreativeInputContext::EditorViewport ||
      inputContext ==
          creative::CreativeInputContext::AssetReplacementPreview ||
      inputContext == creative::CreativeInputContext::TransformPreview;
  CreativeEditorNavigationAdmission admission;
  admission.navigationActive =
      viewportContext && !transformControlsOpen && !catalogToggleRouted &&
      !toolWheelToggleRouted;
  admission.clearRightStickLookRearm =
      rightStickLookRearmRequired && !rightStickLook.active;
  const bool rearmPending =
      rightStickLookRearmRequired && !admission.clearRightStickLookRearm;
  admission.rightStickLookActive =
      admission.navigationActive && !rearmPending;
  return admission;
}

CreativeEditorFrameInputResult beginCreativeEditorFrameInput(
    iggy3d::SdlWindow& window,
    iggy3d::VulkanBackend& backend,
    CreativeEditorGamepad& gamepad,
    CreativeEditorState& editor,
    bool captureMode) {
  CreativeEditorFrameInputResult result;
  result.activeControlDevice = editor.activeControlDevice;
  if (!prepareCreativeEditorDrawableFrame(window, backend, editor, result)) {
    return result;
  }

  const bool* keys = SDL_GetKeyboardState(nullptr);
  const creative::CreativeControllerSample controllerSample = gamepad.sample();
  const creative::CreativeControllerFrame controller =
      creative::stepCreativeControllerInput(editor.controllerInputState,
                                            controllerSample);
  editor.controllerInputState = controller.next;
  const bool desktopUiWantsInput =
      backend.externalUiWantsMouse() || backend.externalUiWantsKeyboard();
  const creative::CreativeInputContext inputContext =
      resolveInputContext(captureMode, desktopUiWantsInput, editor);
  const creative::CreativeInputFrame inputFrame = makeCreativeInputFrame(
      keys, SDL_GetModState(), inputContext, controller);
  result.inputFrame = inputFrame;
  result.routedInput =
      creative::routeCreativeInput(editor.inputRouterState, inputFrame,
                                   editor.controlProfile.bindingSpan());
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
  const std::int32_t wheelSteps = creative::quantizeCreativeWheelSteps(
      window.eventState().mouseWheelY, kFrameWheelProfile);
  result.transformNudgeWheelSteps =
      inputContext == creative::CreativeInputContext::TransformPreview
          ? wheelSteps
          : 0;
  result.transformFineNudge =
      (inputFrame.modifiers & creative::kCreativeInputModifierShift) != 0U ||
      creative::creativeControllerButtonDown(
          controller, creative::CreativeControllerButton::LeftShoulder);
  result.toolWheelDirectionX = radialStick.x;
  result.toolWheelDirectionY = radialStick.y;
  const creative::CreativeWorldInputSample worldInput =
      makeCreativeWorldInputSample(
          inputFrame, result.routedInput, editor.controlProfile.bindingSpan(),
          !captureMode && result.windowFocused, wheelSteps);
  result.worldActions = creative::routeCreativeWorldActions(
      editor.interaction.actionRouter, worldInput);
  const CreativeEditorNavigationAdmission navigation =
      admitCreativeEditorNavigation(
          inputContext, editor.transform.controlsOpen,
          editor.rightStickLookRearmRequired, lookStick,
          routedActionPresent(result.routedInput,
                              creative::CreativeInputActionId::ToggleCatalog),
          routedActionPresent(
              result.routedInput,
              creative::CreativeInputActionId::ToggleToolWheel));
  if (navigation.clearRightStickLookRearm) {
    editor.rightStickLookRearmRequired = false;
  }
  iggy3d::ProductCreativeFlyInput flyInput = makeCreativeEditorFlyInput(
      inputFrame, result.routedInput, editor.controlProfile.bindingSpan(),
      moveStick, navigation.navigationActive,
      inputContext == creative::CreativeInputContext::TransformPreview,
      result.transformNudgeWheelSteps, result.transformFineNudge);

  float mouseDx = 0.0F;
  float mouseDy = 0.0F;
  SDL_GetRelativeMouseState(&mouseDx, &mouseDy);
  const creative::CreativeControlDeviceActivity deviceActivity =
      creative::measureCreativeControlDeviceActivity(
          inputFrame,
          mouseDx != 0.0F || mouseDy != 0.0F ||
              window.eventState().mouseWheelY != 0.0F,
          moveStick.active || lookStick.active);
  editor.activeControlDevice = creative::resolveCreativeActiveControlDevice(
      editor.activeControlDevice, deviceActivity);
  result.activeControlDevice = editor.activeControlDevice;
  if (navigation.navigationActive) {
    const float gamepadYaw = navigation.rightStickLookActive
                                 ? gamepadLook.yawDegrees
                                 : 0.0F;
    const float gamepadPitch = navigation.rightStickLookActive
                                   ? gamepadLook.pitchDegrees
                                   : 0.0F;
    editor.yawDegrees += mouseDx * editor.controlProfile.mouseLookSensitivity +
                         gamepadYaw;
    editor.pitchDegrees = std::clamp(
        editor.pitchDegrees -
            mouseDy * editor.controlProfile.mouseLookSensitivity +
            gamepadPitch,
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


}  // namespace iggy3d_creative_app
