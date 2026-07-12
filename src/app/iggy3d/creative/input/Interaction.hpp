#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/tools/Volume.hpp"

namespace iggy3d::creative {

enum class CreativeWorldActionId : std::uint8_t {
  Primary,
  Secondary,
  Accept,
  Reject,
  Pick,
  HotbarPrevious,
  HotbarNext,
  Count,
};

inline constexpr std::size_t kCreativeWorldActionCount =
    static_cast<std::size_t>(CreativeWorldActionId::Count);

struct CreativeWorldInputSample {
  std::array<bool, kCreativeWorldActionCount> actionDown{};
  std::int32_t hotbarWheelSteps = 0;
};

struct CreativeWorldActionRouterState {
  std::array<bool, kCreativeWorldActionCount> actionDown{};
};

struct CreativeWorldActionFrame {
  std::array<bool, kCreativeWorldActionCount> down{};
  std::array<bool, kCreativeWorldActionCount> pressed{};
  std::array<bool, kCreativeWorldActionCount> released{};
  std::int32_t hotbarWheelSteps = 0;
};

inline constexpr std::uint64_t kCreativeMaterialStrokeRepeatNanoseconds =
    200'000'000ULL;

enum class CreativeMaterialStrokeKind : std::uint8_t {
  None,
  Remove,
  Place,
};

struct CreativeMaterialRepeatState {
  CreativeMaterialStrokeKind kind = CreativeMaterialStrokeKind::None;
  std::uint64_t nextRepeatAtNanoseconds = 0;
  bool active = false;
};

struct CreativeMaterialRepeatRequest {
  std::uint64_t nowNanoseconds = 0;
  bool primaryPressed = false;
  bool primaryDown = false;
  bool primaryReleased = false;
  bool secondaryPressed = false;
  bool secondaryDown = false;
  bool secondaryReleased = false;
  bool interrupted = false;
};

struct CreativeMaterialRepeatResult {
  CreativeMaterialRepeatState next{};
  CreativeMaterialStrokeKind dueKind = CreativeMaterialStrokeKind::None;
  bool began = false;
  bool mutationDue = false;
  bool finalized = false;
  bool primaryWon = false;
};

[[nodiscard]] CreativeMaterialRepeatResult stepCreativeMaterialRepeat(
    CreativeMaterialRepeatState state,
    const CreativeMaterialRepeatRequest& request) noexcept;

void setCreativeWorldAction(CreativeWorldInputSample& sample,
                            CreativeWorldActionId action,
                            bool down) noexcept;
[[nodiscard]] bool creativeWorldActionDown(
    const CreativeWorldActionFrame& frame,
    CreativeWorldActionId action) noexcept;
[[nodiscard]] bool creativeWorldActionPressed(
    const CreativeWorldActionFrame& frame,
    CreativeWorldActionId action) noexcept;
[[nodiscard]] bool creativeWorldActionReleased(
    const CreativeWorldActionFrame& frame,
    CreativeWorldActionId action) noexcept;
[[nodiscard]] CreativeWorldActionFrame routeCreativeWorldActions(
    CreativeWorldActionRouterState& state,
    const CreativeWorldInputSample& sample) noexcept;

enum class CreativeHeldItemKind : std::uint8_t {
  Material,
  MaterialBrush,
  ObjectSelect,
  ObjectMove,
  VolumeSelect,
  VolumeFill,
  VolumeHollow,
  VolumeReplace,
  VolumeErase,
  VolumeClone,
  LinearArray,
  ConnectedFill,
  Count,
};

inline constexpr std::size_t kCreativeHeldItemKindCount =
    static_cast<std::size_t>(CreativeHeldItemKind::Count);

inline constexpr std::size_t kCreativeHotbarSlotCount = 9;

struct CreativeHotbarEntry {
  CreativeHeldItemKind kind = CreativeHeldItemKind::Material;
  CreativeObjectKind objectKind = CreativeObjectKind::Unknown;
};

struct CreativeHotbarState {
  std::array<CreativeHotbarEntry, kCreativeHotbarSlotCount> entries{};
  std::uint8_t selectedSlot = 0;
};

[[nodiscard]] std::string_view toString(CreativeWorldActionId action) noexcept;
[[nodiscard]] std::string_view toString(CreativeHeldItemKind kind) noexcept;
[[nodiscard]] CreativeHotbarState makeDefaultCreativeHotbar(
    std::span<const CreativeObjectKind> materialPalette) noexcept;
[[nodiscard]] CreativeHotbarEntry& selectedCreativeHotbarEntry(
    CreativeHotbarState& hotbar) noexcept;
[[nodiscard]] const CreativeHotbarEntry& selectedCreativeHotbarEntry(
    const CreativeHotbarState& hotbar) noexcept;
[[nodiscard]] bool selectCreativeHotbarSlot(CreativeHotbarState& hotbar,
                                            std::size_t slot) noexcept;
[[nodiscard]] bool cycleCreativeHotbar(CreativeHotbarState& hotbar,
                                       std::int32_t steps) noexcept;
[[nodiscard]] bool assignCreativeHotbarMaterial(
    CreativeHotbarState& hotbar,
    CreativeObjectKind objectKind) noexcept;
[[nodiscard]] bool creativeHeldItemIsVolumeOperation(
    CreativeHeldItemKind kind) noexcept;
[[nodiscard]] bool creativeHeldItemUsesDirectShapeGesture(
    CreativeHeldItemKind kind) noexcept;
[[nodiscard]] bool creativeHeldItemUsesMaterial(
    CreativeHeldItemKind kind) noexcept;
[[nodiscard]] bool applyCreativeHeldItemMaterial(
    CreativeHotbarEntry& entry,
    CreativeObjectKind objectKind) noexcept;
[[nodiscard]] CreativeVolumeOperationKind creativeVolumeOperationForHeldItem(
    CreativeHeldItemKind kind) noexcept;

struct CreativeGridTarget {
  bool valid = false;
  CreativeVec3 hitPoint{};
  CreativeVec3 faceNormal{};
  CreativeVec3 placerForward{0.0, 0.0, -1.0};
  CreativeGridCoord3 targetCell{};
  CreativeGridCoord3 adjacentCell{};
  CreativeBounds targetCellBounds{};
  CreativeBounds adjacentCellBounds{};
  CreativeVec3 placementAnchor{};
};

[[nodiscard]] CreativeGridTarget resolveCreativeGridTargetFromHit(
    CreativeVec3 hitPoint,
    CreativeVec3 faceNormal,
    double cellSize,
    CreativeVec3 origin = {},
    CreativeVec3 placerForward = {0.0, 0.0, -1.0}) noexcept;

}  // namespace iggy3d::creative
