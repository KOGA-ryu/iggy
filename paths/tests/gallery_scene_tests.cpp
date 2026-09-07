#include "scene/GalleryScene.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
using namespace paths;
using Kind=GalleryActionKind;
int failures=0;
void expect(bool condition,std::string_view message) {
  if(!condition) {++failures;std::cerr<<"FAIL: "<<message<<'\n';}
}
void apply(GalleryScene& scene,GalleryAction action) {
  const auto r=scene.dispatch(action);
  expect(r.accepted,r.reason);
}
void publish(GalleryScene& scene) {static_cast<void>(scene.publishFrame());}
void pickWorld(GalleryScene& scene,iggy3d::Vec3 world) {
  publish(scene);
  const auto point=scene.project(world);
  apply(scene,{Kind::Pick,{point.x,point.y,0}});
}
void testCameraAndPicking() {
  GalleryScene s;
  s.setViewport({0,78,1030,822});publish(s);
  const auto original=s.camera().anchorPositionMeters;
  apply(s,{Kind::Fly,{0,1,0},0.25F});
  expect(iggy3d::nearlyEqual(s.camera().anchorPositionMeters,original+iggy3d::Vec3{0,0,-1.5F}),"ported fly moves forward in world meters");
  expect(!s.dispatch({Kind::Pick,{0.5F,0.5F,0}}).accepted,"unpublished camera change rejects stale-frame pick");
  apply(s,{Kind::ResetCamera});
  apply(s,{Kind::Fly,{1,1,1},0.25F});
  expect(std::fabs(iggy3d::length(s.camera().anchorPositionMeters-original)-1.5F)<1e-4F,"diagonal movement is normalized");
  apply(s,{Kind::ResetCamera});
  pickWorld(s,{0,2,-4});
  expect(s.selectedId()==SceneObjectId{2},"projected target centre selects the visible box");
  expect(!s.dispatch({Kind::Pick,{1.1F,0.5F,0}}).accepted && s.selectedId()==SceneObjectId{2},"question/control panel cannot select through gallery");
  apply(s,{Kind::Look,{15,-5,0}});
  apply(s,{Kind::Pan,{15,8,0}});
  apply(s,{Kind::Orbit,{25,10,0}});
  apply(s,{Kind::Dolly,{0,1,0}});
  pickWorld(s,{0,2,-4});
  expect(s.selectedId()==SceneObjectId{2},"camera navigation, projection, and picking share a basis");
  const auto ticks=s.tickCount();publish(s);publish(s);
  expect(s.tickCount()==ticks,"drawing never advances simulation");
}
void testGeometryAndOcclusion() {
  GalleryScene s;s.setViewport({0,78,1030,822});
  apply(s,{Kind::AddFrame});
  const auto frameId=s.selectedId();
  apply(s,{Kind::Position,{0,2,0}});apply(s,{Kind::Size,{4,4,0.5F}});
  pickWorld(s,{0,2,0});
  expect(s.selectedId()==SceneObjectId{2},"frame opening does not select the enclosing AABB");
  pickWorld(s,{1.6F,2,0});
  expect(s.selectedId()==frameId,"frame pier selects the actual mesh");
  apply(s,{Kind::AddBox});
  const auto nearId=s.selectedId();
  apply(s,{Kind::Position,{0,2,1}});apply(s,{Kind::Size,{2,2,1}});apply(s,{Kind::Yaw,{},35});
  pickWorld(s,{0,2,1});
  expect(s.selectedId()==nearId,"nearest rotated surface wins over objects behind it");
  apply(s,{Kind::Delete});
  expect(s.selectedId()==SceneObjectId{},"deleting selection clears its stable ID");
  apply(s,{Kind::AddRamp});
  const auto rampId=s.selectedId();
  apply(s,{Kind::Position,{0,2,1}});apply(s,{Kind::Size,{2,2,2}});
  pickWorld(s,{0,1.3F,1});
  expect(s.selectedId()==rampId,"ramp mesh remains selectable");
  publish(s);
  for(auto i:s.frame().indices)expect(i<s.frame().vertices.size(),"all mesh indices have vertices");
  expect(s.frame().indices.size()%3==0,"mesh is complete triangles");
}
void testMotionAndPause() {
  GalleryScene a,b;
  for(int i=0;i<60;++i)apply(a,{Kind::Tick,{},1.0F/60});
  for(int i=0;i<120;++i)apply(b,{Kind::Tick,{},1.0F/120});
  expect(a.tickCount()==60 && b.tickCount()==60,"render rate does not change fixed tick count");
  for(std::size_t i=0;i<a.objects().size();++i)
    expect(iggy3d::nearlyEqual(a.objects()[i].position,b.objects()[i].position),"same elapsed time yields same patrol positions");
  const auto position=a.objects()[0].position;
  apply(a,{Kind::Pause,{},1});
  for(int i=0;i<4;++i)apply(a,{Kind::Tick,{},0.25F});
  expect(a.tickCount()==60 && iggy3d::nearlyEqual(a.objects()[0].position,position),"pause freezes time and patrols");
  publish(a);
  expect(!a.dispatch({Kind::Pick,{0.5F,0.5F,0}}).accepted,"paused gallery blocks pointer selection");
  expect(!a.dispatch({Kind::Fly,{0,1,0},0.1F}).accepted,"paused gallery blocks camera movement");
  apply(a,{Kind::Select,{},0,1});
  apply(a,{Kind::Speed,{},2});apply(a,{Kind::Loop,{},1});
  apply(a,{Kind::Waypoint,{4,0,0},0,1});apply(a,{Kind::Dwell,{},0.5F,1});
  apply(a,{Kind::SegmentSpeed,{},2,1});apply(a,{Kind::Scrub,{},0.5F});
  const auto scrubbed=a.selected()->position;
  expect(!iggy3d::nearlyEqual(scrubbed,a.selected()->origin),"scrub samples an edited route without ticking");
  expect(a.tickCount()==60,"route editing and scrubbing do not consume active time");
  apply(a,{Kind::Pause,{},0});apply(a,{Kind::Tick,{},0.1F});
  expect(a.tickCount()==66,"resume continues fixed simulation ticks");
  expect(!a.dispatch({Kind::Scrub,{},0.2F}).accepted,"running patrol rejects timeline scrubbing");
}
void testAtomicRejectionAndCapacity() {
  GalleryScene s;apply(s,{Kind::Select,{},0,2});
  const auto original=s.selected()->origin;
  expect(!s.dispatch({Kind::Position,{std::numeric_limits<float>::infinity(),0,0}}).accepted,"nonfinite position rejected");
  expect(!s.dispatch({Kind::Size,{1,0,1}}).accepted,"zero-size shape rejected");
  expect(!s.dispatch({Kind::Speed,{},-1}).accepted,"negative route speed rejected");
  expect(!s.dispatch({Kind::Dwell,{},61,1}).accepted,"route wait beyond parent limit rejected");
  expect(iggy3d::nearlyEqual(s.selected()->origin,original) && s.selected()->route.spec.settings.speedMetersPerSecond==0.7,"rejected edits preserve the original object");
  expect(!s.dispatch({Kind::Tick,{},1}).accepted && s.tickCount()==0,"oversize tick rejected before mutation");
  while(s.objects().size()<kGalleryObjectCapacity)apply(s,{Kind::AddFrame});
  expect(!s.dispatch({Kind::AddBox}).accepted,"object budget enforced before allocation");
  publish(s);
  expect(s.frame().vertices.size()<=kSceneVertexCapacity && s.frame().indices.size()<=kSceneIndexCapacity,"maximum frame scene fits native buffer budgets");
}
void testWaypointArrivalAndReverseSpeed() {
  namespace m=iggy3d::creative;
  std::array<m::CreativePathPoint,2> points{{{{0,0,0},0,1},{{1,0,0},0.5,2}}};
  m::CreativeMovingPlatformSettings settings{1,m::CreativeMovingPlatformTraversalMode::PingPong,true};
  const auto built=m::buildCreativeRuntimeMovingPlatformDefinition(points,settings,{0,0,0});
  expect(built.ok,"ported route builds without a Creative world");
  auto state=m::sampleCreativeRuntimeMovingPlatformProgress(built.definition,0).state;
  const auto step=[&](const auto& definition) {
    const auto result=m::planCreativeRuntimeMovingPlatformStep({&definition,&state,4,true});
    expect(result.ok,"ported fixed-tick planner accepts valid state");
    state=result.nextState;
  };
  for(int i=0;i<4;++i)step(built.definition);
  expect(iggy3d::nearlyEqual(state.positionMeters,{1,0,0}) && state.dwellTicksRemaining==2,"arrival schedules the authored half-second dwell");
  step(built.definition);step(built.definition);
  expect(iggy3d::nearlyEqual(state.positionMeters,{1,0,0}),"dwell consumes ticks without displacement");
  step(built.definition);
  expect(iggy3d::nearlyEqual(state.positionMeters,{0.75F,0,0}),"ping-pong reverse retains the physical segment speed");
  points[1].dwellSeconds=0;settings.traversalMode=m::CreativeMovingPlatformTraversalMode::Loop;
  const auto loop=m::buildCreativeRuntimeMovingPlatformDefinition(points,settings,{0,0,0});
  expect(loop.ok,"loop route builds");
  state=m::sampleCreativeRuntimeMovingPlatformProgress(loop.definition,0).state;
  for(int i=0;i<6;++i)step(loop.definition);
  expect(iggy3d::nearlyEqual(state.positionMeters,{0,0,0}),"closing segment applies its own speed multiplier");
}
void testSphereLifecycleAndReadOnlyHit() {
  GalleryScene scene(false);scene.setViewport({0,78,1030,822});
  apply(scene,{Kind::AddSphere});
  const auto id=scene.selectedId();
  apply(scene,{Kind::Position,{0,2,0}});apply(scene,{Kind::Size,{4,4,4}});
  publish(scene);const auto firstFrame=scene.frame().id;
  const auto centre=scene.project({0,2,0});
  const auto selected=scene.selectedId();
  const auto hit=scene.hitTestPresentedFrame(firstFrame,centre.x,centre.y);
  expect(hit.accepted && hit.object==id && hit.phase==VisualPhase::Active,"shared mesh query hits a sphere");
  expect(scene.selectedId()==selected,"read-only hit query cannot change selection");
  const auto corner=scene.project({1.8F,3.8F,0});
  expect(scene.hitTestPresentedFrame(firstFrame,corner.x,corner.y).object!=id,"sphere AABB corner is not an answer hit");
  publish(scene);
  expect(!scene.hitTestPresentedFrame(firstFrame,centre.x,centre.y).accepted,"older frame identity is rejected even without camera movement");
  apply(scene,{Kind::Pop,{},0,id.value});publish(scene);
  expect(scene.hitTestPresentedFrame(scene.frame().id,centre.x,centre.y).phase==VisualPhase::Popping,"visible popping surface still occludes");
  expect(!scene.dispatch({Kind::Pop,{},0,id.value}).accepted,"a pop cannot be submitted twice");
  apply(scene,{Kind::Pause,{},1});apply(scene,{Kind::Tick,{},0.25F});
  expect(scene.objects()[0].phase==VisualPhase::Popping && scene.tickCount()==0,"pause freezes pop age");
  apply(scene,{Kind::Pause,{},0});apply(scene,{Kind::Tick,{},0.15F});publish(scene);
  const auto& draw=scene.frame().draws.back();
  expect(std::fabs((draw.bounds.max.x-draw.bounds.min.x)-2)<0.001F,"half-aged pop shrinks the visible diameter by half");
  apply(scene,{Kind::Tick,{},0.15F});publish(scene);
  expect(scene.objects()[0].phase==VisualPhase::Retired,"pop retires after eighteen common ticks");
  expect(scene.hitTestPresentedFrame(scene.frame().id,centre.x,centre.y).object!=id,"retired sphere has no pick geometry");
  apply(scene,{Kind::Activate,{},0,id.value});
  expect(scene.objects()[0].phase==VisualPhase::Spawning,"reset uses the spawn phase");
  apply(scene,{Kind::Tick,{},0.15F});
  expect(scene.objects()[0].phase==VisualPhase::Active,"spawn activates at nine common ticks");
  while(scene.objects().size()<kGalleryObjectCapacity)apply(scene,{Kind::AddSphere});
  publish(scene);
  expect(scene.frame().vertices.size()<=kSceneVertexCapacity && scene.frame().indices.size()<=kSceneIndexCapacity,
    "sixty-four shared spheres fit the unchanged native buffer budgets");
}
void testCommonMotionAndPreview() {
  for(const auto descriptor:targetRouteDescriptors()) {
    RouteSpec spec;spec.kind=descriptor.kind;
    spec.points={{{{0,0,0},0,1},{{1,0,0},0.5,2}}};spec.pointCount=2;
    PreparedRoute route;
    expect(prepareRoute(spec,{0,2,0},route).accepted,"every advertised route prepares");
    auto live=initialMotion(route);MotionPreview preview{live};
    for(std::uint64_t tick=1;tick<=237;++tick) {
      expect(advanceMotion(route,live,tick).accepted,"common advancement handles every route");
      if(tick==150)preview.anchor=live;
      expect(iggy3d::isFinite(live.position) && iggy3d::length(live.position-route.origin)<5,"route remains in its bounded region");
    }
    auto sought=initialMotion(route);
    expect(seekMotion(route,237,preview,sought).accepted && iggy3d::nearlyEqual(sought.position,live.position) &&
      sought.waypoint.dwellTicksRemaining==live.waypoint.dwellTicksRemaining &&
      sought.waypoint.phaseMeters==live.waypoint.phaseMeters,"time preview exactly matches continued live motion including dwell");
    const auto previous=sought;
    expect(!seekMotion(route,preview.anchor.tick+kMotionPreviewWindowTicks+1,preview,sought).accepted &&
      sought.tick==previous.tick,"out-of-window preview leaves state unchanged");
    const auto kind=route.spec.kind;
    spec.settings.speedMetersPerSecond=-1;
    expect(!prepareRoute(spec,{0,2,0},route).accepted && route.spec.kind==kind && route.ready,
      "failed preparation preserves the existing route");
  }
  RouteSpec spec;spec.kind=RouteKind::Waypoints;spec.pointCount=2;
  spec.points={{{{0,0,0},0,1},{{1,0,0},0.5,2}}};spec.settings.speedMetersPerSecond=1;
  PreparedRoute route;expect(prepareRoute(spec,{},route).accepted,"dwell fixture prepares");
  auto state=initialMotion(route);const MotionPreview preview{state};
  expect(seekMotion(route,70,preview,state).accepted && iggy3d::nearlyEqual(state.position,{1,0,0}) &&
    state.waypoint.dwellTicksRemaining==20,"seeking seventy ticks includes ten ticks of the half-second dwell");
  expect(seekMotion(route,105,preview,state).accepted && iggy3d::nearlyEqual(state.position,{0.75F,0,0}),
    "time seeking preserves physical segment speed on reversal");
}
}
int main() {
  testCameraAndPicking();testGeometryAndOcclusion();testMotionAndPause();testAtomicRejectionAndCapacity();testWaypointArrivalAndReverseSpeed();
  testSphereLifecycleAndReadOnlyHit();testCommonMotionAndPreview();
  if(failures) {std::cerr<<failures<<" scene assertions failed\n";return 1;}
  std::cout<<"paths_scene_tests: camera, visible picking, geometry, patrol, pause, and rejection passed\n";
  return 0;
}
