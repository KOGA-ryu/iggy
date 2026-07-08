#pragma once

#include "app/iggy3d/menu/FrontendRouter.hpp"

namespace iggy3d {
struct ProductAppWindowState;
}

namespace iggy3d::test {

inline ProductActiveSurfaceFrame liveSurface(const FrontendState& frontend,
                                             ProductAppWindowState& window) {
  return resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(frontend, window));
}

}  // namespace iggy3d::test
