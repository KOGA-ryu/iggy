#pragma once

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/tools/Clipboard.hpp"
#include "app/iggy3d/creative/tools/TerrainStamp.hpp"

namespace iggy3d::creative {

struct CreativeAppState {
  Facade facade;
  CreativeDocumentHistory history;
  CreativeClipboard clipboard;
  CreativeTerrainStamp terrainStamp;
};

}  // namespace iggy3d::creative
