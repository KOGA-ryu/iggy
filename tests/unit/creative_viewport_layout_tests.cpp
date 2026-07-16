#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "app/iggy3d/creative/input/UiInput.hpp"
#include "app/iggy3d/creative/render/CreativeSceneFrame.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"

#include "EditorDesktopUi.hpp"
#include "EditorFrame.hpp"

#include <iostream>
#include <string_view>

// UI-1b (docs/creative_desktop_ui_plan.md): the central dock node rect is
// mapped to a drawable-pixel content viewport (DL-6, the one home for
// logical<->drawable rect conversion), which drives the camera aspect. Until
// docked panels shrink the central node the rect is full-frame, so behavior —
// and the --capture hash — is unchanged; these tests pin the conversion and
// the aspect wiring directly rather than through the renderer.

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool nearly(float a, float b, float tol = 1.0e-4F) {
  const float d = a - b;
  return (d < 0.0F ? -d : d) <= tol;
}

bool conversionAtUnitScaleIsIdentity() {
  const iggy3d::creative::CreativeContentRect rect =
      iggy3d::creative::resolveCreativeContentViewport(
          100.0F, 60.0F, 500.0F, 360.0F, 1.0F, 1.0F, 800U, 600U);
  return expect(rect.valid, "unit-scale rect is valid") &&
         expect(rect.x == 100 && rect.y == 60, "unit-scale origin preserved") &&
         expect(rect.width == 400U && rect.height == 300U,
                "unit-scale size preserved");
}

bool conversionAtHiDpiScalesToDrawablePixels() {
  // macOS Retina: logical points -> 2x drawable pixels.
  const iggy3d::creative::CreativeContentRect rect =
      iggy3d::creative::resolveCreativeContentViewport(
          100.0F, 50.0F, 500.0F, 350.0F, 2.0F, 2.0F, 1600U, 1200U);
  return expect(rect.valid, "hidpi rect is valid") &&
         expect(rect.x == 200 && rect.y == 100, "hidpi origin scaled 2x") &&
         expect(rect.width == 800U && rect.height == 600U,
                "hidpi size scaled 2x");
}

bool conversionClampsToDrawableExtent() {
  // A rect running past the window edges clamps rather than overflowing.
  const iggy3d::creative::CreativeContentRect rect =
      iggy3d::creative::resolveCreativeContentViewport(
          -20.0F, -10.0F, 900.0F, 700.0F, 1.0F, 1.0F, 800U, 600U);
  return expect(rect.valid, "clamped rect is valid") &&
         expect(rect.x == 0 && rect.y == 0, "negative origin clamps to zero") &&
         expect(rect.width == 800U && rect.height == 600U,
                "oversize rect clamps to the drawable extent");
}

bool degenerateConversionsAreInvalid() {
  const iggy3d::creative::CreativeContentRect zeroExtent =
      iggy3d::creative::resolveCreativeContentViewport(
          0.0F, 0.0F, 100.0F, 100.0F, 1.0F, 1.0F, 0U, 600U);
  const iggy3d::creative::CreativeContentRect empty =
      iggy3d::creative::resolveCreativeContentViewport(
          10.0F, 10.0F, 10.0F, 200.0F, 1.0F, 1.0F, 800U, 600U);
  return expect(!zeroExtent.valid, "zero drawable extent is invalid") &&
         expect(!empty.valid, "zero-width logical rect is invalid");
}

iggy3d::FrameInput sceneFrame(const iggy3d::SceneProjectionResult& scene,
                              const iggy3d::DebugProjectionResult& debug,
                              iggy3d::RenderContentViewport content) {
  return iggy3d::makeCreativeVulkanFrame(
      scene, debug, /*frameIndex=*/0U, /*viewportWidth=*/1280U,
      /*viewportHeight=*/720U, /*yaw=*/0.0F, /*pitch=*/0.0F,
      /*cameraAnchorOverrideAvailable=*/false, /*cameraAnchor=*/{}, content);
}

bool sentinelContentKeepsSwapchainAspect() {
  iggy3d::SceneProjectionResult scene;
  iggy3d::DebugProjectionResult debug;
  const iggy3d::FrameInput frame = sceneFrame(scene, debug, {});
  return expect(iggy3d::isFullFrameContentViewport(frame.contentViewport),
                "sentinel content viewport is carried through") &&
         expect(frame.viewport.width == 1280U && frame.viewport.height == 720U,
                "viewport stays swapchain-sized") &&
         expect(iggy3d::validateFrameInput(frame) ==
                    iggy3d::FrameInputStatus::Valid,
                "sentinel content frame validates");
}

bool narrowerContentTightensCameraAspect() {
  iggy3d::SceneProjectionResult scene;
  iggy3d::DebugProjectionResult debug;
  const iggy3d::FrameInput full = sceneFrame(scene, debug, {});
  // Half-width central node: aspect 640/720 < 1280/720, so f/aspect (m[0]) grows.
  const iggy3d::RenderContentViewport half{0, 0, 640U, 720U};
  const iggy3d::FrameInput narrow = sceneFrame(scene, debug, half);
  return expect(!iggy3d::isFullFrameContentViewport(narrow.contentViewport),
                "explicit content viewport is carried") &&
         expect(narrow.contentViewport.width == 640U,
                "explicit content width preserved") &&
         expect(iggy3d::validateFrameInput(narrow) ==
                    iggy3d::FrameInputStatus::Valid,
                "explicit sub-rect frame validates") &&
         expect(narrow.camera.clipFromView.m[0] > full.camera.clipFromView.m[0],
                "narrower content increases the horizontal projection scale") &&
         expect(nearly(full.camera.clipFromView.m[5],
                       narrow.camera.clipFromView.m[5]),
                "vertical projection scale is unchanged by content width");
}

bool pointerPolicyCapturesOnViewportClick() {
  using iggy3d_creative_app::decideCreativeDesktopPointerCapture;
  // Click on the viewport (not a panel), no modal, focused -> capture.
  const auto capture = decideCreativeDesktopPointerCapture(
      /*shellEnabled=*/true, /*currentlyCaptured=*/false,
      /*primaryPressedOverViewport=*/true, /*viewportContext=*/true,
      /*windowFocused=*/true);
  // Click that ImGui consumed (over a panel) must NOT capture.
  const auto overPanel = decideCreativeDesktopPointerCapture(
      true, false, /*primaryPressedOverViewport=*/false, true, true);
  return expect(capture.captured && capture.changed,
                "viewport click captures the pointer") &&
         expect(!overPanel.captured && !overPanel.changed,
                "a click consumed by a panel does not capture");
}

bool pointerPolicyReleasesWhenLeavingViewport() {
  using iggy3d_creative_app::decideCreativeDesktopPointerCapture;
  // Held capture + a modal opened (context left EditorViewport) -> release.
  const auto modalOpened = decideCreativeDesktopPointerCapture(
      true, /*currentlyCaptured=*/true, false, /*viewportContext=*/false, true);
  // Held capture + focus lost -> release.
  const auto focusLost = decideCreativeDesktopPointerCapture(
      true, true, false, true, /*windowFocused=*/false);
  // Held capture + still in the viewport, focused -> stays captured.
  const auto stays = decideCreativeDesktopPointerCapture(true, true, false,
                                                         true, true);
  return expect(!modalOpened.captured && modalOpened.changed,
                "leaving the viewport context releases the pointer") &&
         expect(!focusLost.captured && focusLost.changed,
                "losing focus releases the pointer") &&
         expect(stays.captured && !stays.changed,
                "staying in the viewport keeps the pointer captured");
}

bool pointerPolicyIsInertWhenShellOff() {
  using iggy3d_creative_app::decideCreativeDesktopPointerCapture;
  // Shell off (capture mode / non-desktop): never captures, and reports a
  // release only if we were somehow holding it.
  const auto clean = decideCreativeDesktopPointerCapture(
      /*shellEnabled=*/false, false, true, true, true);
  const auto releaseStale = decideCreativeDesktopPointerCapture(
      false, /*currentlyCaptured=*/true, true, true, true);
  return expect(!clean.captured && !clean.changed,
                "shell-off never captures") &&
         expect(!releaseStale.captured && releaseStale.changed,
                "shell-off releases a stale capture");
}

bool wantInputHelperIgnoresImGuiWhileCaptured() {
  using iggy3d_creative_app::creativeDesktopUiWantsInput;
  // The pointer-fix invariant: while the viewport owns the pointer, the shell
  // never claims input no matter what ImGui's want-capture flags say (that was
  // releasing fly-look the instant the camera moved).
  return expect(!creativeDesktopUiWantsInput(true, true, true),
                "captured viewport suppresses the desktop-UI input claim") &&
         expect(!creativeDesktopUiWantsInput(true, false, false),
                "captured + no ImGui intent is still no claim") &&
         expect(creativeDesktopUiWantsInput(false, true, false),
                "free pointer + ImGui wants mouse claims input") &&
         expect(creativeDesktopUiWantsInput(false, false, true),
                "free pointer + ImGui wants keyboard claims input") &&
         expect(!creativeDesktopUiWantsInput(false, false, false),
                "free pointer + no ImGui intent does not claim input");
}

bool routeRemoveDropsActionForEscRelease() {
  namespace cr = iggy3d::creative;
  cr::CreativeInputRouteResult route;
  route.actions[0].action = cr::CreativeInputActionId::ToggleControls;
  route.actions[1].action = cr::CreativeInputActionId::Undo;
  route.actions[2].action = cr::CreativeInputActionId::ToggleControls;
  route.actionCount = 3;
  const bool had = cr::creativeInputRouteContains(
      route, cr::CreativeInputActionId::ToggleControls);
  cr::creativeInputRouteRemove(route,
                               cr::CreativeInputActionId::ToggleControls);
  return expect(had, "route reports the action present before removal") &&
         expect(!cr::creativeInputRouteContains(
                    route, cr::CreativeInputActionId::ToggleControls),
                "route no longer contains the removed action") &&
         expect(route.actionCount == 1U &&
                    route.actions[0].action ==
                        cr::CreativeInputActionId::Undo,
                "remove compacts the route and preserves other actions");
}

bool desktopUiContextDisablesFlyNavigation() {
  using iggy3d_creative_app::admitCreativeEditorNavigation;
  namespace cr = iggy3d::creative;
  const cr::CreativeStickSignal noStick{};
  const auto viewport = admitCreativeEditorNavigation(
      cr::CreativeInputContext::EditorViewport, false, false, noStick, false,
      false);
  const auto runtimePlay = admitCreativeEditorNavigation(
      cr::CreativeInputContext::RuntimePlay, false, false, noStick, false,
      false);
  const auto desktopUi = admitCreativeEditorNavigation(
      cr::CreativeInputContext::DesktopUi, false, false, noStick, false, false);
  return expect(viewport.navigationActive,
                "fly navigation is active in the viewport context") &&
         expect(runtimePlay.navigationActive,
                "runtime movement and look stay active in Play") &&
         expect(!desktopUi.navigationActive,
                "fly navigation is off while the desktop UI owns input");
}

bool desktopShellRequiresExplicitLaunchRequest() {
  using iggy3d_creative_app::creativeDesktopShellEnabledForLaunch;
  return expect(!creativeDesktopShellEnabledForLaunch(false, false),
                "plain launch keeps the full-viewport editor") &&
         expect(creativeDesktopShellEnabledForLaunch(true, false),
                "desktop flag explicitly enables the IDE shell") &&
         expect(!creativeDesktopShellEnabledForLaunch(true, true),
                "capture mode remains UI-free");
}

}  // namespace

int main() {
  bool ok = true;
  ok = conversionAtUnitScaleIsIdentity() && ok;
  ok = conversionAtHiDpiScalesToDrawablePixels() && ok;
  ok = conversionClampsToDrawableExtent() && ok;
  ok = degenerateConversionsAreInvalid() && ok;
  ok = sentinelContentKeepsSwapchainAspect() && ok;
  ok = narrowerContentTightensCameraAspect() && ok;
  ok = pointerPolicyCapturesOnViewportClick() && ok;
  ok = pointerPolicyReleasesWhenLeavingViewport() && ok;
  ok = pointerPolicyIsInertWhenShellOff() && ok;
  ok = wantInputHelperIgnoresImGuiWhileCaptured() && ok;
  ok = routeRemoveDropsActionForEscRelease() && ok;
  ok = desktopUiContextDisablesFlyNavigation() && ok;
  ok = desktopShellRequiresExplicitLaunchRequest() && ok;
  return ok ? 0 : 1;
}
