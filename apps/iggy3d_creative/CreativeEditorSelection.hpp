#pragma once

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorSelectionFrame {
  iggy3d::creative::Id selectedId = 0;
  const iggy3d::creative::CreativeObject* selected = nullptr;
  bool hasSelection = false;
  iggy3d::Vec3 boxMin{-0.5F, 0.0F, -0.5F};
  iggy3d::Vec3 boxMax{0.5F, 1.0F, 0.5F};
};

[[nodiscard]] CreativeEditorSelectionFrame resolveCreativeEditorSelectionFrame(
    const iggy3d::creative::Facade& facade);

}  // namespace iggy3d_creative_app
