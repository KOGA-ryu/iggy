#include "runtime/math_objects/LatheGeometry.hpp"
#include "runtime/math_objects/MathObjects.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace paths {
namespace {
using namespace iggy3d;
constexpr double pi=3.14159265358979323846;
constexpr Vec3 teal{.12F,.78F,.67F},blue{.24F,.49F,.9F},gold{1,.72F,.25F},violet{.72F,.51F,.91F};
constexpr unsigned slices=32,columns=slices+1;
class Writer {
public:
  Writer(MathTriangleSurface& mesh,const LatheDisplay& view):m(mesh) {
    if(!std::isfinite(view.turn)||view.turn<0||view.turn>1||!std::isfinite(view.cut)||view.cut<0||view.cut>.75)throw std::invalid_argument("invalid lathe display extent");
    m.vertexCount=m.indexCount=0;angle=2*pi*view.turn*(1-view.cut);start=0;closed=angle>=2*pi-1e-12;
  }
  double theta(unsigned j)const{return closed&&j==slices?start:start+angle*j/slices;}
  Vec3 point(double r,double y,unsigned j)const{const double a=theta(j);return {static_cast<float>(r*std::cos(a)),static_cast<float>(y),static_cast<float>(r*std::sin(a))};}
  unsigned vertex(Vec3 p,Vec3 n,Vec3 c){if(m.vertexCount==m.vertices.size())throw std::logic_error("lathe vertex capacity");m.vertices[m.vertexCount]={p,n,c};return m.vertexCount++;}
  void triangle(unsigned a,unsigned b,unsigned c){const auto g=cross(m.vertices[b].position-m.vertices[a].position,m.vertices[c].position-m.vertices[a].position);if(length(g)<1e-10F)return;if(dot(g,m.vertices[a].normal+m.vertices[b].normal+m.vertices[c].normal)<0)std::swap(b,c);if(m.indexCount+3>m.indices.size())throw std::logic_error("lathe triangle capacity");for(auto i:{a,b,c})m.indices[m.indexCount++]=static_cast<std::uint16_t>(i);}
  void join(unsigned a,unsigned b){for(unsigned j=0;j<slices;++j){triangle(a+j,b+j,a+j+1);triangle(a+j+1,b+j,b+j+1);}}
  unsigned ring(double r,double y,double slope,bool inside,Vec3 color){const unsigned base=m.vertexCount;for(unsigned j=0;j<columns;++j){const double a=theta(j);Vec3 n=normalized(Vec3{static_cast<float>(std::cos(a)),static_cast<float>(-slope),static_cast<float>(std::sin(a))});if(inside)n=n*-1;vertex(point(r,y,j),n,color);}return base;}
  void cap(double y,double outer,double inner,Vec3 normal,Vec3 color){if(outer<=inner+1e-12)return;const unsigned base=m.vertexCount;for(double r:{outer,inner})for(unsigned j=0;j<columns;++j)vertex(point(r,y,j),normal,color);join(base,base+columns);}
  void cut(double a,double b,double ra,double rb,double ia,double ib){if(closed)return;for(unsigned side:{0U,slices}){const double t=theta(side);const Vec3 n{static_cast<float>(std::sin(t)*(side==0?1:-1)),0,static_cast<float>(std::cos(t)*(side==0?-1:1))};const auto first=vertex(point(ra,a,side),n,gold);vertex(point(rb,b,side),n,gold);vertex(point(ib,b,side),n,gold);vertex(point(ia,a,side),n,gold);triangle(first,first+1,first+2);triangle(first,first+2,first+3);}}
  void cylinder(double a,double b,double outer,double inner){if(outer<=inner||b<=a)return;const auto o=ring(outer,a,0,false,gold);join(o,ring(outer,b,0,false,gold));if(inner>0){const auto i=ring(inner,a,0,true,violet);join(i,ring(inner,b,0,true,violet));}cap(a,outer,inner,{0,-1,0},blue);cap(b,outer,inner,{0,1,0},gold);cut(a,b,outer,outer,inner,inner);}
  MathTriangleSurface& m;double angle=0,start=0;bool closed=false;
};
}
void buildLatheSurface(const LatheProfile& p,const LatheDisplay& display,MathTriangleSurface& mesh) {
  Writer w(mesh,display);if(w.angle==0||p.maximumRadius==0)return;
  auto heights=latheHeights(p,36);
  for(double u:{display.bandFrom,display.bandTo})if(u>0&&u<1){if(heights.count==heights.values.size())throw std::logic_error("lathe display partition");heights.values[heights.count++]=u;}
  std::sort(heights.values.begin(),heights.values.begin()+heights.count);heights.count=static_cast<unsigned>(std::unique(heights.values.begin(),heights.values.begin()+heights.count,[](double a,double b){return std::fabs(a-b)<1e-12;})-heights.values.begin());
  std::array<unsigned,80> outer{},inner{};
  for(unsigned i=0;i<heights.count;++i){const double u=heights.values[i];const auto s=sampleLathe(p,u);const bool band=display.bandTo>display.bandFrom&&u>=display.bandFrom-1e-12&&u<=display.bandTo+1e-12;outer[i]=w.ring(s.radius,p.input.height*(u-.5),s.slope,false,band?gold:teal);}
  if(p.input.hollow)for(unsigned i=0;i<heights.count;++i){const double u=heights.values[i];const auto s=sampleLathe(p,u);inner[i]=w.ring(s.inner,p.input.height*(u-.5),s.slope,true,violet);}
  for(unsigned i=1;i<heights.count;++i){const double a=heights.values[i-1],b=heights.values[i];const auto lo=sampleLathe(p,a),hi=sampleLathe(p,b);const bool cavity=sampleLathe(p,(a+b)/2).inner>0;w.join(outer[i-1],outer[i]);if(cavity)w.join(inner[i-1],inner[i]);w.cut(p.input.height*(a-.5),p.input.height*(b-.5),lo.radius,hi.radius,cavity?std::max(0.0,lo.radius-p.input.wall):0,cavity?std::max(0.0,hi.radius-p.input.wall):0);}
  const auto lo=sampleLathe(p,0),hi=sampleLathe(p,1);w.cap(-p.input.height/2,lo.radius,0,{0,-1,0},blue);w.cap(p.input.height/2,hi.radius,hi.inner,{0,1,0},gold);
  if(p.input.hollow)w.cap(p.input.height*(p.input.floor-.5),sampleLathe(p,p.input.floor).inner,0,{0,1,0},violet);
}
void buildLatheElement(const LatheProfile& p,const LatheDisplay& display,unsigned n,bool shells,unsigned selected,MathTriangleSurface& mesh) {
  if(n<4||n>32||selected>=n)throw std::invalid_argument("invalid lathe highlighted element");Writer w(mesh,display);if(w.angle==0||p.maximumRadius==0)return;
  if(shells){const double dr=p.maximumRadius/n,r=(selected+.5)*dr;const auto shell=latheShell(p,r);for(unsigned i=0;i<shell.count;++i)w.cylinder(p.input.height*(shell.spans[i].from-.5),p.input.height*(shell.spans[i].to-.5),r+dr/2,r-dr/2);}
  else {const auto s=sampleLathe(p,(selected+.5)/n);w.cylinder(p.input.height*(static_cast<double>(selected)/n-.5),p.input.height*(static_cast<double>(selected+1)/n-.5),s.radius,s.inner);}
}
}
