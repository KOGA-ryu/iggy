#include "render/mesh/BeanMesh.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool indicesStayInRange(const iggy3d::BeanMesh& mesh) {
  for (const std::uint16_t index : mesh.indices) {
    if (index >= mesh.vertices.size()) {
      return false;
    }
  }
  return true;
}

bool modelIdsRoundTrip() {
  iggy3d::SceneModelKind parsed = iggy3d::SceneModelKind::PlayerBean;
  return expect(iggy3d::parseSceneModelId("bean_player", parsed) &&
                    parsed == iggy3d::SceneModelKind::PlayerBean,
                "player model id") &&
         expect(iggy3d::parseSceneModelId("bean_npc", parsed) &&
                    parsed == iggy3d::SceneModelKind::NpcBean,
                "npc model id") &&
         expect(iggy3d::parseSceneModelId("bean_codex_probe", parsed) &&
                    parsed == iggy3d::SceneModelKind::CodexProbeBean,
                "codex probe model id") &&
         expect(!iggy3d::parseSceneModelId("bean_unknown", parsed), "unknown model id");
}

bool meshesAreDrawableAndBounded() {
  const iggy3d::BeanMesh player = iggy3d::buildBeanMesh(iggy3d::SceneModelKind::PlayerBean);
  const iggy3d::BeanMesh npc = iggy3d::buildBeanMesh(iggy3d::SceneModelKind::NpcBean);
  const iggy3d::BeanMesh codex = iggy3d::buildBeanMesh(iggy3d::SceneModelKind::CodexProbeBean);
  return expect(!player.vertices.empty() && !player.indices.empty(), "player mesh populated") &&
         expect(!npc.vertices.empty() && !npc.indices.empty(), "npc mesh populated") &&
         expect(!codex.vertices.empty() && !codex.indices.empty(), "codex mesh populated") &&
         expect(player.indices.size() % 3U == 0U, "player triangles") &&
         expect(npc.indices.size() % 3U == 0U, "npc triangles") &&
         expect(codex.indices.size() % 3U == 0U, "codex triangles") &&
         expect(indicesStayInRange(player), "player indices bounded") &&
         expect(indicesStayInRange(npc), "npc indices bounded") &&
         expect(indicesStayInRange(codex), "codex indices bounded") &&
         expect(codex.vertices.size() > player.vertices.size(), "codex cap silhouette");
}

bool defaultSizesAreDistinct() {
  const iggy3d::Vec3 player = iggy3d::defaultBeanModelSize(iggy3d::SceneModelKind::PlayerBean);
  const iggy3d::Vec3 npc = iggy3d::defaultBeanModelSize(iggy3d::SceneModelKind::NpcBean);
  const iggy3d::Vec3 codex = iggy3d::defaultBeanModelSize(iggy3d::SceneModelKind::CodexProbeBean);
  return expect(player.y > npc.y, "player taller than npc") &&
         expect(npc.y > codex.y, "npc taller than codex") &&
         expect(player.x > codex.x, "player wider than codex");
}

}  // namespace

int main() {
  const bool ok = modelIdsRoundTrip() && meshesAreDrawableAndBounded() &&
                  defaultSizesAreDistinct();
  return ok ? 0 : 1;
}
