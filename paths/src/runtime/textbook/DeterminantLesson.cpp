#include "Textbook.hpp"
#include <utility>

namespace paths {
const ObjectLessonSpec& determinantVolumeSpec(){
  using P=MathParameter;
  static const ObjectLessonSpec spec{
    MathObjectKind::Linear,0,
    "det(A) = s; geometric volume = |s|",
    "Grey: the original unit cube. Teal: its image. Coral, teal and violet arrows are A e1, A e2 and A e3. Positive determinant preserves orientation; negative reverses it; zero collapses this family into a plane.",
    "Make the image of the unit cube collapse into a plane, then press Check.",
    {{P::Shear,"Shear k"},{P::Scale,"Vertical scale s"}},
    {{"Unit cube","Start with the identity: determinant 1 and volume 1.",{{P::Shear,0},{P::Scale,1}}},
     {"Shear","The sloping edges change angles, while determinant and volume remain 1.",{{P::Shear,1},{P::Scale,1}}},
     {"Stretch","Doubling the vertical scale doubles the volume.",{{P::Shear,0},{P::Scale,2}}},
     {"Reflection","A negative scale reverses orientation. The volume is still positive.",{{P::Shear,.6},{P::Scale,-1}}},
     {"Collapse","The image lies in y=0. Two independent directions remain.",{{P::Shear,.6},{P::Scale,0}}}},
    {"Signed determinant","Volume","Rank"},{"A"}
  };
  return spec;
}
const std::vector<BookBlock>& determinantLesson(){
  using Kind=BookPassage::Kind;
  const auto p=[](std::string s){return BookPassage{Kind::Prose,std::move(s)};};
  const auto m=[](std::string s){return BookPassage{Kind::DisplayMath,std::move(s)};};
  const auto help=[](BookPassage hint,BookPassage answer,BookPassage solution){
    std::array<std::vector<BookPassage>,4> h;h[1].push_back(std::move(hint));h[2].push_back(std::move(answer));h[3].push_back(std::move(solution));return h;
  };
  static const std::vector<BookBlock> blocks{
    {"determinant.intro",BookBlockKind::Introduction,"","Watch a matrix move a solid",
      {p("A linear map sends each vector to a new vector. Its columns tell you where the three standard basis vectors go. Moving all points of a unit cube produces a parallelepiped, which can stretch, slant, reverse orientation, or collapse."),
       p("This lesson studies one bounded family of real 3 by 3 matrices. Change the shear and vertical scale, then compare a signed determinant with a nonnegative geometric volume.")}},
    {"determinant.map",BookBlockKind::Definition,"1.9.1","The map and its columns",
      {m(R"(A=\begin{bmatrix}1&k&0\\0&s&0\\0&0&1\end{bmatrix},\qquad A\begin{bmatrix}x\\y\\z\end{bmatrix}=\begin{bmatrix}x+ky\\sy\\z\end{bmatrix})"),
       p("The columns are Ae1=(1,0,0), Ae2=(k,s,0), and Ae3=(0,0,1). The grey cage is [0,1]^3, with volume one. The teal cage is the image of that cube; its edges need not meet at right angles.")}},
    {"determinant.volume",BookBlockKind::Definition,"1.9.2","Signed determinant and geometric volume",
      {p("For a real 3 by 3 matrix, the determinant is the oriented volume factor: the signed scalar triple product of its columns. The image of a unit cube has geometric volume equal to the determinant's absolute value. A negative number describes orientation, never a negative geometric volume."),
       m(R"(\det(A)=s,\qquad V=|\det(A)|=|s|)"),
       p("In this upper-triangular family, multiplying the diagonal entries gives 1 times s times 1. The shear k does not appear in the determinant.")}},
    {"determinant.orientation",BookBlockKind::Definition,"1.9.3","Orientation and collapse",
      {p("For an invertible map, a positive determinant preserves orientation and a negative determinant reverses it. At determinant zero, orientation is degenerate. Here s=0 sends every point into y=0."),
       m(R"(\operatorname{rank}(A)=\begin{cases}3,&s\ne0,\\2,&s=0.\end{cases})"),
       p("At s=0, Ae1 and Ae3 remain independent, while Ae2=k Ae1. Thus the map's image is the whole xz-plane. The transformed unit cube itself occupies a bounded region in that plane. A general zero-determinant matrix can instead have rank one or zero; those cases are outside this family.")}},
    {"determinant.shear",BookBlockKind::Example,"1.9.4","Slant without changing volume",
      {p("Compare Unit cube and Shear. With s=1, the transformation shifts each horizontal slice by k times its original height. It changes the angles of the side edges but preserves the slice areas, height, determinant, and volume."),
       m(R"(k=1,\ s=1:\qquad \det(A)=1,\quad V=1)")}, {},
      {{"Definition 1.9.2 / volume scaling","determinant.volume"}}},
    {"determinant.reflect",BookBlockKind::Example,"1.9.5","Stretch and reflection",
      {p("Stretch uses k=0 and s=2, giving determinant 2 and volume 2. Reflection uses k=0.6 and s=-1, giving determinant -1 and volume 1. The second basis arrow now points to the opposite side of the original xz-plane."),
       p("Crossing from positive to negative s passes through zero. Watch the three-dimensional solid flatten before it appears on the other side.")}},
    {"determinant.collapse",BookBlockKind::Example,"1.9.6","Two surviving directions",
      {m(R"(k=0.6,\ s=0:\qquad A(x,y,z)=(x+0.6y,0,z))"),
       p("The vertical coordinate disappears. The remaining x and z directions are still independent, so rank is two. Changing k moves the second column along the first without recovering a third direction.")}, {},
      {{"Definition 1.9.3 / rank at collapse","determinant.orientation"}}},
    {"determinant.figure",BookBlockKind::Figure,"1.9.7","Change one cause at a time",
      {p("Keep s=1 and move k: the shape changes but the volume stays one. Then keep k fixed and move s through positive, zero, and negative values. Read the determinant, volume, and rank together."),
       p("The inputs snap to steps of 0.1. The smallest supported nonzero scale has magnitude 0.1, so the displayed rank is well separated from the model's numerical zero threshold. The finite cage represents the image of a unit cube, not all of the map's image space.")}},
    {"determinant.practice.volume",BookBlockKind::Exercise,"1.9.8","Predict a positive volume",
      {p("For k=1 and s=2, give the determinant and geometric volume before moving the controls.")},
      help(p("Use the diagonal entries; the shear is off the diagonal."),p("Determinant 2; geometric volume 2."),p("The upper-triangular diagonal product is 1 times 2 times 1 = 2. Taking its absolute value gives volume 2."))},
    {"determinant.practice.orientation",BookBlockKind::Exercise,"1.9.9","Separate sign from size",
      {p("For k=-1 and s=-0.5, state the orientation and geometric volume.")},
      help(p("The determinant's sign and absolute value answer different questions."),p("Orientation is reversed; geometric volume is 0.5."),p("The determinant is -0.5. Its negative sign indicates reversal, and |-0.5|=0.5 gives the nonnegative volume."))},
    {"determinant.practice.rank",BookBlockKind::Exercise,"1.9.10","Explain the plane",
      {p("Why does every allowed k at s=0 give rank two, rather than rank one or zero? Then open Exercise and make your separate practice image collapse.")},
      help(p("Inspect which two columns remain independent."),p("Ae1 and Ae3 remain independent, and Ae2 is a multiple of Ae1."),p("The columns (1,0,0) and (0,0,1) span the xz-plane. The remaining column (k,0,0) adds no independent direction, so the rank is exactly two."))},
    {"determinant.summary",BookBlockKind::Summary,"","What to carry forward",
      {p("Columns describe the images of basis vectors. The determinant records oriented volume scaling; its absolute value gives geometric volume. A shear can change the shape without changing volume. In this family, zero vertical scale removes one independent direction."),
       p("The written checks are ungraded. Exercise uses an independent manipulation example and records feedback only when you press Check. Exploration and reading do not complete it, and this practice state lasts for the current session.")}}
  };
  return blocks;
}
BookSection determinantSection(){
  return {"matrix.determinant-volume","1.9 Determinants as signed volume",
    "Predict how shear and vertical scaling change volume, orientation, and the image of a unit cube.",{},
    {{"Determinant","The oriented volume scaling factor of a real square linear map.","determinant.volume"},
     {"Geometric volume","The nonnegative volume of the image of the unit cube, equal to the absolute determinant.","determinant.volume"},
     {"Orientation","The handedness preserved or reversed by an invertible real linear map.","determinant.orientation"},
     {"Shear","A linear map that shifts one coordinate in proportion to another, slanting a shape.","determinant.map"}},
    "","",{},"Make a separate practice image collapse into a plane, then check the result.",
    "Separately authored examples for the existing linear cube model. All matrices belong to the stated two-parameter family; no source-card results are recorded.",
    0,determinantLesson(),{BookFigureKind::Object,"determinant.cube","The image of a unit cube",
      "Figure 1.9.7. Move k to shear the unit cube and s to stretch, reflect, or collapse its image. The arrows show the transformed basis vectors."},
    BookExerciseKind::Object,&determinantVolumeSpec(),3};
}
} // namespace paths
