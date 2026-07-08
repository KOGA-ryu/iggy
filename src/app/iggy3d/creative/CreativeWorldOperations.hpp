#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "app/iggy3d/ProductCreativeBakedRoomRefresh.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/world/WorldService.hpp"

namespace iggy3d {

struct FrontendState;
struct ProductAppOptions;
struct ProductAppWindowState;
class Session;

namespace creative {
struct CreativeAppState;
}  // namespace creative

struct ProductCreativeNewWorldLaunchRequest {
  std::string title;
  std::string templateId = "empty";
  std::string requestedAtUtc;
  std::string attemptToken = "attempt_001";
  std::string packageId = "iggy3d.creative";
  std::string scenarioId = "creative.document";
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

}  // namespace iggy3d
