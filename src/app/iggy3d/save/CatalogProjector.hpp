#pragma once

#include <string_view>

#include "app/frontend/SaveBrowser.hpp"
#include "app/frontend/SaveSlotModel.hpp"
#include "app/iggy3d/save/Catalog.hpp"

namespace iggy3d {

SaveSlotCompatibility saveSlotCompatibilityFromCatalogEntry(
    const ProductSaveCatalogEntry& entry);
SaveSlotPreview saveSlotPreviewFromCatalogEntry(
    const ProductSaveCatalogEntry& entry);
SaveSlotList buildSaveSlotListFromCatalog(const ProductSaveCatalog& catalog);
SaveBrowserModel buildSaveBrowserModelFromCatalog(
    const ProductSaveCatalog& catalog,
    std::string_view selectedSaveId,
    FrontendSaveBrowserMode mode = FrontendSaveBrowserMode::Load);

}  // namespace iggy3d
