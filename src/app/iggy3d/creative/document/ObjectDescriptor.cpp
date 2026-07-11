

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

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

using DescriptorCapabilityFlags = std::uint32_t;

constexpr DescriptorCapabilityFlags kNoCapabilities = 0;
constexpr DescriptorCapabilityFlags kHasTransform = 1u << 0;
constexpr DescriptorCapabilityFlags kHasBounds = 1u << 1;
constexpr DescriptorCapabilityFlags kCanHaveParent = 1u << 2;
constexpr DescriptorCapabilityFlags kCanOwnChildren = 1u << 3;
constexpr DescriptorCapabilityFlags kRuntimeMeaningful = 1u << 4;
constexpr DescriptorCapabilityFlags kEditorOnly = 1u << 5;
constexpr DescriptorCapabilityFlags kAuthoringBrushPalette = 1u << 6;

constexpr bool hasCapability(
    DescriptorCapabilityFlags capabilities,
    DescriptorCapabilityFlags capability) noexcept {
    return (capabilities & capability) != 0;
}

constexpr bool descriptorSupportsPlacementGeometry(
    CreativeObjectKind kind,
    CreativeObjectShapeKind shapeKind,
    CreativeSpatialProjectionProfile projectionProfile,
    DescriptorCapabilityFlags capabilities) noexcept {
    if (kind == CreativeObjectKind::Unknown) {
        return false;
    }
    const bool hasTransform = hasCapability(capabilities, kHasTransform);
    const bool hasBounds = hasCapability(capabilities, kHasBounds);
    switch (shapeKind) {
        case CreativeObjectShapeKind::BoxVolume:
        case CreativeObjectShapeKind::Surface:
        case CreativeObjectShapeKind::MeshProxy:
            return hasTransform && hasBounds &&
                   projectionProfile ==
                       CreativeSpatialProjectionProfile::BoxProjection;
        case CreativeObjectShapeKind::Line:
            return hasTransform && hasBounds &&
                   (projectionProfile ==
                        CreativeSpatialProjectionProfile::BoxProjection ||
                    projectionProfile ==
                        CreativeSpatialProjectionProfile::LineProjection);
        case CreativeObjectShapeKind::Point:
            return hasTransform && !hasBounds;
        case CreativeObjectShapeKind::Path:
            return projectionProfile ==
                   CreativeSpatialProjectionProfile::PathProjection;
        case CreativeObjectShapeKind::Unknown:
            return false;
    }
    return false;
}

constexpr CreativeObjectPlacementPolicy placementPolicyForDescriptor(
    CreativeObjectKind kind,
    CreativeObjectShapeKind shapeKind,
    CreativeSpatialProjectionProfile projectionProfile,
    DescriptorCapabilityFlags capabilities,
    CreativePlacementOrientationPolicy orientationPolicy,
    CreativePlacementFace localForwardFace,
    CreativePlacementStoragePolicy storagePolicy) noexcept {
    if (!descriptorSupportsPlacementGeometry(kind, shapeKind,
                                             projectionProfile,
                                             capabilities)) {
        return {};
    }
    return CreativeObjectPlacementPolicy{
        kCreativePlacementAllFaces,
        CreativePlacementTargetPolicy::AdjacentCell,
        orientationPolicy,
        localForwardFace,
        storagePolicy == CreativePlacementStoragePolicy::VoxelCell
            ? CreativePlacementOccupancyPolicy::RejectOccupied
            : CreativePlacementOccupancyPolicy::AllowOverlap,
        storagePolicy,
        true,
    };
}

constexpr CreativeObjectDescriptor descriptor(
    CreativeObjectKind kind,
    CreativeObjectCategory category,
    CreativeObjectProfile profile,
    CreativeObjectShapeKind shapeKind,
    CreativeSpatialProjectionProfile projectionProfile,
    CreativeSpatialOccupancyKind occupancyKind,
    std::string_view name,
    std::string_view displayName,
    std::string_view purpose,
    CreativeObjectDirtyFlags creationDirtyFlags,
    CreativeObjectDefaults defaults,
    DescriptorCapabilityFlags capabilities,
    CreativeRuntimeAnchorSemantic runtimeAnchorSemantic =
        CreativeRuntimeAnchorSemantic::None,
    CreativePlacementOrientationPolicy placementOrientationPolicy =
        CreativePlacementOrientationPolicy::DescriptorDefault,
    CreativePlacementFace localForwardFace =
        CreativePlacementFace::PositiveZ,
    CreativePlacementStoragePolicy placementStoragePolicy =
        CreativePlacementStoragePolicy::AuthoredObject) noexcept {
    return CreativeObjectDescriptor{
        kind,
        category,
        profile,
        shapeKind,
        projectionProfile,
        occupancyKind,
        runtimeAnchorSemantic,
        hasCapability(capabilities, kAuthoringBrushPalette)
            ? CreativeAuthoringPaletteVisibility::Brush
            : CreativeAuthoringPaletteVisibility::Hidden,
        placementPolicyForDescriptor(kind, shapeKind, projectionProfile,
                                     capabilities,
                                     placementOrientationPolicy,
                                     localForwardFace,
                                     placementStoragePolicy),
        name,
        displayName,
        purpose,
        creationDirtyFlags,
        defaults,
        hasCapability(capabilities, kHasTransform),
        hasCapability(capabilities, kHasBounds),
        hasCapability(capabilities, kCanHaveParent),
        hasCapability(capabilities, kCanOwnChildren),
        hasCapability(capabilities, kRuntimeMeaningful),
        hasCapability(capabilities, kEditorOnly),
    };
}

template <std::size_t ResultSize, std::size_t SectionSize>
constexpr void appendDescriptorSection(
    std::array<CreativeObjectDescriptor, ResultSize>& result,
    const std::array<CreativeObjectDescriptor, SectionSize>& section,
    std::size_t& offset) noexcept {
    for (const CreativeObjectDescriptor& descriptor : section) {
        result[offset++] = descriptor;
    }
}

template <std::size_t... Sizes>
constexpr auto concatDescriptorSections(
    const std::array<CreativeObjectDescriptor, Sizes>&... sections) noexcept {
    std::array<CreativeObjectDescriptor, (Sizes + ...)> result{};
    std::size_t offset = 0;
    (appendDescriptorSection(result, sections, offset), ...);
    return result;
}

constexpr auto kUnknownDescriptors = std::to_array<CreativeObjectDescriptor>({
    descriptor(
        CreativeObjectKind::Unknown,
        CreativeObjectCategory::Unknown,
        CreativeObjectProfile::Unknown,
        CreativeObjectShapeKind::Unknown,
        CreativeSpatialProjectionProfile::NoProjection,
        CreativeSpatialOccupancyKind::Unknown,
        "Unknown",
        "Unknown",
        "invalid or unclassified creative object",
        0,
        CreativeObjectDefaults{},
        kNoCapabilities
    )
});

constexpr auto kStructuralDescriptors = std::to_array<CreativeObjectDescriptor>({
    descriptor(
        CreativeObjectKind::Room,
        CreativeObjectCategory::Structural,
        CreativeObjectProfile::RoomContainer,
        CreativeObjectShapeKind::BoxVolume,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "Room",
        "Room",
        "authored room volume or level container",
        structuralCreationDirtyFlags(),
        boxDefaults(10.0, 4.0, 10.0),
        kHasBounds | kCanOwnChildren | kRuntimeMeaningful
    ),
    descriptor(
        CreativeObjectKind::Wall,
        CreativeObjectCategory::Structural,
        CreativeObjectProfile::BoxStructural,
        CreativeObjectShapeKind::Surface,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "Wall",
        "Wall",
        "vertical structural surface",
        structuralCreationDirtyFlags(),
        boxDefaults(1.0, 3.0, 4.0),
        kHasTransform | kHasBounds | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::None,
        CreativePlacementOrientationPolicy::DescriptorDefault,
        CreativePlacementFace::PositiveZ,
        CreativePlacementStoragePolicy::VoxelCell
    ),
    descriptor(
        CreativeObjectKind::Floor,
        CreativeObjectCategory::Structural,
        CreativeObjectProfile::BoxStructural,
        CreativeObjectShapeKind::Surface,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "Floor",
        "Floor",
        "walkable horizontal structural surface",
        structuralCreationDirtyFlags(),
        boxDefaults(4.0, 0.25, 4.0),
        kHasTransform | kHasBounds | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::None,
        CreativePlacementOrientationPolicy::DescriptorDefault,
        CreativePlacementFace::PositiveZ,
        CreativePlacementStoragePolicy::VoxelCell
    ),
    descriptor(
        CreativeObjectKind::Ceiling,
        CreativeObjectCategory::Structural,
        CreativeObjectProfile::BoxStructural,
        CreativeObjectShapeKind::Surface,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "Ceiling",
        "Ceiling",
        "upper room boundary surface",
        structuralCreationDirtyFlags(),
        boxDefaults(4.0, 0.25, 4.0),
        kHasTransform | kHasBounds | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::None,
        CreativePlacementOrientationPolicy::DescriptorDefault,
        CreativePlacementFace::PositiveZ,
        CreativePlacementStoragePolicy::VoxelCell
    ),
    descriptor(
        CreativeObjectKind::Roof,
        CreativeObjectCategory::Structural,
        CreativeObjectProfile::BoxStructural,
        CreativeObjectShapeKind::Surface,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "Roof",
        "Roof",
        "exterior upper structural cover",
        structuralCreationDirtyFlags(),
        boxDefaults(5.0, 1.0, 5.0),
        kHasTransform | kHasBounds | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::None,
        CreativePlacementOrientationPolicy::DescriptorDefault,
        CreativePlacementFace::PositiveZ,
        CreativePlacementStoragePolicy::VoxelCell
    ),
    descriptor(
        CreativeObjectKind::Door,
        CreativeObjectCategory::Structural,
        CreativeObjectProfile::Attachment,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "Door",
        "Door",
        "openable passage attachment",
        structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Logic),
        boxDefaults(1.0, 2.25, 0.2),
        kHasTransform | kHasBounds | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::None,
        CreativePlacementOrientationPolicy::CardinalFaceOrPlacerFacing
    ),
    descriptor(
        CreativeObjectKind::Window,
        CreativeObjectCategory::Structural,
        CreativeObjectProfile::Attachment,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "Window",
        "Window",
        "wall opening or visual aperture",
        structuralCreationDirtyFlags(),
        boxDefaults(1.5, 1.0, 0.2),
        kHasTransform | kHasBounds | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::None,
        CreativePlacementOrientationPolicy::CardinalFaceOrPlacerFacing
    ),
    descriptor(
        CreativeObjectKind::Stair,
        CreativeObjectCategory::Structural,
        CreativeObjectProfile::BoxStructural,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Navigation,
        "Stair",
        "Stair",
        "stepped traversal structure",
        structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Navigation),
        boxDefaults(2.0, 1.0, 3.0),
        kHasTransform | kHasBounds | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::Ramp,
        CreativeObjectCategory::Structural,
        CreativeObjectProfile::BoxStructural,
        CreativeObjectShapeKind::Surface,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Navigation,
        "Ramp",
        "Ramp",
        "sloped traversal structure",
        structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Navigation),
        boxDefaults(2.0, 1.0, 4.0),
        kHasTransform | kHasBounds | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::Platform,
        CreativeObjectCategory::Structural,
        CreativeObjectProfile::BoxStructural,
        CreativeObjectShapeKind::Surface,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Navigation,
        "Platform",
        "Platform",
        "static traversal platform",
        structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Navigation),
        boxDefaults(3.0, 0.35, 3.0),
        kHasTransform | kHasBounds | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::MovingPlatform,
        CreativeObjectCategory::Structural,
        CreativeObjectProfile::BoxStructural,
        CreativeObjectShapeKind::Surface,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Navigation,
        "MovingPlatform",
        "Moving Platform",
        "animated traversal platform",
        structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Logic) | flagValue(CreativeObjectDirtyFlag::Navigation),
        boxDefaults(3.0, 0.35, 3.0),
        kHasTransform | kHasBounds | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::Column,
        CreativeObjectCategory::Structural,
        CreativeObjectProfile::BoxStructural,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "Column",
        "Column",
        "vertical support structure",
        structuralCreationDirtyFlags(),
        boxDefaults(0.75, 3.0, 0.75),
        kHasTransform | kHasBounds | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::Pillar,
        CreativeObjectCategory::Structural,
        CreativeObjectProfile::BoxStructural,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "Pillar",
        "Pillar",
        "thick vertical support structure",
        structuralCreationDirtyFlags(),
        boxDefaults(1.0, 3.0, 1.0),
        kHasTransform | kHasBounds | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::Beam,
        CreativeObjectCategory::Structural,
        CreativeObjectProfile::BoxStructural,
        CreativeObjectShapeKind::Line,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "Beam",
        "Beam",
        "horizontal support structure",
        structuralCreationDirtyFlags(),
        boxDefaults(4.0, 0.35, 0.35),
        kHasTransform | kHasBounds | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::Arch,
        CreativeObjectCategory::Structural,
        CreativeObjectProfile::BoxStructural,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "Arch",
        "Arch",
        "arched structural opening",
        structuralCreationDirtyFlags(),
        boxDefaults(3.0, 3.0, 0.5),
        kHasTransform | kHasBounds | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::None,
        CreativePlacementOrientationPolicy::CardinalFaceOrPlacerFacing
    ),
    descriptor(
        CreativeObjectKind::Fence,
        CreativeObjectCategory::Structural,
        CreativeObjectProfile::BoxStructural,
        CreativeObjectShapeKind::Line,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "Fence",
        "Fence",
        "thin boundary structure",
        structuralCreationDirtyFlags(),
        boxDefaults(4.0, 1.25, 0.2),
        kHasTransform | kHasBounds | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::None,
        CreativePlacementOrientationPolicy::CardinalFaceOrPlacerFacing
    ),
    descriptor(
        CreativeObjectKind::Railing,
        CreativeObjectCategory::Structural,
        CreativeObjectProfile::BoxStructural,
        CreativeObjectShapeKind::Line,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "Railing",
        "Railing",
        "edge safety or visual rail",
        structuralCreationDirtyFlags(),
        boxDefaults(4.0, 1.0, 0.2),
        kHasTransform | kHasBounds | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::None,
        CreativePlacementOrientationPolicy::CardinalFaceOrPlacerFacing
    ),
    descriptor(
        CreativeObjectKind::Bridge,
        CreativeObjectCategory::Structural,
        CreativeObjectProfile::BoxStructural,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Navigation,
        "Bridge",
        "Bridge",
        "traversable span between regions",
        structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Navigation),
        boxDefaults(3.0, 0.35, 8.0),
        kHasTransform | kHasBounds | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::Ladder,
        CreativeObjectCategory::Structural,
        CreativeObjectProfile::BoxStructural,
        CreativeObjectShapeKind::Line,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Navigation,
        "Ladder",
        "Ladder",
        "vertical climb structure",
        structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Navigation),
        boxDefaults(0.75, 4.0, 0.2),
        kHasTransform | kHasBounds | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::None,
        CreativePlacementOrientationPolicy::CardinalFaceOrPlacerFacing
    )
});

constexpr auto kTerrainOrVolumeDescriptors = std::to_array<CreativeObjectDescriptor>({
    descriptor(
        CreativeObjectKind::TerrainPatch,
        CreativeObjectCategory::TerrainOrVolume,
        CreativeObjectProfile::TerrainVolume,
        CreativeObjectShapeKind::Surface,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "TerrainPatch",
        "Terrain Patch",
        "authored terrain surface region",
        structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Navigation),
        boxDefaults(8.0, 0.5, 8.0),
        kHasTransform | kHasBounds | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::WaterVolume,
        CreativeObjectCategory::TerrainOrVolume,
        CreativeObjectProfile::TerrainVolume,
        CreativeObjectShapeKind::BoxVolume,
        CreativeSpatialProjectionProfile::VolumeProjection,
        CreativeSpatialOccupancyKind::Unknown,
        "WaterVolume",
        "Water Volume",
        "water gameplay or visual volume",
        structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay),
        boxDefaults(4.0, 1.0, 4.0),
        kHasTransform | kHasBounds | kRuntimeMeaningful
    ),
    descriptor(
        CreativeObjectKind::LavaVolume,
        CreativeObjectCategory::TerrainOrVolume,
        CreativeObjectProfile::TerrainVolume,
        CreativeObjectShapeKind::BoxVolume,
        CreativeSpatialProjectionProfile::VolumeProjection,
        CreativeSpatialOccupancyKind::Unknown,
        "LavaVolume",
        "Lava Volume",
        "hazardous liquid volume",
        structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay),
        boxDefaults(4.0, 1.0, 4.0),
        kHasTransform | kHasBounds | kRuntimeMeaningful
    ),
    descriptor(
        CreativeObjectKind::Pit,
        CreativeObjectCategory::TerrainOrVolume,
        CreativeObjectProfile::TerrainVolume,
        CreativeObjectShapeKind::BoxVolume,
        CreativeSpatialProjectionProfile::VolumeProjection,
        CreativeSpatialOccupancyKind::Collision,
        "Pit",
        "Pit",
        "fall or hazard depression",
        structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay),
        boxDefaults(3.0, 3.0, 3.0),
        kHasTransform | kHasBounds | kRuntimeMeaningful
    ),
    descriptor(
        CreativeObjectKind::Slope,
        CreativeObjectCategory::TerrainOrVolume,
        CreativeObjectProfile::TerrainVolume,
        CreativeObjectShapeKind::Surface,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "Slope",
        "Slope",
        "inclined terrain surface",
        structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Navigation),
        boxDefaults(3.0, 1.0, 4.0),
        kHasTransform | kHasBounds | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::Cliff,
        CreativeObjectCategory::TerrainOrVolume,
        CreativeObjectProfile::TerrainVolume,
        CreativeObjectShapeKind::Surface,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "Cliff",
        "Cliff",
        "vertical terrain boundary",
        structuralCreationDirtyFlags(),
        boxDefaults(4.0, 4.0, 1.0),
        kHasTransform | kHasBounds | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::CaveOpening,
        CreativeObjectCategory::TerrainOrVolume,
        CreativeObjectProfile::TerrainVolume,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "CaveOpening",
        "Cave Opening",
        "terrain entrance aperture",
        structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Navigation),
        boxDefaults(3.0, 2.5, 1.0),
        kHasTransform | kHasBounds | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::BoundaryVolume,
        CreativeObjectCategory::TerrainOrVolume,
        CreativeObjectProfile::Volume,
        CreativeObjectShapeKind::BoxVolume,
        CreativeSpatialProjectionProfile::VolumeProjection,
        CreativeSpatialOccupancyKind::Collision,
        "BoundaryVolume",
        "Boundary Volume",
        "world boundary or containment volume",
        structuralCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay),
        boxDefaults(10.0, 4.0, 10.0),
        kHasTransform | kHasBounds | kRuntimeMeaningful
    ),
    descriptor(
        CreativeObjectKind::KillPlane,
        CreativeObjectCategory::TerrainOrVolume,
        CreativeObjectProfile::Volume,
        CreativeObjectShapeKind::BoxVolume,
        CreativeSpatialProjectionProfile::VolumeProjection,
        CreativeSpatialOccupancyKind::Collision,
        "KillPlane",
        "Kill Plane",
        "fall reset or death boundary",
        logicCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Bounds),
        boxDefaults(10.0, 0.1, 10.0),
        kHasTransform | kHasBounds | kRuntimeMeaningful
    )
});

constexpr auto kNavigationOrMovementDescriptors = std::to_array<CreativeObjectDescriptor>({
    descriptor(
        CreativeObjectKind::SpawnPoint,
        CreativeObjectCategory::NavigationOrMovement,
        CreativeObjectProfile::Marker,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Navigation,
        "SpawnPoint",
        "Spawn Point",
        "player or entity start marker",
        navigationCreationDirtyFlags(),
        markerDefaults(),
        kHasTransform | kRuntimeMeaningful | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::Spawn
    ),
    descriptor(
        CreativeObjectKind::ExitPoint,
        CreativeObjectCategory::NavigationOrMovement,
        CreativeObjectProfile::Marker,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Navigation,
        "ExitPoint",
        "Exit Point",
        "level exit marker",
        navigationCreationDirtyFlags(),
        markerDefaults(),
        kHasTransform | kRuntimeMeaningful | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::Exit
    ),
    descriptor(
        CreativeObjectKind::EntrancePoint,
        CreativeObjectCategory::NavigationOrMovement,
        CreativeObjectProfile::Marker,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Navigation,
        "EntrancePoint",
        "Entrance Point",
        "level entrance marker",
        navigationCreationDirtyFlags(),
        markerDefaults(),
        kHasTransform | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::Checkpoint,
        CreativeObjectCategory::NavigationOrMovement,
        CreativeObjectProfile::Marker,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Navigation,
        "Checkpoint",
        "Checkpoint",
        "progress or respawn checkpoint",
        navigationCreationDirtyFlags(),
        markerDefaults(),
        kHasTransform | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::NavRegion,
        CreativeObjectCategory::NavigationOrMovement,
        CreativeObjectProfile::Volume,
        CreativeObjectShapeKind::BoxVolume,
        CreativeSpatialProjectionProfile::VolumeProjection,
        CreativeSpatialOccupancyKind::Navigation,
        "NavRegion",
        "Navigation Region",
        "navigation cost or traversal region",
        navigationCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Bounds),
        boxDefaults(4.0, 2.0, 4.0),
        kHasTransform | kHasBounds | kRuntimeMeaningful
    ),
    descriptor(
        CreativeObjectKind::NavLink,
        CreativeObjectCategory::NavigationOrMovement,
        CreativeObjectProfile::LinkOrRoute,
        CreativeObjectShapeKind::Line,
        CreativeSpatialProjectionProfile::LinkProjection,
        CreativeSpatialOccupancyKind::Navigation,
        "NavLink",
        "Navigation Link",
        "direct navigation connection",
        navigationCreationDirtyFlags(),
        markerDefaults(),
        kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::JumpLink,
        CreativeObjectCategory::NavigationOrMovement,
        CreativeObjectProfile::LinkOrRoute,
        CreativeObjectShapeKind::Line,
        CreativeSpatialProjectionProfile::LinkProjection,
        CreativeSpatialOccupancyKind::Navigation,
        "JumpLink",
        "Jump Link",
        "jump traversal connection",
        navigationCreationDirtyFlags(),
        markerDefaults(),
        kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::ClimbLink,
        CreativeObjectCategory::NavigationOrMovement,
        CreativeObjectProfile::LinkOrRoute,
        CreativeObjectShapeKind::Line,
        CreativeSpatialProjectionProfile::LinkProjection,
        CreativeSpatialOccupancyKind::Navigation,
        "ClimbLink",
        "Climb Link",
        "climb traversal connection",
        navigationCreationDirtyFlags(),
        markerDefaults(),
        kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::WallRunSurface,
        CreativeObjectCategory::NavigationOrMovement,
        CreativeObjectProfile::MovementTest,
        CreativeObjectShapeKind::Surface,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Navigation,
        "WallRunSurface",
        "Wall Run Surface",
        "wall-runable surface with movement tuning rules",
        testingCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Navigation),
        boxDefaults(0.25, 3.0, 6.0),
        kHasTransform | kHasBounds | kRuntimeMeaningful | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::None,
        CreativePlacementOrientationPolicy::CardinalFaceOrPlacerFacing,
        CreativePlacementFace::PositiveX
    ),
    descriptor(
        CreativeObjectKind::SlideSurface,
        CreativeObjectCategory::NavigationOrMovement,
        CreativeObjectProfile::MovementTest,
        CreativeObjectShapeKind::Surface,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Navigation,
        "SlideSurface",
        "Slide Surface",
        "slideable surface with movement tuning rules",
        testingCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Navigation),
        boxDefaults(3.0, 0.25, 6.0),
        kHasTransform | kHasBounds | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::CoverPoint,
        CreativeObjectCategory::NavigationOrMovement,
        CreativeObjectProfile::Marker,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Navigation,
        "CoverPoint",
        "Cover Point",
        "AI cover marker",
        navigationCreationDirtyFlags(),
        markerDefaults(),
        kHasTransform | kRuntimeMeaningful | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::Cover
    ),
    descriptor(
        CreativeObjectKind::PatrolNode,
        CreativeObjectCategory::NavigationOrMovement,
        CreativeObjectProfile::Marker,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Navigation,
        "PatrolNode",
        "Patrol Node",
        "AI patrol path point",
        navigationCreationDirtyFlags(),
        markerDefaults(),
        kHasTransform | kRuntimeMeaningful | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::PatrolPost
    )
});

constexpr auto kLogicDescriptors = std::to_array<CreativeObjectDescriptor>({
    descriptor(
        CreativeObjectKind::TriggerZone,
        CreativeObjectCategory::Logic,
        CreativeObjectProfile::LogicNode,
        CreativeObjectShapeKind::BoxVolume,
        CreativeSpatialProjectionProfile::VolumeProjection,
        CreativeSpatialOccupancyKind::Trigger,
        "TriggerZone",
        "Trigger Zone",
        "event trigger volume",
        logicCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Bounds),
        boxDefaults(2.0, 2.0, 2.0),
        kHasTransform | kHasBounds | kRuntimeMeaningful
    ),
    descriptor(
        CreativeObjectKind::Switch,
        CreativeObjectCategory::Logic,
        CreativeObjectProfile::LogicNode,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Gameplay,
        "Switch",
        "Switch",
        "binary interaction node",
        logicCreationDirtyFlags(),
        markerDefaults(),
        kHasTransform | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::Lever,
        CreativeObjectCategory::Logic,
        CreativeObjectProfile::LogicNode,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Gameplay,
        "Lever",
        "Lever",
        "directional interaction node",
        logicCreationDirtyFlags(),
        markerDefaults(),
        kHasTransform | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::PressurePlate,
        CreativeObjectCategory::Logic,
        CreativeObjectProfile::LogicNode,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::VolumeProjection,
        CreativeSpatialOccupancyKind::Trigger,
        "PressurePlate",
        "Pressure Plate",
        "weight or presence activated trigger",
        logicCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Bounds),
        boxDefaults(1.5, 0.1, 1.5),
        kHasTransform | kHasBounds | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::Button,
        CreativeObjectCategory::Logic,
        CreativeObjectProfile::LogicNode,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Gameplay,
        "Button",
        "Button",
        "press interaction node",
        logicCreationDirtyFlags(),
        markerDefaults(),
        kHasTransform | kCanHaveParent | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::ConditionGate,
        CreativeObjectCategory::Logic,
        CreativeObjectProfile::LogicNode,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::NoProjection,
        CreativeSpatialOccupancyKind::Gameplay,
        "ConditionGate",
        "Condition Gate",
        "logic condition checkpoint",
        logicCreationDirtyFlags(),
        markerDefaults(),
        kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::EventRelay,
        CreativeObjectCategory::Logic,
        CreativeObjectProfile::LogicNode,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::NoProjection,
        CreativeSpatialOccupancyKind::Gameplay,
        "EventRelay",
        "Event Relay",
        "event forwarding node",
        logicCreationDirtyFlags(),
        markerDefaults(),
        kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::Spawner,
        CreativeObjectCategory::Logic,
        CreativeObjectProfile::LogicNode,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Gameplay,
        "Spawner",
        "Spawner",
        "runtime entity spawn rule",
        logicCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay),
        markerDefaults(),
        kHasTransform | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::DespawnZone,
        CreativeObjectCategory::Logic,
        CreativeObjectProfile::LogicNode,
        CreativeObjectShapeKind::BoxVolume,
        CreativeSpatialProjectionProfile::VolumeProjection,
        CreativeSpatialOccupancyKind::Trigger,
        "DespawnZone",
        "Despawn Zone",
        "runtime entity cleanup volume",
        logicCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Bounds),
        boxDefaults(2.0, 2.0, 2.0),
        kHasTransform | kHasBounds | kRuntimeMeaningful
    ),
    descriptor(
        CreativeObjectKind::ScriptMarker,
        CreativeObjectCategory::Logic,
        CreativeObjectProfile::LogicNode,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Gameplay,
        "ScriptMarker",
        "Script Marker",
        "script hook marker",
        logicCreationDirtyFlags(),
        markerDefaults(),
        kHasTransform | kRuntimeMeaningful | kAuthoringBrushPalette
    )
});

constexpr auto kTestingDescriptors = std::to_array<CreativeObjectDescriptor>({
    descriptor(
        CreativeObjectKind::TestLane,
        CreativeObjectCategory::Testing,
        CreativeObjectProfile::MovementTest,
        CreativeObjectShapeKind::BoxVolume,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Testing,
        "TestLane",
        "Test Lane",
        "movement lab lane container",
        testingCreationDirtyFlags(),
        boxDefaults(3.0, 0.2, 12.0),
        kHasTransform | kHasBounds | kCanOwnChildren
    ),
    descriptor(
        CreativeObjectKind::DistanceMarker,
        CreativeObjectCategory::Testing,
        CreativeObjectProfile::MovementTest,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Testing,
        "DistanceMarker",
        "Distance Marker",
        "known-distance measurement marker",
        testingCreationDirtyFlags(),
        markerDefaults(),
        kHasTransform | kCanHaveParent | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::SpeedMarker,
        CreativeObjectCategory::Testing,
        CreativeObjectProfile::MovementTest,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Testing,
        "SpeedMarker",
        "Speed Marker",
        "speed measurement marker",
        testingCreationDirtyFlags(),
        markerDefaults(),
        kHasTransform | kCanHaveParent | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::JumpTarget,
        CreativeObjectCategory::Testing,
        CreativeObjectProfile::MovementTest,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Testing,
        "JumpTarget",
        "Jump Target",
        "jump tuning target",
        testingCreationDirtyFlags(),
        boxDefaults(1.0, 0.2, 1.0),
        kHasTransform | kHasBounds | kCanHaveParent | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::CoyoteTimeLedge,
        CreativeObjectCategory::Testing,
        CreativeObjectProfile::MovementTest,
        CreativeObjectShapeKind::Surface,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Testing,
        "CoyoteTimeLedge",
        "Coyote-Time Ledge",
        "ledge for coyote-time tuning",
        testingCreationDirtyFlags(),
        boxDefaults(2.0, 0.25, 2.0),
        kHasTransform | kHasBounds | kCanHaveParent | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::FallShaft,
        CreativeObjectCategory::Testing,
        CreativeObjectProfile::MovementTest,
        CreativeObjectShapeKind::BoxVolume,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Testing,
        "FallShaft",
        "Fall Shaft",
        "fall/gravity tuning shaft",
        testingCreationDirtyFlags(),
        boxDefaults(2.0, 8.0, 2.0),
        kHasTransform | kHasBounds | kCanHaveParent
    ),
    descriptor(
        CreativeObjectKind::CollisionProbe,
        CreativeObjectCategory::Testing,
        CreativeObjectProfile::MovementTest,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Testing,
        "CollisionProbe",
        "Collision Probe",
        "collision behavior test probe",
        testingCreationDirtyFlags(),
        markerDefaults(),
        kHasTransform | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::PhysicsProbe,
        CreativeObjectCategory::Testing,
        CreativeObjectProfile::MovementTest,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Testing,
        "PhysicsProbe",
        "Physics Probe",
        "physics behavior test probe",
        testingCreationDirtyFlags(),
        markerDefaults(),
        kHasTransform | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::TimingGate,
        CreativeObjectCategory::Testing,
        CreativeObjectProfile::MovementTest,
        CreativeObjectShapeKind::BoxVolume,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Testing,
        "TimingGate",
        "Timing Gate",
        "timing window measurement gate",
        testingCreationDirtyFlags(),
        boxDefaults(2.0, 2.0, 0.2),
        kHasTransform | kHasBounds | kCanHaveParent
    ),
    descriptor(
        CreativeObjectKind::TestStart,
        CreativeObjectCategory::Testing,
        CreativeObjectProfile::MovementTest,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Testing,
        "TestStart",
        "Test Start",
        "test route start marker",
        testingCreationDirtyFlags(),
        markerDefaults(),
        kHasTransform | kCanHaveParent | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::TestEnd,
        CreativeObjectCategory::Testing,
        CreativeObjectProfile::MovementTest,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Testing,
        "TestEnd",
        "Test End",
        "test route end marker",
        testingCreationDirtyFlags(),
        markerDefaults(),
        kHasTransform | kCanHaveParent | kAuthoringBrushPalette
    )
});

constexpr auto kVisualDressingDescriptors = std::to_array<CreativeObjectDescriptor>({
    descriptor(
        CreativeObjectKind::Prop,
        CreativeObjectCategory::VisualDressing,
        CreativeObjectProfile::Dressing,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "Prop",
        "Prop",
        "generic visual object",
        commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Transform) | flagValue(CreativeObjectDirtyFlag::Bounds),
        boxDefaults(1.0, 1.0, 1.0),
        kHasTransform | kHasBounds | kCanHaveParent | kCanOwnChildren | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::Decal,
        CreativeObjectCategory::VisualDressing,
        CreativeObjectProfile::Dressing,
        CreativeObjectShapeKind::Surface,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Unknown,
        "Decal",
        "Decal",
        "surface visual decal",
        commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Transform),
        boxDefaults(1.0, 0.01, 1.0),
        kHasTransform | kHasBounds | kCanHaveParent | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::Sign,
        CreativeObjectCategory::VisualDressing,
        CreativeObjectProfile::Dressing,
        CreativeObjectShapeKind::Surface,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Unknown,
        "Sign",
        "Sign",
        "visual sign object",
        commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Transform),
        boxDefaults(1.5, 1.0, 0.1),
        kHasTransform | kHasBounds | kCanHaveParent | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::None,
        CreativePlacementOrientationPolicy::CardinalFaceOrPlacerFacing
    ),
    descriptor(
        CreativeObjectKind::Banner,
        CreativeObjectCategory::VisualDressing,
        CreativeObjectProfile::Dressing,
        CreativeObjectShapeKind::Surface,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Unknown,
        "Banner",
        "Banner",
        "hanging visual banner",
        commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Transform),
        boxDefaults(1.5, 2.0, 0.05),
        kHasTransform | kHasBounds | kCanHaveParent | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::None,
        CreativePlacementOrientationPolicy::CardinalFaceOrPlacerFacing
    ),
    descriptor(
        CreativeObjectKind::FoliagePatch,
        CreativeObjectCategory::VisualDressing,
        CreativeObjectProfile::Dressing,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Unknown,
        "FoliagePatch",
        "Foliage Patch",
        "visual vegetation patch",
        commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Transform) | flagValue(CreativeObjectDirtyFlag::Bounds),
        boxDefaults(2.0, 1.0, 2.0),
        kHasTransform | kHasBounds | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::Rock,
        CreativeObjectCategory::VisualDressing,
        CreativeObjectProfile::Dressing,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "Rock",
        "Rock",
        "visual or collision rock prop",
        structuralCreationDirtyFlags(),
        boxDefaults(1.0, 1.0, 1.0),
        kHasTransform | kHasBounds | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::Crate,
        CreativeObjectCategory::VisualDressing,
        CreativeObjectProfile::Dressing,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "Crate",
        "Crate",
        "box prop",
        structuralCreationDirtyFlags(),
        boxDefaults(1.0, 1.0, 1.0),
        kHasTransform | kHasBounds | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::Barrel,
        CreativeObjectCategory::VisualDressing,
        CreativeObjectProfile::Dressing,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "Barrel",
        "Barrel",
        "barrel prop",
        structuralCreationDirtyFlags(),
        boxDefaults(0.8, 1.2, 0.8),
        kHasTransform | kHasBounds | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::Furniture,
        CreativeObjectCategory::VisualDressing,
        CreativeObjectProfile::Dressing,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Structural,
        "Furniture",
        "Furniture",
        "furniture prop",
        structuralCreationDirtyFlags(),
        boxDefaults(2.0, 1.0, 1.0),
        kHasTransform | kHasBounds | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::Decoration,
        CreativeObjectCategory::VisualDressing,
        CreativeObjectProfile::Dressing,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Unknown,
        "Decoration",
        "Decoration",
        "generic decoration object",
        commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Transform),
        markerDefaults(),
        kHasTransform | kCanHaveParent | kAuthoringBrushPalette
    )
});

constexpr auto kLightSoundOrCameraDescriptors = std::to_array<CreativeObjectDescriptor>({
    descriptor(
        CreativeObjectKind::PointLight,
        CreativeObjectCategory::LightSoundOrCamera,
        CreativeObjectProfile::Light,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Light,
        "PointLight",
        "Point Light",
        "omnidirectional light emitter",
        sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Lighting),
        markerDefaults(),
        kHasTransform | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::Light
    ),
    descriptor(
        CreativeObjectKind::SpotLight,
        CreativeObjectCategory::LightSoundOrCamera,
        CreativeObjectProfile::Light,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Light,
        "SpotLight",
        "Spot Light",
        "directional cone light emitter",
        sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Lighting),
        markerDefaults(),
        kHasTransform | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::Light
    ),
    descriptor(
        CreativeObjectKind::AreaLight,
        CreativeObjectCategory::LightSoundOrCamera,
        CreativeObjectProfile::Light,
        CreativeObjectShapeKind::Surface,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Light,
        "AreaLight",
        "Area Light",
        "area-based light emitter",
        sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Lighting) | flagValue(CreativeObjectDirtyFlag::Bounds),
        boxDefaults(2.0, 0.1, 2.0),
        kHasTransform | kHasBounds | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::AmbientZone,
        CreativeObjectCategory::LightSoundOrCamera,
        CreativeObjectProfile::Light,
        CreativeObjectShapeKind::BoxVolume,
        CreativeSpatialProjectionProfile::VolumeProjection,
        CreativeSpatialOccupancyKind::Light,
        "AmbientZone",
        "Ambient Zone",
        "ambient lighting volume",
        sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Lighting) | flagValue(CreativeObjectDirtyFlag::Bounds),
        boxDefaults(4.0, 3.0, 4.0),
        kHasTransform | kHasBounds
    ),
    descriptor(
        CreativeObjectKind::ReverbZone,
        CreativeObjectCategory::LightSoundOrCamera,
        CreativeObjectProfile::Audio,
        CreativeObjectShapeKind::BoxVolume,
        CreativeSpatialProjectionProfile::VolumeProjection,
        CreativeSpatialOccupancyKind::Audio,
        "ReverbZone",
        "Reverb Zone",
        "audio reverb volume",
        sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Audio) | flagValue(CreativeObjectDirtyFlag::Bounds),
        boxDefaults(4.0, 3.0, 4.0),
        kHasTransform | kHasBounds
    ),
    descriptor(
        CreativeObjectKind::SoundEmitter,
        CreativeObjectCategory::LightSoundOrCamera,
        CreativeObjectProfile::Audio,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Audio,
        "SoundEmitter",
        "Sound Emitter",
        "positional sound source",
        sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Audio),
        markerDefaults(),
        kHasTransform | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::Audio
    ),
    descriptor(
        CreativeObjectKind::MusicZone,
        CreativeObjectCategory::LightSoundOrCamera,
        CreativeObjectProfile::Audio,
        CreativeObjectShapeKind::BoxVolume,
        CreativeSpatialProjectionProfile::VolumeProjection,
        CreativeSpatialOccupancyKind::Audio,
        "MusicZone",
        "Music Zone",
        "music cue trigger volume",
        sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Audio) | flagValue(CreativeObjectDirtyFlag::Bounds),
        boxDefaults(4.0, 3.0, 4.0),
        kHasTransform | kHasBounds
    ),
    descriptor(
        CreativeObjectKind::CameraMarker,
        CreativeObjectCategory::LightSoundOrCamera,
        CreativeObjectProfile::Camera,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Camera,
        "CameraMarker",
        "Camera Marker",
        "camera placement marker",
        sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Camera),
        markerDefaults(),
        kHasTransform | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::Camera
    ),
    descriptor(
        CreativeObjectKind::CameraRail,
        CreativeObjectCategory::LightSoundOrCamera,
        CreativeObjectProfile::Camera,
        CreativeObjectShapeKind::Line,
        CreativeSpatialProjectionProfile::LineProjection,
        CreativeSpatialOccupancyKind::Camera,
        "CameraRail",
        "Camera Rail",
        "camera path or dolly rail",
        sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Camera) | flagValue(CreativeObjectDirtyFlag::Bounds),
        boxDefaults(1.0, 1.0, 4.0),
        kHasTransform | kHasBounds | kCanOwnChildren | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::CameraTarget,
        CreativeObjectCategory::LightSoundOrCamera,
        CreativeObjectProfile::Camera,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Camera,
        "CameraTarget",
        "Camera Target",
        "camera look-at target",
        sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Camera),
        markerDefaults(),
        kHasTransform | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::Camera
    ),
    descriptor(
        CreativeObjectKind::CutsceneMarker,
        CreativeObjectCategory::LightSoundOrCamera,
        CreativeObjectProfile::Camera,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::NoProjection,
        CreativeSpatialOccupancyKind::Camera,
        "CutsceneMarker",
        "Cutscene Marker",
        "cutscene timing or camera marker",
        sensoryCreationDirtyFlags(CreativeObjectDirtyFlag::Camera) | flagValue(CreativeObjectDirtyFlag::Logic),
        markerDefaults(),
        kHasTransform | kRuntimeMeaningful | kAuthoringBrushPalette
    )
});

constexpr auto kAuthoringMetaDescriptors = std::to_array<CreativeObjectDescriptor>({
    descriptor(
        CreativeObjectKind::Note,
        CreativeObjectCategory::AuthoringMeta,
        CreativeObjectProfile::TextAnnotation,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Authoring,
        "Note",
        "Note",
        "authoring note",
        commonAuthoredDirtyFlags(),
        markerDefaults(),
        kHasTransform | kEditorOnly
    ),
    descriptor(
        CreativeObjectKind::Label,
        CreativeObjectCategory::AuthoringMeta,
        CreativeObjectProfile::TextAnnotation,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Authoring,
        "Label",
        "Label",
        "visible authoring label",
        commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Preview),
        markerDefaults(),
        kHasTransform | kEditorOnly
    ),
    descriptor(
        CreativeObjectKind::Comment,
        CreativeObjectCategory::AuthoringMeta,
        CreativeObjectProfile::TextAnnotation,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Authoring,
        "Comment",
        "Comment",
        "review or design comment",
        commonAuthoredDirtyFlags(),
        markerDefaults(),
        kHasTransform | kEditorOnly
    ),
    descriptor(
        CreativeObjectKind::MeasurementMarker,
        CreativeObjectCategory::AuthoringMeta,
        CreativeObjectProfile::AuthoringHelper,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Authoring,
        "MeasurementMarker",
        "Measurement Marker",
        "single-point measurement helper",
        testingCreationDirtyFlags(),
        markerDefaults(),
        kHasTransform | kEditorOnly
    ),
    descriptor(
        CreativeObjectKind::MeasurementLine,
        CreativeObjectCategory::AuthoringMeta,
        CreativeObjectProfile::AuthoringHelper,
        CreativeObjectShapeKind::Line,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Authoring,
        "MeasurementLine",
        "Measurement Line",
        "line measurement helper",
        testingCreationDirtyFlags(),
        boxDefaults(1.0, 0.05, 0.05),
        kHasTransform | kHasBounds | kEditorOnly
    ),
    descriptor(
        CreativeObjectKind::MeasurementBox,
        CreativeObjectCategory::AuthoringMeta,
        CreativeObjectProfile::AuthoringHelper,
        CreativeObjectShapeKind::BoxVolume,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Authoring,
        "MeasurementBox",
        "Measurement Box",
        "box measurement helper",
        testingCreationDirtyFlags(),
        boxDefaults(1.0, 1.0, 1.0),
        kHasTransform | kHasBounds | kEditorOnly
    ),
    descriptor(
        CreativeObjectKind::GridAnchor,
        CreativeObjectCategory::AuthoringMeta,
        CreativeObjectProfile::AuthoringHelper,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Authoring,
        "GridAnchor",
        "Grid Anchor",
        "grid alignment anchor",
        commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Preview),
        markerDefaults(),
        kHasTransform | kEditorOnly
    ),
    descriptor(
        CreativeObjectKind::SnapAnchor,
        CreativeObjectCategory::AuthoringMeta,
        CreativeObjectProfile::AuthoringHelper,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Authoring,
        "SnapAnchor",
        "Snap Anchor",
        "snap alignment anchor",
        commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Preview),
        markerDefaults(),
        kHasTransform | kEditorOnly
    ),
    descriptor(
        CreativeObjectKind::ReferenceImage,
        CreativeObjectCategory::AuthoringMeta,
        CreativeObjectProfile::AuthoringHelper,
        CreativeObjectShapeKind::Surface,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Authoring,
        "ReferenceImage",
        "Reference Image",
        "placed reference image plane",
        commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Preview),
        boxDefaults(4.0, 0.01, 4.0),
        kHasTransform | kHasBounds | kEditorOnly
    ),
    descriptor(
        CreativeObjectKind::BlueprintOverlay,
        CreativeObjectCategory::AuthoringMeta,
        CreativeObjectProfile::AuthoringHelper,
        CreativeObjectShapeKind::Surface,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Authoring,
        "BlueprintOverlay",
        "Blueprint Overlay",
        "transparent planning overlay",
        commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Preview),
        boxDefaults(4.0, 0.01, 4.0),
        kHasTransform | kHasBounds | kEditorOnly
    ),
    descriptor(
        CreativeObjectKind::Group,
        CreativeObjectCategory::AuthoringMeta,
        CreativeObjectProfile::AuthoringHelper,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::NoProjection,
        CreativeSpatialOccupancyKind::Authoring,
        "Group",
        "Group",
        "object grouping container",
        commonAuthoredDirtyFlags(),
        markerDefaults(),
        kHasTransform | kCanHaveParent | kCanOwnChildren | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::PrefabInstance,
        CreativeObjectCategory::AuthoringMeta,
        CreativeObjectProfile::Dressing,
        CreativeObjectShapeKind::MeshProxy,
        CreativeSpatialProjectionProfile::BoxProjection,
        CreativeSpatialOccupancyKind::Unknown,
        "PrefabInstance",
        "Prefab Instance",
        "placed reusable authored package",
        structuralCreationDirtyFlags(),
        boxDefaults(1.0, 1.0, 1.0),
        kHasTransform | kHasBounds | kCanHaveParent | kCanOwnChildren | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::Socket,
        CreativeObjectCategory::AuthoringMeta,
        CreativeObjectProfile::Attachment,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Authoring,
        "Socket",
        "Socket",
        "named attachment location",
        commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Transform),
        markerDefaults(),
        kHasTransform | kCanHaveParent | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::AttachmentPoint,
        CreativeObjectCategory::AuthoringMeta,
        CreativeObjectProfile::Attachment,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Authoring,
        "AttachmentPoint",
        "Attachment Point",
        "generic attachment location",
        commonAuthoredDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Transform),
        markerDefaults(),
        kHasTransform | kCanHaveParent | kAuthoringBrushPalette
    )
});

constexpr auto kGameplayDescriptors = std::to_array<CreativeObjectDescriptor>({
    descriptor(
        CreativeObjectKind::EnemySpawn,
        CreativeObjectCategory::Gameplay,
        CreativeObjectProfile::GameplayMarker,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Gameplay,
        "EnemySpawn",
        "Enemy Spawn",
        "enemy spawn marker",
        navigationCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay),
        markerDefaults(),
        kHasTransform | kRuntimeMeaningful | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::Monster
    ),
    descriptor(
        CreativeObjectKind::NpcSpawn,
        CreativeObjectCategory::Gameplay,
        CreativeObjectProfile::GameplayMarker,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Gameplay,
        "NpcSpawn",
        "NPC Spawn",
        "NPC spawn marker",
        navigationCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay),
        markerDefaults(),
        kHasTransform | kRuntimeMeaningful | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::Npc
    ),
    descriptor(
        CreativeObjectKind::PatrolRoute,
        CreativeObjectCategory::Gameplay,
        CreativeObjectProfile::LinkOrRoute,
        CreativeObjectShapeKind::Path,
        CreativeSpatialProjectionProfile::PathProjection,
        CreativeSpatialOccupancyKind::Gameplay,
        "PatrolRoute",
        "Patrol Route",
        "AI patrol route",
        navigationCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay),
        markerDefaults(),
        kCanOwnChildren | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::InterestPoint,
        CreativeObjectCategory::Gameplay,
        CreativeObjectProfile::GameplayMarker,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Gameplay,
        "InterestPoint",
        "Interest Point",
        "AI interest marker",
        navigationCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay),
        markerDefaults(),
        kHasTransform | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::AlertZone,
        CreativeObjectCategory::Gameplay,
        CreativeObjectProfile::Volume,
        CreativeObjectShapeKind::BoxVolume,
        CreativeSpatialProjectionProfile::VolumeProjection,
        CreativeSpatialOccupancyKind::Gameplay,
        "AlertZone",
        "Alert Zone",
        "AI alert behavior volume",
        logicCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Bounds),
        boxDefaults(4.0, 2.0, 4.0),
        kHasTransform | kHasBounds | kRuntimeMeaningful
    ),
    descriptor(
        CreativeObjectKind::SafeZone,
        CreativeObjectCategory::Gameplay,
        CreativeObjectProfile::Volume,
        CreativeObjectShapeKind::BoxVolume,
        CreativeSpatialProjectionProfile::VolumeProjection,
        CreativeSpatialOccupancyKind::Gameplay,
        "SafeZone",
        "Safe Zone",
        "safe gameplay volume",
        logicCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Bounds),
        boxDefaults(4.0, 2.0, 4.0),
        kHasTransform | kHasBounds | kRuntimeMeaningful
    ),
    descriptor(
        CreativeObjectKind::DangerZone,
        CreativeObjectCategory::Gameplay,
        CreativeObjectProfile::Volume,
        CreativeObjectShapeKind::BoxVolume,
        CreativeSpatialProjectionProfile::VolumeProjection,
        CreativeSpatialOccupancyKind::Gameplay,
        "DangerZone",
        "Danger Zone",
        "danger gameplay volume",
        logicCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Bounds),
        boxDefaults(4.0, 2.0, 4.0),
        kHasTransform | kHasBounds | kRuntimeMeaningful
    ),
    descriptor(
        CreativeObjectKind::ResourceNode,
        CreativeObjectCategory::Gameplay,
        CreativeObjectProfile::GameplayMarker,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Gameplay,
        "ResourceNode",
        "Resource Node",
        "resource gathering marker",
        navigationCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay),
        markerDefaults(),
        kHasTransform | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::LootPoint,
        CreativeObjectCategory::Gameplay,
        CreativeObjectProfile::GameplayMarker,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Gameplay,
        "LootPoint",
        "Loot Point",
        "loot placement marker",
        navigationCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay),
        markerDefaults(),
        kHasTransform | kRuntimeMeaningful | kAuthoringBrushPalette,
        CreativeRuntimeAnchorSemantic::Pickup
    ),
    descriptor(
        CreativeObjectKind::QuestMarker,
        CreativeObjectCategory::Gameplay,
        CreativeObjectProfile::GameplayMarker,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Gameplay,
        "QuestMarker",
        "Quest Marker",
        "quest objective marker",
        navigationCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay),
        markerDefaults(),
        kHasTransform | kRuntimeMeaningful | kAuthoringBrushPalette
    ),
    descriptor(
        CreativeObjectKind::DialogueMarker,
        CreativeObjectCategory::Gameplay,
        CreativeObjectProfile::GameplayMarker,
        CreativeObjectShapeKind::Point,
        CreativeSpatialProjectionProfile::PointProjection,
        CreativeSpatialOccupancyKind::Gameplay,
        "DialogueMarker",
        "Dialogue Marker",
        "dialogue trigger or speaker marker",
        navigationCreationDirtyFlags() | flagValue(CreativeObjectDirtyFlag::Gameplay),
        markerDefaults(),
        kHasTransform | kRuntimeMeaningful | kAuthoringBrushPalette
    )
});

constexpr auto kDescriptors = concatDescriptorSections(
    kUnknownDescriptors,
    kStructuralDescriptors,
    kTerrainOrVolumeDescriptors,
    kNavigationOrMovementDescriptors,
    kLogicDescriptors,
    kTestingDescriptors,
    kVisualDressingDescriptors,
    kLightSoundOrCameraDescriptors,
    kAuthoringMetaDescriptors,
    kGameplayDescriptors
);

[[nodiscard]] bool projectionUsesSpatialExtent(
    CreativeSpatialProjectionProfile profile) noexcept {
    switch (profile) {
    case CreativeSpatialProjectionProfile::BoxProjection:
    case CreativeSpatialProjectionProfile::VolumeProjection:
    case CreativeSpatialProjectionProfile::LineProjection:
    case CreativeSpatialProjectionProfile::PathProjection:
    case CreativeSpatialProjectionProfile::LinkProjection:
        return true;
    case CreativeSpatialProjectionProfile::Unknown:
    case CreativeSpatialProjectionProfile::NoProjection:
    case CreativeSpatialProjectionProfile::PointProjection:
        return false;
    }

    return false;
}

[[nodiscard]] CreativeObjectDirtyFlags systemDirtyFlagsForOccupancy(
    CreativeSpatialOccupancyKind occupancyKind) noexcept {
    switch (occupancyKind) {
    case CreativeSpatialOccupancyKind::Structural:
    case CreativeSpatialOccupancyKind::Collision:
        return flagValue(CreativeObjectDirtyFlag::Collision);
    case CreativeSpatialOccupancyKind::Navigation:
        return flagValue(CreativeObjectDirtyFlag::Navigation);
    case CreativeSpatialOccupancyKind::Trigger:
        return flagValue(CreativeObjectDirtyFlag::Logic) |
               flagValue(CreativeObjectDirtyFlag::Gameplay);
    case CreativeSpatialOccupancyKind::Gameplay:
        return flagValue(CreativeObjectDirtyFlag::Gameplay) |
               flagValue(CreativeObjectDirtyFlag::Logic);
    case CreativeSpatialOccupancyKind::Light:
        return flagValue(CreativeObjectDirtyFlag::Lighting);
    case CreativeSpatialOccupancyKind::Audio:
        return flagValue(CreativeObjectDirtyFlag::Audio);
    case CreativeSpatialOccupancyKind::Camera:
        return flagValue(CreativeObjectDirtyFlag::Camera);
    case CreativeSpatialOccupancyKind::Testing:
        return flagValue(CreativeObjectDirtyFlag::Testing);
    case CreativeSpatialOccupancyKind::Unknown:
    case CreativeSpatialOccupancyKind::Authoring:
        return 0;
    }

    return 0;
}

[[nodiscard]] CreativeObjectDirtyFlags transformSpatialDirtyFlags(
    const CreativeObjectDescriptor& descriptor) noexcept {
    CreativeObjectDirtyFlags flags =
        flagValue(CreativeObjectDirtyFlag::Transform) |
        flagValue(CreativeObjectDirtyFlag::Preview);

    if (projectionUsesSpatialExtent(descriptor.projectionProfile)) {
        flags = flags | CreativeObjectDirtyFlag::Bounds |
                CreativeObjectDirtyFlag::Geometry;
    }

    return flags;
}

[[nodiscard]] CreativeObjectDirtyFlags shapeSpatialDirtyFlags(
    const CreativeObjectDescriptor& descriptor) noexcept {
    CreativeObjectDirtyFlags flags =
        flagValue(CreativeObjectDirtyFlag::Bounds) |
        flagValue(CreativeObjectDirtyFlag::Preview);

    if (projectionUsesSpatialExtent(descriptor.projectionProfile)) {
        flags = flags | CreativeObjectDirtyFlag::Geometry;
    }

    return flags;
}

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

std::string_view toString(CreativeObjectShapeKind shapeKind) noexcept {
    switch (shapeKind) {
    case CreativeObjectShapeKind::Unknown: return "Unknown";
    case CreativeObjectShapeKind::Point: return "Point";
    case CreativeObjectShapeKind::Line: return "Line";
    case CreativeObjectShapeKind::BoxVolume: return "BoxVolume";
    case CreativeObjectShapeKind::Surface: return "Surface";
    case CreativeObjectShapeKind::Path: return "Path";
    case CreativeObjectShapeKind::MeshProxy: return "MeshProxy";
    }

    return "Unknown";
}

std::string_view toString(CreativeRuntimeAnchorSemantic semantic) noexcept {
    switch (semantic) {
    case CreativeRuntimeAnchorSemantic::None: return "";
    case CreativeRuntimeAnchorSemantic::Spawn: return "spawn";
    case CreativeRuntimeAnchorSemantic::Exit: return "exit";
    case CreativeRuntimeAnchorSemantic::Npc: return "npc";
    case CreativeRuntimeAnchorSemantic::Monster: return "monster";
    case CreativeRuntimeAnchorSemantic::Pickup: return "pickup";
    case CreativeRuntimeAnchorSemantic::Light: return "light";
    case CreativeRuntimeAnchorSemantic::Audio: return "audio";
    case CreativeRuntimeAnchorSemantic::Camera: return "camera";
    case CreativeRuntimeAnchorSemantic::Cover: return "cover";
    case CreativeRuntimeAnchorSemantic::PatrolPost: return "patrol_post";
    }

    return "";
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

CreativeObjectShapeKind shapeKindForObject(CreativeObjectKind kind) noexcept {
    return describeObject(kind).shapeKind;
}

CreativeObjectDirtyFlags dirtyFlagsForCreation(CreativeObjectKind kind) noexcept {
    return describeObject(kind).creationDirtyFlags;
}

CreativeObjectDirtyFlags dirtyFlagsForMutation(CreativeObjectKind objectKind, CreativeMutationKind mutationKind) noexcept {
    auto flags = commonAuthoredDirtyFlags();
    const auto& descriptor = describeObject(objectKind);
    const auto systemFlags = systemDirtyFlagsForOccupancy(descriptor.occupancyKind);

    if (isTransformMutation(mutationKind)) {
        flags = flags | transformSpatialDirtyFlags(descriptor) | systemFlags;
    }

    if (isShapeMutation(mutationKind)) {
        flags = flags | shapeSpatialDirtyFlags(descriptor) | systemFlags;
    }

    if (isRelationshipMutation(mutationKind)) {
        flags = flags | systemFlags;
    }

    if (isLogicMutation(mutationKind)) {
        flags = flags | CreativeObjectDirtyFlag::Logic | systemFlags;
    }

    if (isNavigationMutation(mutationKind)) {
        flags = flags | systemFlags;
    }

    if (isTestingMutation(mutationKind)) {
        flags = flags | CreativeObjectDirtyFlag::Preview | systemFlags;
    }

    if (isSensoryMutation(mutationKind)) {
        flags = flags | CreativeObjectDirtyFlag::Preview | systemFlags;
    }

    if (isGameplayMutation(mutationKind)) {
        flags = flags | CreativeObjectDirtyFlag::Gameplay | systemFlags;
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

bool descriptorShowsInAuthoringBrushPalette(
    const CreativeObjectDescriptor& descriptor) noexcept {
    return descriptor.authoringPaletteVisibility ==
           CreativeAuthoringPaletteVisibility::Brush;
}

bool objectShowsInAuthoringBrushPalette(CreativeObjectKind kind) noexcept {
    return descriptorShowsInAuthoringBrushPalette(describeObject(kind));
}

CreativePlacementFace creativePlacementFaceFromNormal(
    CreativeVec3 normal) noexcept {
    const double ax = std::fabs(normal.x);
    const double ay = std::fabs(normal.y);
    const double az = std::fabs(normal.z);
    if (!std::isfinite(ax) || !std::isfinite(ay) || !std::isfinite(az) ||
        std::max({ax, ay, az}) <= 1.0e-12) {
        return CreativePlacementFace::Count;
    }
    if (ax >= ay && ax >= az) {
        return normal.x < 0.0 ? CreativePlacementFace::NegativeX
                              : CreativePlacementFace::PositiveX;
    }
    if (ay >= az) {
        return normal.y < 0.0 ? CreativePlacementFace::NegativeY
                              : CreativePlacementFace::PositiveY;
    }
    return normal.z < 0.0 ? CreativePlacementFace::NegativeZ
                          : CreativePlacementFace::PositiveZ;
}

bool creativePlacementPolicyAllowsFace(
    const CreativeObjectPlacementPolicy& policy,
    CreativeVec3 normal) noexcept {
    if (!policy.enabled) {
        return false;
    }
    const CreativePlacementFace face = creativePlacementFaceFromNormal(normal);
    return (policy.allowedFaces & creativePlacementFaceBit(face)) != 0U;
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
        // TD-2: Move stays legal for bounds-only kinds because it places the
        // corner anchor; other transform verbs still require a transform.
        if (mutationKind != CreativeMutationKind::Move || !descriptor.hasBounds) {
            return false;
        }
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
