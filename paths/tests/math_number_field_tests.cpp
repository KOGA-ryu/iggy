#include "runtime/math_objects/MathObjects.hpp"
#include "scene/MathObjectScene.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
namespace {
using namespace paths;
constexpr double pi=3.14159265358979323846;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void near(double a,double b,double eps,const char* why){if(!std::isfinite(a)||std::fabs(a-b)>eps)throw std::runtime_error(std::string(why)+": "+std::to_string(a)+" vs "+std::to_string(b));}
void action(MathObjects& m,MathAction a){const auto r=m.dispatch(a);if(!r.accepted)throw std::runtime_error(std::string(r.reason));}
void level(MathObjects& m,unsigned n){action(m,{MathActionKind::SetLevel,{},{},static_cast<double>(n)});}
void select(MathObjects& m,MathObjectKind k,unsigned n=0){action(m,{MathActionKind::Select,k});level(m,n);action(m,{MathActionKind::Reset});}
void set(MathObjects& m,MathParameter p,double x){action(m,{MathActionKind::SetParameter,{},p,x});}
double metric(const MathObjects& m,std::string_view name){const auto& s=m.snapshot();for(std::size_t i=0;i<s.metricCount;++i)if(s.metrics[i].label==name)return s.metrics[i].value;throw std::runtime_error("missing metric: "+std::string(name));}
void checked(MathObjects& m,bool solved){action(m,{MathActionKind::Check});require((m.snapshot().feedback==MathFeedback::Solved)==solved,"wrong challenge verdict");}
void reject(MathObjects& m,MathAction a){const auto revision=m.snapshot().revision;const auto mask=m.snapshot().modularVisitedMask;require(!m.dispatch(a).accepted,"invalid action accepted");require(m.snapshot().revision==revision&&m.snapshot().modularVisitedMask==mask,"rejection changed state");}
std::size_t states=0,maxVertices=0,maxIndices=0;
void inspect(const MathObjects& m,MathObjectScene& scene) {
  const auto& s=m.snapshot();const auto& frame=scene.publish(s,{0,0,950,650});++states;
  require(!frame.vertices.empty()&&frame.vertices.size()<=kSceneVertexCapacity&&frame.indices.size()<=kSceneIndexCapacity,"mesh capacity");
  maxVertices=std::max(maxVertices,frame.vertices.size());maxIndices=std::max(maxIndices,frame.indices.size());
  require(s.partCount<=s.parts.size()&&s.labelCount<=s.labels.size()&&iggy3d::isFinite(frame.clipFromWorld),"invalid snapshot or camera");
  std::size_t end=0;std::set<std::uint32_t> ids;
  for(const auto& draw:frame.draws){require(ids.insert(draw.objectId.value).second&&draw.firstIndex==end&&draw.indexCount%3==0,"invalid draw");end+=draw.indexCount;}
  require(end==frame.indices.size(),"unowned geometry");for(auto i:frame.indices)require(i<frame.vertices.size(),"invalid index");
  for(const auto& vertex:frame.vertices){for(auto v:vertex.position)require(std::isfinite(v),"nonfinite position");for(auto v:vertex.color)require(std::isfinite(v)&&v>=0&&v<=1,"invalid colour");}
  for(std::size_t i=0;i<s.metricCount;++i)require(std::isfinite(s.metrics[i].value),"nonfinite metric");
  for(std::size_t p=0;p<s.plotCount;++p){const auto& plot=s.plots[p];require(plot.seriesCount>0&&plot.seriesCount<=plot.series.size(),"invalid series count");for(std::size_t j=0;j<plot.seriesCount;++j){const auto& line=plot.series[j];require(line.count>0&&line.count<=line.points.size(),"invalid point count");for(std::size_t i=0;i<line.count;++i)require(std::isfinite(line.points[i].x)&&std::isfinite(line.points[i].y),"nonfinite plot");}}
  const auto& table=s.table;require(table.rowCount<=table.values.size()&&table.columnCount<=table.columns.size(),"table overflow");
  for(std::size_t i=0;i<table.rowCount;++i){require(!table.rowLabels[i].empty(),"missing row label");for(std::size_t j=0;j<table.columnCount;++j)require(!table.columns[j].empty()&&std::isfinite(table.values[i][j]),"invalid table cell");}
}
int mod(int a,int n){return static_cast<int>(a-n*std::floor(static_cast<double>(a)/n));}
void modular() {
  MathObjects m;MathObjectScene scene;select(m,MathObjectKind::Modular);checked(m,false);
  for(int n=2;n<=12;++n)for(int a=-48;a<=48;++a){set(m,MathParameter::Modulus,n);set(m,MathParameter::ModValue,a);const auto r=metric(m,"Canonical residue"),q=metric(m,"Integer quotient");require(r>=0&&r<n,"noncanonical remainder");near(q*n+r,a,0,"Euclidean division");if(a==-48||a==0||a==48)inspect(m,scene);}
  set(m,MathParameter::Modulus,7);set(m,MathParameter::ModValue,-3);checked(m,true);
  level(m,1);reject(m,{MathActionKind::SetParameter,{},MathParameter::ModValue,0});
  for(int n=2;n<=12;++n)for(int k=0;k<=12;++k) {
    set(m,MathParameter::Modulus,n);set(m,MathParameter::ModStep,k);action(m,{MathActionKind::ResetModularWalk});const int length=n/std::gcd(n,k);near(metric(m,"Cycle length"),length,0,"cycle length");
    for(int i=1;i<=length;++i){action(m,{MathActionKind::ModularStep,{},{},1});near(metric(m,"Canonical residue"),mod(i*k,n),0,"walk residue");}
    near(metric(m,"Visited residues"),length,0,"visited cycle cardinality");checked(m,length==n&&k>0);inspect(m,scene);
    action(m,{MathActionKind::ModularStep,{},{},-1});near(metric(m,"Canonical residue"),mod(-k,n),0,"reverse step");
  }
  action(m,{MathActionKind::ResetModularWalk});for(unsigned i=0;i<64;++i)action(m,{MathActionKind::ModularStep,{},{},1});reject(m,{MathActionKind::ModularStep,{},{},1});reject(m,{MathActionKind::ModularStep,{},{},0});reject(m,{MathActionKind::ModularStep,{},{},std::numeric_limits<double>::quiet_NaN()});
  set(m,MathParameter::ModStep,3);require(m.snapshot().modularWalkSteps==0&&metric(m,"Visited residues")==1,"setup retained stale walk evidence");
  level(m,2);for(int n=2;n<=12;++n)for(int k=0;k<=12;++k){set(m,MathParameter::Modulus,n);set(m,MathParameter::ModStep,k);near(metric(m,"Inverse exists"),std::gcd(n,k)==1?1:0,0,"inverse existence");if(std::gcd(n,k)==1){const int inverse=static_cast<int>(metric(m,"Canonical inverse"));require(inverse>=0&&inverse<n&&mod(k*inverse,n)==1,"inverse incorrect");}}
  set(m,MathParameter::Modulus,7);set(m,MathParameter::ModStep,3);set(m,MathParameter::InverseGuess,5);checked(m,true);inspect(m,scene);
  level(m,3);unsigned cases=0;
  for(int n=2;n<=12;++n)for(int other=2;other<=12;++other) {
    set(m,MathParameter::Modulus,n);set(m,MathParameter::SecondModulus,other);
    for(int a=0;a<n;++a)for(int b=0;b<other;++b) {
      set(m,MathParameter::ModValue,a);set(m,MathParameter::SecondResidue,b);++cases;
      const bool compatible=mod(a-b,std::gcd(n,other))==0;near(metric(m,"Compatible"),compatible?1:0,0,"generalised CRT compatibility");near(metric(m,"Common period"),std::lcm(n,other),0,"CRT period");
      if(compatible){const int solution=static_cast<int>(metric(m,"Smallest solution"));require(solution>=0&&solution<std::lcm(n,other)&&mod(solution,n)==a&&mod(solution,other)==b,"CRT solution");for(int x=0;x<solution;++x)require(mod(x,n)!=a||mod(x,other)!=b,"CRT solution not least");}
    }
    inspect(m,scene);
  }
  require(cases==5929,"CRT coverage changed");
  set(m,MathParameter::Modulus,3);set(m,MathParameter::SecondModulus,5);set(m,MathParameter::ModValue,2);set(m,MathParameter::SecondResidue,3);set(m,MathParameter::CrtGuess,8);checked(m,true);set(m,MathParameter::CrtGuess,23);checked(m,false);
  set(m,MathParameter::Modulus,4);set(m,MathParameter::SecondModulus,6);set(m,MathParameter::ModValue,0);set(m,MathParameter::SecondResidue,1);checked(m,false);require(m.snapshot().plots[0].seriesCount==2,"incompatible CRT claimed an intersection");inspect(m,scene);
}
struct G {int x=0,y=0;};
G product(G a,G b){return {a.x*b.x-a.y*b.y,a.x*b.y+a.y*b.x};}
bool equivalent(G a,G b,G g){const int x=a.x-b.x,y=a.y-b.y,n=g.x*g.x+g.y*g.y;return (x*g.x+y*g.y)%n==0&&(y*g.x-x*g.y)%n==0;}
void z(MathObjects& m,G v){set(m,MathParameter::GaussianReal,v.x);set(m,MathParameter::GaussianImag,v.y);}
void w(MathObjects& m,G v){set(m,MathParameter::GaussianOtherReal,v.x);set(m,MathParameter::GaussianOtherImag,v.y);}
void gaussian() {
  MathObjects m;MathObjectScene scene;select(m,MathObjectKind::Gaussian);checked(m,false);z(m,{1,1});w(m,{1,-1});checked(m,true);set(m,MathParameter::GaussianHeight,1);inspect(m,scene);
  for(std::size_t i=0;i<m.snapshot().partCount;++i){const auto& p=m.snapshot().parts[i];if(p.role=="real_axis"||p.role=="imaginary_axis")near(p.center.z,0,0,"height plot moved the coordinate axes");}
  level(m,1);unsigned divisions=0;
  for(int x=-3;x<=3;++x)for(int y=-3;y<=3;++y)for(int a=-2;a<=2;++a)for(int b=-2;b<=2;++b) {
    z(m,{x,y});w(m,{a,b});++divisions;const int nz=x*x+y*y,nw=a*a+b*b;near(metric(m,"Norm product"),nz*nw,0,"Gaussian norm multiplicativity");
    if(nw){const int qx=static_cast<int>(metric(m,"Quotient real")),qy=static_cast<int>(metric(m,"Quotient imaginary")),rx=static_cast<int>(metric(m,"Remainder real")),ry=static_cast<int>(metric(m,"Remainder imaginary"));require(a*qx-b*qy+rx==x&&a*qy+b*qx+ry==y,"division does not reconstruct dividend");require(rx*rx+ry*ry<nw,"Euclidean remainder norm bound");near(metric(m,"Divides exactly"),equivalent({x,y},{}, {a,b})?1:0,0,"divisibility mismatch");}
    else {near(metric(m,"Division defined"),0,0,"zero divisor accepted for division");checked(m,false);}
    if(x==3&&y==3)inspect(m,scene);
  }
  require(divisions==1225,"Gaussian division coverage changed");z(m,{2,2});w(m,{1,1});checked(m,true);
  level(m,2);w(m,{2,1});z(m,{-1,2});checked(m,true);inspect(m,scene);w(m,{0,0});z(m,{0,0});near(metric(m,"In principal ideal"),1,0,"zero ideal membership");near(metric(m,"Finite index"),0,0,"zero ideal has finite index");z(m,{1,0});near(metric(m,"In principal ideal"),0,0,"nonzero point in zero ideal");checked(m,false);inspect(m,scene);
  level(m,3);
  const std::array<G,3> generators{{{2,0},{2,1},{3,0}}};const std::array<unsigned,3> sizes{4,5,9},units{2,4,8},chars{2,5,3};
  for(unsigned preset=0;preset<3;++preset) {
    set(m,MathParameter::GaussianQuotient,preset);const auto n=sizes[preset];const auto g=generators[preset];std::array<G,9> reps{};
    const auto table=m.snapshot().table;require(table.rowCount==n,"wrong quotient size");near(metric(m,"Units"),units[preset],0,"unit count");near(metric(m,"Characteristic"),chars[preset],0,"quotient characteristic");near(metric(m,"Nonzero zero divisors"),preset==0?1:0,0,"zero divisor count");
    unsigned identity=0;for(unsigned i=0;i<n;++i){reps[i]={static_cast<int>(table.values[i][0]),static_cast<int>(table.values[i][1])};const int u=reps[i].x*g.x+reps[i].y*g.y,v=reps[i].y*g.x-reps[i].x*g.y;require(u>=0&&u<static_cast<int>(n)&&v>=0&&v<static_cast<int>(n),"representative outside half-open cell");if(equivalent(reps[i],{1,0},g))identity=i;for(unsigned j=0;j<i;++j)require(!equivalent(reps[i],reps[j],g),"duplicate residue class");}
    std::array<std::array<unsigned,9>,9> addition{},multiplication{};
    for(unsigned i=0;i<n;++i)for(unsigned j=0;j<n;++j) {
      z(m,reps[i]);w(m,reps[j]);set(m,MathParameter::GaussianOperation,0);addition[i][j]=static_cast<unsigned>(metric(m,"Result class"));set(m,MathParameter::GaussianOperation,1);multiplication[i][j]=static_cast<unsigned>(metric(m,"Result class"));
      require(addition[i][j]<n&&multiplication[i][j]<n,"class operation not closed");require(equivalent(reps[addition[i][j]],{reps[i].x+reps[j].x,reps[i].y+reps[j].y},g),"addition not well-defined");require(equivalent(reps[multiplication[i][j]],product(reps[i],reps[j]),g),"multiplication not well-defined");
    }
    for(unsigned i=0;i<n;++i)for(unsigned j=0;j<n;++j){require(multiplication[i][j]==multiplication[j][i]&&addition[i][j]==addition[j][i],"noncommutative quotient");require(multiplication[i][identity]==i&&addition[i][0]==i,"identity law");for(unsigned k=0;k<n;++k){require(multiplication[multiplication[i][j]][k]==multiplication[i][multiplication[j][k]],"multiplication associativity");require(addition[addition[i][j]][k]==addition[i][addition[j][k]],"addition associativity");require(multiplication[i][addition[j][k]]==addition[multiplication[i][j]][multiplication[i][k]],"distributive law");}}
    for(int x=-3;x<=3;++x)for(int y=-3;y<=3;++y){z(m,{x,y});const auto index=static_cast<unsigned>(metric(m,"z class"));require(index<n&&equivalent({x,y},reps[index],g),"incorrect lattice colouring class");}inspect(m,scene);
  }
  set(m,MathParameter::GaussianQuotient,0);z(m,{1,1});w(m,{1,1});checked(m,true);near(metric(m,"z squared class"),0,0,"nilpotent square");require(metric(m,"z class")!=0,"nilpotent was zero");
}
using V=std::array<double,3>;
V position(unsigned path,double t){const double a=pi*t;switch(path){case 0:return {-1+2*t,0,0};case 1:return {-std::cos(a),std::sin(a),0};case 2:return {-std::cos(a),-std::sin(a),0};case 3:return {-std::cos(a),std::sin(a),.5*std::sin(2*a)};default:return {std::cos(2*a),std::sin(2*a),0};}}
V field(unsigned rule,V p){if(rule==0)return {1,.5,-.25};if(rule==1)return p;return {-p[1],p[0],0};}
double independentIntegral(unsigned rule,unsigned path,double t,bool reversed) {
  // Composite Gauss-Legendre integration with numerical path derivatives.
  constexpr std::array<double,4> nodes{-.8611363115940526,-.3399810435848563,.3399810435848563,.8611363115940526};constexpr std::array<double,4> weights{.3478548451374538,.6521451548625461,.6521451548625461,.3478548451374538};double sum=0;
  for(unsigned segment=0;segment<16;++segment)for(unsigned j=0;j<4;++j){const double u=t*(segment+.5+.5*nodes[j])/16,time=reversed?1-u:u,eps=1e-6;const auto p=position(path,time),f=field(rule,p),before=position(path,time-eps),after=position(path,time+eps);double dot=0;for(unsigned k=0;k<3;++k)dot+=f[k]*(after[k]-before[k])/(2*eps)*(reversed?-1:1);sum+=weights[j]*dot*t/32;}return sum;
}
void fields() {
  MathObjects m;MathObjectScene scene;select(m,MathObjectKind::VectorField);checked(m,false);set(m,MathParameter::FieldX,1);set(m,MathParameter::FieldY,0);set(m,MathParameter::FieldZ,0);set(m,MathParameter::FieldYaw,90);set(m,MathParameter::FieldPitch,0);checked(m,true);inspect(m,scene);
  level(m,1);set(m,MathParameter::FieldPath,1);set(m,MathParameter::FieldTime,.5);checked(m,true);
  for(unsigned path=0;path<5;++path)for(double t:{.25,.5,.75}) {
    set(m,MathParameter::FieldPath,path);set(m,MathParameter::FieldTime,t);const auto snapshot=m.snapshot();const double speed=metric(m,"Path speed");set(m,MathParameter::FieldTime,t-.005);const auto before=m.snapshot().table.values[1];set(m,MathParameter::FieldTime,t+.005);const auto after=m.snapshot().table.values[1];
    for(unsigned i=0;i<3;++i)near(snapshot.table.values[3][i]*speed,(after[i]-before[i])/.01,.0011,"parameter velocity finite difference");inspect(m,scene);
  }
  level(m,3);
  for(unsigned rule=0;rule<3;++rule)for(unsigned path=0;path<5;++path)for(unsigned reverse=0;reverse<2;++reverse)for(double t:{0.0,.25,.5,.75,1.0}) {
    set(m,MathParameter::FieldRule,rule);set(m,MathParameter::FieldPath,path);if(reverse)action(m,{MathActionKind::ReverseFieldPath});set(m,MathParameter::FieldTime,t);
    near(metric(m,"Work so far"),independentIntegral(rule,path,t,reverse),2e-8,"line integral vs independent quadrature");near(metric(m,"Total work"),independentIntegral(rule,path,1,reverse),2e-8,"full line integral");require(metric(m,"Quadrature error")<2e-8,"analytic path integral error");
    if(rule<2)near(metric(m,"Path difference"),0,2e-8,"gradient field path dependence");else {const std::array<double,5> expected{0,-pi,pi,-pi,2*pi};near(metric(m,"Path difference"),expected[path]*(reverse?-1:1),2e-8,"vortex circulation");}
    near(m.snapshot().plots[2].marker.y,metric(m,"Work so far"),0,"unlinked work marker");inspect(m,scene);
  }
  set(m,MathParameter::FieldRule,2);set(m,MathParameter::FieldPath,3);set(m,MathParameter::FieldTime,.3);const auto before=m.snapshot().table.values[1];const double oldWork=metric(m,"Total work");action(m,{MathActionKind::ReverseFieldPath});for(unsigned i=0;i<3;++i)near(m.snapshot().table.values[1][i],before[i],1e-14,"reversal moved the probe");near(metric(m,"Total work"),-oldWork,2e-12,"reversal did not negate work");checked(m,true);
  level(m,2);set(m,MathParameter::FieldRule,2);set(m,MathParameter::FieldPath,4);action(m,{MathActionKind::ReverseFieldPath});set(m,MathParameter::FieldTime,1);checked(m,true);
  set(m,MathParameter::FieldTime,0);action(m,{MathActionKind::TogglePlayback});action(m,{MathActionKind::AdvanceTime,{},{},.2});near(m.parameter(MathParameter::FieldTime),.2,1e-14,"field playback duration");action(m,{MathActionKind::ReverseFieldPath});require(!m.snapshot().playing,"reverse did not pause");near(m.parameter(MathParameter::FieldTime),.8,1e-14,"reverse parameter mapping");action(m,{MathActionKind::TogglePlayback});action(m,{MathActionKind::AdvanceTime,{},{},12});require(!m.snapshot().playing&&m.parameter(MathParameter::FieldTime)==1,"path playback did not stop");reject(m,{MathActionKind::AdvanceTime,{},{},.1});
  level(m,0);reject(m,{MathActionKind::ReverseFieldPath});reject(m,{MathActionKind::TogglePlayback});require(!m.snapshot().fieldPathReversed,"orientation leaked to lower layer");
}
void boundaries() {
  MathObjects m;MathObjectScene scene;require(mathObjectSpecs().size()==static_cast<std::size_t>(MathObjectKind::Count),"wrong object count");const auto specs=mathParameterSpecs();for(std::size_t i=0;i<specs.size();++i)require(static_cast<std::size_t>(specs[i].id)==i&&!specs[i].key.empty(),"incomplete parameter catalogue");
  for(auto kind:{MathObjectKind::Modular,MathObjectKind::Gaussian,MathObjectKind::VectorField})for(unsigned n=0;n<4;++n) {
    select(m,kind,n);require(mathLessons(kind).size()==4,"missing four layers");
    for(unsigned pass=0;pass<2;++pass)for(const auto& p:specs)if(m.parameterAvailable(p.id)){set(m,p.id,pass?p.maximum:p.minimum);inspect(m,scene);reject(m,{MathActionKind::SetParameter,{},p.id,std::numeric_limits<double>::quiet_NaN()});reject(m,{MathActionKind::SetParameter,{},p.id,p.maximum+1});}
    reject(m,{MathActionKind::SetLevel,{},{},4});
  }
  select(m,MathObjectKind::Algebra);require(m.snapshot().table.rowCount==0,"table leaked across subjects");reject(m,{MathActionKind::ModularStep,{},{},1});reject(m,{MathActionKind::ResetModularWalk});reject(m,{MathActionKind::ReverseFieldPath});
}
}
int main(){try{modular();gaussian();fields();boundaries();std::printf("Numbers and fields passed: 5929 CRT cases, 1225 Gaussian divisions, quotient ring laws, independent path quadrature, challenges and %zu geometry states; maxima %zu vertices / %zu indices\n",states,maxVertices,maxIndices);return 0;}catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}
