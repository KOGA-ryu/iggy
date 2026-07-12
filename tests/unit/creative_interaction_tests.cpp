#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"
#include "app/iggy3d/creative/camera/Fly.hpp"
#include "EditorInteraction.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <span>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double actual, double expected) {
  return std::fabs(actual - expected) <= 1.0e-9;
}

bool nearFloat(float actual, float expected) {
  return std::fabs(actual - expected) <= 1.0e-6F;
}

bool controllerTransitionSanitizesAndOwnsEdges() {
  cr::CreativeControllerSample sample;
  sample.connected = true;
  cr::setCreativeControllerAxis(
      sample, cr::CreativeControllerAxis::LeftStickX,
      cr::kCreativeControllerStickDeadzone);
  cr::setCreativeControllerAxis(sample,
                                cr::CreativeControllerAxis::LeftStickY, 0.0F);
  cr::setCreativeControllerAxis(sample,
                                cr::CreativeControllerAxis::RightStickX,
                                1.5F);
  cr::setCreativeControllerAxis(
      sample, cr::CreativeControllerAxis::RightStickY,
      std::numeric_limits<float>::quiet_NaN());
  cr::setCreativeControllerAxis(
      sample, cr::CreativeControllerAxis::LeftTrigger,
      cr::kCreativeControllerTriggerThreshold - 0.01F);
  cr::setCreativeControllerAxis(
      sample, cr::CreativeControllerAxis::RightTrigger,
      cr::kCreativeControllerTriggerThreshold);
  cr::setCreativeControllerButton(
      sample, cr::CreativeControllerButton::South, true);
  cr::setCreativeControllerButton(
      sample, cr::CreativeControllerButton::LeftTrigger, true);

  const cr::CreativeControllerFrame pressed =
      cr::stepCreativeControllerInput({}, sample);
  const cr::CreativeControllerFrame held =
      cr::stepCreativeControllerInput(pressed.next, sample);
  const cr::CreativeControllerFrame disconnected =
      cr::stepCreativeControllerInput(held.next, {});
  const cr::CreativeStickSignal leftStick = cr::creativeControllerStick(
      pressed, cr::CreativeControllerStick::Left);
  const cr::CreativeStickSignal rightStick = cr::creativeControllerStick(
      pressed, cr::CreativeControllerStick::Right);

  return expect(pressed.next.connected, "controller connection is preserved") &&
         expect(nearFloat(cr::creativeControllerAxis(
                              pressed,
                              cr::CreativeControllerAxis::LeftStickX),
                          cr::kCreativeControllerStickDeadzone),
                "frame preserves sanitized physical stick travel") &&
         expect(!leftStick.active && nearFloat(leftStick.magnitude, 0.0F),
                "stick primitive owns the radial deadzone") &&
         expect(nearFloat(cr::creativeControllerAxis(
                              pressed,
                              cr::CreativeControllerAxis::RightStickX),
                          1.0F),
                "physical stick axes are clamped") &&
         expect(nearFloat(cr::creativeControllerAxis(
                              pressed,
                              cr::CreativeControllerAxis::RightStickY),
                          0.0F),
                "non-finite physical axes sanitize to zero") &&
         expect(rightStick.active && nearFloat(rightStick.x, 1.0F) &&
                    nearFloat(rightStick.y, 0.0F),
                "stick lookup shapes the selected physical pair") &&
         expect(cr::creativeControllerButtonPressed(
                    pressed, cr::CreativeControllerButton::South) &&
                    cr::creativeControllerButtonDown(
                        pressed, cr::CreativeControllerButton::South),
                "digital button press edge is emitted") &&
         expect(!cr::creativeControllerButtonDown(
                    pressed, cr::CreativeControllerButton::LeftTrigger),
                "trigger down state is derived from its axis") &&
         expect(cr::creativeControllerButtonPressed(
                    pressed, cr::CreativeControllerButton::RightTrigger),
                "trigger threshold is inclusive") &&
         expect(!cr::creativeControllerButtonPressed(
                    held, cr::CreativeControllerButton::South) &&
                    cr::creativeControllerButtonDown(
                        held, cr::CreativeControllerButton::South),
                "held buttons do not repeat press edges") &&
         expect(!disconnected.next.connected &&
                    cr::creativeControllerButtonReleased(
                        disconnected, cr::CreativeControllerButton::South) &&
                    cr::creativeControllerButtonReleased(
                        disconnected,
                        cr::CreativeControllerButton::RightTrigger),
                "disconnect emits release edges and clears connection") &&
         expect(nearFloat(cr::creativeControllerAxis(
                              disconnected,
                              cr::CreativeControllerAxis::RightTrigger),
                          0.0F),
                "disconnect clears normalized axes") &&
         expect(!cr::creativeControllerButtonDown(
                    pressed, cr::CreativeControllerButton::Count) &&
                    nearFloat(cr::creativeControllerAxis(
                                  pressed, cr::CreativeControllerAxis::Count),
                              0.0F),
                "sentinel controller controls are safely ignored");
}

bool controllerStickPrimitiveHasCanonicalDirections() {
  struct DirectionCase {
    float rawX;
    float rawY;
    float expectedX;
    float expectedY;
    std::string_view label;
  };
  constexpr std::array directions{
      DirectionCase{-1.0F, 0.0F, -1.0F, 0.0F, "left stays left"},
      DirectionCase{1.0F, 0.0F, 1.0F, 0.0F, "right stays right"},
      DirectionCase{0.0F, -1.0F, 0.0F, -1.0F, "down stays down"},
      DirectionCase{0.0F, 1.0F, 0.0F, 1.0F, "up stays up"},
  };
  cr::CreativeStickProfile noDeadzone;
  noDeadzone.deadzone = 0.0F;
  bool ok =
      expect(nearFloat(cr::canonicalCreativeStickComponent(
                           std::numeric_limits<std::int16_t>::min(),
                           cr::CreativeStickComponent::X),
                       -1.0F),
             "raw hardware left maps to canonical left") &&
      expect(nearFloat(cr::canonicalCreativeStickComponent(
                           std::numeric_limits<std::int16_t>::max(),
                           cr::CreativeStickComponent::X),
                       1.0F),
             "raw hardware right maps to canonical right") &&
      expect(nearFloat(cr::canonicalCreativeStickComponent(
                           std::numeric_limits<std::int16_t>::min(),
                           cr::CreativeStickComponent::Y),
                       1.0F),
             "raw hardware up maps to canonical up") &&
      expect(nearFloat(cr::canonicalCreativeStickComponent(
                           std::numeric_limits<std::int16_t>::max(),
                           cr::CreativeStickComponent::Y),
                       -1.0F),
             "raw hardware down maps to canonical down") &&
      expect(nearFloat(cr::canonicalCreativeStickComponent(
                           123, cr::CreativeStickComponent::Count),
                       0.0F),
             "sentinel hardware component is safely ignored");
  for (const DirectionCase& direction : directions) {
    const cr::CreativeStickSignal signal = cr::shapeCreativeControllerStick(
        direction.rawX, direction.rawY, noDeadzone);
    ok = expect(signal.active && nearFloat(signal.x, direction.expectedX) &&
                    nearFloat(signal.y, direction.expectedY) &&
                    nearFloat(signal.magnitude, 1.0F),
                direction.label) &&
         ok;
  }

  const cr::CreativeStickSignal boundary = cr::shapeCreativeControllerStick(
      cr::kCreativeControllerStickDeadzone, 0.0F);
  const cr::CreativeStickSignal diagonal = cr::shapeCreativeControllerStick(
      cr::kCreativeControllerStickDeadzone,
      cr::kCreativeControllerStickDeadzone);
  const cr::CreativeStickSignal fullDiagonal =
      cr::shapeCreativeControllerStick(1.0F, 1.0F, noDeadzone);
  cr::CreativeStickProfile shaped = noDeadzone;
  shaped.responseExponent = 2.0F;
  const cr::CreativeStickSignal halfShaped =
      cr::shapeCreativeControllerStick(0.5F, 0.0F, shaped);
  shaped.invertX = true;
  shaped.invertY = true;
  const cr::CreativeStickSignal inverted =
      cr::shapeCreativeControllerStick(-1.0F, 1.0F, shaped);
  const cr::CreativeControllerLookDelta lookLeft =
      cr::creativeControllerLookDelta(
          cr::shapeCreativeControllerStick(-1.0F, 0.0F, noDeadzone), 2.4F);
  const cr::CreativeControllerLookDelta lookRight =
      cr::creativeControllerLookDelta(
          cr::shapeCreativeControllerStick(1.0F, 0.0F, noDeadzone), 2.4F);
  const cr::CreativeControllerLookDelta lookDown =
      cr::creativeControllerLookDelta(
          cr::shapeCreativeControllerStick(0.0F, -1.0F, noDeadzone), 2.4F);
  const cr::CreativeControllerLookDelta lookUp =
      cr::creativeControllerLookDelta(
          cr::shapeCreativeControllerStick(0.0F, 1.0F, noDeadzone), 2.4F);
  cr::CreativeStickProfile invalid = noDeadzone;
  invalid.deadzone = 1.0F;

  return expect(!boundary.active,
                "radial deadzone boundary is inactive") &&
         expect(diagonal.active && diagonal.x > 0.0F && diagonal.y > 0.0F,
                "diagonal travel is tested as one vector") &&
         expect(fullDiagonal.active &&
                    nearFloat(fullDiagonal.magnitude, 1.0F) &&
                    nearFloat(fullDiagonal.x, 0.70710677F) &&
                    nearFloat(fullDiagonal.y, 0.70710677F),
                "diagonal travel is normalized to the unit circle") &&
         expect(halfShaped.active && nearFloat(halfShaped.magnitude, 0.25F),
                "consumer response exponent shapes magnitude") &&
         expect(inverted.active && inverted.x > 0.0F && inverted.y < 0.0F,
                "consumer profile owns explicit axis inversion") &&
         expect(nearFloat(lookLeft.yawDegrees, -2.4F) &&
                    nearFloat(lookRight.yawDegrees, 2.4F),
                "camera look maps left and right without reversal") &&
         expect(nearFloat(lookDown.pitchDegrees, -2.4F) &&
                    nearFloat(lookUp.pitchDegrees, 2.4F),
                "camera look maps down and up without reversal") &&
         expect(!cr::shapeCreativeControllerStick(
                     std::numeric_limits<float>::quiet_NaN(), 0.0F)
                     .active &&
                    !cr::shapeCreativeControllerStick(1.0F, 0.0F, invalid)
                         .active,
                "invalid stick samples and profiles are inactive") &&
         expect(!cr::creativeControllerStick(
                     {}, cr::CreativeControllerStick::Count)
                     .active,
                "sentinel physical stick is safely ignored") &&
         ok;
}

bool controllerMovementPreservesDirectionAndFineTravel() {
  iggy3d::ProductCreativeFlyConfig config;
  config.enabled = true;
  config.speedMetersPerSecond = 8.0F;
  config.inputStepSeconds = 1.0F;

  iggy3d::ProductCreativeFlyInput left;
  left.moveX = -0.5F;
  const iggy3d::ProductCreativeFlyResult leftResult =
      iggy3d::applyProductCreativeFlyInput(config, left, {});
  iggy3d::ProductCreativeFlyInput right;
  right.moveX = 0.5F;
  const iggy3d::ProductCreativeFlyResult rightResult =
      iggy3d::applyProductCreativeFlyInput(config, right, {});
  iggy3d::ProductCreativeFlyInput forward;
  forward.moveY = 0.5F;
  const iggy3d::ProductCreativeFlyResult forwardResult =
      iggy3d::applyProductCreativeFlyInput(config, forward, {});
  iggy3d::ProductCreativeFlyInput keyboardDiagonal;
  keyboardDiagonal.moveX = 1.0F;
  keyboardDiagonal.moveY = 1.0F;
  const iggy3d::ProductCreativeFlyResult diagonalResult =
      iggy3d::applyProductCreativeFlyInput(config, keyboardDiagonal, {});

  return expect(leftResult.applied && nearFloat(leftResult.deltaMeters.x,
                                                -4.0F),
                "half stick left moves left at half speed") &&
         expect(rightResult.applied && nearFloat(rightResult.deltaMeters.x,
                                                 4.0F),
                "half stick right moves right at half speed") &&
         expect(forwardResult.applied && nearFloat(forwardResult.deltaMeters.z,
                                                   -4.0F),
                "half stick forward moves forward at half speed") &&
         expect(diagonalResult.applied &&
                    nearFloat(std::hypot(diagonalResult.deltaMeters.x,
                                         diagonalResult.deltaMeters.z),
                              8.0F),
                "full keyboard diagonal remains capped at full speed");
}

bool worldActionsAreEdgeTriggered() {
  cr::CreativeWorldActionRouterState state;
  cr::CreativeWorldInputSample sample;
  cr::setCreativeWorldAction(sample, cr::CreativeWorldActionId::Primary, true);
  sample.hotbarWheelSteps = -2;

  const cr::CreativeWorldActionFrame pressed =
      cr::routeCreativeWorldActions(state, sample);
  const cr::CreativeWorldActionFrame held =
      cr::routeCreativeWorldActions(state, sample);
  cr::setCreativeWorldAction(sample, cr::CreativeWorldActionId::Primary, false);
  sample.hotbarWheelSteps = 0;
  const cr::CreativeWorldActionFrame released =
      cr::routeCreativeWorldActions(state, sample);

  return expect(cr::creativeWorldActionDown(
                    pressed, cr::CreativeWorldActionId::Primary),
                "primary down routed") &&
         expect(cr::creativeWorldActionPressed(
                    pressed, cr::CreativeWorldActionId::Primary),
                "primary press edge routed") &&
         expect(!cr::creativeWorldActionReleased(
                    pressed, cr::CreativeWorldActionId::Primary),
                "primary press is not release") &&
         expect(pressed.hotbarWheelSteps == -2,
                "wheel steps preserved in action frame") &&
         expect(cr::creativeWorldActionDown(
                    held, cr::CreativeWorldActionId::Primary) &&
                    !cr::creativeWorldActionPressed(
                        held, cr::CreativeWorldActionId::Primary),
                "held action does not repeat press") &&
         expect(cr::creativeWorldActionReleased(
                    released, cr::CreativeWorldActionId::Primary) &&
                    !cr::creativeWorldActionDown(
                        released, cr::CreativeWorldActionId::Primary),
                "primary release edge routed");
}

bool hotbarHasStableNineSlotGrammar() {
  constexpr std::array palette{cr::CreativeObjectKind::Wall,
                               cr::CreativeObjectKind::Crate};
  cr::CreativeHotbarState hotbar = cr::makeDefaultCreativeHotbar(palette);
  constexpr std::array expectedKinds{
      cr::CreativeHeldItemKind::Material,
      cr::CreativeHeldItemKind::ObjectSelect,
      cr::CreativeHeldItemKind::ObjectMove,
      cr::CreativeHeldItemKind::VolumeSelect,
      cr::CreativeHeldItemKind::VolumeFill,
      cr::CreativeHeldItemKind::VolumeHollow,
      cr::CreativeHeldItemKind::VolumeReplace,
      cr::CreativeHeldItemKind::VolumeErase,
      cr::CreativeHeldItemKind::VolumeClone,
  };

  bool ok = expect(hotbar.selectedSlot == 0U,
                   "default hotbar selects material slot") &&
            expect(hotbar.entries[0].objectKind == cr::CreativeObjectKind::Wall,
                   "default material comes from palette") &&
            expect(std::equal(expectedKinds.begin(), expectedKinds.end(),
                              hotbar.entries.begin(),
                              [](cr::CreativeHeldItemKind expected,
                                 const cr::CreativeHotbarEntry& actual) {
                                return expected == actual.kind;
                              }),
                   "nine slots retain held-item order");

  ok = expect(cr::cycleCreativeHotbar(hotbar, -1) &&
                  hotbar.selectedSlot == 8U,
              "wheel wraps to final hotbar slot") &&
       expect(cr::cycleCreativeHotbar(hotbar, 3) &&
                  hotbar.selectedSlot == 2U,
              "multi-step wheel wraps forward") &&
       expect(cr::assignCreativeHotbarMaterial(
                  hotbar, cr::CreativeObjectKind::Crate),
              "pick block replaces selected slot") &&
       expect(hotbar.entries[2].kind == cr::CreativeHeldItemKind::Material &&
                  hotbar.entries[2].objectKind == cr::CreativeObjectKind::Crate,
              "picked material becomes held item") &&
       expect(!cr::selectCreativeHotbarSlot(hotbar,
                                            cr::kCreativeHotbarSlotCount),
              "out-of-range direct slot rejected") &&
       ok;
  return ok;
}

bool heldVolumeItemsMapWithoutBranchesAtCallers() {
  constexpr std::array operationItems{
      cr::CreativeHeldItemKind::VolumeFill,
      cr::CreativeHeldItemKind::VolumeHollow,
      cr::CreativeHeldItemKind::VolumeReplace,
      cr::CreativeHeldItemKind::VolumeErase,
      cr::CreativeHeldItemKind::VolumeClone,
  };
  constexpr std::array operations{
      cr::CreativeVolumeOperationKind::Fill,
      cr::CreativeVolumeOperationKind::Hollow,
      cr::CreativeVolumeOperationKind::Replace,
      cr::CreativeVolumeOperationKind::Erase,
      cr::CreativeVolumeOperationKind::Clone,
  };

  bool ok = true;
  for (std::size_t index = 0; index < operationItems.size(); ++index) {
    ok = expect(cr::creativeHeldItemIsVolumeOperation(operationItems[index]),
                "volume item classified") &&
         expect(cr::creativeVolumeOperationForHeldItem(operationItems[index]) ==
                    operations[index],
                "volume item maps to operation") &&
         ok;
  }
  cr::CreativeHotbarEntry materialTool{
      cr::CreativeHeldItemKind::VolumeFill,
      cr::CreativeObjectKind::Wall};
  cr::CreativeHotbarEntry materialFreeTool{
      cr::CreativeHeldItemKind::VolumeErase,
      cr::CreativeObjectKind::Unknown};
  return expect(!cr::creativeHeldItemIsVolumeOperation(
                    cr::CreativeHeldItemKind::Material),
                "material is not volume operation") &&
         expect(!cr::creativeHeldItemIsVolumeOperation(
                    cr::CreativeHeldItemKind::LinearArray) &&
                    cr::toString(cr::CreativeHeldItemKind::LinearArray) ==
                        "Array",
                "array is a distinct held tool") &&
         expect(!cr::creativeHeldItemIsVolumeOperation(
                    cr::CreativeHeldItemKind::ConnectedFill) &&
                    cr::toString(cr::CreativeHeldItemKind::ConnectedFill) ==
                        "Flood",
                "connected fill is a distinct non-volume held tool") &&
         expect(!cr::creativeHeldItemIsVolumeOperation(
                    cr::CreativeHeldItemKind::SurfaceExtrude) &&
                    cr::toString(cr::CreativeHeldItemKind::SurfaceExtrude) ==
                        "Extrude",
                "surface extrude is a distinct non-volume held tool") &&
         expect(cr::creativeHeldItemUsesDirectShapeGesture(
                    cr::CreativeHeldItemKind::VolumeFill) &&
                    cr::creativeHeldItemUsesDirectShapeGesture(
                        cr::CreativeHeldItemKind::VolumeHollow) &&
                    !cr::creativeHeldItemUsesDirectShapeGesture(
                        cr::CreativeHeldItemKind::VolumeReplace),
                "fill and hollow alone own direct shape gestures") &&
         expect(cr::creativeHeldItemUsesMaterial(
                    cr::CreativeHeldItemKind::Material) &&
                    cr::creativeHeldItemUsesMaterial(
                        cr::CreativeHeldItemKind::VolumeFill) &&
                    cr::creativeHeldItemUsesMaterial(
                        cr::CreativeHeldItemKind::VolumeHollow) &&
                    cr::creativeHeldItemUsesMaterial(
                        cr::CreativeHeldItemKind::VolumeReplace) &&
                    cr::creativeHeldItemUsesMaterial(
                        cr::CreativeHeldItemKind::ConnectedFill) &&
                    cr::creativeHeldItemUsesMaterial(
                        cr::CreativeHeldItemKind::SurfaceExtrude),
                "material-backed held items are explicit") &&
         expect(!cr::creativeHeldItemUsesMaterial(
                    cr::CreativeHeldItemKind::VolumeErase) &&
                    !cr::creativeHeldItemUsesMaterial(
                        cr::CreativeHeldItemKind::VolumeClone) &&
                    !cr::creativeHeldItemUsesMaterial(
                        cr::CreativeHeldItemKind::ObjectSelect) &&
                    !cr::creativeHeldItemUsesMaterial(
                        cr::CreativeHeldItemKind::LinearArray),
                "material-free held items remain explicit") &&
         expect(cr::applyCreativeHeldItemMaterial(
                    materialTool, cr::CreativeObjectKind::Crate) &&
                    materialTool.objectKind == cr::CreativeObjectKind::Crate &&
                    !cr::applyCreativeHeldItemMaterial(
                        materialTool, cr::CreativeObjectKind::Crate),
                "material override applies on semantic change only") &&
         expect(!cr::applyCreativeHeldItemMaterial(
                    materialFreeTool, cr::CreativeObjectKind::Crate) &&
                    !cr::applyCreativeHeldItemMaterial(
                        materialTool, cr::CreativeObjectKind::Unknown),
                "invalid or material-free override is rejected") &&
         ok;
}

bool gridTargetResolvesHitFaceAndPlacementCell() {
  const cr::CreativeGridTarget top = cr::resolveCreativeGridTargetFromHit(
      {1.25, 1.0, 2.25}, {0.1, 0.9, 0.2}, 1.0);
  bool ok = expect(top.valid, "top-face target valid") &&
            expect(top.faceNormal.x == 0.0 && top.faceNormal.y == 1.0 &&
                       top.faceNormal.z == 0.0,
                   "face normal snaps to dominant axis") &&
            expect(top.targetCell.x == 1 && top.targetCell.y == 0 &&
                       top.targetCell.z == 2,
                   "hit-side cell resolved") &&
            expect(top.adjacentCell.x == 1 && top.adjacentCell.y == 1 &&
                       top.adjacentCell.z == 2,
                   "placement-side cell resolved") &&
            expect(near(top.placementAnchor.x, 1.5) &&
                       near(top.placementAnchor.y, 1.0) &&
                       near(top.placementAnchor.z, 2.5),
                   "placement anchor uses adjacent cell base") &&
            expect(top.placerForward.x == 0.0 &&
                       top.placerForward.z == -1.0,
                   "default placement facing is deterministic");

  const cr::CreativeGridTarget facing = cr::resolveCreativeGridTargetFromHit(
      {1.25, 1.0, 2.25}, {0.0, 1.0, 0.0}, 1.0, {},
      {-0.9, 0.2, 0.1});
  ok = expect(facing.valid && facing.placerForward.x == -1.0 &&
                  facing.placerForward.y == 0.0 &&
                  facing.placerForward.z == 0.0,
              "placer look direction snaps to a horizontal cardinal axis") &&
       ok;

  const cr::CreativeGridTarget side = cr::resolveCreativeGridTargetFromHit(
      {0.0, 2.4, -1.2}, {-1.0, 0.0, 0.0}, 1.0);
  ok = expect(side.valid, "negative side target valid") &&
       expect(side.targetCell.x == 0 && side.targetCell.y == 2 &&
                  side.targetCell.z == -2,
              "negative side hit cell resolved") &&
       expect(side.adjacentCell.x == -1 && side.adjacentCell.y == 2 &&
                  side.adjacentCell.z == -2,
              "negative side adjacent cell resolved") &&
       expect(near(side.placementAnchor.x, -0.5) &&
                  near(side.placementAnchor.y, 2.0) &&
                  near(side.placementAnchor.z, -1.5),
              "side placement anchor uses adjacent cell base") &&
       ok;

  const cr::CreativeGridTarget unaligned =
      cr::resolveCreativeGridTargetFromHit(
          {1.25, 2.4, -1.2}, {1.0, 0.0, 0.0}, 1.0);
  ok = expect(unaligned.valid, "non-grid-aligned face target valid") &&
       expect(unaligned.targetCell.x == 1 &&
                  unaligned.adjacentCell.x == 2,
              "adjacent cell follows face step, not epsilon bucket") &&
       expect(near(unaligned.placementAnchor.x, 2.5),
              "non-grid-aligned placement cannot overlap target cell") &&
       ok;

  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double maxCellFace =
      static_cast<double>(std::numeric_limits<std::int32_t>::max()) + 0.5;
  return expect(!cr::resolveCreativeGridTargetFromHit(
                     {nan, 0.0, 0.0}, {0.0, 1.0, 0.0}, 1.0)
                     .valid,
                "non-finite hit rejected") &&
         expect(!cr::resolveCreativeGridTargetFromHit(
                     {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, 1.0)
                     .valid,
                "degenerate face normal rejected") &&
         expect(!cr::resolveCreativeGridTargetFromHit(
                     {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 0.0)
                     .valid,
                "non-positive cell size rejected") &&
         expect(!cr::resolveCreativeGridTargetFromHit(
                     {maxCellFace, 0.5, 0.5}, {1.0, 0.0, 0.0}, 1.0)
                     .valid,
                "adjacent-cell coordinate overflow rejected") &&
         expect(!cr::resolveCreativeGridTargetFromHit(
                     {0.5, 1.0, 0.5}, {0.0, 1.0, 0.0}, 1.0, {},
                     {nan, 0.0, 0.0})
                     .valid,
                "non-finite placer facing rejected") &&
         ok;
}

bool placementFeedbackHasABoundedVisibleLifetime() {
  using iggy3d_creative_app::CreativeEditorPlacementFeedback;
  using iggy3d_creative_app::CreativeEditorPlacementFeedbackStatus;
  using iggy3d_creative_app::creativeEditorPlacementFeedbackVisible;
  using iggy3d_creative_app::kCreativeEditorPlacementFeedbackFrames;

  CreativeEditorPlacementFeedback feedback;
  bool ok = expect(!creativeEditorPlacementFeedbackVisible(feedback, 10U),
                   "empty placement feedback stays hidden");
  feedback.status = CreativeEditorPlacementFeedbackStatus::Placed;
  feedback.objectId = 42U;
  feedback.frameIndex = 100U;
  ok = expect(!creativeEditorPlacementFeedbackVisible(feedback, 99U),
              "placement feedback does not appear before its receipt") &&
       expect(creativeEditorPlacementFeedbackVisible(feedback, 100U),
              "placement feedback appears on its receipt frame") &&
       expect(creativeEditorPlacementFeedbackVisible(
                  feedback,
                  100U + kCreativeEditorPlacementFeedbackFrames - 1U),
              "placement feedback remains visible for its bounded window") &&
       expect(!creativeEditorPlacementFeedbackVisible(
                  feedback, 100U + kCreativeEditorPlacementFeedbackFrames),
              "placement feedback expires deterministically") &&
       ok;
  feedback.status = CreativeEditorPlacementFeedbackStatus::Rejected;
  return expect(creativeEditorPlacementFeedbackVisible(feedback, 100U),
                "rejected placement uses the same acknowledgement window") &&
         ok;
}

bool materialRepeatCadenceAndPrecedenceAreDeterministic() {
  cr::CreativeMaterialRepeatRequest press;
  press.secondaryPressed = true;
  press.secondaryDown = true;
  const cr::CreativeMaterialRepeatResult first =
      cr::stepCreativeMaterialRepeat({}, press);

  cr::CreativeMaterialRepeatRequest beforeDue;
  beforeDue.nowNanoseconds =
      cr::kCreativeMaterialStrokeRepeatNanoseconds - 1U;
  beforeDue.secondaryDown = true;
  const cr::CreativeMaterialRepeatResult at199 =
      cr::stepCreativeMaterialRepeat(first.next, beforeDue);

  cr::CreativeMaterialRepeatRequest due;
  due.nowNanoseconds = cr::kCreativeMaterialStrokeRepeatNanoseconds;
  due.secondaryDown = true;
  const cr::CreativeMaterialRepeatResult at200 =
      cr::stepCreativeMaterialRepeat(at199.next, due);

  cr::CreativeMaterialRepeatRequest release;
  release.nowNanoseconds =
      cr::kCreativeMaterialStrokeRepeatNanoseconds + 1U;
  release.secondaryReleased = true;
  const cr::CreativeMaterialRepeatResult ended =
      cr::stepCreativeMaterialRepeat(at200.next, release);

  cr::CreativeMaterialRepeatRequest simultaneous;
  simultaneous.primaryPressed = true;
  simultaneous.primaryDown = true;
  simultaneous.secondaryPressed = true;
  simultaneous.secondaryDown = true;
  const cr::CreativeMaterialRepeatResult primary =
      cr::stepCreativeMaterialRepeat({}, simultaneous);

  cr::CreativeMaterialRepeatRequest interrupted;
  interrupted.interrupted = true;
  const cr::CreativeMaterialRepeatResult stopped =
      cr::stepCreativeMaterialRepeat(primary.next, interrupted);

  return expect(first.began && first.mutationDue &&
                    first.dueKind == cr::CreativeMaterialStrokeKind::Place,
                "secondary begins with an immediate placement") &&
         expect(first.next.nextRepeatAtNanoseconds ==
                    cr::kCreativeMaterialStrokeRepeatNanoseconds,
                "first repeat deadline is 200 ms") &&
         expect(!at199.mutationDue, "no repeat at 199 ms") &&
         expect(at200.mutationDue &&
                    at200.dueKind == cr::CreativeMaterialStrokeKind::Place,
                "repeat is due at 200 ms") &&
         expect(ended.finalized && !ended.next.active,
                "release finalizes the gesture") &&
         expect(primary.primaryWon && primary.mutationDue &&
                    primary.dueKind == cr::CreativeMaterialStrokeKind::Remove,
                "primary wins simultaneous presses") &&
         expect(stopped.finalized && !stopped.next.active,
                "interruption finalizes active repeat state");
}

bool advancingTwoSecondHoldProducesElevenDueActions() {
  cr::CreativeMaterialRepeatRequest request;
  request.secondaryPressed = true;
  request.secondaryDown = true;
  cr::CreativeMaterialRepeatResult step =
      cr::stepCreativeMaterialRepeat({}, request);
  std::size_t dueCount = step.mutationDue ? 1U : 0U;
  cr::CreativeMaterialRepeatState state = step.next;
  for (std::uint64_t index = 1U; index <= 10U; ++index) {
    request = {};
    request.nowNanoseconds =
        index * cr::kCreativeMaterialStrokeRepeatNanoseconds;
    request.secondaryDown = true;
    step = cr::stepCreativeMaterialRepeat(state, request);
    dueCount += step.mutationDue ? 1U : 0U;
    state = step.next;
  }
  return expect(dueCount == 11U,
                "advancing two-second hold has immediate plus ten repeats") &&
         expect(state.nextRepeatAtNanoseconds ==
                    11U * cr::kCreativeMaterialStrokeRepeatNanoseconds,
                "repeat deadline advances without frame bursts");
}

bool minecraftBindingsAreConflictFreeAndEdgeTriggered() {
  const cr::CreativeInputBindingAuditResult audit =
      cr::auditCreativeInputBindings();
  const std::span<const cr::CreativeInputBinding> bindings =
      cr::defaultCreativeInputBindings();
  constexpr std::array hotbarActions{
      cr::CreativeInputActionId::HotbarSlot1,
      cr::CreativeInputActionId::HotbarSlot2,
      cr::CreativeInputActionId::HotbarSlot3,
      cr::CreativeInputActionId::HotbarSlot4,
      cr::CreativeInputActionId::HotbarSlot5,
      cr::CreativeInputActionId::HotbarSlot6,
      cr::CreativeInputActionId::HotbarSlot7,
      cr::CreativeInputActionId::HotbarSlot8,
      cr::CreativeInputActionId::HotbarSlot9,
  };
  constexpr std::array hotbarKeys{
      cr::CreativeInputKey::Digit1, cr::CreativeInputKey::Digit2,
      cr::CreativeInputKey::Digit3, cr::CreativeInputKey::Digit4,
      cr::CreativeInputKey::Digit5, cr::CreativeInputKey::Digit6,
      cr::CreativeInputKey::Digit7, cr::CreativeInputKey::Digit8,
      cr::CreativeInputKey::Digit9,
  };
  bool ok = expect(audit.conflictCount == 0U,
                   "default creative bindings are conflict free");
  for (std::size_t slot = 0; slot < hotbarActions.size(); ++slot) {
    const bool found = std::any_of(
        bindings.begin(), bindings.end(), [&](const cr::CreativeInputBinding& b) {
          return b.action == hotbarActions[slot] && b.trigger == hotbarKeys[slot] &&
                 b.requiredAllModifiers == cr::kCreativeInputModifierNone &&
                 b.requiredAnyModifiers == cr::kCreativeInputModifierNone;
        });
    ok = expect(found, "number key selects matching hotbar slot") && ok;
  }

  cr::CreativeInputRouterState state;
  cr::CreativeInputFrame frame;
  cr::setCreativeInputKey(frame, cr::CreativeInputKey::Digit9, true);
  const cr::CreativeInputRouteResult pressed =
      cr::routeCreativeInput(state, frame);
  const cr::CreativeInputRouteResult held =
      cr::routeCreativeInput(state, frame);
  cr::CreativeInputRouterState modifiedState;
  cr::CreativeInputFrame modifiedFrame;
  modifiedFrame.modifiers = cr::kCreativeInputModifierShift;
  cr::setCreativeInputKey(modifiedFrame, cr::CreativeInputKey::Digit9, true);
  cr::setCreativeInputKey(modifiedFrame, cr::CreativeInputKey::LeftShift, true);
  const cr::CreativeInputRouteResult modified =
      cr::routeCreativeInput(modifiedState, modifiedFrame);
  return expect(pressed.actionCount == 1U &&
                    pressed.actions[0].action ==
                        cr::CreativeInputActionId::HotbarSlot9,
                "number key emits matching hotbar action") &&
         expect(cr::creativeInputKeyConsumed(pressed,
                                             cr::CreativeInputKey::Digit9),
                "hotbar key consumed after handling") &&
         expect(held.actionCount == 0U,
                "held number key does not retrigger hotbar action") &&
         expect(modified.actionCount == 1U &&
                    modified.actions[0].action ==
                        cr::CreativeInputActionId::HotbarSlot9,
                "movement modifier does not block hotbar selection") &&
         expect(!cr::creativeInputKeyConsumed(
                    modified, cr::CreativeInputKey::LeftShift),
                "hotbar selection leaves flight modifier available") &&
         ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok = controllerTransitionSanitizesAndOwnsEdges() && ok;
  ok = controllerStickPrimitiveHasCanonicalDirections() && ok;
  ok = controllerMovementPreservesDirectionAndFineTravel() && ok;
  ok = worldActionsAreEdgeTriggered() && ok;
  ok = hotbarHasStableNineSlotGrammar() && ok;
  ok = heldVolumeItemsMapWithoutBranchesAtCallers() && ok;
  ok = gridTargetResolvesHitFaceAndPlacementCell() && ok;
  ok = placementFeedbackHasABoundedVisibleLifetime() && ok;
  ok = materialRepeatCadenceAndPrecedenceAreDeterministic() && ok;
  ok = advancingTwoSecondHoldProducesElevenDueActions() && ok;
  ok = minecraftBindingsAreConflictFreeAndEdgeTriggered() && ok;
  return ok ? 0 : 1;
}
