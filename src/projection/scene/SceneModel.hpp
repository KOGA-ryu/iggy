#pragma once

#include <cstdint>
#include <string_view>

namespace iggy3d {

enum class SceneModelKind : std::uint8_t {
  PlayerBean,
  NpcBean,
  CodexProbeBean,
};

std::string_view sceneModelId(SceneModelKind kind);
bool parseSceneModelId(std::string_view value, SceneModelKind& out);

}  // namespace iggy3d
