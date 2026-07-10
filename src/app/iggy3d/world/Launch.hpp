#pragma once

#include <optional>

#include "runtime/session/Session.hpp"

namespace iggy3d {

struct FrontendState;
struct PackageLoadResult;
struct ProductAppOptions;
struct ProductAppWindowState;
struct ProductSaveBridgeResult;
struct ProductWorldTemplate;
struct WorldSetupDraft;

void clearProductGameplayLaunchState(std::optional<Session>& activeSession,
                                     ProductAppWindowState& window);

bool createProductSessionFromPackage(const PackageLoadResult& package,
                                     std::optional<Session>& activeSession,
                                     ProductAppWindowState& window);

bool createProductSession(const ProductAppOptions& options,
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

void launchProductNewWorld(const ProductAppOptions& options,
                           const WorldSetupDraft& worldSetupDraft,
                           FrontendState& frontend,
                           std::optional<Session>& activeSession,
                           ProductAppWindowState& window);

}  // namespace iggy3d
