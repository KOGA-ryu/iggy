cmake_minimum_required(VERSION 3.20)

project(iggy3d VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/iggy3d_options.cmake)
include(cmake/iggy3d_warnings.cmake)
include(cmake/iggy3d_vulkan_deps.cmake)
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
    src/core/geom/StairMesh.cpp
    src/core/spatial/AabbGridIndex.cpp
    src/core/math/Vec3.cpp
    src/core/math/EulerRotation.cpp
    src/core/math/Transform3.cpp
    src/core/math/Aabb3.cpp
    src/core/math/OrientedBox.cpp
    src/core/math/Snap.cpp
    src/core/math/Frustum.cpp
    src/core/math/Mat4.cpp
    src/app/AppConfig.cpp
    src/app/CliParser.cpp
    src/app/PackageRuntimeLookup.cpp
    src/app/iggy3d/AppShell.cpp
    src/app/iggy3d/AppKernel.cpp
    src/app/iggy3d/ascii_room/AsciiRoomGrid.cpp
    src/app/iggy3d/ascii_room/AsciiRoomSource.cpp
    src/app/iggy3d/ascii_room/AsciiRoomCanvas.cpp
    src/app/iggy3d/ascii_room/AsciiRoomToAuthoredRoom.cpp
    src/app/iggy3d/ascii_room/AsciiRoomAssetText.cpp
    src/app/iggy3d/ascii_room/AsciiRoomToEditableRoom.cpp
    src/app/iggy3d/ascii_room/AsciiRoomToRoomAsset.cpp
    src/app/iggy3d/world/DefaultWorldTemplate.cpp
    src/app/iggy3d/world/ProductWorldTemplateOperations.cpp
    src/app/iggy3d/world/ProductSessionLaunch.cpp
    src/app/iggy3d/world/ProductNewWorldLaunch.cpp
    src/app/iggy3d/world/ProductLaunchState.cpp
    src/app/iggy3d/room_editor/EditableRoomToAuthoredRoom.cpp
    src/app/iggy3d/creative/document/Object.cpp
    src/app/iggy3d/creative/document/Document.cpp
    src/app/iggy3d/creative/mutation/Mutation.cpp
    src/app/iggy3d/creative/document/ObjectDescriptor.cpp
    src/app/iggy3d/creative/mutation/MutationApply.cpp
    src/app/iggy3d/creative/document/DocumentMutation.cpp
    src/app/iggy3d/creative/spatial/SpatialProjection.cpp
    src/app/iggy3d/creative/document/DocumentWireframe.cpp
    src/app/iggy3d/creative/spatial/ViewportPick.cpp
    src/app/iggy3d/creative/tools/Tools.cpp
    src/app/iggy3d/creative/tools/Select.cpp
    src/app/iggy3d/creative/tools/Measure.cpp
    src/app/iggy3d/creative/spatial/Snap.cpp
    src/app/iggy3d/creative/document/DocumentSnap.cpp
    src/app/iggy3d/creative/adapters/RoomBake.cpp
    src/app/iggy3d/creative/adapters/RoomBakeGreedyFloors.cpp
    src/app/iggy3d/creative/adapters/RoomBakeReachability.cpp
    src/app/iggy3d/creative/spatial/Ghost.cpp
    src/app/iggy3d/creative/tools/Placement.cpp
    src/app/iggy3d/creative/tools/RoomShell.cpp
    src/app/iggy3d/creative/BakedActiveRoomRefresh.cpp
    src/app/iggy3d/creative/CreativeWorldOperations.cpp
    src/app/iggy3d/creative/CreativeBlankStageSession.cpp
    src/app/iggy3d/creative/ui/Ui.cpp
    src/app/iggy3d/creative/mutation/Metrics.cpp
    src/app/iggy3d/creative/Facade.cpp
    src/app/iggy3d/creative/tools/Palette.cpp
    src/app/iggy3d/creative/document/Block.cpp
    src/app/iggy3d/view/OpeningMenuHitTest.cpp
    src/app/iggy3d/view/OpeningMenuView.cpp
    src/app/iggy3d/view/MenuPanelsView.cpp
    src/app/iggy3d/view/DebugHudView.cpp
    src/app/iggy3d/view/ScenePrimitiveView.cpp
    src/app/iggy3d/view/SdlDraw.cpp
    src/app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.cpp
    src/app/iggy3d/gameplay/ActiveRoomCollision.cpp
    src/app/iggy3d/gameplay/ActiveRoomState.cpp
    src/app/iggy3d/gameplay/ProductRoomStore.cpp
    src/app/iggy3d/world/BuiltinDungeon.cpp
    src/app/iggy3d/world/MovementTestLab.cpp
    src/app/iggy3d/view/CameraController.cpp
    src/app/iggy3d/view/CreativeFlyAnchorStore.cpp
    src/app/iggy3d/world/DungeonDraft.cpp
    src/app/iggy3d/ascii_room/Activation.cpp
    src/app/iggy3d/ascii_room/Authoring.cpp
    src/app/iggy3d/ascii_room/Editing.cpp
    src/app/iggy3d/ascii_room/Package.cpp
    src/app/iggy3d/ascii_room/Preview.cpp
    src/app/iggy3d/gameplay/Controller.cpp
    src/app/iggy3d/gameplay/ControllerGroundQueries.cpp
    src/app/iggy3d/gameplay/ControllerJumpDashState.cpp
    src/app/iggy3d/gameplay/ControllerKinematics.cpp
    src/app/iggy3d/gameplay/ControllerMovementProof.cpp
    src/app/iggy3d/gameplay/ControllerWallRunEvaluation.cpp
    src/app/iggy3d/gameplay/ControllerWallQueries.cpp
    src/app/iggy3d/gameplay/GameplayFeedback.cpp
    src/app/iggy3d/gameplay/MovementProof.cpp
    src/app/iggy3d/gameplay/ProjectionRefresh.cpp
    src/app/iggy3d/menu/FrontendRouter.cpp
    src/app/iggy3d/gameplay/Tape.cpp
    src/app/iggy3d/gameplay/TapeRunner.cpp
    src/app/iggy3d/input/ControllerActionMap.cpp
    src/app/iggy3d/input/ControllerActionRouting.cpp
    src/app/iggy3d/input/InteractionMode.cpp
    src/app/iggy3d/debug/InteractionModeHud.cpp
    src/app/iggy3d/input/InteractionModeState.cpp
    src/app/iggy3d/creative/camera/Fly.cpp
    src/app/iggy3d/map_maker/Grid.cpp
    src/app/iggy3d/map_maker/Presentation.cpp
    src/app/iggy3d/menu/Transitions.cpp
    src/app/iggy3d/window/MouseCapturePolicy.cpp
    src/app/iggy3d/debug/MovementDebugHud.cpp
    src/app/iggy3d/debug/NpcBehaviorDebugHud.cpp
    src/app/iggy3d/debug/PhysicsDebugHud.cpp
    src/app/iggy3d/debug/PositionHud.cpp
    src/app/iggy3d/world/PackageSessionSeed.cpp
    src/app/iggy3d/CreativeReasoningActivation.cpp
    src/app/iggy3d/PatrolRouteWaypoints.cpp
    src/app/iggy3d/Options.cpp
    src/app/iggy3d/automation/Automation.cpp
    src/app/iggy3d/automation/AutomationControl.cpp
    src/app/iggy3d/automation/AutomationDispatch.cpp
    src/app/iggy3d/automation/AutomationGameplay.cpp
    src/app/iggy3d/automation/AutomationRoomEditing.cpp
    src/app/iggy3d/automation/AutomationSaveBrowser.cpp
    src/app/iggy3d/automation/AutomationSystem.cpp
    src/app/iggy3d/menu/ActionHandlers.cpp
    src/app/iggy3d/menu/InputRouter.cpp
    src/app/iggy3d/creative/render/WireframeDebugLines.cpp
    src/app/iggy3d/view/PrimitiveDrawList.cpp
    src/app/iggy3d/view/RenderBridge.cpp
    src/app/iggy3d/room/GeometryOptimization.cpp
    src/app/iggy3d/room_editor/AuthoringController.cpp
    src/app/iggy3d/room_editor/ActionController.cpp
    src/app/iggy3d/room_editor/Cursor.cpp
    src/app/iggy3d/room_editor/Presentation.cpp
    src/app/iggy3d/room_editor/Preview.cpp
    src/app/iggy3d/room_editor/EditingState.cpp
    src/app/iggy3d/gameplay/ScriptedDriver.cpp
    src/app/iggy3d/creative/world/DocumentSection.cpp
    src/app/iggy3d/save/RoomMarkerBinding.cpp
    src/app/iggy3d/save/Catalog.cpp
    src/app/iggy3d/save/CatalogProjector.cpp
    src/app/iggy3d/save/Flow.cpp
    src/app/iggy3d/save/CurrentSessionSave.cpp
    src/app/iggy3d/save/SaveSlotOperations.cpp
    src/app/iggy3d/world/NpcProfileAssignment.cpp
    src/app/iggy3d/debug/TopDownMapOverlay.cpp
    src/app/iggy3d/menu/DrawList.cpp
    src/app/iggy3d/creative/ui/UiDrawList.cpp
    src/app/iggy3d/creative/ui/UiProjection.cpp
    src/app/iggy3d/creative/ui/UiFrame.cpp
    src/app/iggy3d/menu/UiHitRouter.cpp
    src/app/iggy3d/menu/Notebook.cpp
    src/app/iggy3d/menu/PauseUi.cpp
    src/app/iggy3d/ui/Widget.cpp
    src/app/iggy3d/view/ViewportFraming.cpp
    src/app/iggy3d/window/FramePresenter.cpp
    src/app/iggy3d/creative/bridge/WindowCoordinateSpace.cpp
    src/app/iggy3d/creative/bridge/UiWindowFrame.cpp
    src/app/iggy3d/creative/bridge/UiInputFrame.cpp
    src/app/iggy3d/creative/bridge/UiCommandFrame.cpp
    src/app/iggy3d/creative/bridge/ViewportPickFrame.cpp
    src/app/iggy3d/creative/bridge/WireframeFrame.cpp
    src/app/iggy3d/creative/bridge/InputFrame.cpp
    src/app/iggy3d/window/InputFrame.cpp
    src/app/iggy3d/window/Loop.cpp
    src/app/iggy3d/window/RendererLifecycle.cpp
    src/app/iggy3d/world/Creation.cpp
    src/app/iggy3d/creative/world/WorldService.cpp
    src/app/iggy3d/ReceiptBuilder.cpp
    src/app/iggy3d/receipt/ReceiptFields.cpp
    src/app/iggy3d/receipt/PhysicsReceiptRecording.cpp
    src/app/iggy3d/receipt/CreativeReceiptRecording.cpp
    src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp
    src/app/iggy3d/receipt/StartupProbeFields.cpp
    src/app/iggy3d/receipt/WorldAuthoringFields.cpp
    src/app/iggy3d/receipt/ActiveRoomFields.cpp
    src/app/iggy3d/receipt/StartupWorldBuildoutFields.cpp
    src/app/iggy3d/receipt/SaveStateFields.cpp
    src/app/iggy3d/receipt/GameplayRuntimeMovementFields.cpp
    src/app/iggy3d/receipt/DebugHudFields.cpp
    src/app/iggy3d/receipt/GameplaySceneStateFields.cpp
    src/app/iggy3d/receipt/FeedbackSurfaceAutomationVulkanFields.cpp
    src/app/iggy3d/receipt/CreativeUiFields.cpp
    src/app/iggy3d/receipt/CreativePickWireframeFields.cpp
    src/app/iggy3d/receipt/TailFields.cpp
    src/app/iggy3d/save/SaveBridge.cpp
    src/app/frontend/FrontendState.cpp
    src/app/frontend/FrontendRoute.cpp
    src/app/frontend/MenuInput.cpp
    src/app/frontend/PauseMenu.cpp
    src/app/frontend/SaveBrowser.cpp
    src/app/frontend/StarterScreen.cpp
    src/app/frontend/WorldSetupModel.cpp
    src/app/frontend/SaveSlotModel.cpp
    src/app/frontend/SettingsMenu.cpp
    src/app/frontend/DevToolsMenu.cpp
    src/app/input/GamepadInput.cpp
    src/app/input/ActionState.cpp
    src/app/input/InputAction.cpp
    src/app/input/InputActionRegistry.cpp
    src/app/input/InputBindings.cpp
    src/app/input/InputDeviceEvent.cpp
    src/app/input/InputRouter.cpp
    src/app/input/KeyboardInput.cpp
    src/app/input/MouseInput.cpp
    src/app/platform/ExecutablePath.cpp
    src/content/PackageLoader.cpp
    src/content/PackageValidator.cpp
    src/content/authoring/EditableRoomDocument.cpp
    src/content/assets/MaterialAsset.cpp
    src/content/assets/MeshAsset.cpp
    src/content/assets/TraversalTag.cpp
    src/content/assets/RoomAsset.cpp
    src/content/FixtureScenarioLoader.cpp
    src/runtime/player/PlayerRoster.cpp
    src/runtime/player/PlayerMotor.cpp
    src/runtime/player/PlayerPhysicsMovePlanner.cpp
    src/runtime/world/WorldState.cpp
    src/runtime/clock/Clock.cpp
    src/runtime/camera/CameraModePolicy.cpp
    src/runtime/ai/NpcBehaviorSystem.cpp
    src/runtime/ai/NpcBehaviorProfile.cpp
    src/runtime/ai/NpcBehaviorDebugSnapshot.cpp
    src/runtime/ai/NpcAlertSystem.cpp
    src/runtime/ai/NpcPatrolSystem.cpp
    src/runtime/ai/NpcInvestigateSystem.cpp
    src/runtime/ai/NpcSoundPerception.cpp
    src/runtime/ai/ReasoningGraph.cpp
    src/runtime/ai/GuardRecon.cpp
    src/runtime/ai/ReconIntel.cpp
    src/runtime/ai/ReasoningRoute.cpp
    src/runtime/ai/GuardDecision.cpp
    src/runtime/combat/CombatSystem.cpp
    src/runtime/collision/SpatialSurfaceSet.cpp
    src/runtime/collision/CollisionQuery.cpp
    src/runtime/collision/EntityHitQuery.cpp
    src/runtime/debug/RuntimeDebugSnapshot.cpp
    src/runtime/command/CommandAdmission.cpp
    src/runtime/replay/CommandLog.cpp
    src/runtime/session/Session.cpp
    src/runtime/movement/MovementKinematics.cpp
    src/runtime/movement/MovementPolicy.cpp
    src/runtime/movement/MovementSystem.cpp
    src/runtime/movement/MovementTraversalSlots.cpp
    src/runtime/movement/MovementTraversal.cpp
    src/runtime/ability/AbilitySystem.cpp
    src/runtime/projectile/ProjectileSystem.cpp
    src/runtime/targeting/TargetQuery.cpp
    src/runtime/targeting/ReachQuery.cpp
    src/runtime/inventory/InventorySystem.cpp
    src/runtime/objective/ObjectiveSystem.cpp
    src/runtime/objective/ObjectiveOutcome.cpp
    src/runtime/interaction/InteractionSystem.cpp
    src/runtime/object/ObjectTraits.cpp
    src/runtime/physics/PhysicsAabbCollider.cpp
    src/runtime/physics/PhysicsAabbContact.cpp
    src/runtime/physics/PhysicsAabbContactSolver.cpp
    src/runtime/physics/PhysicsBodyStore.cpp
    src/runtime/physics/PhysicsBroadphase.cpp
    src/runtime/physics/PhysicsCollisionQueries.cpp
    src/runtime/physics/PhysicsDebugSnapshot.cpp
    src/runtime/physics/PhysicsFrameStats.cpp
    src/runtime/physics/PhysicsKinematicMotor.cpp
    src/runtime/physics/PhysicsKernelBenchmark.cpp
    src/runtime/physics/PhysicsKernelBenchmarkJson.cpp
    src/runtime/physics/PhysicsMaterialTraits.cpp
    src/runtime/physics/PhysicsShapeStore.cpp
    src/runtime/physics/PhysicsSpatialSurfaceColliderBake.cpp
    src/runtime/physics/PhysicsTypes.cpp
    src/runtime/session/SessionTick.cpp
    src/runtime/session/SessionRunner.cpp
    src/runtime/replay/StateHash.cpp
    src/runtime/replay/CommandReplay.cpp
    src/runtime/save/SaveCompatibility.cpp
    src/runtime/save/SaveCodec.cpp
    src/runtime/save/SaveFileStore.cpp
    src/runtime/save/SaveLoad.cpp
    src/runtime/diagnostics/RuntimeSummary.cpp
    src/projection/scene/SceneProjection.cpp
    src/projection/debug/DebugProjection.cpp
    src/render/RendererApi.cpp
    src/render/FrameInput.cpp
    src/render/RendererConfig.cpp
    src/render/RenderDiagnostics.cpp
    src/render/debug/DebugHudText.cpp
    src/render/mesh/BeanMesh.cpp
    src/render/null/NullRenderer.cpp
)
target_include_directories(iggy3d PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
iggy3d_apply_warnings(iggy3d)

if(IGGY3D_HAS_SDL3_TARGET)
  target_sources(iggy3d PRIVATE src/app/platform/SdlWindow.cpp)
  target_link_libraries(iggy3d PUBLIC "${IGGY3D_SDL3_TARGET}")
  target_compile_definitions(iggy3d PUBLIC IGGY3D_HAS_SDL3=1)
endif()

if(IGGY3D_HAS_SDL3_TARGET AND IGGY3D_HAS_VULKAN_TARGET)
  target_sources(iggy3d PRIVATE src/app/platform/SdlVulkanSurface.cpp)
endif()

set(IGGY3D_BUILD_VULKAN_BACKEND OFF)
if(IGGY3D_HAS_VULKAN_TARGET AND (IGGY3D_BUILD_TOOLS OR IGGY3D_ENABLE_VULKAN))
  set(IGGY3D_BUILD_VULKAN_BACKEND ON)
endif()

if(IGGY3D_HAS_VULKAN_TARGET AND
   (IGGY3D_BUILD_VULKAN_BACKEND OR IGGY3D_ENABLE_VULKAN_SMOKE))
  target_link_libraries(iggy3d PUBLIC "${IGGY3D_VULKAN_TARGET}")
  target_compile_definitions(iggy3d PUBLIC IGGY3D_HAS_VULKAN=1)
endif()

if(IGGY3D_BUILD_VULKAN_BACKEND)
  target_sources(iggy3d
    PRIVATE
      src/render/vulkan/VulkanResult.cpp
      src/render/vulkan/VulkanFunctions.cpp
      src/render/vulkan/VulkanFeatureSupport.cpp
      src/render/vulkan/DebugValidation.cpp
      src/render/vulkan/InstanceDeviceSurface.cpp
      src/render/vulkan/Swapchain.cpp
      src/render/vulkan/FrameSync.cpp
      src/render/vulkan/CommandRecording.cpp
      src/render/vulkan/ProjectileOverlayProjection.cpp
      src/render/vulkan/RenderLoop.cpp
      src/render/vulkan/ShaderModule.cpp
      src/render/vulkan/PipelineLayout.cpp
      src/render/vulkan/FirstRoomPipeline.cpp
      src/render/vulkan/VulkanMemoryAllocator.cpp
      src/render/vulkan/BufferImageResources.cpp
      src/render/vulkan/DescriptorSets.cpp
      src/render/vulkan/FrameCapture.cpp
      src/render/vulkan/VulkanBackend.cpp
  )
  if(IGGY3D_ENABLE_VULKAN)
    target_compile_definitions(iggy3d PUBLIC IGGY3D_ENABLE_VULKAN=1)
  endif()
  if(IGGY3D_ENABLE_VULKAN_SHADERS AND IGGY3D_SHADER_COMPILER_AVAILABLE)
    add_dependencies(iggy3d iggy3d_vulkan_shaders)
  endif()
  if(IGGY3D_HAS_SDL3_TARGET)
    target_compile_definitions(iggy3d PUBLIC IGGY3D_APP_VULKAN_BACKEND=1)
  endif()
elseif(IGGY3D_HAS_VULKAN_TARGET)
  target_compile_definitions(iggy3d PUBLIC IGGY3D_HAS_VULKAN_TARGET_AVAILABLE=1)
endif()

add_executable(iggy3d_app apps/iggy3d/main.cpp)
target_link_libraries(iggy3d_app PRIVATE iggy3d)
set_target_properties(iggy3d_app PROPERTIES OUTPUT_NAME iggy3d)
iggy3d_apply_warnings(iggy3d_app)

add_executable(iggy3d_creative
  apps/iggy3d_creative/main.cpp
  apps/iggy3d_creative/CreativeRendererBootstrap.cpp
  apps/iggy3d_creative/StandaloneBrushPalette.cpp
  apps/iggy3d_creative/StandaloneCaptureScenario.cpp
  apps/iggy3d_creative/StandaloneFrustumCull.cpp
  apps/iggy3d_creative/StandaloneGizmo.cpp
  apps/iggy3d_creative/StandalonePathEditing.cpp
  apps/iggy3d_creative/StandalonePersistenceProof.cpp
  apps/iggy3d_creative/StandalonePicking.cpp
  apps/iggy3d_creative/StandalonePlacement.cpp
  apps/iggy3d_creative/StandalonePreviewProxies.cpp
  apps/iggy3d_creative/StandaloneRoomBakePreview.cpp)
target_link_libraries(iggy3d_creative PRIVATE iggy3d)
iggy3d_apply_warnings(iggy3d_creative)

if(IGGY3D_BUILD_TOOLS)
  add_executable(iggy3d_headless_demo apps/iggy3d_headless_demo/main.cpp)
  target_link_libraries(iggy3d_headless_demo PRIVATE iggy3d)
  iggy3d_apply_warnings(iggy3d_headless_demo)

  add_executable(iggy3d_replay_tool apps/iggy3d_replay_tool/main.cpp)
  target_link_libraries(iggy3d_replay_tool PRIVATE iggy3d)
  iggy3d_apply_warnings(iggy3d_replay_tool)

  add_executable(iggy3d_collision_probe apps/iggy3d_collision_probe/main.cpp)
  target_link_libraries(iggy3d_collision_probe PRIVATE iggy3d)
  iggy3d_apply_warnings(iggy3d_collision_probe)

  add_executable(iggy3d_physics_kernel_bench
    apps/iggy3d_physics_kernel_bench/main.cpp)
  target_link_libraries(iggy3d_physics_kernel_bench PRIVATE iggy3d)
  iggy3d_apply_warnings(iggy3d_physics_kernel_bench)

  add_executable(iggy3d_product_frame_metrics
    tools/iggy3d_product_frame_metrics/main.cpp)
  target_link_libraries(iggy3d_product_frame_metrics PRIVATE iggy3d)
  iggy3d_apply_warnings(iggy3d_product_frame_metrics)
endif()

include(cmake/iggy3d_install.cmake)

if(IGGY3D_BUILD_TESTS)
  enable_testing()
  include(cmake/iggy3d_tests.cmake)
endif()
