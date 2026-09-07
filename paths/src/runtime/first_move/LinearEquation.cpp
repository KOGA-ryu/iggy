#include "runtime/first_move/LinearEquation.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <numeric>

namespace iggy3d::first_move {
namespace {
constexpr std::int64_t limit=1'000'000'000;
struct MathError { std::string_view message; };
ExactNumber number(std::int64_t n,std::int64_t d=1) {
  if(!d)throw MathError{"Division by zero is undefined."};
  if(d<0) {n=-n;d=-d;}
  const auto gcd=std::gcd(n,d);n/=gcd;d/=gcd;
  if(n < -limit || n > limit || d > limit)
    throw MathError{"This value exceeds the exact-number limit (1,000,000,000)."};
  return {n,d};
}
ExactNumber add(ExactNumber a,ExactNumber b) {return number(a.numerator*b.denominator+b.numerator*a.denominator,a.denominator*b.denominator);}
ExactNumber neg(ExactNumber a) {return {-a.numerator,a.denominator};}
ExactNumber mul(ExactNumber a,ExactNumber b) {return number(a.numerator*b.numerator,a.denominator*b.denominator);}
ExactNumber divide(ExactNumber a,ExactNumber b) {return number(a.numerator*b.denominator,a.denominator*b.numerator);}
std::string text(ExactNumber n) {
  return std::to_string(n.numerator)+(n.denominator==1?"":"/"+std::to_string(n.denominator));
}
LinearExpression sum(LinearExpression a,LinearExpression b) {return {add(a.coefficient,b.coefficient),add(a.constant,b.constant)};}
LinearExpression scale(LinearExpression a,ExactNumber n) {return {mul(a.coefficient,n),mul(a.constant,n)};}
enum class Form { Number, Variable, Expression };
struct Parsed {
  LinearExpression value;
  Form form=Form::Expression;
  bool containsVariable=false, expandable=false, sum=false;
};
class Parser {
public:
  explicit Parser(std::string_view input):input_(input) {
    if(input.empty() || input.size()>160)throw MathError{"Enter an equation of at most 160 characters."};
    for(unsigned char c:input)if(c<32 || c>126)throw MathError{"Use x, numbers, parentheses and + - * / =."};
  }
  LinearEquation equation() {
    auto left=expression(0);
    if(!take('='))throw MathError{"Include one equals sign between the two sides."};
    auto right=expression(0);
    finish();
    std::string display;
    for(char c:input_)if(c!=' ')display+=c;
    display.insert(display.find('=')," ");display.insert(display.find('=')+1," ");
    return {left.value,right.value,std::move(display),left.expandable || right.expandable,
        left.form==Form::Variable && right.form==Form::Number};
  }
  ExactNumber operand() {
    auto result=expression(0);finish();
    if(result.containsVariable)throw MathError{"Use a number for the operation, such as -2 or 1/3."};
    return result.value.constant;
  }
private:
  void spaces() {while(pos_<input_.size() && input_[pos_]==' ')++pos_;}
  char peek() {spaces();return pos_==input_.size()?'\0':input_[pos_];}
  bool take(char c) {if(peek()!=c)return false;++pos_;return true;}
  void finish() {if(peek())throw MathError{"Use a linear expression in x with + - * / and parentheses."};}
  Parsed expression(unsigned depth) {
    auto result=product(depth);
    while(peek()=='+' || peek()=='-') {
      const bool minus=take('-');if(!minus)take('+');
      auto right=product(depth);
      result.value=sum(result.value,minus?scale(right.value,{-1,1}):right.value);
      result.form=Form::Expression;result.containsVariable|=right.containsVariable;
      result.expandable|=right.expandable;result.sum=true;
    }
    return result;
  }
  Parsed product(unsigned depth) {
    auto result=atom(depth);
    while(true) {
      const char next=peek();
      const bool dividing=next=='/';
      const bool explicitMultiply=next=='*';
      // Only conventional number/bracket/variable adjacency; separated
      // numeric literals ("1 2") are not silently interpreted as a product.
      const bool implicit=next=='(' || next=='x';
      if(!dividing && !explicitMultiply && !implicit)break;
      if(!implicit)++pos_;
      auto right=atom(depth);
      const auto left=result;
      if(dividing) {
        if(right.containsVariable)throw MathError{"Dividing by an expression containing x is outside this exercise."};
        result.value=scale(left.value,divide({1,1},right.value.constant));
      } else {
        if(left.containsVariable && right.containsVariable)
          throw MathError{"Products containing x on both sides are outside linear equations."};
        result.value=left.containsVariable?scale(left.value,right.value.constant):scale(right.value,left.value.constant);
      }
      result.form=dividing && left.form==Form::Number && right.form==Form::Number?Form::Number:Form::Expression;
      result.expandable=left.expandable || right.expandable || (!dividing && (left.sum || right.sum));
      result.containsVariable=left.containsVariable || right.containsVariable;result.sum=false;
    }
    return result;
  }
  Parsed atom(unsigned depth) {
    if(depth>16 || ++nodes_>128)throw MathError{"This expression is too deeply nested or too long."};
    if(take('+'))return atom(depth+1);
    if(take('-')) {
      auto result=atom(depth+1);result.value=scale(result.value,{-1,1});
      result.expandable|=result.sum;
      if(result.form==Form::Variable)result.form=Form::Expression;
      return result;
    }
    if(take('(')) {
      auto result=expression(depth+1);
      if(!take(')'))throw MathError{"Close each opening parenthesis."};
      return result;
    }
    if(take('x'))return {{{1,1},{0,1}},Form::Variable,true};
    std::int64_t n=0,d=1;unsigned digits=0,places=0;bool decimal=false;
    while(true) {
      const char c=pos_==input_.size()?'\0':input_[pos_];
      if(c=='.' && !decimal) {decimal=true;++pos_;continue;}
      if(c<'0' || c>'9')break;
      if(n>(limit-(c-'0'))/10 || (decimal && ++places>6))
        throw MathError{"Use smaller numbers or a fraction; decimals allow six places."};
      n=n*10+c-'0';++digits;++pos_;if(decimal)d*=10;
    }
    if(!digits)throw MathError{"Enter a number, x, or a parenthesized expression."};
    return {{{0,1},number(n,d)},Form::Number};
  }
  std::string_view input_;std::size_t pos_=0;unsigned nodes_=0;
};
struct MathOperationDescriptor { MathOperation operation;std::string_view label;bool needsNumber; };
constexpr std::array operations{
  MathOperationDescriptor{MathOperation::Expand,"a(b+c)",false},
  MathOperationDescriptor{MathOperation::Simplify,"=",false},
  MathOperationDescriptor{MathOperation::Add,"+",true},
  MathOperationDescriptor{MathOperation::Subtract,"-",true},
  MathOperationDescriptor{MathOperation::Multiply,"×",true},
  MathOperationDescriptor{MathOperation::Divide,"÷",true},
};
struct MathTransformation {
  LinearEquation expected;
  std::string operation, explanation;
};
MathTransformation transform(const LinearEquation& before,MathOperation operation,std::string_view operand) {
  const auto descriptor=std::find_if(operations.begin(),operations.end(),[&](const auto& d){return d.operation==operation;});
  if(descriptor==operations.end())throw MathError{"Choose an available operation."};
  const ExactNumber value=descriptor->needsNumber?Parser(operand).operand():ExactNumber{};
  auto expected=before;
  std::string label,explanation;
  switch(operation) {
    case MathOperation::Expand:
      if(!before.expandable)throw MathError{"There is no multiplied bracket to expand."};
      label="Expand brackets";explanation="Distribute the factor over every term in the bracket.";break;
    case MathOperation::Simplify:
      label="Simplify";explanation="Rewriting each side with the same value preserves the equation.";break;
    case MathOperation::Add:case MathOperation::Subtract: {
      if(!value.numerator)throw MathError{"Adding or subtracting zero does not change the working."};
      const auto amount=operation==MathOperation::Add?value:neg(value);
      expected.left.constant=add(before.left.constant,amount);expected.right.constant=add(before.right.constant,amount);
      label=std::string(descriptor->label)+" ("+text(value)+")";
      explanation="Adding or subtracting the same number on both sides preserves equality.";break;
    }
    case MathOperation::Multiply:case MathOperation::Divide: {
      if(!value.numerator)throw MathError{"Use a nonzero number to preserve the solutions."};
      if(value==ExactNumber{1,1})throw MathError{"Multiplying or dividing by one does not change the working."};
      const auto factor=operation==MathOperation::Multiply?value:divide({1,1},value);
      expected.left=scale(before.left,factor);expected.right=scale(before.right,factor);
      label=std::string(descriptor->label)+" ("+text(value)+")";
      explanation="Multiplying or dividing both sides by the same nonzero number preserves the solutions.";break;
    }
    default:throw MathError{"Choose a linear-equation operation."};
  }
  return {std::move(expected),std::move(label),std::move(explanation)};
}
std::string expressionText(LinearExpression value) {
  std::string result;
  if(value.coefficient.numerator) {
    result=value.coefficient==ExactNumber{1,1}?"x":value.coefficient==ExactNumber{-1,1}?"-x":text(value.coefficient)+"x";
  }
  if(value.constant.numerator || result.empty()) {
    if(!result.empty() && value.constant.numerator>0)result+='+';
    result+=text(value.constant);
  }
  return result;
}
std::string equationText(const LinearEquation& equation) {
  return expressionText(equation.left)+" = "+expressionText(equation.right);
}
void shuffleResults(MathMoveChoice& choice,std::string_view before) {
  std::uint32_t order=2166136261U;
  for(unsigned char c:std::string(before)+choice.label)order=(order^c)*16777619U;
  for(std::size_t i=choice.results.size()-1;i>0;--i) {
    order=order*1664525U+1013904223U;
    std::swap(choice.results[i],choice.results[order%(i+1)]);
  }
}
}
LinearEquationResult parseLinearEquation(std::string_view input) {
  try {return {Parser(input).equation(),{}};}
  catch(const MathError& error) {return {std::nullopt,error.message};}
}
MathMoveCheck checkMathMove(const LinearEquation& before,MathOperation operation,
                           std::string_view operand,std::string_view entry) {
  try {
    auto transformed=transform(before,operation,operand);
    const auto& expected=transformed.expected;
    auto after=Parser(entry).equation();
    const bool left=after.left==expected.left,right=after.right==expected.right;
    if(!left && !right)throw MathError{"Check both sides against the operation you selected."};
    if(!left)throw MathError{"Check the left side: it does not match the selected operation."};
    if(!right)throw MathError{"Check the right side: apply the same operation and check the arithmetic."};
    if(operation==MathOperation::Expand && after.expandable)throw MathError{"Distribute over every multiplied bracket."};
    if(after.display==before.display)throw MathError{"Enter a changed equation for this move."};
    return {std::move(after),std::move(transformed.operation),std::move(transformed.explanation),"Step checked."};
  } catch(const MathError& error) {return {{},{},{},error.message};}
}
std::vector<MathMoveChoice> availableMathMoves(const LinearEquation& before) {
  std::vector<MathMoveChoice> choices;
  choices.reserve(kMathMoveChoiceCapacity);
  const auto offer=[&](MathOperation operation,ExactNumber value=ExactNumber{}) {
    try {
      const auto& descriptor=operations[static_cast<std::size_t>(operation)];
      const auto operand=descriptor.needsNumber?text(value):std::string{};
      const auto transformed=transform(before,operation,operand);
      const auto checked=checkMathMove(before,operation,operand,equationText(transformed.expected));
      if(!checked.result)return;
      const auto& after=*checked.result;
      std::string label(descriptor.label);
      if(descriptor.needsNumber)label+=" "+(value.numerator<0?"("+operand+")":operand);
      MathMoveChoice choice{operation,operand,std::move(label),{after.display}};
      std::size_t count=1;
      const auto wrong=[&](LinearEquation candidate) {
        if(count==choice.results.size())return;
        const auto entry=equationText(candidate);
        if(!parseLinearEquation(entry).equation || checkMathMove(before,operation,operand,entry).result ||
            std::find(choice.results.begin(),choice.results.begin()+count,entry)!=choice.results.begin()+count)return;
        choice.results[count++]=entry;
      };
      // Expansion choices include missing distribution on either term.
      auto candidate=after;
      if(operation==MathOperation::Expand && after.left.coefficient.numerator) {
        try {candidate.left.constant=divide(after.left.constant,after.left.coefficient);wrong(candidate);}
        catch(const MathError&) {}
        candidate=after;candidate.left.coefficient={1,1};wrong(candidate);
      }
      // Common one-sided mistakes, followed by nearby arithmetic errors.
      candidate=after;candidate.left=before.left;wrong(candidate);
      candidate=after;candidate.right=before.right;wrong(candidate);
      for(const auto delta:{1,-1,2,-2}) {
        try {candidate=after;candidate.right.constant=add(after.right.constant,{delta,1});wrong(candidate);}
        catch(const MathError&) {} // A boundary value must not remove a valid move.
      }
      for(const auto value:{0,1,-1,2,-2}) {
        candidate=after;candidate.right.constant={value,1};wrong(candidate);
      }
      if(count!=choice.results.size())return;
      shuffleResults(choice,before.display);
      choices.push_back(std::move(choice));
    } catch(const MathError&) {} // Offer only operations within the exact-number bounds.
  };
  if(before.expandable)offer(MathOperation::Expand);
  else if(before.display!=equationText(before))offer(MathOperation::Simplify);
  if(before.left.coefficient.numerator && before.left.coefficient!=ExactNumber{1,1}) {
    offer(MathOperation::Divide,before.left.coefficient);
    offer(MathOperation::Multiply,divide({1,1},before.left.coefficient));
  }
  if(before.left.constant.numerator) {
    const bool positive=before.left.constant.numerator>0;
    const auto magnitude=positive?before.left.constant:neg(before.left.constant);
    offer(positive?MathOperation::Subtract:MathOperation::Add,magnitude);
    offer(positive?MathOperation::Add:MathOperation::Subtract,magnitude);
  }
  return choices;
}
std::string verifyMathSolution(const LinearEquation& original,const LinearEquation& result) {
  if(!result.isolated)return {};
  try {
    const auto x=result.right.constant;
    const auto left=add(mul(original.left.coefficient,x),original.left.constant);
    const auto right=add(mul(original.right.coefficient,x),original.right.constant);
    if(left!=right)return {};
    return "x = "+text(x)+"; original equation: "+text(left)+" = "+text(right)+". Checked.";
  } catch(const MathError&) {return {};}
}
namespace {
std::string matrixText(const AugmentedMatrix& matrix) {
  std::array<std::array<std::string,3>,2> cells;
  std::array<std::size_t,3> widths{};
  for(std::size_t row=0;row<2;++row)for(std::size_t col=0;col<3;++col) {
    cells[row][col]=text(matrix.rows[row][col]);widths[col]=std::max(widths[col],cells[row][col].size());
  }
  std::string result;
  for(std::size_t row=0;row<2;++row) {
    if(row)result+='\n';result+='[';
    for(std::size_t col=0;col<3;++col) {
      if(col)result+=col==2?" | ":", ";
      result+=std::string(widths[col]-cells[row][col].size(),' ')+cells[row][col];
    }
    result+=']';
  }
  return result;
}
AugmentedMatrix readMatrix(std::string_view input) {
  if(input.empty() || input.size()>160)throw MathError{"Use a two-row matrix of at most 160 characters."};
  std::size_t position=0;
  const auto token=[&](char expected) {
    while(position<input.size() && (input[position]==' ' || input[position]=='\n'))++position;
    if(position==input.size() || input[position++]!=expected)throw MathError{"Use two rows: [a, b | c] followed by [d, e | f]."};
  };
  AugmentedMatrix matrix;
  for(auto& row:matrix.rows) {
    token('[');
    for(std::size_t col=0;col<3;++col) {
      const char delimiter=col==0?',':col==1?'|':']';
      const auto end=input.find(delimiter,position);
      if(end==std::string_view::npos)throw MathError{"Each row needs two coefficients and one right-hand value."};
      row[col]=Parser(input.substr(position,end-position)).operand();position=end;token(delimiter);
    }
  }
  while(position<input.size() && (input[position]==' ' || input[position]=='\n'))++position;
  if(position!=input.size())throw MathError{"This exercise uses exactly two rows and three columns."};
  matrix.display=matrixText(matrix);return matrix;
}
bool identityMatrix(const AugmentedMatrix& matrix) {
  return matrix.rows[0][0]==ExactNumber{1,1} && matrix.rows[0][1]==ExactNumber{} &&
      matrix.rows[1][0]==ExactNumber{} && matrix.rows[1][1]==ExactNumber{1,1};
}
enum class RowMove { Swap, Divide, Add };
struct RowOperation { MathOperation operation;RowMove kind;std::size_t target,source; };
constexpr std::array rowOperations{
  RowOperation{MathOperation::SwapRows,RowMove::Swap,0,1},
  RowOperation{MathOperation::DivideRow1,RowMove::Divide,0,0},
  RowOperation{MathOperation::DivideRow2,RowMove::Divide,1,1},
  RowOperation{MathOperation::AddRow1ToRow2,RowMove::Add,1,0},
  RowOperation{MathOperation::AddRow2ToRow1,RowMove::Add,0,1}
};
CheckedMathMove<AugmentedMatrix> transformMatrix(const AugmentedMatrix& before,MathOperation operation,std::string_view operand) {
  const auto found=std::find_if(rowOperations.begin(),rowOperations.end(),[&](const auto& move){return move.operation==operation;});
  if(found==rowOperations.end())throw MathError{"Choose a row operation for this matrix."};
  auto after=before;std::string label,reason;
  const auto target="R"+std::to_string(found->target+1),source="R"+std::to_string(found->source+1);
  switch(found->kind) {
    case RowMove::Swap:
      std::swap(after.rows[0],after.rows[1]);label="R1 <-> R2";
      reason="Swapping whole equations preserves their common solutions.";break;
    case RowMove::Divide: {
      const auto value=Parser(operand).operand();
      if(!value.numerator)throw MathError{"A row divisor must be nonzero."};
      for(auto& cell:after.rows[found->target])cell=divide(cell,value);
      label=target+" ÷ "+(value.numerator<0?"("+text(value)+")":text(value));
      reason="Divide every value in "+target+", including its right-hand value, by the same nonzero number.";break;
    }
    case RowMove::Add: {
      const auto value=Parser(operand).operand();
      for(std::size_t col=0;col<3;++col)after.rows[found->target][col]=add(before.rows[found->target][col],mul(value,before.rows[found->source][col]));
      const auto magnitude=value.numerator<0?neg(value):value;
      label=target+(value.numerator<0?" - ":" + ")+(magnitude==ExactNumber{1,1}?"":magnitude.denominator==1?text(magnitude):"("+text(magnitude)+")")+source;
      reason="Replace "+target+" by "+label+" across all three columns. "+source+" stays unchanged.";break;
    }
  }
  if(after.rows==before.rows)throw MathError{"Choose a row operation that changes the matrix."};
  after.display=matrixText(after);return {std::move(after),std::move(label),std::move(reason),"Step checked."};
}
}
CheckedMathMove<AugmentedMatrix> parseAugmentedMatrix(std::string_view input) {
  try {return {readMatrix(input),{},{},{}};}
  catch(const MathError& error) {return {{},{},{},error.message};}
}
CheckedMathMove<MathWorkingValue> prepareMathWorking(MathWorkingModel model,std::string_view input) {
  try {
    switch(model) {
      case MathWorkingModel::LinearEquation: {
        auto parsed=Parser(input).equation();
        if(!parsed.left.coefficient.numerator || parsed.right.coefficient.numerator || parsed.isolated)
          throw MathError{"Use an unsolved linear equation with x on the left."};
        return {std::move(parsed),{},{},{}};
      }
      case MathWorkingModel::RowReduction: {
        auto matrix=readMatrix(input);const auto& a=matrix.rows;
        if(!add(mul(a[0][0],a[1][1]),neg(mul(a[0][1],a[1][0]))).numerator || identityMatrix(matrix))
          throw MathError{"Use an unsolved two-variable system with one solution."};
        return {std::move(matrix),{},{},{}};
      }
    }
    throw MathError{"Unknown mathematical working model."};
  } catch(const MathError& error) {return {{},{},{},error.message};}
}
CheckedMathMove<AugmentedMatrix> checkMathMove(const AugmentedMatrix& before,MathOperation operation,
                                              std::string_view operand,std::string_view entry) {
  try {
    auto expected=transformMatrix(before,operation,operand);auto after=readMatrix(entry);
    if(after.rows!=expected.result->rows)throw MathError{"Check both rows against the operation, including the right-hand values."};
    expected.result=std::move(after);return expected;
  } catch(const MathError& error) {return {{},{},{},error.message};}
}
std::vector<MathMoveChoice> availableMathMoves(const AugmentedMatrix& before) {
  std::vector<MathMoveChoice> choices;choices.reserve(kMathMoveChoiceCapacity);
  const auto offer=[&](MathOperation operation,ExactNumber value=ExactNumber{}) {
    try {
      const auto operand=operation==MathOperation::SwapRows?std::string{}:text(value);
      const auto transformed=transformMatrix(before,operation,operand);
      const auto& after=*transformed.result;
      if(after.display.size()>160)return;
      MathMoveChoice choice{operation,operand,transformed.operation,{after.display}};std::size_t count=1;
      const auto wrong=[&](AugmentedMatrix candidate) {
        if(count==choice.results.size())return;
        const auto entry=matrixText(candidate);
        if(!parseAugmentedMatrix(entry).result || checkMathMove(before,operation,operand,entry).result ||
            std::find(choice.results.begin(),choice.results.begin()+count,entry)!=choice.results.begin()+count)return;
        choice.results[count++]=entry;
      };
      // Leave a changed cell behind: common missed coefficient or right-hand work.
      for(std::size_t row=0;row<2;++row)for(std::size_t col=0;col<3;++col) {
        auto candidate=after;candidate.rows[row][col]=before.rows[row][col];wrong(candidate);
      }
      for(int delta:{1,-1})for(std::size_t row=0;row<2;++row)for(std::size_t col=0;col<3;++col) {
        try {auto candidate=after;candidate.rows[row][col]=add(candidate.rows[row][col],{delta,1});wrong(candidate);}
        catch(const MathError&) {}
      }
      if(count!=choice.results.size())return;
      shuffleResults(choice,before.display);choices.push_back(std::move(choice));
    } catch(const MathError&) {} // A failed candidate cannot alter working or remove other legal choices.
  };
  offer(MathOperation::SwapRows);
  for(std::size_t row=0;row<2;++row) {
    const std::size_t col=before.rows[row][0].numerator?0:1;
    const auto pivot=before.rows[row][col];
    if(!pivot.numerator)continue;
    if(pivot!=ExactNumber{1,1})offer(row==0?MathOperation::DivideRow1:MathOperation::DivideRow2,pivot);
    try {
      const auto factor=neg(divide(before.rows[1-row][col],pivot));
      if(factor.numerator)offer(row==0?MathOperation::AddRow1ToRow2:MathOperation::AddRow2ToRow1,factor);
    } catch(const MathError&) {}
  }
  return choices;
}
std::string verifyMathSolution(const AugmentedMatrix& original,const AugmentedMatrix& result) {
  if(!identityMatrix(result))return {};
  try {
    const auto x=result.rows[0][2],y=result.rows[1][2];
    for(const auto& row:original.rows)if(add(mul(row[0],x),mul(row[1],y))!=row[2])return {};
    return "x = "+text(x)+", y = "+text(y)+"; original rows: "+text(original.rows[0][2])+" = "+text(original.rows[0][2])+", "+text(original.rows[1][2])+" = "+text(original.rows[1][2])+". Checked.";
  } catch(const MathError&) {return {};}
}
} // namespace iggy3d::first_move
