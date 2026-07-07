
#include "app/iggy3d/creative/document/Object.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include <array>
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

}  // namespace

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
