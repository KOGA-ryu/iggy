#pragma once

#include <string>
#include <cstdint>

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
  bool productDrawRoomEditorCursorVisible = false;
  std::uint64_t productDrawRoomEditorCursorCount = 0;
  bool productDrawRoomEditorPreviewVisible = false;
  std::uint64_t productDrawRoomEditorPreviewCount = 0;
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
