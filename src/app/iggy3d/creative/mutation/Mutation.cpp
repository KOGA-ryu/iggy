

#include "app/iggy3d/creative/mutation/Mutation.hpp"

#include <algorithm>
#include <utility>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool containsMutation(const std::vector<CreativeMutationKind>& mutations, CreativeMutationKind kind) noexcept {
    return std::find(mutations.begin(), mutations.end(), kind) != mutations.end();
}

[[nodiscard]] std::vector<CreativeMutationKind> commonIdentityMutations() {
    return {
        CreativeMutationKind::Rename,
        CreativeMutationKind::SetVisible,
        CreativeMutationKind::SetLocked,
        CreativeMutationKind::AssignLayer,
        CreativeMutationKind::AddTag,
        CreativeMutationKind::RemoveTag,
        CreativeMutationKind::ClearTags,
    };
}

void appendTransformMutations(std::vector<CreativeMutationKind>& mutations) {
    mutations.push_back(CreativeMutationKind::Move);
    mutations.push_back(CreativeMutationKind::Rotate);
    mutations.push_back(CreativeMutationKind::Scale);
    mutations.push_back(CreativeMutationKind::SetTransform);
}

void appendBoxShapeMutations(std::vector<CreativeMutationKind>& mutations) {
    mutations.push_back(CreativeMutationKind::Resize);
    mutations.push_back(CreativeMutationKind::Stretch);
    mutations.push_back(CreativeMutationKind::SetBounds);
    mutations.push_back(CreativeMutationKind::SetHeight);
    mutations.push_back(CreativeMutationKind::SetLength);
    mutations.push_back(CreativeMutationKind::SetWidth);
    mutations.push_back(CreativeMutationKind::SetDepth);
}

void appendRelationshipMutations(std::vector<CreativeMutationKind>& mutations) {
    mutations.push_back(CreativeMutationKind::SetParent);
    mutations.push_back(CreativeMutationKind::ClearParent);
    mutations.push_back(CreativeMutationKind::AttachTo);
    mutations.push_back(CreativeMutationKind::DetachFrom);
    mutations.push_back(CreativeMutationKind::LinkTarget);
    mutations.push_back(CreativeMutationKind::UnlinkTarget);
    mutations.push_back(CreativeMutationKind::SetSocket);
    mutations.push_back(CreativeMutationKind::ClearSocket);
}

} // namespace

std::string_view toString(CreativeMutationCategory category) noexcept {
    switch (category) {
    case CreativeMutationCategory::Unknown: return "Unknown";
    case CreativeMutationCategory::Identity: return "Identity";
    case CreativeMutationCategory::Transform: return "Transform";
    case CreativeMutationCategory::Shape: return "Shape";
    case CreativeMutationCategory::Relationship: return "Relationship";
    case CreativeMutationCategory::Organization: return "Organization";
    case CreativeMutationCategory::Content: return "Content";
    case CreativeMutationCategory::Logic: return "Logic";
    case CreativeMutationCategory::Navigation: return "Navigation";
    case CreativeMutationCategory::Testing: return "Testing";
    case CreativeMutationCategory::Sensory: return "Sensory";
    case CreativeMutationCategory::Gameplay: return "Gameplay";
    }

    return "Unknown";
}

std::string_view toString(CreativeMutationKind kind) noexcept {
    switch (kind) {
    case CreativeMutationKind::Unknown: return "Unknown";

    case CreativeMutationKind::Rename: return "Rename";
    case CreativeMutationKind::SetVisible: return "SetVisible";
    case CreativeMutationKind::SetLocked: return "SetLocked";

    case CreativeMutationKind::Move: return "Move";
    case CreativeMutationKind::Rotate: return "Rotate";
    case CreativeMutationKind::Scale: return "Scale";
    case CreativeMutationKind::SetTransform: return "SetTransform";

    case CreativeMutationKind::Resize: return "Resize";
    case CreativeMutationKind::Stretch: return "Stretch";
    case CreativeMutationKind::SetBounds: return "SetBounds";
    case CreativeMutationKind::SetHeight: return "SetHeight";
    case CreativeMutationKind::SetRadius: return "SetRadius";
    case CreativeMutationKind::SetThickness: return "SetThickness";
    case CreativeMutationKind::SetLength: return "SetLength";
    case CreativeMutationKind::SetWidth: return "SetWidth";
    case CreativeMutationKind::SetDepth: return "SetDepth";

    case CreativeMutationKind::SetParent: return "SetParent";
    case CreativeMutationKind::ClearParent: return "ClearParent";
    case CreativeMutationKind::AttachTo: return "AttachTo";
    case CreativeMutationKind::DetachFrom: return "DetachFrom";
    case CreativeMutationKind::LinkTarget: return "LinkTarget";
    case CreativeMutationKind::UnlinkTarget: return "UnlinkTarget";
    case CreativeMutationKind::SetSocket: return "SetSocket";
    case CreativeMutationKind::ClearSocket: return "ClearSocket";

    case CreativeMutationKind::AssignLayer: return "AssignLayer";
    case CreativeMutationKind::AddTag: return "AddTag";
    case CreativeMutationKind::RemoveTag: return "RemoveTag";
    case CreativeMutationKind::ClearTags: return "ClearTags";

    case CreativeMutationKind::EditText: return "EditText";
    case CreativeMutationKind::SetLabel: return "SetLabel";
    case CreativeMutationKind::SetNotes: return "SetNotes";
    case CreativeMutationKind::SetReferenceSource: return "SetReferenceSource";
    case CreativeMutationKind::SetBlueprintOpacity: return "SetBlueprintOpacity";

    case CreativeMutationKind::SetTriggerShape: return "SetTriggerShape";
    case CreativeMutationKind::SetTriggerEvent: return "SetTriggerEvent";
    case CreativeMutationKind::SetCondition: return "SetCondition";
    case CreativeMutationKind::SetEventRelayTarget: return "SetEventRelayTarget";
    case CreativeMutationKind::SetSpawnerProfile: return "SetSpawnerProfile";
    case CreativeMutationKind::SetDespawnRule: return "SetDespawnRule";

    case CreativeMutationKind::SetSpawnFacing: return "SetSpawnFacing";
    case CreativeMutationKind::SetCheckpointId: return "SetCheckpointId";
    case CreativeMutationKind::SetNavCost: return "SetNavCost";
    case CreativeMutationKind::SetPatrolRoute: return "SetPatrolRoute";
    case CreativeMutationKind::SetJumpArc: return "SetJumpArc";
    case CreativeMutationKind::SetClimbRule: return "SetClimbRule";
    case CreativeMutationKind::SetWallRunRule: return "SetWallRunRule";
    case CreativeMutationKind::SetSlideRule: return "SetSlideRule";

    case CreativeMutationKind::SetTestLaneKind: return "SetTestLaneKind";
    case CreativeMutationKind::SetDistanceValue: return "SetDistanceValue";
    case CreativeMutationKind::SetSpeedValue: return "SetSpeedValue";
    case CreativeMutationKind::SetTimingWindow: return "SetTimingWindow";
    case CreativeMutationKind::SetProbeKind: return "SetProbeKind";
    case CreativeMutationKind::SetExpectedResult: return "SetExpectedResult";

    case CreativeMutationKind::SetLightColor: return "SetLightColor";
    case CreativeMutationKind::SetLightIntensity: return "SetLightIntensity";
    case CreativeMutationKind::SetLightRadius: return "SetLightRadius";
    case CreativeMutationKind::SetLightConeAngle: return "SetLightConeAngle";
    case CreativeMutationKind::SetAudioRadius: return "SetAudioRadius";
    case CreativeMutationKind::SetAudioSource: return "SetAudioSource";
    case CreativeMutationKind::SetMusicCue: return "SetMusicCue";
    case CreativeMutationKind::SetCameraTarget: return "SetCameraTarget";
    case CreativeMutationKind::SetCameraRail: return "SetCameraRail";

    case CreativeMutationKind::SetEnemyProfile: return "SetEnemyProfile";
    case CreativeMutationKind::SetNpcProfile: return "SetNpcProfile";
    case CreativeMutationKind::SetResourceKind: return "SetResourceKind";
    case CreativeMutationKind::SetLootTable: return "SetLootTable";
    case CreativeMutationKind::SetQuestId: return "SetQuestId";
    case CreativeMutationKind::SetDialogueId: return "SetDialogueId";
    case CreativeMutationKind::SetDangerLevel: return "SetDangerLevel";
    case CreativeMutationKind::SetSafeZoneRule: return "SetSafeZoneRule";
    }

    return "Unknown";
}

CreativeMutationCategory categoryOf(CreativeMutationKind kind) noexcept {
    if (isIdentityMutation(kind)) {
        return CreativeMutationCategory::Identity;
    }
    if (isTransformMutation(kind)) {
        return CreativeMutationCategory::Transform;
    }
    if (isShapeMutation(kind)) {
        return CreativeMutationCategory::Shape;
    }
    if (isRelationshipMutation(kind)) {
        return CreativeMutationCategory::Relationship;
    }
    if (isOrganizationMutation(kind)) {
        return CreativeMutationCategory::Organization;
    }
    if (isContentMutation(kind)) {
        return CreativeMutationCategory::Content;
    }
    if (isLogicMutation(kind)) {
        return CreativeMutationCategory::Logic;
    }
    if (isNavigationMutation(kind)) {
        return CreativeMutationCategory::Navigation;
    }
    if (isTestingMutation(kind)) {
        return CreativeMutationCategory::Testing;
    }
    if (isSensoryMutation(kind)) {
        return CreativeMutationCategory::Sensory;
    }
    if (isGameplayMutation(kind)) {
        return CreativeMutationCategory::Gameplay;
    }

    return CreativeMutationCategory::Unknown;
}

CreativeMutationDescriptor describeMutation(CreativeMutationKind kind) noexcept {
    return CreativeMutationDescriptor{
        kind,
        categoryOf(kind),
        toString(kind),
        "authored creative object mutation",
        mutationChangesGeometry(kind),
        mutationChangesRelationships(kind),
        mutationChangesRuntimeMeaning(kind),
    };
}

bool isIdentityMutation(CreativeMutationKind kind) noexcept {
    switch (kind) {
    case CreativeMutationKind::Rename:
    case CreativeMutationKind::SetVisible:
    case CreativeMutationKind::SetLocked:
        return true;
    default:
        return false;
    }
}

bool isTransformMutation(CreativeMutationKind kind) noexcept {
    switch (kind) {
    case CreativeMutationKind::Move:
    case CreativeMutationKind::Rotate:
    case CreativeMutationKind::Scale:
    case CreativeMutationKind::SetTransform:
        return true;
    default:
        return false;
    }
}

bool isShapeMutation(CreativeMutationKind kind) noexcept {
    switch (kind) {
    case CreativeMutationKind::Resize:
    case CreativeMutationKind::Stretch:
    case CreativeMutationKind::SetBounds:
    case CreativeMutationKind::SetHeight:
    case CreativeMutationKind::SetRadius:
    case CreativeMutationKind::SetThickness:
    case CreativeMutationKind::SetLength:
    case CreativeMutationKind::SetWidth:
    case CreativeMutationKind::SetDepth:
        return true;
    default:
        return false;
    }
}

bool isRelationshipMutation(CreativeMutationKind kind) noexcept {
    switch (kind) {
    case CreativeMutationKind::SetParent:
    case CreativeMutationKind::ClearParent:
    case CreativeMutationKind::AttachTo:
    case CreativeMutationKind::DetachFrom:
    case CreativeMutationKind::LinkTarget:
    case CreativeMutationKind::UnlinkTarget:
    case CreativeMutationKind::SetSocket:
    case CreativeMutationKind::ClearSocket:
        return true;
    default:
        return false;
    }
}

bool isOrganizationMutation(CreativeMutationKind kind) noexcept {
    switch (kind) {
    case CreativeMutationKind::AssignLayer:
    case CreativeMutationKind::AddTag:
    case CreativeMutationKind::RemoveTag:
    case CreativeMutationKind::ClearTags:
        return true;
    default:
        return false;
    }
}

bool isContentMutation(CreativeMutationKind kind) noexcept {
    switch (kind) {
    case CreativeMutationKind::EditText:
    case CreativeMutationKind::SetLabel:
    case CreativeMutationKind::SetNotes:
    case CreativeMutationKind::SetReferenceSource:
    case CreativeMutationKind::SetBlueprintOpacity:
        return true;
    default:
        return false;
    }
}

bool isLogicMutation(CreativeMutationKind kind) noexcept {
    switch (kind) {
    case CreativeMutationKind::SetTriggerShape:
    case CreativeMutationKind::SetTriggerEvent:
    case CreativeMutationKind::SetCondition:
    case CreativeMutationKind::SetEventRelayTarget:
    case CreativeMutationKind::SetSpawnerProfile:
    case CreativeMutationKind::SetDespawnRule:
        return true;
    default:
        return false;
    }
}

bool isNavigationMutation(CreativeMutationKind kind) noexcept {
    switch (kind) {
    case CreativeMutationKind::SetSpawnFacing:
    case CreativeMutationKind::SetCheckpointId:
    case CreativeMutationKind::SetNavCost:
    case CreativeMutationKind::SetPatrolRoute:
    case CreativeMutationKind::SetJumpArc:
    case CreativeMutationKind::SetClimbRule:
    case CreativeMutationKind::SetWallRunRule:
    case CreativeMutationKind::SetSlideRule:
        return true;
    default:
        return false;
    }
}

bool isTestingMutation(CreativeMutationKind kind) noexcept {
    switch (kind) {
    case CreativeMutationKind::SetTestLaneKind:
    case CreativeMutationKind::SetDistanceValue:
    case CreativeMutationKind::SetSpeedValue:
    case CreativeMutationKind::SetTimingWindow:
    case CreativeMutationKind::SetProbeKind:
    case CreativeMutationKind::SetExpectedResult:
        return true;
    default:
        return false;
    }
}

bool isSensoryMutation(CreativeMutationKind kind) noexcept {
    switch (kind) {
    case CreativeMutationKind::SetLightColor:
    case CreativeMutationKind::SetLightIntensity:
    case CreativeMutationKind::SetLightRadius:
    case CreativeMutationKind::SetLightConeAngle:
    case CreativeMutationKind::SetAudioRadius:
    case CreativeMutationKind::SetAudioSource:
    case CreativeMutationKind::SetMusicCue:
    case CreativeMutationKind::SetCameraTarget:
    case CreativeMutationKind::SetCameraRail:
        return true;
    default:
        return false;
    }
}

bool isGameplayMutation(CreativeMutationKind kind) noexcept {
    switch (kind) {
    case CreativeMutationKind::SetEnemyProfile:
    case CreativeMutationKind::SetNpcProfile:
    case CreativeMutationKind::SetResourceKind:
    case CreativeMutationKind::SetLootTable:
    case CreativeMutationKind::SetQuestId:
    case CreativeMutationKind::SetDialogueId:
    case CreativeMutationKind::SetDangerLevel:
    case CreativeMutationKind::SetSafeZoneRule:
        return true;
    default:
        return false;
    }
}

bool mutationChangesGeometry(CreativeMutationKind kind) noexcept {
    return isTransformMutation(kind) || isShapeMutation(kind) || kind == CreativeMutationKind::SetTriggerShape;
}

bool mutationChangesRelationships(CreativeMutationKind kind) noexcept {
    return isRelationshipMutation(kind) || kind == CreativeMutationKind::SetEventRelayTarget ||
           kind == CreativeMutationKind::SetCameraTarget || kind == CreativeMutationKind::SetCameraRail;
}

bool mutationChangesRuntimeMeaning(CreativeMutationKind kind) noexcept {
    return isLogicMutation(kind) || isNavigationMutation(kind) || isTestingMutation(kind) ||
           isSensoryMutation(kind) || isGameplayMutation(kind);
}

std::vector<CreativeMutationKind> allowedMutations(CreativeObjectKind objectKind) {
    auto mutations = commonIdentityMutations();

    switch (objectKind) {
    case CreativeObjectKind::Unknown:
        return {};

    case CreativeObjectKind::Room:
        // TD-2: no-transform kinds with bounds move by corner anchor, so Move
        // is a real verb for Room even though it stores no transform.
        mutations.push_back(CreativeMutationKind::Move);
        appendBoxShapeMutations(mutations);
        return mutations;

    case CreativeObjectKind::Wall:
    case CreativeObjectKind::Floor:
    case CreativeObjectKind::Ceiling:
    case CreativeObjectKind::Roof:
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
        appendTransformMutations(mutations);
        appendBoxShapeMutations(mutations);
        appendRelationshipMutations(mutations);
        return mutations;

    case CreativeObjectKind::Door:
    case CreativeObjectKind::Window:
    case CreativeObjectKind::Socket:
    case CreativeObjectKind::AttachmentPoint:
        appendTransformMutations(mutations);
        appendBoxShapeMutations(mutations);
        appendRelationshipMutations(mutations);
        return mutations;

    case CreativeObjectKind::TerrainPatch:
    case CreativeObjectKind::WaterVolume:
    case CreativeObjectKind::LavaVolume:
    case CreativeObjectKind::Pit:
    case CreativeObjectKind::Slope:
    case CreativeObjectKind::Cliff:
    case CreativeObjectKind::CaveOpening:
    case CreativeObjectKind::BoundaryVolume:
    case CreativeObjectKind::KillPlane:
        appendTransformMutations(mutations);
        appendBoxShapeMutations(mutations);
        mutations.push_back(CreativeMutationKind::SetRadius);
        mutations.push_back(CreativeMutationKind::SetThickness);
        return mutations;

    case CreativeObjectKind::SpawnPoint:
    case CreativeObjectKind::ExitPoint:
    case CreativeObjectKind::EntrancePoint:
    case CreativeObjectKind::Checkpoint:
    case CreativeObjectKind::CoverPoint:
    case CreativeObjectKind::PatrolNode:
    case CreativeObjectKind::EnemySpawn:
    case CreativeObjectKind::NpcSpawn:
    case CreativeObjectKind::InterestPoint:
    case CreativeObjectKind::ResourceNode:
    case CreativeObjectKind::LootPoint:
    case CreativeObjectKind::QuestMarker:
    case CreativeObjectKind::DialogueMarker:
        mutations.push_back(CreativeMutationKind::Move);
        mutations.push_back(CreativeMutationKind::Rotate);
        mutations.push_back(CreativeMutationKind::SetSpawnFacing);
        mutations.push_back(CreativeMutationKind::SetCheckpointId);
        return mutations;

    case CreativeObjectKind::NavRegion:
    case CreativeObjectKind::AlertZone:
    case CreativeObjectKind::SafeZone:
    case CreativeObjectKind::DangerZone:
        appendTransformMutations(mutations);
        appendBoxShapeMutations(mutations);
        mutations.push_back(CreativeMutationKind::SetNavCost);
        mutations.push_back(CreativeMutationKind::SetDangerLevel);
        mutations.push_back(CreativeMutationKind::SetSafeZoneRule);
        return mutations;

    case CreativeObjectKind::NavLink:
    case CreativeObjectKind::JumpLink:
    case CreativeObjectKind::ClimbLink:
    case CreativeObjectKind::PatrolRoute:
        appendRelationshipMutations(mutations);
        mutations.push_back(CreativeMutationKind::SetNavCost);
        mutations.push_back(CreativeMutationKind::SetJumpArc);
        mutations.push_back(CreativeMutationKind::SetClimbRule);
        mutations.push_back(CreativeMutationKind::SetPatrolRoute);
        return mutations;

    case CreativeObjectKind::WallRunSurface:
    case CreativeObjectKind::SlideSurface:
        appendTransformMutations(mutations);
        appendBoxShapeMutations(mutations);
        mutations.push_back(CreativeMutationKind::SetWallRunRule);
        mutations.push_back(CreativeMutationKind::SetSlideRule);
        return mutations;

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
        appendTransformMutations(mutations);
        appendBoxShapeMutations(mutations);
        appendRelationshipMutations(mutations);
        mutations.push_back(CreativeMutationKind::SetTriggerShape);
        mutations.push_back(CreativeMutationKind::SetTriggerEvent);
        mutations.push_back(CreativeMutationKind::SetCondition);
        mutations.push_back(CreativeMutationKind::SetEventRelayTarget);
        mutations.push_back(CreativeMutationKind::SetSpawnerProfile);
        mutations.push_back(CreativeMutationKind::SetDespawnRule);
        return mutations;

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
        appendTransformMutations(mutations);
        appendBoxShapeMutations(mutations);
        mutations.push_back(CreativeMutationKind::SetTestLaneKind);
        mutations.push_back(CreativeMutationKind::SetDistanceValue);
        mutations.push_back(CreativeMutationKind::SetSpeedValue);
        mutations.push_back(CreativeMutationKind::SetTimingWindow);
        mutations.push_back(CreativeMutationKind::SetProbeKind);
        mutations.push_back(CreativeMutationKind::SetExpectedResult);
        return mutations;

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
    case CreativeObjectKind::Group:
    case CreativeObjectKind::PrefabInstance:
        appendTransformMutations(mutations);
        appendBoxShapeMutations(mutations);
        appendRelationshipMutations(mutations);
        return mutations;

    case CreativeObjectKind::PointLight:
    case CreativeObjectKind::SpotLight:
    case CreativeObjectKind::AreaLight:
    case CreativeObjectKind::AmbientZone:
        mutations.push_back(CreativeMutationKind::Move);
        mutations.push_back(CreativeMutationKind::Rotate);
        mutations.push_back(CreativeMutationKind::SetLightColor);
        mutations.push_back(CreativeMutationKind::SetLightIntensity);
        mutations.push_back(CreativeMutationKind::SetLightRadius);
        mutations.push_back(CreativeMutationKind::SetLightConeAngle);
        return mutations;

    case CreativeObjectKind::ReverbZone:
    case CreativeObjectKind::SoundEmitter:
    case CreativeObjectKind::MusicZone:
        mutations.push_back(CreativeMutationKind::Move);
        mutations.push_back(CreativeMutationKind::SetAudioRadius);
        mutations.push_back(CreativeMutationKind::SetAudioSource);
        mutations.push_back(CreativeMutationKind::SetMusicCue);
        return mutations;

    case CreativeObjectKind::CameraMarker:
    case CreativeObjectKind::CameraTarget:
        mutations.push_back(CreativeMutationKind::Move);
        mutations.push_back(CreativeMutationKind::Rotate);
        mutations.push_back(CreativeMutationKind::SetCameraTarget);
        return mutations;

    case CreativeObjectKind::CameraRail:
    case CreativeObjectKind::CutsceneMarker:
        appendTransformMutations(mutations);
        appendRelationshipMutations(mutations);
        mutations.push_back(CreativeMutationKind::SetCameraTarget);
        mutations.push_back(CreativeMutationKind::SetCameraRail);
        return mutations;

    case CreativeObjectKind::Note:
    case CreativeObjectKind::Label:
    case CreativeObjectKind::Comment:
        mutations.push_back(CreativeMutationKind::Move);
        mutations.push_back(CreativeMutationKind::EditText);
        mutations.push_back(CreativeMutationKind::SetLabel);
        mutations.push_back(CreativeMutationKind::SetNotes);
        return mutations;

    case CreativeObjectKind::MeasurementMarker:
    case CreativeObjectKind::MeasurementLine:
    case CreativeObjectKind::MeasurementBox:
    case CreativeObjectKind::GridAnchor:
    case CreativeObjectKind::SnapAnchor:
        appendTransformMutations(mutations);
        appendBoxShapeMutations(mutations);
        mutations.push_back(CreativeMutationKind::SetDistanceValue);
        return mutations;

    case CreativeObjectKind::ReferenceImage:
    case CreativeObjectKind::BlueprintOverlay:
        appendTransformMutations(mutations);
        appendBoxShapeMutations(mutations);
        mutations.push_back(CreativeMutationKind::SetReferenceSource);
        mutations.push_back(CreativeMutationKind::SetBlueprintOpacity);
        return mutations;
    }

    return mutations;
}

bool canMutate(CreativeObjectKind objectKind, CreativeMutationKind mutationKind) noexcept {
    const auto mutations = allowedMutations(objectKind);
    return containsMutation(mutations, mutationKind);
}

bool requiresPayload(CreativeMutationKind kind) noexcept {
    switch (kind) {
    case CreativeMutationKind::Unknown:
    case CreativeMutationKind::ClearParent:
    case CreativeMutationKind::DetachFrom:
    case CreativeMutationKind::UnlinkTarget:
    case CreativeMutationKind::ClearSocket:
    case CreativeMutationKind::ClearTags:
        return false;
    default:
        return true;
    }
}

bool payloadMatchesMutation(CreativeMutationKind kind, const CreativeMutationPayload& payload) noexcept {
    const auto& value = payload.value;

    switch (kind) {
    case CreativeMutationKind::Unknown:
        return std::holds_alternative<std::monostate>(value);

    case CreativeMutationKind::Rename:
        return std::holds_alternative<RenameMutation>(value);
    case CreativeMutationKind::SetVisible:
        return std::holds_alternative<VisibilityMutation>(value);
    case CreativeMutationKind::SetLocked:
        return std::holds_alternative<LockMutation>(value);

    case CreativeMutationKind::Move:
        return std::holds_alternative<MoveMutation>(value);
    case CreativeMutationKind::Rotate:
    case CreativeMutationKind::SetSpawnFacing:
        return std::holds_alternative<RotateMutation>(value);
    case CreativeMutationKind::Scale:
        return std::holds_alternative<ScaleMutation>(value);
    case CreativeMutationKind::SetTransform:
        return std::holds_alternative<SetTransformMutation>(value);

    case CreativeMutationKind::Resize:
        return std::holds_alternative<ResizeMutation>(value);
    case CreativeMutationKind::Stretch:
        return std::holds_alternative<StretchMutation>(value);
    case CreativeMutationKind::SetBounds:
    case CreativeMutationKind::SetTriggerShape:
        return std::holds_alternative<SetBoundsMutation>(value);
    case CreativeMutationKind::SetHeight:
    case CreativeMutationKind::SetRadius:
    case CreativeMutationKind::SetThickness:
    case CreativeMutationKind::SetLength:
    case CreativeMutationKind::SetWidth:
    case CreativeMutationKind::SetDepth:
    case CreativeMutationKind::SetBlueprintOpacity:
    case CreativeMutationKind::SetLightIntensity:
    case CreativeMutationKind::SetLightRadius:
    case CreativeMutationKind::SetLightConeAngle:
    case CreativeMutationKind::SetAudioRadius:
    case CreativeMutationKind::SetNavCost:
    case CreativeMutationKind::SetDistanceValue:
    case CreativeMutationKind::SetSpeedValue:
    case CreativeMutationKind::SetTimingWindow:
    case CreativeMutationKind::SetDangerLevel:
        return std::holds_alternative<ScalarMutation>(value);

    case CreativeMutationKind::SetParent:
        return std::holds_alternative<SetParentMutation>(value);
    case CreativeMutationKind::AttachTo:
        return std::holds_alternative<AttachToMutation>(value);
    case CreativeMutationKind::LinkTarget:
    case CreativeMutationKind::SetEventRelayTarget:
    case CreativeMutationKind::SetCameraTarget:
    case CreativeMutationKind::SetCameraRail:
        return std::holds_alternative<LinkTargetMutation>(value);
    case CreativeMutationKind::SetSocket:
        return std::holds_alternative<SetSocketMutation>(value);

    case CreativeMutationKind::AssignLayer:
        return std::holds_alternative<AssignLayerMutation>(value);
    case CreativeMutationKind::AddTag:
    case CreativeMutationKind::RemoveTag:
        return std::holds_alternative<TagMutation>(value);

    case CreativeMutationKind::EditText:
    case CreativeMutationKind::SetLabel:
    case CreativeMutationKind::SetNotes:
    case CreativeMutationKind::SetTriggerEvent:
    case CreativeMutationKind::SetCondition:
    case CreativeMutationKind::SetSpawnerProfile:
    case CreativeMutationKind::SetDespawnRule:
    case CreativeMutationKind::SetCheckpointId:
    case CreativeMutationKind::SetTestLaneKind:
    case CreativeMutationKind::SetProbeKind:
    case CreativeMutationKind::SetExpectedResult:
    case CreativeMutationKind::SetEnemyProfile:
    case CreativeMutationKind::SetNpcProfile:
    case CreativeMutationKind::SetResourceKind:
    case CreativeMutationKind::SetLootTable:
    case CreativeMutationKind::SetQuestId:
    case CreativeMutationKind::SetDialogueId:
    case CreativeMutationKind::SetSafeZoneRule:
    case CreativeMutationKind::SetPatrolRoute:
    case CreativeMutationKind::SetJumpArc:
    case CreativeMutationKind::SetClimbRule:
    case CreativeMutationKind::SetWallRunRule:
    case CreativeMutationKind::SetSlideRule:
    case CreativeMutationKind::SetMusicCue:
        return std::holds_alternative<TextMutation>(value) || std::holds_alternative<StringIdMutation>(value);

    case CreativeMutationKind::SetReferenceSource:
        return std::holds_alternative<ReferenceSourceMutation>(value);
    case CreativeMutationKind::SetLightColor:
        return std::holds_alternative<ColorMutation>(value);
    case CreativeMutationKind::SetAudioSource:
        return std::holds_alternative<AudioSourceMutation>(value);

    case CreativeMutationKind::ClearParent:
    case CreativeMutationKind::DetachFrom:
    case CreativeMutationKind::UnlinkTarget:
    case CreativeMutationKind::ClearSocket:
    case CreativeMutationKind::ClearTags:
        return std::holds_alternative<std::monostate>(value);
    }

    return false;
}

CreativeMutationPayload makeRenamePayload(std::string name) {
    return CreativeMutationPayload{RenameMutation{std::move(name)}};
}

CreativeMutationPayload makeVisibilityPayload(bool visible) {
    return CreativeMutationPayload{VisibilityMutation{visible}};
}

CreativeMutationPayload makeLockPayload(bool locked) {
    return CreativeMutationPayload{LockMutation{locked}};
}

CreativeMutationPayload makeMovePayload(CreativeVec3 position) {
    return CreativeMutationPayload{MoveMutation{position}};
}

CreativeMutationPayload makeRotatePayload(CreativeVec3 rotation) {
    return CreativeMutationPayload{RotateMutation{rotation}};
}

CreativeMutationPayload makeScalePayload(CreativeVec3 scale) {
    return CreativeMutationPayload{ScaleMutation{scale}};
}

CreativeMutationPayload makeTransformPayload(CreativeTransform transform) {
    return CreativeMutationPayload{SetTransformMutation{transform}};
}

CreativeMutationPayload makeResizePayload(CreativeVec3 size) {
    return CreativeMutationPayload{ResizeMutation{size}};
}

CreativeMutationPayload makeStretchPayload(CreativeVec3 delta) {
    return CreativeMutationPayload{StretchMutation{delta}};
}

CreativeMutationPayload makeBoundsPayload(CreativeBounds bounds) {
    return CreativeMutationPayload{SetBoundsMutation{bounds}};
}

CreativeMutationPayload makeScalarPayload(double value) {
    return CreativeMutationPayload{ScalarMutation{value}};
}

CreativeMutationPayload makeParentPayload(CreativeObjectId parentId) {
    return CreativeMutationPayload{SetParentMutation{parentId}};
}

CreativeMutationPayload makeAttachPayload(CreativeObjectId targetId, std::string socket) {
    return CreativeMutationPayload{AttachToMutation{targetId, std::move(socket)}};
}

CreativeMutationPayload makeLinkPayload(CreativeObjectId targetId) {
    return CreativeMutationPayload{LinkTargetMutation{targetId}};
}

CreativeMutationPayload makeSocketPayload(std::string socket) {
    return CreativeMutationPayload{SetSocketMutation{std::move(socket)}};
}

CreativeMutationPayload makeLayerPayload(CreativeLayerId layerId) {
    return CreativeMutationPayload{AssignLayerMutation{layerId}};
}

CreativeMutationPayload makeTagPayload(std::string tag) {
    return CreativeMutationPayload{TagMutation{std::move(tag)}};
}

CreativeMutationPayload makeTextPayload(std::string text) {
    return CreativeMutationPayload{TextMutation{std::move(text)}};
}

CreativeMutationPayload makeReferenceSourcePayload(std::string source) {
    return CreativeMutationPayload{ReferenceSourceMutation{std::move(source)}};
}

CreativeMutationPayload makeColorPayload(CreativeRgba color) {
    return CreativeMutationPayload{ColorMutation{color}};
}

CreativeMutationPayload makeAudioSourcePayload(std::string source) {
    return CreativeMutationPayload{AudioSourceMutation{std::move(source)}};
}

CreativeMutationPayload makeStringIdPayload(std::string id) {
    return CreativeMutationPayload{StringIdMutation{std::move(id)}};
}

CreativeMutationPayload makeObjectKindPayload(CreativeObjectKind kind) {
    return CreativeMutationPayload{ObjectKindMutation{kind}};
}

} // namespace iggy3d::creative
