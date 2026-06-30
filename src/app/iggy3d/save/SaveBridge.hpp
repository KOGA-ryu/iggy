#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

#include "app/frontend/SaveSlotModel.hpp"
#include "app/iggy3d/save/Catalog.hpp"
#include "runtime/save/SaveFileStore.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

struct ProductSaveBridgeResult {
  std::filesystem::path saveRoot;
  ProductSaveCatalogBuildResult catalog;
  SaveSlotList slots;
  std::string_view status = "save_bridge_ready";
};

struct ProductSaveWriteRequest {
  std::filesystem::path saveRoot;
  std::string saveIdHint;
  std::string attemptToken;
  const SessionState* state = nullptr;
  const SaveAuthoredRoomSection* authoredRoom = nullptr;
  std::string worldId;
  std::string worldTitle;
  std::string saveTitle;
  std::string saveType;
  std::string createdAtUtc;
  std::string savedAtUtc;
};

struct ProductSaveWriteResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string durableReason = "not_requested";
  SaveFileRecord record;
  SaveFileDurableWritePaths paths;
  std::uint64_t encodedBytes = 0;
  bool durableWriteRequested = false;
  bool tempWritten = false;
  bool tempValidated = false;
  bool committed = false;
  bool finalValidated = false;
  bool previousExisted = false;
  bool previousPreserved = true;
  std::string worldId;
  std::string worldTitle;
  std::string saveTitle;
  std::string saveType;
  std::string createdAtUtc;
  std::string savedAtUtc;
};

struct ProductSaveLoadRequest {
  std::filesystem::path path;
  Session* session = nullptr;
  std::string expectedPackageId;
  std::string expectedScenarioId;
};

struct ProductSaveLoadResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  SaveFileRecord record;
  std::uint64_t previousHash = 0;
  std::uint64_t loadedHash = 0;
  bool fileRead = false;
  bool decoded = false;
  bool compatibilityChecked = false;
  bool sessionLoaded = false;
  SaveCodecStatus codecStatus = SaveCodecStatus::Ok;
  SaveLoadStatus loadStatus = SaveLoadStatus::InvalidEnvelope;
  SaveCompatibilityStatus compatibilityStatus = SaveCompatibilityStatus::Compatible;
  SessionLoadStatus sessionLoadStatus = SessionLoadStatus::Ok;
  bool authoredRoomPresent = false;
  std::string authoredRoomId = "none";
  std::uint64_t authoredFloorCount = 0;
  std::uint64_t authoredWallCount = 0;
  std::uint64_t authoredObjectCount = 0;
  std::uint64_t authoredMarkerCount = 0;
  SaveAuthoredRoomSection authoredRoom;
};

struct ProductSaveSoftDeleteRequest {
  std::filesystem::path saveRoot;
  std::string saveId;
};

struct ProductSaveSoftDeleteResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string softDeleteReason = "not_requested";
  SaveFileSoftDeletePaths paths;
  std::string saveId = "none";
  bool saveMoved = false;
  bool snapshotMoved = false;
  bool snapshotMissing = false;
  bool targetExisted = false;
  bool snapshotTargetExisted = false;
};

struct ProductSaveRecoverRequest {
  std::filesystem::path saveRoot;
  std::string saveId;
};

struct ProductSaveRecoverResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string recoverReason = "not_requested";
  SaveFileSoftDeletePaths paths;
  std::string saveId = "none";
  bool saveRecovered = false;
  bool snapshotRecovered = false;
  bool snapshotMissing = false;
  bool targetExisted = false;
  bool snapshotTargetExisted = false;
};

enum class ProductSaveMutationStatus : std::uint8_t {
  Succeeded,
  SaveNotFound,
  NotActiveSave,
  FileOperationFailed,
  SnapshotMoveFailed,
  DecodeFailedAfterMutation,
};

struct ProductSaveMutationRequest {
  std::filesystem::path saveRoot;
  std::string saveId;
  std::string packageId;
  std::string scenarioId;
};

struct ProductSaveMutationResult {
  bool ok = false;
  ProductSaveMutationStatus status = ProductSaveMutationStatus::FileOperationFailed;
  std::string reasonCode = "not_requested";
  std::string affectedSaveId = "none";
  ProductSaveBridgeResult activeSaves;
  ProductSaveBridgeResult deletedSaves;
  ProductSaveSoftDeleteResult softDelete;
  ProductSaveRecoverResult recover;
};

// Current wall-clock time formatted as a UTC ISO-8601 second-granularity string
// ("YYYY-MM-DDTHH:MM:SSZ"). This is the single product save clock: world
// creation and every progress save stamp savedAtUtc through it so the catalog's
// newest-first ordering and Continue policy reflect real recency. Callers that
// need determinism (unit tests) supply their own timestamp instead of calling
// this.
std::string productSaveTimestampNowUtc();

// Mint a fresh, unique world id of the form "world_NNNN" by scanning the active
// and deleted save roots for the highest existing world_<number> and returning
// the next one (an empty root yields "world_0001"). World ids are set once at
// creation and preserved across re-saves by writeProductSessionSaveDurably, so
// this keeps each world's lineage distinct without reusing a deleted world's id.
std::string nextProductWorldId(const std::filesystem::path& saveRoot);

ProductSaveBridgeResult scanProductSaves(const std::filesystem::path& saveRoot,
                                         std::string_view packageId,
                                         std::string_view scenarioId);
ProductSaveBridgeResult scanDeletedProductSaves(
    const std::filesystem::path& saveRoot,
    std::string_view packageId,
    std::string_view scenarioId);
ProductSaveWriteResult writeProductSessionSaveDurably(
    const ProductSaveWriteRequest& request);
ProductSaveLoadResult loadProductSessionSave(
    const ProductSaveLoadRequest& request);
ProductSaveSoftDeleteResult softDeleteProductSave(
    const ProductSaveSoftDeleteRequest& request);
ProductSaveRecoverResult recoverProductSave(
    const ProductSaveRecoverRequest& request);
ProductSaveMutationResult softDeleteProductSaveAndRefresh(
    const ProductSaveMutationRequest& request);
ProductSaveMutationResult recoverProductSaveAndRefresh(
    const ProductSaveMutationRequest& request);

}  // namespace iggy3d
