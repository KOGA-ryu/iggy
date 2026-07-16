#include "render/FrameInput.hpp"

#include <iostream>
#include <limits>
#include <string_view>

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

bool frameProjectionValidationWorks() {
  iggy3d::SceneProjectionResult scene;
  scene.sourceStateHash = 42U;
  scene.sourceTick = 3U;
  iggy3d::DebugProjectionResult debug;
  iggy3d::FrameInput frame = validFrame(scene);
  frame.projections.debug = &debug;

  iggy3d::FrameInput noDebug = validFrame(scene);
  iggy3d::FrameInput missingScene = validFrame(scene);
  missingScene.projections.scene = nullptr;
  iggy3d::RenderUiRect rect;
  rect.width = 320U;
  rect.height = 120U;
  iggy3d::FrameInput uiOnly = missingScene;
  uiOnly.ui.visible = true;
  uiOnly.ui.rects = &rect;
  uiOnly.ui.rectCount = 1U;
  uiOnly.ui.primitiveCount = 1U;

  return expect(iggy3d::validateFrameInput(frame) == iggy3d::FrameInputStatus::Valid,
                "valid projection frame") &&
         expect(iggy3d::validateFrameInput(noDebug) == iggy3d::FrameInputStatus::Valid,
                "debug absent valid") &&
         expect(iggy3d::validateFrameInput(missingScene) ==
                    iggy3d::FrameInputStatus::MissingSceneProjection,
                "scene required") &&
         expect(iggy3d::validateFrameInput(uiOnly) == iggy3d::FrameInputStatus::Valid,
                "ui-only frame valid") &&
         expect(iggy3d::frameInputReasonCode(iggy3d::FrameInputStatus::MissingSceneProjection) ==
                    "frame_scene_missing",
                "scene reason");
}

bool viewportAndClockFailuresAreStable() {
  iggy3d::SceneProjectionResult scene;
  iggy3d::FrameInput zeroWidth = validFrame(scene);
  zeroWidth.viewport.width = 0U;
  iggy3d::FrameInput zeroHeight = validFrame(scene);
  zeroHeight.viewport.height = 0U;
  iggy3d::FrameInput badAspect = validFrame(scene);
  badAspect.viewport.aspectRatio = 1.0F;
  iggy3d::FrameInput negativeDelta = validFrame(scene);
  negativeDelta.clock.presentationDeltaSeconds = -0.01F;
  iggy3d::FrameInput badAlpha = validFrame(scene);
  badAlpha.clock.interpolationAlpha = 1.1F;

  return expect(iggy3d::validateFrameInput(zeroWidth) == iggy3d::FrameInputStatus::NotDrawable,
                "zero width") &&
         expect(iggy3d::validateFrameInput(zeroHeight) == iggy3d::FrameInputStatus::NotDrawable,
                "zero height") &&
         expect(iggy3d::validateFrameInput(badAspect) ==
                    iggy3d::FrameInputStatus::InvalidAspectRatio,
                "bad aspect") &&
         expect(iggy3d::validateFrameInput(negativeDelta) ==
                    iggy3d::FrameInputStatus::InvalidClock,
                "negative delta") &&
         expect(iggy3d::validateFrameInput(badAlpha) == iggy3d::FrameInputStatus::InvalidClock,
                "bad alpha") &&
         expect(iggy3d::frameInputReasonCode(iggy3d::FrameInputStatus::InvalidClock) ==
                    "frame_clock_invalid",
                "clock reason");
}

bool creativeWireframeDebugChannelValidationWorks() {
  iggy3d::SceneProjectionResult scene;
  iggy3d::FrameInput absent = validFrame(scene);
  iggy3d::FrameInput zeroCount = validFrame(scene);
  zeroCount.creativeWireframeDebug.available = true;
  zeroCount.creativeWireframeDebug.visible = true;

  iggy3d::RenderCreativeWireframeDebugLine line;
  line.start = {0.0F, 0.0F, 0.0F};
  line.end = {1.0F, 0.0F, 0.0F};
  iggy3d::FrameInput present = validFrame(scene);
  present.creativeWireframeDebug.available = true;
  present.creativeWireframeDebug.visible = true;
  present.creativeWireframeDebug.lines = &line;
  present.creativeWireframeDebug.lineCount = 1U;

  iggy3d::FrameInput missingPointer = present;
  missingPointer.creativeWireframeDebug.lines = nullptr;

  return expect(iggy3d::validateFrameInput(absent) == iggy3d::FrameInputStatus::Valid,
                "creative wireframe absent valid") &&
         expect(iggy3d::validateFrameInput(zeroCount) == iggy3d::FrameInputStatus::Valid,
                "creative wireframe zero count valid") &&
         expect(iggy3d::validateFrameInput(present) == iggy3d::FrameInputStatus::Valid,
                "creative wireframe present valid") &&
         expect(iggy3d::validateFrameInput(missingPointer) ==
                    iggy3d::FrameInputStatus::InvalidCreativeWireframeDebugLines,
                "creative wireframe pointer required") &&
         expect(iggy3d::frameInputReasonCode(
                    iggy3d::FrameInputStatus::InvalidCreativeWireframeDebugLines) ==
                    "frame_creative_wireframe_debug_lines_invalid",
                "creative wireframe invalid reason");
}

bool creativePreviewChannelIsFixedAndValidated() {
  iggy3d::SceneProjectionResult scene;
  iggy3d::FrameInput valid = validFrame(scene);
  valid.creativePreview.itemCount = 3U;
  valid.creativePreview.items[0].role =
      iggy3d::RenderCreativePreviewRole::PlacementValid;
  valid.creativePreview.items[0].geometryProfile =
      iggy3d::RenderCreativePreviewGeometryProfile::RampWedge;
  valid.creativePreview.items[1].role =
      iggy3d::RenderCreativePreviewRole::Held;
  valid.creativePreview.items[2].role =
      iggy3d::RenderCreativePreviewRole::MovingPlatformRoute;
  valid.creativePreview.items[2].geometryProfile =
      iggy3d::RenderCreativePreviewGeometryProfile::StairSteps;
  valid.creativePreview.items[2].proceduralSegmentCount = 4U;
  const bool assetSet = iggy3d::setRenderCreativePreviewAssetId(
      valid.creativePreview.items[1], "boulder_01");

  iggy3d::FrameInput tooMany = valid;
  tooMany.creativePreview.itemCount =
      static_cast<std::uint8_t>(iggy3d::kRenderCreativePreviewCapacity + 1U);
  iggy3d::FrameInput invalidRole = valid;
  invalidRole.creativePreview.items[0].role =
      iggy3d::RenderCreativePreviewRole::Count;
  iggy3d::FrameInput invalidMatrix = valid;
  invalidMatrix.creativePreview.items[1].clipFromModel.m[0] =
      std::numeric_limits<float>::quiet_NaN();
  iggy3d::FrameInput invalidAsset = valid;
  invalidAsset.creativePreview.items[1].assetId[0] = '/';
  iggy3d::FrameInput invalidGeometryProfile = valid;
  invalidGeometryProfile.creativePreview.items[0].geometryProfile =
      iggy3d::RenderCreativePreviewGeometryProfile::Count;
  iggy3d::FrameInput openFrame = valid;
  openFrame.creativePreview.items[0].geometryProfile =
      iggy3d::RenderCreativePreviewGeometryProfile::OpenFrame;
  iggy3d::FrameInput invalidOpenFrameSegments = openFrame;
  invalidOpenFrameSegments.creativePreview.items[0].proceduralSegmentCount = 1U;
  iggy3d::FrameInput missingStairSegments = valid;
  missingStairSegments.creativePreview.items[2].proceduralSegmentCount = 0U;
  iggy3d::FrameInput excessiveStairSegments = valid;
  excessiveStairSegments.creativePreview.items[2].proceduralSegmentCount =
      iggy3d::kRenderCreativePreviewMaximumStairSegmentCount + 1U;
  iggy3d::FrameInput assetWithGeneratedProfile = valid;
  assetWithGeneratedProfile.creativePreview.items[1].geometryProfile =
      iggy3d::RenderCreativePreviewGeometryProfile::RampWedge;

  return expect(assetSet &&
                    iggy3d::renderCreativePreviewAssetId(
                        valid.creativePreview.items[1]) == "boulder_01",
                "bounded creative preview asset id accepted") &&
         expect(iggy3d::validateFrameInput(valid) ==
                    iggy3d::FrameInputStatus::Valid,
                "three creative previews are valid") &&
         expect(iggy3d::validateFrameInput(openFrame) ==
                    iggy3d::FrameInputStatus::Valid,
                "open frame preview is valid without procedural segments") &&
         expect(iggy3d::validateFrameInput(tooMany) ==
                    iggy3d::FrameInputStatus::InvalidCreativePreviewItems,
                "creative preview capacity is enforced") &&
         expect(iggy3d::validateFrameInput(invalidRole) ==
                    iggy3d::FrameInputStatus::InvalidCreativePreviewItems,
                "creative preview role is validated") &&
         expect(iggy3d::validateFrameInput(invalidMatrix) ==
                    iggy3d::FrameInputStatus::InvalidCreativePreviewItems,
                "creative preview matrix must be finite") &&
         expect(iggy3d::validateFrameInput(invalidAsset) ==
                    iggy3d::FrameInputStatus::InvalidCreativePreviewItems,
                "creative preview asset id must be safe") &&
         expect(iggy3d::validateFrameInput(invalidGeometryProfile) ==
                        iggy3d::FrameInputStatus::InvalidCreativePreviewItems &&
                    iggy3d::validateFrameInput(missingStairSegments) ==
                        iggy3d::FrameInputStatus::InvalidCreativePreviewItems &&
                    iggy3d::validateFrameInput(excessiveStairSegments) ==
                        iggy3d::FrameInputStatus::InvalidCreativePreviewItems &&
                    iggy3d::validateFrameInput(invalidOpenFrameSegments) ==
                        iggy3d::FrameInputStatus::InvalidCreativePreviewItems,
                "generated preview profiles and stair counts are bounded") &&
         expect(iggy3d::validateFrameInput(assetWithGeneratedProfile) ==
                    iggy3d::FrameInputStatus::InvalidCreativePreviewItems,
                "asset and generated preview ownership cannot overlap") &&
         expect(iggy3d::frameInputReasonCode(
                    iggy3d::FrameInputStatus::InvalidCreativePreviewItems) ==
                    "frame_creative_preview_items_invalid",
                "creative preview invalid reason");
}

bool firstFailureOrderAndNoMutation() {
  iggy3d::SceneProjectionResult scene;
  scene.sourceStateHash = 99U;
  scene.items.resize(1U);
  iggy3d::FrameInput frame = validFrame(scene);
  frame.viewport.width = 0U;
  frame.clock.interpolationAlpha = -1.0F;
  frame.projections.scene = nullptr;

  const iggy3d::StateHashValue hashBefore = scene.sourceStateHash;
  const std::size_t itemCountBefore = scene.items.size();
  const iggy3d::FrameInputStatus status = iggy3d::validateFrameInput(frame);
  return expect(status == iggy3d::FrameInputStatus::NotDrawable, "first failure viewport") &&
         expect(scene.sourceStateHash == hashBefore, "scene hash unchanged") &&
         expect(scene.items.size() == itemCountBefore, "scene items unchanged");
}

}  // namespace

int main() {
  bool ok = true;
  ok = frameProjectionValidationWorks() && ok;
  ok = viewportAndClockFailuresAreStable() && ok;
  ok = creativeWireframeDebugChannelValidationWorks() && ok;
  ok = creativePreviewChannelIsFixedAndValidated() && ok;
  ok = firstFailureOrderAndNoMutation() && ok;
  return ok ? 0 : 1;
}
