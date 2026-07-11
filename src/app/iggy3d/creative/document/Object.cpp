
#include "app/iggy3d/creative/document/Object.hpp"

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

[[nodiscard]] bool finite(CreativeVec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) &&
           std::isfinite(value.z);
}

[[nodiscard]] CreativeVec3 rotateEulerXyz(CreativeVec3 point,
                                          CreativeVec3 radians) noexcept {
    const double cx = std::cos(radians.x);
    const double sx = std::sin(radians.x);
    const double cy = std::cos(radians.y);
    const double sy = std::sin(radians.y);
    const double cz = std::cos(radians.z);
    const double sz = std::sin(radians.z);

    const double y1 = point.y * cx - point.z * sx;
    const double z1 = point.y * sx + point.z * cx;
    const double x1 = point.x;
    const double x2 = x1 * cy + z1 * sy;
    const double z2 = -x1 * sy + z1 * cy;
    const double y2 = y1;
    return {x2 * cz - y2 * sz, x2 * sz + y2 * cz, z2};
}

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
    const CreativeVec3 authoredSize{
        authoredBounds.max.x - authoredBounds.min.x,
        authoredBounds.max.y - authoredBounds.min.y,
        authoredBounds.max.z - authoredBounds.min.z,
    };
    if (!finite(authoredBounds.min) || !finite(authoredBounds.max) ||
        !finite(transform.position) ||
        !finite(transform.rotationEulerRadians) || !finite(transform.scale) ||
        !(authoredSize.x > 0.0) || !(authoredSize.y > 0.0) ||
        !(authoredSize.z > 0.0) || !(transform.scale.x > 0.0) ||
        !(transform.scale.y > 0.0) || !(transform.scale.z > 0.0)) {
        return result;
    }

    const CreativeVec3 authoredCenter{
        (authoredBounds.min.x + authoredBounds.max.x) * 0.5,
        (authoredBounds.min.y + authoredBounds.max.y) * 0.5,
        (authoredBounds.min.z + authoredBounds.max.z) * 0.5,
    };
    result.size = multiply(authoredSize, transform.scale);
    result.rotationEulerRadians = transform.rotationEulerRadians;
    result.center = add(
        transform.position,
        rotateEulerXyz(
            multiply(subtract(authoredCenter, transform.position),
                     transform.scale),
            transform.rotationEulerRadians));
    if (!finite(result.center) || !finite(result.size)) {
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
                rotateEulerXyz(local, transform.rotationEulerRadians));
        if (!finite(result.corners[index])) {
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
