#include "app/iggy3d/creative/mutation/Mutation.hpp"

#include <algorithm>

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

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

void appendTerrainVolumeMutations(std::vector<CreativeMutationKind>& mutations) {
    appendTransformMutations(mutations);
    appendBoxShapeMutations(mutations);
    mutations.push_back(CreativeMutationKind::SetRadius);
    mutations.push_back(CreativeMutationKind::SetThickness);
}

void appendNavigationMarkerMutations(std::vector<CreativeMutationKind>& mutations) {
    mutations.push_back(CreativeMutationKind::Move);
    mutations.push_back(CreativeMutationKind::Rotate);
    mutations.push_back(CreativeMutationKind::SetSpawnFacing);
    mutations.push_back(CreativeMutationKind::SetCheckpointId);
}

void appendNavigationVolumeMutations(std::vector<CreativeMutationKind>& mutations) {
    appendTransformMutations(mutations);
    appendBoxShapeMutations(mutations);
    mutations.push_back(CreativeMutationKind::SetNavCost);
    mutations.push_back(CreativeMutationKind::SetDangerLevel);
    mutations.push_back(CreativeMutationKind::SetSafeZoneRule);
}

void appendLinkOrRouteMutations(std::vector<CreativeMutationKind>& mutations) {
    appendRelationshipMutations(mutations);
    mutations.push_back(CreativeMutationKind::SetNavCost);
    mutations.push_back(CreativeMutationKind::SetJumpArc);
    mutations.push_back(CreativeMutationKind::SetClimbRule);
    mutations.push_back(CreativeMutationKind::SetPatrolRoute);
}

void appendMovementSurfaceMutations(std::vector<CreativeMutationKind>& mutations) {
    appendTransformMutations(mutations);
    appendBoxShapeMutations(mutations);
    mutations.push_back(CreativeMutationKind::SetWallRunRule);
    mutations.push_back(CreativeMutationKind::SetSlideRule);
}

void appendLogicNodeMutations(std::vector<CreativeMutationKind>& mutations) {
    appendTransformMutations(mutations);
    appendBoxShapeMutations(mutations);
    appendRelationshipMutations(mutations);
    mutations.push_back(CreativeMutationKind::SetTriggerShape);
    mutations.push_back(CreativeMutationKind::SetTriggerEvent);
    mutations.push_back(CreativeMutationKind::SetCondition);
    mutations.push_back(CreativeMutationKind::SetEventRelayTarget);
    mutations.push_back(CreativeMutationKind::SetSpawnerProfile);
    mutations.push_back(CreativeMutationKind::SetDespawnRule);
}

void appendTestingMutations(std::vector<CreativeMutationKind>& mutations) {
    appendTransformMutations(mutations);
    appendBoxShapeMutations(mutations);
    mutations.push_back(CreativeMutationKind::SetTestLaneKind);
    mutations.push_back(CreativeMutationKind::SetDistanceValue);
    mutations.push_back(CreativeMutationKind::SetSpeedValue);
    mutations.push_back(CreativeMutationKind::SetTimingWindow);
    mutations.push_back(CreativeMutationKind::SetProbeKind);
    mutations.push_back(CreativeMutationKind::SetExpectedResult);
}

void appendDressingMutations(std::vector<CreativeMutationKind>& mutations) {
    appendTransformMutations(mutations);
    appendBoxShapeMutations(mutations);
    appendRelationshipMutations(mutations);
}

void appendLightMutations(std::vector<CreativeMutationKind>& mutations) {
    mutations.push_back(CreativeMutationKind::Move);
    mutations.push_back(CreativeMutationKind::Rotate);
    mutations.push_back(CreativeMutationKind::SetLightColor);
    mutations.push_back(CreativeMutationKind::SetLightIntensity);
    mutations.push_back(CreativeMutationKind::SetLightRadius);
    mutations.push_back(CreativeMutationKind::SetLightConeAngle);
}

void appendAudioMutations(std::vector<CreativeMutationKind>& mutations) {
    mutations.push_back(CreativeMutationKind::Move);
    mutations.push_back(CreativeMutationKind::SetAudioRadius);
    mutations.push_back(CreativeMutationKind::SetAudioSource);
    mutations.push_back(CreativeMutationKind::SetMusicCue);
}

void appendCameraMarkerMutations(std::vector<CreativeMutationKind>& mutations) {
    mutations.push_back(CreativeMutationKind::Move);
    mutations.push_back(CreativeMutationKind::Rotate);
    mutations.push_back(CreativeMutationKind::SetCameraTarget);
}

void appendCameraPathMutations(std::vector<CreativeMutationKind>& mutations) {
    appendTransformMutations(mutations);
    appendRelationshipMutations(mutations);
    mutations.push_back(CreativeMutationKind::SetCameraTarget);
    mutations.push_back(CreativeMutationKind::SetCameraRail);
}

void appendTextAnnotationMutations(std::vector<CreativeMutationKind>& mutations) {
    mutations.push_back(CreativeMutationKind::Move);
    mutations.push_back(CreativeMutationKind::EditText);
    mutations.push_back(CreativeMutationKind::SetLabel);
    mutations.push_back(CreativeMutationKind::SetNotes);
}

void appendMeasurementMutations(std::vector<CreativeMutationKind>& mutations) {
    appendTransformMutations(mutations);
    appendBoxShapeMutations(mutations);
    mutations.push_back(CreativeMutationKind::SetDistanceValue);
}

void appendReferencePlaneMutations(std::vector<CreativeMutationKind>& mutations) {
    appendTransformMutations(mutations);
    appendBoxShapeMutations(mutations);
    mutations.push_back(CreativeMutationKind::SetReferenceSource);
    mutations.push_back(CreativeMutationKind::SetBlueprintOpacity);
}

[[nodiscard]] bool isKnownObjectDescriptor(
    CreativeObjectKind requestedKind,
    const CreativeObjectDescriptor& descriptor) noexcept {
    return requestedKind != CreativeObjectKind::Unknown &&
           requestedKind != CreativeObjectKind::Count &&
           descriptor.kind == requestedKind &&
           descriptor.kind != CreativeObjectKind::Unknown;
}

[[nodiscard]] bool isCameraPathDescriptor(
    const CreativeObjectDescriptor& descriptor) noexcept {
    return descriptor.shapeKind == CreativeObjectShapeKind::Line ||
           descriptor.projectionProfile == CreativeSpatialProjectionProfile::NoProjection;
}

[[nodiscard]] bool supportsImportedAssetReplacement(
    CreativeObjectKind kind) noexcept {
    return kind == CreativeObjectKind::Prop || kind == CreativeObjectKind::Rock ||
           kind == CreativeObjectKind::Bridge;
}

void appendDescriptorProfileMutations(
    const CreativeObjectDescriptor& descriptor,
    std::vector<CreativeMutationKind>& mutations) {
    switch (descriptor.profile) {
    case CreativeObjectProfile::Unknown:
        return;
    case CreativeObjectProfile::RoomContainer:
        mutations.push_back(CreativeMutationKind::Move);
        appendBoxShapeMutations(mutations);
        return;
    case CreativeObjectProfile::BoxStructural:
    case CreativeObjectProfile::Attachment:
        appendDressingMutations(mutations);
        return;
    case CreativeObjectProfile::TerrainVolume:
        appendTerrainVolumeMutations(mutations);
        return;
    case CreativeObjectProfile::Marker:
    case CreativeObjectProfile::GameplayMarker:
        appendNavigationMarkerMutations(mutations);
        return;
    case CreativeObjectProfile::Volume:
        if (descriptor.category == CreativeObjectCategory::TerrainOrVolume) {
            appendTerrainVolumeMutations(mutations);
            return;
        }
        appendNavigationVolumeMutations(mutations);
        return;
    case CreativeObjectProfile::LinkOrRoute:
        appendLinkOrRouteMutations(mutations);
        return;
    case CreativeObjectProfile::LogicNode:
        appendLogicNodeMutations(mutations);
        return;
    case CreativeObjectProfile::MovementTest:
        if (descriptor.category == CreativeObjectCategory::NavigationOrMovement) {
            appendMovementSurfaceMutations(mutations);
            return;
        }
        appendTestingMutations(mutations);
        return;
    case CreativeObjectProfile::Dressing:
        appendDressingMutations(mutations);
        return;
    case CreativeObjectProfile::Light:
        appendLightMutations(mutations);
        return;
    case CreativeObjectProfile::Audio:
        appendAudioMutations(mutations);
        return;
    case CreativeObjectProfile::Camera:
        if (isCameraPathDescriptor(descriptor)) {
            appendCameraPathMutations(mutations);
            return;
        }
        appendCameraMarkerMutations(mutations);
        return;
    case CreativeObjectProfile::TextAnnotation:
        appendTextAnnotationMutations(mutations);
        return;
    case CreativeObjectProfile::AuthoringHelper:
        if (descriptor.canOwnChildren) {
            appendDressingMutations(mutations);
            return;
        }
        if (descriptor.shapeKind == CreativeObjectShapeKind::Surface) {
            appendReferencePlaneMutations(mutations);
            return;
        }
        appendMeasurementMutations(mutations);
        return;
    }
}

} // namespace

std::vector<CreativeMutationKind> allowedMutations(CreativeObjectKind objectKind) {
    const CreativeObjectDescriptor& descriptor = describeObject(objectKind);
    if (!isKnownObjectDescriptor(objectKind, descriptor)) {
        return {};
    }

    auto mutations = commonIdentityMutations();
    appendDescriptorProfileMutations(descriptor, mutations);
    if (objectKind == CreativeObjectKind::MovingPlatform) {
        mutations.push_back(CreativeMutationKind::SetPatrolRoute);
        mutations.push_back(CreativeMutationKind::SetMovingPlatformSettings);
    }
    if (objectKind == CreativeObjectKind::SpawnPoint) {
        mutations.push_back(CreativeMutationKind::SetPlayerSpawnSettings);
    }
    if (objectKind == CreativeObjectKind::NpcSpawn ||
        objectKind == CreativeObjectKind::EnemySpawn) {
        mutations.push_back(CreativeMutationKind::SetNpcSpawnSettings);
    }
    if (objectKind == CreativeObjectKind::LootPoint) {
        mutations.push_back(CreativeMutationKind::SetLootPointSettings);
    }
    if (objectKind == CreativeObjectKind::ExitPoint) {
        mutations.push_back(CreativeMutationKind::SetExitPointSettings);
    }
    if (supportsImportedAssetReplacement(objectKind)) {
        mutations.push_back(CreativeMutationKind::SetAsset);
    }
    return mutations;
}

bool canMutate(CreativeObjectKind objectKind, CreativeMutationKind mutationKind) noexcept {
    const auto mutations = allowedMutations(objectKind);
    return containsMutation(mutations, mutationKind);
}

} // namespace iggy3d::creative
