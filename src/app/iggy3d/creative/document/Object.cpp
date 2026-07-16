
#include "app/iggy3d/creative/document/Object.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <utility>

namespace iggy3d::creative {
namespace {

constexpr std::size_t creativeObjectKindCount() noexcept {
    return static_cast<std::size_t>(CreativeObjectKind::Count);
}

constexpr auto makeAllCreativeObjectKinds() noexcept {
    std::array<CreativeObjectKind, creativeObjectKindCount()> kinds{};
    for (std::size_t index = 0; index < kinds.size(); ++index) {
        kinds[index] = static_cast<CreativeObjectKind>(index);
    }
    return kinds;
}

constexpr auto kAllCreativeObjectKinds = makeAllCreativeObjectKinds();

constexpr auto kSerializedCreativeObjectKindIds =
    std::to_array<std::string_view>({
        "Unknown",
        "Room",
        "Wall",
        "Floor",
        "Ceiling",
        "Roof",
        "Door",
        "Window",
        "Stair",
        "Ramp",
        "Platform",
        "MovingPlatform",
        "Column",
        "Pillar",
        "Beam",
        "Arch",
        "Fence",
        "Railing",
        "Bridge",
        "Ladder",
        "TerrainPatch",
        "WaterVolume",
        "LavaVolume",
        "Pit",
        "Slope",
        "Cliff",
        "CaveOpening",
        "BoundaryVolume",
        "KillPlane",
        "SpawnPoint",
        "ExitPoint",
        "EntrancePoint",
        "Checkpoint",
        "NavRegion",
        "NavLink",
        "JumpLink",
        "ClimbLink",
        "WallRunSurface",
        "SlideSurface",
        "CoverPoint",
        "PatrolNode",
        "TriggerZone",
        "Switch",
        "Lever",
        "PressurePlate",
        "Button",
        "ConditionGate",
        "EventRelay",
        "Spawner",
        "DespawnZone",
        "ScriptMarker",
        "TestLane",
        "DistanceMarker",
        "SpeedMarker",
        "JumpTarget",
        "CoyoteTimeLedge",
        "FallShaft",
        "CollisionProbe",
        "PhysicsProbe",
        "TimingGate",
        "TestStart",
        "TestEnd",
        "Prop",
        "Decal",
        "Sign",
        "Banner",
        "FoliagePatch",
        "Rock",
        "Crate",
        "Barrel",
        "Furniture",
        "Decoration",
        "PointLight",
        "SpotLight",
        "AreaLight",
        "AmbientZone",
        "ReverbZone",
        "SoundEmitter",
        "MusicZone",
        "CameraMarker",
        "CameraRail",
        "CameraTarget",
        "CutsceneMarker",
        "Note",
        "Label",
        "Comment",
        "MeasurementMarker",
        "MeasurementLine",
        "MeasurementBox",
        "GridAnchor",
        "SnapAnchor",
        "ReferenceImage",
        "BlueprintOverlay",
        "Group",
        "PrefabInstance",
        "Socket",
        "AttachmentPoint",
        "EnemySpawn",
        "NpcSpawn",
        "PatrolRoute",
        "InterestPoint",
        "AlertZone",
        "SafeZone",
        "DangerZone",
        "ResourceNode",
        "LootPoint",
        "QuestMarker",
        "DialogueMarker",
    });

static_assert(kSerializedCreativeObjectKindIds.size() ==
              creativeObjectKindCount());

[[nodiscard]] CreativeVec3 add(CreativeVec3 lhs,
                               CreativeVec3 rhs) noexcept {
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

[[nodiscard]] CreativeVec3 subtract(CreativeVec3 lhs,
                                    CreativeVec3 rhs) noexcept {
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

[[nodiscard]] CreativeVec3 multiply(CreativeVec3 lhs,
                                    CreativeVec3 rhs) noexcept {
    return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}

}  // namespace

CreativeTransformedBounds resolveCreativeTransformedBounds(
    CreativeBounds authoredBounds,
    CreativeTransform transform) noexcept {
    CreativeTransformedBounds result;
    const CreativeBoundsMetrics authored = measureCreativeBounds(authoredBounds);
    if (!authored.valid || !isFiniteCreativeVec3(transform.position) ||
        !isFiniteCreativeVec3(transform.rotationEulerRadians) ||
        !isPositiveCreativeVec3(transform.scale) ||
        !isPositiveCreativeVec3(authored.size)) {
        return result;
    }

    result.size = multiply(authored.size, transform.scale);
    result.rotationEulerRadians = transform.rotationEulerRadians;
    result.center = add(
        transform.position,
        rotateCreativeVectorEulerXyz(
            multiply(subtract(authored.center, transform.position),
                     transform.scale),
            transform.rotationEulerRadians));
    if (!isFiniteCreativeVec3(result.center) ||
        !isFiniteCreativeVec3(result.size)) {
        return {};
    }

    const CreativeVec3 half{result.size.x * 0.5, result.size.y * 0.5,
                            result.size.z * 0.5};
    for (std::size_t index = 0; index < result.corners.size(); ++index) {
        const CreativeVec3 local{
            (index & 1U) != 0U ? half.x : -half.x,
            (index & 2U) != 0U ? half.y : -half.y,
            (index & 4U) != 0U ? half.z : -half.z,
        };
        result.corners[index] =
            add(result.center,
                rotateCreativeVectorEulerXyz(
                    local, transform.rotationEulerRadians));
        if (!isFiniteCreativeVec3(result.corners[index])) {
            return {};
        }
    }

    result.worldBounds = {result.corners.front(), result.corners.front()};
    for (std::size_t index = 1; index < result.corners.size(); ++index) {
        const CreativeVec3& corner = result.corners[index];
        result.worldBounds.min.x =
            std::min(result.worldBounds.min.x, corner.x);
        result.worldBounds.min.y =
            std::min(result.worldBounds.min.y, corner.y);
        result.worldBounds.min.z =
            std::min(result.worldBounds.min.z, corner.z);
        result.worldBounds.max.x =
            std::max(result.worldBounds.max.x, corner.x);
        result.worldBounds.max.y =
            std::max(result.worldBounds.max.y, corner.y);
        result.worldBounds.max.z =
            std::max(result.worldBounds.max.z, corner.z);
    }
    result.valid = true;
    return result;
}

CreativeTransformedBounds resolveCreativeObjectBounds(
    const CreativeObject& object) noexcept {
    CreativeTransform transform = object.transform;
    if (!objectHasTransform(object.kind)) {
        transform = {};
    }
    return resolveCreativeTransformedBounds(object.bounds, transform);
}

CreativeObject makeRoomObject(
    CreativeObjectId id,
    std::string name,
    CreativeTransform transform,
    CreativeBounds bounds,
    CreativeLayerId layerId,
    bool visible,
    bool locked,
    std::vector<std::string> tags,
    std::optional<CreativeObjectId> parentId) {
    CreativeObject object;
    object.id = id;
    object.kind = CreativeObjectKind::Room;
    object.name = std::move(name);
    object.transform = transform;
    object.bounds = bounds;
    object.layerId = layerId;
    object.visible = visible;
    object.locked = locked;
    object.tags = std::move(tags);
    object.parentId = parentId;
    return object;
}

std::string_view toString(CreativeObjectKind kind) noexcept {
    return describeObject(kind).name;
}

std::string_view toString(
    CreativeMovingPlatformTraversalMode mode) noexcept {
  switch (mode) {
    case CreativeMovingPlatformTraversalMode::PingPong:
      return "PingPong";
    case CreativeMovingPlatformTraversalMode::Loop:
      return "Loop";
    case CreativeMovingPlatformTraversalMode::Count:
      break;
  }
  return "Unknown";
}

bool parseCreativeMovingPlatformTraversalMode(
    std::string_view value,
    CreativeMovingPlatformTraversalMode& output) noexcept {
  for (std::uint8_t index = 0U;
       index < static_cast<std::uint8_t>(
                   CreativeMovingPlatformTraversalMode::Count);
       ++index) {
    const auto mode =
        static_cast<CreativeMovingPlatformTraversalMode>(index);
    if (toString(mode) == value) {
      output = mode;
      return true;
    }
  }
  return false;
}

bool isValidCreativeMovingPlatformSettings(
    const CreativeMovingPlatformSettings& settings) noexcept {
  return std::isfinite(settings.speedMetersPerSecond) &&
         settings.speedMetersPerSecond > 0.0 &&
         settings.speedMetersPerSecond <= 100.0 &&
         static_cast<std::uint8_t>(settings.traversalMode) <
             static_cast<std::uint8_t>(
                 CreativeMovingPlatformTraversalMode::Count);
}

bool isValidCreativePathPoint(const CreativePathPoint& point) noexcept {
  return std::isfinite(point.position.x) &&
         std::isfinite(point.position.y) &&
         std::isfinite(point.position.z) &&
         std::isfinite(point.dwellSeconds) && point.dwellSeconds >= 0.0 &&
         point.dwellSeconds <= kCreativePathPointMaximumDwellSeconds &&
         std::isfinite(point.outgoingSpeedMultiplier) &&
         point.outgoingSpeedMultiplier >=
             kCreativePathPointMinimumOutgoingSpeedMultiplier &&
         point.outgoingSpeedMultiplier <=
             kCreativePathPointMaximumOutgoingSpeedMultiplier;
}

bool isValidCreativeMovingPlatformPath(
    std::span<const CreativePathPoint> pathPoints) noexcept {
  if (pathPoints.size() < 2U ||
      pathPoints.size() > kCreativeMovingPlatformPathPointCapacity) {
    return false;
  }
  double totalLengthMeters = 0.0;
  for (std::size_t index = 0U; index < pathPoints.size(); ++index) {
    if (!isValidCreativePathPoint(pathPoints[index])) {
      return false;
    }
    const CreativeVec3 point = pathPoints[index].position;
    if (index == 0U) {
      continue;
    }
    const CreativeVec3 previous = pathPoints[index - 1U].position;
    totalLengthMeters += std::hypot(point.x - previous.x,
                                    point.y - previous.y,
                                    point.z - previous.z);
  }
  return std::isfinite(totalLengthMeters) && totalLengthMeters > 1.0e-5;
}

std::string_view serializedObjectKindId(CreativeObjectKind kind) noexcept {
    const auto index = static_cast<std::size_t>(kind);
    if (index >= kSerializedCreativeObjectKindIds.size()) {
        return "Unknown";
    }
    return kSerializedCreativeObjectKindIds[index];
}

bool parseSerializedObjectKindId(std::string_view value,
                                 CreativeObjectKind& out) noexcept {
    if (value.empty()) {
        return false;
    }
    for (const CreativeObjectKind kind : allCreativeObjectKinds()) {
        if (kind == CreativeObjectKind::Unknown) {
            continue;
        }
        if (serializedObjectKindId(kind) == value) {
            out = kind;
            return true;
        }
    }
    return false;
}

bool validCreativeAttachmentSocketName(std::string_view value) noexcept {
    if (value.empty() ||
        value.size() > kCreativeAttachmentSocketNameCapacity) {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](char valueByte) {
        const unsigned char byte = static_cast<unsigned char>(valueByte);
        return (byte >= 'a' && byte <= 'z') ||
               (byte >= 'A' && byte <= 'Z') ||
               (byte >= '0' && byte <= '9') || byte == '_' || byte == '-' ||
               byte == '.';
    });
}

std::span<const CreativeObjectKind> allCreativeObjectKinds() noexcept {
    return std::span<const CreativeObjectKind>{kAllCreativeObjectKinds.data(),
                                               kAllCreativeObjectKinds.size()};
}

bool isStructuralObject(CreativeObjectKind kind) noexcept {
    return objectUsesCategory(kind, CreativeObjectCategory::Structural);
}

bool isTerrainOrVolumeObject(CreativeObjectKind kind) noexcept {
    return objectUsesCategory(kind, CreativeObjectCategory::TerrainOrVolume);
}

bool isNavigationOrMovementObject(CreativeObjectKind kind) noexcept {
    return objectUsesCategory(kind,
                              CreativeObjectCategory::NavigationOrMovement);
}

bool isLogicObject(CreativeObjectKind kind) noexcept {
    return objectUsesCategory(kind, CreativeObjectCategory::Logic);
}

bool isTestingObject(CreativeObjectKind kind) noexcept {
    return objectUsesCategory(kind, CreativeObjectCategory::Testing);
}

bool isVisualDressingObject(CreativeObjectKind kind) noexcept {
    return objectUsesCategory(kind, CreativeObjectCategory::VisualDressing);
}

bool isLightSoundOrCameraObject(CreativeObjectKind kind) noexcept {
    return objectUsesCategory(kind,
                              CreativeObjectCategory::LightSoundOrCamera);
}

bool isAuthoringMetaObject(CreativeObjectKind kind) noexcept {
    return objectUsesCategory(kind, CreativeObjectCategory::AuthoringMeta);
}

bool isGameplayObject(CreativeObjectKind kind) noexcept {
    return objectUsesCategory(kind, CreativeObjectCategory::Gameplay);
}

} // namespace iggy3d::creative
