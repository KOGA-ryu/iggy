#include "scene/MathObjectScene.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace paths {
using namespace iggy3d;
namespace {
constexpr float pi=3.14159265358979323846F;
struct UnitVertex { Vec3 position, normal; };
struct UnitMesh { std::vector<UnitVertex> vertices;std::vector<std::uint16_t> indices; };
void tri(UnitMesh& m,unsigned a,unsigned b,unsigned c) {
  m.indices.push_back(static_cast<std::uint16_t>(a));m.indices.push_back(static_cast<std::uint16_t>(b));m.indices.push_back(static_cast<std::uint16_t>(c));
}
UnitMesh box() {
  UnitMesh m;
  const std::array<Vec3,6> normals{{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}}};
  for(const auto n:normals) {
    const auto u=normalized(cross(std::fabs(n.y)>.9F?Vec3{1,0,0}:Vec3{0,1,0},n)),v=cross(n,u);
    const auto base=static_cast<unsigned>(m.vertices.size());
    for(const auto st:std::array<std::array<float,2>,4>{{{-1,-1},{1,-1},{1,1},{-1,1}}})m.vertices.push_back({(n+u*st[0]+v*st[1])*.5F,n});
    tri(m,base,base+1,base+2);tri(m,base,base+2,base+3);
  }
  return m;
}
UnitMesh cylinder(unsigned segments,bool cone=false) {
  UnitMesh m;
  // Separate cap vertices keep disk faces flat under the existing vertex-colour shader.
  for(unsigned i=0;i<segments;++i) {
    const float a=2*pi*i/segments;const Vec3 radial{std::cos(a),0,std::sin(a)};
    const auto normal=cone?normalized(radial+Vec3{0,1,0}):radial;
    m.vertices.push_back({radial+Vec3{0,-.5F,0},normal});
    m.vertices.push_back({(cone?Vec3{}:radial)+Vec3{0,.5F,0},normal});
  }
  for(unsigned i=0;i<segments;++i) {
    const auto a=2*i,b=2*((i+1)%segments);
    tri(m,a,a+1,b);if(!cone)tri(m,b,a+1,b+1);
  }
  for(float y:{-.5F,.5F}) {
    if(cone && y>0)continue;
    const auto center=static_cast<unsigned>(m.vertices.size());const Vec3 normal{0,y*2,0};m.vertices.push_back({{0,y,0},normal});
    for(unsigned i=0;i<segments;++i) {const float a=2*pi*i/segments;m.vertices.push_back({{std::cos(a),y,std::sin(a)},normal});}
    for(unsigned i=0;i<segments;++i) {
      const auto a=center+1+i,b=center+1+(i+1)%segments;
      if(y<0)tri(m,center,a,b);else tri(m,center,b,a);
    }
  }
  return m;
}
UnitMesh sphere() {
  UnitMesh m;
  m.vertices.push_back({{0,1,0},{0,1,0}});
  for(unsigned ring=1;ring<8;++ring)for(unsigned slice=0;slice<16;++slice) {
    const float a=pi*ring/8,b=2*pi*slice/16;const Vec3 p{std::sin(a)*std::cos(b),std::cos(a),std::sin(a)*std::sin(b)};m.vertices.push_back({p,p});
  }
  m.vertices.push_back({{0,-1,0},{0,-1,0}});
  for(unsigned i=0;i<16;++i) {
    const auto next=(i+1)%16;tri(m,0,1+next,1+i);tri(m,113,97+i,97+next);
    for(unsigned ring=0;ring<6;++ring) {const auto a=1+ring*16+i,b=1+ring*16+next;tri(m,a,b,a+16);tri(m,b,b+16,a+16);}
  }
  return m;
}
UnitMesh ring() {
  UnitMesh m;constexpr unsigned major=96,minor=6;
  for(unsigned i=0;i<major;++i)for(unsigned j=0;j<minor;++j) {
    const float a=2*pi*i/major,b=2*pi*j/minor;const Vec3 n{std::cos(a)*std::cos(b),std::sin(a)*std::cos(b),std::sin(b)};
    m.vertices.push_back({Vec3{std::cos(a),std::sin(a),0}+n*.015F,n});
  }
  for(unsigned i=0;i<major;++i)for(unsigned j=0;j<minor;++j) {
    const auto a=i*minor+j,b=((i+1)%major)*minor+j,c=i*minor+(j+1)%minor,d=((i+1)%major)*minor+(j+1)%minor;
    tri(m,a,b,c);tri(m,b,d,c);
  }
  return m;
}
const UnitMesh& unit(MathShape shape) {
  static const std::array<UnitMesh,6> meshes{box(),cylinder(8),cylinder(24),sphere(),ring(),cylinder(8,true)};
  return meshes.at(static_cast<std::size_t>(shape));
}
Vec3 position(const SceneVertex& v){return {v.position[0],v.position[1],v.position[2]};}
void include(Aabb3& b,Vec3 p) {
  b.min={std::min(b.min.x,p.x),std::min(b.min.y,p.y),std::min(b.min.z,p.z)};
  b.max={std::max(b.max.x,p.x),std::max(b.max.y,p.y),std::max(b.max.z,p.z)};
}
} // namespace

MathObjectScene::MathObjectScene() {
  frame_.vertices.reserve(kSceneVertexCapacity);frame_.indices.reserve(kSceneIndexCapacity);frame_.draws.reserve(MathObjectSnapshot::kPartCapacity+2);
}
void MathObjectScene::rebuild(const MathObjectSnapshot& snapshot) {
  frame_.vertices.clear();frame_.indices.clear();frame_.draws.clear();
  const Vec3 light=normalized(Vec3{-.4F,.8F,.6F});
  for(std::size_t i=0;i<snapshot.partCount;++i) {
    const auto& part=snapshot.parts[i];const auto& source=unit(part.shape);const auto base=frame_.vertices.size();
    if(base+source.vertices.size()>kSceneVertexCapacity || frame_.indices.size()+source.indices.size()>kSceneIndexCapacity)throw std::runtime_error("math object exceeds Paths scene buffer capacity");
    const auto nx=cross(part.y,part.z),ny=cross(part.z,part.x),nz=cross(part.x,part.y);const float determinant=dot(part.x,nx);
    if(!std::isfinite(determinant)||std::fabs(determinant)<1e-15F)throw std::runtime_error("singular math primitive transform");
    SceneDraw draw{{part.id},frame_.indices.size(),source.indices.size(),{}};
    for(const auto& vertex:source.vertices) {
      const auto p=part.center+part.x*vertex.position.x+part.y*vertex.position.y+part.z*vertex.position.z;
      const auto normal=normalized((nx*vertex.normal.x+ny*vertex.normal.y+nz*vertex.normal.z)*(1/determinant));
      const auto color=part.color*(.48F+.52F*std::max(0.0F,dot(normal,light)));
      if(!isFinite(p)||!isFinite(color))throw std::runtime_error("nonfinite math vertex");
      frame_.vertices.push_back({{p.x,p.y,p.z},{color.x,color.y,color.z},{}});
    }
    for(std::size_t n=0;n<source.indices.size();n+=3) {
      frame_.indices.push_back(static_cast<std::uint16_t>(base+source.indices[n]));
      frame_.indices.push_back(static_cast<std::uint16_t>(base+source.indices[n+(determinant>0?1:2)]));
      frame_.indices.push_back(static_cast<std::uint16_t>(base+source.indices[n+(determinant>0?2:1)]));
    }
    draw.bounds={position(frame_.vertices[base]),position(frame_.vertices[base])};
    for(std::size_t n=base;n<frame_.vertices.size();++n)include(draw.bounds,position(frame_.vertices[n]));
    frame_.draws.push_back(draw);
  }
  const auto& patch=snapshot.surface;
  if(patch.rows||patch.columns) {
    if(patch.rows<2||patch.columns<2||patch.rows>patch.vertices.size()/patch.columns)throw std::runtime_error("invalid mathematical surface grid");
    const auto count=patch.rows*patch.columns,indexCount=(patch.rows-1)*(patch.columns-1)*6;
    const auto base=frame_.vertices.size();
    if(base+count>kSceneVertexCapacity||frame_.indices.size()+indexCount>kSceneIndexCapacity)throw std::runtime_error("math surface exceeds Paths scene buffer capacity");
    SceneDraw draw{{static_cast<std::uint32_t>((static_cast<unsigned>(snapshot.kind)+1)*1000+999)},frame_.indices.size(),indexCount,{patch.vertices[0].position,patch.vertices[0].position}};
    for(unsigned i=0;i<count;++i) {
      const auto& v=patch.vertices[i];
      if(!isFinite(v.position)||!isFinite(v.normal)||!isFinite(v.color))throw std::runtime_error("nonfinite math surface vertex");
      const auto color=v.color*(.48F+.52F*std::max(0.0F,dot(v.normal,light)));
      frame_.vertices.push_back({{v.position.x,v.position.y,v.position.z},{color.x,color.y,color.z},{}});include(draw.bounds,v.position);
    }
    const auto triangle=[&](unsigned a,unsigned b,unsigned c) {
      const auto geometric=cross(patch.vertices[b].position-patch.vertices[a].position,patch.vertices[c].position-patch.vertices[a].position);
      const auto normal=patch.vertices[a].normal+patch.vertices[b].normal+patch.vertices[c].normal;
      if(dot(geometric,normal)<0)std::swap(b,c);
      for(auto i:{a,b,c})frame_.indices.push_back(static_cast<std::uint16_t>(base+i));
    };
    for(unsigned row=0;row+1<patch.rows;++row)for(unsigned col=0;col+1<patch.columns;++col) {
      const unsigned a=row*patch.columns+col,b=a+patch.columns;triangle(a,b,a+1);triangle(a+1,b,b+1);
    }
    frame_.draws.push_back(draw);
  }
  const auto& solid=snapshot.solid;
  if(solid.indexCount) {
    if(solid.vertexCount>solid.vertices.size()||solid.indexCount>solid.indices.size()||solid.indexCount%3)throw std::runtime_error("invalid indexed mathematical surface");
    const auto base=frame_.vertices.size();
    if(base+solid.vertexCount>kSceneVertexCapacity||frame_.indices.size()+solid.indexCount>kSceneIndexCapacity)throw std::runtime_error("indexed surface exceeds Paths scene buffer capacity");
    SceneDraw draw{{static_cast<std::uint32_t>((static_cast<unsigned>(snapshot.kind)+1)*1000+998)},frame_.indices.size(),solid.indexCount,{solid.vertices[0].position,solid.vertices[0].position}};
    for(unsigned i=0;i<solid.vertexCount;++i){const auto& v=solid.vertices[i];if(!isFinite(v.position)||!isFinite(v.normal)||!isFinite(v.color))throw std::runtime_error("nonfinite indexed surface vertex");const auto color=v.color*(.48F+.52F*std::max(0.0F,dot(v.normal,light)));frame_.vertices.push_back({{v.position.x,v.position.y,v.position.z},{color.x,color.y,color.z},{}});include(draw.bounds,v.position);}
    for(unsigned i=0;i<solid.indexCount;++i){if(solid.indices[i]>=solid.vertexCount)throw std::runtime_error("indexed surface index out of range");frame_.indices.push_back(static_cast<std::uint16_t>(base+solid.indices[i]));}
    frame_.draws.push_back(draw);
  }
  if(frame_.vertices.empty())throw std::runtime_error("empty mathematical object");
  bounds_={position(frame_.vertices.front()),position(frame_.vertices.front())};for(const auto& v:frame_.vertices)include(bounds_,position(v));
}
const SceneFrame& MathObjectScene::publish(const MathObjectSnapshot& snapshot,SceneViewport viewport) {
  if(!std::isfinite(viewport.x)||!std::isfinite(viewport.y)||!std::isfinite(viewport.width)||!std::isfinite(viewport.height)||viewport.width<1||viewport.height<1)throw std::invalid_argument("invalid math viewport");
  const bool changed=kind_!=snapshot.kind||level_!=snapshot.level;frame_.viewport=viewport;
  if(changed || revision_!=snapshot.revision){rebuild(snapshot);kind_=snapshot.kind;level_=snapshot.level;revision_=snapshot.revision;}
  if(changed)resetView();
  publishSceneCamera(frame_,camera_);frame_.id={++frameId_};return frame_;
}
void MathObjectScene::resetView() {
  if(kind_==MathObjectKind::Count)return;
  const auto direction=normalized(mathObjectSpecs()[static_cast<std::size_t>(kind_)].cameraDirection)*-1;
  camera_.yawDegrees=std::atan2(direction.x,-direction.z)*180/pi;camera_.pitchDegrees=std::asin(direction.y)*180/pi;
  ProductCreativeCameraFrameRequest request;request.boundsMinMeters=bounds_.min;request.boundsMaxMeters=bounds_.max;
  request.cameraYawDegrees=camera_.yawDegrees;request.cameraPitchDegrees=camera_.pitchDegrees;request.viewportAspectRatio=frame_.viewport.width/frame_.viewport.height;
  const auto planned=planProductCreativeCameraFrame(request);if(!planned.applied)throw std::runtime_error("math camera framing rejected");
  camera_.anchorPositionMeters=planned.anchorPositionMeters;focus_=makeProductCreativeViewportFocus((bounds_.min+bounds_.max)*.5F,planned.distanceMeters);
}
bool MathObjectScene::navigate(ProductCreativeViewportNavigationOperation operation,float x,float y) {
  ProductCreativeViewportNavigationRequest request;request.operation=operation;request.horizontalInput=x;request.verticalInput=y;
  request.pose=camera_;request.focus=focus_;request.viewportHeightPixels=frame_.viewport.height;
  const auto result=applyProductCreativeViewportNavigation(request);if(!result.applied)return false;
  camera_=result.pose;focus_=result.focus;return true;
}
Vec3 MathObjectScene::project(Vec3 world) const {
  const auto point=projectPoint(frame_.clipFromWorld,world);if(!point.finite||point.w<=0||point.ndc.z<0||point.ndc.z>1)return {-1,-1,-1};
  return {frame_.viewport.x+(point.ndc.x+1)*.5F*frame_.viewport.width,frame_.viewport.y+(point.ndc.y+1)*.5F*frame_.viewport.height,point.ndc.z};
}
} // namespace paths
