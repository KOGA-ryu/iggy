#pragma once

#if defined(IGGY3D_HAS_SDL3)

#include <cstdint>
#include <string>
#include <string_view>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"

struct SDL_Renderer;

namespace iggy3d {

struct ProductDeleteConfirmModel;
struct ProductSaveBridgeResult;
struct ProductWorldTemplate;
struct WorldSetupDraft;

void drawGameplayMovementTuningHud(
    SDL_Renderer& renderer,
    const ProductGameplayMovementTuning& tuning,
    ProductGameplayMovementTuningField selectedField,
    bool visible);
void drawMenuRow(SDL_Renderer& renderer,
                 std::string_view label,
                 bool selected,
                 bool enabled,
                 float x,
                 float y);
void drawStarterDetailPanel(SDL_Renderer& renderer);
void drawNewWorldPanel(SDL_Renderer& renderer,
                       const ProductWorldTemplate& world,
                       const ProductSaveBridgeResult& saves,
                       const WorldSetupDraft& draft,
                       bool dungeonDraftEditMode,
                       bool dungeonDraftModified,
                       std::uint64_t dungeonDraftCursorRow,
                       std::uint64_t dungeonDraftCursorColumn,
                       const std::string& dungeonDraftSelectedGlyph,
                       const std::string& dungeonDraftLastGlyph);
void drawLoadSavePanel(SDL_Renderer& renderer,
                       FrontendSaveBrowserMode mode,
                       const ProductSaveBridgeResult& saves);
void drawDeleteConfirmPanel(SDL_Renderer& renderer,
                            const ProductDeleteConfirmModel& model);
void drawDevToolsPanel(SDL_Renderer& renderer,
                       FrontendDevToolsCategory selected);
void drawSettingsPanel(SDL_Renderer& renderer,
                       FrontendSettingsTab selected,
                       const ProductGameplayMovementTuning& movementTuning,
                       ProductGameplayMovementTuningField movementTuningField);

}  // namespace iggy3d

#endif
