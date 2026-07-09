#pragma once

#include <cctype>
#include <string>
#include <string_view>

namespace iggy3d {

struct NpcBehaviorProfileId {
  std::string value;
};

inline bool isValidNpcBehaviorProfileId(std::string_view profileId) {
  if (profileId.empty()) {
    return false;
  }
  for (const char c : profileId) {
    const auto uc = static_cast<unsigned char>(c);
    if (!(std::islower(uc) || std::isdigit(uc) || c == '_')) {
      return false;
    }
  }
  return true;
}

}  // namespace iggy3d
