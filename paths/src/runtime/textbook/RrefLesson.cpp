#include "Textbook.hpp"
#include <utility>

namespace paths {
namespace {
BookPassage prose(const char* text){return {BookPassage::Kind::Prose,text};}
BookPassage math(const char* text,const char* number=""){return {BookPassage::Kind::DisplayMath,text,number};}
using Passages=std::vector<BookPassage>;
std::array<Passages,4> proof(Passages text){return {std::move(text),{},{},{}};}
std::array<Passages,4> solution(Passages text){return {Passages{},{},{},std::move(text)};}
std::array<Passages,4> practice(Passages hint,Passages answer,Passages working){return {Passages{},std::move(hint),std::move(answer),std::move(working)};}
}
const std::vector<BookBlock>& rrefLesson(){
  // Stable IDs carry references and disclosure state; printed numbers are labels.
  // These examples are authored here, independently of the six printed card parts.
  static const std::vector<BookBlock> blocks{
    {"rref.start",BookBlockKind::Introduction,"","Before you begin",
      {prose("Prerequisite: Section 1.1, Reading a matrix. You should be able to name an entry, state a matrix's dimensions, and recognize an explicitly augmented system."),
       prose("Our question is how to simplify a matrix without losing information. First we identify reversible moves, then describe the shape we want, and finally explain why these moves are valid. Work over the real or complex numbers; the same definitions apply to both.")}},
    {"rref.operations",BookBlockKind::Definition,"1.2.1","Elementary row operations",
      {prose("An elementary row operation is one of the following three moves. Apply it to every entry of the affected row, including an explicitly attached right-hand side."),
       prose("Swap two distinct rows:"),math(R"(R_i \leftrightarrow R_j,\quad i\ne j)"),
       prose("Multiply one row by a nonzero scalar:"),math(R"(R_i \leftarrow cR_i,\quad c\ne 0)"),
       prose("Add a multiple of a different row to a row:"),math(R"(R_i \leftarrow R_i+cR_j,\quad i\ne j)"),
       prose("The row on the left is replaced. In the third move, the other row is left as it was. The scalar in a row addition may be zero; that move simply changes nothing.")}},
    {"rref.equivalence",BookBlockKind::Definition,"1.2.2","Row equivalence",
      {prose("Two matrices of the same dimensions are row equivalent if a finite sequence of elementary row operations transforms one into the other. We write:"),
       math(R"(A\sim B)"),
       prose("The matrices need not have equal entries. Row equivalence describes a relationship created by reversible operations.")}, {},
      {{"Definition 1.2.1 / allowed operations","rref.operations"}}},
    {"rref.echelon",BookBlockKind::Definition,"1.2.3","Row echelon form (REF)",
      {prose("A matrix is in row echelon form when all zero rows are below the nonzero rows, and the first nonzero entry of each nonzero row is strictly to the right of the first nonzero entry in the row above it."),
       prose("That first nonzero entry is the row's leading entry, or pivot. All entries below a pivot are therefore zero. A leading entry in REF need not equal 1. The zero matrix also satisfies these conditions.")}},
    {"rref.reduced",BookBlockKind::Definition,"1.2.4","Reduced row echelon form (RREF)",
      {prose("A matrix is in reduced row echelon form if it is in REF and satisfies two further conditions: each pivot equals 1, and every other entry in each pivot column equals zero."),
       prose("Check the whole pivot column, including entries above the pivot. Columns without pivots may contain nonzero entries. A rectangular matrix need not reduce to an identity matrix, and a column without an available pivot is skipped.")}, {},
      {{"Definition 1.2.3 / echelon conditions","rref.echelon"}}},
    {"rref.preservation",BookBlockKind::Proposition,"1.2.5","Row operations preserve a system's solutions",
      {prose("If an elementary row operation is applied to the entire augmented matrix of a linear system, the resulting system has exactly the same solution set. Consequently, a finite sequence of these operations preserves that set.")},
      proof({prose("A row swap only reorders equations, so a solution still satisfies all of them. The inverse is the same swap."),
        prose("Multiplying an equation by a nonzero scalar preserves its truth. Multiplying the new equation by the reciprocal recovers the old one."),
        math(R"(R_i\leftarrow cR_i\qquad\text{inverse: }R_i\leftarrow \frac{1}{c}R_i)"),
        prose("For row addition, any solution of the old equations satisfies their stated linear combination, so it solves the new system. Subtracting the same multiple of the unchanged other equation reverses the operation; hence every new solution is also an old solution."),
        math(R"(R_i\leftarrow R_i+cR_j\qquad\text{inverse: }R_i\leftarrow R_i-cR_j)"),
        prose("Each move preserves solutions in both directions. Apply this argument successively to every move in a finite sequence. This proves the claim.")}),
      {{"Definition 1.2.1 / nonzero scaling and distinct rows","rref.operations"}}},
    {"rref.example",BookBlockKind::Example,"1.2.6","Reduce a two by two matrix",
      {prose("Find the RREF of the following matrix. Before opening the solution, choose an operation that clears the entry below the first pivot."),
       math(R"(A=\begin{bmatrix}1&2\\3&7\end{bmatrix})")},
      solution({prose("1. The first pivot is already 1. Subtract three times the first row from the second row:"),
        math(R"(R_2\leftarrow R_2-3R_1)"),
        math(R"(A\sim\begin{bmatrix}1&2\\0&1\end{bmatrix})"),
        prose("This is REF. It is not yet RREF, because the second pivot has a nonzero entry above it."),
        prose("2. Subtract twice the second row from the first row:"),
        math(R"(R_1\leftarrow R_1-2R_2)"),
        math(R"(A\sim\begin{bmatrix}1&0\\0&1\end{bmatrix}=I_2)","1.2.1"),
        prose("In Equation (1.2.1), the pivots move right as we move down, both pivots equal 1, and each is the only nonzero entry in its column. These are precisely the RREF conditions. This separately authored example does not operate on your exercise board.")}),
      {{"Definition 1.2.4 / check the result","rref.reduced"}}},
    {"rref.figure",BookBlockKind::Figure,"1.2.7","A matrix changing under elimination",
      {prose("Predict: identify the next usable pivot and the entries its elimination should clear. Operate: perform one guided pivot, or open the full exercise to choose a manual row operation. Explain: identify which REF or RREF conditions now hold."),
       prose("The board begins with part (a) of card 004. The full exercise selects the other printed parts and provides Undo. A guided pivot may combine a swap, normalization, and several row additions. All columns of these printed matrices belong to the matrix; none is an attached right-hand side.")}, {},
      {{"Definition 1.2.4 / what to inspect","rref.reduced"}}},
    {"rref.practice.recognize",BookBlockKind::Exercise,"1.2.8","Recognize the form",
      {prose("Is this matrix in REF? Is it in RREF? Give a reason for each answer."),
       math(R"(B=\begin{bmatrix}1&2&0\\0&1&3\\0&0&0\end{bmatrix})")},
      practice({prose("Locate the leading entries first. Then check the entry above the second pivot.")},
        {prose("It is in REF, but it is not in RREF.")},
        {prose("The zero row is at the bottom, and the leading entries occur in columns 1 and 2, in that order, so B is in REF. Both leading entries equal 1. However, the 2 above the second pivot is nonzero. That violates the requirement that a pivot be the only nonzero entry in its column.")}),
      {{"Definition 1.2.3 / REF","rref.echelon"},{"Definition 1.2.4 / RREF","rref.reduced"}}},
    {"rref.practice.reduce",BookBlockKind::Exercise,"1.2.9","Reduce a rectangular matrix",
      {prose("Find the RREF of C. Write the row operation beside each step, then check every condition of Definition 1.2.4."),
       math(R"(C=\begin{bmatrix}1&2&3\\2&5&8\end{bmatrix})")},
      practice({prose("Use the leading 1 to eliminate the 2 below it. Then clear the entry above the second pivot.")},
        {math(R"(\begin{bmatrix}1&0&-1\\0&1&2\end{bmatrix})")},
        {prose("Subtract twice the first row from the second row:"),
         math(R"(R_2\leftarrow R_2-2R_1)"),
         math(R"(C\sim\begin{bmatrix}1&2&3\\0&1&2\end{bmatrix})"),
         prose("Now subtract twice the second row from the first row:"),
         math(R"(R_1\leftarrow R_1-2R_2)"),
         math(R"(C\sim\begin{bmatrix}1&0&-1\\0&1&2\end{bmatrix})"),
         prose("The pivots are leading 1s in columns 1 and 2, and both pivot columns have zeros elsewhere. The third column is not a pivot column; its nonzero entries are permitted.")}),
      {{"Example 1.2.6 / the same two-stage idea","rref.example"}}},
    {"rref.practice.explain",BookBlockKind::Exercise,"1.2.10","Why exclude multiplication by zero?",
      {prose("Explain why multiplying a row by zero is not an elementary row operation. Give a one-equation system whose solution set changes under that move.")},
      practice({prose("Start with an equation that restricts an unknown. Compare it with the equation obtained by multiplying both sides by zero.")},
        {prose("The move is not reversible and can lose a constraint. For example, x = 1 becomes 0 = 0, allowing every scalar x.")},
        {prose("The original system x = 1 has one solution. Multiplying its entire augmented row by zero produces 0 = 0, which is satisfied by every x. No multiplication can recover the lost equation from that zero row. This is why Definition 1.2.1 requires a nonzero scalar and Proposition 1.2.5 does not apply to multiplication by zero.")}),
      {{"Proposition 1.2.5 / why reversibility matters","rref.preservation"}}},
    {"rref.summary",BookBlockKind::Summary,"","What to carry forward",
      {prose("Choose reversible row operations, check echelon order, normalize each pivot, and clear the rest of each pivot column. Keep any right-hand side attached to every move. A correct final matrix and an explanation of why the moves are valid answer different mathematical questions."),
       prose("The three practice questions above are for written or mental work. Hints, short answers, and full solutions open independently. Reading or revealing them creates no score. Next, use card 004 for the six printed reductions, or continue to Section 1.3 to study systems and partial pivoting.")}}
  };
  return blocks;
}
} // namespace paths
