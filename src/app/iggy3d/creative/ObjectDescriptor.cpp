

#include "iggy3d/creative/ObjectDescriptor.hpp"

#include <array>

namespace iggy3d::creative {
namespace {

constexpr CreativeObjectDirtyFlags flagValue(CreativeObjectDirtyFlag flag) noexcept {
    return static_cast<CreativeObjectDirtyFlags>(flag);
}

constexpr CreativeObjectDirtyFlags commonAuthoredDirtyFlags() noexcept {
    return flagValue(CreativeObjectDirtyFlag::Identity) |
           flagValue(CreativeObjectDirtyFlag::Preview) |
           flagValue(CreativeObjectDirtyFlag::Serialization);
}

constexpr CreativeObjectDirtyFlags structuralCreationDirtyFlags() noexcept {
    return commonAuthoredDirtyFlags() |
           flagValue(CreativeObjectDirtyFlag::Bounds) |
           flagValue(CreativeObjectDirtyFlag::Geometry) |
           flagValue(CreativeObjectDirtyFlag::Collision);
}

constexpr CreativeObjectDirtyFlags navigationCreationDirtyFlags() noexcept {
    return commonAuthoredDirtyFlags() |
           flagValue(CreativeObjectDirtyFlag::Transform) |
           flagValue(CreativeObjectDirtyFlag::Navigation) |
           flagValue(CreativeObjectDirtyFlag::Gameplay);
}

constexpr CreativeObjectDirtyFlags logicCreationDirtyFlags() noexcept {
    return commonAuthoredDirtyFlags() |
           flagValue(CreativeObjectDirtyFlag::Logic) |
           flagValue(CreativeObjectDirtyFlag::Gameplay);
}

constexpr CreativeObjectDirtyFlags testingCreationDirtyFlags() noexcept {
    return commonAuthoredDirtyFlags() |
           flagValue(CreativeObjectDirtyFlag::Transform) |
           flagValue(CreativeObjectDirtyFlag::Bounds) |
           flagValue(CreativeObjectDirtyFlag::Testing) |
           flagValue(CreativeObjectDirtyFlag::Preview);
}

constexpr CreativeObjectDirtyFlags sensoryCreationDirtyFlags(CreativeObjectDirtyFlag systemFlag) noexcept {
    return commonAuthoredDirtyFlags() |
           flagValue(CreativeObjectDirtyFlag::Transform) |
           flagValue(systemFlag);
}

constexpr CreativeObjectDefaults markerDefaults() noexcept {
    return CreativeObjectDefaults{};
}

constexpr CreativeObjectDefaults boxDefaults(double width, double height, double depth) noexcept {
    return CreativeObjectDefaults{
        CreativeTransform{},
        CreativeBounds{CreativeVec3{0.0, 0.0, 0.0}, CreativeVec3{width, height, depth}},
        0,
        true,
        false,
    };
}

constexpr CreativeObjectDescriptor descriptor(
    CreativeObjectKind kind,
    CreativeObjectCategory category,
    CreativeObjectProfile profile,
    std::string_view name,
    std::string_view displayName,
    std::string_view purpose,
    CreativeObjectDirtyFlags creationDirtyFlags,
    CreativeObjectDefaults defaults,
    bool hasTransform,
    bool hasBounds,
    bool canHaveParent,
    bool canOwnChildren,
    bool isRuntimeMeaningful,
    bool isEditorOnly) noexcept {
    return CreativeObjectDescriptor{
        kind,
        category,
        profile,
        name,
        displayName,
        purpose,
        creationDirtyFlags,
        defaults,
        hasTransform,
        hasBounds,
        canHaveParent,
        canOwnChildren,
        true,
        true,
        true,
        isRuntimeMeaningful,
        isEditorOnly,
    };
}

constexpr std::array<CreativeObjectDescriptor, 96> kDescriptors{{
    descriptor(CreativeObjectKind::Unknown, CreativeObjectCategory::Unknown, CreativeObjectProfile::Unknown, "Unknown", "Unknown", "invalid or unclassified creative object", 0, CreativeObjectDefaults{}, false, false, false, false, false, false),

    descriptor(CreativeObjectKind::Room, CreativeObjectCategory::Structural, CreativeObjectProfile::RoomContainer, "Room", "Room", "authored room volume or level container", structuralCreationDirtyFlags(), boxDefaults(10.0, 4.0, 10.0), false, true, false, true, true, false),
    descriptor(CreativeObjectKind::Wall, CreativeObjectCategory::Structural, CreativeObjectProfile::BoxStructural, "Wall", "Wall", "vertical structural surface", structuralCreationDirtyFlags(), boxDefaults(1.0, 3.0, 4.0), true, true, true, false, true, false),
    descriptor(CreativeObjectKind::Floor, CreativeObjectCategory::Structural, CreativeObjectProfile::BoxStructural, "Floor", "Floor", "walkable horizontal structural surface", structuralCreationDirtyFlags(), boxDefaults(4.0, 0.25, 4.0), true, true, true, false, true, false),
    descriptor(CreativeObjectKind::Ceiling, CreativeObjectCategory::Structural, CreativeObjectProfile::BoxStructural, "Ceiling", "Ceiling", "upper room boundary surface", structuralCreationDirtyFlags(), boxDefaults(4.0, 0.25, 4.0), true, true, true, false, true, false),
    descriptor(CreativeObjectKind::Roof, CreativeObjectCategory::Structural, CreativeObjectProfile::BoxStructural, "Roof", "Roof", "exterior upper structural cover", structuralCreationDirtyFlags(), boxDefaults(5.0, 1.0, 5.0), true, true, true, false, true, false),
    descriptor(CreativeObjectKind::Door, CreativeObjectCategory::Structural, CreativeObjectProfile::Attachment, "Door", "Door", "openable passage attachment", structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Logic), boxDefaults(1.0, 2.25, 0.2), true, true, true, false, true, false),
    descriptor(CreativeObjectKind::Window, CreativeObjectCategory::Structural, CreativeObjectProfile::Attachment, "Window", "Window", "wall opening or visual aperture", structuralCreationDirtyFlags(), boxDefaults(1.5, 1.0, 0.2), true, true, true, false, true, false),
    descriptor(CreativeObjectKind::Stair, CreativeObjectCategory::Structural, CreativeObjectProfile::BoxStructural, "Stair", "Stair", "stepped traversal structure", structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Navigation), boxDefaults(2.0, 1.0, 3.0), true, true, true, false, true, false),
    descriptor(CreativeObjectKind::Ramp, CreativeObjectCategory::Structural, CreativeObjectProfile::BoxStructural, "Ramp", "Ramp", "sloped traversal structure", structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Navigation), boxDefaults(2.0, 1.0, 4.0), true, true, true, false, true, false),
    descriptor(CreativeObjectKind::Platform, CreativeObjectCategory::Structural, CreativeObjectProfile::BoxStructural, "Platform", "Platform", "static traversal platform", structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Navigation), boxDefaults(3.0, 0.35, 3.0), true, true, true, false, true, false),
    descriptor(CreativeObjectKind::MovingPlatform, CreativeObjectCategory::Structural, CreativeObjectProfile::BoxStructural, "MovingPlatform", "Moving Platform", "animated traversal platform", structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Logic) | flagValue(CreativeObjectDirtyFlag::Navigation), boxDefaults(3.0, 0.35, 3.0), true, true, true, false, true, false),
    descriptor(CreativeObjectKind::Column, CreativeObjectCategory::Structural, CreativeObjectProfile::BoxStructural, "Column", "Column", "vertical support structure", structuralCreationDirtyFlags(), boxDefaults(0.75, 3.0, 0.75), true, true, true, false, true, false),
    descriptor(CreativeObjectKind::Pillar, CreativeObjectCategory::Structural, CreativeObjectProfile::BoxStructural, "Pillar", "Pillar", "thick vertical support structure", structuralCreationDirtyFlags(), boxDefaults(1.0, 3.0, 1.0), true, true, true, false, true, false),
    descriptor(CreativeObjectKind::Beam, CreativeObjectCategory::Structural, CreativeObjectProfile::BoxStructural, "Beam", "Beam", "horizontal support structure", structuralCreationDirtyFlags(), boxDefaults(4.0, 0.35, 0.35), true, true, true, false, true, false),
    descriptor(CreativeObjectKind::Arch, CreativeObjectCategory::Structural, CreativeObjectProfile::BoxStructural, "Arch", "Arch", "arched structural opening", structuralCreationDirtyFlags(), boxDefaults(3.0, 3.0, 0.5), true, true, true, false, true, false),
    descriptor(CreativeObjectKind::Fence, CreativeObjectCategory::Structural, CreativeObjectProfile::BoxStructural, "Fence", "Fence", "thin boundary structure", structuralCreationDirtyFlags(), boxDefaults(4.0, 1.25, 0.2), true, true, true, false, true, false),
    descriptor(CreativeObjectKind::Railing, CreativeObjectCategory::Structural, CreativeObjectProfile::BoxStructural, "Railing", "Railing", "edge safety or visual rail", structuralCreationDirtyFlags(), boxDefaults(4.0, 1.0, 0.2), true, true, true, false, true, false),
    descriptor(CreativeObjectKind::Bridge, CreativeObjectCategory::Structural, CreativeObjectProfile::BoxStructural, "Bridge", "Bridge", "traversable span between regions", structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Navigation), boxDefaults(3.0, 0.35, 8.0), true, true, true, false, true, false),
    descriptor(CreativeObjectKind::Ladder, CreativeObjectCategory::Structural, CreativeObjectProfile::BoxStructural, "Ladder", "Ladder", "vertical climb structure", structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Navigation), boxDefaults(0.75, 4.0, 0.2), true, true, true, false, true, false),

    descriptor(CreativeObjectKind::TerrainPatch, CreativeObjectCategory::TerrainOrVolume, CreativeObjectProfile::TerrainVolume, "TerrainPatch", "Terrain Patch", "authored terrain surface region", structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Navigation), boxDefaults(8.0, 0.5, 8.0), true, true, false, false, true, false),
    descriptor(CreativeObjectKind::WaterVolume, CreativeObjectCategory::TerrainOrVolume, CreativeObjectProfile::TerrainVolume, "WaterVolume", "Water Volume", "water gameplay or visual volume", structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay), boxDefaults(4.0, 1.0, 4.0), true, true, false, false, true, false),
    descriptor(CreativeObjectKind::LavaVolume, CreativeObjectCategory::TerrainOrVolume, CreativeObjectProfile::TerrainVolume, "LavaVolume", "Lava Volume", "hazardous liquid volume", structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay), boxDefaults(4.0, 1.0, 4.0), true, true, false, false, true, false),
    descriptor(CreativeObjectKind::Pit, CreativeObjectCategory::TerrainOrVolume, CreativeObjectProfile::TerrainVolume, "Pit", "Pit", "fall or hazard depression", structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay), boxDefaults(3.0, 3.0, 3.0), true, true, false, false, true, false),
    descriptor(CreativeObjectKind::Slope, CreativeObjectCategory::TerrainOrVolume, CreativeObjectProfile::TerrainVolume, "Slope", "Slope", "inclined terrain surface", structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Navigation), boxDefaults(3.0, 1.0, 4.0), true, true, false, false, true, false),
    descriptor(CreativeObjectKind::Cliff, CreativeObjectCategory::TerrainOrVolume, CreativeObjectProfile::TerrainVolume, "Cliff", "Cliff", "vertical terrain boundary", structuralCreationDirtyFlags(), boxDefaults(4.0, 4.0, 1.0), true, true, false, false, true, false),
    descriptor(CreativeObjectKind::CaveOpening, CreativeObjectCategory::TerrainOrVolume, CreativeObjectProfile::TerrainVolume, "CaveOpening", "Cave Opening", "terrain entrance aperture", structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Navigation), boxDefaults(3.0, 2.5, 1.0), true, true, false, false, true, false),
    descriptor(CreativeObjectKind::BoundaryVolume, CreativeObjectCategory::TerrainOrVolume, CreativeObjectProfile::Volume, "BoundaryVolume", "Boundary Volume", "world boundary or containment volume", structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay), boxDefaults(10.0, 4.0, 10.0), true, true, false, false, true, false),
    descriptor(CreativeObjectKind::KillPlane, CreativeObjectCategory::TerrainOrVolume, CreativeObjectProfile::Volume, "KillPlane", "Kill Plane", "fall reset or death boundary", logicCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Bounds), boxDefaults(10.0, 0.1, 10.0), true, true, false, false, true, false),

    descriptor(CreativeObjectKind::SpawnPoint, CreativeObjectCategory::NavigationOrMovement, CreativeObjectProfile::Marker, "SpawnPoint", "Spawn Point", "player or entity start marker", navigationCreationDirtyFlags(), markerDefaults(), true, false, false, false, true, false),
    descriptor(CreativeObjectKind::ExitPoint, CreativeObjectCategory::NavigationOrMovement, CreativeObjectProfile::Marker, "ExitPoint", "Exit Point", "level exit marker", navigationCreationDirtyFlags(), markerDefaults(), true, false, false, false, true, false),
    descriptor(CreativeObjectKind::EntrancePoint, CreativeObjectCategory::NavigationOrMovement, CreativeObjectProfile::Marker, "EntrancePoint", "Entrance Point", "level entrance marker", navigationCreationDirtyFlags(), markerDefaults(), true, false, false, false, true, false),
    descriptor(CreativeObjectKind::Checkpoint, CreativeObjectCategory::NavigationOrMovement, CreativeObjectProfile::Marker, "Checkpoint", "Checkpoint", "progress or respawn checkpoint", navigationCreationDirtyFlags(), markerDefaults(), true, false, false, false, true, false),
    descriptor(CreativeObjectKind::NavRegion, CreativeObjectCategory::NavigationOrMovement, CreativeObjectProfile::Volume, "NavRegion", "Navigation Region", "navigation cost or traversal region", navigationCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Bounds), boxDefaults(4.0, 2.0, 4.0), true, true, false, false, true, false),
    descriptor(CreativeObjectKind::NavLink, CreativeObjectCategory::NavigationOrMovement, CreativeObjectProfile::LinkOrRoute, "NavLink", "Navigation Link", "direct navigation connection", navigationCreationDirtyFlags(), markerDefaults(), false, false, false, false, true, false),
    descriptor(CreativeObjectKind::JumpLink, CreativeObjectCategory::NavigationOrMovement, CreativeObjectProfile::LinkOrRoute, "JumpLink", "Jump Link", "jump traversal connection", navigationCreationDirtyFlags(), markerDefaults(), false, false, false, false, true, false),
    descriptor(CreativeObjectKind::ClimbLink, CreativeObjectCategory::NavigationOrMovement, CreativeObjectProfile::LinkOrRoute, "ClimbLink", "Climb Link", "climb traversal connection", navigationCreationDirtyFlags(), markerDefaults(), false, false, false, false, true, false),
    descriptor(CreativeObjectKind::WallRunSurface, CreativeObjectCategory::NavigationOrMovement, CreativeObjectProfile::MovementTest, "WallRunSurface", "Wall Run Surface", "wall-runable surface with movement tuning rules", testingCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Navigation), boxDefaults(0.25, 3.0, 6.0), true, true, false, false, true, false),
    descriptor(CreativeObjectKind::SlideSurface, CreativeObjectCategory::NavigationOrMovement, CreativeObjectProfile::MovementTest, "SlideSurface", "Slide Surface", "slideable surface with movement tuning rules", testingCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Navigation), boxDefaults(3.0, 0.25, 6.0), true, true, false, false, true, false),
    descriptor(CreativeObjectKind::CoverPoint, CreativeObjectCategory::NavigationOrMovement, CreativeObjectProfile::Marker, "CoverPoint", "Cover Point", "AI cover marker", navigationCreationDirtyFlags(), markerDefaults(), true, false, false, false, true, false),
    descriptor(CreativeObjectKind::PatrolNode, CreativeObjectCategory::NavigationOrMovement, CreativeObjectProfile::Marker, "PatrolNode", "Patrol Node", "AI patrol path point", navigationCreationDirtyFlags(), markerDefaults(), true, false, false, false, true, false),

    descriptor(CreativeObjectKind::TriggerZone, CreativeObjectCategory::Logic, CreativeObjectProfile::LogicNode, "TriggerZone", "Trigger Zone", "event trigger volume", logicCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Bounds), boxDefaults(2.0, 2.0, 2.0), true, true, false, false, true, false),
    descriptor(CreativeObjectKind::Switch, CreativeObjectCategory::Logic, CreativeObjectProfile::LogicNode, "Switch", "Switch", "binary interaction node", logicCreationDirtyFlags(), markerDefaults(), true, false, true, false, true, false),
    descriptor(CreativeObjectKind::Lever, CreativeObjectCategory::Logic, CreativeObjectProfile::LogicNode, "Lever", "Lever", "directional interaction node", logicCreationDirtyFlags(), markerDefaults(), true, false, true, false, true, false),
    descriptor(CreativeObjectKind::PressurePlate, CreativeObjectCategory::Logic, CreativeObjectProfile::LogicNode, "PressurePlate", "Pressure Plate", "weight or presence activated trigger", logicCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Bounds), boxDefaults(1.5, 0.1, 1.5), true, true, false, false, true, false),
    descriptor(CreativeObjectKind::Button, CreativeObjectCategory::Logic, CreativeObjectProfile::LogicNode, "Button", "Button", "press interaction node", logicCreationDirtyFlags(), markerDefaults(), true, false, true, false, true, false),
    descriptor(CreativeObjectKind::ConditionGate, CreativeObjectCategory::Logic, CreativeObjectProfile::LogicNode, "ConditionGate", "Condition Gate", "logic condition checkpoint", logicCreationDirtyFlags(), markerDefaults(), false, false, false, false, true, false),
    descriptor(CreativeObjectKind::EventRelay, CreativeObjectCategory::Logic, CreativeObjectProfile::LogicNode, "EventRelay", "Event Relay", "event forwarding node", logicCreationDirtyFlags(), markerDefaults(), false, false, false, false, true, false),
    descriptor(CreativeObjectKind::Spawner, CreativeObjectCategory::Logic, CreativeObjectProfile::LogicNode, "Spawner", "Spawner", "runtime entity spawn rule", logicCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay), markerDefaults(), true, false, false, false, true, false),
    descriptor(CreativeObjectKind::DespawnZone, CreativeObjectCategory::Logic, CreativeObjectProfile::LogicNode, "DespawnZone", "Despawn Zone", "runtime entity cleanup volume", logicCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Bounds), boxDefaults(2.0, 2.0, 2.0), true, true, false, false, true, false),
    descriptor(CreativeObjectKind::ScriptMarker, CreativeObjectCategory::Logic, CreativeObjectProfile::LogicNode, "ScriptMarker", "Script Marker", "script hook marker", logicCreationDirtyFlags(), markerDefaults(), true, false, false, false, true, false),

    descriptor(CreativeObjectKind::TestLane, CreativeObjectCategory::Testing, CreativeObjectProfile::MovementTest, "TestLane", "Test Lane", "movement lab lane container", testingCreationDirtyFlags(), boxDefaults(3.0, 0.2, 12.0), true, true, false, true, false, false),
    descriptor(CreativeObjectKind::DistanceMarker, CreativeObjectCategory::Testing, CreativeObjectProfile::MovementTest, "DistanceMarker", "Distance Marker", "known-distance measurement marker", testingCreationDirtyFlags(), markerDefaults(), true, false, true, false, false, false),
    descriptor(CreativeObjectKind::SpeedMarker, CreativeObjectCategory::Testing, CreativeObjectProfile::MovementTest, "SpeedMarker", "Speed Marker", "speed measurement marker", testingCreationDirtyFlags(), markerDefaults(), true, false, true, false, false, false),
    descriptor(CreativeObjectKind::JumpTarget, CreativeObjectCategory::Testing, CreativeObjectProfile::MovementTest, "JumpTarget", "Jump Target", "jump tuning target", testingCreationDirtyFlags(), boxDefaults(1.0, 0.2, 1.0), true, true, true, false, false, false),
    descriptor(CreativeObjectKind::CoyoteTimeLedge, CreativeObjectCategory::Testing, CreativeObjectProfile::MovementTest, "CoyoteTimeLedge", "Coyote-Time Ledge", "ledge for coyote-time tuning", testingCreationDirtyFlags(), boxDefaults(2.0, 0.25, 2.0), true, true, true, false, false, false),
    descriptor(CreativeObjectKind::FallShaft, CreativeObjectCategory::Testing, CreativeObjectProfile::MovementTest, "FallShaft", "Fall Shaft", "fall/gravity tuning shaft", testingCreationDirtyFlags(), boxDefaults(2.0, 8.0, 2.0), true, true, true, false, false, false),
    descriptor(CreativeObjectKind::CollisionProbe, CreativeObjectCategory::Testing, CreativeObjectProfile::MovementTest, "CollisionProbe", "Collision Probe", "collision behavior test probe", testingCreationDirtyFlags(), markerDefaults(), true, false, false, false, false, false),
    descriptor(CreativeObjectKind::PhysicsProbe, CreativeObjectCategory::Testing, CreativeObjectProfile::MovementTest, "PhysicsProbe", "Physics Probe", "physics behavior test probe", testingCreationDirtyFlags(), markerDefaults(), true, false, false, false, false, false),
    descriptor(CreativeObjectKind::TimingGate, CreativeObjectCategory::Testing, CreativeObjectProfile::MovementTest, "TimingGate", "Timing Gate", "timing window measurement gate", testingCreationDirtyFlags(), boxDefaults(2.0, 2.0, 0.2), true, true, true, false, false, false),
    descriptor(CreativeObjectKind::TestStart, CreativeObjectCategory::Testing, CreativeObjectProfile::MovementTest, "TestStart", "Test Start", "test route start marker", testingCreationDirtyFlags(), markerDefaults(), true, false, true, false, false, false),
    descriptor(CreativeObjectKind::TestEnd, CreativeObjectCategory::Testing, CreativeObjectProfile::MovementTest, "TestEnd", "Test End", "test route end marker", testingCreationDirtyFlags(), markerDefaults(), true, false, true, false, false, false),

    descriptor(CreativeObjectKind::Prop, CreativeObjectCategory::VisualDressing, CreativeObjectProfile::Dressing, "Prop", "Prop", "generic visual object", commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Transform) | flagValue(CreativeObjectDirtyFlag::Bounds), boxDefaults(1.0, 1.0, 1.0), true, true, true, true, false, false),
    descriptor(CreativeObjectKind::Decal, CreativeObjectCategory::VisualDressing, CreativeObjectProfile::Dressing, "Decal", "Decal", "surface visual decal", commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Transform), boxDefaults(1.0, 0.01, 1.0), true, true, true, false, false, false),
    descriptor(CreativeObjectKind::Sign, CreativeObjectCategory::VisualDressing, CreativeObjectProfile::Dressing, "Sign", "Sign", "visual sign object", commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Transform), boxDefaults(1.5, 1.0, 0.1), true, true, true, false, false, false),
    descriptor(CreativeObjectKind::Banner, CreativeObjectCategory::VisualDressing, CreativeObjectProfile::Dressing, "Banner", "Banner", "hanging visual banner", commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Transform), boxDefaults(1.5, 2.0, 0.05), true, true, true, false, false, false),
    descriptor(CreativeObjectKind::FoliagePatch, CreativeObjectCategory::VisualDressing, CreativeObjectProfile::Dressing, "FoliagePatch", "Foliage Patch", "visual vegetation patch", commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Transform) | flagValue(CreativeObjectDirtyFlag::Bounds), boxDefaults(2.0, 1.0, 2.0), true, true, false, false, false, false),
    descriptor(CreativeObjectKind::Rock, CreativeObjectCategory::VisualDressing, CreativeObjectProfile::Dressing, "Rock", "Rock", "visual or collision rock prop", structuralCreationDirtyFlags(), boxDefaults(1.0, 1.0, 1.0), true, true, false, false, false, false),
    descriptor(CreativeObjectKind::Crate, CreativeObjectCategory::VisualDressing, CreativeObjectProfile::Dressing, "Crate", "Crate", "box prop", structuralCreationDirtyFlags(), boxDefaults(1.0, 1.0, 1.0), true, true, false, false, false, false),
    descriptor(CreativeObjectKind::Barrel, CreativeObjectCategory::VisualDressing, CreativeObjectProfile::Dressing, "Barrel", "Barrel", "barrel prop", structuralCreationDirtyFlags(), boxDefaults(0.8, 1.2, 0.8), true, true, false, false, false, false),
    descriptor(CreativeObjectKind::Furniture, CreativeObjectCategory::VisualDressing, CreativeObjectProfile::Dressing, "Furniture", "Furniture", "furniture prop", structuralCreationDirtyFlags(), boxDefaults(2.0, 1.0, 1.0), true, true, false, false, false, false),
    descriptor(CreativeObjectKind::Decoration, CreativeObjectCategory::VisualDressing, CreativeObjectProfile::Dressing, "Decoration", "Decoration", "generic decoration object", commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Transform), markerDefaults(), true, false, true, false, false, false),

    descriptor(CreativeObjectKind::PointLight, CreativeObjectCategory::LightSoundOrCamera, CreativeObjectProfile::Light, "PointLight", "Point Light", "omnidirectional light emitter", sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Lighting), markerDefaults(), true, false, false, false, false, false),
    descriptor(CreativeObjectKind::SpotLight, CreativeObjectCategory::LightSoundOrCamera, CreativeObjectProfile::Light, "SpotLight", "Spot Light", "directional cone light emitter", sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Lighting), markerDefaults(), true, false, false, false, false, false),
    descriptor(CreativeObjectKind::AreaLight, CreativeObjectCategory::LightSoundOrCamera, CreativeObjectProfile::Light, "AreaLight", "Area Light", "area-based light emitter", sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Lighting) | flagValue(CreativeObjectDirtyFlag::Bounds), boxDefaults(2.0, 0.1, 2.0), true, true, false, false, false, false),
    descriptor(CreativeObjectKind::AmbientZone, CreativeObjectCategory::LightSoundOrCamera, CreativeObjectProfile::Light, "AmbientZone", "Ambient Zone", "ambient lighting volume", sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Lighting) | flagValue(CreativeObjectDirtyFlag::Bounds), boxDefaults(4.0, 3.0, 4.0), true, true, false, false, false, false),
    descriptor(CreativeObjectKind::ReverbZone, CreativeObjectCategory::LightSoundOrCamera, CreativeObjectProfile::Audio, "ReverbZone", "Reverb Zone", "audio reverb volume", sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Audio) | flagValue(CreativeObjectDirtyFlag::Bounds), boxDefaults(4.0, 3.0, 4.0), true, true, false, false, false, false),
    descriptor(CreativeObjectKind::SoundEmitter, CreativeObjectCategory::LightSoundOrCamera, CreativeObjectProfile::Audio, "SoundEmitter", "Sound Emitter", "positional sound source", sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Audio), markerDefaults(), true, false, false, false, false, false),
    descriptor(CreativeObjectKind::MusicZone, CreativeObjectCategory::LightSoundOrCamera, CreativeObjectProfile::Audio, "MusicZone", "Music Zone", "music cue trigger volume", sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Audio) | flagValue(CreativeObjectDirtyFlag::Bounds), boxDefaults(4.0, 3.0, 4.0), true, true, false, false, false, false),
    descriptor(CreativeObjectKind::CameraMarker, CreativeObjectCategory::LightSoundOrCamera, CreativeObjectProfile::Camera, "CameraMarker", "Camera Marker", "camera placement marker", sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Camera), markerDefaults(), true, false, false, false, false, false),
    descriptor(CreativeObjectKind::CameraRail, CreativeObjectCategory::LightSoundOrCamera, CreativeObjectProfile::Camera, "CameraRail", "Camera Rail", "camera path or dolly rail", sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Camera) | flagValue(CreativeObjectDirtyFlag::Bounds), boxDefaults(1.0, 1.0, 4.0), true, true, false, true, false, false),
    descriptor(CreativeObjectKind::CameraTarget, CreativeObjectCategory::LightSoundOrCamera, CreativeObjectProfile::Camera, "CameraTarget", "Camera Target", "camera look-at target", sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Camera), markerDefaults(), true, false, false, false, false, false),
    descriptor(CreativeObjectKind::CutsceneMarker, CreativeObjectCategory::LightSoundOrCamera, CreativeObjectProfile::Camera, "CutsceneMarker", "Cutscene Marker", "cutscene timing or camera marker", sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Camera) | flagValue(CreativeObjectDirtyFlag::Logic), markerDefaults(), true, false, false, false, true, false),

    descriptor(CreativeObjectKind::Note, CreativeObjectCategory::AuthoringMeta, CreativeObjectProfile::TextAnnotation, "Note", "Note", "authoring note", commonAuthoredDirtyFlags(), markerDefaults(), true, false, false, false, false, true),
    descriptor(CreativeObjectKind::Label, CreativeObjectCategory::AuthoringMeta, CreativeObjectProfile::TextAnnotation, "Label", "Label", "visible authoring label", commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Preview), markerDefaults(), true, false, false, false, false, true),
    descriptor(CreativeObjectKind::Comment, CreativeObjectCategory::AuthoringMeta, CreativeObjectProfile::TextAnnotation, "Comment", "Comment", "review or design comment", commonAuthoredDirtyFlags(), markerDefaults(), true, false, false, false, false, true),
    descriptor(CreativeObjectKind::MeasurementMarker, CreativeObjectCategory::AuthoringMeta, CreativeObjectProfile::AuthoringHelper, "MeasurementMarker", "Measurement Marker", "single-point measurement helper", testingCreationDirtyFlags(), markerDefaults(), true, false, false, false, false, true),
    descriptor(CreativeObjectKind::MeasurementLine, CreativeObjectCategory::AuthoringMeta, CreativeObjectProfile::AuthoringHelper, "MeasurementLine", "Measurement Line", "line measurement helper", testingCreationDirtyFlags(), boxDefaults(1.0, 0.05, 0.05), true, true, false, false, false, true),
    descriptor(CreativeObjectKind::MeasurementBox, CreativeObjectCategory::AuthoringMeta, CreativeObjectProfile::AuthoringHelper, "MeasurementBox", "Measurement Box", "box measurement helper", testingCreationDirtyFlags(), boxDefaults(1.0, 1.0, 1.0), true, true, false, false, false, true),
    descriptor(CreativeObjectKind::GridAnchor, CreativeObjectCategory::AuthoringMeta, CreativeObjectProfile::AuthoringHelper, "GridAnchor", "Grid Anchor", "grid alignment anchor", commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Preview), markerDefaults(), true, false, false, false, false, true),
    descriptor(CreativeObjectKind::SnapAnchor, CreativeObjectCategory::AuthoringMeta, CreativeObjectProfile::AuthoringHelper, "SnapAnchor", "Snap Anchor", "snap alignment anchor", commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Preview), markerDefaults(), true, false, false, false, false, true),
    descriptor(CreativeObjectKind::ReferenceImage, CreativeObjectCategory::AuthoringMeta, CreativeObjectProfile::AuthoringHelper, "ReferenceImage", "Reference Image", "placed reference image plane", commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Preview), boxDefaults(4.0, 0.01, 4.0), true, true, false, false, false, true),
    descriptor(CreativeObjectKind::BlueprintOverlay, CreativeObjectCategory::AuthoringMeta, CreativeObjectProfile::AuthoringHelper, "BlueprintOverlay", "Blueprint Overlay", "transparent planning overlay", commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Preview), boxDefaults(4.0, 0.01, 4.0), true, true, false, false, false, true),
    descriptor(CreativeObjectKind::Group, CreativeObjectCategory::AuthoringMeta, CreativeObjectProfile::AuthoringHelper, "Group", "Group", "object grouping container", commonAuthoredDirtyFlags(), markerDefaults(), true, false, true, true, false, false),
    descriptor(CreativeObjectKind::PrefabInstance, CreativeObjectCategory::AuthoringMeta, CreativeObjectProfile::Dressing, "PrefabInstance", "Prefab Instance", "placed reusable authored package", structuralCreationDirtyFlags(), boxDefaults(1.0, 1.0, 1.0), true, true, true, true, true, false),
    descriptor(CreativeObjectKind::Socket, CreativeObjectCategory::AuthoringMeta, CreativeObjectProfile::Attachment, "Socket", "Socket", "named attachment location", commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Transform), markerDefaults(), true, false, true, false, false, false),
    descriptor(CreativeObjectKind::AttachmentPoint, CreativeObjectCategory::AuthoringMeta, CreativeObjectProfile::Attachment, "AttachmentPoint", "Attachment Point", "generic attachment location", commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Transform), markerDefaults(), true, false, true, false, false, false),

    descriptor(CreativeObjectKind::EnemySpawn, CreativeObjectCategory::Gameplay, CreativeObjectProfile::GameplayMarker, "EnemySpawn", "Enemy Spawn", "enemy spawn marker", navigationCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay), markerDefaults(), true, false, false, false, true, false),
    descriptor(CreativeObjectKind::NpcSpawn, CreativeObjectCategory::Gameplay, CreativeObjectProfile::GameplayMarker, "NpcSpawn", "NPC Spawn", "NPC spawn marker", navigationCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay), markerDefaults(), true, false, false, false, true, false),
    descriptor(CreativeObjectKind::PatrolRoute, CreativeObjectCategory::Gameplay, CreativeObjectProfile::LinkOrRoute, "PatrolRoute", "Patrol Route", "AI patrol route", navigationCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay), markerDefaults(), false, false, false, true, true, false),
    descriptor(CreativeObjectKind::InterestPoint, CreativeObjectCategory::Gameplay, CreativeObjectProfile::GameplayMarker, "InterestPoint", "Interest Point", "AI interest marker", navigationCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay), markerDefaults(), true, false, false, false, true, false),
    descriptor(CreativeObjectKind::AlertZone, CreativeObjectCategory::Gameplay, CreativeObjectProfile::Volume, "AlertZone", "Alert Zone", "AI alert behavior volume", logicCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Bounds), boxDefaults(4.0, 2.0, 4.0), true, true, false, false, true, false),
    descriptor(CreativeObjectKind::SafeZone, CreativeObjectCategory::Gameplay, CreativeObjectProfile::Volume, "SafeZone", "Safe Zone", "safe gameplay volume", logicCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Bounds), boxDefaults(4.0, 2.0, 4.0), true, true, false, false, true, false),
    descriptor(CreativeObjectKind::DangerZone, CreativeObjectCategory::Gameplay, CreativeObjectProfile::Volume, "DangerZone", "Danger Zone", "danger gameplay volume", logicCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Bounds), boxDefaults(4.0, 2.0, 4.0), true, true, false, false, true, false),
    descriptor(CreativeObjectKind::ResourceNode, CreativeObjectCategory::Gameplay, CreativeObjectProfile::GameplayMarker, "ResourceNode", "Resource Node", "resource gathering marker", navigationCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay), markerDefaults(), true, false, false, false, true, false),
    descriptor(CreativeObjectKind::LootPoint, CreativeObjectCategory::Gameplay, CreativeObjectProfile::GameplayMarker, "LootPoint", "Loot Point", "loot placement marker", navigationCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay), markerDefaults(), true, false, false, false, true, false),
    descriptor(CreativeObjectKind::QuestMarker, CreativeObjectCategory::Gameplay, CreativeObjectProfile::GameplayMarker, "QuestMarker", "Quest Marker", "quest objective marker", navigationCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay), markerDefaults(), true, false, false, false, true, false),
    descriptor(CreativeObjectKind::DialogueMarker, CreativeObjectCategory::Gameplay, CreativeObjectProfile::GameplayMarker, "DialogueMarker", "Dialogue Marker", "dialogue trigger or speaker marker", navigationCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay), markerDefaults(), true, false, false, false, true, false),
}};

} // namespace

CreativeObjectDirtyFlags operator|(CreativeObjectDirtyFlag lhs, CreativeObjectDirtyFlag rhs) noexcept {
    return flagValue(lhs) | flagValue(rhs);
}

CreativeObjectDirtyFlags operator|(CreativeObjectDirtyFlags lhs, CreativeObjectDirtyFlag rhs) noexcept {
    return lhs | flagValue(rhs);
}

bool hasDirtyFlag(CreativeObjectDirtyFlags flags, CreativeObjectDirtyFlag flag) noexcept {
    return (flags & flagValue(flag)) != 0;
}

std::string_view toString(CreativeObjectCategory category) noexcept {
    switch (category) {
    case CreativeObjectCategory::Unknown: return "Unknown";
    case CreativeObjectCategory::Structural: return "Structural";
    case CreativeObjectCategory::TerrainOrVolume: return "TerrainOrVolume";
    case CreativeObjectCategory::NavigationOrMovement: return "NavigationOrMovement";
    case CreativeObjectCategory::Logic: return "Logic";
    case CreativeObjectCategory::Testing: return "Testing";
    case CreativeObjectCategory::VisualDressing: return "VisualDressing";
    case CreativeObjectCategory::LightSoundOrCamera: return "LightSoundOrCamera";
    case CreativeObjectCategory::AuthoringMeta: return "AuthoringMeta";
    case CreativeObjectCategory::Gameplay: return "Gameplay";
    }

    return "Unknown";
}

std::string_view toString(CreativeObjectProfile profile) noexcept {
    switch (profile) {
    case CreativeObjectProfile::Unknown: return "Unknown";
    case CreativeObjectProfile::RoomContainer: return "RoomContainer";
    case CreativeObjectProfile::BoxStructural: return "BoxStructural";
    case CreativeObjectProfile::Attachment: return "Attachment";
    case CreativeObjectProfile::TerrainVolume: return "TerrainVolume";
    case CreativeObjectProfile::Marker: return "Marker";
    case CreativeObjectProfile::Volume: return "Volume";
    case CreativeObjectProfile::LinkOrRoute: return "LinkOrRoute";
    case CreativeObjectProfile::LogicNode: return "LogicNode";
    case CreativeObjectProfile::MovementTest: return "MovementTest";
    case CreativeObjectProfile::Dressing: return "Dressing";
    case CreativeObjectProfile::Light: return "Light";
    case CreativeObjectProfile::Audio: return "Audio";
    case CreativeObjectProfile::Camera: return "Camera";
    case CreativeObjectProfile::TextAnnotation: return "TextAnnotation";
    case CreativeObjectProfile::AuthoringHelper: return "AuthoringHelper";
    case CreativeObjectProfile::GameplayMarker: return "GameplayMarker";
    }

    return "Unknown";
}

std::string_view toString(CreativeObjectDirtyFlag flag) noexcept {
    switch (flag) {
    case CreativeObjectDirtyFlag::None: return "None";
    case CreativeObjectDirtyFlag::Identity: return "Identity";
    case CreativeObjectDirtyFlag::Transform: return "Transform";
    case CreativeObjectDirtyFlag::Bounds: return "Bounds";
    case CreativeObjectDirtyFlag::Geometry: return "Geometry";
    case CreativeObjectDirtyFlag::Collision: return "Collision";
    case CreativeObjectDirtyFlag::Navigation: return "Navigation";
    case CreativeObjectDirtyFlag::Logic: return "Logic";
    case CreativeObjectDirtyFlag::Lighting: return "Lighting";
    case CreativeObjectDirtyFlag::Audio: return "Audio";
    case CreativeObjectDirtyFlag::Camera: return "Camera";
    case CreativeObjectDirtyFlag::Gameplay: return "Gameplay";
    case CreativeObjectDirtyFlag::Testing: return "Testing";
    case CreativeObjectDirtyFlag::Preview: return "Preview";
    case CreativeObjectDirtyFlag::Serialization: return "Serialization";
    }

    return "None";
}

const CreativeObjectDescriptor& describeObject(CreativeObjectKind kind) noexcept {
    for (const auto& candidate : kDescriptors) {
        if (candidate.kind == kind) {
            return candidate;
        }
    }

    return kDescriptors.front();
}

std::span<const CreativeObjectDescriptor> allObjectDescriptors() noexcept {
    return std::span<const CreativeObjectDescriptor>{kDescriptors.data(), kDescriptors.size()};
}

CreativeObjectCategory categoryOf(CreativeObjectKind kind) noexcept {
    return describeObject(kind).category;
}

CreativeObjectProfile profileOf(CreativeObjectKind kind) noexcept {
    return describeObject(kind).profile;
}

CreativeObjectDirtyFlags dirtyFlagsForCreation(CreativeObjectKind kind) noexcept {
    return describeObject(kind).creationDirtyFlags;
}

CreativeObjectDirtyFlags dirtyFlagsForMutation(CreativeObjectKind objectKind, CreativeMutationKind mutationKind) noexcept {
    auto flags = commonAuthoredDirtyFlags();
    const auto& descriptor = describeObject(objectKind);

    if (isTransformMutation(mutationKind)) {
        flags = flags | CreativeObjectDirtyFlag::Transform | CreativeObjectDirtyFlag::Preview;
        if (descriptor.hasBounds) {
            flags = flags | CreativeObjectDirtyFlag::Bounds;
        }
        if (descriptor.category == CreativeObjectCategory::Structural || descriptor.profile == CreativeObjectProfile::TerrainVolume) {
            flags = flags | CreativeObjectDirtyFlag::Geometry | CreativeObjectDirtyFlag::Collision;
        }
    }

    if (isShapeMutation(mutationKind)) {
        flags = flags | CreativeObjectDirtyFlag::Bounds | CreativeObjectDirtyFlag::Geometry | CreativeObjectDirtyFlag::Preview;
        if (descriptor.category == CreativeObjectCategory::Structural || descriptor.profile == CreativeObjectProfile::TerrainVolume || descriptor.profile == CreativeObjectProfile::Volume) {
            flags = flags | CreativeObjectDirtyFlag::Collision;
        }
    }

    if (isRelationshipMutation(mutationKind)) {
        flags = flags | CreativeObjectDirtyFlag::Logic | CreativeObjectDirtyFlag::Gameplay;
    }

    if (isLogicMutation(mutationKind)) {
        flags = flags | CreativeObjectDirtyFlag::Logic | CreativeObjectDirtyFlag::Gameplay;
    }

    if (isNavigationMutation(mutationKind)) {
        flags = flags | CreativeObjectDirtyFlag::Navigation | CreativeObjectDirtyFlag::Gameplay;
    }

    if (isTestingMutation(mutationKind)) {
        flags = flags | CreativeObjectDirtyFlag::Testing | CreativeObjectDirtyFlag::Preview;
    }

    if (isSensoryMutation(mutationKind)) {
        flags = flags | CreativeObjectDirtyFlag::Preview;
        if (descriptor.profile == CreativeObjectProfile::Light) {
            flags = flags | CreativeObjectDirtyFlag::Lighting;
        }
        if (descriptor.profile == CreativeObjectProfile::Audio) {
            flags = flags | CreativeObjectDirtyFlag::Audio;
        }
        if (descriptor.profile == CreativeObjectProfile::Camera) {
            flags = flags | CreativeObjectDirtyFlag::Camera;
        }
    }

    if (isGameplayMutation(mutationKind)) {
        flags = flags | CreativeObjectDirtyFlag::Gameplay;
    }

    if (mutationKind == CreativeMutationKind::SetVisible || mutationKind == CreativeMutationKind::SetLocked || mutationKind == CreativeMutationKind::Rename) {
        flags = flags | CreativeObjectDirtyFlag::Identity | CreativeObjectDirtyFlag::Preview;
    }

    return flags;
}

bool objectHasTransform(CreativeObjectKind kind) noexcept {
    return describeObject(kind).hasTransform;
}

bool objectHasBounds(CreativeObjectKind kind) noexcept {
    return describeObject(kind).hasBounds;
}

bool objectCanHaveParent(CreativeObjectKind kind) noexcept {
    return describeObject(kind).canHaveParent;
}

bool objectCanOwnChildren(CreativeObjectKind kind) noexcept {
    return describeObject(kind).canOwnChildren;
}

bool objectIsRuntimeMeaningful(CreativeObjectKind kind) noexcept {
    return describeObject(kind).isRuntimeMeaningful;
}

bool objectIsEditorOnly(CreativeObjectKind kind) noexcept {
    return describeObject(kind).isEditorOnly;
}

bool objectUsesProfile(CreativeObjectKind kind, CreativeObjectProfile profile) noexcept {
    return profileOf(kind) == profile;
}

bool objectUsesCategory(CreativeObjectKind kind, CreativeObjectCategory category) noexcept {
    return categoryOf(kind) == category;
}

bool descriptorAllowsMutation(CreativeObjectKind objectKind, CreativeMutationKind mutationKind) noexcept {
    if (!canMutate(objectKind, mutationKind)) {
        return false;
    }

    const auto& descriptor = describeObject(objectKind);

    if (isTransformMutation(mutationKind) && !descriptor.hasTransform) {
        return false;
    }

    if (isShapeMutation(mutationKind) && !descriptor.hasBounds) {
        return false;
    }

    if ((mutationKind == CreativeMutationKind::SetParent || mutationKind == CreativeMutationKind::AttachTo) && !descriptor.canHaveParent) {
        return false;
    }

    return true;
}

} // namespace iggy3d::creative