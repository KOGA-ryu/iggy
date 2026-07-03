#include "app/iggy3d/save/Catalog.hpp"

#include <algorithm>

namespace iggy3d {
namespace {

bool isActiveEntry(const ProductSaveCatalogEntry& entry) {
  return entry.location == ProductSaveCatalogLocation::Active && !entry.deleted;
}

bool isDeletedEntry(const ProductSaveCatalogEntry& entry) {
  return entry.location == ProductSaveCatalogLocation::Deleted || entry.deleted;
}

bool entrySortBefore(const ProductSaveCatalogEntry& lhs,
                     const ProductSaveCatalogEntry& rhs) {
  const bool lhsHasTime = hasProductSaveCatalogTimestamp(lhs.savedAtUtc);
  const bool rhsHasTime = hasProductSaveCatalogTimestamp(rhs.savedAtUtc);
  if (lhsHasTime != rhsHasTime) {
    return lhsHasTime;
  }
  if (lhsHasTime && lhs.savedAtUtc != rhs.savedAtUtc) {
    return lhs.savedAtUtc > rhs.savedAtUtc;
  }
  if (lhs.saveId != rhs.saveId) {
    return lhs.saveId > rhs.saveId;
  }
  return productSaveCatalogLocationName(lhs.location) <
         productSaveCatalogLocationName(rhs.location);
}

}  // namespace

std::string_view productSaveCatalogLocationName(
    ProductSaveCatalogLocation location) {
  switch (location) {
    case ProductSaveCatalogLocation::Active:
      return "active";
    case ProductSaveCatalogLocation::Deleted:
      return "deleted";
  }
  return "active";
}

std::string_view productSaveContentKindName(ProductSaveContentKind contentKind) {
  switch (contentKind) {
    case ProductSaveContentKind::Unknown:
      return "unknown";
    case ProductSaveContentKind::ProductSession:
      return "product_session";
    case ProductSaveContentKind::CreativeDocument:
      return "creative_document";
  }
  return "unknown";
}

std::string productSaveDisplayTitle(const ProductSaveCatalogEntry& entry) {
  if (!entry.saveTitle.empty()) {
    return entry.saveTitle;
  }
  if (!entry.worldTitle.empty()) {
    return entry.worldTitle;
  }
  if (!entry.saveId.empty()) {
    return entry.saveId;
  }
  return "untitled_save";
}

bool hasProductSaveCatalogTimestamp(std::string_view timestamp) {
  return !timestamp.empty();
}

bool canLoadProductSave(const ProductSaveCatalogEntry& entry) {
  return isActiveEntry(entry) && entry.loadable && entry.compatible &&
         !entry.corrupt &&
         entry.contentKind == ProductSaveContentKind::ProductSession;
}

bool canOpenCreativeWorld(const ProductSaveCatalogEntry& entry) {
  return isActiveEntry(entry) && entry.compatible && !entry.corrupt &&
         entry.contentKind == ProductSaveContentKind::CreativeDocument;
}

ProductSaveCatalogBuildResult buildProductSaveCatalog(
    std::vector<ProductSaveCatalogEntry> entries) {
  ProductSaveCatalogBuildResult result;
  result.catalog.entries = sortProductSaveCatalogEntries(std::move(entries));
  for (auto& entry : result.catalog.entries) {
    if (entry.displayTitle.empty()) {
      entry.displayTitle = productSaveDisplayTitle(entry);
    }
    if (isDeletedEntry(entry)) {
      ++result.deletedCount;
    } else {
      ++result.activeCount;
      if (canLoadProductSave(entry)) {
        ++result.compatibleActiveCount;
      }
    }
    if (entry.corrupt) {
      ++result.corruptCount;
    }
  }
  return result;
}

std::vector<ProductSaveCatalogEntry> sortProductSaveCatalogEntries(
    const std::vector<ProductSaveCatalogEntry>& entries) {
  std::vector<ProductSaveCatalogEntry> sorted = entries;
  std::stable_sort(sorted.begin(), sorted.end(), entrySortBefore);
  return sorted;
}

ProductContinueSelectionResult selectProductContinueSave(
    const ProductSaveCatalog& catalog) {
  ProductContinueSelectionResult result;
  const ProductSaveCatalogEntry* selected = nullptr;

  for (const auto& entry : catalog.entries) {
    if (!isActiveEntry(entry)) {
      continue;
    }
    ++result.consideredCount;
    if (!canLoadProductSave(entry)) {
      continue;
    }
    ++result.compatibleCount;
    if (selected == nullptr || entrySortBefore(entry, *selected)) {
      selected = &entry;
    }
  }

  if (result.consideredCount == 0) {
    result.status = "continue_no_active_saves";
    result.reasonCode = result.status;
    return result;
  }
  if (selected == nullptr) {
    result.status = "continue_no_compatible_saves";
    result.reasonCode = result.status;
    return result;
  }

  result.selected = true;
  result.status = "continue_save_selected";
  result.reasonCode = result.status;
  result.selectedSaveId = selected->saveId;
  result.selectedSavedAtUtc = selected->savedAtUtc;
  result.entry = *selected;
  return result;
}

}  // namespace iggy3d
