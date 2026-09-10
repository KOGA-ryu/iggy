#include "runtime/math_objects/FiniteAlgebra.hpp"
#include "runtime/math_objects/MathObjects.hpp"
#include "scene/MathObjectScene.hpp"
#include "ui/MathObjectLayout.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
namespace {
using namespace paths;using P=MathParameter;using K=MathObjectKind;
unsigned cases=0,frames=0;std::size_t maxParts=0,maxVertices=0,maxIndices=0;
void require(bool b,const char* why){if(!b)throw std::runtime_error(why);}
void oracle(const FiniteOperation& t){
  ++cases;const auto r=analyzeFiniteAlgebra(t);const unsigned n=t.size;
  // Associativity is equality of composed left translations L_a L_b = L_(a*b).
  bool assoc=true,comm=true;std::set<unsigned> identities;
  for(unsigned a=0;a<n;++a){bool e=true;for(unsigned x=0;x<n;++x){e=e&&t(a,x)==x&&t(x,a)==x;comm=comm&&t(a,x)==t(x,a);}if(e)identities.insert(a);
    for(unsigned b=0;b<n;++b){std::array<unsigned,6> composed{},product{};for(unsigned x=0;x<n;++x){composed[x]=t(a,t(b,x));product[x]=t(t(a,b),x);}assoc=assoc&&composed==product;}}
  require(r.associative==assoc&&r.commutative==comm,"law classification");require(identities.size()<2&&r.identity==(identities.empty()?-1:int(*identities.begin())),"identity classification");
  bool inverses=!identities.empty();for(unsigned a=0;a<n;++a){std::set<unsigned> expected;if(r.identity>=0)for(unsigned b=0;b<n;++b)if(t(a,b)==unsigned(r.identity)&&t(b,a)==unsigned(r.identity))expected.insert(b);inverses=inverses&&!expected.empty();require((r.inverse[a]<0)==expected.empty(),"inverse existence");if(r.inverse[a]>=0)require(expected.contains(unsigned(r.inverse[a])),"inverse value");
    if(int(a)!=r.identity){const auto x=r.identityWitness[a];require(x<n&&r.identitySide[a]<2&&(r.identitySide[a]?t(x,a):t(a,x))!=x,"identity counterexample");}}
  require(r.group==(assoc&&inverses),"group classification");
  if(!assoc){const auto w=r.associativityWitness;require(t(t(w[0],w[1]),w[2])!=t(w[0],t(w[1],w[2])),"associativity witness");}
  if(!comm){const auto w=r.commutativityWitness;require(t(w[0],w[1])!=t(w[1],w[0]),"commutativity witness");}
  for(unsigned g=0;g<n;++g)for(bool right:{false,true}){const auto h=generatedFiniteSubgroup(t,g,right);require(h.defined==r.group,"undefined subgroup");if(!r.group){require(h.mask==0&&h.order==0&&h.cosetCount==0,"invented subgroup");continue;}
    // Independent closure of {e,g}, then conjugation test for normality.
    unsigned closure=(1U<<g)|(1U<<r.identity),old=0;while(old!=closure){old=closure;for(unsigned a=0;a<n;++a)for(unsigned b=0;b<n;++b)if((old&(1U<<a))&&(old&(1U<<b)))closure|=1U<<t(a,b);}
    require(h.mask==closure&&h.order==std::popcount(closure)&&h.order*h.cosetCount==n,"subgroup closure / Lagrange");require(h.powers[0]==unsigned(r.identity)&&h.powers[h.order]==unsigned(r.identity),"power endpoints");for(unsigned j=0;j<h.order;++j)require(h.powers[j+1]==t(h.powers[j],g),"power recurrence");
    bool normal=true;for(unsigned a=0;a<n;++a){unsigned coset=0;for(unsigned x=0;x<n;++x)if(closure&(1U<<x)){coset|=1U<<(right?t(x,a):t(a,x));normal=normal&&bool(closure&(1U<<t(t(a,x),unsigned(r.inverse[a]))));}unsigned actual=0;for(unsigned y=0;y<n;++y)if(h.cosetOf[y]==h.cosetOf[a])actual|=1U<<y;require(actual==coset,"coset membership");}require(h.normal==normal,"normality by conjugation");
  }
}
void kernels(){
  for(unsigned n:{2U,3U}){unsigned total=1;for(unsigned k=0;k<n*n;++k)total*=n;for(unsigned code=0;code<total;++code){FiniteOperation t;t.size=n;unsigned digits=code;for(unsigned i=0;i<n;++i)for(unsigned j=0;j<n;++j){t.values[6*i+j]=digits%n;digits/=n;}oracle(t);}}
  for(unsigned kind=0;kind<7;++kind){const auto t=kind<5?modularOperation(kind+2):kind==5?kleinFourOperation():trianglePermutationOperation();oracle(t);require(analyzeFiniteAlgebra(t).group,"known group");for(unsigned i=0;i<t.size;++i)for(unsigned j=0;j<t.size;++j)for(unsigned v=0;v<t.size;++v){auto changed=t;changed.values[6*i+j]=v;oracle(changed);}}
  auto s3=trianglePermutationOperation();require(s3(1,2)==4&&s3(2,1)==3,"permutation composition convention");require(!generatedFiniteSubgroup(s3,1).normal&&generatedFiniteSubgroup(s3,3).normal,"S3 normal subgroups");
  // Relabel S3 to put its identity at every possible index.
  for(unsigned e=0;e<6;++e){std::array<unsigned,6> rename{0,1,2,3,4,5};std::swap(rename[0],rename[e]);FiniteOperation t;t.size=6;for(unsigned a=0;a<6;++a)for(unsigned b=0;b<6;++b)t.values[6*rename[a]+rename[b]]=rename[s3(a,b)];oracle(t);require(analyzeFiniteAlgebra(t).identity==int(e),"relabeled identity");}
  for(unsigned n=2;n<=6;++n){const auto ring=analyzeModularRing(n);unsigned units=0,zeros=0;for(unsigned a=0;a<n;++a){const bool unit=std::gcd(a,n)==1,zero=a&&!unit;require((ring.inverse[a]>=0)==unit&&(ring.zeroDivisorWitness[a]>=0)==zero,"ring classification via gcd");if(unit)require(a*unsigned(ring.inverse[a])%n==1,"unit inverse");if(zero)require(ring.zeroDivisorWitness[a]>0&&a*unsigned(ring.zeroDivisorWitness[a])%n==0,"nonzero annihilator");units+=unit;zeros+=zero;}require(ring.unitCount==units&&ring.zeroDivisorCount==zeros&&ring.field==(n==2||n==3||n==5),"ring counts / fields");const auto add=modularOperation(n),mul=modularOperation(n,true);for(unsigned a=0;a<n;++a)for(unsigned b=0;b<n;++b)for(unsigned c=0;c<n;++c)require(mul(a,add(b,c))==add(mul(a,b),mul(a,c)),"distributivity");}
  for(unsigned n:{0U,1U,7U}){bool rejected=false;try{(void)modularOperation(n);}catch(const std::invalid_argument&){rejected=true;}require(rejected,"bad size accepted");}auto bad=modularOperation(2);bad.values[0]=2;bool rejected=false;try{(void)analyzeFiniteAlgebra(bad);}catch(const std::invalid_argument&){rejected=true;}require(rejected,"nonclosed table accepted");rejected=false;try{(void)generatedFiniteSubgroup(modularOperation(2),2);}catch(const std::invalid_argument&){rejected=true;}require(rejected,"bad generator accepted");
}
void act(MathObjects& m,MathAction a){const auto r=m.dispatch(a);if(!r.accepted)throw std::runtime_error(std::string(r.reason));}
void set(MathObjects& m,P p,double v){act(m,{MathActionKind::SetParameter,{},p,v});}
void level(MathObjects& m,unsigned l){act(m,{MathActionKind::SetLevel,{},{},double(l)});}
void preset(MathObjects& m,unsigned p){act(m,{MathActionKind::ObjectPreset,{},{},0,p});}
double metric(const MathObjects& m,std::string_view name){for(unsigned i=0;i<m.snapshot().metricCount;++i)if(m.snapshot().metrics[i].label==name)return m.snapshot().metrics[i].value;throw std::runtime_error("missing metric");}
void inspect(const MathObjects& m){
  MathObjectScene scene;const auto& s=m.snapshot();const auto& f=scene.publish(s,{0,0,800,600});++frames;maxParts=std::max(maxParts,s.partCount);maxVertices=std::max(maxVertices,f.vertices.size());maxIndices=std::max(maxIndices,f.indices.size());
  require(s.partCount<=s.parts.size()&&s.labelCount<=s.labels.size()&&s.metricCount<=s.metrics.size(),"snapshot capacity");require(!f.vertices.empty()&&f.vertices.size()<=kSceneVertexCapacity&&f.indices.size()<=kSceneIndexCapacity,"mesh budget");
  std::set<unsigned> ids;std::size_t end=0;for(const auto& d:f.draws){require(ids.insert(d.objectId.value).second&&d.firstIndex==end&&d.indexCount%3==0,"draw ownership");end+=d.indexCount;require(iggy3d::isFinite(d.bounds.min)&&iggy3d::isFinite(d.bounds.max),"draw bounds");}require(end==f.indices.size(),"unowned indices");for(auto i:f.indices)require(i<f.vertices.size(),"index bound");for(const auto& v:f.vertices){for(auto x:v.position)require(std::isfinite(x),"nonfinite vertex");for(auto x:v.color)require(std::isfinite(x)&&x>=0&&x<=1,"colour bound");}
  const unsigned n=unsigned(m.parameter(P::FaSize));require(s.curve.active&&s.curve.count==n&&s.curve.selected<n&&s.curve.selectionParameter==P::FaA,"picker wiring");for(unsigned i=0;i<n;++i){const auto p=scene.project(s.curve.controls[i]);require(iggy3d::isFinite(p)&&p.z>=0&&p.x>=0&&p.x<=800&&p.y>=0&&p.y<=600,"offscreen picker");}
  const auto controls=mathControlRows(m);unsigned entries=0;for(unsigned i=0;i<controls.count;++i)for(unsigned j=0;j<controls.rows[i].count;++j){const auto p=controls.rows[i].parameters[j];require(m.parameterAvailable(p)&&m.parameter(p)<=mathControlRange(m,p).maximum,"control range");if(p>=P::FaE00&&p<=P::FaE55){++entries;const auto cell=unsigned(p)-unsigned(P::FaE00);require(cell/6==m.parameter(P::FaA)&&cell%6<n,"wrong row control");}}require(entries==(s.level==3?0:n),"row editor count");for(unsigned i=0;i<s.metricCount;++i)require(std::isfinite(s.metrics[i].value),"nonfinite metric");
  unsigned expectedQuads=n*n;constexpr unsigned segments[]{6,2,5,5,4,5};for(unsigned i=0;i<n;++i)for(unsigned j=0;j<n;++j){const auto value=s.level==3?(m.parameter(P::FaRingOperation)==1?i*j:i+j)%n:unsigned(m.parameter(static_cast<P>(unsigned(P::FaE00)+6*i+j)));expectedQuads+=segments[value];}require(s.solid.vertexCount==4*expectedQuads&&s.solid.indexCount==6*expectedQuads,"numeric table geometry");
}
void scenes(){
  MathObjects m;act(m,{MathActionKind::Select,K::FiniteAlgebra});
  for(auto p:{P::FaA,P::FaB,P::FaC}){const char* choice=mathParameterSpecs()[unsigned(p)].choices.data();for(unsigned i=0;i<6;++i){require(std::strcmp(choice,std::to_string(i).c_str())==0,"numeric dropdown label");choice+=std::strlen(choice)+1;}}
  for(unsigned l=0;l<4;++l){level(m,l);for(unsigned p=0;p<10;++p){preset(m,p);inspect(m);for(const auto& spec:mathParameterSpecs())if(spec.owner==K::FiniteAlgebra&&m.parameterAvailable(spec.id))for(double v:{spec.minimum,m.parameterMaximum(spec.id)}){auto copy=m;set(copy,spec.id,v);inspect(copy);}}}
  for(unsigned l=0;l<4;++l){level(m,l);preset(m,std::array<unsigned,4>{6,7,4,9}[l]);if(l==2)set(m,P::FaGroup,1);act(m,{MathActionKind::Check});require(m.snapshot().feedback==MathFeedback::Solved,"challenge");}
  level(m,2);preset(m,6);for(unsigned g=0;g<6;++g){set(m,P::FaA,g);for(unsigned side=0;side<2;++side){set(m,P::FaSide,side);const auto original=m.snapshot().table.values;for(unsigned step=0;step<=20;++step){set(m,P::FaGroup,step/20.);require(m.snapshot().table.values==original,"layout changed cosets");inspect(m);}}}
  preset(m,4);for(unsigned i=0;i<=120;++i){set(m,P::FaTime,i/20.);inspect(m);}set(m,P::FaTime,0);act(m,{MathActionKind::TogglePlayback});act(m,{MathActionKind::AdvanceTime,{},{},6});require(m.parameter(P::FaTime)==6&&!m.snapshot().playing,"playback endpoint");set(m,P::FaTime,0);act(m,{MathActionKind::TogglePlayback});set(m,P::FaE00,1);require(!m.snapshot().playing&&!m.parameterAvailable(P::FaTime),"invalid group must stop playback");
  level(m,1);preset(m,7);require(metric(m,"Associative")==0&&metric(m,"Gold result")!=metric(m,"Violet result"),"visible failing witness");for(unsigned law=0;law<4;++law){set(m,P::FaLaw,law);inspect(m);}level(m,2);require(metric(m,"Subgroup defined")==0&&!m.parameterAvailable(P::FaGroup),"invented cosets");
  level(m,0);preset(m,6);set(m,P::FaA,5);set(m,P::FaB,5);set(m,P::FaSize,2);require(m.parameter(P::FaA)==1&&m.parameter(P::FaB)==1,"shrinking selector");for(unsigned i=0;i<36;++i)require(m.parameter(static_cast<P>(unsigned(P::FaE00)+i))<=1,"shrinking output");const auto revision=m.snapshot().revision;require(!m.dispatch({MathActionKind::SetParameter,{},P::FaE00,2}).accepted&&m.snapshot().revision==revision,"invalid edit must be atomic");
  level(m,3);preset(m,9);require(metric(m,"a op b")==0&&metric(m,"Units")==2&&metric(m,"Nonzero zero divisors")==3,"modulo 6 scene");set(m,P::FaRingOperation,0);require(metric(m,"a op b")==5,"addition toggle");set(m,P::FaSize,5);require(metric(m,"Field")==1,"prime field scene");
  level(m,0);preset(m,7);const auto stored=m.parameter(P::FaE00);level(m,3);set(m,P::FaRingOperation,1);level(m,1);require(m.parameter(P::FaE00)==stored&&metric(m,"Associative")==0,"ring layer modified custom table");
}
}
int main(){try{kernels();scenes();std::printf("finite algebra passed: %u operation tables, %u CPU scenes; maxima %zu parts / %zu vertices / %zu indices\n",cases,frames,maxParts,maxVertices,maxIndices);return 0;}catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}
