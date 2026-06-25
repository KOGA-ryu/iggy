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
target_sources(iggy3d
  PRIVATE
    src/core/hash/StableHash.cpp
    src/core/math/Vec3.cpp
    src/core/math/Transform3.cpp
    src/core/math/Aabb3.cpp
    src/core/math/Ray3.cpp
    src/core/math/Plane.cpp
    src/core/math/Mat4.cpp
    src/app/AppConfig.cpp
    src/app/CliParser.cpp
    src/app/PackageRuntimeLookup.cpp
    src/app/iggy3d/AppShell.cpp
    src/app/iggy3d/AsciiRoomGrid.cpp
    src/app/iggy3d/AsciiRoomSource.cpp
    src/app/iggy3d/AsciiRoomToAuthoredRoom.cpp
    src/app/iggy3d/AsciiRoomAssetText.cpp
    src/app/iggy3d/AsciiRoomToRoomAsset.cpp
    src/app/iggy3d/DefaultWorldTemplate.cpp
    src/app/iggy3d/FrontendActionExecutor.cpp
    src/app/iggy3d/OpeningMenuView.cpp
    src/app/iggy3d/ProductActiveRoomState.cpp
    src/app/iggy3d/ProductCameraController.cpp
    src/app/iggy3d/ProductAsciiRoomActivation.cpp
    src/app/iggy3d/ProductAsciiRoomAuthoring.cpp
    src/app/iggy3d/ProductAsciiRoomPreview.cpp
    src/app/iggy3d/ProductGameplayController.cpp
    src/app/iggy3d/ProductGameplayFeedback.cpp
    src/app/iggy3d/ProductFrontendRouter.cpp
    src/app/iggy3d/ProductMenuTransitions.cpp
    src/app/iggy3d/ProductPackageSessionSeed.cpp
    src/app/iggy3d/ProductAppOperations.cpp
    src/app/iggy3d/ProductAppOptions.cpp
    src/app/iggy3d/ProductPrimitiveDrawList.cpp
    src/app/iggy3d/ProductRenderBridge.cpp
    src/app/iggy3d/ProductScriptedGameplayDriver.cpp
    src/app/iggy3d/ProductSaveCatalog.cpp
    src/app/iggy3d/ProductViewportFraming.cpp
    src/app/iggy3d/ProductWorldCreation.cpp
    src/app/iggy3d/ReceiptBuilder.cpp
    src/app/iggy3d/SaveBridge.cpp
    src/app/frontend/FrontendState.cpp
    src/app/frontend/FrontendRoute.cpp
    src/app/frontend/MenuInput.cpp
    src/app/frontend/ConfirmDialog.cpp
    src/app/frontend/PauseMenu.cpp
    src/app/frontend/SaveBrowser.cpp
    src/app/frontend/StarterScreen.cpp
    src/app/frontend/VerticalFadedSelector.cpp
    src/app/frontend/WorldSetupModel.cpp
    src/app/frontend/SaveSlotModel.cpp
    src/app/frontend/SettingsMenu.cpp
    src/app/frontend/DevToolsMenu.cpp
    src/app/frontend/FrontendReceipt.cpp
    src/app/input/AutomationInput.cpp
    src/app/input/GamepadInput.cpp
    src/app/input/ActionState.cpp
    src/app/input/GamepadSystemControls.cpp
    src/app/input/InputAction.cpp
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
    src/content/assets/RoomAsset.cpp
    src/content/FixtureScenarioLoader.cpp
    src/runtime/player/PlayerRoster.cpp
    src/runtime/player/PlayerMotor.cpp
    src/runtime/world/WorldState.cpp
    src/runtime/clock/Clock.cpp
    src/runtime/camera/CameraModePolicy.cpp
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
    src/runtime/interaction/InteractionSystem.cpp
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
elseif(IGGY3D_HAS_VULKAN_TARGET)
  target_compile_definitions(iggy3d PUBLIC IGGY3D_HAS_VULKAN_TARGET_AVAILABLE=1)
endif()

add_executable(iggy3d_app apps/iggy3d/main.cpp)
target_link_libraries(iggy3d_app PRIVATE iggy3d)
set_target_properties(iggy3d_app PROPERTIES OUTPUT_NAME iggy3d)
iggy3d_apply_warnings(iggy3d_app)

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
endif()

if(IGGY3D_BUILD_TOOLS OR IGGY3D_ENABLE_VISUAL_DEMO)
  add_executable(iggy3d_visual_demo apps/iggy3d_visual_demo/main.cpp)
  target_link_libraries(iggy3d_visual_demo PRIVATE iggy3d)
  target_compile_definitions(iggy3d_visual_demo
    PRIVATE
      IGGY3D_VULKAN_ICD_FOUND_VALUE=$<BOOL:${IGGY3D_VULKAN_ICD_FOUND}>)
  if(IGGY3D_BUILD_VULKAN_BACKEND AND IGGY3D_HAS_SDL3_TARGET)
    target_compile_definitions(iggy3d_visual_demo PRIVATE IGGY3D_APP_VULKAN_BACKEND=1)
  endif()
  iggy3d_apply_warnings(iggy3d_visual_demo)
endif()

include(cmake/iggy3d_install.cmake)

if(IGGY3D_BUILD_TESTS)
  enable_testing()
  include(cmake/iggy3d_tests.cmake)
endif()
