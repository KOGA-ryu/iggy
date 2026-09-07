#include "runtime/math_objects/MathObjects.hpp"
#include "scene/MathObjectScene.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <set>
#include <stdexcept>

namespace {
using namespace paths;
constexpr double pi=3.14159265358979323846;
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
void near(double actual,double expected,double epsilon,const char* message){require(std::fabs(actual-expected)<=epsilon,message);}
void select(MathObjects& m,MathObjectKind kind){require(m.dispatch({MathActionKind::Select,kind}).accepted,"select rejected");}
void set(MathObjects& m,MathParameter parameter,double value){require(m.dispatch({MathActionKind::SetParameter,{},parameter,value}).accepted,"parameter rejected");}
void checked(MathObjects& m,bool solved){require(m.dispatch({MathActionKind::Check}).accepted,"check rejected");require((m.snapshot().feedback==MathFeedback::Solved)==solved,"wrong challenge result");}
iggy3d::Vec3 vertex(const SceneVertex& v){return {v.position[0],v.position[1],v.position[2]};}
void inspect(const MathObjectSnapshot& snapshot,MathObjectScene& scene,std::size_t& maxVertices,std::size_t& maxIndices) {
  const auto& frame=scene.publish(snapshot,{0,0,1000,700});
  require(!frame.vertices.empty()&&!frame.indices.empty(),"empty native object");
  require(frame.vertices.size()<=kSceneVertexCapacity&&frame.indices.size()<=kSceneIndexCapacity,"native capacity exceeded");
  require(iggy3d::isFinite(frame.clipFromWorld),"invalid camera matrix");
  maxVertices=std::max(maxVertices,frame.vertices.size());maxIndices=std::max(maxIndices,frame.indices.size());
  std::set<std::uint32_t> ids;std::size_t covered=0;
  for(const auto& draw:frame.draws){require(ids.insert(draw.objectId.value).second,"duplicate part ID");require(draw.firstIndex==covered&&draw.indexCount%3==0,"invalid draw range");covered+=draw.indexCount;}
  require(covered==frame.indices.size(),"unowned triangle range");
  for(const auto& v:frame.vertices){require(iggy3d::isFinite(vertex(v)),"nonfinite vertex");for(float color:v.color)require(std::isfinite(color)&&color>=0&&color<=1,"invalid vertex colour");}
  for(auto i:frame.indices)require(i<frame.vertices.size(),"index outside vertex buffer");
}
}
int main() {
  try {
    MathObjects m;MathObjectScene scene;std::size_t maxVertices=0,maxIndices=0;
    for(double x:{.5,1.0,2.0,2.5})for(double gap:{0.0,.6}) {
      set(m,MathParameter::X,x);set(m,MathParameter::Gap,gap);
      double sum=0;for(std::size_t i=0;i<m.snapshot().partCount;++i){const auto& p=m.snapshot().parts[i];sum+=iggy3d::dot(p.x,iggy3d::cross(p.y,p.z));}
      near(sum,std::pow(x+1,3),1e-5,"cube partition does not conserve volume");inspect(m.snapshot(),scene,maxVertices,maxIndices);
    }
    set(m,MathParameter::X,2);checked(m,false);set(m,MathParameter::Gap,0);checked(m,true);
    const auto& cube=scene.publish(m.snapshot(),{0,0,1000,700});double signedVolume=0;
    for(std::size_t i=0;i<cube.indices.size();i+=3)signedVolume+=iggy3d::dot(vertex(cube.vertices[cube.indices[i]]),iggy3d::cross(vertex(cube.vertices[cube.indices[i+1]]),vertex(cube.vertices[cube.indices[i+2]])))/6.0;
    near(signedVolume,27,1e-5,"native cube winding or volume is wrong");
    const auto revision=m.snapshot().revision;
    require(!m.dispatch({MathActionKind::SetParameter,{},MathParameter::X,std::numeric_limits<double>::quiet_NaN()}).accepted,"accepted NaN");
    require(!m.dispatch({MathActionKind::SetParameter,{},MathParameter::X,0}).accepted,"accepted negative/zero length");
    require(!m.dispatch({MathActionKind::SetParameter,{},MathParameter::Scale,0}).accepted,"accepted another object's parameter");
    require(m.snapshot().revision==revision&&m.parameter(MathParameter::X)==2,"rejection changed state");

    select(m,MathObjectKind::Trig);
    for(double angle:{0,45,90,150,180,270,360}) {
      set(m,MathParameter::Angle,angle);const auto& s=m.snapshot();
      near(s.metrics[0].value*s.metrics[0].value+s.metrics[1].value*s.metrics[1].value,1,1e-12,"unit circle identity failed");
      inspect(s,scene,maxVertices,maxIndices);
    }
    checked(m,false);set(m,MathParameter::Angle,150);checked(m,true);

    select(m,MathObjectKind::Calculus);
    for(double n:{3,8,32,64})for(double sample:{0,1,2})for(double gap:{0.0,.12}) {
      set(m,MathParameter::Slices,n);set(m,MathParameter::Sample,sample);set(m,MathParameter::SliceGap,gap);
      const auto& s=m.snapshot();const double exact=8*pi/3,estimate=s.metrics[0].value;
      if(sample==0)require(estimate<exact,"left sum not below cone volume");
      if(sample==2)require(estimate>exact,"right sum not above cone volume");
      if(sample==1)near(s.metrics[2].value,25/(n*n),1e-10,"midpoint convergence incorrect");
      for(std::size_t i=0;i<s.partCount;++i)require(std::fabs(s.parts[i].center.x)<20,"exploded slice overflow");
      inspect(s,scene,maxVertices,maxIndices);
    }
    set(m,MathParameter::Sample,1);set(m,MathParameter::Slices,32);checked(m,true);

    select(m,MathObjectKind::Linear);
    for(double k:{-1.2,0.0,1.2})for(double scale:{-2,-1,0,1,2}) {
      set(m,MathParameter::Shear,k);set(m,MathParameter::Scale,scale);
      const auto& s=m.snapshot();near(s.metrics[0].value,scale,1e-12,"signed determinant wrong");near(s.metrics[1].value,std::fabs(scale),1e-12,"volume sign wrong");
      near(s.metrics[2].value,scale==0?2:3,0,"rank wrong");inspect(s,scene,maxVertices,maxIndices);
    }
    set(m,MathParameter::Scale,0);checked(m,true);

    select(m,MathObjectKind::Discrete);require(m.snapshot().shortestHops==3,"Q3 shortest path wrong");
    require(!m.dispatch({MathActionKind::VisitVertex,{},{},0,7}).accepted,"accepted missing graph edge");
    for(unsigned node:{1,3,7})require(m.dispatch({MathActionKind::VisitVertex,{},{},0,node}).accepted,"valid path rejected");checked(m,true);
    set(m,MathParameter::Depth,.3);require(m.snapshot().routeCount==4&&m.snapshot().shortestHops==3,"layout changed graph evidence");inspect(m.snapshot(),scene,maxVertices,maxIndices);
    set(m,MathParameter::Shortcut,1);require(m.snapshot().routeCount==1&&m.snapshot().shortestHops==1,"shortcut failed to reset path");
    require(m.dispatch({MathActionKind::VisitVertex,{},{},0,7}).accepted,"shortcut not traversable");checked(m,true);
    set(m,MathParameter::Depth,1.5);inspect(m.snapshot(),scene,maxVertices,maxIndices);
    require(m.dispatch({MathActionKind::ResetRoute}).accepted,"route reset failed");
    while(m.snapshot().routeCount<m.snapshot().route.size()) {const unsigned next=m.snapshot().route[m.snapshot().routeCount-1]==0?1:0;require(m.dispatch({MathActionKind::VisitVertex,{},{},0,next}).accepted,"bounded route failed");}
    require(!m.dispatch({MathActionKind::VisitVertex,{},{},0,0}).accepted,"route overflow accepted");
    scene.resetView();const auto old=scene.frame().clipFromWorld.m;
    require(scene.navigate(iggy3d::ProductCreativeViewportNavigationOperation::Orbit,30,20),"camera orbit failed");
    require(scene.publish(m.snapshot(),{0,0,1000,700}).clipFromWorld.m!=old,"camera input not published");
    std::printf("math objects passed: volume, trig, convergence, rank, graph, validation, geometry and camera; maxima %zu vertices / %zu indices\n",maxVertices,maxIndices);return 0;
  }catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}
}
