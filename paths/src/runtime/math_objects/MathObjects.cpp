#include "runtime/math_objects/MathObjects.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace paths {
using namespace iggy3d;
namespace {
constexpr double pi = 3.14159265358979323846;
constexpr Vec3 teal{.19F,.72F,.65F}, blue{.34F,.56F,.91F}, coral{.96F,.57F,.39F},
  gold{.97F,.79F,.35F}, violet{.68F,.52F,.89F}, muted{.43F,.52F,.61F}, white{.85F,.91F,.94F};
constexpr std::array<MathParameterSpec,10> parameters{{
  {MathParameter::X,MathObjectKind::Algebra,"x","Variable x",.5,2.5,.1,1.6},
  {MathParameter::Gap,MathObjectKind::Algebra,"gap","Separate pieces",0,.6,.02,.24},
  {MathParameter::Angle,MathObjectKind::Trig,"angle","Angle (degrees)",0,360,1,45},
  {MathParameter::Slices,MathObjectKind::Calculus,"slices","Number of slices",3,64,1,8},
  {MathParameter::SliceGap,MathObjectKind::Calculus,"slice_gap","Separate slices",0,.12,.01,0},
  {MathParameter::Sample,MathObjectKind::Calculus,"sample","Sampling: left / midpoint / right",0,2,1,1},
  {MathParameter::Shear,MathObjectKind::Linear,"shear","Shear k",-1.2,1.2,.1,.6},
  {MathParameter::Scale,MathObjectKind::Linear,"scale","Vertical scale s",-2,2,.1,1},
  {MathParameter::Depth,MathObjectKind::Discrete,"depth","Layer spacing",.3,1.5,.1,1},
  {MathParameter::Shortcut,MathObjectKind::Discrete,"shortcut","Add A-H shortcut",0,1,1,0}
}};
constexpr std::array<MathObjectSpec,5> objects{{
  {MathObjectKind::Algebra,"algebra","Algebra","A cube full of algebra",
   "(x+1)^3 = x^3 + 3x^2 + 3x + 1","Make a closed cube with total volume 27.",
   "x is a positive length. Gaps change placement, never piece dimensions.",
   {"Measure lengths and volumes. Count eight pieces.","Expand a cube by adding its component volumes.","Connect 1, 3, 3, 1 with binomial coefficients."},{-1,.7F,-1}},
  {MathObjectKind::Trig,"trig","Trigonometry","Turn a circle into a wave",
   "P = (cos(theta), sin(theta))    cos^2 + sin^2 = 1","Find a second-quadrant angle whose sine is 0.5.",
   "Radius = 1. The separate wave chart has its own angle axis, from 0 to 2*pi.",
   {"Connect angle and side ratios in a right triangle.","Read signed coordinates, degrees and radians.","Connect circular motion to waves and phasors."},{0,.05F,1}},
  {MathObjectKind::Calculus,"calculus","Calculus","Build volume one slice at a time",
   "V ~ sum(pi*r_i^2*dx) -> integral[0,2](pi*x^2 dx) = 8*pi/3",
   "Approximate the cone volume with less than 0.1% error.",
   "Error uses analytic disk volumes. Mesh circles have 24 sides; gaps change placement only.",
   {"Calculate cylinder volume as area times thickness.","Compare left, midpoint and right sums.","Take the limit to obtain a volume integral."},{-1,.6F,1}},
  {MathObjectKind::Linear,"linear","Linear algebra","Watch a matrix move space",
   "A = [[1,k,0], [0,s,0], [0,0,1]]    det(A) = s",
   "Collapse the cube into a plane. Watch determinant and rank.",
   "The coloured cage is the image of a unit cube. Volume is abs(det A); the grey cage is the original.",
   {"Explore coordinates, stretching, reflection and volume.","Read matrix columns as transformed basis vectors.","Connect determinant, orientation, rank and null space."},{1,.8F,1}},
  {MathObjectKind::Discrete,"discrete","Discrete maths","Find a path through a network",
   "Q3: each ordinary edge flips one bit. Every edge costs one hop.",
   "Reach H from A in the fewest edge hops.",
   "Layout changes do not change adjacency. Crossings are not vertices. The shortcut changes the graph.",
   {"Follow routes and count steps.","Use three-bit addresses to represent the vertices.","Explore shortest paths, Hamming distance and hypercubes."},{1,.7F,1}}
}};
constexpr std::array<std::string_view,8> nodeLabels{"A 000","B 001","C 010","D 011","E 100","F 101","G 110","H 111"};
constexpr std::array<std::string_view,4> termNames{"1","x","x^2","x^3"};
std::size_t index(auto value) { return static_cast<std::size_t>(value); }
bool edge(unsigned a,unsigned b,bool shortcut) {
  const auto bits=a^b;
  return (bits==1 || bits==2 || bits==4) || (shortcut && ((a==0 && b==7)||(a==7 && b==0)));
}
unsigned shortest(bool shortcut) {
  std::array<int,8> distance;distance.fill(-1);distance[0]=0;
  std::array<unsigned,8> queue{};unsigned tail=1;
  for(unsigned head=0;head<tail;++head) {
    const auto a=queue[head];
    for(unsigned b=0;b<8;++b) if(distance[b]<0 && edge(a,b,shortcut)) {
      distance[b]=distance[a]+1;queue[tail++]=b;
    }
  }
  return static_cast<unsigned>(distance[7]);
}
class SnapshotBuilder {
public:
  explicit SnapshotBuilder(MathObjectSnapshot& snapshot):s(snapshot) {}
  void part(MathShape shape,Vec3 center,Vec3 x,Vec3 y,Vec3 z,Vec3 color,std::string_view role) {
    if(s.partCount==s.parts.size())throw std::logic_error("math part capacity exceeded");
    const auto id=static_cast<std::uint32_t>((index(s.kind)+1)*1000+s.partCount+1);
    s.parts[s.partCount++]={id,shape,center,x,y,z,color,role};
  }
  void scaled(MathShape shape,Vec3 center,Vec3 scale,Vec3 color,std::string_view role) {
    part(shape,center,{scale.x,0,0},{0,scale.y,0},{0,0,scale.z},color,role);
  }
  void ball(Vec3 center,float radius,Vec3 color,std::string_view role) { scaled(MathShape::Sphere,center,{radius,radius,radius},color,role); }
  void rod(Vec3 a,Vec3 b,Vec3 color,float radius=.016F,std::string_view role="guide") {
    const auto delta=b-a;const float len=length(delta);if(len<1e-6F)return;
    const auto y=delta*(1/len), x=normalized(cross(std::fabs(y.y)>.9F?Vec3{1,0,0}:Vec3{0,1,0},y)), z=cross(x,y);
    part(MathShape::Rod,(a+b)*.5F,x*radius,delta,z*radius,color,role);
  }
  void arrow(Vec3 a,Vec3 b,Vec3 color,std::string_view role) {
    const auto delta=b-a;const float len=length(delta);if(len<1e-6F)return;
    const auto y=delta*(1/len), x=normalized(cross(std::fabs(y.y)>.9F?Vec3{1,0,0}:Vec3{0,1,0},y)), z=cross(x,y);
    const float tip=std::min(.15F,len*.23F);
    rod(a,b-y*tip,color,.024F,role);
    part(MathShape::Cone,b-y*(tip*.5F),x*.065F,y*tip,z*.065F,color,role);
  }
  void label(std::string_view text,Vec3 pos,Vec3 color=white) {
    if(s.labelCount==s.labels.size())throw std::logic_error("math label capacity exceeded");
    s.labels[s.labelCount++]={text,pos,color};
  }
  void metric(std::string_view text,double value,std::string_view suffix={}) {
    if(s.metricCount==s.metrics.size())throw std::logic_error("math metric capacity exceeded");
    s.metrics[s.metricCount++]={text,suffix,value};
  }
  MathObjectSnapshot& s;
};
} // namespace

std::span<const MathObjectSpec> mathObjectSpecs(){return objects;}
std::span<const MathParameterSpec> mathParameterSpecs(){return parameters;}
MathObjects::MathObjects() {for(const auto& p:parameters)parameters_[index(p.id)]=p.initial;rebuild();}
double MathObjects::parameter(MathParameter p) const {
  if(index(p)>=parameters_.size())throw std::invalid_argument("unknown math parameter");
  return parameters_[index(p)];
}
MathActionResult MathObjects::dispatch(const MathAction& a) {
  switch(a.kind) {
    case MathActionKind::Select:
      if(index(a.object)>=objects.size())return {false,"unknown_object"};
      snapshot_.kind=a.object;break;
    case MathActionKind::SetParameter: {
      if(index(a.parameter)>=parameters.size())return {false,"unknown_parameter"};
      const auto& p=parameters[index(a.parameter)];
      if(p.owner!=snapshot_.kind)return {false,"parameter_not_owned_by_object"};
      if(!std::isfinite(a.value)||a.value<p.minimum-1e-6||a.value>p.maximum+1e-6)return {false,"parameter_out_of_range"};
      parameters_[index(a.parameter)]=std::clamp(std::round(a.value/p.step)*p.step,p.minimum,p.maximum);
      if(a.parameter==MathParameter::Shortcut)snapshot_.routeCount=1;
      break;
    }
    case MathActionKind::Reset:
      for(const auto& p:parameters)if(p.owner==snapshot_.kind)parameters_[index(p.id)]=p.initial;
      if(snapshot_.kind==MathObjectKind::Discrete)snapshot_.routeCount=1;
      break;
    case MathActionKind::VisitVertex:
      if(snapshot_.kind!=MathObjectKind::Discrete || a.vertex>=8 || !snapshot_.allowedVertices[a.vertex])return {false,"vertex_not_available"};
      if(snapshot_.routeCount==snapshot_.route.size())return {false,"route_capacity_reached"};
      snapshot_.route[snapshot_.routeCount++]=a.vertex;break;
    case MathActionKind::UndoRoute:
      if(snapshot_.kind!=MathObjectKind::Discrete || snapshot_.routeCount<=1)return {false,"no_route_step_to_undo"};
      --snapshot_.routeCount;break;
    case MathActionKind::ResetRoute:
      if(snapshot_.kind!=MathObjectKind::Discrete)return {false,"wrong_object"};
      snapshot_.routeCount=1;break;
    case MathActionKind::Check:check();return {true,"challenge_checked"};
    default:return {false,"unknown_action"};
  }
  snapshot_.feedback=MathFeedback::None;snapshot_.feedbackText={};++snapshot_.revision;rebuild();return {true,"applied"};
}
void MathObjects::check() {
  bool solved=false;std::string_view good,bad;
  switch(snapshot_.kind) {
    case MathObjectKind::Algebra:
      solved=std::fabs(parameter(MathParameter::X)-2)<1e-6 && parameter(MathParameter::Gap)==0;
      good="Yes: x=2 gives volume 8+12+6+1=27.";bad="A volume of 27 needs side length 3. Then close the gaps.";break;
    case MathObjectKind::Trig:
      solved=std::fabs(parameter(MathParameter::Angle)-150)<1e-6;
      good="Yes: at 150 degrees (5*pi/6), sine is 0.5 and cosine is negative.";bad="Look between 90 and 180 degrees for height 0.5.";break;
    case MathObjectKind::Calculus:
      solved=snapshot_.metrics[2].value<.1;
      good="Yes: the disk-sum volume is within 0.1% of the integral.";bad="Increase the slice count and compare sampling rules.";break;
    case MathObjectKind::Linear:
      solved=parameter(MathParameter::Scale)==0;
      good="Yes: determinant 0, rank 2, and zero three-dimensional volume.";bad="Change vertical scale. A shear alone preserves volume.";break;
    case MathObjectKind::Discrete:
      solved=snapshot_.route[snapshot_.routeCount-1]==7 && snapshot_.routeCount-1==snapshot_.shortestHops;
      good="Yes: your route reaches H in the minimum number of edge hops.";bad="Reach H using the fewest hops. Every edge costs one.";break;
    case MathObjectKind::Count:return;
  }
  snapshot_.feedback=solved?MathFeedback::Solved:MathFeedback::TryAgain;snapshot_.feedbackText=solved?good:bad;
}
void MathObjects::rebuild() {
  snapshot_.partCount=0;snapshot_.labelCount=0;snapshot_.metricCount=0;snapshot_.allowedVertices.fill(false);
  SnapshotBuilder b(snapshot_);const auto value=[&](MathParameter p){return static_cast<float>(parameter(p));};
  switch(snapshot_.kind) {
    case MathObjectKind::Algebra: {
      const float x=value(MathParameter::X),gap=value(MathParameter::Gap);
      const std::array<Vec3,4> colors{gold,coral,blue,teal};
      for(unsigned i=0;i<8;++i) {
        const unsigned bx=i&1,by=(i>>1)&1,bz=(i>>2)&1,degree=3-bx-by-bz;
        const Vec3 size{bx?1:x,by?1:x,bz?1:x};
        b.scaled(MathShape::Box,{bx?x+gap+.5F:x*.5F,by?x+gap+.5F:x*.5F,bz?x+gap+.5F:x*.5F},size,colors[degree],termNames[degree]);
      }
      b.label("x^3",{-.05F,x*.5F,x*.5F},teal);b.label("x^2",{-.05F,x+gap+.5F,x*.5F},blue);
      b.label("x",{x+gap+.5F,x+gap+.5F,-.05F},coral);b.label("1",{x+gap+.5F,x+gap+1.15F,x+gap+.5F},gold);
      const double d=parameter(MathParameter::X);
      b.metric("Total volume",std::pow(d+1,3),"units^3");b.metric("x^3",d*d*d);b.metric("3x^2",3*d*d);b.metric("3x",3*d);b.metric("Constant",1);break;
    }
    case MathObjectKind::Trig: {
      const double theta=parameter(MathParameter::Angle)*pi/180,cosine=std::cos(theta),sine=std::sin(theta);
      const float c=static_cast<float>(cosine),s=static_cast<float>(sine),start=1.8F,scale=.55F;
      b.scaled(MathShape::Ring,{},{1,1,1},white,"unit_circle");
      b.arrow({-1.2F,0,0},{1.3F,0,0},muted,"x_axis");b.arrow({0,-1.2F,0},{0,1.3F,0},muted,"y_axis");
      b.arrow({0,0,.04F},{c,s,.04F},gold,"radius");b.rod({0,0,.07F},{c,0,.07F},coral,.025F,"cosine");b.rod({c,0,.07F},{c,s,.07F},teal,.025F,"sine");
      b.ball({c,s,.07F},.065F,gold,"point_P");b.ball({0,0,.07F},.035F,white,"origin");
      b.arrow({start,0,0},{start+static_cast<float>(2*pi)*scale+.15F,0,0},muted,"theta_axis");
      for(unsigned i=0;i<96;++i) {
        const double a=2*pi*i/96,c1=2*pi*(i+1)/96;
        b.rod({start+scale*static_cast<float>(a),static_cast<float>(std::sin(a)),0},{start+scale*static_cast<float>(c1),static_cast<float>(std::sin(c1)),0},teal,.016F,"sine_wave");
      }
      const float wx=start+scale*static_cast<float>(theta);
      b.ball({wx,s,.04F},.065F,coral,"wave_cursor");b.rod({wx,0,0},{wx,s,0},coral,.012F,"wave_projection");
      for(unsigned i=0;i<20;++i)b.rod({c+(wx-c)*i/20,s,-.045F},{c+(wx-c)*(i+.45F)/20,s,-.045F},muted,.006F,"equal_height");
      b.label("P",{c,s+.2F,.07F},gold);b.label("x",{1.4F,-.1F,0});b.label("y",{.15F,1.35F,0});
      b.label("0",{start,-.18F,0});b.label("pi",{start+scale*static_cast<float>(pi),-.18F,0});b.label("2*pi",{start+scale*static_cast<float>(2*pi),-.18F,0});
      b.metric("cos(theta)",cosine);b.metric("sin(theta)",sine);b.metric("Radians",theta);b.metric("cos^2 + sin^2",cosine*cosine+sine*sine);break;
    }
    case MathObjectKind::Calculus: {
      const unsigned n=static_cast<unsigned>(parameter(MathParameter::Slices));const double dx=2.0/n,f=parameter(MathParameter::Sample)/2;
      const float gap=value(MathParameter::SliceGap);double estimate=0;
      for(unsigned i=0;i<n;++i) {
        const double sample=(i+f)*dx;estimate+=pi*sample*sample*dx;if(sample==0)continue;
        const float radius=static_cast<float>(sample),thickness=static_cast<float>(dx),center=static_cast<float>((i+.5)*dx)+(static_cast<float>(i)-(n-1)*.5F)*gap;
        b.part(MathShape::Disk,{center,0,0},{0,radius,0},{thickness,0,0},{0,0,-radius},i%2?blue:teal,"integration_disk");
      }
      const float extension=static_cast<float>(n-1)*.5F*gap,start=-extension,end=2+extension;
      b.arrow({start-.25F,0,0},{end+.3F,0,0},white,"x_axis");
      if(gap==0) {
        for(unsigned i=0;i<8;++i) {const double a=i*pi/4;b.rod({},{2,2*static_cast<float>(std::cos(a)),2*static_cast<float>(std::sin(a))},gold,.008F,"cone_guide");}
        b.part(MathShape::Ring,{2,0,0},{0,2,0},{0,0,2},{2,0,0},gold,"cone_rim");
      }
      b.label("x=0",{start,-.25F,0});b.label("x=2",{end,-2.2F,0});b.label("r(x)=x",{end,1.5F,1.5F},teal);
      const double exact=8*pi/3;b.metric("Disk-sum volume",estimate);b.metric("Exact volume",exact);b.metric("Relative error",std::fabs(estimate-exact)/exact*100,"%");b.metric("Slice width",dx);break;
    }
    case MathObjectKind::Linear: {
      const float k=value(MathParameter::Shear),s=value(MathParameter::Scale);
      const auto mapped=[&](Vec3 p){return Vec3{p.x+k*p.y,s*p.y,p.z};};
      std::array<Vec3,8> corners{};
      for(unsigned i=0;i<8;++i)corners[i]={static_cast<float>(i&1),static_cast<float>((i>>1)&1),static_cast<float>((i>>2)&1)};
      for(unsigned i=0;i<8;++i)for(unsigned axis=0;axis<3;++axis) {
        const unsigned j=i^(1U<<axis);if(j<i)continue;
        b.rod(corners[i],corners[j],muted,.008F,"original_cube");b.rod(mapped(corners[i]),mapped(corners[j]),teal,.018F,"transformed_cube");
      }
      for(unsigned t=1;t<4;++t)for(float face:{0.0F,1.0F}) {
        const float q=t*.25F;
        b.rod(mapped({q,face,0}),mapped({q,face,1}),teal,.005F,"lattice");
        b.rod(mapped({0,q,face}),mapped({1,q,face}),teal,.005F,"lattice");
        b.rod(mapped({face,0,q}),mapped({face,1,q}),teal,.005F,"lattice");
      }
      const std::array<Vec3,3> basis{{{1,0,0},{0,1,0},{0,0,1}}},colors{coral,teal,violet};
      const std::array<std::string_view,3> names{"A e1","A e2","A e3"};
      for(unsigned i=0;i<3;++i) {b.arrow({},mapped(basis[i]),colors[i],names[i]);b.label(names[i],mapped(basis[i])+Vec3{.05F,.12F,.05F},colors[i]);}
      b.ball({},.03F,white,"origin");
      const double d=parameter(MathParameter::Scale);b.metric("Signed determinant",d);b.metric("Volume",std::fabs(d),"units^3");b.metric("Rank",d==0?2:3);break;
    }
    case MathObjectKind::Discrete: {
      const float depth=value(MathParameter::Depth);const bool shortcut=parameter(MathParameter::Shortcut)!=0;
      std::array<Vec3,8> points{};
      for(unsigned i=0;i<8;++i)points[i]={static_cast<float>(((i>>2)&1)*2)-1,static_cast<float>(((i>>1)&1)*2)-1,(static_cast<float>((i&1)*2)-1)*depth};
      unsigned edgeCount=0;
      for(unsigned a=0;a<8;++a)for(unsigned c=a+1;c<8;++c)if(edge(a,c,shortcut)) {
        bool chosen=false;for(std::size_t j=1;j<snapshot_.routeCount;++j)chosen|=(snapshot_.route[j-1]==a&&snapshot_.route[j]==c)||(snapshot_.route[j-1]==c&&snapshot_.route[j]==a);
        b.rod(points[a],points[c],chosen?gold:muted,chosen?.035F:.021F,"graph_edge");++edgeCount;
      }
      for(unsigned i=0;i<8;++i) {
        const bool visited=std::find(snapshot_.route.begin(),snapshot_.route.begin()+snapshot_.routeCount,i)!=snapshot_.route.begin()+snapshot_.routeCount;
        b.ball(points[i],.115F,i==0?teal:i==7?coral:visited?gold:blue,nodeLabels[i]);b.label(nodeLabels[i],points[i]+Vec3{0,.25F,0});
        snapshot_.allowedVertices[i]=snapshot_.route[snapshot_.routeCount-1]!=7 && edge(snapshot_.route[snapshot_.routeCount-1],i,shortcut);
      }
      snapshot_.shortestHops=shortest(shortcut);b.metric("Vertices",8);b.metric("Edges",edgeCount);b.metric("Your hops",snapshot_.routeCount-1);b.metric("Shortest hops",snapshot_.shortestHops);break;
    }
    case MathObjectKind::Count:throw std::logic_error("invalid math object state");
  }
}
} // namespace paths
