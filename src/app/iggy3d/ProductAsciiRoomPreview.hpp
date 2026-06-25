#pragma once

#include <string>
#include <string_view>

#include "app/iggy3d/ReceiptBuilder.hpp"

namespace iggy3d {

std::string decodeProductAsciiRoomAutomationText(std::string_view value);
bool buildProductAsciiRoomPreview(ProductAppWindowState& window);

}  // namespace iggy3d
