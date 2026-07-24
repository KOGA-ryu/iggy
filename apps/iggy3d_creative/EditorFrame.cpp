#include "EditorFrame.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <thread>

#include "app/iggy3d/creative/camera/Fly.hpp"
#include "app/iggy3d/creative/camera/ViewportNavigation.hpp"
#include "app/iggy3d/creative/input/ActionHints.hpp"
#include "app/iggy3d/creative/input/UiInput.hpp"
#include "app/iggy3d/creative/input/WorldActionIntent.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "render/vulkan/VulkanBackend.hpp"

#include "EditorGamepad.hpp"
#include "EditorWorldLayout.hpp"

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

struct SdlKeyMapping {
  creative::CreativeInputKey key;
  SDL_Scancode scancode;
};

constexpr std::array kSdlKeyMappings{
    SdlKeyMapping{creative::CreativeInputKey::Digit1, SDL_SCANCODE_1},
    SdlKeyMapping{creative::CreativeInputKey::Digit2, SDL_SCANCODE_2},
    SdlKeyMapping{creative::CreativeInputKey::Digit3, SDL_SCANCODE_3},
    SdlKeyMapping{creative::CreativeInputKey::Digit4, SDL_SCANCODE_4},
    SdlKeyMapping{creative::CreativeInputKey::Digit5, SDL_SCANCODE_5},
    SdlKeyMapping{creative::CreativeInputKey::Digit6, SDL_SCANCODE_6},
    SdlKeyMapping{creative::CreativeInputKey::Digit7, SDL_SCANCODE_7},
    SdlKeyMapping{creative::CreativeInputKey::Digit8, SDL_SCANCODE_8},
    SdlKeyMapping{creative::CreativeInputKey::Digit9, SDL_SCANCODE_9},
    SdlKeyMapping{creative::CreativeInputKey::C, SDL_SCANCODE_C},
    SdlKeyMapping{creative::CreativeInputKey::D, SDL_SCANCODE_D},
    SdlKeyMapping{creative::CreativeInputKey::E, SDL_SCANCODE_E},
    SdlKeyMapping{creative::CreativeInputKey::N, SDL_SCANCODE_N},
    SdlKeyMapping{creative::CreativeInputKey::O, SDL_SCANCODE_O},
    SdlKeyMapping{creative::CreativeInputKey::R, SDL_SCANCODE_R},
    SdlKeyMapping{creative::CreativeInputKey::S, SDL_SCANCODE_S},
    SdlKeyMapping{creative::CreativeInputKey::V, SDL_SCANCODE_V},
    SdlKeyMapping{creative::CreativeInputKey::X, SDL_SCANCODE_X},
    SdlKeyMapping{creative::CreativeInputKey::Y, SDL_SCANCODE_Y},
    SdlKeyMapping{creative::CreativeInputKey::Z, SDL_SCANCODE_Z},
    SdlKeyMapping{creative::CreativeInputKey::LeftBracket,
                  SDL_SCANCODE_LEFTBRACKET},
    SdlKeyMapping{creative::CreativeInputKey::RightBracket,
                  SDL_SCANCODE_RIGHTBRACKET},
    SdlKeyMapping{creative::CreativeInputKey::Minus, SDL_SCANCODE_MINUS},
    SdlKeyMapping{creative::CreativeInputKey::Equals, SDL_SCANCODE_EQUALS},
    SdlKeyMapping{creative::CreativeInputKey::Delete, SDL_SCANCODE_DELETE},
    SdlKeyMapping{creative::CreativeInputKey::Backspace,
                  SDL_SCANCODE_BACKSPACE},
    SdlKeyMapping{creative::CreativeInputKey::Enter, SDL_SCANCODE_RETURN},
    SdlKeyMapping{creative::CreativeInputKey::Escape, SDL_SCANCODE_ESCAPE},
    SdlKeyMapping{creative::CreativeInputKey::ArrowUp, SDL_SCANCODE_UP},
    SdlKeyMapping{creative::CreativeInputKey::ArrowDown, SDL_SCANCODE_DOWN},
    SdlKeyMapping{creative::CreativeInputKey::ArrowLeft, SDL_SCANCODE_LEFT},
    SdlKeyMapping{creative::CreativeInputKey::ArrowRight, SDL_SCANCODE_RIGHT},
    SdlKeyMapping{creative::CreativeInputKey::W, SDL_SCANCODE_W},
    SdlKeyMapping{creative::CreativeInputKey::A, SDL_SCANCODE_A},
    SdlKeyMapping{creative::CreativeInputKey::Space, SDL_SCANCODE_SPACE},
    SdlKeyMapping{creative::CreativeInputKey::LeftControl, SDL_SCANCODE_LCTRL},
    SdlKeyMapping{creative::CreativeInputKey::RightControl,
                  SDL_SCANCODE_RCTRL},
    SdlKeyMapping{creative::CreativeInputKey::LeftShift, SDL_SCANCODE_LSHIFT},
    SdlKeyMapping{creative::CreativeInputKey::RightShift,
                  SDL_SCANCODE_RSHIFT},
    SdlKeyMapping{creative::CreativeInputKey::LeftAlt, SDL_SCANCODE_LALT},
    SdlKeyMapping{creative::CreativeInputKey::RightAlt, SDL_SCANCODE_RALT},
    SdlKeyMapping{creative::CreativeInputKey::LeftCommand, SDL_SCANCODE_LGUI},
    SdlKeyMapping{creative::CreativeInputKey::RightCommand, SDL_SCANCODE_RGUI},
};

struct ControllerKeyMapping {
  creative::CreativeInputKey key;
  creative::CreativeControllerButton button;
};

constexpr std::array kControllerKeyMappings{
    ControllerKeyMapping{creative::CreativeInputKey::GamepadInventory,
                         creative::CreativeControllerButton::North},
    ControllerKeyMapping{creative::CreativeInputKey::GamepadDpadUp,
                         creative::CreativeControllerButton::DpadUp},
    ControllerKeyMapping{creative::CreativeInputKey::GamepadDpadDown,
                         creative::CreativeControllerButton::DpadDown},
    ControllerKeyMapping{creative::CreativeInputKey::GamepadConfirm,
                         creative::CreativeControllerButton::South},
    ControllerKeyMapping{creative::CreativeInputKey::GamepadCancel,
                         creative::CreativeControllerButton::East},
    ControllerKeyMapping{creative::CreativeInputKey::GamepadDpadLeft,
                         creative::CreativeControllerButton::DpadLeft},
    ControllerKeyMapping{creative::CreativeInputKey::GamepadDpadRight,
                         creative::CreativeControllerButton::DpadRight},
    ControllerKeyMapping{creative::CreativeInputKey::GamepadLeftShoulder,
                         creative::CreativeControllerButton::LeftShoulder},
    ControllerKeyMapping{creative::CreativeInputKey::GamepadRightShoulder,
                         creative::CreativeControllerButton::RightShoulder},
    ControllerKeyMapping{creative::CreativeInputKey::GamepadWest,
                         creative::CreativeControllerButton::West},
    ControllerKeyMapping{creative::CreativeInputKey::GamepadBack,
                         creative::CreativeControllerButton::Back},
    ControllerKeyMapping{creative::CreativeInputKey::GamepadStart,
                         creative::CreativeControllerButton::Start},
    ControllerKeyMapping{creative::CreativeInputKey::GamepadLeftStick,
                         creative::CreativeControllerButton::LeftStick},
    ControllerKeyMapping{creative::CreativeInputKey::GamepadRightStick,
                         creative::CreativeControllerButton::RightStick},
    ControllerKeyMapping{creative::CreativeInputKey::GamepadLeftTrigger,
                         creative::CreativeControllerButton::LeftTrigger},
    ControllerKeyMapping{creative::CreativeInputKey::GamepadRightTrigger,
                         creative::CreativeControllerButton::RightTrigger},
    ControllerKeyMapping{creative::CreativeInputKey::GamepadTouchpad,
                         creative::CreativeControllerButton::Touchpad},
};

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

  for (const SdlKeyMapping& mapping : kSdlKeyMappings) {
    setSdlKey(frame, mapping.key, keys, mapping.scancode);
  }
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
  for (const ControllerKeyMapping& mapping : kControllerKeyMappings) {
    creative::setCreativeInputKey(
        frame, mapping.key,
        creative::creativeControllerButtonDown(controller, mapping.button));
  }
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

[[nodiscard]] creative::CreativeInputContext resolveInputContextImpl(
    bool captureMode,
    bool desktopUiWantsInput,
    bool editorInteractionEnabled,
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
      Candidate{editorInteractionEnabled && editor.assetLibrary.open,
                creative::CreativeInputContext::AssetLibrary},
      Candidate{editorInteractionEnabled && editor.assetEdit.active &&
                    editor.assetEdit.menuOpen,
                creative::CreativeInputContext::AuthoredAssetEditMenu},
      Candidate{editorInteractionEnabled && editor.controls.open,
                creative::CreativeInputContext::Controls},
      Candidate{editorInteractionEnabled && editor.assetReplacement.active,
                creative::CreativeInputContext::AssetReplacementPreview},
      Candidate{editorInteractionEnabled && editor.transform.active,
                editor.transform.controlsOpen
                    ? creative::CreativeInputContext::TransformControls
                    : creative::CreativeInputContext::TransformPreview},
      Candidate{editorInteractionEnabled && editor.catalog.model.open,
                creative::CreativeInputContext::Catalog},
      Candidate{editorInteractionEnabled && editor.catalog.toolWheel.open,
                creative::CreativeInputContext::ToolWheel},
      Candidate{editorInteractionEnabled && editor.toolOptions.open,
                creative::CreativeInputContext::ToolOptions},
  };
  const auto active = std::find_if(
      candidates.begin(), candidates.end(),
      [](const Candidate& candidate) { return candidate.active; });
  return active == candidates.end()
             ? (editorInteractionEnabled
                    ? creative::CreativeInputContext::EditorViewport
                    : creative::CreativeInputContext::RuntimePlay)
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

[[nodiscard]] iggy3d::ProductCreativeFlyInput makeCreativeEditorFlyInput(
    const creative::CreativeInputRouteResult& routedInput,
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
    return creative::creativeInputActionDown(routedInput, action);
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

[[nodiscard]] bool applyCreativeEditorViewportNavigation(
    CreativeEditorState& editor,
    iggy3d::ProductCreativeViewportNavigationOperation operation,
    float horizontalInput,
    float verticalInput,
    float viewportHeightPixels) noexcept {
  iggy3d::ProductCreativeViewportNavigationRequest request;
  request.config = editor.viewportNavigationConfig;
  request.focus = editor.viewportFocus;
  request.pose.anchorPositionMeters = editor.flyPos;
  request.pose.yawDegrees = editor.yawDegrees;
  request.pose.pitchDegrees = editor.pitchDegrees;
  request.operation = operation;
  request.horizontalInput = horizontalInput;
  request.verticalInput = verticalInput;
  request.viewportHeightPixels = viewportHeightPixels;
  request.orbitDegreesPerPixel = editor.controlProfile.mouseLookSensitivity;
  const iggy3d::ProductCreativeViewportNavigationResult result =
      iggy3d::applyProductCreativeViewportNavigation(request);
  if (!result.applied) {
    return false;
  }
  editor.viewportFocus = result.focus;
  editor.flyPos = result.pose.anchorPositionMeters;
  editor.yawDegrees = result.pose.yawDegrees;
  editor.pitchDegrees = result.pose.pitchDegrees;
  return true;
}

}  // namespace

namespace {

constexpr creative::CreativeStickProfile kRadialStickProfile{
    0.0F, 1.0F, false, false};
constexpr creative::CreativeWheelProfile kFrameWheelProfile{};

}  // namespace

creative::CreativeInputContext resolveCreativeEditorInputContext(
    bool captureMode,
    bool desktopUiWantsInput,
    bool editorInteractionEnabled,
    const CreativeEditorState& editor) noexcept {
  return resolveInputContextImpl(captureMode, desktopUiWantsInput,
                                 editorInteractionEnabled, editor);
}

CreativeEditorNavigationAdmission admitCreativeEditorNavigation(
    creative::CreativeInputContext inputContext,
    bool transformControlsOpen,
    bool rightStickLookRearmRequired,
    creative::CreativeStickSignal rightStickLook,
    bool catalogToggleRouted,
    bool toolWheelToggleRouted) noexcept {
  const bool viewportContext =
      inputContext == creative::CreativeInputContext::EditorViewport ||
      inputContext == creative::CreativeInputContext::RuntimePlay ||
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
    bool captureMode,
    bool applyEditorNavigation) {
  CreativeEditorFrameInputResult result;
  result.activeControlDevice = editor.activeControlDevice;
  if (!prepareCreativeEditorDrawableFrame(window, backend, editor, result)) {
    return result;
  }

  // NewFrame-before-context: start the ImGui frame now — after pollEvents (in
  // prepareCreativeEditorDrawableFrame) forwarded this frame's events, and
  // before the input context is resolved — so WantCapture is current, not one
  // frame stale. Only reached on non-skipped frames, so NewFrame stays balanced
  // with the Render in endCreativeEditorDesktopFrame.
  static_cast<void>(beginCreativeEditorDesktopUiFrame(editor.desktopUi, backend));

  const bool* keys = SDL_GetKeyboardState(nullptr);
  const creative::CreativeControllerSample controllerSample = gamepad.sample();
  const creative::CreativeControllerFrame controller =
      creative::stepCreativeControllerInput(editor.controllerInputState,
                                            controllerSample);
  editor.controllerInputState = controller.next;
  // While the viewport owns the pointer (relative-mouse fly-look), the app owns
  // the mouse — ImGui only sees a warped, wandering cursor, so its want-capture
  // must be ignored or the shell reclaims the context and releases the capture
  // the instant the camera moves (plan DD-9).
  const bool desktopUiWantsInput = creativeDesktopUiWantsInput(
      editor.desktopUi.viewportPointerCaptureMode,
      backend.externalUiWantsMouse(), backend.externalUiWantsKeyboard());
  const creative::CreativeInputContext inputContext =
      resolveCreativeEditorInputContext(captureMode, desktopUiWantsInput,
                                        applyEditorNavigation, editor);
  const creative::CreativeInputFrame inputFrame = makeCreativeInputFrame(
      keys, SDL_GetModState(), inputContext, controller);
  result.inputFrame = inputFrame;
  result.routedInput =
      creative::routeCreativeInput(editor.inputRouterState, inputFrame,
                                   editor.controlProfile.bindingSpan(),
                                   editor.controlProfile.controllerCommandSpan());
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
  const bool viewportDollyRequested =
      applyEditorNavigation && creativeDesktopViewportDollyRequested(
                                   editor.desktopUi.viewportPointerCaptureMode,
                                   inputFrame.modifiers,
                                   window.eventState().mouseWheelY,
                                   inputContext == creative::
                                                       CreativeInputContext::
                                                           EditorViewport);
  result.worldActions = creative::routeCreativeWorldActions(
      editor.interaction.actionRouter, result.routedInput,
      !captureMode && result.windowFocused,
      viewportDollyRequested ? 0 : wheelSteps);
  const CreativeEditorNavigationAdmission navigation =
      admitCreativeEditorNavigation(
          inputContext,
          applyEditorNavigation && editor.transform.controlsOpen,
          editor.rightStickLookRearmRequired, lookStick,
          routedActionPresent(result.routedInput,
                              creative::CreativeInputActionId::ToggleCatalog),
          routedActionPresent(
              result.routedInput,
              creative::CreativeInputActionId::ToggleToolWheel));
  if (navigation.clearRightStickLookRearm) {
    editor.rightStickLookRearmRequired = false;
  }
  const CreativeDesktopPointerCaptureMode pointerMode =
      editor.desktopUi.viewportPointerCaptureMode;
  const bool orbitGesture =
      pointerMode == CreativeDesktopPointerCaptureMode::Orbit;
  const bool panGesture =
      pointerMode == CreativeDesktopPointerCaptureMode::Pan;
  const bool viewportGesture = orbitGesture || panGesture;
  iggy3d::ProductCreativeFlyInput flyInput = makeCreativeEditorFlyInput(
      result.routedInput, moveStick,
      navigation.navigationActive && !viewportGesture,
      inputContext == creative::CreativeInputContext::TransformPreview,
      result.transformNudgeWheelSteps, result.transformFineNudge);

  float mouseDx = 0.0F;
  float mouseDy = 0.0F;
  SDL_GetRelativeMouseState(&mouseDx, &mouseDy);
  if (editor.desktopUi.discardNextViewportMouseDelta) {
    mouseDx = 0.0F;
    mouseDy = 0.0F;
    editor.desktopUi.discardNextViewportMouseDelta = false;
  }
  const creative::CreativeControlDeviceActivity deviceActivity =
      creative::measureCreativeControlDeviceActivity(
          inputFrame,
          mouseDx != 0.0F || mouseDy != 0.0F ||
              window.eventState().mouseWheelY != 0.0F,
          moveStick.active || lookStick.active);
  editor.activeControlDevice = creative::resolveCreativeActiveControlDevice(
      editor.activeControlDevice, deviceActivity);
  result.activeControlDevice = editor.activeControlDevice;
  const float gamepadYaw =
      navigation.rightStickLookActive && !viewportGesture
                               ? gamepadLook.yawDegrees
                               : 0.0F;
  const float gamepadPitch =
      navigation.rightStickLookActive && !viewportGesture
                                 ? gamepadLook.pitchDegrees
                                 : 0.0F;
  const bool mouseFlyLookActive = creativeDesktopMouseLookActive(
      editor.desktopUi.shellEnabled, pointerMode);
  result.navigationActive = navigation.navigationActive;
  result.navigationMoveRight = flyInput.moveX;
  result.navigationMoveForward = flyInput.moveY;
  result.navigationSprinting = flyInput.sprinting;
  result.navigationYawDeltaDegrees =
      navigation.navigationActive
          ? (mouseFlyLookActive
                 ? mouseDx * editor.controlProfile.mouseLookSensitivity
                 : 0.0F) +
                gamepadYaw
          : 0.0F;
  result.navigationPitchDeltaDegrees =
      navigation.navigationActive
          ? (mouseFlyLookActive
                 ? -mouseDy * editor.controlProfile.mouseLookSensitivity
                 : 0.0F) +
                gamepadPitch
          : 0.0F;
  const float viewportHeightPixels =
      editor.desktopUi.contentViewport.height > 0U
          ? static_cast<float>(editor.desktopUi.contentViewport.height)
          : static_cast<float>(result.extent.height);
  if (applyEditorNavigation && navigation.navigationActive && viewportGesture) {
    static_cast<void>(applyCreativeEditorViewportNavigation(
        editor,
        orbitGesture
            ? iggy3d::ProductCreativeViewportNavigationOperation::Orbit
            : iggy3d::ProductCreativeViewportNavigationOperation::Pan,
        mouseDx, mouseDy, viewportHeightPixels));
  } else if (applyEditorNavigation && viewportDollyRequested) {
    static_cast<void>(applyCreativeEditorViewportNavigation(
        editor, iggy3d::ProductCreativeViewportNavigationOperation::Dolly,
        0.0F, window.eventState().mouseWheelY, viewportHeightPixels));
  } else if (applyEditorNavigation && navigation.navigationActive) {
    editor.yawDegrees += result.navigationYawDeltaDegrees;
    editor.pitchDegrees = std::clamp(
        editor.pitchDegrees + result.navigationPitchDeltaDegrees,
        -80.0F, 80.0F);
    if (result.navigationYawDeltaDegrees != 0.0F ||
        result.navigationPitchDeltaDegrees != 0.0F) {
      editor.viewportFocus.valid = false;
    }
  }
  flyInput.cameraYawDegrees = editor.yawDegrees;
  flyInput.cameraPitchDegrees = editor.pitchDegrees;

  if (applyEditorNavigation) {
    const iggy3d::ProductCreativeFlyResult flyResult =
        applyProductCreativeFlyInput(editor.flyConfig, flyInput, editor.flyPos);
    if (flyResult.applied) {
      editor.flyPos = flyResult.finalPositionMeters;
      editor.viewportFocus.valid = false;
    }
  }

  return result;
}


}  // namespace iggy3d_creative_app
