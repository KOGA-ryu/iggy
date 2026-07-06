
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
    switch (kind) {
    case CreativeObjectKind::Unknown: return "Unknown";

    case CreativeObjectKind::Room: return "Room";
    case CreativeObjectKind::Wall: return "Wall";
    case CreativeObjectKind::Floor: return "Floor";
    case CreativeObjectKind::Ceiling: return "Ceiling";
    case CreativeObjectKind::Roof: return "Roof";
    case CreativeObjectKind::Door: return "Door";
    case CreativeObjectKind::Window: return "Window";
    case CreativeObjectKind::Stair: return "Stair";
    case CreativeObjectKind::Ramp: return "Ramp";
    case CreativeObjectKind::Platform: return "Platform";
    case CreativeObjectKind::MovingPlatform: return "MovingPlatform";
    case CreativeObjectKind::Column: return "Column";
    case CreativeObjectKind::Pillar: return "Pillar";
    case CreativeObjectKind::Beam: return "Beam";
    case CreativeObjectKind::Arch: return "Arch";
    case CreativeObjectKind::Fence: return "Fence";
    case CreativeObjectKind::Railing: return "Railing";
    case CreativeObjectKind::Bridge: return "Bridge";
    case CreativeObjectKind::Ladder: return "Ladder";

    case CreativeObjectKind::TerrainPatch: return "TerrainPatch";
    case CreativeObjectKind::WaterVolume: return "WaterVolume";
    case CreativeObjectKind::LavaVolume: return "LavaVolume";
    case CreativeObjectKind::Pit: return "Pit";
    case CreativeObjectKind::Slope: return "Slope";
    case CreativeObjectKind::Cliff: return "Cliff";
    case CreativeObjectKind::CaveOpening: return "CaveOpening";
    case CreativeObjectKind::BoundaryVolume: return "BoundaryVolume";
    case CreativeObjectKind::KillPlane: return "KillPlane";

    case CreativeObjectKind::SpawnPoint: return "SpawnPoint";
    case CreativeObjectKind::ExitPoint: return "ExitPoint";
    case CreativeObjectKind::EntrancePoint: return "EntrancePoint";
    case CreativeObjectKind::Checkpoint: return "Checkpoint";
    case CreativeObjectKind::NavRegion: return "NavRegion";
    case CreativeObjectKind::NavLink: return "NavLink";
    case CreativeObjectKind::JumpLink: return "JumpLink";
    case CreativeObjectKind::ClimbLink: return "ClimbLink";
    case CreativeObjectKind::WallRunSurface: return "WallRunSurface";
    case CreativeObjectKind::SlideSurface: return "SlideSurface";
    case CreativeObjectKind::CoverPoint: return "CoverPoint";
    case CreativeObjectKind::PatrolNode: return "PatrolNode";

    case CreativeObjectKind::TriggerZone: return "TriggerZone";
    case CreativeObjectKind::Switch: return "Switch";
    case CreativeObjectKind::Lever: return "Lever";
    case CreativeObjectKind::PressurePlate: return "PressurePlate";
    case CreativeObjectKind::Button: return "Button";
    case CreativeObjectKind::ConditionGate: return "ConditionGate";
    case CreativeObjectKind::EventRelay: return "EventRelay";
    case CreativeObjectKind::Spawner: return "Spawner";
    case CreativeObjectKind::DespawnZone: return "DespawnZone";
    case CreativeObjectKind::ScriptMarker: return "ScriptMarker";

    case CreativeObjectKind::TestLane: return "TestLane";
    case CreativeObjectKind::DistanceMarker: return "DistanceMarker";
    case CreativeObjectKind::SpeedMarker: return "SpeedMarker";
    case CreativeObjectKind::JumpTarget: return "JumpTarget";
    case CreativeObjectKind::CoyoteTimeLedge: return "CoyoteTimeLedge";
    case CreativeObjectKind::FallShaft: return "FallShaft";
    case CreativeObjectKind::CollisionProbe: return "CollisionProbe";
    case CreativeObjectKind::PhysicsProbe: return "PhysicsProbe";
    case CreativeObjectKind::TimingGate: return "TimingGate";
    case CreativeObjectKind::TestStart: return "TestStart";
    case CreativeObjectKind::TestEnd: return "TestEnd";

    case CreativeObjectKind::Prop: return "Prop";
    case CreativeObjectKind::Decal: return "Decal";
    case CreativeObjectKind::Sign: return "Sign";
    case CreativeObjectKind::Banner: return "Banner";
    case CreativeObjectKind::FoliagePatch: return "FoliagePatch";
    case CreativeObjectKind::Rock: return "Rock";
    case CreativeObjectKind::Crate: return "Crate";
    case CreativeObjectKind::Barrel: return "Barrel";
    case CreativeObjectKind::Furniture: return "Furniture";
    case CreativeObjectKind::Decoration: return "Decoration";

    case CreativeObjectKind::PointLight: return "PointLight";
    case CreativeObjectKind::SpotLight: return "SpotLight";
    case CreativeObjectKind::AreaLight: return "AreaLight";
    case CreativeObjectKind::AmbientZone: return "AmbientZone";
    case CreativeObjectKind::ReverbZone: return "ReverbZone";
    case CreativeObjectKind::SoundEmitter: return "SoundEmitter";
    case CreativeObjectKind::MusicZone: return "MusicZone";
    case CreativeObjectKind::CameraMarker: return "CameraMarker";
    case CreativeObjectKind::CameraRail: return "CameraRail";
    case CreativeObjectKind::CameraTarget: return "CameraTarget";
    case CreativeObjectKind::CutsceneMarker: return "CutsceneMarker";

    case CreativeObjectKind::Note: return "Note";
    case CreativeObjectKind::Label: return "Label";
    case CreativeObjectKind::Comment: return "Comment";
    case CreativeObjectKind::MeasurementMarker: return "MeasurementMarker";
    case CreativeObjectKind::MeasurementLine: return "MeasurementLine";
    case CreativeObjectKind::MeasurementBox: return "MeasurementBox";
    case CreativeObjectKind::GridAnchor: return "GridAnchor";
    case CreativeObjectKind::SnapAnchor: return "SnapAnchor";
    case CreativeObjectKind::ReferenceImage: return "ReferenceImage";
    case CreativeObjectKind::BlueprintOverlay: return "BlueprintOverlay";
    case CreativeObjectKind::Group: return "Group";
    case CreativeObjectKind::PrefabInstance: return "PrefabInstance";
    case CreativeObjectKind::Socket: return "Socket";
    case CreativeObjectKind::AttachmentPoint: return "AttachmentPoint";

    case CreativeObjectKind::EnemySpawn: return "EnemySpawn";
    case CreativeObjectKind::NpcSpawn: return "NpcSpawn";
    case CreativeObjectKind::PatrolRoute: return "PatrolRoute";
    case CreativeObjectKind::InterestPoint: return "InterestPoint";
    case CreativeObjectKind::AlertZone: return "AlertZone";
    case CreativeObjectKind::SafeZone: return "SafeZone";
    case CreativeObjectKind::DangerZone: return "DangerZone";
    case CreativeObjectKind::ResourceNode: return "ResourceNode";
    case CreativeObjectKind::LootPoint: return "LootPoint";
    case CreativeObjectKind::QuestMarker: return "QuestMarker";
    case CreativeObjectKind::DialogueMarker: return "DialogueMarker";
    case CreativeObjectKind::Count: return "Unknown";
    }

    return "Unknown";
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
