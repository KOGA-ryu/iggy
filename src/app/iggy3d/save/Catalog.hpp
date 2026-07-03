#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d {

enum class ProductSaveCatalogLocation {
  Active,
  Deleted,
};

enum class ProductSaveContentKind {
  Unknown,
  ProductSession,
  CreativeDocument,
};

struct ProductSaveCatalogEntry {
  std::string saveId;
  std::filesystem::path path;
  ProductSaveCatalogLocation location = ProductSaveCatalogLocation::Active;
  ProductSaveContentKind contentKind = ProductSaveContentKind::Unknown;
  std::string worldId;
  std::string worldTitle;
  std::string saveTitle;
  std::string displayTitle;
  std::string saveType;
  std::string createdAtUtc;
  std::string savedAtUtc;
  std::string packageId;
  std::string scenarioId;
  std::uint64_t currentTick = 0;
  std::string savedStateHashHex;
  std::uint64_t authoredFloorCount = 0;
  std::uint64_t authoredWallCount = 0;
  std::uint64_t authoredObjectCount = 0;
  std::uint64_t authoredMarkerCount = 0;
  bool creativeDocumentPresent = false;
  std::uint64_t creativeDocumentId = 0;
  std::uint64_t creativeObjectCount = 0;
  std::uint64_t creativeNextObjectId = 0;
  std::filesystem::path snapshotPath;
  std::string snapshotStatus = "missing";
  bool snapshotAvailable = false;
  bool compatible = false;
  bool loadable = false;
  bool corrupt = false;
  bool deleted = false;
  bool recoverable = false;
  std::string disabledReason = "none";
};

struct ProductSaveCatalog {
  std::vector<ProductSaveCatalogEntry> entries;
};

struct ProductSaveCatalogBuildResult {
  bool ok = true;
  std::string status = "product_save_catalog_ready";
  std::string reasonCode = "product_save_catalog_ready";
  std::uint64_t activeCount = 0;
  std::uint64_t deletedCount = 0;
  std::uint64_t compatibleActiveCount = 0;
  std::uint64_t corruptCount = 0;
  ProductSaveCatalog catalog;
};

struct ProductContinueSelectionResult {
  bool selected = false;
  std::string status = "continue_no_active_saves";
  std::string reasonCode = "continue_no_active_saves";
  std::string selectedSaveId = "none";
  std::string selectedSavedAtUtc;
  std::uint64_t consideredCount = 0;
  std::uint64_t compatibleCount = 0;
  ProductSaveCatalogEntry entry;
};

std::string_view productSaveCatalogLocationName(
    ProductSaveCatalogLocation location);
std::string_view productSaveContentKindName(ProductSaveContentKind contentKind);
std::string productSaveDisplayTitle(const ProductSaveCatalogEntry& entry);
bool hasProductSaveCatalogTimestamp(std::string_view timestamp);
bool canLoadProductSave(const ProductSaveCatalogEntry& entry);
bool canOpenCreativeWorld(const ProductSaveCatalogEntry& entry);
ProductSaveCatalogBuildResult buildProductSaveCatalog(
    std::vector<ProductSaveCatalogEntry> entries);
std::vector<ProductSaveCatalogEntry> sortProductSaveCatalogEntries(
    const std::vector<ProductSaveCatalogEntry>& entries);
ProductContinueSelectionResult selectProductContinueSave(
    const ProductSaveCatalog& catalog);

}  // namespace iggy3d
