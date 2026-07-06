#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/world/DefaultWorldTemplate.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/iggy3d/creative/world/WorldService.hpp"
#include "app/input/InputAction.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

enum class ProductSaveFlowOperation {
  None,
  Delete,
};

struct ProductSaveFlowRequest {
  ProductSaveFlowOperation operation = ProductSaveFlowOperation::None;
  std::string slotId = "none";
  std::string sourceSurface = "none";
  std::string confirmationToken = "none";
};

struct ProductSaveFlowResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reason = "not_requested";
  std::string affectedSlotId = "none";
  std::uint64_t activeCountBefore = 0;
  std::uint64_t activeCountAfter = 0;
  std::uint64_t deletedCountAfter = 0;
  std::string selectedSlotAfter = "none";
};

struct ProductCreativeNewWorldLaunchRequest {
  std::string title;
  std::string templateId = "empty";
  std::string requestedAtUtc;
  std::string attemptToken = "attempt_001";
  std::string packageId = "iggy3d.creative";
  std::string scenarioId = "creative.document";
};

struct ProductCreativeBakedActiveRoomRefreshRequest {
  std::string roomId = "iggy3d_creative_baked_room";
  std::string sourceName = "iggy3d.creative";
  std::string sourceSubset = "creative_document_bake";
  bool includeHidden = false;
};

struct ProductCreativeBakedActiveRoomRefreshResult {
  bool accepted = false;
  std::string status = "product_creative_baked_room_not_requested";
  std::string reasonCode = "product_creative_baked_room_not_requested";
  creative::CreativeDocumentId documentId = creative::kInvalidDocumentId;
  std::uint64_t objectCount = 0;
  creative::CreativeRoomBakeReceipt bakeReceipt;
  std::uint64_t staticMeshCount = 0;
  std::uint64_t anchorCount = 0;
  std::uint64_t spatialSurfaceCount = 0;
  std::uint64_t staticMeshSourceCount = 0;
  std::uint64_t anchorSourceCount = 0;
  std::uint64_t spatialSurfaceSourceCount = 0;
  bool activeRoomLoaded = false;
  std::string activeRoomStatus = "not_loaded";
  bool collisionReady = false;
  std::uint64_t collisionQuerySurfaceCount = 0;
};

struct ProductCreativeNewWorldLaunchResult {
  bool accepted = false;
  std::string status = "product_creative_new_world_not_requested";
  std::string reasonCode = "product_creative_new_world_not_requested";
  bool sessionCreated = false;
  bool documentInstalled = false;
  bool enteredGameplay = false;
  std::string saveId = "none";
  std::filesystem::path path;
  std::string worldId;
  creative::CreativeDocumentId documentId = creative::kInvalidDocumentId;
  std::uint64_t objectCount = 0;
  creative::CreativeObjectId nextObjectId = creative::kInvalidObjectId;
  CreativeWorldCreateResult createResult;
  creative::CreativeFacadeDocumentInstallReceipt installReceipt;
  bool bakedActiveRoomRefreshRequested = false;
  bool bakedActiveRoomRefreshAccepted = false;
  ProductCreativeBakedActiveRoomRefreshResult bakedActiveRoomRefresh;
};

struct ProductCreativeOpenWorldLaunchRequest {
  std::string saveId;
};

struct ProductCreativeOpenWorldLaunchResult {
  bool accepted = false;
  std::string status = "product_creative_open_world_not_requested";
  std::string reasonCode = "product_creative_open_world_not_requested";
  bool sessionCreated = false;
  bool documentInstalled = false;
  bool enteredGameplay = false;
  std::string saveId = "none";
  std::filesystem::path path;
  std::string worldId;
  creative::CreativeDocumentId documentId = creative::kInvalidDocumentId;
  std::uint64_t objectCount = 0;
  creative::CreativeObjectId nextObjectId = creative::kInvalidObjectId;
  CreativeWorldOpenResult openResult;
  creative::CreativeFacadeDocumentInstallReceipt installReceipt;
  bool bakedActiveRoomRefreshRequested = false;
  bool bakedActiveRoomRefreshAccepted = false;
  ProductCreativeBakedActiveRoomRefreshResult bakedActiveRoomRefresh;
};

struct ProductCreativeCurrentWorldSaveResult {
  bool accepted = false;
  std::string status = "product_creative_save_not_requested";
  std::string reasonCode = "product_creative_save_not_requested";
  std::string saveId = "none";
  std::filesystem::path path;
  std::string worldId = "none";
  creative::CreativeDocumentId documentId = creative::kInvalidDocumentId;
  std::uint64_t objectCount = 0;
  creative::CreativeObjectId nextObjectId = creative::kInvalidObjectId;
  creative::CreativeObjectDirtyFlags dirtyFlagsBefore = 0;
  creative::CreativeObjectDirtyFlags dirtyFlagsDrained = 0;
  creative::CreativeObjectDirtyFlags dirtyFlagsAfter = 0;
  bool saved = false;
  CreativeWorldSaveResult saveResult;
};

std::string_view productSaveFlowOperationName(ProductSaveFlowOperation operation);

ProductWorldTemplate productWorldTemplateFromOptions(
    const ProductAppOptions& options);

ProductSaveWriteResult writeProductCurrentSessionSave(
    const ProductAppOptions& options,
    const std::optional<Session>& activeSession,
    std::string_view source,
    ProductAppWindowState& window);

const SaveSlotPreview* initializeSelectedProductSaveSlot(
    const SaveSlotList& slots,
    ProductAppWindowState& window);
const SaveSlotPreview* moveSelectedProductSaveSlot(const SaveSlotList& slots,
                                                   InputAction action,
                                                   ProductAppWindowState& window);
bool selectProductSaveSlotById(const SaveSlotList& slots,
                               std::string_view selectedId,
                               ProductAppWindowState& window);

ProductSaveBridgeResult scanDeletedProductSavesForOptions(
    const ProductAppOptions& options);
void recordDeletedProductSaveSlots(const ProductSaveBridgeResult& deletedSaves,
                                   ProductAppWindowState& window);
bool selectDeletedProductSaveSlotById(const SaveSlotList& slots,
                                      std::string_view selectedId,
                                      ProductAppWindowState& window);
void openDeletedProductSaveBrowser(const ProductAppOptions& options,
                                   ProductAppWindowState& window,
                                   FrontendState& frontend);
void executeProductSaveRecover(const ProductAppOptions& options,
                               ProductAppWindowState& window,
                               FrontendState& frontend);

void openProductSaveDeleteConfirmation(const SaveSlotList& slots,
                                       ProductAppWindowState& window,
                                       FrontendState& frontend);
void cancelProductSaveDeleteConfirmation(ProductAppWindowState& window,
                                         FrontendState& frontend);
ProductSaveFlowResult executeProductSaveSoftDelete(
    const ProductAppOptions& options,
    ProductSaveBridgeResult& saves,
    ProductAppWindowState& window,
    FrontendState& frontend);

void launchProductNewWorld(const ProductAppOptions& options,
                           const WorldSetupDraft& worldSetupDraft,
                           FrontendState& frontend,
                           std::optional<Session>& activeSession,
                           ProductAppWindowState& window);
ProductCreativeNewWorldLaunchResult launchProductCreativeNewWorld(
    const ProductAppOptions& options,
    const ProductCreativeNewWorldLaunchRequest& request,
    FrontendState& frontend,
    std::optional<Session>& activeSession,
    ProductAppWindowState& window,
    creative::CreativeAppState& creativeApp);
ProductCreativeOpenWorldLaunchResult launchProductCreativeOpenWorld(
    const ProductAppOptions& options,
    const ProductCreativeOpenWorldLaunchRequest& request,
    FrontendState& frontend,
    std::optional<Session>& activeSession,
    ProductAppWindowState& window,
    creative::CreativeAppState& creativeApp);
ProductCreativeCurrentWorldSaveResult saveProductCurrentCreativeWorld(
    const ProductAppOptions& options,
    creative::CreativeAppState& creativeApp,
    std::string_view source,
    ProductAppWindowState& window);
ProductCreativeBakedActiveRoomRefreshResult refreshProductCreativeBakedActiveRoom(
    const ProductCreativeBakedActiveRoomRefreshRequest& request,
    const std::optional<Session>& activeSession,
    ProductAppWindowState& window,
    const creative::CreativeAppState& creativeApp);
void launchProductContinueSave(const ProductAppOptions& options,
                               const ProductWorldTemplate& world,
                               const ProductSaveBridgeResult& saves,
                               FrontendState& frontend,
                               std::optional<Session>& activeSession,
                               ProductAppWindowState& window);
void launchProductLoadSaveSelection(const ProductAppOptions& options,
                                    const ProductWorldTemplate& world,
                                    const ProductSaveBridgeResult& saves,
                                    FrontendState& frontend,
                                    std::optional<Session>& activeSession,
                                    ProductAppWindowState& window);

}  // namespace iggy3d
