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

bool hasActiveCollision(const ProductSaveCatalogEntry& deletedEntry,
                        const std::vector<ProductSaveCatalogEntry>& activeEntries) {
  return std::any_of(activeEntries.begin(), activeEntries.end(),
                     [&deletedEntry](const ProductSaveCatalogEntry& activeEntry) {
                       return isActiveEntry(activeEntry) &&
                              activeEntry.saveId == deletedEntry.saveId;
                     });
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
         !entry.corrupt;
}

bool canSoftDeleteProductSave(const ProductSaveCatalogEntry& entry) {
  return isActiveEntry(entry) && !entry.saveId.empty();
}

bool canRecoverProductSave(
    const ProductSaveCatalogEntry& entry,
    const std::vector<ProductSaveCatalogEntry>& activeEntries) {
  return isDeletedEntry(entry) && entry.recoverable && !entry.saveId.empty() &&
         !hasActiveCollision(entry, activeEntries);
}

ProductSaveCatalogBuildResult buildProductSaveCatalog(
    std::vector<ProductSaveCatalogEntry> entries) {
  ProductSaveCatalogBuildResult result;
  result.catalog.entries = sortProductSaveCatalogEntries(std::move(entries));
  // Pass 1: fill display titles and collect the active entries. canRecoverProductSave needs the
  // active list to reject a deleted id that collides with a live save (the store's recover would
  // refuse it), so the recoverable count can only be predicate-authoritative after we know them.
  std::vector<ProductSaveCatalogEntry> activeEntries;
  for (auto& entry : result.catalog.entries) {
    if (entry.displayTitle.empty()) {
      entry.displayTitle = productSaveDisplayTitle(entry);
    }
    if (isActiveEntry(entry)) {
      activeEntries.push_back(entry);
    }
  }
  // Pass 2: counts. recoverableDeletedCount now goes through canRecoverProductSave — a deleted
  // entry with an empty id OR an active-id collision cannot actually be recovered, so it is no
  // longer counted (declared count correction; recoverableDeletedCount has no receipt consumer).
  for (const auto& entry : result.catalog.entries) {
    if (isDeletedEntry(entry)) {
      ++result.deletedCount;
      if (canRecoverProductSave(entry, activeEntries)) {
        ++result.recoverableDeletedCount;
      }
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
