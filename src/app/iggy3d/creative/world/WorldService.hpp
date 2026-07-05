#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"

namespace iggy3d {

struct CreativeWorldCreateRequest {
  std::filesystem::path saveRoot;
  std::string title;
  std::string templateId = "empty";
  std::string requestedAtUtc;
  std::string attemptToken = "attempt_001";
  std::string packageId = "iggy3d.creative";
  std::string scenarioId = "creative.document";
};

struct CreativeWorldCreateResult {
  bool accepted = false;
  std::string status = "creative_world_not_requested";
  std::string reasonCode = "creative_world_not_requested";
  std::string title;
  std::string templateId;
  std::string worldId;
  creative::CreativeDocumentId documentId = creative::kInvalidDocumentId;
  bool documentCreated = false;
  bool initialSaveWritten = false;
  std::string saveId = "none";
  std::filesystem::path path;
  bool worldIdScanMeasured = false;
  std::uint64_t worldIdScanMicroseconds = 0;
  std::uint64_t worldIdScanEntryCount = 0;
  std::string worldIdScanStatus = "product_world_id_scan_not_requested";
  bool documentIdScanMeasured = false;
  std::uint64_t documentIdScanMicroseconds = 0;
  std::uint64_t documentIdScanEntryCount = 0;
  std::string documentIdScanStatus =
      "creative_document_id_scan_not_requested";
  ProductCreativeSaveWriteResult saveWrite;
  creative::CreativeDocument document;
};

struct CreativeWorldOpenRequest {
  std::filesystem::path saveRoot;
  std::string saveId;
};

struct CreativeWorldSaveRequest {
  std::filesystem::path saveRoot;
  std::string saveId;
  std::string attemptToken = "attempt_001";
  creative::CreativeDocument* document = nullptr;
  std::string packageId = "iggy3d.creative";
  std::string scenarioId = "creative.document";
  std::string worldId;
  std::string worldTitle;
  std::string saveTitle;
  std::string saveType;
  std::string createdAtUtc;
  std::string savedAtUtc;
};

struct CreativeWorldOpenResult {
  bool accepted = false;
  std::string status = "creative_world_open_not_requested";
  std::string reasonCode = "creative_world_open_not_requested";
  std::string saveId = "none";
  std::filesystem::path path;
  ProductCreativeSaveLoadResult load;
  creative::CreativeDocument document;
  creative::CreativeDocumentId documentId = creative::kInvalidDocumentId;
  std::uint64_t objectCount = 0;
  creative::CreativeObjectId nextObjectId = creative::kInvalidObjectId;
  std::string packageId;
  std::string scenarioId;
  std::string worldId;
  std::string worldTitle;
  std::string saveTitle;
  std::string saveType;
  std::string createdAtUtc;
  std::string savedAtUtc;
};

struct CreativeWorldSaveResult {
  bool accepted = false;
  std::string status = "creative_world_save_not_requested";
  std::string reasonCode = "creative_world_save_not_requested";
  std::string saveId = "none";
  std::filesystem::path path;
  creative::CreativeDocumentId documentId = creative::kInvalidDocumentId;
  std::uint64_t objectCount = 0;
  creative::CreativeObjectId nextObjectId = creative::kInvalidObjectId;
  creative::CreativeObjectDirtyFlags dirtyFlagsBefore = 0;
  creative::CreativeObjectDirtyFlags dirtyFlagsDrained = 0;
  creative::CreativeObjectDirtyFlags dirtyFlagsAfter = 0;
  bool saved = false;
  ProductCreativeSaveWriteResult saveWrite;
  std::string packageId;
  std::string scenarioId;
  std::string worldId;
  std::string worldTitle;
  std::string saveTitle;
  std::string saveType;
  std::string createdAtUtc;
  std::string savedAtUtc;
};

[[nodiscard]] CreativeWorldCreateResult createCreativeWorld(
    const CreativeWorldCreateRequest& request);
[[nodiscard]] CreativeWorldOpenResult openCreativeWorld(
    const CreativeWorldOpenRequest& request);
[[nodiscard]] CreativeWorldSaveResult saveCreativeWorld(
    const CreativeWorldSaveRequest& request);

}  // namespace iggy3d
