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
constexpr std::array<MathParameterSpec,static_cast<std::size_t>(MathParameter::Count)> parameters{{
  {MathParameter::X,MathObjectKind::Algebra,"x","Variable x",.5,2.5,.1,1.6},
  {MathParameter::Gap,MathObjectKind::Algebra,"gap","Separate pieces",0,.6,.02,.24},
  {MathParameter::Angle,MathObjectKind::Trig,"angle","Angle (degrees)",0,360,1,45},
  {MathParameter::Slices,MathObjectKind::Calculus,"slices","Number of slices",3,64,1,8},
  {MathParameter::SliceGap,MathObjectKind::Calculus,"slice_gap","Separate slices",0,.12,.01,0},
  {MathParameter::Sample,MathObjectKind::Calculus,"sample","Sampling: left / midpoint / right",0,2,1,1},
  {MathParameter::Shear,MathObjectKind::Linear,"shear","A[0,1] / shear k",-1.2,1.2,.1,.6,0,true},
  {MathParameter::Scale,MathObjectKind::Linear,"scale","A[1,1] / vertical scale",-2,2,.1,1,0,true},
  {MathParameter::Depth,MathObjectKind::Discrete,"depth","Layer spacing",.3,1.5,.1,1},
  {MathParameter::Shortcut,MathObjectKind::Discrete,"shortcut","Add A-H shortcut",0,1,1,0},
  {MathParameter::FunctionRule,MathObjectKind::Function,"function","Function",0,3,1,0,0,false,"x^2\0x^3 - 3x\0sin(x)\0exp(x) - 1\0"},
  {MathParameter::FunctionX,MathObjectKind::Function,"at","Input x",-2,2,.005,.75},
  {MathParameter::DeltaX,MathObjectKind::Function,"h","Secant step h",-1,1,.005,.5,1},
  {MathParameter::IntegralStart,MathObjectKind::Function,"from","Integral start a",-2,2,.005,0,2},
  {MathParameter::TaylorCenter,MathObjectKind::Function,"center","Taylor centre c",-1,1,.05,0,3},
  {MathParameter::TaylorDegree,MathObjectKind::Function,"degree","Taylor degree",0,5,1,1,3},
  {MathParameter::A00,MathObjectKind::Linear,"a00","A[0,0]",-2,2,.1,1,1,true},
  {MathParameter::A02,MathObjectKind::Linear,"a02","A[0,2]",-2,2,.1,0,1,true},
  {MathParameter::A10,MathObjectKind::Linear,"a10","A[1,0]",-2,2,.1,0,1,true},
  {MathParameter::A12,MathObjectKind::Linear,"a12","A[1,2]",-2,2,.1,0,1,true},
  {MathParameter::A20,MathObjectKind::Linear,"a20","A[2,0]",-2,2,.1,0,1,true},
  {MathParameter::A21,MathObjectKind::Linear,"a21","A[2,1]",-2,2,.1,0,1,true},
  {MathParameter::A22,MathObjectKind::Linear,"a22","A[2,2]",-2,2,.1,1,1,true},
  {MathParameter::VectorX,MathObjectKind::Linear,"vx","Vector x",-2,2,.05,1,1},
  {MathParameter::VectorY,MathObjectKind::Linear,"vy","Vector y",-2,2,.05,1,1},
  {MathParameter::VectorZ,MathObjectKind::Linear,"vz","Vector z",-2,2,.05,1,1},
  {MathParameter::ComposeAngle,MathObjectKind::Linear,"rotate","B: rotation about z (degrees)",-180,180,1,30,1},
  {MathParameter::SvdStage,MathObjectKind::Linear,"svd_stage","SVD stage",0,3,1,3,3,false,"Unit sphere\0Apply V transpose\0Then apply Sigma\0Then apply U\0"},
  {MathParameter::SurfaceRule,MathObjectKind::Surface,"surface","Surface",0,2,1,0,0,false,"Bowl: (u^2 + v^2)/2\0Saddle: (u^2 - v^2)/2\0Wave: sin(u) cos(v)\0"},
  {MathParameter::SurfaceU,MathObjectKind::Surface,"u","Input u",-2,2,.025,.75},
  {MathParameter::SurfaceV,MathObjectKind::Surface,"v","Input v",-2,2,.025,.5},
  {MathParameter::DirectionAngle,MathObjectKind::Surface,"direction","Direction (degrees)",0,360,1,45,1},
  {MathParameter::DescentRate,MathObjectKind::Surface,"rate","Descent step size",.05,1,.05,.25,2},
  {MathParameter::Constraint,MathObjectKind::Surface,"constraint","Constrain to unit circle",0,1,1,0,3,false,"Free point\0Unit circle\0"},
  {MathParameter::CircleAngle,MathObjectKind::Surface,"circle_angle","Position on circle (degrees)",0,360,1,45,3}
}};
constexpr std::array<MathObjectSpec,7> objects{{
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
   {"Follow routes and count steps.","Use three-bit addresses to represent the vertices.","Explore shortest paths, Hamming distance and hypercubes."},{1,.7F,1}},
  {MathObjectKind::Function,"function","Functions","Connect a function to its calculus",
   "f(x), f'(x), and F(x) = integral[a,x] f(t) dt","Find an input where f(x) = 0.",
   "All linked views share x. Signed accumulation follows the order of the bounds. Curves are sampled; readouts are analytic.",
   {"Find inputs, outputs and roots.","Compare secants, tangents and derivatives.","Link accumulation, differentiation and Taylor approximation."},{0,0,1}},
  {MathObjectKind::Surface,"surface","Surfaces","Read a surface through its contours",
   "z = f(u,v)","On the bowl, find a point with height 1.",
   "World position is (u, height, v). Contours interpolate the surface grid; derivatives and readouts are analytic.",
   {"Connect coordinates, height and contours.","Read partial derivatives and a tangent plane.","Explore gradients, optimisation and constrained stationary points."},{1,.8F,1}}
}};
constexpr std::array<MathLesson,4> functionLessons{{
  {"Inputs and roots","y = f(x)","Find an input where f(x) = 0.","Move the point or scrub a graph. The selected function owns every linked value."},
  {"Secants and limits","secant = [f(x+h)-f(x)]/h  ->  f'(x)","Use a NONZERO h with secant error below 0.02.","Compare the gold secant with the coral tangent. At h=0 the display uses the derivative limit; division by zero is never evaluated."},
  {"Signed accumulation","F(x) = integral[a,x] f(t) dt; F'(x) = f(x)","Make the signed integral negative, then use Swap bounds to reverse its sign.","The filled region lies between a and x. The integral includes both the sign of f and the orientation of the bounds."},
  {"Taylor approximation","T_n(x) = sum[k=0,n] f^(k)(c) (x-c)^k / k!","At least 0.5 from c, approximate f(x) within 0.001.","Increase the degree and compare the approximation with f. Accuracy depends on both degree and distance from the centre."}
}};
constexpr std::array<MathLesson,4> linearLessons{{
  {"Volume and rank","det(A) = s; volume = abs(det(A))","Collapse the cube into a plane.","Shear preserves volume; zero vertical scale removes one independent direction."},
  {"Composition and invariant directions","v -> A v -> B A v; compare B A with A B","Choose a nonzero v with A v = lambda v and a nonzero A v.","Edit A in the matrix diagram. B rotates about z. Gold is v, teal is Av, violet is BAv; an eigenvector stays on its original line."},
  {"Projection and least squares","p = projection of v onto span(A e1, A e2)","Make a nonzero v lie in the column plane: residual below 0.01.","The gold residual is perpendicular to the spanned subspace. If the columns become dependent, the projection uses their remaining span."},
  {"Singular value decomposition","A = U Sigma V^T","Set the smallest singular value to zero while retaining rank 2.","Step the sphere through V transpose, Sigma and U. Singular values measure axis stretches; the full matrix diagram still owns A."}
}};
constexpr std::array<MathLesson,4> surfaceLessons{{
  {"Height and contours","z = f(u,v)","On the bowl, find a point with height 1.","A contour connects equal heights. Click the contour map to move the point; the two section plots follow it."},
  {"Partials and tangent planes","D_d f = grad(f) dot d","On the bowl away from the origin, find a direction with slope below 0.02 in magnitude.","The u and v sections show the two partial derivatives. The tangent plane combines them; the direction arrow lives in the input plane."},
  {"Gradient descent and curvature","next point = point - step * grad(f)","On the bowl, reach gradient magnitude below 0.02.","Descent uses a bounded line search. Inspect the Hessian: positive curvature gives a minimum; opposite signs identify a saddle."},
  {"Constrained stationary points","u^2 + v^2 = 1; grad(f) = lambda grad(g)","On the saddle with the circle enabled, make the tangential slope smaller than 0.02.","Move around the circle. At a constrained stationary point, the gradient is normal to the constraint, even when it is not zero."}
}};
constexpr std::array<MathParameter,9> matrixParameters{MathParameter::A00,MathParameter::Shear,MathParameter::A02,MathParameter::A10,MathParameter::Scale,MathParameter::A12,MathParameter::A20,MathParameter::A21,MathParameter::A22};
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
double functionDerivative(unsigned rule,double x,unsigned order=0) {
  switch(rule) {
    case 0: {const std::array<double,3> d{x*x,2*x,2};return order<d.size()?d[order]:0;}
    case 1: {const std::array<double,4> d{x*x*x-3*x,3*x*x-3,6*x,6};return order<d.size()?d[order]:0;}
    case 2:return std::sin(x+order*pi/2);
    case 3:return std::exp(x)-(order==0?1:0);
    default:throw std::logic_error("invalid function rule");
  }
}
double primitive(unsigned rule,double x) {
  switch(rule) {
    case 0:return x*x*x/3;
    case 1:return x*x*x*x/4-1.5*x*x;
    case 2:return -std::cos(x);
    case 3:return std::exp(x)-x;
    default:throw std::logic_error("invalid function rule");
  }
}
double taylor(unsigned rule,double x,double center,unsigned degree) {
  double sum=0,power=1;
  for(unsigned k=0;k<=degree;++k) {sum+=functionDerivative(rule,center,k)*power;power*=(x-center)/(k+1);}
  return sum;
}
struct SurfaceValue { double height,du,dv,duu,duv,dvv; };
SurfaceValue surfaceValue(unsigned rule,double u,double v) {
  switch(rule) {
    case 0:return {.5*(u*u+v*v),u,v,1,0,1};
    case 1:return {.5*(u*u-v*v),u,-v,1,0,-1};
    case 2: {const double f=std::sin(u)*std::cos(v);return {f,std::cos(u)*std::cos(v),-std::sin(u)*std::sin(v),-f,-std::cos(u)*std::sin(v),-f};}
    default:throw std::logic_error("invalid surface rule");
  }
}
using Matrix = std::array<double,9>;
constexpr Matrix identity{1,0,0,0,1,0,0,0,1};
Matrix transpose(const Matrix& a) {Matrix r{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)r[3*i+j]=a[3*j+i];return r;}
Matrix multiply(const Matrix& a,const Matrix& b) {
  Matrix r{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)r[3*i+j]+=a[3*i+k]*b[3*k+j];return r;
}
Vec3 mapped(const Matrix& a,Vec3 v) {
  return {static_cast<float>(a[0]*v.x+a[1]*v.y+a[2]*v.z),static_cast<float>(a[3]*v.x+a[4]*v.y+a[5]*v.z),static_cast<float>(a[6]*v.x+a[7]*v.y+a[8]*v.z)};
}
double determinant(const Matrix& a) {return a[0]*(a[4]*a[8]-a[5]*a[7])-a[1]*(a[3]*a[8]-a[5]*a[6])+a[2]*(a[3]*a[7]-a[4]*a[6]);}
unsigned rank(Matrix a) {
  unsigned r=0;
  for(unsigned c=0;c<3&&r<3;++c) {
    unsigned pivot=r;for(unsigned i=r+1;i<3;++i)if(std::fabs(a[3*i+c])>std::fabs(a[3*pivot+c]))pivot=i;
    if(std::fabs(a[3*pivot+c])<1e-9)continue;
    for(unsigned j=0;j<3;++j)std::swap(a[3*r+j],a[3*pivot+j]);
    for(unsigned i=r+1;i<3;++i) {const double factor=a[3*i+c]/a[3*r+c];for(unsigned j=c;j<3;++j)a[3*i+j]-=factor*a[3*r+j];}
    ++r;
  }
  return r;
}
// Bounded Jacobi diagonalisation of A^T A. Columns are sorted by descending
// singular value; null columns of U are completed by deterministic Gram-Schmidt.
struct Svd { Matrix u=identity,v=identity;std::array<double,3> sigma{}; };
Svd svd(const Matrix& a) {
  Matrix gram=multiply(transpose(a),a),v=identity;
  for(unsigned sweep=0;sweep<24;++sweep)for(unsigned p=0;p<2;++p)for(unsigned q=p+1;q<3;++q) {
    const double off=gram[3*p+q];if(std::fabs(off)<1e-14)continue;
    const double theta=.5*std::atan2(2*off,gram[3*q+q]-gram[3*p+p]);
    Matrix rotation=identity;rotation[3*p+p]=rotation[3*q+q]=std::cos(theta);rotation[3*p+q]=std::sin(theta);rotation[3*q+p]=-std::sin(theta);
    gram=multiply(multiply(transpose(rotation),gram),rotation);v=multiply(v,rotation);
  }
  std::array<unsigned,3> order{0,1,2};std::sort(order.begin(),order.end(),[&](unsigned i,unsigned j){return gram[3*i+i]==gram[3*j+j]?i<j:gram[3*i+i]>gram[3*j+j];});
  Svd result;result.u={};result.v={};
  for(unsigned c=0;c<3;++c) {
    result.sigma[c]=std::sqrt(std::max(0.0,gram[3*order[c]+order[c]]));
    if(result.sigma[c]<1e-7)result.sigma[c]=0;
    for(unsigned r=0;r<3;++r)result.v[3*r+c]=v[3*r+order[c]];
    std::array<double,3> column{};
    if(result.sigma[c]>0)for(unsigned r=0;r<3;++r)for(unsigned k=0;k<3;++k)column[r]+=a[3*r+k]*result.v[3*k+c]/result.sigma[c];
    else {
      double best=-1;
      for(unsigned axis=0;axis<3;++axis) {
        std::array<double,3> candidate{};candidate[axis]=1;
        for(unsigned j=0;j<c;++j) {double d=0;for(unsigned r=0;r<3;++r)d+=candidate[r]*result.u[3*r+j];for(unsigned r=0;r<3;++r)candidate[r]-=d*result.u[3*r+j];}
        double norm=0;for(auto n:candidate)norm+=n*n;
        if(norm>best){best=norm;column=candidate;}
      }
    }
    double norm=0;for(auto n:column)norm+=n*n;norm=std::sqrt(norm);
    for(unsigned r=0;r<3;++r)result.u[3*r+c]=column[r]/norm;
  }
  return result;
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
  MathPlot& plot(std::string_view title,MathParameter scrub=MathParameter::Count) {
    if(s.plotCount==s.plots.size())throw std::logic_error("math plot capacity exceeded");
    auto& p=s.plots[s.plotCount++];p={};p.title=title;p.scrubParameter=scrub;return p;
  }
  template<class F> void curve(MathPlot& p,std::string_view name,Vec3 color,double from,double to,F f,bool filled=false) {
    if(p.seriesCount==p.series.size())throw std::logic_error("math series capacity exceeded");
    auto& line=p.series[p.seriesCount++];line={};line.name=name;line.color=color;line.count=line.points.size();line.signedFill=filled;
    for(std::size_t i=0;i<line.count;++i) {const double x=from+(to-from)*i/(line.count-1);line.points[i]={x,f(x)};}
  }
  void matrix(std::string_view name,const Matrix& values,bool editable=false) {
    if(s.matrixCount==s.matrices.size())throw std::logic_error("math matrix capacity exceeded");
    auto& view=s.matrices[s.matrixCount++];view={};view.name=name;view.values=values;view.editable=editable;view.parameters=matrixParameters;
  }
  MathObjectSnapshot& s;
};
} // namespace

std::span<const MathObjectSpec> mathObjectSpecs(){return objects;}
std::span<const MathParameterSpec> mathParameterSpecs(){return parameters;}
std::span<const MathLesson> mathLessons(MathObjectKind kind) {
  switch(kind) {
    case MathObjectKind::Function:return functionLessons;
    case MathObjectKind::Linear:return linearLessons;
    case MathObjectKind::Surface:return surfaceLessons;
    default:return {};
  }
}
std::span<const std::string_view> mathMatrixPresetNames() {
  static constexpr std::array<std::string_view,5> names{"Identity","Shear","Project onto xy","Stretch and reflect","Rotate 90 degrees about z"};return names;
}
MathObjects::MathObjects() {for(const auto& p:parameters)parameters_[index(p.id)]=p.initial;rebuild();}
double MathObjects::parameter(MathParameter p) const {
  if(index(p)>=parameters_.size())throw std::invalid_argument("unknown math parameter");
  return parameters_[index(p)];
}
bool MathObjects::parameterAvailable(MathParameter p) const {
  if(index(p)>=parameters.size())return false;
  const auto& spec=parameters[index(p)];
  if(spec.owner!=snapshot_.kind||spec.minimumLevel>snapshot_.level)return false;
  switch(p) {
    case MathParameter::SurfaceU:case MathParameter::SurfaceV:case MathParameter::DirectionAngle:case MathParameter::DescentRate:
      return parameter(MathParameter::Constraint)==0;
    case MathParameter::CircleAngle:return parameter(MathParameter::Constraint)!=0;
    case MathParameter::ComposeAngle:return snapshot_.level==1;
    default:return true;
  }
}
MathActionResult MathObjects::dispatch(const MathAction& a) {
  switch(a.kind) {
    case MathActionKind::Select:
      if(index(a.object)>=objects.size())return {false,"unknown_object"};
      snapshot_.kind=a.object;snapshot_.level=0;
      for(const auto& p:parameters)if(p.owner==snapshot_.kind&&p.minimumLevel>0)parameters_[index(p.id)]=p.initial;
      break;
    case MathActionKind::SetParameter: {
      if(index(a.parameter)>=parameters.size())return {false,"unknown_parameter"};
      const auto& p=parameters[index(a.parameter)];
      if(p.owner!=snapshot_.kind)return {false,"parameter_not_owned_by_object"};
      if(!parameterAvailable(a.parameter))return {false,"parameter_not_available_at_this_level"};
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
    case MathActionKind::SetLevel: {
      const auto count=std::max<std::size_t>(1,mathLessons(snapshot_.kind).size());
      if(!std::isfinite(a.value)||a.value<0||a.value>=count||std::floor(a.value)!=a.value)return {false,"unknown_learning_level"};
      snapshot_.level=static_cast<unsigned>(a.value);
      for(const auto& p:parameters)if(p.owner==snapshot_.kind&&p.minimumLevel>snapshot_.level)parameters_[index(p.id)]=p.initial;
      break;
    }
    case MathActionKind::SwapBounds: {
      if(snapshot_.kind!=MathObjectKind::Function||snapshot_.level<2)return {false,"accumulation_not_available"};
      const auto rule=static_cast<unsigned>(parameter(MathParameter::FunctionRule));
      reversedNegativeIntegral_=primitive(rule,parameter(MathParameter::FunctionX))-primitive(rule,parameter(MathParameter::IntegralStart))<-.001;
      std::swap(parameters_[index(MathParameter::FunctionX)],parameters_[index(MathParameter::IntegralStart)]);break;
    }
    case MathActionKind::DescentStep: {
      if(snapshot_.kind!=MathObjectKind::Surface||!parameterAvailable(MathParameter::DescentRate))return {false,"descent_not_available"};
      const auto rule=static_cast<unsigned>(parameter(MathParameter::SurfaceRule));
      const double u=parameter(MathParameter::SurfaceU),v=parameter(MathParameter::SurfaceV);const auto current=surfaceValue(rule,u,v);
      if(std::hypot(current.du,current.dv)<1e-9)return {false,"already_stationary"};
      double rate=parameter(MathParameter::DescentRate);bool accepted=false;
      for(unsigned trial=0;trial<12;++trial,rate*=.5) {
        const double nu=std::clamp(u-rate*current.du,-2.0,2.0),nv=std::clamp(v-rate*current.dv,-2.0,2.0);
        if(surfaceValue(rule,nu,nv).height<current.height) {parameters_[index(MathParameter::SurfaceU)]=nu;parameters_[index(MathParameter::SurfaceV)]=nv;accepted=true;break;}
      }
      if(!accepted)return {false,"no_decreasing_step_in_domain"};break;
    }
    case MathActionKind::MatrixPreset: {
      if(snapshot_.kind!=MathObjectKind::Linear||snapshot_.level==0)return {false,"matrix_editor_not_available"};
      static constexpr std::array<Matrix,5> presets{identity,Matrix{1,.6,0,0,1,0,0,0,1},Matrix{1,0,0,0,1,0,0,0,0},Matrix{2,0,0,0,1,0,0,0,-1},Matrix{0,-1,0,1,0,0,0,0,1}};
      if(a.vertex>=presets.size())return {false,"unknown_matrix_preset"};
      for(unsigned i=0;i<9;++i)parameters_[index(matrixParameters[i])]=presets[a.vertex][i];break;
    }
    case MathActionKind::MoveSurfacePoint:
      if(snapshot_.kind!=MathObjectKind::Surface||!parameterAvailable(MathParameter::SurfaceU))return {false,"surface_point_not_available"};
      if(!std::isfinite(a.value)||!std::isfinite(a.secondary)||std::fabs(a.value)>2||std::fabs(a.secondary)>2)return {false,"surface_point_out_of_range"};
      parameters_[index(MathParameter::SurfaceU)]=a.value;parameters_[index(MathParameter::SurfaceV)]=a.secondary;break;
    case MathActionKind::Check:check();return {true,"challenge_checked"};
    default:return {false,"unknown_action"};
  }
  if(a.kind!=MathActionKind::SwapBounds)reversedNegativeIntegral_=false;
  snapshot_.feedback=MathFeedback::None;snapshot_.feedbackText={};++snapshot_.revision;rebuild();return {true,"applied"};
}
void MathObjects::check() {
  bool solved=false;std::string_view good,bad;
  const auto measured=[&](std::string_view name) {
    for(std::size_t i=0;i<snapshot_.metricCount;++i)if(snapshot_.metrics[i].label==name)return snapshot_.metrics[i].value;
    throw std::logic_error("missing challenge measurement");
  };
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
      switch(snapshot_.level) {
        case 0:solved=measured("Rank")==2;good="Yes: rank 2 and zero three-dimensional volume.";bad="Change vertical scale. A shear alone preserves volume.";break;
        case 1:solved=measured("Vector length")>.1&&measured("Length of A v")>.01&&measured("Eigenvector residual")<.01;good="Yes: A v remains on the line spanned by nonzero v.";bad="Try the stretch/reflection preset and align v with a coordinate axis.";break;
        case 2:solved=measured("Vector length")>.1&&measured("Residual length")<.01;good="Yes: the nonzero vector lies in the column span.";bad="Choose v in the span of the first two columns. The projection residual must vanish.";break;
        case 3:solved=measured("Rank")==2&&measured("Singular value 3")<1e-7;good="Yes: one stretch vanished and two independent directions remain.";bad="Try the xy projection preset and inspect the three singular values.";break;
      }
      break;
    case MathObjectKind::Discrete:
      solved=snapshot_.route[snapshot_.routeCount-1]==7 && snapshot_.routeCount-1==snapshot_.shortestHops;
      good="Yes: your route reaches H in the minimum number of edge hops.";bad="Reach H using the fewest hops. Every edge costs one.";break;
    case MathObjectKind::Function:
      switch(snapshot_.level) {
        case 0:solved=std::fabs(measured("f(x)"))<.01;good="Yes: the output is zero to within 0.01.";bad="Move x toward a crossing or touch of the horizontal axis.";break;
        case 1:solved=std::fabs(parameter(MathParameter::DeltaX))>1e-12&&measured("Secant error")<.02;good="Yes: a nonzero secant step approximates the derivative within 0.02.";bad="Reduce the nonzero magnitude of h and compare the slopes.";break;
        case 2:solved=reversedNegativeIntegral_&&measured("Signed integral")>.001;good="Yes: reversing the bounds turned the negative integral into its positive opposite.";bad="First make the integral negative, then press Swap bounds and check.";break;
        case 3:solved=std::fabs(parameter(MathParameter::FunctionX)-parameter(MathParameter::TaylorCenter))>=.5-1e-9&&measured("Taylor error")<.001;good="Yes: the Taylor value is within 0.001 at least 0.5 from its centre.";bad="Move at least 0.5 from the centre and adjust the approximation degree.";break;
      }
      break;
    case MathObjectKind::Surface:
      switch(snapshot_.level) {
        case 0:solved=parameter(MathParameter::SurfaceRule)==0&&std::fabs(measured("Height")-1)<.01;good="Yes: the bowl has height 1 here.";bad="Select the bowl and try u=1, v=1.";break;
        case 1:solved=parameter(MathParameter::SurfaceRule)==0&&measured("Gradient magnitude")>.5&&std::fabs(measured("Directional derivative"))<.02;good="Yes: that direction is tangent to a contour and has almost zero slope.";bad="Away from the origin on the bowl, rotate the direction perpendicular to the gradient.";break;
        case 2:solved=parameter(MathParameter::SurfaceRule)==0&&measured("Gradient magnitude")<.02;good="Yes: descent reached the bowl's minimum to within the gradient tolerance.";bad="Select the bowl and take descent steps toward its minimum.";break;
        case 3:solved=parameter(MathParameter::SurfaceRule)==1&&parameter(MathParameter::Constraint)==1&&std::fabs(measured("Tangential derivative"))<.02;good="Yes: this is stationary along the circle although the full gradient need not vanish.";bad="Select the saddle, enable the circle, and try a point on a coordinate axis.";break;
      }
      break;
    case MathObjectKind::Count:return;
  }
  snapshot_.feedback=solved?MathFeedback::Solved:MathFeedback::TryAgain;snapshot_.feedbackText=solved?good:bad;
}
void MathObjects::rebuild() {
  snapshot_.partCount=0;snapshot_.labelCount=0;snapshot_.metricCount=0;snapshot_.allowedVertices.fill(false);
  snapshot_.plotCount=0;snapshot_.matrixCount=0;snapshot_.surface.rows=0;snapshot_.surface.columns=0;
  snapshot_.contours.count=0;snapshot_.contours.active=false;
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
      Matrix a{};for(unsigned i=0;i<9;++i)a[i]=parameter(matrixParameters[i]);
      const auto transformed=[&](Vec3 p){return mapped(a,p);};
      const auto cage=[&](const Matrix& matrix,Vec3 color,float radius,std::string_view role) {
        for(unsigned i=0;i<8;++i)for(unsigned axis=0;axis<3;++axis) {
          const unsigned j=i^(1U<<axis);if(j<i)continue;
          const auto corner=[](unsigned n){return Vec3{static_cast<float>(n&1),static_cast<float>((n>>1)&1),static_cast<float>((n>>2)&1)};};
          b.rod(mapped(matrix,corner(i)),mapped(matrix,corner(j)),color,radius,role);
        }
      };
      cage(identity,muted,.008F,"original_cube");cage(a,teal,.018F,"transformed_cube");
      if(snapshot_.level==0)for(unsigned t=1;t<4;++t)for(float face:{0.0F,1.0F}) {
        const float q=t*.25F;
        b.rod(transformed({q,face,0}),transformed({q,face,1}),teal,.005F,"lattice");
        b.rod(transformed({0,q,face}),transformed({1,q,face}),teal,.005F,"lattice");
        b.rod(transformed({face,0,q}),transformed({face,1,q}),teal,.005F,"lattice");
      }
      const std::array<Vec3,3> basis{{{1,0,0},{0,1,0},{0,0,1}}},colors{coral,teal,violet};
      const std::array<std::string_view,3> names{"A e1","A e2","A e3"};
      for(unsigned i=0;i<3;++i) {b.arrow({},transformed(basis[i]),colors[i],names[i]);b.label(names[i],transformed(basis[i])+Vec3{.05F,.12F,.05F},colors[i]);}
      b.ball({},.03F,white,"origin");
      const double d=determinant(a);b.metric("Signed determinant",d);b.metric("Volume",std::fabs(d),"units^3");b.metric("Rank",rank(a));
      b.matrix("A",a,snapshot_.level>0);
      if(snapshot_.level==0)break;
      const Vec3 v{value(MathParameter::VectorX),value(MathParameter::VectorY),value(MathParameter::VectorZ)},av=transformed(v);
      b.arrow({},v,gold,"input_vector");b.label("v",v+Vec3{0,.15F,0},gold);b.metric("Vector length",length(v));
      switch(snapshot_.level) {
        case 1: {
          const double angle=parameter(MathParameter::ComposeAngle)*pi/180,c=std::cos(angle),s=std::sin(angle);
          const Matrix rotation{c,-s,0,s,c,0,0,0,1},ba=multiply(rotation,a),ab=multiply(a,rotation);
          b.matrix("B A",ba);b.matrix("A B",ab);cage(ba,violet,.012F,"composed_cube");
          const Vec3 bav=mapped(ba,v);b.arrow({},av,teal,"mapped_vector");b.arrow({},bav,violet,"composed_vector");
          b.label("A v",av+Vec3{0,.15F,0},teal);b.label("B A v",bav+Vec3{0,.15F,0},violet);
          const double norm2=dot(v,v),lambda=norm2>1e-12?dot(v,av)/norm2:0;
          b.metric("Length of A v",length(av));b.metric("Eigenvalue candidate",lambda);b.metric("Eigenvector residual",length(av-v*static_cast<float>(lambda)));
          double orderError=0;for(unsigned i=0;i<9;++i)orderError+=(ba[i]-ab[i])*(ba[i]-ab[i]);b.metric("Composition order difference",std::sqrt(orderError));break;
        }
        case 2: {
          std::array<Vec3,2> q{};unsigned count=0;
          for(unsigned i=0;i<2;++i) {
            Vec3 candidate=transformed(basis[i]);for(unsigned j=0;j<count;++j)candidate=candidate-q[j]*dot(candidate,q[j]);
            if(length(candidate)>1e-6F)q[count++]=normalized(candidate);
          }
          Vec3 projected{};Matrix projection{};
          for(unsigned i=0;i<count;++i) {
            projected=projected+q[i]*dot(v,q[i]);const std::array<double,3> coordinates{q[i].x,q[i].y,q[i].z};
            for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)projection[3*r+c]+=coordinates[r]*coordinates[c];
            b.rod(q[i]*-2,q[i]*2,blue,.012F,"column_span");
          }
          const Vec3 residual=v-projected;b.arrow({},projected,teal,"projection");b.rod(projected,v,gold,.022F,"projection_residual");
          b.label("projection",projected+Vec3{0,.15F,0},teal);b.matrix("Projection P",projection);
          b.metric("Subspace dimension",count);b.metric("Residual length",length(residual));b.metric("Projected length",length(projected));
          double error=0;for(unsigned i=0;i<count;++i)error=std::max(error,static_cast<double>(std::fabs(dot(residual,q[i]))));b.metric("Orthogonality error",error);break;
        }
        case 3: {
          const auto decomposition=svd(a);Matrix sigma{};for(unsigned i=0;i<3;++i)sigma[4*i]=decomposition.sigma[i];
          const auto vt=transpose(decomposition.v),stretch=multiply(sigma,vt),reconstructed=multiply(decomposition.u,stretch);
          const std::array<Matrix,4> stages{identity,vt,stretch,reconstructed};const auto& transform=stages[static_cast<unsigned>(parameter(MathParameter::SvdStage))];
          const std::array<std::string_view,3> stageNames{"stage e1","stage e2","stage e3"};
          for(unsigned i=0;i<3;++i) {const auto tip=mapped(transform,basis[i]);b.arrow({},tip,colors[i],stageNames[i]);b.label(stageNames[i],tip+Vec3{.1F,.15F,.1F},colors[i]);}
          b.matrix("U",decomposition.u);b.matrix("V transpose",vt);
          b.metric("Singular value 1",decomposition.sigma[0]);b.metric("Singular value 2",decomposition.sigma[1]);b.metric("Singular value 3",decomposition.sigma[2]);
          double error=0;for(unsigned i=0;i<9;++i)error=std::max(error,std::fabs(reconstructed[i]-a[i]));b.metric("SVD reconstruction error",error);
          auto& patch=snapshot_.surface;patch.rows=17;patch.columns=21;
          for(unsigned row=0;row<patch.rows;++row)for(unsigned col=0;col<patch.columns;++col) {
            const double latitude=pi*row/(patch.rows-1),longitude=2*pi*col/(patch.columns-1);
            const Vec3 unit{static_cast<float>(std::sin(latitude)*std::cos(longitude)),static_cast<float>(std::cos(latitude)),static_cast<float>(std::sin(latitude)*std::sin(longitude))};
            auto& vertex=patch.vertices[row*patch.columns+col];vertex.position=mapped(transform,unit);vertex.normal=unit;
            const float weight=std::fabs(unit.x)+std::fabs(unit.y)+std::fabs(unit.z);
            vertex.color=(coral*std::fabs(unit.x)+teal*std::fabs(unit.y)+violet*std::fabs(unit.z))*(1/weight);
          }
          // Normals from transformed tangents also cover reflected and collapsed surfaces.
          for(unsigned row=0;row<patch.rows;++row)for(unsigned col=0;col<patch.columns;++col) {
            const unsigned lo=row?row-1:row,hi=std::min(row+1,patch.rows-1),left=col?col-1:patch.columns-2,right=(col+1)%(patch.columns-1);
            const auto across=patch.vertices[row*patch.columns+right].position-patch.vertices[row*patch.columns+left].position;
            const auto down=patch.vertices[hi*patch.columns+col].position-patch.vertices[lo*patch.columns+col].position;
            auto& vertex=patch.vertices[row*patch.columns+col];const auto n=cross(across,down);vertex.normal=length(n)>1e-8F?normalized(n):Vec3{0,1,0};
          }
          break;
        }
      }
      break;
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
    case MathObjectKind::Function: {
      const unsigned rule=static_cast<unsigned>(parameter(MathParameter::FunctionRule));
      const double x=parameter(MathParameter::FunctionX),h=parameter(MathParameter::DeltaX),a=parameter(MathParameter::IntegralStart),center=parameter(MathParameter::TaylorCenter);
      const auto f=[&](double t){return functionDerivative(rule,t);};
      const auto derivative=[&](double t){return functionDerivative(rule,t,1);};
      const double fx=f(x),slope=derivative(x),secant=h==0?slope:(f(x+h)-fx)/h;
      auto& graph=b.plot("Function f(x)",MathParameter::FunctionX);b.curve(graph,"f",teal,-3,3,f);graph.marker={x,fx};graph.hasMarker=true;
      for(unsigned i=0;i<128;i+=2) {
        const auto p=graph.series[0].points[i],q=graph.series[0].points[i+2];
        b.rod({static_cast<float>(p.x),static_cast<float>(p.y),0},{static_cast<float>(q.x),static_cast<float>(q.y),0},teal,.022F,"function_curve");
      }
      b.arrow({-3.2F,0,0},{3.2F,0,0},muted,"x_axis");b.arrow({0,-3.2F,0},{0,4.2F,0},muted,"y_axis");
      const Vec3 point{static_cast<float>(x),static_cast<float>(fx),.05F};b.ball(point,.07F,gold,"function_point");b.label("f(x)",point+Vec3{.1F,.2F,0},gold);
      b.metric("f(x)",fx);b.metric("f'(x)",slope);
      if(snapshot_.level>=1) {
        b.rod({static_cast<float>(x-.5),static_cast<float>(fx-.5*slope),.025F},{static_cast<float>(x+.5),static_cast<float>(fx+.5*slope),.025F},coral,.024F,"tangent");
        const Vec3 end{static_cast<float>(x+h),static_cast<float>(f(x+h)),.07F};b.ball(end,.05F,blue,"secant_point");b.rod(point,end,gold,.019F,"secant");
        auto& plot=b.plot("Derivative f'(x)",MathParameter::FunctionX);b.curve(plot,"f'",coral,-3,3,derivative);plot.marker={x,slope};plot.hasMarker=true;
        b.metric("Secant slope",secant);b.metric("Secant error",std::fabs(secant-slope));
      }
      if(snapshot_.level>=2) {
        const auto accumulated=[&](double t){return primitive(rule,t)-primitive(rule,a);};
        auto& plot=b.plot("Accumulation F(x)",MathParameter::FunctionX);b.curve(plot,"Integral from a",blue,-3,3,accumulated);plot.marker={x,accumulated(x)};plot.hasMarker=true;
        b.curve(graph,"Between bounds",blue,std::min(a,x),std::max(a,x),f,true);
        b.metric("Signed integral",accumulated(x));b.metric("F'(x) = f(x)",fx);
        if(std::fabs(x-a)>1e-9) {
          auto& patch=snapshot_.surface;patch.rows=2;patch.columns=65;
          for(unsigned i=0;i<patch.columns;++i) {
            const double t=std::min(a,x)+std::fabs(x-a)*i/(patch.columns-1),height=f(t);const auto color=height>=0?blue:coral;
            patch.vertices[i]={{static_cast<float>(t),0,-.015F},{0,0,1},color};
            patch.vertices[patch.columns+i]={{static_cast<float>(t),static_cast<float>(height),-.015F},{0,0,1},color};
          }
        }
      }
      if(snapshot_.level==3) {
        const auto degree=static_cast<unsigned>(parameter(MathParameter::TaylorDegree));const auto approximation=[&](double t){return taylor(rule,t,center,degree);};
        b.curve(graph,"Taylor",violet,-3,3,approximation);
        b.metric("Taylor value",approximation(x));b.metric("Taylor error",std::fabs(approximation(x)-fx));
      }
      break;
    }
    case MathObjectKind::Surface: {
      const unsigned rule=static_cast<unsigned>(parameter(MathParameter::SurfaceRule));const bool constrained=parameter(MathParameter::Constraint)!=0;
      const double angle=parameter(MathParameter::CircleAngle)*pi/180;
      const double u=constrained?std::cos(angle):parameter(MathParameter::SurfaceU),v=constrained?std::sin(angle):parameter(MathParameter::SurfaceV);
      const auto sample=surfaceValue(rule,u,v);auto& patch=snapshot_.surface;patch.rows=patch.columns=MathSurfacePatch::kResolution;
      for(unsigned row=0;row<patch.rows;++row)for(unsigned col=0;col<patch.columns;++col) {
        const double pu=-2+4.0*col/(patch.columns-1),pv=-2+4.0*row/(patch.rows-1);const auto value=surfaceValue(rule,pu,pv);
        const float blend=static_cast<float>(std::clamp((value.height+2)/6,0.0,1.0));
        patch.vertices[row*patch.columns+col]={{static_cast<float>(pu),static_cast<float>(value.height),static_cast<float>(pv)},normalized(Vec3{static_cast<float>(-value.du),1,static_cast<float>(-value.dv)}),blue*(1-blend)+teal*blend};
      }
      auto& contours=snapshot_.contours;contours.active=true;contours.point={u,v};contours.gradient={sample.du,sample.dv};contours.constrained=constrained;
      const auto contourTriangle=[&](unsigned ia,unsigned ib,unsigned ic) {
        const std::array<Vec3,3> vertices{patch.vertices[ia].position,patch.vertices[ib].position,patch.vertices[ic].position};
        for(double height:std::array<double,6>{-1,0,.5,1,2,3}) {
          std::array<MathPlotPoint,2> crossings{};unsigned count=0;
          for(unsigned edge=0;edge<3;++edge) {
            const auto p=vertices[edge],q=vertices[(edge+1)%3];
            if((p.y<=height&&q.y>height)||(q.y<=height&&p.y>height)) {
              const double t=(height-p.y)/(q.y-p.y);crossings[count++]={p.x+t*(q.x-p.x),p.z+t*(q.z-p.z)};
            }
          }
          if(count==2&&std::hypot(crossings[0].x-crossings[1].x,crossings[0].y-crossings[1].y)>1e-10) {
            if(contours.count==contours.segments.size())throw std::logic_error("contour capacity exceeded");
            contours.segments[contours.count++]={crossings[0],crossings[1],height};
          }
        }
      };
      for(unsigned row=0;row+1<patch.rows;++row)for(unsigned col=0;col+1<patch.columns;++col) {
        const unsigned i=row*patch.columns+col,j=i+patch.columns;contourTriangle(i,j,i+1);contourTriangle(i+1,j,j+1);
      }
      const Vec3 point{static_cast<float>(u),static_cast<float>(sample.height+.04),static_cast<float>(v)};
      b.ball(point,.08F,gold,"surface_point");b.label("f(u,v)",point+Vec3{0,.2F,0},gold);
      auto& uPlot=b.plot("u section: v fixed",constrained?MathParameter::Count:MathParameter::SurfaceU);
      b.curve(uPlot,"f(u,v0)",coral,-2,2,[&](double t){return surfaceValue(rule,t,v).height;});uPlot.marker={u,sample.height};uPlot.hasMarker=true;
      auto& vPlot=b.plot("v section: u fixed",constrained?MathParameter::Count:MathParameter::SurfaceV);
      b.curve(vPlot,"f(u0,v)",violet,-2,2,[&](double t){return surfaceValue(rule,u,t).height;});vPlot.marker={v,sample.height};vPlot.hasMarker=true;
      for(unsigned i=0;i<32;++i) {
        const double a=-2+i*.125,c=a+.125;
        b.rod({static_cast<float>(a),static_cast<float>(surfaceValue(rule,a,v).height+.012),static_cast<float>(v)},{static_cast<float>(c),static_cast<float>(surfaceValue(rule,c,v).height+.012),static_cast<float>(v)},coral,.017F,"u_section");
        b.rod({static_cast<float>(u),static_cast<float>(surfaceValue(rule,u,a).height+.012),static_cast<float>(a)},{static_cast<float>(u),static_cast<float>(surfaceValue(rule,u,c).height+.012),static_cast<float>(c)},violet,.017F,"v_section");
      }
      b.metric("Height",sample.height);b.metric("Partial u",sample.du);b.metric("Partial v",sample.dv);b.metric("Gradient magnitude",std::hypot(sample.du,sample.dv));
      if(snapshot_.level>=1) {
        const auto plane=[&](double x,double y){return Vec3{static_cast<float>(u+x),static_cast<float>(sample.height+sample.du*x+sample.dv*y+.025),static_cast<float>(v+y)};};
        for(double offset:{-.45,0.0,.45}) {b.rod(plane(-.45,offset),plane(.45,offset),gold,.009F,"tangent_plane");b.rod(plane(offset,-.45),plane(offset,.45),gold,.009F,"tangent_plane");}
        const double angle=parameter(MathParameter::DirectionAngle)*pi/180,dx=std::cos(angle),dy=std::sin(angle);
        b.arrow(point,point+Vec3{static_cast<float>(dx*.6),0,static_cast<float>(dy*.6)},white,"input_direction");
        b.arrow(point,point+Vec3{static_cast<float>(sample.du*.3),0,static_cast<float>(sample.dv*.3)},teal,"gradient_direction_scaled");
        b.metric("Directional derivative",sample.du*dx+sample.dv*dy);
      }
      if(snapshot_.level>=2) {
        const double hessianDet=sample.duu*sample.dvv-sample.duv*sample.duv;
        b.metric("Hessian determinant",hessianDet);b.metric("Curvature in u",sample.duu);
      }
      if(snapshot_.level==3&&constrained) {
        const double tangent=-sample.du*v+sample.dv*u,lambda=(sample.du*u+sample.dv*v)/2;
        b.metric("Tangential derivative",tangent);b.metric("Lagrange multiplier",lambda);b.metric("Constraint error",std::fabs(u*u+v*v-1));
        for(unsigned i=0;i<48;++i) {
          const double a=2*pi*i/48,c=2*pi*(i+1)/48;
          const auto position=[&](double t){return Vec3{static_cast<float>(std::cos(t)),static_cast<float>(surfaceValue(rule,std::cos(t),std::sin(t)).height+.03),static_cast<float>(std::sin(t))};};
          b.rod(position(a),position(c),white,.02F,"constraint_curve");
        }
      }
      break;
    }
    case MathObjectKind::Count:throw std::logic_error("invalid math object state");
  }
}
} // namespace paths
