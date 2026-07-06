#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "core/math/Vec3.hpp"

#include "StandaloneUndo.hpp"

namespace iggy3d_creative_app {

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
