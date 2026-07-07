#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

struct ProductCreativeUiCommandMutationDiagnostics {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  std::string status = "Unknown";
  std::string documentStatus = "Unknown";
  std::string kind = "Unknown";
  std::uint64_t target = 0;
  std::uint64_t objectId = 0;
  std::string objectKind = "Unknown";
  bool visibleBefore = false;
  bool visibleAfter = false;
  bool lockedBefore = false;
  bool lockedAfter = false;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::string message = "none";
};

struct ProductCreativeUiCommandCreateDiagnostics {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  std::string status = "Unknown";
  std::uint64_t objectId = 0;
  std::string objectKind = "Unknown";
  std::string objectName = "none";
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::uint64_t dirtyFlags = 0;
  std::string message = "none";
  std::string reasonCode = "none";
};

struct ProductCreativeUiCommandDeleteDiagnostics {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  bool removed = false;
  std::uint64_t objectId = 0;
  std::string objectKind = "Unknown";
  std::string objectName = "none";
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::uint64_t dirtyFlags = 0;
  std::string status = "Unknown";
  std::string message = "none";
  std::string reasonCode = "none";
};

struct ProductCreativeUiCommandUndoDiagnostics {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  bool hadSnapshot = false;
  std::uint64_t documentId = 0;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::uint64_t objectCountBefore = 0;
  std::uint64_t objectCountAfter = 0;
  std::uint64_t depthBefore = 0;
  std::uint64_t depthAfter = 0;
  std::string status = "creative_undo_not_requested";
  std::string message = "creative_undo_not_requested";
  std::string reasonCode = "creative_undo_not_requested";
};

struct ProductCreativeUiCommandRoomShellDiagnostics {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  std::uint64_t roomObjectId = 0;
  std::uint64_t generatedObjectCount = 0;
  std::uint64_t removedObjectCount = 0;
  std::uint64_t floorCount = 0;
  std::uint64_t wallCount = 0;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::string status = "creative_room_shell_not_requested";
  std::string reasonCode = "creative_room_shell_not_requested";
  std::string message = "creative_room_shell_not_requested";
};

struct ProductCreativeBakedRoomRefreshDiagnostics {
  bool requested = false;
  bool accepted = false;
  bool clearedActiveRoom = false;
  std::string status = "product_creative_baked_room_not_requested";
  std::string reasonCode = "product_creative_baked_room_not_requested";
  bool bakeMeasured = false;
  std::uint64_t bakeElapsedMicroseconds = 0;
  std::uint64_t bakedDocumentRevision = 0;
  std::uint64_t staticMeshCount = 0;
  std::uint64_t anchorCount = 0;
  std::uint64_t spatialSurfaceCount = 0;
  bool collisionReady = false;
  std::uint64_t collisionQuerySurfaceCount = 0;
};

struct ProductCreativeUiCommandDiagnostics {
  bool requested = false;
  bool facadeAvailable = false;
  bool inputConsumed = false;
  bool inputEnabled = false;
  bool accepted = false;
  bool changed = false;
  std::string kind = "none";
  std::string tool = "none";
  std::string objectKind = "Unknown";
  std::string toolBefore = "Select";
  std::string toolAfter = "Select";
  std::string semanticId = "none";
  std::string status = "product_creative_ui_command_not_requested";
  std::string reasonCode = "product_creative_ui_command_not_requested";
  ProductCreativeUiCommandMutationDiagnostics mutation;
  ProductCreativeUiCommandCreateDiagnostics create;
  ProductCreativeUiCommandDeleteDiagnostics deleteObject;
  ProductCreativeUiCommandUndoDiagnostics undo;
  ProductCreativeUiCommandRoomShellDiagnostics shell;
  ProductCreativeBakedRoomRefreshDiagnostics bakedRoomRefresh;
};

}  // namespace iggy3d
