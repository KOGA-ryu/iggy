#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/world/DefaultWorldTemplate.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
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
