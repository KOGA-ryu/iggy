#pragma once

#include "EditorDesktopCommandsInternal.hpp"
#include "EditorWorldLayout.hpp"

#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <cstddef>
#include <string_view>

namespace iggy3d_creative_app {

struct GeneratedSourceScopeResolution {
  bool ancestor = false;
  bool stable = false;
};

[[nodiscard]] GeneratedSourceScopeResolution resolveGeneratedSourceScope(
    const iggy3d::creative::CreativeWorldLayout& layout,
    const iggy3d::creative::CreativeObject& object,
    iggy3d::creative::CreativeWorldLayoutTable table,
    std::size_t index, std::string_view stableKey);

[[nodiscard]] iggy3d::creative::CreativeObjectId
findGeneratedSourceScopeObject(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::creative::CreativeWorldLayout& layout,
    iggy3d::creative::CreativeWorldLayoutTable table,
    std::size_t index,
    iggy3d::creative::CreativeObjectKind preferredKind) noexcept;

[[nodiscard]] const iggy3d::creative::CreativeCatalogEntry* findCatalogAsset(
    const iggy3d::creative::CreativeCatalogState& catalog,
    std::string_view assetId) noexcept;

CreativeEditorWorldLayoutEditReceipt repairWorldLayoutAsset(
    CreativeEditorWorldLayoutState& state,
    const iggy3d::creative::CreativeCatalogState& catalog,
    double gridCellSizeMeters,
    const CreativeDesktopWorldLayoutAssetRepairPayload& payload);

bool dispatchCreativeDesktopWorldLayoutSourceCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result);

bool dispatchCreativeDesktopWorldLayoutBuildingCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result);

bool dispatchCreativeDesktopWorldLayoutStructureCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result);

bool dispatchCreativeDesktopWorldLayoutLifecycleCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result);

}  // namespace iggy3d_creative_app
