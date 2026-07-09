#pragma once

#include <cstddef>

#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/map_maker/Grid.hpp"
#include "projection/scene/SceneProjection.hpp"

namespace iggy3d_creative_app {

struct StandaloneRoomBakePreviewScene {
  iggy3d::SceneProjectionResult scene;
  iggy3d::creative::CreativeRoomBakeResult roomBake;
  std::size_t standalonePreviewMeshCount = 0;
};

[[nodiscard]] StandaloneRoomBakePreviewScene buildStandaloneRoomBakePreviewScene(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::ProductMapMakerGridSnapshot& gridSnapshot);

void logStandaloneRoomBakeFinal(
    const StandaloneRoomBakePreviewScene& preview);

}  // namespace iggy3d_creative_app
