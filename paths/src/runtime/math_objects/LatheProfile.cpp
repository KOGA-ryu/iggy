#include "runtime/math_objects/LatheProfile.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace paths {
namespace {
constexpr double pi=3.14159265358979323846;
void require(bool valid,const char* why){if(!valid)throw std::invalid_argument(why);}
void fraction(double x){require(std::isfinite(x)&&x>=0&&x<=1,"lathe fraction outside [0,1]");}
void divisions(unsigned n){require(n>=4&&n<=32,"lathe subdivisions outside [4,32]");}
void add(LatheHeights& h,double x){if(h.count==h.values.size())throw std::logic_error("lathe partition capacity");h.values[h.count++]=x;}
void sort(LatheHeights& h){std::sort(h.values.begin(),h.values.begin()+h.count);unsigned n=0;for(unsigned i=0;i<h.count;++i)if(n==0||h.values[i]-h.values[n-1]>1e-12)h.values[n++]=h.values[i];h.count=n;}
LatheSample sample(const LatheProfile& p,double u) {
  unsigned segment=0;while(segment<5&&u>p.input.heights[segment+1])++segment;
  const double width=p.input.heights[segment+1]-p.input.heights[segment];
  const auto b=sampleBezier(p.segments[segment],std::clamp((u-p.input.heights[segment])/width,0.0,1.0));
  const double radius=std::max(0.0,b.position[0]);
  return {radius,p.input.hollow&&u>=p.input.floor?std::max(0.0,radius-p.input.wall):0,b.first[0]/(width*p.input.height)};
}
void crossings(const LatheProfile& p,double radius,LatheHeights& h) {
  for(unsigned i=0;i<6;++i){const double a=p.input.radii[i],b=p.input.radii[i+1];if(radius<=std::min(a,b)||radius>=std::max(a,b))continue;
    double lo=p.input.heights[i],hi=p.input.heights[i+1];for(unsigned step=0;step<44;++step){const double mid=(lo+hi)/2;if((sample(p,mid).radius<radius)==(b>a))lo=mid;else hi=mid;}add(h,(lo+hi)/2);
  }
}
template<class F> double gauss4(F f,double a,double b) {
  constexpr double x[]{.3399810435848562648,.8611363115940525752},w[]{.6521451548625461426,.3478548451374538574};
  double sum=0;const double mid=(a+b)/2,half=(b-a)/2;for(unsigned i=0;i<2;++i)sum+=w[i]*(f(mid-half*x[i])+f(mid+half*x[i]));return sum*half;
}
template<class F> double gauss8(F f,double a,double b) {
  constexpr double x[]{.1834346424956498049,.5255324099163289858,.7966664774136267396,.9602898564975362317};
  constexpr double w[]{.3626837833783619829,.3137066458778872873,.2223810344533744705,.1012285362903762592};
  double sum=0;const double mid=(a+b)/2,half=(b-a)/2;for(unsigned i=0;i<4;++i)sum+=w[i]*(f(mid-half*x[i])+f(mid+half*x[i]));return sum*half;
}
template<class F> double area(F f,double a,double b,double tolerance,double& difference,unsigned depth=0) {
  const double mid=(a+b)/2,whole=gauss8(f,a,b),fine=gauss8(f,a,mid)+gauss8(f,mid,b),error=std::fabs(fine-whole);
  if(error<=tolerance||depth==10){difference+=error;return fine;}
  return area(f,a,mid,tolerance/2,difference,depth+1)+area(f,mid,b,tolerance/2,difference,depth+1);
}
}
LatheProfile prepareLathe(const LatheInput& input) {
  require(std::isfinite(input.height)&&input.height>=1&&input.height<=4,"lathe height outside [1,4]");
  require(std::isfinite(input.wall)&&input.wall>=.03&&input.wall<=.35,"lathe wall outside [0.03,0.35]");
  require(std::isfinite(input.floor)&&input.floor>=.04&&input.floor<=.8,"lathe floor outside [0.04,0.8]");
  require(input.heights.front()==0&&input.heights.back()==1,"lathe endpoint heights must be 0 and 1");
  for(unsigned i=0;i<7;++i){require(std::isfinite(input.radii[i])&&input.radii[i]>=0&&input.radii[i]<=1.5,"lathe radius outside [0,1.5]");require(std::isfinite(input.heights[i])&&(i==0||input.heights[i]-input.heights[i-1]>=.04-1e-12),"profile heights must remain ordered with a 0.04 gap");}
  LatheProfile p;p.input=input;p.maximumRadius=*std::max_element(input.radii.begin(),input.radii.end());
  std::array<double,6> width{},secant{};std::array<double,7> slope{};
  for(unsigned i=0;i<6;++i){width[i]=input.heights[i+1]-input.heights[i];secant[i]=(input.radii[i+1]-input.radii[i])/width[i];}
  const auto endpoint=[](double a,double b,double da,double db){double m=((2*a+b)*da-a*db)/(a+b);if(m*da<=0)return 0.0;if(da*db<0&&std::fabs(m)>3*std::fabs(da))m=3*da;return m;};
  slope[0]=endpoint(width[0],width[1],secant[0],secant[1]);slope[6]=endpoint(width[5],width[4],secant[5],secant[4]);
  for(unsigned i=1;i<6;++i)if(secant[i-1]*secant[i]>0){const double w1=2*width[i]+width[i-1],w2=width[i]+2*width[i-1];slope[i]=(w1+w2)/(w1/secant[i-1]+w2/secant[i]);}
  for(unsigned i=0;i<6;++i)p.segments[i].controls={BezierPoint{input.radii[i],input.heights[i],0},BezierPoint{input.radii[i]+slope[i]*width[i]/3,input.heights[i]+width[i]/3,0},BezierPoint{input.radii[i+1]-slope[i+1]*width[i]/3,input.heights[i+1]-width[i]/3,0},BezierPoint{input.radii[i+1],input.heights[i+1],0}};
  return p;
}
LatheSample sampleLathe(const LatheProfile& p,double u){fraction(u);return sample(p,u);}
LatheHeights latheHeights(const LatheProfile& p,unsigned n) {
  require(n<=64,"lathe partition subdivision limit");LatheHeights h;for(double u:p.input.heights)add(h,u);
  if(p.input.hollow){add(h,p.input.floor);crossings(p,p.input.wall,h);}
  if(n)for(unsigned i=1;i<n;++i)add(h,static_cast<double>(i)/n);sort(h);return h;
}
LatheShell latheShell(const LatheProfile& p,double radius) {
  require(std::isfinite(radius)&&radius>=0&&radius<=1.5,"lathe shell radius outside [0,1.5]");
  auto h=latheHeights(p);crossings(p,radius,h);if(p.input.hollow)crossings(p,radius+p.input.wall,h);sort(h);LatheShell result;
  for(unsigned i=1;i<h.count;++i){const double a=h.values[i-1],b=h.values[i];const auto point=sample(p,(a+b)/2);
    if(radius>point.radius||radius<point.inner)continue;
    if(result.count&&std::fabs(result.spans[result.count-1].to-a)<1e-12)result.spans[result.count-1].to=b;
    else {if(result.count==result.spans.size())throw std::logic_error("lathe shell span capacity");result.spans[result.count++]={a,b};}
    result.height+=(b-a)*p.input.height;
  }
  return result;
}
LatheMeasure measureLathe(const LatheProfile& p,double turn) {
  fraction(turn);LatheMeasure m;if(turn==0)return m;const auto h=latheHeights(p);double meridian=0;
  for(unsigned i=1;i<h.count;++i){const double a=h.values[i-1],b=h.values[i],tolerance=1e-10*(b-a);
    m.outerVolume+=pi*p.input.height*gauss4([&](double u){const double r=sample(p,u).radius;return r*r;},a,b);
    m.voidVolume+=pi*p.input.height*gauss4([&](double u){const double r=sample(p,u).inner;return r*r;},a,b);
    meridian+=p.input.height*gauss4([&](double u){const auto s=sample(p,u);return s.radius-s.inner;},a,b);
    m.outerArea+=2*pi*p.input.height*area([&](double u){const auto s=sample(p,u);return s.radius*std::hypot(1,s.slope);},a,b,tolerance,m.areaDifference);
    if(sample(p,(a+b)/2).inner>0)m.innerArea+=2*pi*p.input.height*area([&](double u){const auto s=sample(p,u);return s.inner*std::hypot(1,s.slope);},a,b,tolerance,m.areaDifference);
  }
  const auto bottom=sample(p,0),top=sample(p,1),floor=sample(p,p.input.floor);
  m.closureArea=pi*(bottom.radius*bottom.radius+top.radius*top.radius-top.inner*top.inner+(p.input.hollow?floor.inner*floor.inner:0));
  m.outerVolume*=turn;m.voidVolume*=turn;m.volume=std::max(0.0,m.outerVolume-m.voidVolume);m.outerArea*=turn;m.innerArea*=turn;m.closureArea*=turn;
  m.areaDifference*=2*pi*p.input.height*turn;m.cutArea=turn<1?2*meridian:0;m.area=m.outerArea+m.innerArea+m.closureArea+m.cutArea;return m;
}
LatheApproximation approximateLatheVolume(const LatheProfile& p,unsigned n,bool shells,double turn) {
  divisions(n);fraction(turn);LatheApproximation result;result.count=n;
  for(unsigned i=0;i<n;++i){auto& e=result.elements[i];const double u=(i+.5)/n;
    if(shells){e.center=u*p.maximumRadius;e.step=p.maximumRadius/n;e.section=latheShell(p,e.center).height;e.volume=turn*2*pi*e.center*e.step*e.section;}
    else {const auto s=sample(p,u);e.center=u*p.input.height;e.step=p.input.height/n;e.section=pi*(s.radius*s.radius-s.inner*s.inner);e.volume=turn*e.step*e.section;}
    result.volume+=e.volume;
  }
  return result;
}
double approximateLatheArea(const LatheProfile& p,unsigned n,double turn) {
  divisions(n);fraction(turn);const auto h=latheHeights(p,n);double lateral=0;
  for(unsigned i=1;i<h.count;++i){const double a=h.values[i-1],b=h.values[i],dh=(b-a)*p.input.height;const auto lo=sample(p,a),hi=sample(p,b);
    lateral+=pi*(lo.radius+hi.radius)*std::hypot(dh,hi.radius-lo.radius);
    if(sample(p,(a+b)/2).inner>0){const double ra=std::max(0.0,lo.radius-p.input.wall),rb=std::max(0.0,hi.radius-p.input.wall);lateral+=pi*(ra+rb)*std::hypot(dh,rb-ra);}
  }
  const auto exact=measureLathe(p,turn);return turn*lateral+exact.closureArea+exact.cutArea;
}
}
