#pragma once

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"

namespace iggy3d {

void initializeProductStarterTransition(FrontendState& frontend,
                                        ProductAppWindowState& window,
                                        bool hasCompatibleSave);

void enterProductGameplayTransition(FrontendState& frontend,
                                    ProductAppWindowState& window,
                                    FrontendAction launchAction);

void clearProductMapMakerMode(ProductAppWindowState& window);
void clearProductGameplayMovementTuning(ProductAppWindowState& window);
void clearProductMenuOwnedTransientModes(ProductAppWindowState& window);
void clearProductGameplayOnlyModes(ProductAppWindowState& window);
void clearProductGameplayOnlyModes(ProductAppWindowState& window,
                                   FrontendSettings& settings);

void openProductPauseTransition(FrontendState& frontend,
                                ProductAppWindowState& window,
                                FrontendAction selectedAction);

void openProductPauseSettingsTransition(FrontendState& frontend,
                                        ProductAppWindowState& window,
                                        FrontendSettingsTab& settingsTab);

void openProductPauseDevToolsTransition(FrontendState& frontend,
                                        ProductAppWindowState& window,
                                        FrontendDevToolsCategory category);

void closeProductOverlayToGameplayTransition(FrontendState& frontend,
                                             ProductAppWindowState& window);

void returnProductToTitleTransition(FrontendState& frontend,
                                    ProductAppWindowState& window);
void returnProductToTitleTransition(FrontendState& frontend,
                                    ProductAppWindowState& window,
                                    FrontendSettings& settings);

}  // namespace iggy3d
