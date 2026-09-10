#include "runtime/math_objects/MathObjects.hpp"
#include "scene/MathObjectScene.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>

namespace {
using namespace paths;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void near(double a,double b,double tolerance,const char* message){if(!std::isfinite(a)||std::fabs(a-b)>tolerance)throw std::runtime_error(std::string(message)+": "+std::to_string(a)+" vs "+std::to_string(b));}
void action(MathObjects& m,MathAction a){const auto r=m.dispatch(a);if(!r.accepted)throw std::runtime_error(std::string(r.reason));}
void select(MathObjects& m,MathObjectKind k,unsigned level){action(m,{MathActionKind::Select,k});action(m,{MathActionKind::SetLevel,{},{},static_cast<double>(level)});}
void set(MathObjects& m,MathParameter p,double value){action(m,{MathActionKind::SetParameter,{},p,value});}
double metric(const MathObjects& m,std::string_view name){const auto& s=m.snapshot();for(std::size_t i=0;i<s.metricCount;++i)if(s.metrics[i].label==name)return s.metrics[i].value;throw std::runtime_error("missing metric "+std::string(name));}
void checked(MathObjects& m,bool solved){action(m,{MathActionKind::Check});require((m.snapshot().feedback==MathFeedback::Solved)==solved,"wrong challenge verdict");}
void reject(MathObjects& m,MathAction a){const auto revision=m.snapshot().revision;require(!m.dispatch(a).accepted,"invalid action accepted");require(m.snapshot().revision==revision,"rejection changed revision");}
double f(unsigned rule,double x){switch(rule){case 0:return x*x;case 1:return x*x*x-3*x;case 2:return std::sin(x);default:return std::expm1(x);}}
double integral(unsigned rule,double a,double b){constexpr unsigned n=1024;const double h=(b-a)/n;double sum=f(rule,a)+f(rule,b);for(unsigned i=1;i<n;++i)sum+=(i%2?4:2)*f(rule,a+i*h);return sum*h/3;}
double surface(unsigned rule,double u,double v){switch(rule){case 0:return (u*u+v*v)/2;case 1:return (u*u-v*v)/2;default:return std::sin(u)*std::cos(v);}}
std::size_t maxVertices=0,maxIndices=0,maxContours=0,frames=0;
void inspect(const MathObjects& m,MathObjectScene& scene) {
  const auto& s=m.snapshot();const auto& frame=scene.publish(s,{10,20,950,640});++frames;
  require(!frame.vertices.empty()&&frame.vertices.size()<=kSceneVertexCapacity&&frame.indices.size()<=kSceneIndexCapacity,"mesh capacity exceeded");
  maxVertices=std::max(maxVertices,frame.vertices.size());maxIndices=std::max(maxIndices,frame.indices.size());maxContours=std::max(maxContours,s.contours.count);
  std::set<std::uint32_t> ids;std::size_t end=0;
  for(const auto& d:frame.draws){require(ids.insert(d.objectId.value).second,"duplicate draw ID");require(d.firstIndex==end&&d.indexCount%3==0,"invalid draw interval");end+=d.indexCount;require(iggy3d::isFinite(d.bounds.min)&&iggy3d::isFinite(d.bounds.max),"invalid bounds");}
  require(end==frame.indices.size(),"unowned triangles");
  for(const auto& v:frame.vertices){for(float n:v.position)require(std::isfinite(n),"nonfinite position");for(float n:v.color)require(std::isfinite(n)&&n>=0&&n<=1,"invalid colour");}
  for(auto i:frame.indices)require(i<frame.vertices.size(),"index outside mesh");
  require(iggy3d::isFinite(frame.clipFromWorld),"invalid camera");
  for(std::size_t p=0;p<s.plotCount;++p){const auto& plot=s.plots[p];require(plot.seriesCount>0&&plot.seriesCount<=plot.series.size(),"invalid plot");for(std::size_t line=0;line<plot.seriesCount;++line){const auto& series=plot.series[line];require(series.count<=series.points.size(),"plot overflow");for(std::size_t i=0;i<series.count;++i)require(std::isfinite(series.points[i].x)&&std::isfinite(series.points[i].y),"nonfinite plot");}}
  for(std::size_t i=0;i<s.metricCount;++i)require(std::isfinite(s.metrics[i].value),"nonfinite metric");
  for(std::size_t i=0;i<s.matrixCount;++i)for(double v:s.matrices[i].values)require(std::isfinite(v),"nonfinite matrix");
}
std::size_t plotStates=0,plotViews=0,plotSamples=0;
void plotContracts(){
  const auto parameters=mathParameterSpecs();
  for(const auto& object:mathObjectSpecs()){
    const unsigned levels=std::max<std::size_t>(1,mathLessons(object.id).size());
    for(unsigned level=0;level<levels;++level){
      const auto presets=mathObjectPresets(object.id,level);
      for(unsigned example=0;example<=presets.size();++example){
        MathObjects m;select(m,object.id,level);
        if(example<presets.size())action(m,{MathActionKind::ObjectPreset,{},{},0,example});
        const auto& state=m.snapshot();++plotStates;
        require(state.plotCount<=state.plots.size(),"plot capacity exceeded");
        for(unsigned i=0;i<state.plotCount;++i){
          const auto& plot=state.plots[i];++plotViews;
          require(!plot.title.empty()&&plot.seriesCount>0&&plot.seriesCount<=plot.series.size(),"invalid plot declaration");
          if(plot.scrubParameter!=MathParameter::Count){
            const auto index=static_cast<unsigned>(plot.scrubParameter);
            require(index<parameters.size()&&parameters[index].owner==object.id,"plot scrubs another model's parameter");
          }
          if(plot.hasMarker)require(std::isfinite(plot.marker.x)&&std::isfinite(plot.marker.y),"nonfinite plot marker");
          for(unsigned j=0;j<plot.seriesCount;++j){
            const auto& series=plot.series[j];plotSamples+=series.count;
            require(!series.name.empty()&&series.count<=series.points.size(),"invalid plot series");
            require(iggy3d::isFinite(series.color),"nonfinite plot colour");
            for(unsigned k=0;k<series.count;++k)require(std::isfinite(series.points[k].x)&&std::isfinite(series.points[k].y),"nonfinite plot sample");
          }
        }
      }
    }
  }
}
void functions() {
  MathObjects m;MathObjectScene scene;select(m,MathObjectKind::Function,0);
  reject(m,{MathActionKind::SetParameter,{},MathParameter::DeltaX,.1});reject(m,{MathActionKind::SetLevel,{},{},1.5});reject(m,{MathActionKind::SetLevel,{},{},4});
  reject(m,{MathActionKind::DescentStep});reject(m,{MathActionKind::SwapBounds});
  select(m,MathObjectKind::Function,3);
  for(unsigned rule=0;rule<4;++rule)for(double x:{-2.0,-.75,0.0,.75,2.0}) {
    set(m,MathParameter::FunctionRule,rule);set(m,MathParameter::FunctionX,x);set(m,MathParameter::IntegralStart,.5);
    near(metric(m,"f(x)"),f(rule,x),1e-12,"function evaluation");
    near(metric(m,"f'(x)"),(f(rule,x+1e-5)-f(rule,x-1e-5))/2e-5,2e-8,"derivative finite difference");
    near(metric(m,"Signed integral"),integral(rule,.5,x),1e-10,"integral quadrature");
    const auto& s=m.snapshot();require(s.plotCount==3,"missing linked views");
    near(s.plots[0].marker.y,metric(m,"f(x)"),1e-12,"function marker");near(s.plots[1].marker.y,metric(m,"f'(x)"),1e-12,"derivative marker");near(s.plots[2].marker.y,metric(m,"Signed integral"),1e-12,"integral marker");
    for(const auto& p:s.plots)near(p.marker.x,x,1e-12,"unlinked input marker");
    for(double h:{-1.0,-.005,0.0,.005,1.0}){set(m,MathParameter::DeltaX,h);require(std::isfinite(metric(m,"Secant slope")),"secant singularity");inspect(m,scene);}
    set(m,MathParameter::TaylorDegree,rule==0?2:3);
    if(rule<2)near(metric(m,"Taylor value"),f(rule,x),1e-10,"polynomial Taylor exactness");
  }
  select(m,MathObjectKind::Function,1);set(m,MathParameter::FunctionRule,0);set(m,MathParameter::DeltaX,0);checked(m,false);
  set(m,MathParameter::DeltaX,.005);checked(m,true);
  action(m,{MathActionKind::SetLevel,{},{},2});set(m,MathParameter::FunctionX,0);set(m,MathParameter::IntegralStart,1);checked(m,false);
  const auto negative=metric(m,"Signed integral");action(m,{MathActionKind::SwapBounds});near(metric(m,"Signed integral"),-negative,1e-12,"bound reversal");checked(m,true);
  set(m,MathParameter::FunctionX,1);checked(m,false);
  action(m,{MathActionKind::SetLevel,{},{},3});set(m,MathParameter::FunctionRule,2);set(m,MathParameter::FunctionX,.5);set(m,MathParameter::TaylorDegree,1);const auto lowError=metric(m,"Taylor error");
  set(m,MathParameter::TaylorDegree,5);require(metric(m,"Taylor error")<lowError,"Taylor approximation did not improve");checked(m,true);
  const auto x=m.parameter(MathParameter::FunctionX);reject(m,{MathActionKind::SetParameter,{},MathParameter::FunctionX,std::numeric_limits<double>::quiet_NaN()});near(m.parameter(MathParameter::FunctionX),x,0,"NaN changed input");
  action(m,{MathActionKind::SetLevel,{},{},0});require(m.snapshot().plotCount==1&&!m.parameterAvailable(MathParameter::DeltaX),"lower layer leaked advanced state");
}
void surfaces() {
  MathObjects m;MathObjectScene scene;select(m,MathObjectKind::Surface,3);
  for(unsigned rule=0;rule<3;++rule)for(double u:{-2.0,-.75,0.0,.75,2.0})for(double v:{-2.0,0.0,2.0}) {
    set(m,MathParameter::SurfaceRule,rule);set(m,MathParameter::SurfaceU,u);set(m,MathParameter::SurfaceV,v);
    near(metric(m,"Height"),surface(rule,u,v),1e-12,"surface height");
    near(metric(m,"Partial u"),(surface(rule,u+1e-5,v)-surface(rule,u-1e-5,v))/2e-5,1e-9,"partial u");
    near(metric(m,"Partial v"),(surface(rule,u,v+1e-5)-surface(rule,u,v-1e-5))/2e-5,1e-9,"partial v");
    const auto& s=m.snapshot();near(s.plots[0].marker.x,u,1e-12,"u section position");near(s.plots[1].marker.x,v,1e-12,"v section position");
    near(s.plots[0].marker.y,metric(m,"Height"),1e-12,"section height");near(s.contours.point.x,u,1e-12,"contour position");
    require(s.contours.count>0,"missing contours");
    for(std::size_t i=0;i<s.contours.count;++i)for(const auto p:{s.contours.segments[i].a,s.contours.segments[i].b}) {
      require(std::fabs(p.x)<=2.000001&&std::fabs(p.y)<=2.000001,"contour outside domain");near(surface(rule,p.x,p.y),s.contours.segments[i].height,.021,"contour interpolation error");
    }
    inspect(m,scene);
  }
  select(m,MathObjectKind::Surface,2);set(m,MathParameter::SurfaceRule,0);set(m,MathParameter::SurfaceU,1);set(m,MathParameter::SurfaceV,1);
  for(unsigned i=0;i<18;++i){const auto old=metric(m,"Height");action(m,{MathActionKind::DescentStep});require(metric(m,"Height")<old,"descent increased height");}checked(m,true);
  set(m,MathParameter::SurfaceRule,1);set(m,MathParameter::SurfaceU,0);set(m,MathParameter::SurfaceV,2);reject(m,{MathActionKind::DescentStep});
  action(m,{MathActionKind::SetLevel,{},{},3});set(m,MathParameter::Constraint,1);
  reject(m,{MathActionKind::SetParameter,{},MathParameter::SurfaceU,.5});reject(m,{MathActionKind::MoveSurfacePoint,{},{},.5,0,.5});reject(m,{MathActionKind::DescentStep});
  for(double angle:{0.0,90.0,180.0,270.0}){set(m,MathParameter::CircleAngle,angle);near(metric(m,"Constraint error"),0,1e-12,"constraint radius");require(metric(m,"Gradient magnitude")>.9,"constrained stationary gradient vanished");checked(m,true);inspect(m,scene);}
  set(m,MathParameter::CircleAngle,45);checked(m,false);near(std::fabs(metric(m,"Tangential derivative")),1,1e-12,"circle tangent slope");
  set(m,MathParameter::Constraint,0);reject(m,{MathActionKind::MoveSurfacePoint,{},{},.5,0,std::numeric_limits<double>::infinity()});
  action(m,{MathActionKind::MoveSurfacePoint,{},{},.25,0,-.75});near(m.snapshot().contours.point.x,.25,0,"contour scrub u");near(m.snapshot().contours.point.y,-.75,0,"contour scrub v");
  select(m,MathObjectKind::Surface,1);set(m,MathParameter::SurfaceRule,0);set(m,MathParameter::SurfaceU,1);set(m,MathParameter::SurfaceV,0);set(m,MathParameter::DirectionAngle,90);checked(m,true);
  action(m,{MathActionKind::SetLevel,{},{},0});set(m,MathParameter::SurfaceV,1);checked(m,true);
}
using Matrix=std::array<double,9>;
void setMatrix(MathObjects& m,const Matrix& a){const auto keys=m.snapshot().matrices[0].parameters;for(unsigned i=0;i<9;++i)set(m,keys[i],a[i]);}
void matrices() {
  MathObjects m;MathObjectScene scene;select(m,MathObjectKind::Linear,1);
  action(m,{MathActionKind::MatrixPreset,{},{},0,3});set(m,MathParameter::ComposeAngle,90);
  const auto& ba=m.snapshot().matrices[1].values;near(ba[1],-1,1e-12,"B A order");near(m.snapshot().matrices[2].values[1],-2,1e-12,"A B order");
  require(metric(m,"Composition order difference")>1,"lost noncommutativity");
  set(m,MathParameter::VectorY,0);set(m,MathParameter::VectorZ,0);checked(m,true);set(m,MathParameter::VectorX,0);checked(m,false);
  action(m,{MathActionKind::SetLevel,{},{},2});action(m,{MathActionKind::MatrixPreset,{},{},0,0});
  set(m,MathParameter::VectorX,1);set(m,MathParameter::VectorY,1);set(m,MathParameter::VectorZ,1);
  near(metric(m,"Residual length"),1,1e-6,"projection residual");near(metric(m,"Orthogonality error"),0,1e-6,"projection orthogonality");checked(m,false);
  set(m,MathParameter::VectorZ,0);checked(m,true);
  setMatrix(m,{1,1,0,0,0,0,0,0,1});near(metric(m,"Subspace dimension"),1,0,"dependent column span");near(metric(m,"Residual length"),1,1e-6,"line projection");
  setMatrix(m,{0,0,0,0,0,0,0,0,1});near(metric(m,"Subspace dimension"),0,0,"zero column span");inspect(m,scene);
  action(m,{MathActionKind::SetLevel,{},{},3});
  std::uint32_t seed=923871;const auto random=[&](){seed=seed*1664525U+1013904223U;return seed;};
  for(unsigned trial=0;trial<48;++trial) {
    Matrix input{};for(unsigned i=0;i<9;++i)input[i]=(static_cast<int>(random()%21)-10)*.1;
    if(trial==0)input={};if(trial==1)input={1,0,0,0,1,0,0,0,1};if(trial==2)input={1,1,0,1,1,0,0,0,0};if(trial==3)input={-2,0,0,0,1,0,0,0,0};
    setMatrix(m,input);const auto& s=m.snapshot();const auto& u=s.matrices[1].values;const auto& vt=s.matrices[2].values;
    const std::array<double,3> sigma{metric(m,"Singular value 1"),metric(m,"Singular value 2"),metric(m,"Singular value 3")};
    require(sigma[0]>=sigma[1]&&sigma[1]>=sigma[2]&&sigma[2]>=0,"singular value order");
    double norm=0;for(auto a:input)norm+=a*a;near(sigma[0]*sigma[0]+sigma[1]*sigma[1]+sigma[2]*sigma[2],norm,1e-8,"singular value norm conservation");
    for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c) {
      double reconstructed=0,orthU=0,orthV=0;for(unsigned k=0;k<3;++k){reconstructed+=u[3*r+k]*sigma[k]*vt[3*k+c];orthU+=u[3*k+r]*u[3*k+c];orthV+=vt[3*r+k]*vt[3*c+k];}
      near(reconstructed,input[3*r+c],1e-7,"SVD reconstruction");near(orthU,r==c?1:0,1e-6,"U orthogonality");near(orthV,r==c?1:0,1e-6,"V orthogonality");
    }
    for(unsigned stage=0;stage<4;++stage){set(m,MathParameter::SvdStage,stage);inspect(m,scene);}
  }
  action(m,{MathActionKind::MatrixPreset,{},{},0,2});checked(m,true);
  action(m,{MathActionKind::MatrixPreset,{},{},0,0});checked(m,false);
  action(m,{MathActionKind::SetLevel,{},{},0});reject(m,{MathActionKind::SetParameter,{},MathParameter::A10,1});require(!m.snapshot().matrices[0].editable,"base layer exposes full matrix editor");
}
void envelopes() {
  const auto params=mathParameterSpecs();require(params.size()==static_cast<std::size_t>(MathParameter::Count),"parameter schema mismatch");
  for(std::size_t i=0;i<params.size();++i)require(static_cast<std::size_t>(params[i].id)==i&&params[i].minimum<=params[i].initial&&params[i].initial<=params[i].maximum&&params[i].step>0,"invalid parameter schema");
  MathObjects m;MathObjectScene scene;
  for(auto kind:{MathObjectKind::Function,MathObjectKind::Linear,MathObjectKind::Surface})for(unsigned level=0;level<4;++level) {
    select(m,kind,level);action(m,{MathActionKind::Reset});inspect(m,scene);
    for(const auto& p:params)if(m.parameterAvailable(p.id)) {
      for(double value:{p.minimum,p.maximum}){set(m,p.id,value);inspect(m,scene);}
      set(m,p.id,p.initial);
    }
    reject(m,{MathActionKind::SetLevel,{},{},std::numeric_limits<double>::infinity()});
    inspect(m,scene);const auto geometry=scene.frame().vertices.size();scene.resetView();require(scene.navigate(iggy3d::ProductCreativeViewportNavigationOperation::Orbit,20,10),"camera navigation rejected");
    inspect(m,scene);require(scene.frame().vertices.size()==geometry,"camera changed geometry");
  }
}
}
int main(){try{functions();surfaces();matrices();envelopes();plotContracts();std::printf("math layers passed: analytic calculus, linked plots, surface contours, descent, constraints, matrix composition, projection, SVD and validation; %zu frames; maxima %zu vertices / %zu indices / %zu contours\n",frames,maxVertices,maxIndices,maxContours);std::printf("plot contracts passed: %zu states, %zu plots, %zu samples\n",plotStates,plotViews,plotSamples);return 0;}catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}
