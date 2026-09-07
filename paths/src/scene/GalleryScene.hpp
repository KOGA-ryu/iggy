#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "core/math/Aabb3.hpp"
#include "core/math/Mat4.hpp"
#include "app/iggy3d/creative/camera/ViewportNavigation.hpp"
#include "scene/TargetMotion.hpp"

namespace paths {

inline constexpr std::size_t kGalleryObjectCapacity = 64;
inline constexpr std::size_t kSceneVertexCapacity = 8192;
inline constexpr std::size_t kSceneIndexCapacity = 65536;
inline constexpr std::uint64_t kTargetSpawnTicks = 9, kTargetPopTicks = 18;

struct SceneObjectId {
  std::uint32_t value = 0;
  bool operator==(const SceneObjectId&) const = default;
};
struct SceneFrameId {
  std::uint64_t value = 0;
  bool operator==(const SceneFrameId&) const = default;
};
enum class VisualPhase : std::uint8_t { Spawning, Active, Popping, Retired };

// The parent's FirstRoomVertex layout, shared by CPU geometry and Vulkan.
struct SceneVertex { float position[3]{}, color[3]{}, uv0[2]{}; };
struct SceneViewport {
  float x = 0, y = 0, width = 1, height = 1;
  bool operator==(const SceneViewport&) const = default;
};
struct SceneDraw {
  SceneObjectId objectId;
  std::size_t firstIndex = 0, indexCount = 0;
  iggy3d::Aabb3 bounds{};
  VisualPhase phase = VisualPhase::Active;
};
struct SceneFrame {
  SceneFrameId id;
  SceneViewport viewport;
  iggy3d::Mat4 clipFromWorld = iggy3d::identityMat4();
  iggy3d::Vec3 eye{}, forward{}, right{}, up{};
  std::vector<SceneVertex> vertices;
  std::vector<std::uint16_t> indices;
  std::vector<SceneDraw> draws;
};

enum class GalleryPrimitive : std::uint8_t { Box, Ramp, Frame, Sphere };
struct GalleryObject {
  SceneObjectId id;
  GalleryPrimitive primitive = GalleryPrimitive::Box;
  iggy3d::Vec3 origin{}, position{}, size{1,1,1}, color{0.2F,0.7F,0.8F};
  float yawDegrees = 0;
  PreparedRoute route;
  MotionState motion;
  MotionPreview preview;
  VisualPhase phase = VisualPhase::Active;
  std::uint64_t phaseStartTick = 0;
};

enum class GalleryActionKind : std::uint8_t {
  Tick, Pause, ResetCamera, FrameAll, Fly, Look, Orbit, Pan, Dolly, Pick, Select,
  AddBox, AddRamp, AddFrame, AddSphere, Delete, Position, Size, Yaw, Color, Patrol,
  Speed, Loop, Waypoint, Dwell, SegmentSpeed, Scrub, Route, Extent, RouteDwell, RouteSeed, Activate, Pop,
  FrameTargets,
};
struct GalleryAction {
  GalleryActionKind kind;
  iggy3d::Vec3 value{};
  float amount = 0;
  std::uint32_t index = 0;
};
struct GalleryResult {
  bool accepted = false;
  std::string_view reason = "invalid_action";
};
struct SceneHit {
  bool accepted = false;
  std::string_view reason = "picking_unavailable";
  SceneObjectId object;
  float distance = 0;
  VisualPhase phase = VisualPhase::Retired;
};
struct ResetTargets {
  std::span<const iggy3d::Vec3> colours;
  // Supplying a route creates/reconfigures slots. Omitting it preserves their
  // physical motion while resetting the visual lifecycle for a new challenge.
  std::optional<RouteSpec> route;
  bool animateSpawn = true;
};

// Sole scene mutation/input owner. No SDL, ImGui, Vulkan, or filesystem access.
// Bounded scene and fixed 60 Hz patrol ticks; a realtime call accepts <=250 ms.
class GalleryScene {
public:
  explicit GalleryScene(bool seedWorkshop = true);
  [[nodiscard]] GalleryResult dispatch(const GalleryAction& action);
  [[nodiscard]] GalleryResult dispatch(const ResetTargets& action);
  void setViewport(SceneViewport viewport);
  // Publish once after input/simulation. Picking uses this exact displayed mesh.
  [[nodiscard]] const SceneFrame& publishFrame();
  [[nodiscard]] const SceneFrame& frame() const { return frame_; }
  [[nodiscard]] SceneHit hitTestPresentedFrame(SceneFrameId, float u, float v) const;
  [[nodiscard]] std::span<const GalleryObject> objects() const { return objects_; }
  [[nodiscard]] const GalleryObject* selected() const;
  [[nodiscard]] SceneObjectId selectedId() const { return selected_; }
  [[nodiscard]] bool paused() const { return paused_; }
  [[nodiscard]] std::uint64_t tickCount() const { return tickCount_; }
  [[nodiscard]] const iggy3d::ProductCreativeViewportPose& camera() const { return camera_; }
  [[nodiscard]] iggy3d::Vec3 project(iggy3d::Vec3 world) const;
private:
  [[nodiscard]] bool rebuildRoute(GalleryObject& object);
  std::vector<GalleryObject> objects_;
  SceneFrame frame_;
  iggy3d::ProductCreativeViewportPose camera_;
  iggy3d::ProductCreativeViewportFocus focus_;
  SceneObjectId selected_;
  std::uint32_t nextId_ = 1;
  std::uint64_t nextFrameId_ = 1;
  std::uint64_t tickCount_ = 0;
  double accumulator_ = 0;
  bool paused_ = false, presented_ = false;
};

}  // namespace paths
