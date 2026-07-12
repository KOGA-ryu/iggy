#pragma once

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/tools/ConnectedFill.hpp"
#include "app/iggy3d/creative/tools/Pattern.hpp"
#include "app/iggy3d/creative/tools/ShapeBrush.hpp"
#include "app/iggy3d/creative/tools/SurfaceExtrude.hpp"
#include "app/iggy3d/creative/tools/TerrainProfile.hpp"
#include "app/iggy3d/creative/tools/TerrainPath.hpp"
#include "app/iggy3d/creative/tools/TerrainRegion.hpp"
#include "app/iggy3d/creative/tools/TerrainSeed.hpp"
#include "app/iggy3d/creative/tools/TerrainSculpt.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace iggy3d::creative {

enum class CreativeHeldItemKind : std::uint8_t;

enum class CreativeToolInputKind : std::uint8_t {
  Unknown,
  PointerMove,
  PointerPress,
  PointerRelease,
  Cancel,
};

enum class CreativeToolIntentKind : std::uint8_t {
  NoIntent,
  SelectObjectCandidate,
  BeginMeasurement,
  UpdateMeasurement,
  EndMeasurement,
  CancelToolAction,
  PreviewPointer,
  // Move-tool drag lifecycle (TV1-G, TD-6 preview-then-commit): a press on the
  // Move tool begins a drag, held pointer moves preview only, release commits a
  // single snapped Move mutation, cancel/escape discards with no mutation.
  BeginMove,
  PreviewMove,
  CommitMove,
  CancelMove,
};

enum class CreativeToolPointerButton : std::uint8_t {
  None,
  Primary,
  Secondary,
  Middle,
};

using CreativeToolModifierFlags = std::uint32_t;

inline constexpr CreativeToolModifierFlags kCreativeToolModifierNone = 0;
inline constexpr CreativeToolModifierFlags kCreativeToolModifierShift = 1u << 0;
inline constexpr CreativeToolModifierFlags kCreativeToolModifierControl = 1u << 1;
inline constexpr CreativeToolModifierFlags kCreativeToolModifierAlt = 1u << 2;
inline constexpr CreativeToolModifierFlags kCreativeToolModifierCommand = 1u << 3;

enum class CreativeMoveConstraint : std::uint8_t {
  Free,
  X,
  Z,
  Count,
};

enum class CreativeRotationStep : std::uint8_t {
  Degrees15,
  Degrees45,
  Degrees90,
  Count,
};

enum class CreativePlacementYaw : std::uint8_t {
  Degrees0,
  Degrees90,
  Degrees180,
  Degrees270,
  Count,
};

enum class CreativeSnapIncrement : std::uint8_t {
  QuarterMeter,
  HalfMeter,
  OneMeter,
  TwoMeters,
  Count,
};

enum class CreativeCloneOffsetAxis : std::uint8_t {
  X,
  Y,
  Z,
  Count,
};

enum class CreativeCloneOffsetDistance : std::uint8_t {
  OneCell,
  TwoCells,
  FourCells,
  EightCells,
  Count,
};

enum class CreativeArrayMode : std::uint8_t {
  Linear,
  Radial,
  Count,
};

enum class CreativeToolOptionId : std::uint8_t {
  MoveConstraint,
  RotationStep,
  PlacementYaw,
  SnapIncrement,
  MaterialBrushShape,
  MaterialBrushAxis,
  MaterialBrushSize,
  MaterialBrushFill,
  MaterialBrushGuide,
  MaterialBrushSymmetry,
  MaterialBrushMask,
  MaterialBrushReplaceSource,
  ShapeBrushKind,
  ShapeBrushAxis,
  ReplaceSource,
  CloneOffsetAxis,
  CloneOffsetDistance,
  ArrayMode,
  ArrayDirection,
  ArrayCopyCount,
  ArraySpacing,
  RadialArrayAxis,
  RadialArrayInstanceCount,
  RadialArraySweep,
  ConnectedFillLimit,
  SurfaceExtrudeDepth,
  SurfaceExtrudeLimit,
  TerrainSculptMode,
  TerrainSculptRadius,
  TerrainSculptStrength,
  TerrainSculptFalloff,
  TerrainRodStampMode,
  TerrainSeedRadius,
  TerrainSeedSpacing,
  TerrainProfileKind,
  TerrainProfileBlend,
  TerrainProfileRodPolicy,
  TerrainProfileRadius,
  TerrainProfileAmplitude,
  TerrainProfileSpacing,
  TerrainProfileDirection,
  TerrainProfileFrequency,
  TerrainPathKind,
  TerrainPathElevation,
  TerrainPathWidth,
  TerrainPathAmplitude,
  TerrainRegionOperation,
  TerrainRegionAmount,
  Count,
};

enum class CreativeToolOptionValueKind : std::uint8_t {
  Choice,
  MaterialOrAny,
};

using CreativeHeldItemMask = std::uint32_t;
inline constexpr std::size_t kCreativeToolOptionCapacity = 8;
inline constexpr std::size_t kCreativeToolOptionDescriptorCount =
    static_cast<std::size_t>(CreativeToolOptionId::Count);

struct CreativeToolOptionDescriptor {
  CreativeToolOptionId id = CreativeToolOptionId::MoveConstraint;
  std::string_view label;
  CreativeToolOptionValueKind valueKind =
      CreativeToolOptionValueKind::Choice;
  CreativeHeldItemMask applicableHeldItems = 0;
};

struct CreativeToolOptionList {
  std::array<CreativeToolOptionId, kCreativeToolOptionCapacity> ids{};
  std::size_t count = 0;
  bool capacityExceeded = false;

  [[nodiscard]] std::span<const CreativeToolOptionId> items() const noexcept {
    return {ids.data(), count};
  }
};

struct CreativeToolSettings {
  CreativeMoveConstraint moveConstraint = CreativeMoveConstraint::Free;
  CreativeRotationStep rotationStep = CreativeRotationStep::Degrees15;
  CreativePlacementYaw placementYaw = CreativePlacementYaw::Degrees0;
  CreativeSnapIncrement snapIncrement = CreativeSnapIncrement::OneMeter;
  CreativeMaterialBrushShape materialBrushShape =
      CreativeMaterialBrushShape::Sphere;
  CreativeAxis3 materialBrushAxis = CreativeAxis3::Y;
  CreativeMaterialBrushSize materialBrushSize =
      CreativeMaterialBrushSize::ThreeCells;
  CreativeMaterialBrushFill materialBrushFill =
      CreativeMaterialBrushFill::Solid;
  CreativeMaterialBrushGuide materialBrushGuide =
      CreativeMaterialBrushGuide::Free;
  CreativeMaterialBrushSymmetry materialBrushSymmetry =
      CreativeMaterialBrushSymmetry::Off;
  CreativeMaterialBrushMask materialBrushMask =
      CreativeMaterialBrushMask::Overwrite;
  CreativeObjectKind materialBrushReplaceSourceKind =
      CreativeObjectKind::Unknown;
  CreativeConnectedFillLimit connectedFillLimit =
      CreativeConnectedFillLimit::Cells256;
  CreativeSurfaceExtrudeDepth surfaceExtrudeDepth =
      CreativeSurfaceExtrudeDepth::OneCell;
  CreativeConnectedFillLimit surfaceExtrudeLimit =
      CreativeConnectedFillLimit::Cells256;
  CreativeShapeBrushKind shapeBrushKind = CreativeShapeBrushKind::Box;
  CreativeShapeBrushAxis shapeBrushAxis = CreativeShapeBrushAxis::Y;
  CreativeObjectKind replaceSourceKind = CreativeObjectKind::Unknown;
  CreativeCloneOffsetAxis cloneOffsetAxis = CreativeCloneOffsetAxis::X;
  CreativeCloneOffsetDistance cloneOffsetDistance =
      CreativeCloneOffsetDistance::OneCell;
  CreativeArrayMode arrayMode = CreativeArrayMode::Linear;
  CreativeLinearArrayDirection arrayDirection =
      CreativeLinearArrayDirection::PositiveX;
  CreativeLinearArrayCopyCount arrayCopyCount =
      CreativeLinearArrayCopyCount::Four;
  CreativeLinearArraySpacing arraySpacing =
      CreativeLinearArraySpacing::OneCell;
  CreativeAxis3 radialArrayAxis = CreativeAxis3::Y;
  CreativeRadialArrayInstanceCount radialArrayInstanceCount =
      CreativeRadialArrayInstanceCount::Eight;
  CreativeRadialArraySweep radialArraySweep =
      CreativeRadialArraySweep::Degrees360;
  CreativeTerrainSculptMode terrainSculptMode =
      CreativeTerrainSculptMode::Flatten;
  CreativeTerrainSculptRadius terrainSculptRadius =
      CreativeTerrainSculptRadius::FourCells;
  CreativeTerrainSculptStrength terrainSculptStrength =
      CreativeTerrainSculptStrength::OneCell;
  CreativeTerrainSculptFalloff terrainSculptFalloff =
      CreativeTerrainSculptFalloff::Uniform;
  CreativeTerrainRodStampMode terrainRodStampMode =
      CreativeTerrainRodStampMode::Single;
  CreativeTerrainSeedRadius terrainSeedRadius =
      CreativeTerrainSeedRadius::FourCells;
  CreativeTerrainSeedSpacing terrainSeedSpacing =
      CreativeTerrainSeedSpacing::TwoCells;
  CreativeTerrainProfileKind terrainProfileKind =
      CreativeTerrainProfileKind::Hill;
  CreativeTerrainProfileBlend terrainProfileBlend =
      CreativeTerrainProfileBlend::Set;
  CreativeTerrainProfileRodPolicy terrainProfileRodPolicy =
      CreativeTerrainProfileRodPolicy::Fill;
  CreativeTerrainProfileRadius terrainProfileRadius =
      CreativeTerrainProfileRadius::FourCells;
  CreativeTerrainProfileAmplitude terrainProfileAmplitude =
      CreativeTerrainProfileAmplitude::FourCells;
  CreativeTerrainProfileSpacing terrainProfileSpacing =
      CreativeTerrainProfileSpacing::OneCell;
  CreativeTerrainProfileDirection terrainProfileDirection =
      CreativeTerrainProfileDirection::PositiveX;
  CreativeTerrainProfileFrequency terrainProfileFrequency =
      CreativeTerrainProfileFrequency::OneCycle;
  CreativeTerrainPathKind terrainPathKind = CreativeTerrainPathKind::Road;
  CreativeTerrainPathElevation terrainPathElevation =
      CreativeTerrainPathElevation::Follow;
  CreativeTerrainPathWidth terrainPathWidth =
      CreativeTerrainPathWidth::ThreeCells;
  CreativeTerrainPathAmplitude terrainPathAmplitude =
      CreativeTerrainPathAmplitude::OneCell;
  CreativeTerrainRegionOperation terrainRegionOperation =
      CreativeTerrainRegionOperation::Raise;
  CreativeTerrainRegionAmount terrainRegionAmount =
      CreativeTerrainRegionAmount::OneCell;

  [[nodiscard]] bool operator==(
      const CreativeToolSettings&) const noexcept = default;
};

static_assert(std::is_trivially_copyable_v<CreativeToolSettings>);
static_assert(std::is_standard_layout_v<CreativeToolSettings>);

enum class CreativeToolOptionAdjustStatus : std::uint8_t {
  NotRequested,
  InvalidOption,
  InvalidSettings,
  NoAvailableValue,
  NoChange,
  Applied,
};

struct CreativeToolOptionAdjustReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeToolOptionId option = CreativeToolOptionId::MoveConstraint;
  CreativeToolOptionAdjustStatus status =
      CreativeToolOptionAdjustStatus::NotRequested;
  std::string_view reasonCode = "creative_tool_option_not_requested";
};

// Grid/world destination the window layer resolves from the pointer's grid XZ
// (the same pointer->grid-cell conversion the viewport pick uses; TD-7). Only
// the Move tool's drag Move/Release lifecycle fills it — the tool core carries
// it to the facade, which snaps it (TL-4, document snap) and commits one Move.
struct CreativeToolWorldPoint {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

// Which world axis a Move drag HOLDS (keeps at the start anchor's value) while
// the other two follow worldDestination. The CALLER picks this from its camera:
// a flat FRONT view holds Z (screen = world XY), a GROUND-PLANE editor holds Y
// (slide along the floor in XZ). Defaults to Z for the product's legacy
// front-view projection, so existing callers keep their behavior unchanged.
enum class CreativeToolMoveHeldAxis : std::uint8_t { X, Y, Z };

struct CreativeToolPointerPacket {
  double x = 0.0;
  double y = 0.0;
  CreativeToolPointerButton button = CreativeToolPointerButton::None;
  CreativeToolModifierFlags modifiers = kCreativeToolModifierNone;
  TargetRef target;
  // XZ destination for a Move drag (Y is unchanged in v1, TD-7). Filled by the
  // window for Move/Release lifecycle packets only; `hasWorldDestination` gates
  // whether the facade may commit a mutation to it.
  bool hasWorldDestination = false;
  CreativeToolWorldPoint worldDestination;
  // The axis the Move drag holds at the start anchor (default Z = front-view).
  CreativeToolMoveHeldAxis moveHeldAxis = CreativeToolMoveHeldAxis::Z;
  CreativeMoveConstraint moveConstraint = CreativeMoveConstraint::Free;
  bool hasMoveSnapStepOverride = false;
  double moveSnapStepOverride = 1.0;
};

struct CreativeToolInputPacket {
  CreativeToolInputKind kind = CreativeToolInputKind::Unknown;
  CreativeToolPointerPacket pointer;
};

struct CreativeToolState {
  Tool activeTool = Tool::Select;
  CreativeToolPointerPacket pointer;
  bool measurementActive = false;
  // Move-tool drag in flight (TV1-G). Set on a Move-tool press, cleared on
  // release/cancel/tool-switch. `moveDragTarget` is the picked object from the
  // press (the facade falls back to the current selection when it is invalid).
  bool moveDragActive = false;
  TargetRef moveDragTarget;
};

struct CreativeToolIntent {
  CreativeToolIntentKind kind = CreativeToolIntentKind::NoIntent;
  Tool tool = Tool::Select;
  CreativeToolPointerPacket pointer;
};

using CreativeToolIntentList = std::vector<CreativeToolIntent>;

struct CreativeToolDispatchReceipt {
  Tool activeToolBefore = Tool::Select;
  Tool activeToolAfter = Tool::Select;
  CreativeToolInputKind inputKind = CreativeToolInputKind::Unknown;
  std::size_t emittedIntentCount = 0;
  bool changedState = false;
  bool accepted = false;
  std::string_view message = "unsupported_input";
  CreativeToolIntentList intents;
};

[[nodiscard]] CreativeToolState makeDefaultCreativeToolState() noexcept;
[[nodiscard]] bool setActiveTool(CreativeToolState& state,
                                 Tool tool) noexcept;
[[nodiscard]] CreativeToolDispatchReceipt dispatchToolInput(
    CreativeToolState& state,
    const CreativeToolInputPacket& input);

[[nodiscard]] CreativeToolSettings makeDefaultCreativeToolSettings() noexcept;
[[nodiscard]] bool isValidCreativeToolSettings(
    const CreativeToolSettings& settings) noexcept;
[[nodiscard]] std::span<const CreativeToolOptionDescriptor>
creativeToolOptionDescriptors() noexcept;
[[nodiscard]] const CreativeToolOptionDescriptor* creativeToolOptionDescriptor(
    CreativeToolOptionId option) noexcept;
[[nodiscard]] CreativeToolOptionList creativeToolOptionsForHeldItem(
    CreativeHeldItemKind heldItem) noexcept;
[[nodiscard]] CreativeToolOptionList creativeToolOptionsForHeldItem(
    CreativeHeldItemKind heldItem,
    const CreativeToolSettings& settings) noexcept;
[[nodiscard]] bool creativeToolOptionAppliesToHeldItem(
    CreativeToolOptionId option,
    CreativeHeldItemKind heldItem) noexcept;
[[nodiscard]] bool creativeMaterialBrushPaintAllows(
    CreativeMaterialBrushMask mask,
    CreativeObjectKind currentMaterial,
    CreativeObjectKind replaceSource) noexcept;

[[nodiscard]] std::string_view toString(
    CreativeMoveConstraint constraint) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeRotationStep step) noexcept;
[[nodiscard]] std::string_view toString(
    CreativePlacementYaw yaw) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeSnapIncrement increment) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeCloneOffsetAxis axis) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeCloneOffsetDistance distance) noexcept;
[[nodiscard]] std::string_view toString(CreativeArrayMode mode) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeToolOptionAdjustStatus status) noexcept;
[[nodiscard]] std::string_view creativeToolOptionValueLabel(
    const CreativeToolSettings& settings,
    CreativeToolOptionId option) noexcept;

// O(option count + palette size), both caller-bounded. Adjustment is atomic:
// invalid settings or unavailable values leave the input settings unchanged.
[[nodiscard]] CreativeToolOptionAdjustReceipt adjustCreativeToolOption(
    CreativeToolSettings& settings,
    CreativeToolOptionId option,
    std::int32_t direction,
    std::span<const CreativeObjectKind> materialPalette = {}) noexcept;

[[nodiscard]] double creativeRotationStepDegrees(
    CreativeRotationStep step) noexcept;
[[nodiscard]] double creativePlacementYawRadians(
    CreativePlacementYaw yaw) noexcept;
[[nodiscard]] double creativeSnapIncrementMeters(
    CreativeSnapIncrement increment) noexcept;
[[nodiscard]] bool tryCreativeCloneOffset(
    const CreativeToolSettings& settings,
    double cellSize,
    CreativeToolWorldPoint& output) noexcept;

}  // namespace iggy3d::creative
