#pragma once

#include "runtime/math_objects/MathObjects.hpp"
#include "scene/GalleryScene.hpp"

namespace paths {

// Tessellates the model's primitive placements into the existing renderer's
// vertex/uint16 index contract. Geometry rebuilds only when the model changes.
// Cached unit meshes and reserved frame buffers avoid allocation during orbit.
class MathObjectScene {
public:
  MathObjectScene();
  [[nodiscard]] const SceneFrame& publish(const MathObjectSnapshot&, SceneViewport);
  void resetView();
  [[nodiscard]] bool navigate(iggy3d::ProductCreativeViewportNavigationOperation, float x, float y);
  [[nodiscard]] iggy3d::Vec3 project(iggy3d::Vec3) const;
  [[nodiscard]] const SceneFrame& frame() const { return frame_; }
private:
  void rebuild(const MathObjectSnapshot&);
  SceneFrame frame_;
  iggy3d::Aabb3 bounds_{};
  iggy3d::ProductCreativeViewportPose camera_{};
  iggy3d::ProductCreativeViewportFocus focus_{};
  MathObjectKind kind_ = MathObjectKind::Count;
  unsigned level_ = 0;
  std::uint64_t revision_ = 0, frameId_ = 0;
};

} // namespace paths
