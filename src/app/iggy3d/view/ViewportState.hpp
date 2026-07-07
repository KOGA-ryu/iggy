#pragma once

#include <string>
#include <cstdint>

#include "app/iggy3d/view/CreativeFlyAnchorStore.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d {

struct ProductViewportState {
  bool gameplayViewVisible = false;
  std::string cameraMode = "first_person";
  std::string cameraController = "product_camera";
  bool cameraControllerActive = false;
  bool lookInputUsed = false;
  std::string cameraInputSource = "none";
  float cameraYawDegrees = 0.0F;
  float cameraPitchDegrees = 0.0F;
  bool cameraHeadingVisible = false;
  std::string mapMakerStatus = "map_maker_inactive";
  std::string mapMakerReasonCode = "map_maker_inactive";
  bool mapMakerGridVisible = false;
  std::string mapMakerGridStatus = "map_maker_grid_disabled";
  std::string mapMakerGridReasonCode = "map_maker_grid_disabled";
  float mapMakerGridPitchMeters = 1.0F;
  float mapMakerGridMajorStepMeters = 5.0F;
  float mapMakerGridPlaneY = 0.0F;
  std::uint64_t mapMakerGridLayerCount = 0;
  std::uint64_t mapMakerGridDotCount = 0;
  std::uint64_t mapMakerGridMajorDotCount = 0;
  ProductCreativeFlyAnchorStore creativeFlyAnchor;
  bool creativeFlyActive = false;
  float creativeFlySpeedMetersPerSecond = 0.0F;
  std::string creativeFlyStatus = "creative_fly_not_requested";
  std::string creativeFlyReasonCode = "creative_fly_not_requested";
  bool productDrawGridVisible = false;
  bool productDrawPlayerVisible = false;
  bool productDrawRoomVisible = false;
  bool productDrawObjectiveVisible = false;
  bool productDrawTargetIndicatorVisible = false;
  bool productDrawDoorVisible = false;
  bool productDrawOpenDoorVisible = false;
  bool productDrawClosedDoorVisible = false;
  std::uint64_t productDrawItemCount = 0;
  std::uint64_t productDrawDebugMarkerCount = 0;
  std::uint64_t productDrawDoorCount = 0;
  std::uint64_t productDrawOpenDoorCount = 0;
  std::uint64_t productDrawClosedDoorCount = 0;
  std::uint64_t productDrawRoomGeometryCount = 0;
  std::uint64_t productDrawFloorTileCount = 0;
  std::uint64_t productDrawElevatedFloorTileCount = 0;
  std::uint64_t productDrawRampTileCount = 0;
  std::uint64_t productDrawBlockedSlopeTileCount = 0;
  std::uint64_t productDrawWallTileCount = 0;
  bool productDrawPropVisible = false;
  std::uint64_t productDrawPropTileCount = 0;
  bool productDrawRoomEditorCursorVisible = false;
  std::uint64_t productDrawRoomEditorCursorCount = 0;
  bool productDrawRoomEditorPreviewVisible = false;
  std::uint64_t productDrawRoomEditorPreviewCount = 0;
  bool productDrawPhysicsDebugVisible = false;
  std::uint64_t productDrawPhysicsDebugItemCount = 0;
  std::uint64_t productDrawPhysicsAabbDebugCount = 0;
  std::uint64_t productDrawPhysicsContactNormalDebugCount = 0;
  bool productDrawMapMakerGridVisible = false;
  std::uint64_t productDrawMapMakerGridDotCount = 0;
  std::uint64_t productDrawMapMakerMajorGridDotCount = 0;
  bool productDrawMapMakerCubePreviewVisible = false;
  std::uint64_t productDrawMapMakerCubePreviewCount = 0;
  std::string productViewProjection = "primitive_first_person";
  bool productViewYawApplied = false;
  bool productViewPitchApplied = false;
  bool productViewPlayerAnchorFound = false;
  bool productRenderBridgeReady = false;
  bool productViewFrameReady = false;
  std::uint64_t productViewFrameItemCount = 0;
  std::uint64_t productViewFrameOnScreenItemCount = 0;
  std::uint64_t productViewFrameTargetItemCount = 0;
  bool productRenderBridgeRoomEditorCursorVisible = false;
  std::uint64_t productRenderBridgeRoomEditorCursorCount = 0;
  bool productRenderBridgeRoomEditorPreviewVisible = false;
  std::uint64_t productRenderBridgeRoomEditorPreviewCount = 0;
  bool productRenderBridgePropVisible = false;
  std::uint64_t productRenderBridgePropTileCount = 0;
  bool productRenderBridgePhysicsDebugVisible = false;
  std::uint64_t productRenderBridgePhysicsDebugItemCount = 0;
  std::uint64_t productRenderBridgePhysicsAabbDebugCount = 0;
  std::uint64_t productRenderBridgePhysicsContactNormalDebugCount = 0;
  bool productRenderBridgeMapMakerGridVisible = false;
  std::uint64_t productRenderBridgeMapMakerGridDotCount = 0;
  std::uint64_t productRenderBridgeMapMakerMajorGridDotCount = 0;
  bool productRenderBridgeMapMakerCubePreviewVisible = false;
  std::uint64_t productRenderBridgeMapMakerCubePreviewCount = 0;
  bool productFeedbackBridgeReady = false;
  std::uint64_t productFeedbackBridgeLineCount = 0;
  bool productVulkanRoomMeshCpuReady = false;
  bool productVulkanRoomMeshBackendPresented = false;
  std::string productVulkanRoomMeshSource = "none";
  std::string productVulkanRoomAssetId = "none";
  bool productVulkanRoomFloorVisible = false;
  bool productVulkanRoomWallVisible = false;
  bool productVulkanRoomGridVisible = false;
  std::uint64_t productVulkanRoomSourceMeshCount = 0;
  std::uint64_t productVulkanRoomVertexCount = 0;
  std::uint64_t productVulkanRoomIndexCount = 0;
  std::uint64_t productVulkanRoomDrawCount = 0;
  std::uint64_t productVulkanRoomFloorDrawCount = 0;
  std::uint64_t productVulkanRoomWallDrawCount = 0;
  std::uint64_t productVulkanRoomGridLineDrawCount = 0;
  bool productVulkanRoomGridTruncated = false;
  std::uint64_t productVulkanRoomGeometrySignature = 0;
};

}  // namespace iggy3d
