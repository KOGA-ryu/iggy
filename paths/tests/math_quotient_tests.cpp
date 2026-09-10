#include "runtime/math_objects/FiniteQuotient.hpp"
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
std::size_t maps=0,subsets=0,ringSubsets=0,frames=0,maxParts=0,maxVertices=0,maxIndices=0;
void require(bool v,const char* why){if(!v)throw std::runtime_error(why);}
void partitionOracle(const FiniteOperation& t,const FinitePartitionOperation& p){
  require(p.classCount>0&&p.classCount<=t.size,"partition class count");unsigned unionMask=0;bool defined=true;
  for(unsigned c=0;c<p.classCount;++c){require(p.members[c]&&!(p.members[c]&unionMask),"partition overlap/empty");unionMask|=p.members[c];require(p.members[c]&(1U<<p.representative[c]),"representative membership");for(unsigned i=0;i<t.size;++i)require((p.classOf[i]==c)==bool(p.members[c]&(1U<<i)),"class labels");}
  require(unionMask==(1U<<t.size)-1,"partition cover");
  for(unsigned x=0;x<p.classCount;++x)for(unsigned y=0;y<p.classCount;++y){unsigned results=0;for(unsigned a=0;a<t.size;++a)for(unsigned b=0;b<t.size;++b)if(p.classOf[a]==x&&p.classOf[b]==y)results|=1U<<p.classOf[t(a,b)];const auto cell=6*x+y;require(results==p.resultMasks[cell],"all representative outcomes");const bool single=std::popcount(results)==1;defined=defined&&single;require(single?(p.product[cell]>=0&&results==(1U<<p.product[cell])):p.product[cell]==-1,"ambiguous cell not marked");}
  require(p.wellDefined==defined,"representative independence");if(!defined){const auto w=p.witness;require(p.classOf[w[0]]==p.classOf[w[2]]&&p.classOf[w[1]]==p.classOf[w[3]]&&p.classOf[t(w[0],w[1])]!=p.classOf[t(w[2],w[3])],"representative conflict witness");}
}
void homOracle(const FiniteOperation& s,const FiniteOperation& t,const std::array<unsigned,6>& f,bool expected){
  ++maps;const auto h=analyzeFiniteHomomorphism(s,t,f);require(h.valid==expected,"homomorphism classification");partitionOracle(s,h.fibers);
  unsigned image=0;for(unsigned i=0;i<s.size;++i){image|=1U<<f[i];for(unsigned j=0;j<s.size;++j)require((f[i]==f[j])==(h.fibers.classOf[i]==h.fibers.classOf[j]),"fiber equality");require(h.image[h.fibers.classOf[i]]==f[i],"image correspondence");}
  require(h.injective==(std::popcount(image)==s.size)&&h.surjective==(image==(1U<<t.size)-1),"map classification");
  if(expected){require(h.kernel&&h.fibers.wellDefined,"valid kernel/fiber structure");const auto target=analyzeFiniteAlgebra(t);for(unsigned i=0;i<s.size;++i)require(bool(h.kernel&(1U<<i))==(f[i]==unsigned(target.identity)),"kernel elements");
    const auto q=analyzeFiniteGroupQuotient(s,h.kernel);require(q.normal&&q.quotient.classCount==h.fibers.classCount,"first isomorphism theorem count");for(unsigned a=0;a<s.size;++a)for(unsigned b=0;b<s.size;++b)require((q.quotient.classOf[a]==q.quotient.classOf[b])==(f[a]==f[b]),"kernel cosets equal fibers");
    for(unsigned a=0;a<h.fibers.classCount;++a)for(unsigned b=0;b<h.fibers.classCount;++b)require(h.image[unsigned(h.fibers.product[6*a+b])]==t(h.image[a],h.image[b]),"quotient-image multiplication");
  }else{require(h.kernel==0,"invented kernel for nonhomomorphism");const auto w=h.witness;require(f[s(w[0],w[1])]!=t(f[w[0]],f[w[1]])&&h.mappedProduct==f[s(w[0],w[1])]&&h.productOfImages==t(f[w[0]],f[w[1]]),"map failure witness");}
}
void kernels(){
  // Every map Cn -> Cm, using the independent generator characterization.
  for(unsigned n=2;n<=6;++n)for(unsigned m=2;m<=6;++m){const auto source=modularOperation(n),target=modularOperation(m);unsigned total=1,valid=0;for(unsigned i=0;i<n;++i)total*=m;
    for(unsigned code=0;code<total;++code){std::array<unsigned,6> f{};unsigned digits=code;for(unsigned i=0;i<n;++i){f[i]=digits%m;digits/=m;}bool expected=n*f[1]%m==0;for(unsigned i=0;i<n;++i)expected=expected&&f[i]==i*f[1]%m;valid+=expected;homOracle(source,target,f,expected);}require(valid==std::gcd(n,m),"cyclic homomorphism count");}
  const auto triangle=trianglePermutationOperation();constexpr std::array<unsigned,6> parity{0,1,1,0,0,1};
  for(unsigned code=0;code<64;++code){std::array<unsigned,6> f{};for(unsigned i=0;i<6;++i)f[i]=(code>>i)&1U;homOracle(triangle,modularOperation(2),f,code==0||f==parity);}
  for(unsigned family=0;family<7;++family){const auto t=family<5?modularOperation(family+2):family==5?kleinFourOperation():triangle;const auto a=analyzeFiniteAlgebra(t);
    for(unsigned mask=0;mask<(1U<<t.size);++mask){++subsets;const auto q=analyzeFiniteGroupQuotient(t,mask);bool subgroup=mask&(1U<<a.identity);for(unsigned x=0;x<t.size;++x)if(mask&(1U<<x)){subgroup=subgroup&&bool(mask&(1U<<a.inverse[x]));for(unsigned y=0;y<t.size;++y)if(mask&(1U<<y))subgroup=subgroup&&bool(mask&(1U<<t(x,y)));}require(q.subgroup==subgroup,"subgroup classification");
      if(!subgroup){require(q.quotient.classCount==0&&!q.normal,"invented quotient partition");require(!(mask&(1U<<q.subsetWitness[2])),"subset failure missing element");continue;}
      partitionOracle(t,q.quotient);bool normal=true;for(unsigned g=0;g<t.size;++g)for(unsigned h=0;h<t.size;++h)if(mask&(1U<<h))normal=normal&&bool(mask&(1U<<t(t(g,h),unsigned(a.inverse[g]))));require(q.normal==normal,"normality via conjugation");
      require(q.quotient.classCount*std::popcount(mask)==t.size,"coset count");for(unsigned g=0;g<t.size;++g){unsigned coset=0;for(unsigned h=0;h<t.size;++h)if(mask&(1U<<h))coset|=1U<<t(g,h);require(q.quotient.members[q.quotient.classOf[g]]==coset,"left coset convention");}
    }
  }
  for(unsigned family=0;family<7;++family){const auto ring=finiteRingExample(family);const auto& add=ring.addition;const auto& mul=ring.multiplication;const unsigned n=add.size;
    for(unsigned mask=0;mask<(1U<<n);++mask){++ringSubsets;const auto q=analyzeFiniteRingQuotient(ring,mask);bool subgroup=mask&1U,absorbs=true;for(unsigned a=0;a<n;++a)for(unsigned b=0;b<n;++b){if((mask&(1U<<a))&&(mask&(1U<<b)))subgroup=subgroup&&bool(mask&(1U<<add(a,b)));if(mask&(1U<<b))absorbs=absorbs&&bool(mask&(1U<<mul(a,b)))&&bool(mask&(1U<<mul(b,a)));}require(q.additiveSubgroup==subgroup&&q.ideal==(subgroup&&absorbs),"ideal classification");if(!subgroup){require(q.addition.classCount==0&&q.multiplication.classCount==0,"invalid ideal classes");continue;}partitionOracle(add,q.addition);partitionOracle(mul,q.multiplication);require(q.addition.wellDefined&&q.multiplication.wellDefined==q.ideal,"ideal representative equivalence");if(!q.ideal){const auto w=q.absorptionWitness;require((mask&(1U<<w[1]))&&!(mask&(1U<<w[2]))&&mul(w[0],w[1])==w[2],"absorption witness");}
    }
  }
  const auto product=finiteRingExample(5),dual=finiteRingExample(6);require(product.multiplication(3,1)==1&&product.multiplication(3,2)==2,"product ring identity");require(dual.multiplication(2,2)==0&&dual.multiplication(1,2)==2,"dual number arithmetic");
  const auto diagonal=analyzeFiniteRingQuotient(product,9);require(diagonal.additiveSubgroup&&!diagonal.ideal&&diagonal.addition.classCount==2&&!diagonal.multiplication.wellDefined,"diagonal nonideal");
  auto rejection=[](auto f){bool rejected=false;try{f();}catch(const std::invalid_argument&){rejected=true;}require(rejected,"invalid input accepted");};
  rejection([]{(void)analyzeFiniteHomomorphism(modularOperation(2),modularOperation(2),{0,2});});rejection([]{(void)analyzeFiniteGroupQuotient(modularOperation(2),4);});rejection([]{(void)analyzeFiniteGroupQuotient(modularOperation(2,true),1);});rejection([]{(void)finiteRingExample(7);});rejection([]{auto ring=finiteRingExample(0);ring.multiplication.values[0]=1;(void)analyzeFiniteRingQuotient(ring,1);});
}
void act(MathObjects& m,MathAction a){const auto r=m.dispatch(a);if(!r.accepted)throw std::runtime_error(std::string(r.reason));}
void set(MathObjects& m,P p,double v){act(m,{MathActionKind::SetParameter,{},p,v});}
void level(MathObjects& m,unsigned l){act(m,{MathActionKind::SetLevel,{},{},double(l)});}
void preset(MathObjects& m,unsigned p){act(m,{MathActionKind::ObjectPreset,{},{},0,p});}
double metric(const MathObjects& m,std::string_view label){for(unsigned i=0;i<m.snapshot().metricCount;++i)if(m.snapshot().metrics[i].label==label)return m.snapshot().metrics[i].value;throw std::runtime_error("missing metric");}
void inspect(const MathObjects& m){
  // Independent model branches can share revision numbers, so use a fresh CPU scene.
  MathObjectScene scene;const auto& s=m.snapshot();const auto& f=scene.publish(s,{0,0,800,600});++frames;maxParts=std::max(maxParts,s.partCount);maxVertices=std::max(maxVertices,f.vertices.size());maxIndices=std::max(maxIndices,f.indices.size());
  require(s.partCount<=s.parts.size()&&s.labelCount<=s.labels.size()&&s.metricCount<=s.metrics.size(),"snapshot capacity");require(!f.vertices.empty()&&f.vertices.size()<=kSceneVertexCapacity&&f.indices.size()<=kSceneIndexCapacity,"mesh capacity");
  std::set<unsigned> ids;std::size_t end=0;for(const auto& d:f.draws){require(ids.insert(d.objectId.value).second&&d.firstIndex==end&&d.indexCount%3==0,"draw ownership");end+=d.indexCount;require(iggy3d::isFinite(d.bounds.min)&&iggy3d::isFinite(d.bounds.max),"draw bounds");}require(end==f.indices.size(),"unowned index");for(auto i:f.indices)require(i<f.vertices.size(),"index out of range");for(const auto& v:f.vertices){for(auto x:v.position)require(std::isfinite(x),"nonfinite vertex");for(auto x:v.color)require(std::isfinite(x)&&x>=0&&x<=1,"nonfinite/outside colour");}
  if(s.curve.active){require(s.curve.count>=2&&s.curve.count<=6&&s.curve.selected<s.curve.count&&s.curve.selectionParameter==P::QuA,"picker wiring");for(unsigned i=0;i<s.curve.count;++i){const auto p=scene.project(s.curve.controls[i]);require(iggy3d::isFinite(p)&&p.z>=0&&p.x>=0&&p.x<=800&&p.y>=0&&p.y<=600,"picker outside scene");}}else require(s.curve.count==0&&m.parameter(P::QuCollapse)==1&&s.level>0,"unexpected disabled picker");
  const auto rows=mathControlRows(m);unsigned mapEditors=0;for(unsigned i=0;i<rows.count;++i)for(unsigned j=0;j<rows.rows[i].count;++j){const auto p=rows.rows[i].parameters[j];require(m.parameterAvailable(p)&&m.parameter(p)<=mathControlRange(m,p).maximum,"control bounds");if(p>=P::QuMap0&&p<=P::QuMap5){++mapEditors;require(unsigned(p)-unsigned(P::QuMap0)==m.parameter(P::QuA),"wrong map row");}}require(mapEditors==(s.level<2?1:0),"map editor count");for(unsigned i=0;i<s.metricCount;++i)require(std::isfinite(s.metrics[i].value),"nonfinite metric");
}
void scenes(){
  MathObjects m;act(m,{MathActionKind::Select,K::Quotient});
  for(auto p:{P::QuA,P::QuB,P::QuRepA,P::QuRepB}){const char* choice=mathParameterSpecs()[unsigned(p)].choices.data();for(unsigned i=0;i<6;++i){require(std::strcmp(choice,std::to_string(i).c_str())==0,"numeric dropdown label");choice+=std::strlen(choice)+1;}}
  for(unsigned l=0;l<4;++l){level(m,l);for(unsigned p=0;p<12;++p){preset(m,p);inspect(m);for(const auto& spec:mathParameterSpecs())if(spec.owner==K::Quotient&&m.parameterAvailable(spec.id))for(double v:{spec.minimum,m.parameterMaximum(spec.id)}){auto copy=m;set(copy,spec.id,v);inspect(copy);}}}
  for(unsigned l=0;l<4;++l){level(m,l);preset(m,l==3?8:0);if(l)set(m,P::QuCollapse,1);act(m,{MathActionKind::Check});require(m.snapshot().feedback==MathFeedback::Solved,"layer challenge");}
  for(unsigned l=1;l<4;++l){level(m,l);preset(m,l==3?8:0);const auto table=m.snapshot().table.values;for(unsigned step=0;step<=100;++step){set(m,P::QuCollapse,step/100.);require(m.snapshot().table.values==table,"collapse changed math");inspect(m);}require(!m.snapshot().curve.active,"collapsed classes retain ambiguous source picker");set(m,P::QuCollapse,0);act(m,{MathActionKind::TogglePlayback});act(m,{MathActionKind::AdvanceTime,{},{},4});require(m.parameter(P::QuCollapse)==1&&!m.snapshot().playing,"collapse playback endpoint");}
  level(m,2);preset(m,3);require(metric(m,"Subgroup")==1&&metric(m,"Normal subgroup")==0&&metric(m,"Gold result class")!=metric(m,"Violet result class"),"nonnormal witness visibility");set(m,P::QuWitness,0);for(unsigned a=0;a<6;++a)for(unsigned b=0;b<6;++b){set(m,P::QuA,a);set(m,P::QuB,b);for(unsigned x=0;x<=m.parameterMaximum(P::QuRepA);++x)for(unsigned y=0;y<=m.parameterMaximum(P::QuRepB);++y){set(m,P::QuRepA,x);set(m,P::QuRepB,y);inspect(m);}}
  preset(m,7);require(metric(m,"Subgroup")==0&&metric(m,"Classes")==0&&!m.parameterAvailable(P::QuCollapse)&&!m.parameterAvailable(P::QuRepA),"invalid subgroup controls");require(!m.dispatch({MathActionKind::TogglePlayback}).accepted,"invalid subgroup playback");
  preset(m,5);set(m,P::QuCollapse,1);require(metric(m,"Classes")==1&&metric(m,"Operation well-defined")==1,"one-class quotient");inspect(m);
  level(m,3);preset(m,9);require(metric(m,"Additive subgroup")==1&&metric(m,"Ideal")==0&&metric(m,"Gold result class")!=metric(m,"Violet result class"),"nonideal witness");set(m,P::QuRingOperation,0);require(metric(m,"Operation well-defined")==1&&!m.parameterAvailable(P::QuWitness),"nonideal still has additive quotient");inspect(m);
  // Every ring subset is also presented through the CPU scene, including empty/full.
  for(unsigned family=0;family<7;++family){set(m,P::QuRing,family);const unsigned n=finiteRingExample(family).addition.size;for(unsigned mask=0;mask<(1U<<n);++mask){for(unsigned i=0;i<n;++i)set(m,static_cast<P>(unsigned(P::QuMember0)+i),(mask>>i)&1U);for(unsigned op=0;op<2;++op){set(m,P::QuRingOperation,op);inspect(m);}}}
  level(m,0);preset(m,0);set(m,P::QuA,5);set(m,P::QuB,5);set(m,P::QuSource,0);require(m.parameter(P::QuA)==1&&m.parameter(P::QuB)==1,"source selector shrink");set(m,P::QuTarget,0);for(unsigned i=0;i<6;++i)require(m.parameter(static_cast<P>(unsigned(P::QuMap0)+i))<=1,"target output shrink");const auto revision=m.snapshot().revision;require(!m.dispatch({MathActionKind::SetParameter,{},P::QuMap0,2}).accepted&&m.snapshot().revision==revision,"invalid edit not atomic");
  preset(m,1);require(metric(m,"Homomorphism")==0&&metric(m,"f(a*b)")!=metric(m,"f(a)*f(b)"),"map witness scene");level(m,1);set(m,P::QuCollapse,1);require(metric(m,"Kernel size (-1 undefined)")==-1&&metric(m,"Quotient-to-image defined")==0&&m.snapshot().solid.vertexCount==0,"invalid map quotient claim");inspect(m);
  level(m,0);preset(m,0);set(m,P::QuSource,1);set(m,P::QuTarget,4);for(unsigned i=0;i<3;++i)set(m,static_cast<P>(unsigned(P::QuMap0)+i),2*i);require(metric(m,"Homomorphism")==1&&metric(m,"Surjective")==0,"proper image homomorphism");level(m,1);set(m,P::QuCollapse,1);require(metric(m,"Image size")==3&&metric(m,"Target size")==6,"image not target");inspect(m);
}
}
int main(){try{kernels();scenes();std::printf("quotients passed: %zu maps, %zu group subsets, %zu ring subsets, %zu CPU scenes; maxima %zu parts / %zu vertices / %zu indices\n",maps,subsets,ringSubsets,frames,maxParts,maxVertices,maxIndices);return 0;}catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}
