#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "core/math/Vec3.hpp"
#include "EditorEdits.hpp"

namespace iggy3d_creative_app {

struct BrushFootprint {
  float sizeX = 1.0F;
  float height = 1.0F;
  float sizeZ = 1.0F;
};

[[nodiscard]] BrushFootprint descriptorBoundsFootprint(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor);
[[nodiscard]] bool validBrushFootprint(BrushFootprint footprint);
[[nodiscard]] bool isStandingSurfaceFootprint(BrushFootprint footprint);

[[nodiscard]] bool descriptorSupportsBoxPlacement(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor);
[[nodiscard]] bool descriptorSupportsLinePlacement(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor);
[[nodiscard]] bool descriptorSupportsPointPlacement(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor);
[[nodiscard]] bool descriptorSupportsPathPlacement(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor);
[[nodiscard]] bool descriptorSupportsBrushPlacement(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor);
[[nodiscard]] bool descriptorAvailableInStandaloneBrushPalette(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor);

[[nodiscard]] BrushFootprint brushFootprintForDescriptor(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor);
[[nodiscard]] std::vector<iggy3d::creative::CreativeObjectKind>
buildBrushPaletteFromDescriptors();
[[nodiscard]] iggy3d::creative::CreativeObjectKind firstBrushKind(
    const std::vector<iggy3d::creative::CreativeObjectKind>& palette);
[[nodiscard]] iggy3d::creative::CreativeObjectKind nextBrushKind(
    const std::vector<iggy3d::creative::CreativeObjectKind>& palette,
    iggy3d::creative::CreativeObjectKind current);

[[nodiscard]] std::vector<iggy3d::creative::CreativePathPoint>
initialPathPointsForAnchor(iggy3d::Vec3 cellCenter);

[[nodiscard]] iggy3d::Vec3 snapGroundToCellCenter(double worldX,
                                                  double worldZ,
                                                  double cellSize);

[[nodiscard]] std::string pathPointsSummary(
    const std::vector<iggy3d::creative::CreativePathPoint>& points);

[[nodiscard]] iggy3d::creative::CreativeDocumentCreateRequest
buildBrushCreateRequest(iggy3d::creative::CreativeObjectKind brush,
                        iggy3d::Vec3 cellCenter,
                        std::uint64_t ordinal);

[[nodiscard]] iggy3d::creative::CreativeDocumentCreateReceipt placeBrushObject(
    iggy3d::creative::Facade& facade,
    iggy3d::creative::CreativeObjectKind brush,
    iggy3d::Vec3 cellCenter,
    std::uint64_t ordinal);

[[nodiscard]] iggy3d::creative::CreativeDocumentCreateReceipt
placeBrushObjectWithUndo(iggy3d::creative::Facade& facade,
                         StandaloneUndoStack& undoStack,
                         iggy3d::creative::CreativeObjectKind brush,
                         iggy3d::Vec3 cellCenter,
                         std::uint64_t ordinal,
                         std::string_view source);

}  // namespace iggy3d_creative_app
