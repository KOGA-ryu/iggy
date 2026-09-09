#pragma once
#include "runtime/motion/MotionLesson.hpp"
#include "scene/GalleryScene.hpp"

namespace paths {
// A presentation adapter: every cart coordinate comes from sampleMotion.
class MotionScene {
public:
  [[nodiscard]] const SceneFrame& publish(const MotionLesson&,SceneViewport);
  [[nodiscard]] iggy3d::Vec3 project(iggy3d::Vec3) const;
  [[nodiscard]] const SceneFrame& frame() const { return frame_; }
  [[nodiscard]] double minimum() const { return minimum_; }
  [[nodiscard]] double maximum() const { return maximum_; }
private:
  SceneFrame frame_;
  double minimum_=0,maximum_=12;
};
} // namespace paths
