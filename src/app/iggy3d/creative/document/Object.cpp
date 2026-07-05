
#include "app/iggy3d/creative/document/Object.hpp"

#include <utility>

namespace iggy3d::creative {

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
    }

    return "Unknown";
}

bool isStructuralObject(CreativeObjectKind kind) noexcept {
    switch (kind) {
    case CreativeObjectKind::Room:
    case CreativeObjectKind::Wall:
    case CreativeObjectKind::Floor:
    case CreativeObjectKind::Ceiling:
    case CreativeObjectKind::Roof:
    case CreativeObjectKind::Door:
    case CreativeObjectKind::Window:
    case CreativeObjectKind::Stair:
    case CreativeObjectKind::Ramp:
    case CreativeObjectKind::Platform:
    case CreativeObjectKind::MovingPlatform:
    case CreativeObjectKind::Column:
    case CreativeObjectKind::Pillar:
    case CreativeObjectKind::Beam:
    case CreativeObjectKind::Arch:
    case CreativeObjectKind::Fence:
    case CreativeObjectKind::Railing:
    case CreativeObjectKind::Bridge:
    case CreativeObjectKind::Ladder:
        return true;
    default:
        return false;
    }
}

bool isTerrainOrVolumeObject(CreativeObjectKind kind) noexcept {
    switch (kind) {
    case CreativeObjectKind::TerrainPatch:
    case CreativeObjectKind::WaterVolume:
    case CreativeObjectKind::LavaVolume:
    case CreativeObjectKind::Pit:
    case CreativeObjectKind::Slope:
    case CreativeObjectKind::Cliff:
    case CreativeObjectKind::CaveOpening:
    case CreativeObjectKind::BoundaryVolume:
    case CreativeObjectKind::KillPlane:
        return true;
    default:
        return false;
    }
}

bool isNavigationOrMovementObject(CreativeObjectKind kind) noexcept {
    switch (kind) {
    case CreativeObjectKind::SpawnPoint:
    case CreativeObjectKind::ExitPoint:
    case CreativeObjectKind::EntrancePoint:
    case CreativeObjectKind::Checkpoint:
    case CreativeObjectKind::NavRegion:
    case CreativeObjectKind::NavLink:
    case CreativeObjectKind::JumpLink:
    case CreativeObjectKind::ClimbLink:
    case CreativeObjectKind::WallRunSurface:
    case CreativeObjectKind::SlideSurface:
    case CreativeObjectKind::CoverPoint:
    case CreativeObjectKind::PatrolNode:
        return true;
    default:
        return false;
    }
}

bool isLogicObject(CreativeObjectKind kind) noexcept {
    switch (kind) {
    case CreativeObjectKind::TriggerZone:
    case CreativeObjectKind::Switch:
    case CreativeObjectKind::Lever:
    case CreativeObjectKind::PressurePlate:
    case CreativeObjectKind::Button:
    case CreativeObjectKind::ConditionGate:
    case CreativeObjectKind::EventRelay:
    case CreativeObjectKind::Spawner:
    case CreativeObjectKind::DespawnZone:
    case CreativeObjectKind::ScriptMarker:
        return true;
    default:
        return false;
    }
}

bool isTestingObject(CreativeObjectKind kind) noexcept {
    switch (kind) {
    case CreativeObjectKind::TestLane:
    case CreativeObjectKind::DistanceMarker:
    case CreativeObjectKind::SpeedMarker:
    case CreativeObjectKind::JumpTarget:
    case CreativeObjectKind::CoyoteTimeLedge:
    case CreativeObjectKind::FallShaft:
    case CreativeObjectKind::CollisionProbe:
    case CreativeObjectKind::PhysicsProbe:
    case CreativeObjectKind::TimingGate:
    case CreativeObjectKind::TestStart:
    case CreativeObjectKind::TestEnd:
        return true;
    default:
        return false;
    }
}

bool isVisualDressingObject(CreativeObjectKind kind) noexcept {
    switch (kind) {
    case CreativeObjectKind::Prop:
    case CreativeObjectKind::Decal:
    case CreativeObjectKind::Sign:
    case CreativeObjectKind::Banner:
    case CreativeObjectKind::FoliagePatch:
    case CreativeObjectKind::Rock:
    case CreativeObjectKind::Crate:
    case CreativeObjectKind::Barrel:
    case CreativeObjectKind::Furniture:
    case CreativeObjectKind::Decoration:
        return true;
    default:
        return false;
    }
}

bool isLightSoundOrCameraObject(CreativeObjectKind kind) noexcept {
    switch (kind) {
    case CreativeObjectKind::PointLight:
    case CreativeObjectKind::SpotLight:
    case CreativeObjectKind::AreaLight:
    case CreativeObjectKind::AmbientZone:
    case CreativeObjectKind::ReverbZone:
    case CreativeObjectKind::SoundEmitter:
    case CreativeObjectKind::MusicZone:
    case CreativeObjectKind::CameraMarker:
    case CreativeObjectKind::CameraRail:
    case CreativeObjectKind::CameraTarget:
    case CreativeObjectKind::CutsceneMarker:
        return true;
    default:
        return false;
    }
}

bool isAuthoringMetaObject(CreativeObjectKind kind) noexcept {
    switch (kind) {
    case CreativeObjectKind::Note:
    case CreativeObjectKind::Label:
    case CreativeObjectKind::Comment:
    case CreativeObjectKind::MeasurementMarker:
    case CreativeObjectKind::MeasurementLine:
    case CreativeObjectKind::MeasurementBox:
    case CreativeObjectKind::GridAnchor:
    case CreativeObjectKind::SnapAnchor:
    case CreativeObjectKind::ReferenceImage:
    case CreativeObjectKind::BlueprintOverlay:
    case CreativeObjectKind::Group:
    case CreativeObjectKind::PrefabInstance:
    case CreativeObjectKind::Socket:
    case CreativeObjectKind::AttachmentPoint:
        return true;
    default:
        return false;
    }
}

bool isGameplayObject(CreativeObjectKind kind) noexcept {
    switch (kind) {
    case CreativeObjectKind::EnemySpawn:
    case CreativeObjectKind::NpcSpawn:
    case CreativeObjectKind::PatrolRoute:
    case CreativeObjectKind::InterestPoint:
    case CreativeObjectKind::AlertZone:
    case CreativeObjectKind::SafeZone:
    case CreativeObjectKind::DangerZone:
    case CreativeObjectKind::ResourceNode:
    case CreativeObjectKind::LootPoint:
    case CreativeObjectKind::QuestMarker:
    case CreativeObjectKind::DialogueMarker:
        return true;
    default:
        return false;
    }
}

} // namespace iggy3d::creative
