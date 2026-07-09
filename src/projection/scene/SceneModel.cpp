#include "projection/scene/SceneModel.hpp"

namespace iggy3d {

std::string_view sceneModelId(SceneModelKind kind) {
  switch (kind) {
    case SceneModelKind::PlayerBean:
      return "bean_player";
    case SceneModelKind::NpcBean:
      return "bean_npc";
    case SceneModelKind::CodexProbeBean:
      return "bean_codex_probe";
  }
  return "bean_player";
}

bool parseSceneModelId(std::string_view value, SceneModelKind& out) {
  if (value == "bean_player") {
    out = SceneModelKind::PlayerBean;
    return true;
  }
  if (value == "bean_npc") {
    out = SceneModelKind::NpcBean;
    return true;
  }
  if (value == "bean_codex_probe") {
    out = SceneModelKind::CodexProbeBean;
    return true;
  }
  return false;
}

}  // namespace iggy3d
