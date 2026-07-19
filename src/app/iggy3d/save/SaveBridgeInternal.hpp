#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace iggy3d::save_bridge_internal {

// Product identity carried on a save's durable metadata. Read back from an
// existing save so a re-save preserves the world identity that creation set.
struct ExistingSaveIdentity {
  bool found = false;
  std::string worldId;
  std::string worldTitle;
  std::string saveTitle;
  std::string saveType;
  std::string createdAtUtc;
  std::string savedAtUtc;
};

ExistingSaveIdentity readExistingSaveIdentity(
    const std::filesystem::path& root, std::string_view idHint);

std::string preferNonEmpty(const std::string& primary,
                           const std::string& fallback);

}  // namespace iggy3d::save_bridge_internal
