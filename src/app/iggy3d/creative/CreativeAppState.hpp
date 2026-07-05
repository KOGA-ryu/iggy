#pragma once

#include "app/iggy3d/creative/Facade.hpp"

namespace iggy3d::creative {

// Self-contained home for creative's app-scoped state. SLICE 1 wraps only the
// logical Facade so the container can thread along the seams the Facade already
// travels; identity + receipt mirrors migrate off ProductAppWindowState in
// later slices without re-threading.
struct CreativeAppState {
  Facade facade;
};

}  // namespace iggy3d::creative
