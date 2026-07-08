#pragma once

#include <optional>
#include <string_view>

#include "app/iggy3d/save/SaveBridge.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

struct ProductAppOptions;
struct ProductAppWindowState;

ProductSaveWriteResult writeProductCurrentSessionSave(
    const ProductAppOptions& options,
    const std::optional<Session>& activeSession,
    std::string_view source,
    ProductAppWindowState& window);

}  // namespace iggy3d
