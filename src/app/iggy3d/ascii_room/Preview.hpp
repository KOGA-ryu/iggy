#pragma once

#include <string>
#include <string_view>

#include "app/iggy3d/ascii_room/Authoring.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"

namespace iggy3d {

std::string decodeProductAsciiRoomAutomationText(std::string_view value);
ProductAsciiRoomAuthoringRequest productAsciiRoomAuthoringRequestFromDraft(
    const ProductAppWindowState& window);
void recordProductAsciiRoomPreview(std::string_view sourceName,
                                   std::string_view roomId,
                                   const ProductAsciiRoomAuthoringResult& result,
                                   ProductAppWindowState& window);
ProductAsciiRoomAuthoringResult buildProductAsciiRoomPreviewResult(
    ProductAppWindowState& window);
bool buildProductAsciiRoomPreview(ProductAppWindowState& window);

}  // namespace iggy3d
