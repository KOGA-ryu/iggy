#include "Textbook.hpp"
#include <utility>
namespace paths {
namespace {
BookPassage p(const char* text){return {BookPassage::Kind::Prose,text};}
BookPassage m(const char* text){return {BookPassage::Kind::DisplayMath,text};}
using P=std::vector<BookPassage>;
std::array<P,4> proof(P text){return {std::move(text),{},{},{}};}
}
const std::vector<BookBlock>& systemsLesson(){
  static const std::vector<BookBlock> blocks{
    {"systems.start",BookBlockKind::Introduction,"","From row reduction to solution sets",
      {p("Prerequisite: Section 1.2, Row operations and RREF. Now keep an explicit right-hand side with each row. Our question is whether the equations share a point, and how much freedom remains when they do."),
       m(R"(A\mathbf{x}=\mathbf{b},\qquad [A\mid\mathbf{b}])"),
       p("A nonzero real equation in three variables describes an infinite plane. Its coloured grid shows only a portion. Changing a coefficient changes its orientation; changing the right-hand side translates it along its normal. A solution must satisfy every equation at once.")}},
    {"systems.consistency",BookBlockKind::Definition,"1.3.1","Consistency",
      {p("A system is consistent when at least one solution exists, and inconsistent when no solution exists. During row reduction, keep b attached to A. A zero coefficient row is either harmless or contradictory:"),
       m(R"(0x+0y+0z=0\qquad\text{or}\qquad 0x+0y+0z=c,\quad c\ne0)"),
       p("The first equation places no restriction on a point. The second equation is impossible. Neither has a plane to draw. A missing plane is therefore not by itself evidence that a constraint was simply redundant.")}},
    {"systems.rank",BookBlockKind::Definition,"1.3.2","Rank and augmented rank",
      {p("The rank of A is the dimension of its column space, and equals the number of coefficient pivots in an echelon form. The augmented rank is the rank of [A | b]. An extra pivot in the right-hand-side column identifies a contradiction."),
       m(R"(A\mathbf{x}=\mathbf{b}\text{ is consistent}\quad\Longleftrightarrow\quad\operatorname{rank}(A)=\operatorname{rank}([A\mid\mathbf{b}]))"),
       p("Appending one column can increase rank by at most one. Do not count a pivot in b as a pivot for one of the unknown variables.")}},
    {"systems.nullity",BookBlockKind::Definition,"1.3.3","Null space and free variables",
      {p("The null space of A consists of the vectors sent to zero. Its dimension is called nullity. In a system with three unknowns, each nonpivot coefficient column supplies one free variable."),
       m(R"(\ker(A)=\{\mathbf{v}:A\mathbf{v}=0\},\qquad\operatorname{rank}(A)+\operatorname{nullity}(A)=3)"),
       p("Nullity describes the homogeneous system even if Ax=b is inconsistent. It counts free parameters of Ax=b only when that system is consistent. A contradictory system with nullity one does not have a line of solutions.")}},
    {"systems.family",BookBlockKind::Proposition,"1.3.4","All solutions are a translated null space",
      {p("Suppose the system is consistent and xp is any one solution. Every solution is obtained by adding a null-space vector to xp:"),
       m(R"(\mathbf{x}=\mathbf{x}_p+\mathbf{v},\qquad\mathbf{v}\in\ker(A))"),
       p("With three variables this gives a point, line, plane, or all of space as nullity runs from zero to three. The null space itself always passes through the origin; its translate need not.")},
      proof({p("If A xp=b and A v=0, then A(xp+v)=b. Conversely, if A x=b, subtract A xp=b to obtain A(x-xp)=0. Thus x-xp belongs to the null space. Both inclusions are established.")}),
      {{"Definition 1.3.3 / the homogeneous null space","systems.nullity"}}},
    {"systems.point",BookBlockKind::Example,"1.3.5","One common point",
      {m(R"(x+y=1,\qquad y+z=1,\qquad x+z=1)"),
       p("The coefficient matrix has three pivots. Subtracting equations shows x=y=z, and substitution gives (1/2,1/2,1/2). The three planes share exactly this point. With an invertible coefficient matrix, every new right-hand side still gives one solution.")}},
    {"systems.line",BookBlockKind::Example,"1.3.6","A line of common points",
      {m(R"(x+y=1,\qquad y+z=1,\qquad x+2y+z=2)"),
       p("The third equation is the sum of the first two, so it adds no new constraint. Set z=t, then y=1-t and x=t:"),
       m(R"((x,y,z)=(0,1,0)+t(1,-1,1),\qquad t\in\mathbb{R})"),
       p("Rank is two and nullity is one. The figure uses an orthonormal direction and a minimum-norm starting point for stable probe controls. Its slider parameter need not equal this written t; both describe the same infinite line.")}},
    {"systems.none",BookBlockKind::Example,"1.3.7","Pairwise intersections are not enough",
      {m(R"(x=0,\qquad y=0,\qquad x+y=1)"),
       p("Each pair of these planes intersects in a line. Yet the first two equations force x+y=0, contradicting the third. Subtract the first two rows from the third to obtain 0=1. Rank(A)=2 and rank([A | b])=3. No shared point exists."),
       p("The parallel-plane example is another route to inconsistency. All equations, rather than just pairs, must hold at a solution.")}, {},
      {{"Definition 1.3.1 / identify the contradiction","systems.consistency"}}},
    {"systems.figure",BookBlockKind::Figure,"1.3.8","Move the equations, preserve the solutions",
      {p("Choose One solution and move a right-hand side. Next choose Infinitely many / a line: change b3 away from 2 to break consistency, then restore it. Finally compare the two No solution examples."),
       p("Editing a given starts a new elimination trace for that edited system. Next pivot and manual row operations transform the whole augmented system and preserve its common solutions. Original planes, the gold solution set, and a movable solution probe let you compare these two kinds of change."),
       p("The grids are finite windows onto infinite planes. If a solution lies far from the origin, the figure uses a labelled uniform display scale. Gold marks a shared solution set only when the system is consistent.")}},
    {"systems.practice",BookBlockKind::Exercise,"1.3.9","Predict, justify, then test a point",
      {p("Open Exercise for three separate practice systems. Before seeing the result, choose how many solutions exist and the reason for your prediction. The given equations are visible; the classification and reduction are withheld until you submit or explicitly reveal them."),
       p("After the explanation opens, step through reduction and inspect the right-hand side. For a consistent system, enter a proposed point and check all three original equations. A passed point check shows membership, not that you have described every solution.")}},
    {"systems.summary",BookBlockKind::Summary,"","What to carry forward",
      {p("First decide consistency. Then, if consistent, count free variables. Zero free variables gives a unique point; one or two give a line or plane. The null space controls directions, while the right-hand side controls whether solutions exist and where they lie."),
       p("These systems are separately authored teaching examples. The six source exercise cards remain available in their existing sections. Continue to Section 1.4 for systems and partial pivoting.")}}
  };
  return blocks;
}
}
