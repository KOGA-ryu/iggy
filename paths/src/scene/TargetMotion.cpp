#include "scene/TargetMotion.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

namespace paths {
using iggy3d::Vec3;
namespace m = iggy3d::creative;
namespace {
constexpr double pi = 3.14159265358979323846;
constexpr std::array descriptors{
  RouteDescriptor{RouteKind::Stationary,"stationary","Stationary"},
  RouteDescriptor{RouteKind::Horizontal,"horizontal","Horizontal shuttle"},
  RouteDescriptor{RouteKind::Vertical,"vertical","Vertical lift"},
  RouteDescriptor{RouteKind::DiagonalRebound,"rebound","Diagonal rebound"},
  RouteDescriptor{RouteKind::Circle,"circle","Circle"},
  RouteDescriptor{RouteKind::Ellipse,"ellipse","Ellipse"},
  RouteDescriptor{RouteKind::FigureEight,"figure_eight","Figure eight"},
  RouteDescriptor{RouteKind::Sine,"sine","Sine wave"},
  RouteDescriptor{RouteKind::Zigzag,"zigzag","Zigzag"},
  RouteDescriptor{RouteKind::Box,"box","Box patrol"},
  RouteDescriptor{RouteKind::StopAndGo,"stop_go","Stop and go"},
  RouteDescriptor{RouteKind::BreathingSpiral,"spiral","Breathing spiral"},
  RouteDescriptor{RouteKind::SeededRoam,"roam","Seeded roam"},
  RouteDescriptor{RouteKind::Waypoints,"waypoints","Custom waypoints"},
};
bool bounded(Vec3 v,float limit) {
  return iggy3d::isFinite(v) && std::fabs(v.x)<=limit &&
    std::fabs(v.y)<=limit && std::fabs(v.z)<=limit;
}
Vec3 curvePosition(const PreparedRoute& route,std::uint64_t tick) {
  const auto& s=route.spec;
  // Pace is nominal metres/second at extent.x. Noncircular curves have
  // varying instantaneous speed; their phase is entirely fixed-tick based.
  const double t=std::fmod(static_cast<double>(tick)/kGalleryTickRate *
                         s.settings.speedMetersPerSecond/s.extent.x,2*pi);
  const float sn=static_cast<float>(std::sin(t)), cs=static_cast<float>(std::cos(t));
  Vec3 p{};
  switch(s.kind) {
    case RouteKind::Circle:p={sn*s.extent.x,(cs-1)*s.extent.x,0};break;
    case RouteKind::Ellipse:p={sn*s.extent.x,(cs-1)*s.extent.y,0};break;
    case RouteKind::FigureEight:p={sn*s.extent.x,static_cast<float>(std::sin(2*t))*s.extent.y,0};break;
    case RouteKind::Sine:p={sn*s.extent.x,static_cast<float>(std::sin(3*t))*s.extent.y,0};break;
    case RouteKind::BreathingSpiral: {
      const float radius=0.65F+0.35F*static_cast<float>(std::cos(3*t));
      p={radius*sn*s.extent.x,(radius*cs-1)*s.extent.y,0};break;
    }
    case RouteKind::DiagonalRebound: {
      const auto reflect=[](double phase) {return static_cast<float>(2/pi*std::asin(std::sin(phase)));};
      // Independent rational frequencies form a repeatable bounded patrol.
      p={reflect(t)*s.extent.x,reflect(2*t)*s.extent.y,reflect(3*t)*s.extent.z};break;
    }
    default:break;
  }
  return route.origin+p;
}
}
std::span<const RouteDescriptor> targetRouteDescriptors() {return descriptors;}

MotionResult prepareRoute(const RouteSpec& spec,Vec3 origin,PreparedRoute& output) {
  if(spec.kind>=RouteKind::Count || !bounded(origin,100) ||
     !bounded(spec.extent,20) || spec.extent.x<0.05F || spec.extent.y<0.05F || spec.extent.z<0 ||
     !std::isfinite(spec.settings.speedMetersPerSecond) || spec.settings.speedMetersPerSecond<=0 ||
     spec.settings.speedMetersPerSecond>100 ||
     spec.settings.traversalMode>=m::CreativeMovingPlatformTraversalMode::Count ||
     !std::isfinite(spec.dwellSeconds) || spec.dwellSeconds<0 || spec.dwellSeconds>60 ||
     spec.pointCount>spec.points.size())return {false,"invalid_route_parameters"};
  PreparedRoute next;
  next.spec=spec;next.origin=origin;
  auto points=spec.points;
  std::size_t count=0;
  auto settings=spec.settings;
  const float x=spec.extent.x, y=spec.extent.y;
  const auto point=[&](Vec3 p,double dwell=0) {points[count++]={p,dwell,1};};
  switch(spec.kind) {
    case RouteKind::Horizontal:point({0,0,0});point({x,0,0});point({-x,0,0});break;
    case RouteKind::Vertical:point({0,0,0});point({0,y,0});point({0,-y,0});break;
    case RouteKind::Box:
      point({0,0,0});point({x,0,0});point({x,y,0});point({0,y,0});point({0,0,0});break;
    case RouteKind::Zigzag:
      point({0,0,0});point({x*0.5F,y,0});point({x,0,0});point({x*0.5F,-y,0});break;
    case RouteKind::StopAndGo:
      point({0,0,0},spec.dwellSeconds);point({x,0,0},spec.dwellSeconds);
      point({-x,0,0},spec.dwellSeconds);break;
    case RouteKind::SeededRoam: {
      std::mt19937 engine(spec.seed);
      const auto unit=[&] {return static_cast<float>(static_cast<double>(engine())/std::mt19937::max()*2-1);};
      point({0,0,0});
      for(int i=0;i<7;++i)point({unit()*x,unit()*y,unit()*spec.extent.z},spec.dwellSeconds);
      break;
    }
    case RouteKind::Waypoints:count=spec.pointCount;break;
    case RouteKind::Stationary:case RouteKind::DiagonalRebound:case RouteKind::Circle:
    case RouteKind::Ellipse:case RouteKind::FigureEight:case RouteKind::Sine:
    case RouteKind::BreathingSpiral:break;
    case RouteKind::Count:return {false,"invalid_route_kind"};
  }
  next.usesWaypoints=count!=0 || spec.kind==RouteKind::Waypoints;
  if(next.usesWaypoints) {
    for(std::size_t i=0;i<count;++i)
      if(!bounded(points[i].position,20))return {false,"invalid_waypoint_bounds"};
    const auto result=m::buildCreativeRuntimeMovingPlatformDefinition(
      std::span<const m::CreativePathPoint>(points.data(),count),settings,origin);
    if(!result.ok)return {false,result.reasonCode};
    next.waypoint=result.definition;
  }
  next.ready=true;output=next;return {true,"route_prepared"};
}
MotionState initialMotion(const PreparedRoute& route) {
  MotionState state;
  state.position=route.origin;
  if(route.usesWaypoints)state.waypoint=m::sampleCreativeRuntimeMovingPlatformProgress(route.waypoint,0).state;
  return state;
}
MotionResult advanceMotion(const PreparedRoute& route,MotionState& state,std::uint64_t nextTick) {
  if(!route.ready || state.tick==std::numeric_limits<std::uint64_t>::max() || nextTick!=state.tick+1)
    return {false,"invalid_motion_tick"};
  auto next=state;
  if(route.spec.settings.startsActive) {
    if(route.usesWaypoints) {
      const auto step=m::planCreativeRuntimeMovingPlatformStep({&route.waypoint,&state.waypoint,kGalleryTickRate,true});
      if(!step.ok)return {false,step.reasonCode};
      next.waypoint=step.nextState;next.position=next.waypoint.positionMeters;
    } else next.position=curvePosition(route,nextTick);
  }
  next.tick=nextTick;state=next;return {true,"motion_advanced"};
}
MotionResult seekMotion(const PreparedRoute& route,std::uint64_t tick,
                        const MotionPreview& preview,MotionState& output) {
  if(!route.ready || tick<preview.anchor.tick || tick-preview.anchor.tick>kMotionPreviewWindowTicks)
    return {false,"outside_preview_window"};
  auto next=preview.anchor;
  if(route.usesWaypoints) {
    while(next.tick<tick) {
      const auto result=advanceMotion(route,next,next.tick+1);
      if(!result.accepted)return result;
    }
  } else {
    next.tick=tick;
    if(route.spec.settings.startsActive)next.position=curvePosition(route,tick);
  }
  output=next;return {true,"motion_previewed"};
}
}  // namespace paths
