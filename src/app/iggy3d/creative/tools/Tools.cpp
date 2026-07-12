#include "app/iggy3d/creative/tools/Tools.hpp"

#include <array>
#include <cmath>

#include "app/iggy3d/creative/input/Interaction.hpp"
#include "app/iggy3d/creative/tools/Volume.hpp"

namespace iggy3d::creative {
namespace {

[[nodiscard]] constexpr bool validReplaceSourceKind(
    CreativeObjectKind kind) noexcept {
  return kind > CreativeObjectKind::Unknown &&
         kind < CreativeObjectKind::Count;
}

[[nodiscard]] bool samePointer(const CreativeToolPointerPacket& lhs,
                               const CreativeToolPointerPacket& rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.button == rhs.button &&
         lhs.modifiers == rhs.modifiers && lhs.target.value == rhs.target.value &&
         lhs.hasWorldDestination == rhs.hasWorldDestination &&
         lhs.worldDestination.x == rhs.worldDestination.x &&
         lhs.worldDestination.y == rhs.worldDestination.y &&
         lhs.worldDestination.z == rhs.worldDestination.z &&
         lhs.moveHeldAxis == rhs.moveHeldAxis &&
         lhs.moveConstraint == rhs.moveConstraint &&
         lhs.hasMoveSnapStepOverride == rhs.hasMoveSnapStepOverride &&
         lhs.moveSnapStepOverride == rhs.moveSnapStepOverride;
}

void updatePointer(CreativeToolState& state,
                   const CreativeToolPointerPacket& pointer,
                   bool& changedState) noexcept {
  if (!samePointer(state.pointer, pointer)) {
    state.pointer = pointer;
    changedState = true;
  }
}

[[nodiscard]] CreativeToolDispatchReceipt makeReceipt(
    Tool activeToolBefore,
    CreativeToolInputKind inputKind) {
  CreativeToolDispatchReceipt receipt;
  receipt.activeToolBefore = activeToolBefore;
  receipt.activeToolAfter = activeToolBefore;
  receipt.inputKind = inputKind;
  return receipt;
}

void emitIntent(CreativeToolDispatchReceipt& receipt,
                CreativeToolIntentKind kind,
                Tool tool,
                const CreativeToolPointerPacket& pointer) {
  receipt.intents.push_back(CreativeToolIntent{kind, tool, pointer});
  receipt.emittedIntentCount = receipt.intents.size();
}

[[nodiscard]] constexpr CreativeHeldItemMask heldItemMask(
    CreativeHeldItemKind heldItem) noexcept {
  return static_cast<CreativeHeldItemMask>(
      1U << static_cast<unsigned>(heldItem));
}
static_assert(kCreativeHeldItemKindCount <=
              sizeof(CreativeHeldItemMask) * 8U);

constexpr CreativeHeldItemMask kMoveItems =
    heldItemMask(CreativeHeldItemKind::ObjectMove);
constexpr CreativeHeldItemMask kRotationItems =
    heldItemMask(CreativeHeldItemKind::ObjectSelect) |
    heldItemMask(CreativeHeldItemKind::ObjectMove);
constexpr CreativeHeldItemMask kPlacementItems =
    heldItemMask(CreativeHeldItemKind::Material);
constexpr CreativeHeldItemMask kMaterialBrushItems =
    heldItemMask(CreativeHeldItemKind::MaterialBrush);
constexpr CreativeHeldItemMask kConnectedFillItems =
    heldItemMask(CreativeHeldItemKind::ConnectedFill);
constexpr CreativeHeldItemMask kSurfaceExtrudeItems =
    heldItemMask(CreativeHeldItemKind::SurfaceExtrude);
constexpr CreativeHeldItemMask kTerrainSculptItems =
    heldItemMask(CreativeHeldItemKind::TerrainSculpt);
constexpr CreativeHeldItemMask kTerrainControlItems =
    heldItemMask(CreativeHeldItemKind::TerrainControl);
constexpr CreativeHeldItemMask kTerrainProfileItems =
    heldItemMask(CreativeHeldItemKind::TerrainProfile);
constexpr CreativeHeldItemMask kTerrainPathItems =
    heldItemMask(CreativeHeldItemKind::TerrainPath);
constexpr CreativeHeldItemMask kTerrainRegionItems =
    heldItemMask(CreativeHeldItemKind::TerrainRegion);
constexpr CreativeHeldItemMask kSnapItems =
    heldItemMask(CreativeHeldItemKind::Material) |
    heldItemMask(CreativeHeldItemKind::ObjectMove) |
    heldItemMask(CreativeHeldItemKind::VolumeSelect) |
    heldItemMask(CreativeHeldItemKind::VolumeFill) |
    heldItemMask(CreativeHeldItemKind::VolumeHollow) |
    heldItemMask(CreativeHeldItemKind::VolumeReplace) |
    heldItemMask(CreativeHeldItemKind::VolumeErase) |
    heldItemMask(CreativeHeldItemKind::VolumeClone);
constexpr CreativeHeldItemMask kReplaceItems =
    heldItemMask(CreativeHeldItemKind::VolumeReplace);
constexpr CreativeHeldItemMask kShapeItems =
    heldItemMask(CreativeHeldItemKind::VolumeFill) |
    heldItemMask(CreativeHeldItemKind::VolumeHollow);
constexpr CreativeHeldItemMask kCloneItems =
    heldItemMask(CreativeHeldItemKind::VolumeClone);
constexpr CreativeHeldItemMask kArrayItems =
    heldItemMask(CreativeHeldItemKind::LinearArray);

constexpr std::array kToolOptionDescriptors{
    CreativeToolOptionDescriptor{CreativeToolOptionId::MoveConstraint,
                                 "MOVE AXIS",
                                 CreativeToolOptionValueKind::Choice,
                                 kMoveItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::RotationStep,
                                 "ROTATE STEP",
                                 CreativeToolOptionValueKind::Choice,
                                 kRotationItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::PlacementYaw,
                                 "ORIENTATION",
                                 CreativeToolOptionValueKind::Choice,
                                 kPlacementItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::SnapIncrement,
                                 "GRID SIZE",
                                 CreativeToolOptionValueKind::Choice,
                                 kSnapItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::MaterialBrushShape,
                                 "BRUSH SHAPE",
                                 CreativeToolOptionValueKind::Choice,
                                 kMaterialBrushItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::MaterialBrushAxis,
                                 "CYLINDER AXIS",
                                 CreativeToolOptionValueKind::Choice,
                                 kMaterialBrushItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::MaterialBrushSize,
                                 "BRUSH SIZE",
                                 CreativeToolOptionValueKind::Choice,
                                 kMaterialBrushItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::MaterialBrushFill,
                                 "BRUSH BODY",
                                 CreativeToolOptionValueKind::Choice,
                                 kMaterialBrushItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::MaterialBrushGuide,
                                 "BRUSH GUIDE",
                                 CreativeToolOptionValueKind::Choice,
                                 kMaterialBrushItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::MaterialBrushSymmetry,
                                 "SYMMETRY",
                                 CreativeToolOptionValueKind::Choice,
                                 kMaterialBrushItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::MaterialBrushMask,
                                 "BRUSH MASK",
                                 CreativeToolOptionValueKind::Choice,
                                 kMaterialBrushItems},
    CreativeToolOptionDescriptor{
        CreativeToolOptionId::MaterialBrushReplaceSource,
        "REPLACE SOURCE",
        CreativeToolOptionValueKind::MaterialOrAny,
        kMaterialBrushItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::ShapeBrushKind,
                                 "SHAPE",
                                 CreativeToolOptionValueKind::Choice,
                                 kShapeItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::ShapeBrushAxis,
                                 "SHAPE AXIS",
                                 CreativeToolOptionValueKind::Choice,
                                 kShapeItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::ReplaceSource,
                                 "REPLACE SOURCE",
                                 CreativeToolOptionValueKind::MaterialOrAny,
                                 kReplaceItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::CloneOffsetAxis,
                                 "CLONE AXIS",
                                 CreativeToolOptionValueKind::Choice,
                                 kCloneItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::CloneOffsetDistance,
                                 "CLONE DIST",
                                 CreativeToolOptionValueKind::Choice,
                                 kCloneItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::ArrayMode,
                                 "MODE",
                                 CreativeToolOptionValueKind::Choice,
                                 kArrayItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::ArrayDirection,
                                 "DIRECTION",
                                 CreativeToolOptionValueKind::Choice,
                                 kArrayItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::ArrayCopyCount,
                                 "COPIES",
                                 CreativeToolOptionValueKind::Choice,
                                 kArrayItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::ArraySpacing,
                                 "STEP",
                                 CreativeToolOptionValueKind::Choice,
                                 kArrayItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::RadialArrayAxis,
                                 "AXIS",
                                 CreativeToolOptionValueKind::Choice,
                                 kArrayItems},
    CreativeToolOptionDescriptor{
        CreativeToolOptionId::RadialArrayInstanceCount,
        "INSTANCES",
        CreativeToolOptionValueKind::Choice,
        kArrayItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::RadialArraySweep,
                                 "SWEEP",
                                 CreativeToolOptionValueKind::Choice,
                                 kArrayItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::ConnectedFillLimit,
                                 "FILL LIMIT",
                                 CreativeToolOptionValueKind::Choice,
                                 kConnectedFillItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::SurfaceExtrudeDepth,
                                 "DEPTH",
                                 CreativeToolOptionValueKind::Choice,
                                 kSurfaceExtrudeItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::SurfaceExtrudeLimit,
                                 "AFFECT LIMIT",
                                 CreativeToolOptionValueKind::Choice,
                                 kSurfaceExtrudeItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainSculptMode,
                                 "SCULPT MODE",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainSculptItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainSculptRadius,
                                 "SCULPT RADIUS",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainSculptItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainSculptStrength,
                                 "SCULPT STRENGTH",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainSculptItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainSculptFalloff,
                                 "FALLOFF",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainSculptItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainRodStampMode,
                                 "ROD STAMP",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainControlItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainSeedRadius,
                                 "SEED RADIUS",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainControlItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainSeedSpacing,
                                 "SEED SPACING",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainControlItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainProfileKind,
                                 "PROFILE",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainProfileItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainProfileBlend,
                                 "BLEND",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainProfileItems},
    CreativeToolOptionDescriptor{
        CreativeToolOptionId::TerrainProfileRodPolicy,
        "ROD POLICY",
        CreativeToolOptionValueKind::Choice,
        kTerrainProfileItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainProfileRadius,
                                 "RADIUS",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainProfileItems},
    CreativeToolOptionDescriptor{
        CreativeToolOptionId::TerrainProfileAmplitude,
        "AMPLITUDE",
        CreativeToolOptionValueKind::Choice,
        kTerrainProfileItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainProfileSpacing,
                                 "SPACING",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainProfileItems},
    CreativeToolOptionDescriptor{
        CreativeToolOptionId::TerrainProfileDirection,
        "DIRECTION",
        CreativeToolOptionValueKind::Choice,
        kTerrainProfileItems},
    CreativeToolOptionDescriptor{
        CreativeToolOptionId::TerrainProfileFrequency,
        "FREQUENCY",
        CreativeToolOptionValueKind::Choice,
        kTerrainProfileItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainPathKind,
                                 "PATH TYPE",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainPathItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainPathElevation,
                                 "ELEVATION",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainPathItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainPathWidth,
                                 "WIDTH",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainPathItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainPathAmplitude,
                                 "RISE / DEPTH",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainPathItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainRegionOperation,
                                 "OPERATION",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainRegionItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainRegionAmount,
                                 "AMOUNT",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainRegionItems},
};
static_assert(kToolOptionDescriptors.size() ==
              kCreativeToolOptionDescriptorCount);

template <typename Enum>
[[nodiscard]] bool validEnum(Enum value, Enum count) noexcept {
  return static_cast<std::size_t>(value) <
         static_cast<std::size_t>(count);
}

template <typename Enum>
[[nodiscard]] Enum cycleEnum(Enum value, Enum count,
                             std::int32_t direction) noexcept {
  const std::size_t size = static_cast<std::size_t>(count);
  const std::size_t current = static_cast<std::size_t>(value);
  const std::size_t next = direction > 0
                               ? (current + 1U) % size
                               : (current + size - 1U) % size;
  return static_cast<Enum>(next);
}

[[nodiscard]] bool sameSettings(const CreativeToolSettings& lhs,
                                const CreativeToolSettings& rhs) noexcept {
  return lhs == rhs;
}

[[nodiscard]] CreativeToolOptionAdjustReceipt adjustReceipt(
    CreativeToolOptionId option) noexcept {
  CreativeToolOptionAdjustReceipt receipt;
  receipt.requested = true;
  receipt.option = option;
  return receipt;
}

[[nodiscard]] bool nextReplaceSource(
    std::span<const CreativeObjectKind> palette,
    CreativeObjectKind current,
    std::int32_t direction,
    CreativeObjectKind& output) noexcept {
  const std::size_t candidateCount = palette.size() + 1U;
  std::size_t currentIndex = 0U;
  if (current != CreativeObjectKind::Unknown) {
    for (std::size_t index = 0; index < palette.size(); ++index) {
      if (palette[index] == current) {
        currentIndex = index + 1U;
        break;
      }
    }
  }

  std::size_t candidateIndex = currentIndex;
  for (std::size_t attempt = 0; attempt < candidateCount; ++attempt) {
    candidateIndex = direction > 0
                         ? (candidateIndex + 1U) % candidateCount
                         : (candidateIndex + candidateCount - 1U) %
                               candidateCount;
    if (candidateIndex == 0U) {
      if (current != CreativeObjectKind::Unknown) {
        output = CreativeObjectKind::Unknown;
        return true;
      }
      continue;
    }
    const CreativeObjectKind candidate = palette[candidateIndex - 1U];
    if (candidate != current && validReplaceSourceKind(candidate)) {
      output = candidate;
      return true;
    }
  }
  return false;
}

}  // namespace

CreativeToolState makeDefaultCreativeToolState() noexcept {
  return {};
}

bool setActiveTool(CreativeToolState& state, Tool tool) noexcept {
  if (state.activeTool == tool) {
    return false;
  }

  state.activeTool = tool;
  state.measurementActive = false;
  // A tool switch abandons any Move drag in flight so a stranded drag cannot
  // commit into the newly-selected tool (mirrors the pointer-lifecycle reset).
  state.moveDragActive = false;
  state.moveDragTarget = {};
  return true;
}

[[nodiscard]] bool nextMaterialBrushReplaceSource(
    std::span<const CreativeObjectKind> palette,
    CreativeObjectKind current,
    std::int32_t direction,
    CreativeObjectKind& output) noexcept {
  std::size_t materialCount = 0U;
  for (CreativeObjectKind kind : palette) {
    if (creativeVolumeBrushSupported(kind)) {
      ++materialCount;
    }
  }
  if (materialCount == 0U) {
    return false;
  }

  std::size_t currentIndex = 0U;
  if (current != CreativeObjectKind::Unknown) {
    std::size_t materialIndex = 0U;
    for (CreativeObjectKind kind : palette) {
      if (!creativeVolumeBrushSupported(kind)) {
        continue;
      }
      ++materialIndex;
      if (kind == current) {
        currentIndex = materialIndex;
        break;
      }
    }
  }

  const std::size_t candidateCount = materialCount + 1U;
  const std::size_t nextIndex =
      direction > 0 ? (currentIndex + 1U) % candidateCount
                    : (currentIndex + candidateCount - 1U) % candidateCount;
  if (nextIndex == 0U) {
    output = CreativeObjectKind::Unknown;
    return true;
  }
  std::size_t materialIndex = 0U;
  for (CreativeObjectKind kind : palette) {
    if (!creativeVolumeBrushSupported(kind)) {
      continue;
    }
    ++materialIndex;
    if (materialIndex == nextIndex) {
      output = kind;
      return true;
    }
  }
  return false;
}

CreativeToolDispatchReceipt dispatchToolInput(
    CreativeToolState& state,
    const CreativeToolInputPacket& input) {
  const Tool activeToolBefore = state.activeTool;
  CreativeToolDispatchReceipt receipt =
      makeReceipt(activeToolBefore, input.kind);

  switch (input.kind) {
    case CreativeToolInputKind::PointerMove:
      receipt.accepted = true;
      if (state.activeTool == Tool::Navigate) {
        // Navigate never touches the document; the camera arrives in TV1-H.
        receipt.message = "navigate_pointer_inert";
        break;
      }
      updatePointer(state, input.pointer, receipt.changedState);
      if (state.activeTool == Tool::Measure) {
        emitIntent(receipt,
                   CreativeToolIntentKind::UpdateMeasurement,
                   state.activeTool,
                   input.pointer);
        receipt.message = "measurement_update";
      } else if (state.activeTool == Tool::Move && state.moveDragActive) {
        // TD-6: a held Move drag previews only — no mutation until Release.
        emitIntent(receipt,
                   CreativeToolIntentKind::PreviewMove,
                   state.activeTool,
                   input.pointer);
        receipt.message = "move_preview";
      } else {
        emitIntent(receipt,
                   CreativeToolIntentKind::PreviewPointer,
                   state.activeTool,
                   input.pointer);
        receipt.message = "preview_pointer";
      }
      break;

    case CreativeToolInputKind::PointerPress:
      receipt.accepted = true;
      if (state.activeTool == Tool::Navigate) {
        // Navigate never touches the document; the camera arrives in TV1-H.
        receipt.message = "navigate_pointer_inert";
        break;
      }
      updatePointer(state, input.pointer, receipt.changedState);
      switch (state.activeTool) {
        case Tool::Select:
          emitIntent(receipt,
                     CreativeToolIntentKind::SelectObjectCandidate,
                     state.activeTool,
                     input.pointer);
          receipt.message = "select_object_candidate";
          break;
        case Tool::Move:
          // Move press selects (TV1-C) AND begins a drag (TV1-G). The picked
          // target seeds the drag; the facade falls back to the current
          // selection when the press missed a specific object.
          emitIntent(receipt,
                     CreativeToolIntentKind::SelectObjectCandidate,
                     state.activeTool,
                     input.pointer);
          if ((input.pointer.modifiers & kCreativeToolModifierShift) != 0U) {
            receipt.message = "move_additive_selection";
            break;
          }
          state.moveDragActive = true;
          state.moveDragTarget = input.pointer.target;
          receipt.changedState = true;
          emitIntent(receipt,
                     CreativeToolIntentKind::BeginMove,
                     state.activeTool,
                     input.pointer);
          receipt.message = "move_drag_begin";
          break;
        case Tool::Measure:
          if (!state.measurementActive) {
            state.measurementActive = true;
            receipt.changedState = true;
          }
          emitIntent(receipt,
                     CreativeToolIntentKind::BeginMeasurement,
                     state.activeTool,
                     input.pointer);
          receipt.message = "begin_measurement";
          break;
        case Tool::Navigate:
          break;
      }
      break;

    case CreativeToolInputKind::PointerRelease:
      receipt.accepted = true;
      if (state.activeTool == Tool::Navigate) {
        // Navigate never touches the document; the camera arrives in TV1-H.
        receipt.message = "navigate_pointer_inert";
        break;
      }
      updatePointer(state, input.pointer, receipt.changedState);
      if (state.activeTool == Tool::Measure && state.measurementActive) {
        state.measurementActive = false;
        receipt.changedState = true;
        emitIntent(receipt,
                   CreativeToolIntentKind::EndMeasurement,
                   state.activeTool,
                   input.pointer);
        receipt.message = "end_measurement";
      } else if (state.activeTool == Tool::Move && state.moveDragActive) {
        // TD-6: release ends the drag and commits exactly one snapped Move.
        // (TV1-F entry req ii: a Release with no active drag falls through to
        // no_intent below — a harmless no-op, never a spurious move.)
        state.moveDragActive = false;
        state.moveDragTarget = {};
        receipt.changedState = true;
        emitIntent(receipt,
                   CreativeToolIntentKind::CommitMove,
                   state.activeTool,
                   input.pointer);
        receipt.message = "move_drag_commit";
      } else {
        receipt.message = "no_intent";
      }
      break;

    case CreativeToolInputKind::Cancel:
      receipt.accepted = true;
      if (state.moveDragActive) {
        // TD-6: Esc/Cancel mid-drag discards the drag with NO mutation.
        state.moveDragActive = false;
        state.moveDragTarget = {};
        receipt.changedState = true;
        emitIntent(receipt,
                   CreativeToolIntentKind::CancelMove,
                   state.activeTool,
                   input.pointer);
        receipt.message = "move_drag_cancel";
      } else if (state.measurementActive) {
        state.measurementActive = false;
        receipt.changedState = true;
        emitIntent(receipt,
                   CreativeToolIntentKind::CancelToolAction,
                   state.activeTool,
                   input.pointer);
        receipt.message = "cancel_tool_action";
      } else {
        receipt.message = "no_intent";
      }
      break;

    case CreativeToolInputKind::Unknown:
      receipt.message = "unsupported_input";
      break;
  }

  receipt.activeToolAfter = state.activeTool;
  receipt.emittedIntentCount = receipt.intents.size();
  return receipt;
}

CreativeToolSettings makeDefaultCreativeToolSettings() noexcept {
  return {};
}

bool isValidCreativeToolSettings(
    const CreativeToolSettings& settings) noexcept {
  const bool replaceSourceValid =
      settings.replaceSourceKind == CreativeObjectKind::Unknown ||
      validReplaceSourceKind(settings.replaceSourceKind);
  const bool materialBrushReplaceSourceValid =
      settings.materialBrushReplaceSourceKind == CreativeObjectKind::Unknown ||
      creativeVolumeBrushSupported(
          settings.materialBrushReplaceSourceKind);
  return validEnum(settings.moveConstraint, CreativeMoveConstraint::Count) &&
         validEnum(settings.rotationStep, CreativeRotationStep::Count) &&
         validEnum(settings.placementYaw, CreativePlacementYaw::Count) &&
         validEnum(settings.snapIncrement, CreativeSnapIncrement::Count) &&
         validEnum(settings.materialBrushShape,
                   CreativeMaterialBrushShape::Count) &&
         isValidCreativeAxis3(settings.materialBrushAxis) &&
         validEnum(settings.materialBrushSize,
                   CreativeMaterialBrushSize::Count) &&
         validEnum(settings.materialBrushFill,
                   CreativeMaterialBrushFill::Count) &&
         validEnum(settings.materialBrushGuide,
                   CreativeMaterialBrushGuide::Count) &&
         validEnum(settings.materialBrushSymmetry,
                   CreativeMaterialBrushSymmetry::Count) &&
         validEnum(settings.materialBrushMask,
                   CreativeMaterialBrushMask::Count) &&
         materialBrushReplaceSourceValid &&
         validEnum(settings.connectedFillLimit,
                   CreativeConnectedFillLimit::Count) &&
         validEnum(settings.surfaceExtrudeDepth,
                   CreativeSurfaceExtrudeDepth::Count) &&
         validEnum(settings.surfaceExtrudeLimit,
                   CreativeConnectedFillLimit::Count) &&
         validEnum(settings.shapeBrushKind, CreativeShapeBrushKind::Count) &&
         validEnum(settings.shapeBrushAxis, CreativeShapeBrushAxis::Count) &&
         replaceSourceValid &&
         validEnum(settings.cloneOffsetAxis,
                   CreativeCloneOffsetAxis::Count) &&
         validEnum(settings.cloneOffsetDistance,
                   CreativeCloneOffsetDistance::Count) &&
         validEnum(settings.arrayMode, CreativeArrayMode::Count) &&
         validEnum(settings.arrayDirection,
                   CreativeLinearArrayDirection::Count) &&
         validEnum(settings.arrayCopyCount,
                   CreativeLinearArrayCopyCount::Count) &&
         validEnum(settings.arraySpacing,
                   CreativeLinearArraySpacing::Count) &&
         isValidCreativeAxis3(settings.radialArrayAxis) &&
         validEnum(settings.radialArrayInstanceCount,
                   CreativeRadialArrayInstanceCount::Count) &&
         validEnum(settings.radialArraySweep,
                   CreativeRadialArraySweep::Count) &&
         validEnum(settings.terrainSculptMode,
                   CreativeTerrainSculptMode::Count) &&
         validEnum(settings.terrainSculptRadius,
                   CreativeTerrainSculptRadius::Count) &&
         validEnum(settings.terrainSculptStrength,
                   CreativeTerrainSculptStrength::Count) &&
         validEnum(settings.terrainSculptFalloff,
                   CreativeTerrainSculptFalloff::Count) &&
         validEnum(settings.terrainRodStampMode,
                   CreativeTerrainRodStampMode::Count) &&
         validEnum(settings.terrainSeedRadius,
                   CreativeTerrainSeedRadius::Count) &&
         validEnum(settings.terrainSeedSpacing,
                   CreativeTerrainSeedSpacing::Count) &&
         validEnum(settings.terrainProfileKind,
                   CreativeTerrainProfileKind::Count) &&
         validEnum(settings.terrainProfileBlend,
                   CreativeTerrainProfileBlend::Count) &&
         validEnum(settings.terrainProfileRodPolicy,
                   CreativeTerrainProfileRodPolicy::Count) &&
         validEnum(settings.terrainProfileRadius,
                   CreativeTerrainProfileRadius::Count) &&
         validEnum(settings.terrainProfileAmplitude,
                   CreativeTerrainProfileAmplitude::Count) &&
         validEnum(settings.terrainProfileSpacing,
                   CreativeTerrainProfileSpacing::Count) &&
         validEnum(settings.terrainProfileDirection,
                   CreativeTerrainProfileDirection::Count) &&
         validEnum(settings.terrainProfileFrequency,
                   CreativeTerrainProfileFrequency::Count) &&
         validEnum(settings.terrainPathKind, CreativeTerrainPathKind::Count) &&
         validEnum(settings.terrainPathElevation,
                   CreativeTerrainPathElevation::Count) &&
         validEnum(settings.terrainPathWidth,
                   CreativeTerrainPathWidth::Count) &&
         validEnum(settings.terrainPathAmplitude,
                   CreativeTerrainPathAmplitude::Count) &&
         validEnum(settings.terrainRegionOperation,
                   CreativeTerrainRegionOperation::Count) &&
         validEnum(settings.terrainRegionAmount,
                   CreativeTerrainRegionAmount::Count);
}

std::span<const CreativeToolOptionDescriptor>
creativeToolOptionDescriptors() noexcept {
  return kToolOptionDescriptors;
}

const CreativeToolOptionDescriptor* creativeToolOptionDescriptor(
    CreativeToolOptionId option) noexcept {
  const std::size_t index = static_cast<std::size_t>(option);
  return index < kToolOptionDescriptors.size()
             ? &kToolOptionDescriptors[index]
             : nullptr;
}

CreativeToolOptionList creativeToolOptionsForHeldItem(
    CreativeHeldItemKind heldItem) noexcept {
  return creativeToolOptionsForHeldItem(heldItem,
                                        makeDefaultCreativeToolSettings());
}

CreativeToolOptionList creativeToolOptionsForHeldItem(
    CreativeHeldItemKind heldItem,
    const CreativeToolSettings& settings) noexcept {
  CreativeToolOptionList result;
  if (static_cast<std::size_t>(heldItem) >=
          static_cast<std::size_t>(CreativeHeldItemKind::Count) ||
      !isValidCreativeToolSettings(settings)) {
    return result;
  }
  const CreativeHeldItemMask mask = heldItemMask(heldItem);
  for (const CreativeToolOptionDescriptor& descriptor :
       kToolOptionDescriptors) {
    if ((descriptor.applicableHeldItems & mask) == 0U) {
      continue;
    }
    const bool linearOnly =
        descriptor.id == CreativeToolOptionId::ArrayDirection ||
        descriptor.id == CreativeToolOptionId::ArrayCopyCount ||
        descriptor.id == CreativeToolOptionId::ArraySpacing;
    const bool radialOnly =
        descriptor.id == CreativeToolOptionId::RadialArrayAxis ||
        descriptor.id == CreativeToolOptionId::RadialArrayInstanceCount ||
        descriptor.id == CreativeToolOptionId::RadialArraySweep;
    const bool cylinderOnly =
        descriptor.id == CreativeToolOptionId::MaterialBrushAxis;
    const bool materialBrushReplaceOnly =
        descriptor.id == CreativeToolOptionId::MaterialBrushReplaceSource;
    const bool terrainSeedOnly =
        descriptor.id == CreativeToolOptionId::TerrainSeedRadius ||
        descriptor.id == CreativeToolOptionId::TerrainSeedSpacing;
    const bool terrainProfileSpacingOnly =
        descriptor.id == CreativeToolOptionId::TerrainProfileSpacing;
    const bool terrainProfileDirectionOnly =
        descriptor.id == CreativeToolOptionId::TerrainProfileDirection;
    const bool terrainProfileFrequencyOnly =
        descriptor.id == CreativeToolOptionId::TerrainProfileFrequency;
    const bool terrainRegionAmountOnly =
        descriptor.id == CreativeToolOptionId::TerrainRegionAmount;
    if ((linearOnly && settings.arrayMode != CreativeArrayMode::Linear) ||
        (radialOnly && settings.arrayMode != CreativeArrayMode::Radial) ||
        (cylinderOnly && settings.materialBrushShape !=
                             CreativeMaterialBrushShape::Cylinder) ||
        (materialBrushReplaceOnly && settings.materialBrushMask !=
                                         CreativeMaterialBrushMask::Replace) ||
        (terrainSeedOnly && settings.terrainRodStampMode !=
                                CreativeTerrainRodStampMode::Seed) ||
        (terrainProfileSpacingOnly &&
         settings.terrainProfileRodPolicy !=
             CreativeTerrainProfileRodPolicy::Fill) ||
        (terrainProfileDirectionOnly &&
         !creativeTerrainProfileUsesDirection(settings.terrainProfileKind)) ||
        (terrainProfileFrequencyOnly &&
         !creativeTerrainProfileUsesFrequency(settings.terrainProfileKind)) ||
        (terrainRegionAmountOnly &&
         !creativeTerrainRegionUsesAmount(
             settings.terrainRegionOperation))) {
      continue;
    }
    if (result.count == result.ids.size()) {
      result.capacityExceeded = true;
      continue;
    }
    result.ids[result.count++] = descriptor.id;
  }
  return result;
}

bool creativeToolOptionAppliesToHeldItem(
    CreativeToolOptionId option,
    CreativeHeldItemKind heldItem) noexcept {
  const CreativeToolOptionDescriptor* descriptor =
      creativeToolOptionDescriptor(option);
  if (descriptor == nullptr ||
      static_cast<std::size_t>(heldItem) >=
          static_cast<std::size_t>(CreativeHeldItemKind::Count)) {
    return false;
  }
  return (descriptor->applicableHeldItems & heldItemMask(heldItem)) != 0U;
}

bool creativeMaterialBrushPaintAllows(
    CreativeMaterialBrushMask mask,
    CreativeObjectKind currentMaterial,
    CreativeObjectKind replaceSource) noexcept {
  const bool currentValid = currentMaterial == CreativeObjectKind::Unknown ||
                            creativeVolumeBrushSupported(currentMaterial);
  const bool sourceValid = replaceSource == CreativeObjectKind::Unknown ||
                           creativeVolumeBrushSupported(replaceSource);
  if (!currentValid || !sourceValid) {
    return false;
  }
  switch (mask) {
    case CreativeMaterialBrushMask::AddOnly:
      return currentMaterial == CreativeObjectKind::Unknown;
    case CreativeMaterialBrushMask::Replace:
      return currentMaterial != CreativeObjectKind::Unknown &&
             (replaceSource == CreativeObjectKind::Unknown ||
              currentMaterial == replaceSource);
    case CreativeMaterialBrushMask::Overwrite:
      return true;
    case CreativeMaterialBrushMask::Count:
      return false;
  }
  return false;
}

std::string_view toString(CreativeMoveConstraint constraint) noexcept {
  switch (constraint) {
    case CreativeMoveConstraint::Free: return "FREE";
    case CreativeMoveConstraint::X: return "X";
    case CreativeMoveConstraint::Z: return "Z";
    case CreativeMoveConstraint::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeRotationStep step) noexcept {
  switch (step) {
    case CreativeRotationStep::Degrees15: return "15 DEG";
    case CreativeRotationStep::Degrees45: return "45 DEG";
    case CreativeRotationStep::Degrees90: return "90 DEG";
    case CreativeRotationStep::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativePlacementYaw yaw) noexcept {
  switch (yaw) {
    case CreativePlacementYaw::Degrees0: return "0 DEG";
    case CreativePlacementYaw::Degrees90: return "90 DEG";
    case CreativePlacementYaw::Degrees180: return "180 DEG";
    case CreativePlacementYaw::Degrees270: return "270 DEG";
    case CreativePlacementYaw::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeSnapIncrement increment) noexcept {
  switch (increment) {
    case CreativeSnapIncrement::QuarterMeter: return "0.25 M";
    case CreativeSnapIncrement::HalfMeter: return "0.5 M";
    case CreativeSnapIncrement::OneMeter: return "1 M";
    case CreativeSnapIncrement::TwoMeters: return "2 M";
    case CreativeSnapIncrement::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeCloneOffsetAxis axis) noexcept {
  switch (axis) {
    case CreativeCloneOffsetAxis::X: return "X";
    case CreativeCloneOffsetAxis::Y: return "Y";
    case CreativeCloneOffsetAxis::Z: return "Z";
    case CreativeCloneOffsetAxis::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeCloneOffsetDistance distance) noexcept {
  switch (distance) {
    case CreativeCloneOffsetDistance::OneCell: return "1 CELL";
    case CreativeCloneOffsetDistance::TwoCells: return "2 CELLS";
    case CreativeCloneOffsetDistance::FourCells: return "4 CELLS";
    case CreativeCloneOffsetDistance::EightCells: return "8 CELLS";
    case CreativeCloneOffsetDistance::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeArrayMode mode) noexcept {
  switch (mode) {
    case CreativeArrayMode::Linear: return "LINEAR";
    case CreativeArrayMode::Radial: return "RADIAL";
    case CreativeArrayMode::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeToolOptionAdjustStatus status) noexcept {
  switch (status) {
    case CreativeToolOptionAdjustStatus::NotRequested: return "NotRequested";
    case CreativeToolOptionAdjustStatus::InvalidOption: return "InvalidOption";
    case CreativeToolOptionAdjustStatus::InvalidSettings:
      return "InvalidSettings";
    case CreativeToolOptionAdjustStatus::NoAvailableValue:
      return "NoAvailableValue";
    case CreativeToolOptionAdjustStatus::NoChange: return "NoChange";
    case CreativeToolOptionAdjustStatus::Applied: return "Applied";
  }
  return "Unknown";
}

std::string_view creativeToolOptionValueLabel(
    const CreativeToolSettings& settings,
    CreativeToolOptionId option) noexcept {
  switch (option) {
    case CreativeToolOptionId::MoveConstraint:
      return toString(settings.moveConstraint);
    case CreativeToolOptionId::RotationStep:
      return toString(settings.rotationStep);
    case CreativeToolOptionId::PlacementYaw:
      return toString(settings.placementYaw);
    case CreativeToolOptionId::SnapIncrement:
      return toString(settings.snapIncrement);
    case CreativeToolOptionId::MaterialBrushShape:
      return toString(settings.materialBrushShape);
    case CreativeToolOptionId::MaterialBrushAxis:
      return toString(settings.materialBrushAxis);
    case CreativeToolOptionId::MaterialBrushSize:
      return toString(settings.materialBrushSize);
    case CreativeToolOptionId::MaterialBrushFill:
      return toString(settings.materialBrushFill);
    case CreativeToolOptionId::MaterialBrushGuide:
      return toString(settings.materialBrushGuide);
    case CreativeToolOptionId::MaterialBrushSymmetry:
      return toString(settings.materialBrushSymmetry);
    case CreativeToolOptionId::MaterialBrushMask:
      return toString(settings.materialBrushMask);
    case CreativeToolOptionId::MaterialBrushReplaceSource:
      return settings.materialBrushReplaceSourceKind ==
                     CreativeObjectKind::Unknown
                 ? std::string_view{"ANY"}
                 : toString(settings.materialBrushReplaceSourceKind);
    case CreativeToolOptionId::ConnectedFillLimit:
      return toString(settings.connectedFillLimit);
    case CreativeToolOptionId::SurfaceExtrudeDepth:
      return toString(settings.surfaceExtrudeDepth);
    case CreativeToolOptionId::SurfaceExtrudeLimit:
      return toString(settings.surfaceExtrudeLimit);
    case CreativeToolOptionId::ShapeBrushKind:
      return toString(settings.shapeBrushKind);
    case CreativeToolOptionId::ShapeBrushAxis:
      return toString(settings.shapeBrushAxis);
    case CreativeToolOptionId::ReplaceSource:
      return settings.replaceSourceKind == CreativeObjectKind::Unknown
                 ? std::string_view{"ANY"}
                 : toString(settings.replaceSourceKind);
    case CreativeToolOptionId::CloneOffsetAxis:
      return toString(settings.cloneOffsetAxis);
    case CreativeToolOptionId::CloneOffsetDistance:
      return toString(settings.cloneOffsetDistance);
    case CreativeToolOptionId::ArrayMode:
      return toString(settings.arrayMode);
    case CreativeToolOptionId::ArrayDirection:
      return toString(settings.arrayDirection);
    case CreativeToolOptionId::ArrayCopyCount:
      return toString(settings.arrayCopyCount);
    case CreativeToolOptionId::ArraySpacing:
      return toString(settings.arraySpacing);
    case CreativeToolOptionId::RadialArrayAxis:
      return toString(settings.radialArrayAxis);
    case CreativeToolOptionId::RadialArrayInstanceCount:
      return toString(settings.radialArrayInstanceCount);
    case CreativeToolOptionId::RadialArraySweep:
      return toString(settings.radialArraySweep);
    case CreativeToolOptionId::TerrainSculptMode:
      return toString(settings.terrainSculptMode);
    case CreativeToolOptionId::TerrainSculptRadius:
      return toString(settings.terrainSculptRadius);
    case CreativeToolOptionId::TerrainSculptStrength:
      return toString(settings.terrainSculptStrength);
    case CreativeToolOptionId::TerrainSculptFalloff:
      return toString(settings.terrainSculptFalloff);
    case CreativeToolOptionId::TerrainRodStampMode:
      return toString(settings.terrainRodStampMode);
    case CreativeToolOptionId::TerrainSeedRadius:
      return toString(settings.terrainSeedRadius);
    case CreativeToolOptionId::TerrainSeedSpacing:
      return toString(settings.terrainSeedSpacing);
    case CreativeToolOptionId::TerrainProfileKind:
      return toString(settings.terrainProfileKind);
    case CreativeToolOptionId::TerrainProfileBlend:
      return toString(settings.terrainProfileBlend);
    case CreativeToolOptionId::TerrainProfileRodPolicy:
      return toString(settings.terrainProfileRodPolicy);
    case CreativeToolOptionId::TerrainProfileRadius:
      return toString(settings.terrainProfileRadius);
    case CreativeToolOptionId::TerrainProfileAmplitude:
      return toString(settings.terrainProfileAmplitude);
    case CreativeToolOptionId::TerrainProfileSpacing:
      return toString(settings.terrainProfileSpacing);
    case CreativeToolOptionId::TerrainProfileDirection:
      return toString(settings.terrainProfileDirection);
    case CreativeToolOptionId::TerrainProfileFrequency:
      return toString(settings.terrainProfileFrequency);
    case CreativeToolOptionId::TerrainPathKind:
      return toString(settings.terrainPathKind);
    case CreativeToolOptionId::TerrainPathElevation:
      return toString(settings.terrainPathElevation);
    case CreativeToolOptionId::TerrainPathWidth:
      return toString(settings.terrainPathWidth);
    case CreativeToolOptionId::TerrainPathAmplitude:
      return toString(settings.terrainPathAmplitude);
    case CreativeToolOptionId::TerrainRegionOperation:
      return toString(settings.terrainRegionOperation);
    case CreativeToolOptionId::TerrainRegionAmount:
      return toString(settings.terrainRegionAmount);
    case CreativeToolOptionId::Count:
      break;
  }
  return "INVALID";
}

CreativeToolOptionAdjustReceipt adjustCreativeToolOption(
    CreativeToolSettings& settings,
    CreativeToolOptionId option,
    std::int32_t direction,
    std::span<const CreativeObjectKind> materialPalette) noexcept {
  CreativeToolOptionAdjustReceipt receipt = adjustReceipt(option);
  if (creativeToolOptionDescriptor(option) == nullptr) {
    receipt.status = CreativeToolOptionAdjustStatus::InvalidOption;
    receipt.reasonCode = "creative_tool_option_invalid";
    return receipt;
  }
  if (!isValidCreativeToolSettings(settings)) {
    receipt.status = CreativeToolOptionAdjustStatus::InvalidSettings;
    receipt.reasonCode = "creative_tool_option_settings_invalid";
    return receipt;
  }
  if (direction == 0) {
    receipt.accepted = true;
    receipt.status = CreativeToolOptionAdjustStatus::NoChange;
    receipt.reasonCode = "creative_tool_option_direction_zero";
    return receipt;
  }

  CreativeToolSettings adjusted = settings;
  switch (option) {
    case CreativeToolOptionId::MoveConstraint:
      adjusted.moveConstraint = cycleEnum(
          adjusted.moveConstraint, CreativeMoveConstraint::Count, direction);
      break;
    case CreativeToolOptionId::RotationStep:
      adjusted.rotationStep = cycleEnum(
          adjusted.rotationStep, CreativeRotationStep::Count, direction);
      break;
    case CreativeToolOptionId::PlacementYaw:
      adjusted.placementYaw = cycleEnum(
          adjusted.placementYaw, CreativePlacementYaw::Count, direction);
      break;
    case CreativeToolOptionId::SnapIncrement:
      adjusted.snapIncrement = cycleEnum(
          adjusted.snapIncrement, CreativeSnapIncrement::Count, direction);
      break;
    case CreativeToolOptionId::MaterialBrushShape:
      adjusted.materialBrushShape =
          cycleEnum(adjusted.materialBrushShape,
                    CreativeMaterialBrushShape::Count, direction);
      break;
    case CreativeToolOptionId::MaterialBrushAxis:
      adjusted.materialBrushAxis = cycleEnum(
          adjusted.materialBrushAxis, CreativeAxis3::Count, direction);
      break;
    case CreativeToolOptionId::MaterialBrushSize:
      adjusted.materialBrushSize =
          cycleEnum(adjusted.materialBrushSize,
                    CreativeMaterialBrushSize::Count, direction);
      break;
    case CreativeToolOptionId::MaterialBrushFill:
      adjusted.materialBrushFill =
          cycleEnum(adjusted.materialBrushFill,
                    CreativeMaterialBrushFill::Count, direction);
      break;
    case CreativeToolOptionId::MaterialBrushGuide:
      adjusted.materialBrushGuide = cycleEnum(
          adjusted.materialBrushGuide, CreativeMaterialBrushGuide::Count,
          direction);
      break;
    case CreativeToolOptionId::MaterialBrushSymmetry:
      adjusted.materialBrushSymmetry = cycleEnum(
          adjusted.materialBrushSymmetry,
          CreativeMaterialBrushSymmetry::Count, direction);
      break;
    case CreativeToolOptionId::MaterialBrushMask:
      adjusted.materialBrushMask =
          cycleEnum(adjusted.materialBrushMask,
                    CreativeMaterialBrushMask::Count, direction);
      break;
    case CreativeToolOptionId::MaterialBrushReplaceSource: {
      CreativeObjectKind next = adjusted.materialBrushReplaceSourceKind;
      if (!nextMaterialBrushReplaceSource(
              materialPalette, adjusted.materialBrushReplaceSourceKind,
              direction, next)) {
        receipt.status = CreativeToolOptionAdjustStatus::NoAvailableValue;
        receipt.reasonCode = "creative_tool_option_material_unavailable";
        return receipt;
      }
      adjusted.materialBrushReplaceSourceKind = next;
      break;
    }
    case CreativeToolOptionId::ConnectedFillLimit:
      adjusted.connectedFillLimit = cycleEnum(
          adjusted.connectedFillLimit, CreativeConnectedFillLimit::Count,
          direction);
      break;
    case CreativeToolOptionId::SurfaceExtrudeDepth:
      adjusted.surfaceExtrudeDepth = cycleEnum(
          adjusted.surfaceExtrudeDepth, CreativeSurfaceExtrudeDepth::Count,
          direction);
      break;
    case CreativeToolOptionId::SurfaceExtrudeLimit:
      adjusted.surfaceExtrudeLimit = cycleEnum(
          adjusted.surfaceExtrudeLimit, CreativeConnectedFillLimit::Count,
          direction);
      break;
    case CreativeToolOptionId::ShapeBrushKind:
      adjusted.shapeBrushKind = cycleEnum(
          adjusted.shapeBrushKind, CreativeShapeBrushKind::Count, direction);
      break;
    case CreativeToolOptionId::ShapeBrushAxis:
      adjusted.shapeBrushAxis = cycleEnum(
          adjusted.shapeBrushAxis, CreativeShapeBrushAxis::Count, direction);
      break;
    case CreativeToolOptionId::ReplaceSource: {
      CreativeObjectKind next = adjusted.replaceSourceKind;
      if (!nextReplaceSource(materialPalette, adjusted.replaceSourceKind,
                             direction, next)) {
        receipt.status = CreativeToolOptionAdjustStatus::NoAvailableValue;
        receipt.reasonCode = "creative_tool_option_material_unavailable";
        return receipt;
      }
      adjusted.replaceSourceKind = next;
      break;
    }
    case CreativeToolOptionId::CloneOffsetAxis:
      adjusted.cloneOffsetAxis = cycleEnum(
          adjusted.cloneOffsetAxis, CreativeCloneOffsetAxis::Count, direction);
      break;
    case CreativeToolOptionId::CloneOffsetDistance:
      adjusted.cloneOffsetDistance =
          cycleEnum(adjusted.cloneOffsetDistance,
                    CreativeCloneOffsetDistance::Count, direction);
      break;
    case CreativeToolOptionId::ArrayMode:
      adjusted.arrayMode = cycleEnum(
          adjusted.arrayMode, CreativeArrayMode::Count, direction);
      break;
    case CreativeToolOptionId::ArrayDirection:
      adjusted.arrayDirection = cycleEnum(
          adjusted.arrayDirection, CreativeLinearArrayDirection::Count,
          direction);
      break;
    case CreativeToolOptionId::ArrayCopyCount:
      adjusted.arrayCopyCount = cycleEnum(
          adjusted.arrayCopyCount, CreativeLinearArrayCopyCount::Count,
          direction);
      break;
    case CreativeToolOptionId::ArraySpacing:
      adjusted.arraySpacing = cycleEnum(
          adjusted.arraySpacing, CreativeLinearArraySpacing::Count,
          direction);
      break;
    case CreativeToolOptionId::RadialArrayAxis:
      adjusted.radialArrayAxis = cycleEnum(
          adjusted.radialArrayAxis, CreativeAxis3::Count, direction);
      break;
    case CreativeToolOptionId::RadialArrayInstanceCount:
      adjusted.radialArrayInstanceCount = cycleEnum(
          adjusted.radialArrayInstanceCount,
          CreativeRadialArrayInstanceCount::Count, direction);
      break;
    case CreativeToolOptionId::RadialArraySweep:
      adjusted.radialArraySweep = cycleEnum(
          adjusted.radialArraySweep, CreativeRadialArraySweep::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainSculptMode:
      adjusted.terrainSculptMode = cycleEnum(
          adjusted.terrainSculptMode, CreativeTerrainSculptMode::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainSculptRadius:
      adjusted.terrainSculptRadius = cycleEnum(
          adjusted.terrainSculptRadius, CreativeTerrainSculptRadius::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainSculptStrength:
      adjusted.terrainSculptStrength = cycleEnum(
          adjusted.terrainSculptStrength,
          CreativeTerrainSculptStrength::Count, direction);
      break;
    case CreativeToolOptionId::TerrainSculptFalloff:
      adjusted.terrainSculptFalloff =
          cycleEnum(adjusted.terrainSculptFalloff,
                    CreativeTerrainSculptFalloff::Count, direction);
      break;
    case CreativeToolOptionId::TerrainRodStampMode:
      adjusted.terrainRodStampMode = cycleEnum(
          adjusted.terrainRodStampMode, CreativeTerrainRodStampMode::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainSeedRadius:
      adjusted.terrainSeedRadius = cycleEnum(
          adjusted.terrainSeedRadius, CreativeTerrainSeedRadius::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainSeedSpacing:
      adjusted.terrainSeedSpacing = cycleEnum(
          adjusted.terrainSeedSpacing, CreativeTerrainSeedSpacing::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainProfileKind:
      adjusted.terrainProfileKind = cycleEnum(
          adjusted.terrainProfileKind, CreativeTerrainProfileKind::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainProfileBlend:
      adjusted.terrainProfileBlend = cycleEnum(
          adjusted.terrainProfileBlend, CreativeTerrainProfileBlend::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainProfileRodPolicy:
      adjusted.terrainProfileRodPolicy = cycleEnum(
          adjusted.terrainProfileRodPolicy,
          CreativeTerrainProfileRodPolicy::Count, direction);
      break;
    case CreativeToolOptionId::TerrainProfileRadius:
      adjusted.terrainProfileRadius = cycleEnum(
          adjusted.terrainProfileRadius, CreativeTerrainProfileRadius::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainProfileAmplitude:
      adjusted.terrainProfileAmplitude = cycleEnum(
          adjusted.terrainProfileAmplitude,
          CreativeTerrainProfileAmplitude::Count, direction);
      break;
    case CreativeToolOptionId::TerrainProfileSpacing:
      adjusted.terrainProfileSpacing = cycleEnum(
          adjusted.terrainProfileSpacing,
          CreativeTerrainProfileSpacing::Count, direction);
      break;
    case CreativeToolOptionId::TerrainProfileDirection:
      adjusted.terrainProfileDirection = cycleEnum(
          adjusted.terrainProfileDirection,
          CreativeTerrainProfileDirection::Count, direction);
      break;
    case CreativeToolOptionId::TerrainProfileFrequency:
      adjusted.terrainProfileFrequency = cycleEnum(
          adjusted.terrainProfileFrequency,
          CreativeTerrainProfileFrequency::Count, direction);
      break;
    case CreativeToolOptionId::TerrainPathKind:
      adjusted.terrainPathKind = cycleEnum(
          adjusted.terrainPathKind, CreativeTerrainPathKind::Count, direction);
      break;
    case CreativeToolOptionId::TerrainPathElevation:
      adjusted.terrainPathElevation = cycleEnum(
          adjusted.terrainPathElevation, CreativeTerrainPathElevation::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainPathWidth:
      adjusted.terrainPathWidth = cycleEnum(
          adjusted.terrainPathWidth, CreativeTerrainPathWidth::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainPathAmplitude:
      adjusted.terrainPathAmplitude = cycleEnum(
          adjusted.terrainPathAmplitude, CreativeTerrainPathAmplitude::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainRegionOperation:
      adjusted.terrainRegionOperation = cycleEnum(
          adjusted.terrainRegionOperation,
          CreativeTerrainRegionOperation::Count, direction);
      break;
    case CreativeToolOptionId::TerrainRegionAmount:
      adjusted.terrainRegionAmount = cycleEnum(
          adjusted.terrainRegionAmount, CreativeTerrainRegionAmount::Count,
          direction);
      break;
    case CreativeToolOptionId::Count:
      receipt.status = CreativeToolOptionAdjustStatus::InvalidOption;
      receipt.reasonCode = "creative_tool_option_invalid";
      return receipt;
  }

  if (!isValidCreativeToolSettings(adjusted)) {
    receipt.status = CreativeToolOptionAdjustStatus::InvalidSettings;
    receipt.reasonCode = "creative_tool_option_adjusted_invalid";
    return receipt;
  }
  receipt.accepted = true;
  if (sameSettings(settings, adjusted)) {
    receipt.status = CreativeToolOptionAdjustStatus::NoChange;
    receipt.reasonCode = "creative_tool_option_no_change";
    return receipt;
  }
  settings = adjusted;
  receipt.changed = true;
  receipt.status = CreativeToolOptionAdjustStatus::Applied;
  receipt.reasonCode = "creative_tool_option_applied";
  return receipt;
}

double creativeRotationStepDegrees(CreativeRotationStep step) noexcept {
  constexpr std::array values{15.0, 45.0, 90.0};
  const std::size_t index = static_cast<std::size_t>(step);
  return index < values.size() ? values[index] : 0.0;
}

double creativePlacementYawRadians(CreativePlacementYaw yaw) noexcept {
  constexpr double kQuarterTurn = 1.57079632679489661923;
  const std::size_t index = static_cast<std::size_t>(yaw);
  return index < static_cast<std::size_t>(CreativePlacementYaw::Count)
             ? static_cast<double>(index) * kQuarterTurn
             : 0.0;
}

double creativeSnapIncrementMeters(CreativeSnapIncrement increment) noexcept {
  constexpr std::array values{0.25, 0.5, 1.0, 2.0};
  const std::size_t index = static_cast<std::size_t>(increment);
  return index < values.size() ? values[index] : 0.0;
}

bool tryCreativeCloneOffset(const CreativeToolSettings& settings,
                            double cellSize,
                            CreativeToolWorldPoint& output) noexcept {
  output = {};
  if (!isValidCreativeToolSettings(settings) || !std::isfinite(cellSize) ||
      cellSize <= 0.0) {
    return false;
  }
  constexpr std::array distances{1.0, 2.0, 4.0, 8.0};
  const std::size_t distanceIndex =
      static_cast<std::size_t>(settings.cloneOffsetDistance);
  if (distanceIndex >= distances.size()) {
    return false;
  }
  const double distance = distances[distanceIndex] * cellSize;
  switch (settings.cloneOffsetAxis) {
    case CreativeCloneOffsetAxis::X:
      output.x = distance;
      return true;
    case CreativeCloneOffsetAxis::Y:
      output.y = distance;
      return true;
    case CreativeCloneOffsetAxis::Z:
      output.z = distance;
      return true;
    case CreativeCloneOffsetAxis::Count:
      return false;
  }
  return false;
}

}  // namespace iggy3d::creative
