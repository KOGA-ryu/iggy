#include "scene/GalleryScene.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "content/assets/GeneratedGeometry.hpp"
#include "core/math/EulerRotation.hpp"

namespace paths {
using namespace iggy3d;
namespace motion = iggy3d::creative;
namespace {
constexpr float kPi = 3.14159265358979323846F;
constexpr float kFov = kProductCreativeCameraVerticalFovDegrees * kPi / 180.0F;
constexpr float kNear = 0.1F, kFar = 200.0F;

bool bounded(Vec3 v, float limit) {
  return isFinite(v) && std::fabs(v.x) <= limit &&
         std::fabs(v.y) <= limit && std::fabs(v.z) <= limit;
}
Vec3 forward(const ProductCreativeViewportPose& pose) {
  const float yaw = pose.yawDegrees * kPi / 180.0F;
  const float pitch = pose.pitchDegrees * kPi / 180.0F;
  return {std::sin(yaw)*std::cos(pitch), std::sin(pitch), -std::cos(yaw)*std::cos(pitch)};
}

// Adapted from CreativeSceneFrame: row-major CPU matrices, Vulkan Z in [0,1].
void cameraFrame(SceneFrame& frame, const ProductCreativeViewportPose& pose) {
  frame.eye = pose.anchorPositionMeters + Vec3{0,kProductCreativeCameraEyeHeightMeters,0};
  frame.forward = forward(pose);
  frame.right = normalizedOr(cross(frame.forward, {0,1,0}), {1,0,0});
  frame.up = cross(frame.right, frame.forward);
  Mat4 view = identityMat4();
  const std::array<Vec3,3> axes{frame.right, frame.up, frame.forward * -1};
  for (std::size_t row = 0; row < 3; ++row) {
    view.m[row*4] = axes[row].x;
    view.m[row*4+1] = axes[row].y;
    view.m[row*4+2] = axes[row].z;
    view.m[row*4+3] = -dot(axes[row], frame.eye);
  }
  Mat4 projection{{{}}};
  const float f = 1/std::tan(kFov*0.5F);
  projection.m[0] = f * frame.viewport.height/frame.viewport.width;
  projection.m[5] = -f;
  projection.m[10] = kFar/(kNear-kFar);
  projection.m[11] = -(kFar*kNear)/(kFar-kNear);
  projection.m[14] = -1;
  frame.clipFromWorld = projection * view;
}

void triangle(SceneFrame& frame, std::size_t base, unsigned a, unsigned b, unsigned c) {
  // Parent primitives are double-sided; preserve their topology for room interiors.
  for (auto i : {a,b,c,c,b,a}) frame.indices.push_back(static_cast<std::uint16_t>(base+i));
}
struct UnitSphere {
  std::array<Vec3,114> vertices{};
  std::array<std::uint16_t,672> indices{};
  UnitSphere() {
    vertices[0]={0,0.5F,0};vertices[113]={0,-0.5F,0};
    for(unsigned ring=1;ring<8;++ring)for(unsigned slice=0;slice<16;++slice) {
      const float latitude=kPi*ring/8, longitude=2*kPi*slice/16;
      vertices[1+(ring-1)*16+slice]={0.5F*std::sin(latitude)*std::cos(longitude),
        0.5F*std::cos(latitude),0.5F*std::sin(latitude)*std::sin(longitude)};
    }
    std::size_t index=0;
    const auto tri=[&](unsigned a,unsigned b,unsigned c) {
      indices[index++]=static_cast<std::uint16_t>(a);
      indices[index++]=static_cast<std::uint16_t>(b);
      indices[index++]=static_cast<std::uint16_t>(c);
    };
    for(unsigned slice=0;slice<16;++slice) {
      const auto next=(slice+1)%16;
      tri(0,1+slice,1+next);tri(113,97+next,97+slice);
      for(unsigned ring=0;ring<6;++ring) {
        const auto a=1+ring*16+slice, b=1+ring*16+next;
        tri(a,a+16,b);tri(b,a+16,b+16);
      }
    }
  }
};
void primitive(SceneFrame& frame, GalleryPrimitive shape, Vec3 center, Vec3 size,
               Vec3 color, float yaw) {
  if(shape==GalleryPrimitive::Sphere) {
    static const UnitSphere sphere;
    const auto base=frame.vertices.size();
    const auto light=normalized(Vec3{-0.4F,0.7F,0.6F});
    for(const auto unit:sphere.vertices) {
      const auto p=center+Vec3{unit.x*size.x,unit.y*size.y,unit.z*size.z};
      const auto tint=color*(0.4F+0.6F*std::max(0.0F,dot(unit*2,light)));
      frame.vertices.push_back({{p.x,p.y,p.z},{tint.x,tint.y,tint.z},{}});
    }
    for(const auto index:sphere.indices)frame.indices.push_back(static_cast<std::uint16_t>(base+index));
    return;
  }
  if (shape == GalleryPrimitive::Frame) {
    const auto layout = generatedOpenFrameLayout(size);
    for (const auto& part : layout.parts)
      primitive(frame, GalleryPrimitive::Box,
                center + rotateEulerXyz(part.center, {0,yaw,0}), part.size, color, yaw);
    return;
  }
  // Box and ramp vertices/triangles carried from RoomMeshCpuPrimitives.cpp.
  const float x=size.x*0.5F, y=size.y*0.5F, z=size.z*0.5F;
  const std::array<Vec3,8> box{{{-x,-y,-z},{x,-y,-z},{x,y,-z},{-x,y,-z},
                               {-x,-y,z},{x,-y,z},{x,y,z},{-x,y,z}}};
  const std::array<Vec3,6> ramp{{{-x,-y,-z},{x,-y,-z},{-x,-y,z},
                                {x,-y,z},{-x,y,z},{x,y,z}}};
  const auto points = shape == GalleryPrimitive::Box ? std::span<const Vec3>(box) : std::span<const Vec3>(ramp);
  const auto base = frame.vertices.size();
  for (const auto point : points) {
    const auto p = center + rotateEulerXyz(point, {0,yaw,0});
    frame.vertices.push_back({{p.x,p.y,p.z},{color.x,color.y,color.z},{}});
  }
  constexpr std::array<std::array<unsigned,3>,12> boxTriangles{{
    {0,1,2},{0,2,3},{4,6,5},{4,7,6},{0,3,7},{0,7,4},
    {1,5,6},{1,6,2},{3,2,6},{3,6,7},{0,4,5},{0,5,1}}};
  constexpr std::array<std::array<unsigned,3>,8> rampTriangles{{
    {0,1,3},{0,3,2},{2,3,5},{2,5,4},{0,4,5},{0,5,1},{0,2,4},{1,5,3}}};
  const auto triangles = shape == GalleryPrimitive::Box
    ? std::span<const std::array<unsigned,3>>(boxTriangles)
    : std::span<const std::array<unsigned,3>>(rampTriangles);
  for (const auto t : triangles) triangle(frame,base,t[0],t[1],t[2]);
}
Vec3 vertex(const SceneVertex& v) { return {v.position[0],v.position[1],v.position[2]}; }
void objectMesh(SceneFrame& frame, SceneObjectId id, GalleryPrimitive shape,
                Vec3 pos, Vec3 size, Vec3 color, float yaw = 0,
                VisualPhase phase = VisualPhase::Active) {
  const auto firstVertex = frame.vertices.size();
  SceneDraw draw{id, frame.indices.size(), 0, {}};
  draw.phase=phase;
  primitive(frame,shape,pos,size,color,yaw*kPi/180);
  draw.indexCount = frame.indices.size()-draw.firstIndex;
  draw.bounds = {vertex(frame.vertices[firstVertex]),vertex(frame.vertices[firstVertex])};
  for (std::size_t i=firstVertex; i<frame.vertices.size(); ++i) {
    auto v=vertex(frame.vertices[i]);
    draw.bounds.min={std::min(draw.bounds.min.x,v.x),std::min(draw.bounds.min.y,v.y),std::min(draw.bounds.min.z,v.z)};
    draw.bounds.max={std::max(draw.bounds.max.x,v.x),std::max(draw.bounds.max.y,v.y),std::max(draw.bounds.max.z,v.z)};
  }
  frame.draws.push_back(draw);
}
float visualScale(const GalleryObject& object,std::uint64_t tick) {
  const auto age=tick-object.phaseStartTick;
  switch(object.phase) {
    case VisualPhase::Spawning:return std::clamp(static_cast<float>(age)/kTargetSpawnTicks,0.01F,1.0F);
    case VisualPhase::Active:return 1;
    case VisualPhase::Popping:return std::max(0.0F,1-static_cast<float>(age)/kTargetPopTicks);
    case VisualPhase::Retired:return 0;
  }
  return 0;
}
// Mesh narrow phase keeps ramp slopes and frame openings faithful to the image.
float triangleHit(Ray3 ray, Vec3 a, Vec3 b, Vec3 c) {
  const auto ab=b-a, ac=c-a, p=cross(ray.direction,ac);
  const float det=dot(ab,p);
  if (std::fabs(det)<1e-7F) return kFar;
  const float inv=1/det;
  const auto t=ray.origin-a;
  const float u=dot(t,p)*inv;
  if (u<0 || u>1) return kFar;
  const auto q=cross(t,ab);
  const float v=dot(ray.direction,q)*inv;
  if (v<0 || u+v>1) return kFar;
  const float distance=dot(ac,q)*inv;
  return distance>=kNear && std::isfinite(distance) ? distance : kFar;
}
}

GalleryScene::GalleryScene(bool seedWorkshop) {
  objects_.reserve(kGalleryObjectCapacity);
  frame_.vertices.reserve(kSceneVertexCapacity);
  frame_.indices.reserve(kSceneIndexCapacity);
  frame_.draws.reserve(kGalleryObjectCapacity+128);
  camera_={{0,1.4F,10},0,-6};
  if(!seedWorkshop)return;
  const std::array<Vec3,3> positions{{{-3,2,-3},{0,2,-4},{3,2,-5}}};
  const std::array<Vec3,3> colors{{{0.12F,0.70F,0.72F},{0.60F,0.40F,0.85F},{0.95F,0.58F,0.20F}}};
  for (std::size_t i=0;i<3;++i) {
    GalleryObject o;
    o.id={nextId_++}; o.origin=o.position=positions[i]; o.size={1.5F,1.5F,0.6F}; o.color=colors[i];
    o.route.spec.points={{{{0,0,0},0.2,1},{{0,1.6F,0},0.2,1},{{1,1.6F,-1},0.2,1},{{1,0,-1},0.2,1}}};
    o.route.spec.pointCount=4;
    o.route.spec.settings.speedMetersPerSecond=0.7;
    o.route.spec.settings.startsActive=i!=1;
    if (!rebuildRoute(o)) throw std::runtime_error("invalid gallery seed route");
    objects_.push_back(std::move(o));
  }
}
const GalleryObject* GalleryScene::selected() const {
  const auto it=std::find_if(objects_.begin(),objects_.end(),[&](const auto& o){return o.id==selected_;});
  return it==objects_.end() ? nullptr : &*it;
}
bool GalleryScene::rebuildRoute(GalleryObject& o) {
  if(!prepareRoute(o.route.spec,o.origin,o.route).accepted)return false;
  o.motion=initialMotion(o.route);o.preview.anchor=o.motion;o.position=o.motion.position;
  return true;
}
void GalleryScene::setViewport(SceneViewport viewport) {
  if (!std::isfinite(viewport.x) || !std::isfinite(viewport.y) ||
      !std::isfinite(viewport.width) || !std::isfinite(viewport.height) ||
      viewport.x<0 || viewport.y<0 || viewport.width<1 || viewport.height<1)
    throw std::invalid_argument("invalid gallery viewport");
  if (viewport!=frame_.viewport) presented_=false;
  frame_.viewport=viewport;
}
SceneHit GalleryScene::hitTestPresentedFrame(SceneFrameId frameId,float u,float v) const {
  if (!presented_ || frameId!=frame_.id) return {false,"stale_scene_frame"};
  if (!std::isfinite(u) || !std::isfinite(v) || u<0 || u>=1 || v<0 || v>=1)
    return {false,"outside_gallery"};
  const float tangent=std::tan(kFov*0.5F);
  const Ray3 ray{frame_.eye,normalized(frame_.forward + frame_.right*((2*u-1)*tangent*frame_.viewport.width/frame_.viewport.height) + frame_.up*((1-2*v)*tangent))};
  float closest=kFar;
  SceneObjectId id;
  VisualPhase phase=VisualPhase::Retired;
  for (const auto& draw:frame_.draws) {
    if (!intersectsRay(draw.bounds,ray,closest).hit) continue;
    for (std::size_t i=draw.firstIndex;i<draw.firstIndex+draw.indexCount;i+=3) {
      const float d=triangleHit(ray,vertex(frame_.vertices[frame_.indices[i]]),
        vertex(frame_.vertices[frame_.indices[i+1]]),vertex(frame_.vertices[frame_.indices[i+2]]));
      if (d<closest) { closest=d; id=draw.objectId; phase=draw.phase; }
    }
  }
  return {true,id.value ? "object_hit" : "no_object_hit",id,closest,phase};
}
GalleryResult GalleryScene::dispatch(const ResetTargets& action) {
  if(action.colours.size()<2 || action.colours.size()>8 ||
     (!action.route && action.colours.size()!=objects_.size()))return {false,"invalid_target_count"};
  auto nextId=nextId_;
  std::vector<GalleryObject> candidate;
  candidate.reserve(kGalleryObjectCapacity);
  for(std::size_t i=0;i<action.colours.size();++i) {
    const auto colour=action.colours[i];
    if(!bounded(colour,1) || colour.x<0 || colour.y<0 || colour.z<0)return {false,"invalid_target_colour"};
    GalleryObject object;
    if(i<objects_.size())object=objects_[i];
    else object.id={nextId++};
    object.primitive=GalleryPrimitive::Sphere;
    object.color=colour;object.size={1.2F,1.2F,1.2F};
    object.phase=action.animateSpawn?VisualPhase::Spawning:VisualPhase::Active;object.phaseStartTick=tickCount_;
    if(action.route) {
      const auto columns=(action.colours.size()+1)/2;
      const float spacing=columns<=2 ? 4.8F:2.6F;
      object.origin={(static_cast<float>(i%columns)-(columns-1)*0.5F)*spacing,i<columns?4.6F:2.0F,-4};
      object.route.spec=*action.route;
      object.route.spec.seed+=static_cast<std::uint32_t>(i);
      if(!rebuildRoute(object))return {false,"invalid_target_route"};
    }
    candidate.push_back(std::move(object));
  }
  objects_=std::move(candidate);nextId_=nextId;selected_={};presented_=false;
  return {true,"targets_reset"};
}
GalleryResult GalleryScene::dispatch(const GalleryAction& a) {
  if (!isFinite(a.value) || !std::isfinite(a.amount)) return {false,"nonfinite_input"};
  const auto done=[&]() { presented_=false; return GalleryResult{true,"applied"}; };
  const auto navigate=[&](ProductCreativeViewportNavigationOperation op) {
    if (paused_) return GalleryResult{false,"paused"};
    ProductCreativeViewportNavigationRequest r;
    r.pose=camera_; r.focus=focus_; r.operation=op;
    r.horizontalInput=a.value.x; r.verticalInput=a.value.y; r.viewportHeightPixels=frame_.viewport.height;
    const auto moved=applyProductCreativeViewportNavigation(r);
    if (!moved.applied || !bounded(moved.pose.anchorPositionMeters,100)) return GalleryResult{false,"navigation_rejected"};
    camera_=moved.pose;focus_=moved.focus;return done();
  };
  switch (a.kind) {
    case GalleryActionKind::Tick: {
      if (a.amount<0 || a.amount>0.25F) return {false,"invalid_tick_delta"};
      if (paused_) return {true,"paused"};
      accumulator_+=a.amount;
      while (accumulator_+1e-9>=1.0/60.0) {
        for (auto& o:objects_) {
          const auto step=advanceMotion(o.route,o.motion,o.motion.tick+1);
          if(!step.accepted) return {false,"route_step_failed"};
          o.position=o.motion.position;
          const auto age=tickCount_+1-o.phaseStartTick;
          if(o.phase==VisualPhase::Spawning && age>=kTargetSpawnTicks)o.phase=VisualPhase::Active;
          if(o.phase==VisualPhase::Popping && age>=kTargetPopTicks)o.phase=VisualPhase::Retired;
        }
        accumulator_-=1.0/60.0;++tickCount_;
      }
      return done();
    }
    case GalleryActionKind::Pause:
      if(!paused_ && a.amount!=0)for(auto& o:objects_)o.preview.anchor=o.motion;
      paused_=a.amount!=0;return done();
    case GalleryActionKind::ResetCamera: camera_={{0,1.4F,10},0,-6};focus_={};return done();
    case GalleryActionKind::FrameAll:
    case GalleryActionKind::FrameTargets: {
      ProductCreativeCameraFrameRequest r;
      r.boundsMinMeters={-8,0,-12};r.boundsMaxMeters={8,5,3};r.cameraYawDegrees=camera_.yawDegrees;
      if(a.kind==GalleryActionKind::FrameTargets) {
        if(objects_.empty())return {false,"no_targets_to_frame"};
        r.boundsMinMeters={100,100,100};r.boundsMaxMeters={-100,-100,-100};
        for(const auto& object:objects_) {
          if(object.primitive!=GalleryPrimitive::Sphere || object.route.spec.kind==RouteKind::Waypoints)
            return {false,"framing_requires_preset_targets"};
          const auto extent=object.size*.5F+object.route.spec.extent;
          const auto lo=object.origin-extent,hi=object.origin+extent;
          r.boundsMinMeters={std::min(r.boundsMinMeters.x,lo.x),std::min(r.boundsMinMeters.y,lo.y),std::min(r.boundsMinMeters.z,lo.z)};
          r.boundsMaxMeters={std::max(r.boundsMaxMeters.x,hi.x),std::max(r.boundsMaxMeters.y,hi.y),std::max(r.boundsMaxMeters.z,hi.z)};
        }
      }
      r.cameraPitchDegrees=camera_.pitchDegrees;r.viewportAspectRatio=frame_.viewport.width/frame_.viewport.height;
      const auto framed=planProductCreativeCameraFrame(r);
      if(!framed.applied)return {false,"frame_rejected"};
      camera_.anchorPositionMeters=framed.anchorPositionMeters;
      focus_=makeProductCreativeViewportFocus((r.boundsMinMeters+r.boundsMaxMeters)*0.5F,framed.distanceMeters);
      return done();
    }
    case GalleryActionKind::Fly: {
      if(paused_)return {false,"paused"};
      if(a.amount<=0 || a.amount>0.25F)return {false,"invalid_move_delta"};
      ProductCreativeFlyConfig config{true,6,3,a.amount};
      ProductCreativeFlyInput input{a.value.x,a.value.y,a.value.z,a.index!=0,camera_.yawDegrees,camera_.pitchDegrees};
      const auto move=applyProductCreativeFlyInput(config,input,camera_.anchorPositionMeters);
      if(!move.applied || !bounded(move.finalPositionMeters,100))return {false,"movement_rejected"};
      camera_.anchorPositionMeters=move.finalPositionMeters;focus_={};return done();
    }
    case GalleryActionKind::Look:
      if(paused_)return {false,"paused"};
      camera_.yawDegrees=std::remainder(camera_.yawDegrees+a.value.x,360.0F);
      camera_.pitchDegrees=std::clamp(camera_.pitchDegrees+a.value.y,-80.0F,80.0F);focus_={};return done();
    case GalleryActionKind::Orbit:return navigate(ProductCreativeViewportNavigationOperation::Orbit);
    case GalleryActionKind::Pan:return navigate(ProductCreativeViewportNavigationOperation::Pan);
    case GalleryActionKind::Dolly:return navigate(ProductCreativeViewportNavigationOperation::Dolly);
    case GalleryActionKind::Pick: {
      if(paused_)return {false,"paused"};
      const auto hit=hitTestPresentedFrame(frame_.id,a.value.x,a.value.y);
      if(!hit.accepted)return {false,hit.reason};
      selected_=hit.object;presented_=false;
      return {true,selected_.value?"object_selected":"no_object_hit"};
    }
    case GalleryActionKind::Select:
      if(a.index!=0 && std::none_of(objects_.begin(),objects_.end(),[&](const auto& o){return o.id.value==a.index;}))return {false,"unknown_object"};
      selected_={a.index};return done();
    case GalleryActionKind::Activate:
    case GalleryActionKind::Pop: {
      const auto target=std::find_if(objects_.begin(),objects_.end(),[&](const auto& o){return o.id.value==a.index;});
      if(target==objects_.end() || target->primitive!=GalleryPrimitive::Sphere)return {false,"unknown_target"};
      if(a.kind==GalleryActionKind::Pop && target->phase!=VisualPhase::Active)return {false,"target_not_active"};
      target->phase=a.kind==GalleryActionKind::Pop ? VisualPhase::Popping:VisualPhase::Spawning;
      target->phaseStartTick=tickCount_;return done();
    }
    case GalleryActionKind::AddBox:
    case GalleryActionKind::AddRamp:
    case GalleryActionKind::AddFrame:
    case GalleryActionKind::AddSphere: {
      if(objects_.size()>=kGalleryObjectCapacity)return {false,"object_capacity"};
      GalleryObject o;
      o.id={nextId_};
      o.primitive=a.kind==GalleryActionKind::AddBox ? GalleryPrimitive::Box :
        a.kind==GalleryActionKind::AddRamp ? GalleryPrimitive::Ramp :
        a.kind==GalleryActionKind::AddFrame ? GalleryPrimitive::Frame : GalleryPrimitive::Sphere;
      o.origin=camera_.anchorPositionMeters+Vec3{0,kProductCreativeCameraEyeHeightMeters,0}+forward(camera_)*7;
      if(!bounded(o.origin,100))return {false,"outside_scene_bounds"};
      o.size=o.primitive==GalleryPrimitive::Frame ? Vec3{3,3,0.5F}:Vec3{1.5F,1.5F,1.5F};
      o.route.spec.points={{{{0,0,0},0,1},{{3,0,0},0,1},{{3,0,-3},0,1},{{0,0,-3},0,1}}};
      o.route.spec.pointCount=4;o.route.spec.settings.startsActive=false;
      if(!rebuildRoute(o))return {false,"invalid_default_route"};
      selected_={nextId_++};objects_.push_back(std::move(o));return done();
    }
    default:break;
  }
  const auto it=std::find_if(objects_.begin(),objects_.end(),[&](const auto& o){return o.id==selected_;});
  if(it==objects_.end())return {false,"select_an_object"};
  GalleryObject candidate=*it;
  bool routeChanged=false;
  switch(a.kind) {
    case GalleryActionKind::Delete:objects_.erase(it);selected_={};return done();
    case GalleryActionKind::Position:
      if(!bounded(a.value,100))return {false,"invalid_position"};
      candidate.origin=a.value;routeChanged=true;break;
    case GalleryActionKind::Size:
      if(!bounded(a.value,20) || a.value.x<0.1F || a.value.y<0.1F || a.value.z<0.1F)return {false,"invalid_size"};
      if(candidate.primitive==GalleryPrimitive::Sphere && (a.value.x!=a.value.y || a.value.x!=a.value.z))return {false,"sphere_requires_equal_dimensions"};
      candidate.size=a.value;break;
    case GalleryActionKind::Yaw:candidate.yawDegrees=std::remainder(a.amount,360.0F);break;
    case GalleryActionKind::Color:
      if(!bounded(a.value,1) || a.value.x<0 || a.value.y<0 || a.value.z<0)return {false,"invalid_color"};
      candidate.color=a.value;break;
    case GalleryActionKind::Patrol:candidate.route.spec.settings.startsActive=a.amount!=0;routeChanged=true;break;
    case GalleryActionKind::Speed:candidate.route.spec.settings.speedMetersPerSecond=a.amount;routeChanged=true;break;
    case GalleryActionKind::Loop:candidate.route.spec.settings.traversalMode=a.amount!=0 ? motion::CreativeMovingPlatformTraversalMode::Loop:motion::CreativeMovingPlatformTraversalMode::PingPong;routeChanged=true;break;
    case GalleryActionKind::Route:
      if(a.index>=static_cast<std::uint32_t>(RouteKind::Count))return {false,"invalid_route_kind"};
      candidate.route.spec.kind=static_cast<RouteKind>(a.index);routeChanged=true;break;
    case GalleryActionKind::Extent:candidate.route.spec.extent=a.value;routeChanged=true;break;
    case GalleryActionKind::RouteDwell:candidate.route.spec.dwellSeconds=a.amount;routeChanged=true;break;
    case GalleryActionKind::RouteSeed:candidate.route.spec.seed=a.index;routeChanged=true;break;
    case GalleryActionKind::Waypoint:
      if(a.index>=candidate.route.spec.pointCount || !bounded(a.value,20))return {false,"invalid_waypoint"};
      candidate.route.spec.points[a.index].position=a.value;routeChanged=true;break;
    case GalleryActionKind::Dwell:
    case GalleryActionKind::SegmentSpeed:
      if(a.index>=candidate.route.spec.pointCount)return {false,"invalid_waypoint"};
      if(a.kind==GalleryActionKind::Dwell)candidate.route.spec.points[a.index].dwellSeconds=a.amount;
      else candidate.route.spec.points[a.index].outgoingSpeedMultiplier=a.amount;
      routeChanged=true;break;
    case GalleryActionKind::Scrub: {
      if(!paused_)return {false,"pause_before_scrubbing"};
      if(a.amount<0 || a.amount>1)return {false,"invalid_progress"};
      const auto requested=candidate.preview.anchor.tick+static_cast<std::uint64_t>(std::llround(a.amount*kMotionPreviewWindowTicks));
      const auto sample=seekMotion(candidate.route,requested,candidate.preview,candidate.motion);
      if(!sample.accepted)return {false,sample.reason};
      candidate.position=candidate.motion.position;break;
    }
    default:return {false,"invalid_action"};
  }
  if(routeChanged && !rebuildRoute(candidate))return {false,"invalid_route"};
  *it=std::move(candidate);return done();
}

const SceneFrame& GalleryScene::publishFrame() {
  cameraFrame(frame_,camera_);
  frame_.vertices.clear();frame_.indices.clear();frame_.draws.clear();
  const auto box=[&](Vec3 pos,Vec3 size,Vec3 color){objectMesh(frame_,{},GalleryPrimitive::Box,pos,size,color);};
  box({0,-0.15F,-4},{18,0.3F,20},{0.075F,0.105F,0.14F});
  box({0,3,-14},{18,6,0.2F},{0.11F,0.15F,0.20F});
  box({-9,3,-4},{0.2F,6,20},{0.09F,0.12F,0.17F});
  box({9,3,-4},{0.2F,6,20},{0.09F,0.12F,0.17F});
  // Parent floor/wall grid construction: thin box strips, kept in the mesh.
  for(int x=-8;x<=8;++x) {
    box({static_cast<float>(x),0.015F,-4},{0.025F,0.015F,20},{0.18F,0.26F,0.32F});
    box({static_cast<float>(x),3,-13.88F},{0.025F,6,0.025F},{0.20F,0.29F,0.36F});
  }
  for(int z=-13;z<=6;++z) box({0,0.015F,static_cast<float>(z)},{18,0.015F,0.025F},{0.18F,0.26F,0.32F});
  for(int y=1;y<=5;++y) box({0,static_cast<float>(y),-13.88F},{18,0.025F,0.025F},{0.20F,0.29F,0.36F});
  for(const auto& o:objects_) {
    const auto scale=visualScale(o,tickCount_);
    if(scale<=0)continue;
    const auto color=o.id==selected_ ? o.color*0.65F+Vec3{0.35F,0.35F,0.35F}:o.color;
    objectMesh(frame_,o.id,o.primitive,o.position,o.size*scale,color,o.yawDegrees,o.phase);
  }
  if(frame_.vertices.size()>kSceneVertexCapacity || frame_.indices.size()>kSceneIndexCapacity)
    throw std::runtime_error("gallery geometry capacity exceeded");
  frame_.id={nextFrameId_++};presented_=true;return frame_;
}
Vec3 GalleryScene::project(Vec3 world) const {
  const auto p=projectPoint(frame_.clipFromWorld,world);
  if(!p.finite || p.w<=0 || p.ndc.z<0 || p.ndc.z>1)return {-1,-1,-1};
  return {(p.ndc.x+1)*0.5F,(p.ndc.y+1)*0.5F,p.ndc.z};
}
}  // namespace paths
