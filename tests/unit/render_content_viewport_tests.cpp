#include "render/FrameInput.hpp"

#include <iostream>
#include <string_view>

// UI-1a (docs/creative_desktop_ui_plan.md): the content viewport is a
// sub-rectangle of the swapchain-sized viewport that the 3D scene occupies.
// The all-zero sentinel means full-frame so every existing producer and the
// --capture path stay byte-identical; an explicit rect must be non-zero and
// fit inside the viewport. Existing FrameInputStatus enumerators/reason codes
// must be untouched (render_projection_input_tests pins them).

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::FrameInput validFrame(const iggy3d::SceneProjectionResult& scene) {
  iggy3d::FrameInput frame;
  frame.viewport = {1280U, 720U, 1280.0F / 720.0F};
  frame.clock = {3U, 10U, 0.5F, 1.0F / 60.0F};
  frame.camera.mode = iggy3d::RenderCameraMode::ThirdPerson;
  frame.camera.worldEye = {0.0F, 2.0F, 5.0F};
  frame.camera.worldForward = {0.0F, 0.0F, -1.0F};
  frame.camera.worldUp = {0.0F, 1.0F, 0.0F};
  frame.camera.nearPlane = 0.1F;
  frame.camera.farPlane = 200.0F;
  frame.projections.scene = &scene;
  return frame;
}

bool defaultIsFullFrameSentinel() {
  iggy3d::SceneProjectionResult scene;
  const iggy3d::FrameInput frame = validFrame(scene);
  const iggy3d::RenderContentViewport effective =
      iggy3d::effectiveContentViewport(frame);
  return expect(iggy3d::isFullFrameContentViewport(frame.contentViewport),
                "default content viewport is the full-frame sentinel") &&
         expect(iggy3d::validateFrameInput(frame) ==
                    iggy3d::FrameInputStatus::Valid,
                "default content viewport validates") &&
         expect(effective.x == 0 && effective.y == 0 &&
                    effective.width == frame.viewport.width &&
                    effective.height == frame.viewport.height,
                "sentinel resolves to the full viewport rect");
}

bool explicitSubRectValidatesAndResolves() {
  iggy3d::SceneProjectionResult scene;
  iggy3d::FrameInput frame = validFrame(scene);
  frame.contentViewport = {240, 32, 800, 600};
  const iggy3d::RenderContentViewport effective =
      iggy3d::effectiveContentViewport(frame);
  return expect(!iggy3d::isFullFrameContentViewport(frame.contentViewport),
                "explicit rect is not the sentinel") &&
         expect(iggy3d::validateFrameInput(frame) ==
                    iggy3d::FrameInputStatus::Valid,
                "in-bounds sub-rect validates") &&
         expect(effective.x == 240 && effective.y == 32 &&
                    effective.width == 800U && effective.height == 600U,
                "explicit rect resolves to itself unchanged");
}

bool edgeAlignedRectIsValid() {
  iggy3d::SceneProjectionResult scene;
  iggy3d::FrameInput frame = validFrame(scene);
  // Exactly filling the viewport from a non-zero origin: right/bottom touch
  // the edges but do not exceed them.
  frame.contentViewport = {0, 0, 1280U, 720U};
  iggy3d::FrameInput offset = validFrame(scene);
  offset.contentViewport = {280, 120, 1000U, 600U};
  return expect(iggy3d::validateFrameInput(frame) ==
                    iggy3d::FrameInputStatus::Valid,
                "full-extent explicit rect validates") &&
         expect(iggy3d::validateFrameInput(offset) ==
                    iggy3d::FrameInputStatus::Valid,
                "edge-touching offset rect validates");
}

bool outOfBoundsAndZeroSizedRectsRejected() {
  iggy3d::SceneProjectionResult scene;
  iggy3d::FrameInput tooWide = validFrame(scene);
  tooWide.contentViewport = {1, 0, 1280U, 720U};  // x+width = 1281 > 1280
  iggy3d::FrameInput tooTall = validFrame(scene);
  tooTall.contentViewport = {0, 400, 640U, 400U};  // y+height = 800 > 720
  iggy3d::FrameInput negativeOrigin = validFrame(scene);
  negativeOrigin.contentViewport = {-1, 0, 640U, 360U};
  iggy3d::FrameInput zeroWidth = validFrame(scene);
  zeroWidth.contentViewport = {10, 10, 0U, 360U};  // partial set, not sentinel
  iggy3d::FrameInput zeroHeight = validFrame(scene);
  zeroHeight.contentViewport = {10, 10, 640U, 0U};

  return expect(iggy3d::validateFrameInput(tooWide) ==
                    iggy3d::FrameInputStatus::InvalidContentViewport,
                "rect wider than viewport rejected") &&
         expect(iggy3d::validateFrameInput(tooTall) ==
                    iggy3d::FrameInputStatus::InvalidContentViewport,
                "rect taller than viewport rejected") &&
         expect(iggy3d::validateFrameInput(negativeOrigin) ==
                    iggy3d::FrameInputStatus::InvalidContentViewport,
                "negative origin rejected") &&
         expect(iggy3d::validateFrameInput(zeroWidth) ==
                    iggy3d::FrameInputStatus::InvalidContentViewport,
                "zero-width explicit rect rejected") &&
         expect(iggy3d::validateFrameInput(zeroHeight) ==
                    iggy3d::FrameInputStatus::InvalidContentViewport,
                "zero-height explicit rect rejected") &&
         expect(iggy3d::frameInputReasonCode(
                    iggy3d::FrameInputStatus::InvalidContentViewport) ==
                    "frame_content_viewport_invalid",
                "content viewport reason code is stable");
}

bool existingReasonCodesUnchanged() {
  // Guards the appended enumerator against silently shifting existing codes.
  return expect(iggy3d::frameInputReasonCode(
                    iggy3d::FrameInputStatus::NotDrawable) ==
                    "frame_not_drawable",
                "not_drawable code stable") &&
         expect(iggy3d::frameInputReasonCode(
                    iggy3d::FrameInputStatus::InvalidAspectRatio) ==
                    "frame_aspect_invalid",
                "aspect code stable") &&
         expect(iggy3d::frameInputReasonCode(
                    iggy3d::FrameInputStatus::InvalidCreativePreviewItems) ==
                    "frame_creative_preview_items_invalid",
                "creative preview code stable");
}

bool contentViewportDoesNotShadowViewportFailures() {
  // A zero-extent viewport must still fail NotDrawable first, even with an
  // explicit (and, against the zero viewport, out-of-bounds) content rect.
  iggy3d::SceneProjectionResult scene;
  iggy3d::FrameInput frame = validFrame(scene);
  frame.viewport.width = 0U;
  frame.contentViewport = {0, 0, 640U, 360U};
  return expect(iggy3d::validateFrameInput(frame) ==
                    iggy3d::FrameInputStatus::NotDrawable,
                "zero viewport fails NotDrawable before content-rect check");
}

}  // namespace

int main() {
  bool ok = true;
  ok = defaultIsFullFrameSentinel() && ok;
  ok = explicitSubRectValidatesAndResolves() && ok;
  ok = edgeAlignedRectIsValid() && ok;
  ok = outOfBoundsAndZeroSizedRectsRejected() && ok;
  ok = existingReasonCodesUnchanged() && ok;
  ok = contentViewportDoesNotShadowViewportFailures() && ok;
  return ok ? 0 : 1;
}
