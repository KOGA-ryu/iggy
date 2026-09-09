#include "scene/MotionScene.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace paths {
namespace {
using namespace iggy3d;
constexpr Vec3 cyan{.35F,.85F,1},gold{1,.78F,.3F},violet{.66F,.49F,.90F};
void box(SceneFrame& frame,SceneObjectId id,Vec3 center,Vec3 size,Vec3 color) {
  const auto first=frame.indices.size();
  constexpr std::array<Vec3,6> normals{{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}}};
  for(const auto n:normals) {
    const auto u=normalized(cross(std::abs(n.y)>.9F?Vec3{1,0,0}:Vec3{0,1,0},n)),v=cross(n,u);
    const auto base=frame.vertices.size();const auto tint=color*(.62F+.38F*std::max(0.0F,dot(n,normalized(Vec3{-.4F,.8F,.6F}))));
    for(const auto st:std::array<std::array<float,2>,4>{{{-1,-1},{1,-1},{1,1},{-1,1}}}) {
      const auto q=(n+u*st[0]+v*st[1])*.5F;
      const auto p=center+Vec3{q.x*size.x,q.y*size.y,q.z*size.z};
      frame.vertices.push_back({{p.x,p.y,p.z},{tint.x,tint.y,tint.z},{}});
    }
    for(unsigned i:{0,1,2,0,2,3})frame.indices.push_back(static_cast<std::uint16_t>(base+i));
  }
  frame.draws.push_back({id,first,frame.indices.size()-first,{center-size*.5F,center+size*.5F}});
}
void cart(SceneFrame& frame,float x,float z,Vec3 colour,std::uint32_t id,double displacement) {
  box(frame,{id},{x,.48F,z},{.85F,.32F,.55F},colour);
  box(frame,{id},{x-.1F,.72F,z},{.36F,.2F,.38F},colour*.75F);
  for(float offset:{-.27F,.27F})for(float side:{-.33F,.33F}) {
    const auto first=frame.indices.size(),base=frame.vertices.size();
    const Vec3 center{x+offset,.22F,z+side};
    // Circular wheels with alternating spokes rotate by travelled displacement.
    for(unsigned i=0;i<16;++i) {
      const float angle=static_cast<float>(i*6.283185307179586/16-displacement/.19);
      const Vec3 p=center+Vec3{std::cos(angle)*.19F,std::sin(angle)*.19F,0};
      const Vec3 tint=i%2?colour*.7F:Vec3{.12F,.15F,.2F};
      frame.vertices.push_back({{p.x,p.y,p.z},{tint.x,tint.y,tint.z},{}});
    }
    for(unsigned i=1;i<15;++i)for(unsigned n:{0U,i,i+1})frame.indices.push_back(static_cast<std::uint16_t>(base+n));
    frame.draws.push_back({{id},first,frame.indices.size()-first,{center-Vec3{.19F,.19F,.01F},center+Vec3{.19F,.19F,.01F}}});
  }
}
}
const SceneFrame& MotionScene::publish(const MotionLesson& lesson,SceneViewport viewport) {
  if(viewport.width<=0 || viewport.height<=0)throw std::invalid_argument("Invalid motion viewport");
  frame_.viewport=viewport;frame_.vertices.clear();frame_.indices.clear();frame_.draws.clear();
  frame_.vertices.reserve(kSceneVertexCapacity);frame_.indices.reserve(kSceneIndexCapacity);
  const auto selected=lesson.progress().selected;const auto& c=lesson.chapter();
  minimum_=c.start;maximum_=c.start;
  const auto include=[&](double x){minimum_=std::min(minimum_,x);maximum_=std::max(maximum_,x);};
  for(const auto& goal:c.goals)if(goal.measure==MotionMeasure::Position)include(goal.target);
  const auto range=[&](const MotionPlan& plan) {
    for(unsigned i=0;i<=64;++i)include(sampleMotion(selected,plan,c.duration*i/64).position);
  };
  range(lesson.shownPlan());if(lesson.ghost())range(lesson.ghost()->plan);
  minimum_=std::floor(minimum_)-1;maximum_=std::ceil(maximum_)+1;
  const double width=maximum_-minimum_;const float center=static_cast<float>((minimum_+maximum_)*.5);
  box(frame_,{1},{center,-.11F,0},{static_cast<float>(width),.16F,2.4F},{.07F,.10F,.14F});
  for(float z:{-.32F,.32F,.9F})box(frame_,{2},{center,0,z},{static_cast<float>(width),.07F,.045F},{.34F,.42F,.49F});
  for(int x=static_cast<int>(minimum_);x<=static_cast<int>(maximum_);++x)
    box(frame_,{3},{static_cast<float>(x),.015F,.6F},{.025F,.03F,1.5F},x==0?gold:Vec3{.19F,.27F,.34F});
  for(const auto& goal:c.goals)if(goal.measure==MotionMeasure::Position) {
    const auto colour=goal.time==c.duration?gold:Vec3{.61F,.67F,.42F};
    box(frame_,{10},{static_cast<float>(goal.target),.02F,0},{.06F,.045F,1.6F},colour);
    box(frame_,{10},{static_cast<float>(goal.target),.62F,-.72F},{.035F,1.2F,.035F},colour);
    box(frame_,{10},{static_cast<float>(goal.target)+.15F,1.13F,-.72F},{.3F,.18F,.04F},colour);
  }
  const auto sample=lesson.sample();cart(frame_,static_cast<float>(sample.position),0,cyan,100,sample.displacement);
  if(const auto* ghost=lesson.ghost()) {
    const auto s=sampleMotion(selected,ghost->plan,lesson.progress().time);
    cart(frame_,static_cast<float>(s.position),1.05F,violet,200,s.displacement);
  }
  iggy3d::ProductCreativeCameraFrameRequest request;
  request.boundsMinMeters={static_cast<float>(minimum_),0,-1.2F};request.boundsMaxMeters={static_cast<float>(maximum_),1.35F,1.5F};
  request.cameraYawDegrees=0;request.cameraPitchDegrees=-25;
  request.viewportAspectRatio=viewport.width/viewport.height;
  const auto camera=iggy3d::planProductCreativeCameraFrame(request);
  if(!camera.applied)throw std::runtime_error("Motion camera framing rejected");
  publishSceneCamera(frame_,{camera.anchorPositionMeters,0,-25});++frame_.id.value;
  if(frame_.vertices.size()>kSceneVertexCapacity || frame_.indices.size()>kSceneIndexCapacity)throw std::runtime_error("Motion geometry exceeds renderer capacity");
  return frame_;
}
iggy3d::Vec3 MotionScene::project(iggy3d::Vec3 world) const {
  const auto p=iggy3d::projectPoint(frame_.clipFromWorld,world);
  if(!p.finite || p.w<=0 || p.ndc.z<0 || p.ndc.z>1)return {-1,-1,-1};
  return {frame_.viewport.x+(p.ndc.x+1)*.5F*frame_.viewport.width,frame_.viewport.y+(p.ndc.y+1)*.5F*frame_.viewport.height,p.ndc.z};
}
} // namespace paths
