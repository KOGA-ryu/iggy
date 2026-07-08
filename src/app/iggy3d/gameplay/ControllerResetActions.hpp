#pragma once

#include <string_view>

namespace iggy3d {

class Session;
struct ProductAppWindowState;

void submitProductReset(Session& session,
                        ProductAppWindowState& window,
                        std::string_view source);

}  // namespace iggy3d
