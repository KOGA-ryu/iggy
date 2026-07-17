cmake_minimum_required(VERSION 3.20)

project(iggy3d VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/iggy3d_options.cmake)
include(cmake/iggy3d_warnings.cmake)
include(cmake/iggy3d_vulkan_deps.cmake)
include(cmake/iggy3d_imgui.cmake)
include(cmake/iggy3d_shaders.cmake)

add_library(iggy3d)

# Deterministic floating point across toolchains. Bar FMA contraction so `a*b + c` performs the
# SAME two roundings on macOS/arm64 (Clang) and the Linux/x86-64 box (GCC). Without this, arm64
# fuses into a single-rounding FMA and diverges ~1 ULP for ordinary inputs, breaking the save/hash
# bit-stability the engine depends on (see core/math/Snap, core/math/EulerRotation). PUBLIC so every
# consumer (apps, tests, and header-inlined math) shares the discipline. Both GCC and Clang accept it.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(iggy3d PUBLIC -ffp-contract=off)
endif()

target_sources(iggy3d
  PRIVATE
    src/core/hash/StableHash.cpp
    src/core/grid/GridFootprint.cpp
    src/core/grid/Reachability.cpp
    src/core/grid/GreedyMesh.cpp
    src/core/spatial/AabbGridIndex.cpp
    src/core/math/Vec3.cpp
    src/core/math/EulerRotation.cpp
    src/core/math/Transform3.cpp
    src/core/math/Aabb3.cpp
    src/core/math/OrientedBox.cpp
    src/core/math/Snap.cpp
    src/core/math/Frustum.cpp
    src/core/math/Mat4.cpp
    src/app/PackageRuntimeLookup.cpp
    src/app/iggy3d/creative/Geometry.cpp
    src/app/iggy3d/creative/assets/AuthoredAsset.cpp
    src/app/iggy3d/creative/document/Object.cpp
    src/app/iggy3d/creative/document/Document.cpp
    src/app/iggy3d/creative/document/DocumentLogicLinks.cpp
    src/app/iggy3d/creative/document/DocumentObjects.cpp
    src/app/iggy3d/creative/document/DocumentRestore.cpp
    src/app/iggy3d/creative/document/DocumentValidation.cpp
    src/app/iggy3d/creative/document/TerrainField.cpp
    src/app/iggy3d/creative/document/TerrainRaycast.cpp
    src/app/iggy3d/creative/document/TerrainRender.cpp
    src/app/iggy3d/creative/document/TerrainMaterialField.cpp
    src/app/iggy3d/creative/document/VoxelField.cpp
    src/app/iggy3d/creative/document/VoxelFieldQueries.cpp
    src/app/iggy3d/creative/mutation/Mutation.cpp
    src/app/iggy3d/creative/mutation/MutationAllowed.cpp
    src/app/iggy3d/creative/document/ObjectDescriptor.cpp
    src/app/iggy3d/creative/mutation/MutationApply.cpp
    src/app/iggy3d/creative/document/DocumentMutation.cpp
    src/app/iggy3d/creative/spatial/SpatialProjection.cpp
    src/app/iggy3d/creative/spatial/SpatialProjectionGrid.cpp
    src/app/iggy3d/creative/spatial/SpatialProjectionPlans.cpp
    src/app/iggy3d/creative/spatial/SurfacePose.cpp
    src/app/iggy3d/creative/spatial/PlacementCompatibility.cpp
    src/app/iggy3d/creative/spatial/PlacementClearance.cpp
    src/app/iggy3d/creative/spatial/PlacementGrid.cpp
    src/app/iggy3d/creative/spatial/PlacementContact.cpp
    src/app/iggy3d/creative/spatial/PlacementOrientation.cpp
    src/app/iggy3d/creative/document/DocumentWireframe.cpp
    src/app/iggy3d/creative/input/ControllerInput.cpp
    src/app/iggy3d/creative/input/ActionHints.cpp
    src/app/iggy3d/creative/input/ControlProfile.cpp
    src/app/iggy3d/creative/input/UiInput.cpp
    src/app/iggy3d/creative/input/InputDefaults.cpp
    src/app/iggy3d/creative/input/InputRouter.cpp
    src/app/iggy3d/creative/input/Catalog.cpp
    src/app/iggy3d/creative/input/ToolWheel.cpp
    src/app/iggy3d/creative/input/Interaction.cpp
    src/app/iggy3d/creative/input/HeldItemRegistry.cpp
    src/app/iggy3d/creative/history/History.cpp
    src/app/iggy3d/creative/tools/TerrainGrade.cpp
    src/app/iggy3d/creative/tools/TerrainBrushKernel.cpp
    src/app/iggy3d/creative/tools/TerrainPath.cpp
    src/app/iggy3d/creative/tools/TerrainPaint.cpp
    src/app/iggy3d/creative/tools/TerrainProfile.cpp
    src/app/iggy3d/creative/tools/TerrainRegion.cpp
    src/app/iggy3d/creative/tools/TerrainSeed.cpp
    src/app/iggy3d/creative/tools/TerrainSculpt.cpp
    src/app/iggy3d/creative/tools/TerrainStamp.cpp
    src/app/iggy3d/creative/tools/ToolOptions.cpp
    src/app/iggy3d/creative/tools/ToolSettingLabels.cpp
    src/app/iggy3d/creative/tools/ToolSettings.cpp
    src/app/iggy3d/creative/tools/Tools.cpp
    src/app/iggy3d/creative/tools/AssetScatter.cpp
    src/app/iggy3d/creative/tools/AttachmentSnap.cpp
    src/app/iggy3d/creative/tools/Select.cpp
    src/app/iggy3d/creative/tools/Clipboard.cpp
    src/app/iggy3d/creative/tools/Group.cpp
    src/app/iggy3d/creative/tools/SelectionPlacement.cpp
    src/app/iggy3d/creative/tools/StructuralPlacement.cpp
    src/app/iggy3d/creative/tools/SelectionTransformCommands.cpp
    src/app/iggy3d/creative/tools/Pattern.cpp
    src/app/iggy3d/creative/tools/MaterialBrush.cpp
    src/app/iggy3d/creative/tools/MaterialBrushPath.cpp
    src/app/iggy3d/creative/tools/ShapeBrush.cpp
    src/app/iggy3d/creative/tools/ConnectedFill.cpp
    src/app/iggy3d/creative/tools/SurfaceExtrude.cpp
    src/app/iggy3d/creative/tools/VolumeSelection.cpp
    src/app/iggy3d/creative/tools/Volume.cpp
    src/app/iggy3d/creative/tools/VolumeFill.cpp
    src/app/iggy3d/creative/tools/VolumeObjects.cpp
    src/app/iggy3d/creative/tools/Measure.cpp
    src/app/iggy3d/creative/spatial/Snap.cpp
    src/app/iggy3d/creative/document/DocumentSnap.cpp
    src/app/iggy3d/creative/adapters/RoomBake.cpp
    src/app/iggy3d/creative/adapters/RoomBakeAssetSurfaces.cpp
    src/app/iggy3d/creative/adapters/RoomBakeFields.cpp
    src/app/iggy3d/creative/adapters/RoomBakeObjects.cpp
    src/app/iggy3d/creative/adapters/RoomBakeGreedyFloors.cpp
    src/app/iggy3d/creative/adapters/RoomBakeReachability.cpp
    src/app/iggy3d/creative/validation/MapValidation.cpp
    src/app/iggy3d/creative/play/PlayPreparation.cpp
    src/app/iggy3d/creative/play/RuntimeInteractables.cpp
    src/app/iggy3d/creative/play/RuntimeMovingPlatforms.cpp
    src/app/iggy3d/creative/play/RuntimeSandbox.cpp
    src/app/iggy3d/creative/spatial/Ghost.cpp
    src/app/iggy3d/creative/recipes/CreativeRecipe.cpp
    src/app/iggy3d/creative/recipes/BuildingRecipe.cpp
    src/app/iggy3d/creative/recipes/StructuralRoofRecipe.cpp
    src/app/iggy3d/creative/recipes/StructuralSurfaceRecipe.cpp
    src/app/iggy3d/creative/recipes/StructuralWallRecipe.cpp
    src/app/iggy3d/creative/recipes/ObjectLibraryRecipe.cpp
    src/app/iggy3d/creative/recipes/TerrainRecipe.cpp
    src/app/iggy3d/creative/world/WorldLayout.cpp
    src/app/iggy3d/creative/world/WorldLayoutLevels.cpp
    src/app/iggy3d/creative/world/WorldLayoutRoofs.cpp
    src/app/iggy3d/creative/world/WorldLayoutRooms.cpp
    src/app/iggy3d/creative/world/WorldLayoutVerticalConnectors.cpp
    src/app/iggy3d/creative/world/WorldLayoutProvenance.cpp
    src/app/iggy3d/creative/world/WorldLayoutBuildingOps.cpp
    src/app/iggy3d/creative/world/WorldLayoutBuildingTemplates.cpp
    src/app/iggy3d/creative/world/WorldLayoutBuildingTemplateSync.cpp
    src/app/iggy3d/creative/world/WorldLayoutBuildingTemplateRefresh.cpp
    src/app/iggy3d/creative/world/WorldLayoutTransform.cpp
    src/app/iggy3d/creative/world/WorldLayoutApply.cpp
    src/app/iggy3d/creative/world/WorldLayoutCodec.cpp
    src/app/iggy3d/creative/ui/UiTheme.cpp
    src/app/iggy3d/creative/ui/UiWidgets.cpp
    src/app/iggy3d/creative/Facade.cpp
    src/app/iggy3d/creative/FacadeDocumentMutations.cpp
    src/app/iggy3d/creative/FacadeObjectCommands.cpp
    src/app/iggy3d/creative/FacadeMoveDrag.cpp
    src/app/iggy3d/creative/camera/Fly.cpp
    src/app/iggy3d/map_maker/Grid.cpp
    src/app/iggy3d/creative/render/WireframeDebugLines.cpp
    src/app/iggy3d/creative/render/CreativeOverlayFrame.cpp
    src/app/iggy3d/creative/render/CreativeScreenProjection.cpp
    src/app/iggy3d/creative/render/CreativeSceneFrame.cpp
    src/app/iggy3d/creative/world/DocumentSection.cpp
    src/app/iggy3d/creative/world/DocumentSectionRecords.cpp
    src/app/iggy3d/creative/world/DocumentSectionFields.cpp
    src/app/iggy3d/creative/world/MapTemplate.cpp
    src/app/iggy3d/creative/world/BuilderEstateTemplate.cpp
    src/app/iggy3d/save/Catalog.cpp
    src/app/iggy3d/save/CatalogProjector.cpp
    src/app/iggy3d/creative/world/WorldService.cpp
    src/app/iggy3d/save/SaveBridge.cpp
    src/app/frontend/SaveSlotModel.cpp
    src/app/platform/ExecutablePath.cpp
    src/content/assets/TraversalTag.cpp
    src/content/assets/ImageDecode.cpp
    src/content/assets/StaticMeshAsset.cpp
    src/content/assets/StaticMeshAuthoringMetadata.cpp
    src/content/assets/StaticMeshMaterialImport.cpp
    src/runtime/player/PlayerRoster.cpp
    src/runtime/player/PlayerPhysicsMovePlanner.cpp
    src/runtime/player/PlayerMotor.cpp
    src/runtime/physics/PhysicsBroadphase.cpp
    src/render/RendererApi.cpp
    src/render/RendererConfig.cpp
    src/render/null/NullRenderer.cpp
    src/runtime/world/WorldState.cpp
    src/runtime/clock/Clock.cpp
    src/runtime/camera/CameraModePolicy.cpp
    src/runtime/ai/NpcBehaviorSystem.cpp
    src/runtime/ai/NpcBehaviorProfile.cpp
    src/runtime/ai/NpcAlertSystem.cpp
    src/runtime/ai/NpcPatrolSystem.cpp
    src/runtime/ai/NpcInvestigateSystem.cpp
    src/runtime/ai/NpcSoundPerception.cpp
    src/runtime/ai/SegmentOcclusion.cpp
    src/runtime/ai/ReasoningGraph.cpp
    src/runtime/ai/ReasoningRoute.cpp
    src/runtime/ai/GuardDecision.cpp
    src/runtime/combat/CombatSystem.cpp
    src/runtime/collision/SpatialSurfaceSet.cpp
    src/runtime/collision/CollisionQuery.cpp
    src/runtime/collision/EntityHitQuery.cpp
    src/runtime/command/CommandAdmission.cpp
    src/runtime/replay/CommandLog.cpp
    src/runtime/session/Session.cpp
    src/runtime/session/SessionAi.cpp
    src/runtime/session/SessionStateTransitions.cpp
    src/runtime/movement/MovementKinematics.cpp
    src/runtime/movement/MovementPolicy.cpp
    src/runtime/movement/MovementSystem.cpp
    src/runtime/ability/AbilitySystem.cpp
    src/runtime/projectile/ProjectileSystem.cpp
    src/runtime/targeting/TargetQuery.cpp
    src/runtime/targeting/ReachQuery.cpp
    src/runtime/inventory/InventorySystem.cpp
    src/runtime/objective/ObjectiveSystem.cpp
    src/runtime/objective/ObjectiveOutcome.cpp
    src/runtime/interaction/InteractionSystem.cpp
    src/runtime/physics/PhysicsAabbCollider.cpp
    src/runtime/physics/PhysicsCollisionQueries.cpp
    src/runtime/physics/PhysicsFrameStats.cpp
    src/runtime/physics/PhysicsKinematicMotor.cpp
    src/runtime/physics/PhysicsShapeStore.cpp
    src/runtime/physics/PhysicsSpatialSurfaceColliderBake.cpp
    src/runtime/physics/PhysicsTypes.cpp
    src/runtime/session/SessionTick.cpp
    src/runtime/session/ScenarioSeedConversion.cpp
    src/runtime/session/SessionRunner.cpp
    src/runtime/replay/StateHash.cpp
    src/runtime/save/SaveCompatibility.cpp
    src/runtime/save/SaveCodec.cpp
    src/runtime/save/SaveCodecDecode.cpp
    src/runtime/save/SaveCodecDecodeSections.cpp
    src/runtime/save/SaveFileStore.cpp
    src/runtime/save/SaveLoad.cpp
    src/projection/scene/SceneModel.cpp
    src/projection/scene/SceneProjection.cpp
    src/render/FrameInput.cpp
    src/render/RenderDiagnostics.cpp
    src/render/debug/DebugHudText.cpp
    src/render/mesh/BeanMesh.cpp
)
target_include_directories(iggy3d
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src"
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
iggy3d_apply_warnings(iggy3d)

if(NOT IGGY3D_HAS_SDL3_TARGET)
  message(FATAL_ERROR "iggy3d_creative requires SDL3")
endif()
if(NOT IGGY3D_HAS_VULKAN_TARGET)
  message(FATAL_ERROR "iggy3d_creative requires Vulkan")
endif()

target_sources(iggy3d
  PRIVATE
    src/app/platform/SdlWindow.cpp
    src/app/platform/SdlVulkanSurface.cpp
    src/render/vulkan/VulkanResult.cpp
    src/render/vulkan/VulkanFunctions.cpp
    src/render/vulkan/VulkanFeatureSupport.cpp
    src/render/vulkan/DebugValidation.cpp
    src/render/vulkan/InstanceDeviceSurface.cpp
    src/render/vulkan/Swapchain.cpp
    src/render/vulkan/FrameSync.cpp
    src/render/vulkan/CommandRecording.cpp
    src/render/vulkan/DearImGuiVulkanBridge.cpp
    src/render/vulkan/ProjectileOverlayProjection.cpp
    src/render/vulkan/RenderLoop.cpp
    src/render/vulkan/ShaderModule.cpp
    src/render/vulkan/PipelineLayout.cpp
    src/render/vulkan/FirstRoomPipeline.cpp
    src/render/vulkan/VulkanMemoryAllocator.cpp
    src/render/vulkan/StaticMeshMaterialTextures.cpp
    src/render/vulkan/StaticMeshInstances.cpp
    src/render/vulkan/BufferImageResources.cpp
    src/render/vulkan/FrameCapture.cpp
    src/render/vulkan/VulkanBackend.cpp)
target_link_libraries(iggy3d PUBLIC
  "${IGGY3D_SDL3_TARGET}"
  "${IGGY3D_VULKAN_TARGET}")
target_link_libraries(iggy3d PRIVATE iggy3d_imgui)
target_compile_definitions(iggy3d PUBLIC
  IGGY3D_HAS_SDL3=1
  IGGY3D_HAS_VULKAN=1
  IGGY3D_ENABLE_VULKAN=1
  IGGY3D_APP_VULKAN_BACKEND=1)
if(IGGY3D_ENABLE_VULKAN_SHADERS AND IGGY3D_SHADER_COMPILER_AVAILABLE)
  add_dependencies(iggy3d iggy3d_vulkan_shaders)
endif()

set(IGGY3D_CREATIVE_APP_SOURCES
  apps/iggy3d_creative/EditorActionHints.cpp
  apps/iggy3d_creative/EditorAssetLibrary.cpp
  apps/iggy3d_creative/EditorAssetLibraryFrame.cpp
  apps/iggy3d_creative/EditorAssetLibraryOverlay.cpp
  apps/iggy3d_creative/EditorAssetReplacement.cpp
  apps/iggy3d_creative/EditorAssetScatter.cpp
  apps/iggy3d_creative/EditorAuthoredAssets.cpp
  apps/iggy3d_creative/EditorAssets.cpp
  apps/iggy3d_creative/EditorToolCapabilities.cpp
  apps/iggy3d_creative/EditorLogicLinks.cpp
  apps/iggy3d_creative/EditorCatalog.cpp
  apps/iggy3d_creative/EditorToolWheel.cpp
  apps/iggy3d_creative/EditorCatalogLayout.cpp
  apps/iggy3d_creative/EditorCatalogOverlay.cpp
  apps/iggy3d_creative/EditorToolWheelPreferences.cpp
  apps/iggy3d_creative/EditorControls.cpp
  apps/iggy3d_creative/EditorControlsPersistence.cpp
  apps/iggy3d_creative/EditorControlsUi.cpp
  apps/iggy3d_creative/EditorToolOptions.cpp
  apps/iggy3d_creative/EditorToolOptionsPanel.cpp
  apps/iggy3d_creative/EditorObjectActions.cpp
  apps/iggy3d_creative/EditorCommandInput.cpp
  apps/iggy3d_creative/EditorTransform.cpp
  apps/iggy3d_creative/EditorTransformFrame.cpp
  apps/iggy3d_creative/EditorTransformOverlay.cpp
  apps/iggy3d_creative/EditorCapture.cpp
  apps/iggy3d_creative/EditorFrame.cpp
  apps/iggy3d_creative/EditorFrameSubmit.cpp
  apps/iggy3d_creative/EditorGamepad.cpp
  apps/iggy3d_creative/EditorGizmo.cpp
  apps/iggy3d_creative/EditorGroup.cpp
  apps/iggy3d_creative/EditorHeldItemCommands.cpp
  apps/iggy3d_creative/EditorHeldItemDispatch.cpp
  apps/iggy3d_creative/EditorHeldItemWorldOperations.cpp
  apps/iggy3d_creative/EditorInteraction.cpp
  apps/iggy3d_creative/EditorInteractionOverlay.cpp
  apps/iggy3d_creative/EditorWorldTargeting.cpp
  apps/iggy3d_creative/EditorWorldLayout.cpp
  apps/iggy3d_creative/EditorWorldLayoutElevationProjection.cpp
  apps/iggy3d_creative/EditorWorldLayoutElevationEditing.cpp
  apps/iggy3d_creative/EditorWorldLayoutElevationPanel.cpp
  apps/iggy3d_creative/EditorWorldLayoutDiagnostics.cpp
  apps/iggy3d_creative/EditorWorldLayoutLevels.cpp
  apps/iggy3d_creative/EditorWorldLayoutHistory.cpp
  apps/iggy3d_creative/EditorWorldLayoutShells.cpp
  apps/iggy3d_creative/EditorWorldLayoutStructures.cpp
  apps/iggy3d_creative/EditorWorldLayoutVerticalConnectors.cpp
  apps/iggy3d_creative/EditorWorldLayoutGroups.cpp
  apps/iggy3d_creative/EditorWorldLayoutTemplates.cpp
  apps/iggy3d_creative/EditorConnectedFill.cpp
  apps/iggy3d_creative/EditorSurfaceExtrude.cpp
  apps/iggy3d_creative/EditorTerrain.cpp
  apps/iggy3d_creative/EditorTerrainOverlay.cpp
  apps/iggy3d_creative/EditorTerrainGrade.cpp
  apps/iggy3d_creative/EditorTerrainPath.cpp
  apps/iggy3d_creative/EditorTerrainPaint.cpp
  apps/iggy3d_creative/EditorTerrainProfile.cpp
  apps/iggy3d_creative/EditorTerrainRegion.cpp
  apps/iggy3d_creative/EditorTerrainSculpt.cpp
  apps/iggy3d_creative/EditorTerrainStamp.cpp
  apps/iggy3d_creative/EditorMaterialStroke.cpp
  apps/iggy3d_creative/EditorStructuralPlacement.cpp
  apps/iggy3d_creative/EditorPattern.cpp
  apps/iggy3d_creative/EditorBootstrap.cpp
  apps/iggy3d_creative/EditorPlacement.cpp
  apps/iggy3d_creative/EditorAttachmentPlacement.cpp
  apps/iggy3d_creative/EditorPlacementClearance.cpp
  apps/iggy3d_creative/EditorPlacementFeedback.cpp
  apps/iggy3d_creative/EditorRoomPlacement.cpp
  apps/iggy3d_creative/EditorPlacementMutation.cpp
  apps/iggy3d_creative/EditorPlacementPreview.cpp
  apps/iggy3d_creative/EditorPlacementGridOverlay.cpp
  apps/iggy3d_creative/EditorEdits.cpp
  apps/iggy3d_creative/EditorVolume.cpp
  apps/iggy3d_creative/EditorFrustumCull.cpp
  apps/iggy3d_creative/EditorPathEditing.cpp
  apps/iggy3d_creative/EditorMovingPlatformPreview.cpp
  apps/iggy3d_creative/EditorPersistence.cpp
  apps/iggy3d_creative/EditorPlayMode.cpp
  apps/iggy3d_creative/EditorPlayLogicOverlay.cpp
  apps/iggy3d_creative/EditorPlayInteraction.cpp
  apps/iggy3d_creative/EditorPickFrame.cpp
  apps/iggy3d_creative/EditorPicking.cpp
  apps/iggy3d_creative/EditorOverlayHud.cpp
  apps/iggy3d_creative/EditorOverlayWireframes.cpp
  apps/iggy3d_creative/EditorBrushWireframes.cpp
  apps/iggy3d_creative/EditorVolumeWireframes.cpp
  apps/iggy3d_creative/EditorPreviewProxies.cpp
  apps/iggy3d_creative/EditorScenePreview.cpp
  apps/iggy3d_creative/EditorShapePreview.cpp
  apps/iggy3d_creative/EditorPreviewFrame.cpp
  apps/iggy3d_creative/EditorDesktopUi.cpp
  apps/iggy3d_creative/EditorDesktopCommands.cpp
  apps/iggy3d_creative/EditorDesktopModel.cpp
  apps/iggy3d_creative/EditorDesktopOutliner.cpp
  apps/iggy3d_creative/EditorDesktopInspector.cpp
  apps/iggy3d_creative/EditorDesktopWorldLayoutInspector.cpp
  apps/iggy3d_creative/EditorDesktopWorldLayoutVerticalConnectorInspector.cpp
  apps/iggy3d_creative/EditorDesktopPanels.cpp
  apps/iggy3d_creative/EditorWorldLayoutPanel.cpp)

add_library(iggy3d_creative_app STATIC ${IGGY3D_CREATIVE_APP_SOURCES})
target_link_libraries(iggy3d_creative_app PUBLIC iggy3d)
# PRIVATE on both targets: only the DL-2 allowlist TUs (the bridge inside
# iggy3d, the EditorDesktop*.cpp files here) may include imgui headers; no
# PUBLIC header anywhere exposes them, so tests never see ImGui types.
target_link_libraries(iggy3d_creative_app PRIVATE iggy3d_imgui)
target_include_directories(iggy3d_creative_app PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/apps/iggy3d_creative")
target_compile_definitions(iggy3d_creative_app PRIVATE
  IGGY3D_CREATIVE_ASSET_ROOT_VALUE="${CMAKE_CURRENT_SOURCE_DIR}/assets/creative")
iggy3d_apply_warnings(iggy3d_creative_app)

add_executable(i3dc apps/iggy3d_creative/main.cpp)
target_link_libraries(i3dc PRIVATE iggy3d_creative_app)
iggy3d_apply_warnings(i3dc)

# Preserve the descriptive build target while keeping the executable convenient.
add_custom_target(iggy3d_creative DEPENDS i3dc)

if(IGGY3D_BUILD_TESTS)
  enable_testing()
  include(cmake/iggy3d_tests.cmake)
endif()
