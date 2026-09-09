#include "runtime/math_objects/DistanceGeometry.hpp"
#include "runtime/math_objects/MathObjects.hpp"
#include "scene/MathObjectScene.hpp"
#include "ui/MathObjectLayout.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>

using namespace paths;
namespace {
using P=MathParameter;using K=MathObjectKind;
unsigned checks=0,scenes=0,exactCases=0;std::size_t maxVertices=0,maxIndices=0;double maxReconstruction=0,maxEigenResidual=0;
void require(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
void near(double a,double b,double tolerance,const char* why){++checks;if(!std::isfinite(a)||!std::isfinite(b)||std::fabs(a-b)>tolerance){char buffer[300];std::snprintf(buffer,sizeof(buffer),"%s: %.17g vs %.17g tolerance %.3g",why,a,b,tolerance);throw std::runtime_error(buffer);}}
template<class Fn>void rejects(Fn f){bool caught=false;try{f();}catch(const std::invalid_argument&){caught=true;}require(caught,"invalid request accepted");}
// Independent exact PSD/rank oracle: all principal minors of 2*G, using
// integer arithmetic. Production instead uses a scaled Jacobi eigensystem.
std::pair<bool,unsigned> exactOracle(const DistanceEdges& d){
  std::array<long long,9> g{};std::array<long long,16> matrix{};for(unsigned i=0;i<6;++i){const auto pair=distancePairs[i];matrix[4*pair[0]+pair[1]]=matrix[4*pair[1]+pair[0]]=static_cast<long long>(d[i]);}
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)g[3*i+j]=matrix[i+1]+matrix[j+1]-matrix[4*(i+1)+j+1];
  bool psd=true;unsigned rank=0;for(unsigned i=0;i<3;++i){psd=psd&&g[4*i]>=0;if(g[4*i])rank=1;}
  for(unsigned i=0;i<3;++i)for(unsigned j=i+1;j<3;++j){const auto minor=g[4*i]*g[4*j]-g[3*i+j]*g[3*i+j];psd=psd&&minor>=0;if(minor)rank=2;}
  const auto det=g[0]*(g[4]*g[8]-g[5]*g[7])-g[1]*(g[3]*g[8]-g[5]*g[6])+g[2]*(g[3]*g[7]-g[4]*g[6]);psd=psd&&det>=0;if(det)rank=3;return {psd,rank};
}
double signedVolume(const DistancePoints& p){DistancePoint a{},b{},c{};for(unsigned i=0;i<3;++i){a[i]=p[1][i]-p[0][i];b[i]=p[2][i]-p[0][i];c[i]=p[3][i]-p[0][i];}return (a[0]*(b[1]*c[2]-b[2]*c[1])-a[1]*(b[0]*c[2]-b[2]*c[0])+a[2]*(b[0]*c[1]-b[1]*c[0]))/6;}
void certificate(const DistanceEdges& d,const DistanceAnalysis& a){
  require(a.eigenvalues[0]>=a.eigenvalues[1]&&a.eigenvalues[1]>=a.eigenvalues[2],"eigenvalue order");maxEigenResidual=std::max(maxEigenResidual,a.eigenResidual);require(a.eigenResidual<=3e-14*a.scale,"eigenpair residual");
  for(unsigned i=0;i<4;++i){near(a.squared[5*i],0,0,"distance diagonal");for(unsigned j=0;j<4;++j)near(a.squared[4*i+j],a.squared[4*j+i],0,"distance symmetry");}
  if(a.realizable){double maxError=0;for(unsigned e=0;e<6;++e){const auto ij=distancePairs[e];double actual=0;for(unsigned k=0;k<3;++k){const double v=a.points[ij[0]][k]-a.points[ij[1]][k];actual+=v*v;}maxError=std::max(maxError,std::fabs(actual-d[e]));}near(maxError,a.reconstructionError,0,"reported reconstruction error");require(maxError<=5e-9*a.scale,"distance reconstruction");maxReconstruction=std::max(maxReconstruction,maxError);near(std::fabs(signedVolume(a.points)),a.volume,1e-10*std::pow(a.scale,1.5),"volume from independent coordinate determinant");for(unsigned k=a.dimension;k<3;++k)for(auto& p:a.points)near(p[k],0,0,"coordinate uses discarded dimension");}
  else{double sum=0,norm=0,value=0;for(unsigned i=0;i<4;++i){sum+=a.witness[i];norm+=a.witness[i]*a.witness[i];for(unsigned j=0;j<4;++j)value+=a.witness[i]*a.squared[4*i+j]*a.witness[j];}near(sum,0,5e-16,"zero-sum witness");near(norm,1,8e-16,"unit witness");require(value>0,"witness does not disprove distance geometry");near(value,a.witnessValue,0,"witness quadratic form");for(auto& p:a.points)for(double x:p)near(x,0,0,"invalid input published fabricated coordinates");}
}
void mathematics(){
  const std::array<double,4> entries{0,1,2,4};
  for(unsigned code=0;code<4096;++code){unsigned digits=code;DistanceEdges d{};for(auto& x:d){x=entries[digits%4];digits/=4;}const auto expected=exactOracle(d);const auto a=analyzeDistances(d);++exactCases;require(a.realizable==expected.first,"exact principal-minor PSD classification");if(a.realizable)require(a.dimension==expected.second,"exact principal-minor rank");certificate(d,a);}
  auto tet=analyzeDistances(distanceExample(0));near(tet.volume,2*std::sqrt(2.)/3,2e-15,"regular tetrahedron volume");near(tet.eigenvalues[0],8,3e-15,"regular tetrahedron Gram spectrum");near(tet.eigenvalues[1],2,3e-15,"regular tetrahedron repeated eigenvalue");
  for(unsigned i=0;i<5;++i){const auto d=distanceExample(i);const auto a=analyzeDistances(d),b=analyzeDistances(d,true);require(a.realizable&&a.dimension==std::array<unsigned,5>{3,2,1,1,0}[i],"example dimension");certificate(d,a);certificate(d,b);for(unsigned k=0;k<4;++k){near(a.points[k][0],b.points[k][0],0,"reflection x");near(a.points[k][1],b.points[k][1],0,"reflection y");near(a.points[k][2],-b.points[k][2],0,"reflection z");}near(signedVolume(a.points),-signedVolume(b.points),1e-15,"opposite handedness");}
  DistanceEdges impossible{4,4,1.21,4,1.21,1.21};const auto bad=analyzeDistances(impossible);require(!bad.realizable&&bad.trianglesPass&&bad.minTriangleSlack>0,"valid faces but impossible tetrahedron");certificate(impossible,bad);
  DistanceEdges broken{16,1,1,1,1,1};require(!analyzeDistances(broken).trianglesPass,"broken face triangle accepted");
  for(unsigned n=0;n<120;++n){DistancePoints points{};for(unsigned i=0;i<4;++i)for(unsigned j=0;j<3;++j)points[i][j]=std::sin(n*.37+i*1.8+j*.9+i*j*.27);auto d=squaredDistances(points);const auto a=analyzeDistances(d);require(a.realizable,"actual points rejected");certificate(d,a);auto moved=points;const double angle=.37,c=std::cos(angle),s=std::sin(angle);for(auto& p:moved){const double x=p[0],y=p[1];p[0]=c*x-s*y+.8;p[1]=s*x+c*y-.3;p[2]=-p[2]+.2;}const auto transformed=squaredDistances(moved);for(unsigned e=0;e<6;++e)near(d[e],transformed[e],4e-15,"rigid-motion/reflection invariance");}
  for(double factor:{1e-150,1e-12,.01,1.,4.}){auto d=distanceExample(0);for(auto& x:d)x*=factor;const auto a=analyzeDistances(d);require(a.realizable&&a.dimension==3,"relative rank changed with scale");certificate(d,a);near(a.volume/(factor*std::sqrt(factor)),tet.volume,1e-14,"volume scales as squared-scale to power 3/2");}
  for(unsigned a=0;a<5;++a)for(unsigned b=0;b<5;++b)for(double scale:{0.,.1,1.,4.})for(unsigned step=0;step<=20;++step){const auto first=distanceExample(a),second=distanceExample(b);const double t=step/20.;const auto d=mixDistances(first,second,t,scale);const auto result=analyzeDistances(d);require(result.realizable,"nonnegative cone mixture left Euclidean set");certificate(d,result);for(unsigned i=0;i<6;++i)near(d[i],scale*((1-t)*first[i]+t*second[i]),0,"squared-distance interpolation");if(scale==0)require(result.dimension==0,"zero multiplier not a point");}
  for(double t:{.01,.1,.5,.9,.99})require(analyzeDistances(mixDistances(distanceExample(2),distanceExample(3),t,1)).dimension==2,"line mixture failed to gain a dimension");
  auto mixed=mixDistances(distanceExample(2),distanceExample(3),.5,1);near(std::sqrt(mixed[2]),std::sqrt(6.5),0,"mixture uses squared lengths");require(std::fabs(std::sqrt(mixed[2])-2.5)>.04,"accidentally averaged ordinary lengths");
  for(unsigned i=0;i<6;++i)for(double value:{-1.,65.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){auto d=distanceExample(0);d[i]=value;rejects([&]{analyzeDistances(d);});}
  rejects([&]{mixDistances(distanceExample(0),distanceExample(1),1.1,1);});rejects([&]{mixDistances(distanceExample(0),distanceExample(1),.5,-1);});rejects([&]{distanceExample(5);});
}
void act(MathObjects& m,MathAction a){const auto r=m.dispatch(a);if(!r.accepted)throw std::runtime_error(std::string(r.reason));}
void set(MathObjects& m,P p,double v){act(m,{MathActionKind::SetParameter,{},p,v});}
void level(MathObjects& m,unsigned l){act(m,{MathActionKind::SetLevel,{},{},static_cast<double>(l)});}
void preset(MathObjects& m,unsigned n){act(m,{MathActionKind::ObjectPreset,{},{},0,n});}
double metric(const MathObjects& m,std::string_view key){for(unsigned i=0;i<m.snapshot().metricCount;++i)if(m.snapshot().metrics[i].label==key)return m.snapshot().metrics[i].value;throw std::runtime_error("missing metric "+std::string(key));}
void challenge(MathObjects& m,bool pass){act(m,{MathActionKind::Check});require((m.snapshot().feedback==MathFeedback::Solved)==pass,"incorrect distance challenge");}
void inspect(MathObjects& m,MathObjectScene& scene){
  const auto& s=m.snapshot();const auto& frame=scene.publish(s,{0,0,800,600});++scenes;maxVertices=std::max(maxVertices,frame.vertices.size());maxIndices=std::max(maxIndices,frame.indices.size());require(!frame.vertices.empty()&&frame.vertices.size()<=kSceneVertexCapacity&&frame.indices.size()<=kSceneIndexCapacity,"scene capacity");require(iggy3d::isFinite(frame.clipFromWorld),"finite camera");
  std::set<unsigned> ids;unsigned end=0;for(auto& draw:frame.draws){require(ids.insert(draw.objectId.value).second&&draw.firstIndex==end,"draw ownership");end+=draw.indexCount;}require(end==frame.indices.size(),"draw coverage");for(auto& v:frame.vertices){for(float x:v.position)require(std::isfinite(x),"finite vertex");for(float x:v.color)require(std::isfinite(x)&&x>=0&&x<=1,"finite colour");}for(auto i:frame.indices)require(i<frame.vertices.size(),"valid mesh index");
  for(unsigned i=0;i<s.metricCount;++i)require(std::isfinite(s.metrics[i].value),"finite metric");for(unsigned i=0;i<s.table.rowCount;++i)for(unsigned j=0;j<s.table.columnCount;++j)require(std::isfinite(s.table.values[i][j]),"finite table");for(unsigned i=0;i<s.matrixCount;++i)for(double x:s.matrices[i].values)require(std::isfinite(x),"finite matrix");
  for(unsigned i=0;i<s.plotCount;++i){const auto& p=s.plots[i];require(p.seriesCount<=3,"plot series capacity");for(unsigned j=0;j<p.seriesCount;++j)for(unsigned k=0;k<p.series[j].count;++k){const auto x=p.series[j].points[k];require(std::isfinite(x.x)&&std::isfinite(x.y),"finite plot");}}
  DistanceEdges d{};for(unsigned e=0;e<6;++e){const double x=m.parameter(static_cast<P>(static_cast<unsigned>(P::DistanceAB)+e));d[e]=x*x;}if(s.level==3)d=mixDistances(d,distanceExample(static_cast<unsigned>(m.parameter(P::DistanceSecond))),m.parameter(P::DistanceMix),m.parameter(P::DistanceScale));const auto a=analyzeDistances(d,m.parameter(P::DistanceMirror)==1);
  require(s.table.rowCount==4&&s.table.columnCount==4,"distance table shape");for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)near(s.table.values[i][j],a.squared[4*i+j],0,"squared-distance table");
  unsigned edge=0,points=0,bars=0,mirror=0;for(unsigned i=0;i<s.partCount;++i){const auto& part=s.parts[i];points+=part.role=="distance_point";bars+=part.role=="unassembled_distance";mirror+=part.role=="distance_mirror_edge";if(part.role=="distance_edge"){while(edge<6&&d[edge]<1e-12)++edge;require(edge<6,"extra edge");near(iggy3d::length(part.y),std::sqrt(d[edge++]),3e-6,"drawn edge length");}}
  if(a.realizable){require(points==4&&bars==0,"valid shape representation");require(s.solid.indexCount==(a.dimension==3?12U:0U),"collapsed tetrahedron has invented faces");for(unsigned i=0;i<s.solid.vertexCount;++i)near(iggy3d::length(s.solid.vertices[i].normal),1,2e-7,"face normal");if(s.level==1&&a.dimension==3&&m.parameter(P::DistanceGuides)==1)require(mirror>0,"reflected counterpart missing");}
  else{require(points==0&&s.solid.indexCount==0,"impossible shape invented");require(bars<=6,"invalid length bars");}
}
void model(){
  MathObjects m;MathObjectScene scene;act(m,{MathActionKind::Select,K::Distance});require(static_cast<unsigned>(K::Simplex)==31&&static_cast<unsigned>(K::Distance)==32,"object IDs changed");require(mathLessons(K::Distance).size()==4&&mathObjectPresets(K::Distance,0).size()==6,"distance catalogue");std::set<std::string_view> keys;for(unsigned i=0;i<mathParameterSpecs().size();++i){const auto& p=mathParameterSpecs()[i];require(static_cast<unsigned>(p.id)==i&&keys.insert(p.key).second,"parameter registry");}
  for(unsigned l=0;l<4;++l){level(m,l);for(unsigned e=0;e<6;++e){const auto rev=m.snapshot().revision;preset(m,e);require(m.snapshot().revision==rev+1,"preset not atomic");const auto& example=mathObjectPresets(K::Distance,l)[e];require(example.count==12,"incomplete preset");for(unsigned i=0;i<example.count;++i)near(m.parameter(example.parameters[i]),example.values[i],0,"preset value");inspect(m,scene);set(m,P::DistanceMirror,1);inspect(m,scene);set(m,P::DistanceGuides,0);inspect(m,scene);if(l==3)for(double t:{0.,.17,.5,1.}){set(m,P::DistanceMix,t);inspect(m,scene);}}}
  level(m,0);preset(m,0);challenge(m,true);preset(m,1);challenge(m,false);preset(m,4);challenge(m,false);
  level(m,1);preset(m,0);challenge(m,false);set(m,P::DistanceMirror,1);challenge(m,true);
  level(m,2);preset(m,4);challenge(m,true);preset(m,0);challenge(m,false);set(m,P::DistanceAB,4);set(m,P::DistanceAC,1);set(m,P::DistanceBC,1);challenge(m,false);inspect(m,scene);
  level(m,3);preset(m,5);challenge(m,true);set(m,P::DistanceMix,0);challenge(m,false);set(m,P::DistanceMix,.5);set(m,P::DistanceScale,0);challenge(m,false);inspect(m,scene);
  for(unsigned second=0;second<5;++second)for(double scale:{0.,.05,4.})for(double t:{0.,.37,1.}){preset(m,4);set(m,P::DistanceSecond,second);set(m,P::DistanceScale,scale);set(m,P::DistanceMix,t);inspect(m,scene);}
  for(unsigned l=0;l<4;++l){level(m,l);for(bool high:{false,true}){preset(m,0);for(auto p:mathParameterSpecs())if(p.owner==K::Distance&&m.parameterAvailable(p.id))set(m,p.id,high?p.maximum:p.minimum);inspect(m,scene);}}
  level(m,3);preset(m,5);set(m,P::DistanceMix,0);act(m,{MathActionKind::TogglePlayback});act(m,{MathActionKind::AdvanceTime,{},{},2});near(m.parameter(P::DistanceMix),.24,1e-15,"mixture rate");set(m,P::DistanceEdge,3);require(!m.snapshot().playing,"edit did not pause");set(m,P::DistanceMix,.99);act(m,{MathActionKind::TogglePlayback});act(m,{MathActionKind::AdvanceTime,{},{},1});near(m.parameter(P::DistanceMix),1,0,"mixture endpoint");require(!m.snapshot().playing,"endpoint did not stop");
  set(m,P::DistanceMix,.23);set(m,P::DistanceAB,1.37);level(m,0);require(m.playbackParameter()==P::Count,"playback available outside cone layer");level(m,3);near(m.parameter(P::DistanceMix),.23,1e-15,"layer lost mixture");near(m.parameter(P::DistanceAB),1.37,1e-15,"layer lost input");act(m,mathResetControlGroup(m,MathControlGroup::Shape));for(auto p:{P::DistanceAB,P::DistanceAC,P::DistanceAD,P::DistanceBC,P::DistanceBD,P::DistanceCD})near(m.parameter(p),2,0,"shape reset omitted length");
  const auto rev=m.snapshot().revision;require(!m.dispatch({MathActionKind::SetParameter,{},P::DistanceAB,5}).accepted&&!m.dispatch({MathActionKind::SetParameter,{},P::DistanceAC,std::numeric_limits<double>::quiet_NaN()}).accepted&&!m.dispatch({MathActionKind::SetParameter,{},P::SimplexMix,.4}).accepted,"invalid model edit accepted");require(m.snapshot().revision==rev,"rejected edit changed revision");
  act(m,{MathActionKind::Select,K::Simplex});require(m.snapshot().table.rowCount==3,"distance table leaked");act(m,{MathActionKind::Select,K::Algebra});require(!m.snapshot().solid.indexCount&&m.playbackParameter()==P::Count,"distance geometry leaked");
}
}
int main(){try{mathematics();model();std::printf("distance CPU checks: %u assertions, %u exact integer matrices, %u scenes; maxima %zu vertices / %zu indices; max reconstruction error %.4g, eigenpair residual %.4g; no host, fonts or images\n",checks,exactCases,scenes,maxVertices,maxIndices,maxReconstruction,maxEigenResidual);return 0;}catch(const std::exception& e){std::fprintf(stderr,"distance failure: %s\n",e.what());return 1;}}
