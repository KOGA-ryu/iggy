#include "app/iggy3d/creative/input/Interaction.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/input/UiInput.hpp"

namespace iggy3d::creative {
namespace {

[[nodiscard]] std::size_t actionIndex(CreativeWorldActionId action) noexcept {
  return static_cast<std::size_t>(action);
}

[[nodiscard]] std::size_t selectedSlotIndex(
    const CreativeHotbarState& hotbar) noexcept {
  return std::min<std::size_t>(hotbar.selectedSlot,
                               kCreativeHotbarSlotCount - 1U);
}

[[nodiscard]] CreativeVec3 dominantAxisNormal(CreativeVec3 normal) noexcept {
  const double ax = std::fabs(normal.x);
  const double ay = std::fabs(normal.y);
  const double az = std::fabs(normal.z);
  if (!std::isfinite(ax) || !std::isfinite(ay) || !std::isfinite(az) ||
      std::max({ax, ay, az}) <= 1.0e-12) {
    return {};
  }
  if (ax >= ay && ax >= az) {
    return {std::copysign(1.0, normal.x), 0.0, 0.0};
  }
  if (ay >= az) {
    return {0.0, std::copysign(1.0, normal.y), 0.0};
  }
  return {0.0, 0.0, std::copysign(1.0, normal.z)};
}

[[nodiscard]] CreativeVec3 dominantHorizontalNormal(
    CreativeVec3 direction) noexcept {
  if (!isFiniteCreativeVec3(direction)) {
    return {};
  }
  const double ax = std::fabs(direction.x);
  const double az = std::fabs(direction.z);
  if (std::max(ax, az) <= 1.0e-12) {
    return {};
  }
  return ax >= az ? CreativeVec3{std::copysign(1.0, direction.x), 0.0, 0.0}
                  : CreativeVec3{0.0, 0.0,
                                 std::copysign(1.0, direction.z)};
}

[[nodiscard]] bool tryAdjacentCell(CreativeGridCoord3 cell,
                                   CreativeVec3 faceNormal,
                                   CreativeGridCoord3& adjacent) noexcept {
  const std::int64_t x = static_cast<std::int64_t>(cell.x) +
                         static_cast<std::int64_t>(faceNormal.x);
  const std::int64_t y = static_cast<std::int64_t>(cell.y) +
                         static_cast<std::int64_t>(faceNormal.y);
  const std::int64_t z = static_cast<std::int64_t>(cell.z) +
                         static_cast<std::int64_t>(faceNormal.z);
  constexpr std::int64_t minCoord =
      std::numeric_limits<std::int32_t>::min();
  constexpr std::int64_t maxCoord =
      std::numeric_limits<std::int32_t>::max();
  if (x < minCoord || x > maxCoord || y < minCoord || y > maxCoord ||
      z < minCoord || z > maxCoord) {
    return false;
  }
  adjacent = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(y),
              static_cast<std::int32_t>(z)};
  return true;
}

}  // namespace

void setCreativeWorldAction(CreativeWorldInputSample& sample,
                            CreativeWorldActionId action,
                            bool down) noexcept {
  const std::size_t index = actionIndex(action);
  if (index < sample.actionDown.size()) {
    sample.actionDown[index] = down;
  }
}

bool creativeWorldActionDown(const CreativeWorldActionFrame& frame,
                             CreativeWorldActionId action) noexcept {
  const std::size_t index = actionIndex(action);
  return index < frame.down.size() && frame.down[index];
}

bool creativeWorldActionPressed(const CreativeWorldActionFrame& frame,
                                CreativeWorldActionId action) noexcept {
  const std::size_t index = actionIndex(action);
  return index < frame.pressed.size() && frame.pressed[index];
}

bool creativeWorldActionReleased(const CreativeWorldActionFrame& frame,
                                 CreativeWorldActionId action) noexcept {
  const std::size_t index = actionIndex(action);
  return index < frame.released.size() && frame.released[index];
}

CreativeWorldActionFrame routeCreativeWorldActions(
    CreativeWorldActionRouterState& state,
    const CreativeWorldInputSample& sample) noexcept {
  CreativeWorldActionFrame frame;
  frame.hotbarWheelSteps = sample.hotbarWheelSteps;
  for (std::size_t index = 0; index < kCreativeWorldActionCount; ++index) {
    frame.down[index] = sample.actionDown[index];
    frame.pressed[index] = sample.actionDown[index] && !state.actionDown[index];
    frame.released[index] = !sample.actionDown[index] && state.actionDown[index];
  }
  state.actionDown = sample.actionDown;
  return frame;
}

std::string_view toString(CreativeWorldActionId action) noexcept {
  switch (action) {
    case CreativeWorldActionId::Primary: return "Primary";
    case CreativeWorldActionId::Secondary: return "Secondary";
    case CreativeWorldActionId::Accept: return "Accept";
    case CreativeWorldActionId::Reject: return "Reject";
    case CreativeWorldActionId::Pick: return "Pick";
    case CreativeWorldActionId::HotbarPrevious: return "HotbarPrevious";
    case CreativeWorldActionId::HotbarNext: return "HotbarNext";
    case CreativeWorldActionId::Count: break;
  }
  return "Unknown";
}

std::string_view toString(CreativeHeldItemKind kind) noexcept {
  switch (kind) {
    case CreativeHeldItemKind::Material: return "Material";
    case CreativeHeldItemKind::MaterialBrush: return "Brush";
    case CreativeHeldItemKind::ObjectSelect: return "Select";
    case CreativeHeldItemKind::ObjectMove: return "Move";
    case CreativeHeldItemKind::VolumeSelect: return "VolumeSelect";
    case CreativeHeldItemKind::VolumeFill: return "Fill";
    case CreativeHeldItemKind::VolumeHollow: return "Hollow";
    case CreativeHeldItemKind::VolumeReplace: return "Replace";
    case CreativeHeldItemKind::VolumeErase: return "Erase";
    case CreativeHeldItemKind::VolumeClone: return "Clone";
    case CreativeHeldItemKind::LinearArray: return "Array";
    case CreativeHeldItemKind::ConnectedFill: return "Flood";
    case CreativeHeldItemKind::SurfaceExtrude: return "Extrude";
    case CreativeHeldItemKind::TerrainControl: return "Terrain";
    case CreativeHeldItemKind::TerrainGrade: return "TerrainGrade";
    case CreativeHeldItemKind::Count: break;
  }
  return "Unknown";
}

bool parseCreativeHeldItemKind(std::string_view value,
                               CreativeHeldItemKind& out) noexcept {
  for (std::size_t index = 0;
       index < static_cast<std::size_t>(CreativeHeldItemKind::Count);
       ++index) {
    const CreativeHeldItemKind candidate =
        static_cast<CreativeHeldItemKind>(index);
    if (toString(candidate) == value) {
      out = candidate;
      return true;
    }
  }
  return false;
}

CreativeHotbarState makeDefaultCreativeHotbar(
    std::span<const CreativeObjectKind> materialPalette) noexcept {
  const CreativeObjectKind material =
      materialPalette.empty() ? CreativeObjectKind::Unknown
                              : materialPalette.front();
  CreativeHotbarState hotbar;
  hotbar.entries = {
      CreativeHotbarEntry{CreativeHeldItemKind::Material, material},
      CreativeHotbarEntry{CreativeHeldItemKind::ObjectSelect, {}},
      CreativeHotbarEntry{CreativeHeldItemKind::ObjectMove, {}},
      CreativeHotbarEntry{CreativeHeldItemKind::VolumeSelect, {}},
      CreativeHotbarEntry{CreativeHeldItemKind::VolumeFill, material},
      CreativeHotbarEntry{CreativeHeldItemKind::VolumeHollow, material},
      CreativeHotbarEntry{CreativeHeldItemKind::VolumeReplace, material},
      CreativeHotbarEntry{CreativeHeldItemKind::VolumeErase, {}},
      CreativeHotbarEntry{CreativeHeldItemKind::VolumeClone, {}},
  };
  return hotbar;
}

CreativeHotbarEntry& selectedCreativeHotbarEntry(
    CreativeHotbarState& hotbar) noexcept {
  return hotbar.entries[selectedSlotIndex(hotbar)];
}

const CreativeHotbarEntry& selectedCreativeHotbarEntry(
    const CreativeHotbarState& hotbar) noexcept {
  return hotbar.entries[selectedSlotIndex(hotbar)];
}

bool selectCreativeHotbarSlot(CreativeHotbarState& hotbar,
                              std::size_t slot) noexcept {
  if (slot >= kCreativeHotbarSlotCount || hotbar.selectedSlot == slot) {
    return false;
  }
  hotbar.selectedSlot = static_cast<std::uint8_t>(slot);
  return true;
}

bool cycleCreativeHotbar(CreativeHotbarState& hotbar,
                         std::int32_t steps) noexcept {
  if (steps == 0) {
    return false;
  }
  const CreativeWrappedIndexResult next = stepCreativeWrappedIndex(
      selectedSlotIndex(hotbar), kCreativeHotbarSlotCount, steps);
  if (!next.valid) {
    return false;
  }
  hotbar.selectedSlot = static_cast<std::uint8_t>(next.index);
  return next.changed;
}

bool assignCreativeHotbarMaterial(CreativeHotbarState& hotbar,
                                  CreativeObjectKind objectKind) noexcept {
  if (objectKind == CreativeObjectKind::Unknown) {
    return false;
  }
  CreativeHotbarEntry& selected = selectedCreativeHotbarEntry(hotbar);
  if (selected.kind == CreativeHeldItemKind::Material &&
      selected.objectKind == objectKind) {
    return false;
  }
  selected = {CreativeHeldItemKind::Material, objectKind};
  return true;
}

bool creativeHeldItemIsVolumeOperation(CreativeHeldItemKind kind) noexcept {
  return kind >= CreativeHeldItemKind::VolumeFill &&
         kind <= CreativeHeldItemKind::VolumeClone;
}

bool creativeHeldItemUsesDirectShapeGesture(
    CreativeHeldItemKind kind) noexcept {
  return kind == CreativeHeldItemKind::VolumeFill ||
         kind == CreativeHeldItemKind::VolumeHollow;
}

bool creativeHeldItemUsesMaterial(CreativeHeldItemKind kind) noexcept {
  switch (kind) {
    case CreativeHeldItemKind::Material:
    case CreativeHeldItemKind::MaterialBrush:
    case CreativeHeldItemKind::VolumeFill:
    case CreativeHeldItemKind::VolumeHollow:
    case CreativeHeldItemKind::VolumeReplace:
    case CreativeHeldItemKind::ConnectedFill:
    case CreativeHeldItemKind::SurfaceExtrude:
      return true;
    case CreativeHeldItemKind::ObjectSelect:
    case CreativeHeldItemKind::ObjectMove:
    case CreativeHeldItemKind::VolumeSelect:
    case CreativeHeldItemKind::VolumeErase:
    case CreativeHeldItemKind::VolumeClone:
    case CreativeHeldItemKind::LinearArray:
    case CreativeHeldItemKind::TerrainControl:
    case CreativeHeldItemKind::TerrainGrade:
    case CreativeHeldItemKind::Count:
      return false;
  }
  return false;
}

bool applyCreativeHeldItemMaterial(CreativeHotbarEntry& entry,
                                   CreativeObjectKind objectKind) noexcept {
  if (!creativeHeldItemUsesMaterial(entry.kind) ||
      objectKind == CreativeObjectKind::Unknown ||
      entry.objectKind == objectKind) {
    return false;
  }
  entry.objectKind = objectKind;
  return true;
}

CreativeVolumeOperationKind creativeVolumeOperationForHeldItem(
    CreativeHeldItemKind kind) noexcept {
  constexpr std::array operations{
      CreativeVolumeOperationKind::Fill,
      CreativeVolumeOperationKind::Hollow,
      CreativeVolumeOperationKind::Replace,
      CreativeVolumeOperationKind::Erase,
      CreativeVolumeOperationKind::Clone,
  };
  if (!creativeHeldItemIsVolumeOperation(kind)) {
    return CreativeVolumeOperationKind::Fill;
  }
  const std::size_t index =
      static_cast<std::size_t>(kind) -
      static_cast<std::size_t>(CreativeHeldItemKind::VolumeFill);
  return operations[index];
}

CreativeGridTarget resolveCreativeGridTargetFromHit(
    CreativeVec3 hitPoint,
    CreativeVec3 faceNormal,
    double cellSize,
    CreativeVec3 origin,
    CreativeVec3 placerForward) noexcept {
  CreativeGridTarget target;
  if (!isFiniteCreativeVec3(hitPoint) ||
      !isFiniteCreativeVec3(faceNormal) ||
      !isFiniteCreativeVec3(origin) ||
      !isFiniteCreativeVec3(placerForward) ||
      !std::isfinite(cellSize) || cellSize <= 0.0) {
    return target;
  }

  const CreativeVec3 snappedNormal = dominantAxisNormal(faceNormal);
  if (snappedNormal.x == 0.0 && snappedNormal.y == 0.0 &&
      snappedNormal.z == 0.0) {
    return target;
  }
  const double epsilon = std::max(cellSize * 1.0e-4, 1.0e-7);
  const CreativeVec3 inside{hitPoint.x - snappedNormal.x * epsilon,
                            hitPoint.y - snappedNormal.y * epsilon,
                            hitPoint.z - snappedNormal.z * epsilon};
  if (!tryCreativeVolumeCellFromWorld(inside, cellSize, origin,
                                      target.targetCell) ||
      !tryAdjacentCell(target.targetCell, snappedNormal,
                       target.adjacentCell)) {
    return target;
  }

  target.hitPoint = hitPoint;
  target.faceNormal = snappedNormal;
  target.placerForward = dominantHorizontalNormal(placerForward);
  target.targetCellBounds =
      creativeVolumeCellBounds(target.targetCell, cellSize, origin);
  target.adjacentCellBounds =
      creativeVolumeCellBounds(target.adjacentCell, cellSize, origin);
  const CreativeVec3 adjacentCenter =
      measureCreativeBounds(target.adjacentCellBounds).center;
  target.placementAnchor = {adjacentCenter.x,
                            target.adjacentCellBounds.min.y,
                            adjacentCenter.z};
  target.valid = true;
  return target;
}

CreativeMaterialRepeatRequest makeCreativeWorldStrokeRepeatRequest(
    const CreativeWorldActionFrame& actions,
    std::uint64_t nowNanoseconds) noexcept {
  CreativeMaterialRepeatRequest request;
  request.nowNanoseconds = nowNanoseconds;
  request.primaryPressed =
      creativeWorldActionPressed(actions, CreativeWorldActionId::Primary) ||
      creativeWorldActionPressed(actions, CreativeWorldActionId::Reject);
  request.primaryDown =
      creativeWorldActionDown(actions, CreativeWorldActionId::Primary) ||
      creativeWorldActionDown(actions, CreativeWorldActionId::Reject);
  request.primaryReleased =
      creativeWorldActionReleased(actions, CreativeWorldActionId::Primary) ||
      creativeWorldActionReleased(actions, CreativeWorldActionId::Reject);
  request.secondaryPressed =
      creativeWorldActionPressed(actions, CreativeWorldActionId::Secondary) ||
      creativeWorldActionPressed(actions, CreativeWorldActionId::Accept);
  request.secondaryDown =
      creativeWorldActionDown(actions, CreativeWorldActionId::Secondary) ||
      creativeWorldActionDown(actions, CreativeWorldActionId::Accept);
  request.secondaryReleased =
      creativeWorldActionReleased(actions, CreativeWorldActionId::Secondary) ||
      creativeWorldActionReleased(actions, CreativeWorldActionId::Accept);
  return request;
}

CreativeMaterialRepeatResult stepCreativeMaterialRepeat(
    CreativeMaterialRepeatState state,
    const CreativeMaterialRepeatRequest& request) noexcept {
  CreativeMaterialRepeatResult result;
  result.next = state;
  if (state.active) {
    const bool primary = state.kind == CreativeMaterialStrokeKind::Remove;
    const bool down = primary ? request.primaryDown : request.secondaryDown;
    const bool released =
        primary ? request.primaryReleased : request.secondaryReleased;
    if (request.interrupted || released || !down) {
      result.next = {};
      result.finalized = true;
      return result;
    }
    if (request.nowNanoseconds >= state.nextRepeatAtNanoseconds) {
      result.mutationDue = true;
      result.dueKind = state.kind;
      const std::uint64_t elapsed =
          request.nowNanoseconds - state.nextRepeatAtNanoseconds;
      const std::uint64_t steps =
          elapsed / kCreativeMaterialStrokeRepeatNanoseconds + 1U;
      const std::uint64_t remaining =
          std::numeric_limits<std::uint64_t>::max() -
          state.nextRepeatAtNanoseconds;
      result.next.nextRepeatAtNanoseconds =
          steps > remaining / kCreativeMaterialStrokeRepeatNanoseconds
              ? std::numeric_limits<std::uint64_t>::max()
              : state.nextRepeatAtNanoseconds +
                    steps * kCreativeMaterialStrokeRepeatNanoseconds;
    }
    return result;
  }

  if (request.interrupted ||
      (!request.primaryPressed && !request.secondaryPressed)) {
    return result;
  }
  result.began = true;
  result.mutationDue = true;
  result.primaryWon = request.primaryPressed && request.secondaryPressed;
  result.dueKind = request.primaryPressed
                       ? CreativeMaterialStrokeKind::Remove
                       : CreativeMaterialStrokeKind::Place;
  result.next.active = true;
  result.next.kind = result.dueKind;
  result.next.nextRepeatAtNanoseconds =
      request.nowNanoseconds >
              std::numeric_limits<std::uint64_t>::max() -
                  kCreativeMaterialStrokeRepeatNanoseconds
          ? std::numeric_limits<std::uint64_t>::max()
          : request.nowNanoseconds +
                kCreativeMaterialStrokeRepeatNanoseconds;
  return result;
}

}  // namespace iggy3d::creative
