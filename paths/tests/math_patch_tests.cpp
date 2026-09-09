#include "runtime/math_objects/BezierPatch.hpp"
#include "runtime/math_objects/PatchGeometry.hpp"
#include "runtime/math_objects/MathObjects.hpp"
#include "scene/MathObjectScene.hpp"
#include "ui/MathObjectLayout.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>

namespace {
using namespace paths;using V=BezierPoint;using P=MathParameter;
unsigned checks=0,meshes=0;std::size_t maxVertices=0,maxIndices=0;
void require(bool b,const char* why){++checks;if(!b)throw std::runtime_error(why);}
void near(double a,double b,double tolerance,const char* why){++checks;if(!std::isfinite(a)||!std::isfinite(b)||std::fabs(a-b)>tolerance)throw std::runtime_error(std::string(why)+": "+std::to_string(a)+" versus "+std::to_string(b));}
V difference(V a,V b){for(unsigned k=0;k<3;++k)a[k]-=b[k];return a;}
V cross(V a,V b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
double norm(V a){return std::hypot(a[0],a[1],a[2]);}
double dot(V a,V b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
// Independent power-basis oracle: expand each Bernstein polynomial once,
// then evaluate/differentiate its ordinary monomial coefficients.
V polynomial(const BicubicPatch& p,double u,double v,unsigned du=0,unsigned dv=0){
  constexpr double coefficients[4][4]{{1,0,0,0},{-3,3,0,0},{3,-6,3,0},{-1,3,-3,1}};V out{};
  for(unsigned a=du;a<4;++a)for(unsigned b=dv;b<4;++b){V c{};for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)for(unsigned k=0;k<3;++k)c[k]+=coefficients[a][i]*coefficients[b][j]*p.controls[4*i+j][k];
    double factor=std::pow(u,a-du)*std::pow(v,b-dv);for(unsigned k=0;k<du;++k)factor*=a-k;for(unsigned k=0;k<dv;++k)factor*=b-k;for(unsigned k=0;k<3;++k)out[k]+=c[k]*factor;
  }return out;
}
BicubicPatch graph(double a,double b,bool saddle=false){BicubicPatch p;constexpr std::array<double,4> square{1,-1./3,-1./3,1};for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j){const double x=-1+2.*i/3,z=1-2.*j/3;p.controls[4*i+j]={x,saddle?a*x*z:a*square[i]+b*square[j],z};}return p;}
void kernel(){
  for(unsigned specimen=0;specimen<5;++specimen){BicubicPatch p;for(unsigned i=0;i<16;++i)for(unsigned k=0;k<3;++k)p.controls[i][k]=1.7*std::sin((i+1)*(k+1)*(specimen+1)*.173);
    for(unsigned i=0;i<=8;++i)for(unsigned j=0;j<=8;++j){const double u=i/8.,v=j/8.;const auto s=samplePatch(p,u,v);const std::array values{s.position,s.du,s.dv,s.duu,s.duv,s.dvv};constexpr std::array<unsigned,6> us{0,1,0,2,1,0},vs{0,0,1,0,1,2};
      for(unsigned q=0;q<values.size();++q){const auto oracle=polynomial(p,u,v,us[q],vs[q]);for(unsigned k=0;k<3;++k)near(values[q][k],oracle[k],2e-12,"power-basis derivative");}
      double weights=0;for(double w:s.weights){require(w>=0&&w<=1,"invalid Bernstein weight");weights+=w;}near(weights,1,2e-15,"partition of unity");
      for(unsigned k=0;k<3;++k){double lo=2,hi=-2;for(auto c:p.controls){lo=std::min(lo,c[k]);hi=std::max(hi,c[k]);}require(s.position[k]>=lo-1e-12&&s.position[k]<=hi+1e-12,"patch leaves control bounds");}
      if(s.regular){near(norm(s.normal),1,2e-14,"unit normal");near(dot(s.normal,s.du),0,1e-12,"normal not perpendicular to u");near(dot(s.normal,s.dv),0,1e-12,"normal not perpendicular to v");near(s.principalMin*s.principalMax,s.gaussian,1e-7*std::max(1.0,std::fabs(s.gaussian)),"principal curvature product");near((s.principalMin+s.principalMax)/2,s.mean,1e-8*std::max(1.0,std::fabs(s.mean)),"principal curvature mean");}
    }
    for(unsigned side=0;side<4;++side){CubicBezier edge;for(unsigned i=0;i<4;++i)edge.controls[i]=p.controls[side==0?i:side==1?12+i:side==2?4*i:4*i+3];for(unsigned t=0;t<=16;++t){const double f=t/16.;const auto a=sampleBezier(edge,f).position,b=samplePatch(p,side<2?static_cast<double>(side):f,side<2?f:static_cast<double>(side-2)).position;for(unsigned k=0;k<3;++k)near(a[k],b[k],2e-14,"boundary curve disagreement");}}
  }
  for(double a:{-.4,0.,.35})for(double b:{-.2,0.,.6}){const auto p=graph(a,b);for(double u:{0.,.15,.5,.85,1.})for(double v:{0.,.3,.5,1.}){const auto s=samplePatch(p,u,v);const double x=2*u-1,z=1-2*v,w=1+4*a*a*x*x+4*b*b*z*z;
    require(s.regular,"regular graph rejected");near(s.position[1],a*x*x+b*z*z,1e-13,"quadratic height");near(s.jacobian,4*std::sqrt(w),1e-12,"graph area density");near(s.gaussian,4*a*b/(w*w),1e-12,"graph Gaussian curvature");near(s.mean,(a*(1+4*b*b*z*z)+b*(1+4*a*a*x*x))/std::pow(w,1.5),1e-12,"graph mean curvature");
  }}
  // Strongly unequal principal curvatures: the small root must not cancel.
  const auto uneven=samplePatch(graph(.5,1e-12),.5,.5);near(uneven.principalMin,2e-12,1e-15,"small principal curvature cancellation");near(uneven.principalMax,1,1e-13,"large principal curvature");
  const auto saddle=graph(.7,0,true);const auto middle=samplePatch(saddle,.5,.5);near(middle.gaussian,-.49,1e-13,"saddle Gaussian curvature");near(middle.mean,0,1e-13,"saddle mean curvature");near(middle.principalMin,-.7,1e-13,"saddle principal curvature");
  BicubicPatch plane;for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j){const double x=-1+2.*i/3,z=1-2.*j/3;plane.controls[4*i+j]={x,.3*x-.4*z,z};}
  for(unsigned n:{1,2,4,16,32}){near(integratePatchArea(plane,n),4*std::sqrt(1.25),2e-12,"exact plane quadrature");const auto mesh=measurePatchMesh(plane,n);near(mesh.area,4*std::sqrt(1.25),2e-12,"exact triangulated plane");require(mesh.triangles==2*n*n&&!mesh.skipped,"plane triangles missing");}
  // y=a*x^2 is extruded across z. Its area has an independent asinh integral.
  for(double a:{.1,.4,.8}){const auto p=graph(a,0);const double c=2*a,area=2*(std::sqrt(1+c*c)+std::asinh(c)/c);near(integratePatchArea(p,32),area,2e-8,"parabolic cylinder area integral");require(std::fabs(measurePatchMesh(p,32).area-area)<std::fabs(measurePatchMesh(p,4).area-area),"area mesh does not converge");}
  const auto p=graph(.3,.6);BicubicPatch reversed,scaled;
  for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j){reversed.controls[4*i+j]=p.controls[4*j+i];for(unsigned k=0;k<3;++k)scaled.controls[4*i+j][k]=p.controls[4*i+j][k]*.5+.1;}
  const auto s=samplePatch(p,.2,.7),r=samplePatch(reversed,.7,.2),t=samplePatch(scaled,.2,.7);near(r.gaussian,s.gaussian,1e-13,"reversal changed K");near(r.mean,-s.mean,1e-13,"reversal did not reverse H");near(dot(r.normal,s.normal),-1,1e-13,"reversal normal");near(t.gaussian,4*s.gaussian,1e-12,"scale law K");near(t.mean,2*s.mean,1e-12,"scale law H");near(t.jacobian,.25*s.jacobian,1e-13,"scale law area density");
  BicubicPatch collapsed;for(auto& c:collapsed.controls)c={.4,.2,-.1};const auto singular=samplePatch(collapsed,.5,.5);require(!singular.regular&&norm(singular.normal)==0,"collapsed patch reports a normal");near(integratePatchArea(collapsed,8),0,1e-25,"point has area");require(measurePatchMesh(collapsed,8).triangles==0,"point made triangles");
  for(unsigned i=0;i<16;++i)collapsed.controls[i]={-1+2.*i/15,0,0};require(!samplePatch(collapsed,.2,.7).regular,"line patch regular");require(measurePatchMesh(collapsed,8).triangles==0,"line has triangle area");
  BicubicPatch fold;constexpr std::array<double,4> squared{1,-1./3,-1./3,1};
  for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)fold.controls[4*i+j]={squared[i],0,1-2.*j/3};
  require(!samplePatch(fold,.5,.4).regular,"fold crease reports a normal");near(integratePatchArea(fold,8),4,1e-12,"folded area must count both covers");near(measurePatchMesh(fold,8).area,4,1e-12,"folded mesh area multiplicity");
  for(unsigned which=0;which<5;++which){bool rejected=false;try{auto bad=p;switch(which){case 0:bad.controls[0][0]=std::numeric_limits<double>::quiet_NaN();(void)samplePatch(bad,.5,.5);break;case 1:bad.controls[0][0]=2.01;(void)samplePatch(bad,.5,.5);break;case 2:(void)samplePatch(p,-.01,.5);break;case 3:(void)integratePatchArea(p,0);break;case 4:(void)measurePatchMesh(p,33);break;}}catch(const std::invalid_argument&){rejected=true;}require(rejected,"invalid kernel input accepted");}
}
void action(MathObjects& m,MathAction a){const auto r=m.dispatch(a);if(!r.accepted)throw std::runtime_error(std::string(r.reason));}
void set(MathObjects& m,P p,double v){action(m,{MathActionKind::SetParameter,{},p,v});}
void level(MathObjects& m,unsigned l){action(m,{MathActionKind::SetLevel,{},{},static_cast<double>(l)});}
BicubicPatch patch(const MathObjects& m){BicubicPatch p;for(unsigned i=0;i<16;++i)for(unsigned k=0;k<3;++k)p.controls[i][k]=m.parameter(static_cast<P>(static_cast<unsigned>(P::PatchP00X)+3*i+k));return p;}
double metric(const MathObjects& m,std::string_view label){for(unsigned i=0;i<m.snapshot().metricCount;++i)if(m.snapshot().metrics[i].label==label)return m.snapshot().metrics[i].value;throw std::runtime_error("missing metric "+std::string(label));}
void check(MathObjects& m,bool expected){action(m,{MathActionKind::Check});require((m.snapshot().feedback==MathFeedback::Solved)==expected,"wrong patch challenge result");}
void inspect(MathObjects& m,MathObjectScene& scene,bool disk=true){
  const auto& s=m.snapshot();const auto& frame=scene.publish(s,{0,0,1062,560});++meshes;maxVertices=std::max(maxVertices,frame.vertices.size());maxIndices=std::max(maxIndices,frame.indices.size());require(frame.vertices.size()<=kSceneVertexCapacity&&frame.indices.size()<=kSceneIndexCapacity,"scene capacity exceeded");require(!frame.vertices.empty()&&iggy3d::isFinite(frame.clipFromWorld),"empty scene or invalid camera");
  for(const auto& v:frame.vertices){for(float x:v.position)require(std::isfinite(x),"nonfinite vertex");for(float c:v.color)require(std::isfinite(c)&&c>=0&&c<=1,"invalid colour");}
  for(auto i:frame.indices)require(i<frame.vertices.size(),"invalid native index");std::set<unsigned> ids;std::size_t end=0;for(auto d:frame.draws){require(ids.insert(d.objectId.value).second&&d.firstIndex==end,"duplicate ID or unowned indices");end+=d.indexCount;}require(end==frame.indices.size(),"trailing unowned indices");
  for(unsigned i=0;i<s.metricCount;++i)require(std::isfinite(s.metrics[i].value),"nonfinite metric");for(unsigned i=0;i<s.table.rowCount;++i)for(unsigned j=0;j<s.table.columnCount;++j)require(std::isfinite(s.table.values[i][j]),"nonfinite table");
  for(unsigned i=0;i<s.plotCount;++i)for(unsigned j=0;j<s.plots[i].seriesCount;++j){const auto& q=s.plots[i].series[j];for(unsigned k=0;k<q.count;++k)require(std::isfinite(q.points[k].x)&&std::isfinite(q.points[k].y),"undefined value plotted as a number");}
  if(!disk)return;const unsigned n=static_cast<unsigned>(m.parameter(P::PatchResolution));require(s.solid.vertexCount==(n+1)*(n+1)&&s.solid.indexCount==6*n*n,"regular grid counts");
  std::map<std::pair<unsigned,unsigned>,std::pair<unsigned,int>> edges;const auto p=patch(m);double area=0;
  for(unsigned k=0;k<s.solid.indexCount;k+=3){const unsigned a=s.solid.indices[k],b=s.solid.indices[k+1],c=s.solid.indices[k+2];for(auto e:std::array<std::pair<unsigned,unsigned>,3>{{{a,b},{b,c},{c,a}}}){const auto key=std::minmax(e.first,e.second);auto& value=edges[key];++value.first;value.second+=e.first<e.second?1:-1;}
    const auto point=[&](unsigned index){return polynomial(p,static_cast<double>(index/(n+1))/n,static_cast<double>(index%(n+1))/n);};const auto x=point(a),y=point(b),z=point(c);area+=norm(cross(difference(y,x),difference(z,x)))/2;
  }
  unsigned boundary=0;for(auto [key,value]:edges){require(value.first==1||value.first==2,"nonmanifold patch edge");if(value.first==1)++boundary;else require(value.second==0,"inconsistent triangle winding");}require(boundary==4*n,"wrong open boundary");require(static_cast<int>(s.solid.vertexCount)-static_cast<int>(edges.size())+static_cast<int>(s.solid.indexCount/3)==1,"patch disk Euler characteristic");
  near(area,measurePatchMesh(p,n).area,2e-10,"independent tessellated area");
}
void model(){
  static_assert(static_cast<unsigned>(P::Count)>255,"new controls exercise the widened parameter IDs");
  MathObjects m;MathObjectScene scene;action(m,{MathActionKind::Select,MathObjectKind::Patch});require(mathLessons(MathObjectKind::Patch).size()==4,"patch layers absent");
  std::set<std::string_view> keys;for(const auto& p:mathParameterSpecs())require(keys.insert(p.key).second,"duplicate parameter key");
  for(unsigned l=0;l<4;++l){level(m,l);for(unsigned preset=0;preset<4;++preset){const auto rev=m.snapshot().revision;action(m,{MathActionKind::ObjectPreset,{},{},0,preset});require(m.snapshot().revision==rev+1,"preset not atomic");const auto& spec=mathObjectPresets(MathObjectKind::Patch,l)[preset];require(spec.count==53,"preset lost fields");for(unsigned j=0;j<spec.count;++j)near(m.parameter(spec.parameters[j]),spec.values[j],1e-14,"preset field mismatch");
    for(double n:{4.,20.,32.}){set(m,P::PatchResolution,n);inspect(m,scene);require(m.snapshot().curve.count==16&&m.snapshot().curve.selectionParameter==P::PatchControl,"picker contract");}
    if(l==2){const auto s=samplePatch(patch(m),.5,.5);near(metric(m,"Gaussian curvature"),s.gaussian,1e-12,"curvature metric disagrees with patch");}
    set(m,P::PatchGuides,0);inspect(m,scene);require(!m.snapshot().curve.active&&m.snapshot().partCount==0,"shape-only still has guides");set(m,P::PatchGuides,1);
  }}
  level(m,0);MathInspectorMemory memory;memory.visit(m);action(m,{MathActionKind::ObjectPreset,{},{},0,0});memory.rememberExample(m,"Canopy");
  for(unsigned selected=0;selected<16;++selected){set(m,P::PatchControl,selected);const auto rows=mathControlRows(m);unsigned controls=0;for(unsigned i=0;i<rows.count;++i)for(unsigned j=0;j<rows.rows[i].count;++j){const auto p=rows.rows[i].parameters[j];if(p>=P::PatchP00X&&p<=P::PatchP33Z){require((static_cast<unsigned>(p)-static_cast<unsigned>(P::PatchP00X))/3==selected,"wrong point's controls visible");++controls;}}require(controls==3,"selected XYZ not available");require(memory.exampleTitle(m)=="Canopy","handle selection customizes shape");}
  set(m,P::PatchP33Y,.7);require(memory.exampleTitle(m)=="Custom","shape edit not marked Custom");const auto rev=m.snapshot().revision;action(m,mathResetControlGroup(m,MathControlGroup::Profile));require(m.snapshot().revision==rev+1&&mathChangedControlCount(m,MathControlGroup::Profile)==0,"hidden controls not reset atomically");
  set(m,P::PatchControl,0);set(m,P::PatchU,0);set(m,P::PatchV,0);check(m,true);set(m,P::PatchU,.5);check(m,false);
  level(m,1);action(m,{MathActionKind::ObjectPreset,{},{},0,1});check(m,true);level(m,2);action(m,{MathActionKind::ObjectPreset,{},{},0,3});check(m,true);level(m,3);action(m,{MathActionKind::ObjectPreset,{},{},0,0});set(m,P::PatchResolution,32);check(m,true);
  set(m,P::PatchP12Y,1.2);level(m,0);level(m,3);near(m.parameter(P::PatchP12Y),1.2,1e-14,"layer switch lost shape");
  const auto before=m.snapshot().revision;require(!m.dispatch({MathActionKind::SetParameter,{},P::PatchP12Y,2.1}).accepted&&!m.dispatch({MathActionKind::SetParameter,{},P::LatheH1,.1}).accepted,"invalid edit accepted");require(m.snapshot().revision==before,"invalid edit mutated model");
  for(unsigned i=0;i<48;++i)set(m,static_cast<P>(static_cast<unsigned>(P::PatchP00X)+i),0);set(m,P::PatchGuides,0);inspect(m,scene,false);require(m.snapshot().curve.active&&m.snapshot().solid.indexCount==0,"collapsed patch has no recovery controls");check(m,false);
  level(m,1);check(m,false);level(m,2);check(m,false);require(m.snapshot().plotCount==0&&m.snapshot().matrixCount==1,"singular patch fabricated curvature data");inspect(m,scene,false);
  action(m,{MathActionKind::ObjectPreset,{},{},0,0});
  for(unsigned j=1;j<4;++j)for(unsigned k=0;k<3;++k)set(m,static_cast<P>(static_cast<unsigned>(P::PatchP00X)+3*j+k),m.parameter(static_cast<P>(static_cast<unsigned>(P::PatchP00X)+k)));
  set(m,P::PatchU,0);set(m,P::PatchV,.5);require(metric(m,"Regular probe")==0&&m.snapshot().plotCount==0,"collapsed boundary reported a regular normal or trace");inspect(m,scene,false);
  set(m,P::PatchU,.5);require(metric(m,"Regular probe")==1&&metric(m,"Curvature trace available")==0,"regular probe must not hide a singular trace endpoint");inspect(m,scene,false);
  for(unsigned seed=1;seed<=4;++seed){for(unsigned i=0;i<48;++i)set(m,static_cast<P>(static_cast<unsigned>(P::PatchP00X)+i),std::round(180*std::sin((i+1)*seed*.419))/100);inspect(m,scene,false);}
  action(m,{MathActionKind::Select,MathObjectKind::Curve});require(m.snapshot().curve.count==4,"curve picker regression");action(m,{MathActionKind::Select,MathObjectKind::Lathe});require(m.snapshot().curve.count==7,"lathe picker regression");action(m,{MathActionKind::Select,MathObjectKind::Algebra});require(!m.snapshot().solid.indexCount,"patch mesh leaked to another object");
  auto p=graph(.2,.3);MathTriangleSurface mesh;mesh.indexCount=3;mesh.vertexCount=1;bool rejected=false;try{buildPatchSurface(p,{33,0,PatchColour::Material},mesh);}catch(const std::invalid_argument&){rejected=true;}require(rejected&&mesh.indexCount==3&&mesh.vertexCount==1,"invalid mesh request changed output");
}
}
int main(){try{kernel();model();std::printf("patch CPU checks: %u assertions, %u mesh states; maxima %zu vertices / %zu indices; no host or images\n",checks,meshes,maxVertices,maxIndices);return 0;}catch(const std::exception& e){std::fprintf(stderr,"patch failure: %s\n",e.what());return 1;}}
