#pragma once

#include "EditorPlacement.hpp"
#include "app/iggy3d/creative/tools/AttachmentSnap.hpp"
#include "content/assets/StaticMeshAsset.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorWorldTarget;

struct CreativeEditorPlacementResolution {
  CreativeBrushPlacementAdmission admission;
  iggy3d::creative::CreativeAttachmentSnapResult attachment;
  bool socketTargeted = false;
};

[[nodiscard]] CreativeEditorPlacementResolution
resolveCreativeEditorPlacement(
    const iggy3d::creative::CreativeHotbarEntry& held,
    const CreativeEditorWorldTarget& target,
    iggy3d::creative::CreativePlacementYaw placementYaw,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog) noexcept;

}  // namespace iggy3d_creative_app
