#include "runtime/math_objects/BooleanGeometry.hpp"
#include "runtime/math_objects/MathObjects.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace paths {
namespace {
using iggy3d::Vec3;
constexpr unsigned maximum=28,side=maximum+1,gridCapacity=side*side*side,hashCapacity=32768;
struct EdgeEntry {std::uint64_t key=0;unsigned vertex=0;};
struct Mesher {
  const BooleanSolid& solid;BooleanDisplay display;MathTriangleSurface& mesh;
  std::array<double,gridCapacity> values;
  std::array<EdgeEntry,hashCapacity> edges{};
  unsigned n=0,stride=0;SolidPoint step{};bool full=false;
  SolidSample sample(SolidPoint p) const {
    auto s=sampleBoolean(solid,p);if(display.section){SolidSample plane;plane.value=p[2]-display.sectionZ;plane.gradient={0,0,1};plane.regular=true;plane.material=.5;s=combineSolids(s,plane,SolidOperation::Intersection,0);}return s;
  }
  SolidPoint position(unsigned id) const {const std::array<unsigned,3> xyz{id%stride,(id/stride)%stride,id/(stride*stride)};SolidPoint p{};for(unsigned j=0;j<3;++j)p[j]=solid.bounds.minimum[j]+xyz[j]*step[j];return p;}
  unsigned vertex(unsigned a,unsigned b){
    if(values[a]==0)b=a;else if(values[b]==0)a=b;if(a>b)std::swap(a,b);
    const std::uint64_t key=(static_cast<std::uint64_t>(a+1)<<32)|(b+1);unsigned slot=static_cast<unsigned>((key^(key>>31))*0x9e3779b97f4a7c15ULL)&(hashCapacity-1);
    while(edges[slot].key&&edges[slot].key!=key)slot=(slot+1)&(hashCapacity-1);
    if(edges[slot].key)return edges[slot].vertex;
    if(mesh.vertexCount==mesh.vertices.size()){full=true;return 0;}
    const double t=a==b?0:values[a]/(values[a]-values[b]);const auto p=position(a),q=position(b);SolidPoint point{};for(unsigned j=0;j<3;++j)point[j]=p[j]+t*(q[j]-p[j]);const unsigned index=append(point);edges[slot]={key,index};return index;
  }
  unsigned append(SolidPoint point){
    if(mesh.vertexCount==mesh.vertices.size()){full=true;return 0;}
    const auto s=sample(point);
    const double len=std::sqrt(s.gradient[0]*s.gradient[0]+s.gradient[1]*s.gradient[1]+s.gradient[2]*s.gradient[2]);Vec3 normal{0,1,0};if(len>1e-12)normal={static_cast<float>(s.gradient[0]/len),static_cast<float>(s.gradient[1]/len),static_cast<float>(s.gradient[2]/len)};
    const float w=static_cast<float>(s.material);Vec3 color=Vec3{.22F,.70F,.66F}*(1-w)+Vec3{.91F,.45F,.29F}*w;
    if(display.section&&std::fabs(point[2]-display.sectionZ)<1e-8)color={.84F,.69F,.32F};
    const unsigned index=mesh.vertexCount++;mesh.vertices[index]={{static_cast<float>(point[0]),static_cast<float>(point[1]),static_cast<float>(point[2])},normal,color};return index;
  }
  void triangle(unsigned a,unsigned b,unsigned c){
    if(full||a==b||b==c||c==a)return;
    const auto p=mesh.vertices[a].position,q=mesh.vertices[b].position,r=mesh.vertices[c].position;const auto normal=iggy3d::cross(q-p,r-p);
    if(iggy3d::lengthSquared(normal)==0)return;
    if(mesh.indexCount+3>mesh.indices.size()){full=true;return;}
    for(unsigned v:{a,b,c})mesh.indices[mesh.indexCount++]=static_cast<std::uint16_t>(v);
  }
  void cubeContour(const std::array<unsigned,8>& ids){
    constexpr std::array<std::array<unsigned,2>,12> edgeEnds{{{0,1},{1,3},{3,2},{2,0},{4,5},{5,7},{7,6},{6,4},{0,4},{1,5},{3,7},{2,6}}};
    // Each square is counterclockwise when viewed from outside this cube.
    constexpr std::array<std::array<unsigned,4>,6> faces{{{0,2,3,1},{4,5,7,6},{0,1,5,4},{2,6,7,3},{0,4,6,2},{1,3,7,5}}};
    std::array<int,12> next;next.fill(-1);std::array<bool,12> used{};
    const auto edgeId=[&](unsigned a,unsigned b){for(unsigned i=0;i<12;++i)if((edgeEnds[i][0]==a&&edgeEnds[i][1]==b)||(edgeEnds[i][1]==a&&edgeEnds[i][0]==b))return i;throw std::logic_error("invalid cube edge");};
    for(const auto& face:faces){std::array<unsigned,4> edges{};unsigned crossings=0,entry=0,exit=0;
      for(unsigned i=0;i<4;++i){edges[i]=edgeId(face[i],face[(i+1)%4]);const bool a=values[ids[face[i]]]<0,b=values[ids[face[(i+1)%4]]]<0;if(a!=b){++crossings;if(b)entry=edges[i];else exit=edges[i];}}
      // Surface boundary orientation is opposite the occupied part of the cube face.
      if(crossings==2)next[entry]=static_cast<int>(exit);
      if(crossings==4){SolidPoint center{};const auto low=position(ids[*std::min_element(face.begin(),face.end())]),high=position(ids[*std::max_element(face.begin(),face.end())]);for(unsigned j=0;j<3;++j)center[j]=(low[j]+high[j])/2;const bool inside=sample(center).value<-1e-12;
        for(unsigned i=0;i<4;++i)if((values[ids[face[i]]]<0)!=inside){if(inside)next[edges[i]]=static_cast<int>(edges[(i+3)%4]);else next[edges[(i+3)%4]]=static_cast<int>(edges[i]);}}
    }
    for(unsigned first=0;first<12&&!full;++first)if(next[first]>=0&&!used[first]){
      std::array<unsigned,12> polygon{};unsigned count=0,e=first;SolidPoint center{};
      do{if(e>=12||used[e]||next[e]<0)throw std::logic_error("open cube contour");used[e]=true;const auto v=vertex(ids[edgeEnds[e][0]],ids[edgeEnds[e][1]]);if(full)return;if(!count||polygon[count-1]!=v)polygon[count++]=v;e=static_cast<unsigned>(next[e]);}while(e!=first);
      if(count>1&&polygon[count-1]==polygon[0])--count;if(count<3)continue;
      if(count<=4){for(unsigned i=1;i+1<count;++i)triangle(polygon[0],polygon[i],polygon[i+1]);}
      else{for(unsigned i=0;i<count;++i){const auto q=mesh.vertices[polygon[i]].position;center[0]+=q.x/count;center[1]+=q.y/count;center[2]+=q.z/count;}const auto middle=append(center);for(unsigned i=0;i<count;++i)triangle(middle,polygon[i],polygon[(i+1)%count]);}
    }
  }
  bool run(unsigned cells){
    n=cells;stride=n+1;mesh.vertexCount=mesh.indexCount=0;full=false;edges.fill({});for(unsigned j=0;j<3;++j)step[j]=(solid.bounds.maximum[j]-solid.bounds.minimum[j])/n;
    // Canonicalize roundoff at grid nodes on the zero set, including a section
    // plane aligned with a grid layer. Every incident cube shares the node.
    for(unsigned id=0;id<stride*stride*stride;++id){const double value=sample(position(id)).value;values[id]=std::fabs(value)<1e-12?0:value;}
    for(unsigned z=0;z<n&&!full;++z)for(unsigned y=0;y<n&&!full;++y)for(unsigned x=0;x<n&&!full;++x){std::array<unsigned,8> cube{};for(unsigned i=0;i<8;++i)cube[i]=(x+(i&1))+stride*(y+((i>>1)&1))+stride*stride*(z+((i>>2)&1));unsigned occupied=0;for(auto id:cube)occupied+=values[id]<0;if(occupied&&occupied!=8)cubeContour(cube);}

    return !full;
  }
};
}
BooleanMeshReport buildBooleanSurface(const BooleanSolid& s,BooleanDisplay d,MathTriangleSurface& mesh){
  if(d.cells<4||d.cells>maximum||!std::isfinite(d.sectionZ))throw std::invalid_argument("invalid Boolean display");
  Mesher mesher{s,d,mesh};unsigned n=d.cells;while(!mesher.run(n)){if(n<=4)throw std::logic_error("Boolean mesh cannot fit minimum grid");n=std::max(4U,n-4);}
  BooleanMeshReport report{n,0,0,n!=d.cells};
  for(unsigned i=0;i<mesh.indexCount;i+=3){const auto a=mesh.vertices[mesh.indices[i]].position,b=mesh.vertices[mesh.indices[i+1]].position,c=mesh.vertices[mesh.indices[i+2]].position;report.volume+=(static_cast<double>(a.x)*(b.y*c.z-b.z*c.y)+static_cast<double>(a.y)*(b.z*c.x-b.x*c.z)+static_cast<double>(a.z)*(b.x*c.y-b.y*c.x))/6;}
  for(unsigned i=0;i<mesh.vertexCount;++i){const auto p=mesh.vertices[i].position;report.maximumResidual=std::max(report.maximumResidual,std::fabs(mesher.sample({p.x,p.y,p.z}).value));}
  return report;
}
} // namespace paths
