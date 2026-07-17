#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/spatial/PlacementGrid.hpp"
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

// Shared cadence and bounded visited-key storage for continuous world edits.
inline constexpr std::uint64_t kCreativeWorldGestureRepeatNanoseconds =
    200'000'000ULL;
inline constexpr std::uint64_t kCreativeMaterialStrokeRepeatNanoseconds =
    kCreativeWorldGestureRepeatNanoseconds;
inline constexpr std::size_t kCreativeWorldGestureVisitedKeyCapacity = 256U;

enum class CreativeWorldGestureKind : std::uint8_t {
  None,
  Remove,
  Place,
};

struct CreativeWorldGestureRepeatState {
  CreativeWorldGestureKind kind = CreativeWorldGestureKind::None;
  std::uint64_t nextRepeatAtNanoseconds = 0;
  bool active = false;
};

struct CreativeWorldGestureRepeatRequest {
  std::uint64_t nowNanoseconds = 0;
  bool primaryPressed = false;
  bool primaryDown = false;
  bool primaryReleased = false;
  bool secondaryPressed = false;
  bool secondaryDown = false;
  bool secondaryReleased = false;
  bool interrupted = false;
};

struct CreativeWorldGestureRepeatResult {
  CreativeWorldGestureRepeatState next{};
  CreativeWorldGestureKind dueKind = CreativeWorldGestureKind::None;
  bool began = false;
  bool mutationDue = false;
  bool finalized = false;
  bool primaryWon = false;
};

using CreativeMaterialStrokeKind = CreativeWorldGestureKind;
using CreativeMaterialRepeatState = CreativeWorldGestureRepeatState;
using CreativeMaterialRepeatRequest = CreativeWorldGestureRepeatRequest;
using CreativeMaterialRepeatResult = CreativeWorldGestureRepeatResult;

enum class CreativeWorldGestureVisitStatus : std::uint8_t {
  Inserted,
  AlreadyPresent,
  CapacityExceeded,
};

struct CreativeWorldGestureVisitedKeys {
  std::array<std::uint64_t, kCreativeWorldGestureVisitedKeyCapacity> keys{};
  std::uint16_t count = 0;
};

static_assert(std::is_trivially_copyable_v<CreativeWorldGestureRepeatState>);
static_assert(std::is_standard_layout_v<CreativeWorldGestureRepeatState>);
static_assert(std::is_trivially_copyable_v<CreativeWorldGestureVisitedKeys>);
static_assert(std::is_standard_layout_v<CreativeWorldGestureVisitedKeys>);

[[nodiscard]] bool creativeWorldGestureVisited(
    const CreativeWorldGestureVisitedKeys& visited,
    std::uint64_t key) noexcept;
[[nodiscard]] CreativeWorldGestureVisitStatus rememberCreativeWorldGestureKey(
    CreativeWorldGestureVisitedKeys& visited,
    std::uint64_t key) noexcept;

[[nodiscard]] CreativeWorldGestureRepeatRequest
makeCreativeWorldStrokeRepeatRequest(
    const CreativeWorldActionFrame& actions,
    std::uint64_t nowNanoseconds) noexcept;
[[nodiscard]] CreativeWorldGestureRepeatResult stepCreativeWorldGestureRepeat(
    CreativeWorldGestureRepeatState state,
    const CreativeWorldGestureRepeatRequest& request) noexcept;
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
  SurfaceExtrude,
  TerrainControl,
  TerrainPaint,
  TerrainGrade,
  TerrainSculpt,
  TerrainProfile,
  TerrainPath,
  TerrainRegion,
  ObjectGroup,
  LogicLink,
  BuildingRoom,
  Count,
};

inline constexpr std::size_t kCreativeHeldItemKindCount =
    static_cast<std::size_t>(CreativeHeldItemKind::Count);

inline constexpr std::size_t kCreativeHotbarSlotCount = 9;
inline constexpr std::size_t kCreativeHotbarAssetIdCapacity = 128;

struct CreativeHotbarEntry {
  CreativeHeldItemKind kind = CreativeHeldItemKind::Material;
  CreativeObjectKind objectKind = CreativeObjectKind::Unknown;
  std::array<char, kCreativeHotbarAssetIdCapacity + 1U> assetId{};
  // Imported bounds remain relative to the asset origin used as its pivot.
  CreativeBounds assetSourceBounds{};
  bool hasAssetBounds = false;
};

struct CreativeHotbarState {
  std::array<CreativeHotbarEntry, kCreativeHotbarSlotCount> entries{};
  std::uint8_t selectedSlot = 0;
};

[[nodiscard]] std::string_view toString(CreativeWorldActionId action) noexcept;
[[nodiscard]] std::string_view toString(CreativeHeldItemKind kind) noexcept;
[[nodiscard]] bool parseCreativeHeldItemKind(
    std::string_view value,
    CreativeHeldItemKind& out) noexcept;
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
[[nodiscard]] bool assignCreativeHotbarFromObject(
    CreativeHotbarState& hotbar,
    const CreativeObject& object) noexcept;
[[nodiscard]] std::string_view creativeHotbarAssetId(
    const CreativeHotbarEntry& entry) noexcept;
[[nodiscard]] bool setCreativeHotbarAsset(
    CreativeHotbarEntry& entry,
    std::string_view assetId,
    CreativeBounds sourceBounds) noexcept;
void clearCreativeHotbarAsset(CreativeHotbarEntry& entry) noexcept;
[[nodiscard]] bool creativeHeldItemIsVolumeOperation(
    CreativeHeldItemKind kind) noexcept;
[[nodiscard]] bool creativeHeldItemUsesDirectShapeGesture(
    CreativeHeldItemKind kind) noexcept;
[[nodiscard]] bool creativeHeldItemUsesMaterial(
    CreativeHeldItemKind kind) noexcept;
[[nodiscard]] bool creativeHeldItemIsTerrainTool(
    CreativeHeldItemKind kind) noexcept;
[[nodiscard]] bool applyCreativeHeldItemMaterial(
    CreativeHotbarEntry& entry,
    CreativeObjectKind objectKind) noexcept;
[[nodiscard]] CreativeVolumeOperationKind creativeVolumeOperationForHeldItem(
    CreativeHeldItemKind kind) noexcept;

}  // namespace iggy3d::creative
