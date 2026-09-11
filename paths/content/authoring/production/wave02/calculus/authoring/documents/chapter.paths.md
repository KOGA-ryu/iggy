@paths 1
@subject calculus | Calculus
@chapter topic_0060 | Differentiation

@lesson prod02_calc_derivative_tangent_r | From derivative rules to a tangent line
@template lesson.v2
@block introduction | start | 1.1 | A complete calculation
@prose A tangent problem needs a point on the original curve and the slope there. First find the derivative function, then evaluate the original function and its derivative at the specified input. The line must pass both an incidence check and a slope check.
@endblock
@block definition | terms | 1.2 | Functions, factors and slopes
@prose A real function f assigns the output f(x) to input x. The derivative function f prime(x), written f'(x), assigns an instantaneous rate of change to each input where the derivative exists. The fixed input x0 is written x with subscript 0. The number f(x0) is a height, while f'(x0) is a slope; neither is the whole derivative function. A prime means differentiation with respect to x, not multiplication or an exponent.
@prose A factor is an expression being multiplied. In u(x)v(x), u and v are the two factors. In u(x) raised to an integer power n, u is the inner function and the outer function takes a number t to t raised to n. Parentheses and exponents specify the order of operations; adjacent factors are multiplied. In a method identity below, u and v stand for differentiable functions, u' and v' for their derivatives, and n for the positive integer exponent in the current problem. The shorthand f', u' and v' omits the common input x; each still names a derivative function, not a value at x0.
@prose The tangent line is a straight-line function L with the same value and derivative as f at x0. We write its output as y. Its slope is m and its intercept is b, the value at input zero. A horizontal tangent has slope zero, even when the derivative function is not identically zero. An ordered pair written (f(x0), f'(x0)) lists height first and slope second.
@display tangent_definition
m=f'(x_0),\qquad y=L(x)=f(x_0)+m(x-x_0)
@endblock
@block proposition | rule | 1.3 | Choose rules from the structure
@prose A constant differentiates to zero. For a positive integer power of x, multiply by the exponent and reduce it by one. Differentiate sums term by term. For a product, differentiate one factor at a time and add both contributions. For a power of an inner function, also multiply by the derivative of that inner function. When both structures occur, use both rules.
@display derivative_rules
\frac{d}{dx}x^n=nx^{n-1},\qquad (uv)'=u'v+uv',\qquad (u^n)'=nu^{n-1}u'
@display combined_rule
(u^nv)'=nu^{n-1}u'v+u^nv'
@help proof
@prose A separate way to check these polynomial calculations is to expand the original product or power into coefficients first and differentiate each coefficient by its degree. Equality of all resulting coefficients establishes equality for every real x. Agreement at just one input cannot establish equality of derivative functions. The tangent is unique among lines: its slope is fixed at f'(x0), and substituting x0 fixes its intercept at f(x0)-f'(x0)x0.
@endblock
@block proposition | condition | 1.4 | Conditions and checks
@prose The product rule requires both factors to be differentiable at the input. The chain rule requires the inner function to be differentiable there and the outer function to be differentiable at the inner value. Every function in these problems is a polynomial, so these conditions hold at every real input, including zeros of factors. No division by a factor is needed. Multiplying or adding zeros after evaluation is safe; evaluating too early can hide a missing derivative term.
@prose An expanded derivative and a factored derivative can be the same function. Only combine like powers when simplifying. After choosing the tangent, substitute x0 into that line and compare with the original f(x0), then compare the coefficient of x in the line with f'(x0). Passing through the curve alone is not enough, and a correct slope alone is not enough.
@endblock
@block example | worked | 1.5 | Work from the original function
@prose Find the derivative function and the tangent at the specified input.
@display worked_given
f(x)=(x+2)^2(x-1),\qquad x_0=0
@help hint
@prose There are two factors, and the first is a power of an inner linear function. Keep the original value and derivative value in separate calculations before constructing the line.
@help answer
@display worked_answer
f'(x)=3x^2+6x,\qquad f(0)=-4,\qquad f'(0)=0,\qquad y=-4
@help solution
@prose Let u=x+2 and v=x-1. Their derivatives are u'=1 and v'=1. Differentiate the first factor by the chain rule and retain the second factor; then retain the first factor and differentiate the second. The first contribution is 2(x+2)(x-1), and the second is (x+2) squared.
@display worked_derivative
f'(x)=2(x+2)(x-1)+(x+2)^2=2(x^2+x-2)+(x^2+4x+4)=3x^2+6x
@prose At zero the original output is (0+2) squared times (0-1)=4 times (-1)=-4. The derivative value is 3 times 0 squared plus 6 times 0=0. The derivative function is not zero everywhere; only its value at this point is zero.
@display worked_line
y=-4+0(x-0)=-4
@prose Check incidence: L(0)=-4=f(0). Check slope: the constant line has derivative zero, matching f'(0). Both conditions hold, so this is the tangent. Independently, the original expansion is x cubed+3x squared-4; its termwise derivative is 3x squared+6x, confirming the earlier rule calculation.
@endblock
@block example | errors | 1.6 | Diagnose the particular omission
@prose Differentiating both product factors and multiplying their derivatives loses both required product contributions. Differentiating only the outer power loses the inner multiplier. Retaining the old exponent loses the power rule. Swapping height and slope constructs a different line. Using y=mx+f(x0) treats the height at x0 as the intercept at zero, which is only justified when x0 is zero or the resulting correction happens to vanish.
@endblock
@block exercise | practice | 1.7 | Twelve full solves
@prose The first four problems introduce the chain and product calculations. The next four vary signs, input values and a zero slope. The final four combine product and chain rules and require choosing the complete method before using it. Each problem ends with a tangent checked against its original point and derivative value.
@endblock
@block summary | summary | 1.8 | Keep three objects separate
@prose A function, its derivative function and the derivative value at one input are different objects. Choose rules for the original structure, retain all factors, simplify by polynomial arithmetic, evaluate height and slope separately, then check both conditions on the proposed tangent.
@endblock
@practice prod02_calc_derivative_tangent_q01
@practice prod02_calc_derivative_tangent_q02
@practice prod02_calc_derivative_tangent_q03
@practice prod02_calc_derivative_tangent_q04
@practice prod02_calc_derivative_tangent_q05
@practice prod02_calc_derivative_tangent_q06
@practice prod02_calc_derivative_tangent_q07
@practice prod02_calc_derivative_tangent_q08
@practice prod02_calc_derivative_tangent_q09
@practice prod02_calc_derivative_tangent_q10
@practice prod02_calc_derivative_tangent_q11
@practice prod02_calc_derivative_tangent_q12
@end

@question prod02_calc_derivative_tangent_q01 | Tangent to a squared expression
@template choices.v1
@version 1
@goal Find the derivative function, the height and slope at x0, and the tangent line; verify its point and slope.
@given f(x)=(2x+1)^2,\quad x_0=1
@domain All inputs are real. The given polynomial is differentiable for every real x.
@read prod02_calc_derivative_tangent_r
@step 10 | Let u be the inner expression and n the displayed exponent. Which identity differentiates f=u^n?
@choice 11 | f'=nu^{n-1}u'
@choice 12 | f'=nu^{n-1}
@choice 13 | f'=u^nu'
@answer 11
@feedback 12 | This differentiates the outer power but omits the derivative of the inner expression. Here the inner multiplier is not 1.
@feedback 13 | This multiplies the unchanged power by the inner derivative. The outer power must also be differentiated: multiply by its exponent and reduce the exponent by one.
@after f'=nu^{n-1}u'
@wrong Distinguish a power of x from a power of a changing inner expression.
@why This is a composition: the outer function squares its input, while the inner function is 2x+1. Both are polynomials, so the chain rule applies for all real x.
@step 20 | Compute f'(x), retaining the inner derivative.
@choice 22 | f'(x)=2(2x+1)
@choice 21 | f'(x)=4(2x+1)
@choice 23 | f'(x)=4(2x+1)^2
@answer 21
@feedback 22 | The outer derivative contributes 2(2x+1), but the derivative of 2x+1 is another factor of 2.
@feedback 23 | The exponent must drop from 2 to 1. Multiplying by both coefficients does not justify retaining the square.
@after f'(x)=4(2x+1)=8x+4
@wrong Differentiate the inner linear function as well as the outer square.
@why The power contributes 2(2x+1), and the inner derivative is 2. Their product is 4(2x+1); distribution gives 8x+4.
@step 30 | Evaluate the original height first and the derivative slope second.
@choice 32 | (f(1),f'(1))=(12,9)
@choice 33 | (f(1),f'(1))=(9,6)
@choice 31 | (f(1),f'(1))=(9,12)
@answer 31
@feedback 32 | These numbers reverse the roles. The original square supplies height, while the derivative function supplies slope.
@feedback 33 | The slope 6 comes from the missing-inner-factor derivative. Evaluate the complete derivative 8x+4 instead.
@after (f(1),f'(1))=(9,12)
@wrong Keep original evaluation separate from derivative evaluation.
@why The original value is (2 times 1+1) squared=3 squared=9. The derivative value is 8 times 1+4=12. Thus the point is (1,9) and its slope is 12.
@step 40 | Select the tangent through the original point with the computed slope.
@choice 41 | y=12x-3
@choice 42 | y=12x+9
@choice 43 | y=9x
@answer 41
@feedback 42 | This has slope 12 but at x=1 gives 21 rather than 9. The point height is not the intercept at zero.
@feedback 43 | This passes through (1,9) but has slope 9, the height rather than the derivative value 12.
@after y=12x-3
@wrong A tangent must satisfy both point and slope conditions.
@why Point-slope form gives y=9+12(x-1)=12x-3. Incidence check: 12 times 1-3=9, equal to the original value. Slope check: the line's x coefficient is 12, equal to f'(1). Both conditions hold.
@end

@question prod02_calc_derivative_tangent_q02 | Tangent to a polynomial product
@template choices.v1
@version 1
@goal Find the derivative function, the height and slope at x0, and the tangent line; verify its point and slope.
@given f(x)=(x^2+1)(x+2),\quad x_0=1
@domain All inputs are real. The given polynomial is differentiable for every real x.
@read prod02_calc_derivative_tangent_r
@step 10 | Let u and v be the first and second factors. Which identity differentiates f=uv?
@choice 12 | f'=u'v'
@choice 11 | f'=u'v+uv'
@choice 13 | f'=u'+v'
@answer 11
@feedback 12 | Multiplying the two derivatives omits the unchanged factors required in the two product-rule contributions.
@feedback 13 | Adding derivatives differentiates a sum. The original combines the factors by multiplication, not addition.
@after f'=u'v+uv'
@wrong Identify the operation connecting the two factors.
@why The original is a product of two differentiable polynomials. Differentiate one factor at a time, retain the other, and add both contributions.
@step 20 | Compute the derivatives of u=x^2+1 and v=x+2.
@choice 22 | (u',v')=(2x,0)
@choice 23 | (u',v')=(x^2,1)
@choice 21 | (u',v')=(2x,1)
@answer 21
@feedback 22 | The constant 2 has derivative zero, but the x term in v has derivative 1; the whole linear factor is not constant.
@feedback 23 | The derivative of x squared is 2x, not the unchanged x squared. The constant 1 contributes zero.
@after (u',v')=(2x,1)
@wrong Apply the power and constant rules to each factor separately.
@why For u, differentiating x squared gives 2x and differentiating 1 gives zero. For v, differentiating x gives 1 and differentiating 2 gives zero.
@step 30 | Assemble both product-rule contributions to f'(x).
@choice 31 | f'(x)=2x(x+2)+(x^2+1)
@choice 32 | f'(x)=2x
@choice 33 | f'(x)=2x(x+2)
@answer 31
@feedback 32 | This is the product of the factor derivatives. The derivative of the original product requires the two retained-factor terms.
@feedback 33 | This includes the first contribution but omits (x squared+1) times the derivative 1 of the second factor.
@after f'(x)=2x(x+2)+(x^2+1)=3x^2+4x+1
@wrong Keep both terms before combining like powers.
@why The first contribution expands to 2x squared+4x; the second is x squared+1. Adding like powers gives 3x squared+4x+1.
@step 40 | Evaluate (f(1), f'(1)) in that order.
@choice 42 | (f(1),f'(1))=(8,6)
@choice 41 | (f(1),f'(1))=(6,8)
@choice 43 | (f(1),f'(1))=(6,3)
@answer 41
@feedback 42 | The derivative supplies slope 8, not the height. Evaluate the original product for the first entry.
@feedback 43 | The slope is not just the leading coefficient 3. Substitute 1 into every term of the derivative.
@after (f(1),f'(1))=(6,8)
@wrong Evaluate the two different functions, not just their leading terms.
@why The original gives (1 squared+1)(1+2)=2 times 3=6. The derivative gives 3+4+1=8.
@step 50 | Construct the line using the original point and slope.
@choice 52 | y=8x+6
@choice 53 | y=6x
@choice 51 | y=8x-2
@answer 51
@feedback 52 | At x=1 this gives 14, not 6. Subtract the slope times x0 when converting to intercept form.
@feedback 53 | The point is correct at x=1, but the slope 6 is the original height, not f'(1)=8.
@after y=8x-2
@wrong Check incidence and slope independently.
@why The tangent is y=6+8(x-1)=8x-2. Substitution gives L(1)=8-2=6=f(1), and differentiating the line gives L'=8=f'(1). This verifies both required conditions.
@end

@question prod02_calc_derivative_tangent_q03 | Tangent to a cubed expression
@template choices.v1
@version 1
@goal Find the derivative function, the height and slope at x0, and the tangent line; verify its point and slope.
@given f(x)=(-x+2)^3,\quad x_0=0
@domain All inputs are real. The given polynomial is differentiable for every real x.
@read prod02_calc_derivative_tangent_r
@step 10 | Let u be the inner expression and n the displayed exponent. Which identity differentiates f=u^n?
@choice 12 | f'=nu^{n-1}
@choice 13 | f'=u^nu'
@choice 11 | f'=nu^{n-1}u'
@answer 11
@feedback 12 | The outer power derivative alone misses the inner derivative; here that omitted multiplier carries a negative sign.
@feedback 13 | The outer cube cannot remain unchanged. Differentiate its power as well as the inner expression.
@after f'=nu^{n-1}u'
@wrong Account for the changing inner input to the cube.
@why The cube is composed with -x+2. Both functions are differentiable everywhere, so use the chain rule and retain the inner derivative.
@step 20 | Compute the derivative function before evaluating at zero.
@choice 21 | f'(x)=-3(-x+2)^2
@choice 22 | f'(x)=3(-x+2)^2
@choice 23 | f'(x)=-3(-x+2)^3
@answer 21
@feedback 22 | The derivative of -x+2 is -1. Losing it reverses the derivative's sign.
@feedback 23 | The cube's exponent drops from 3 to 2. Retaining exponent 3 does not differentiate the outer function.
@after f'(x)=-3(-x+2)^2=-3x^2+12x-12
@wrong Keep the negative inner multiplier and reduce the exponent.
@why The outer derivative is 3(-x+2) squared and the inner derivative is -1. Expanding the square as x squared-4x+4 and multiplying by -3 gives -3x squared+12x-12.
@step 30 | Evaluate height and slope at the specified input.
@choice 32 | (f(0),f'(0))=(-12,8)
@choice 31 | (f(0),f'(0))=(8,-12)
@choice 33 | (f(0),f'(0))=(8,12)
@answer 31
@feedback 32 | The original output and derivative value are in the opposite order. The cube produces the height.
@feedback 33 | The missing inner minus sign makes the slope positive. The complete derivative at zero is negative.
@after (f(0),f'(0))=(8,-12)
@wrong Use the original cube for the height and its derivative for the slope.
@why The original at zero is 2 cubed=8. The derivative is -3 times 2 squared=-12. These are different quantities even though both are evaluated at zero.
@step 40 | Select the tangent at x0=0.
@choice 42 | y=12x+8
@choice 43 | y=-12x-8
@choice 41 | y=-12x+8
@answer 41
@feedback 42 | This has the correct intercept but the wrong slope sign. The derivative value is -12.
@feedback 43 | The slope is right, but at zero this gives -8 rather than the original height 8.
@after y=-12x+8
@wrong Preserve both the height and the signed slope.
@why Point-slope form is y=8-12(x-0)=-12x+8. The line gives L(0)=8=f(0) and L'=-12=f'(0), so both checks pass.
@end

@question prod02_calc_derivative_tangent_q04 | Tangent at a nonzero input
@template choices.v1
@version 1
@goal Find the derivative function, the height and slope at x0, and the tangent line; verify its point and slope.
@given f(x)=(2x^2-x+1)(x-1),\quad x_0=2
@domain All inputs are real. The given polynomial is differentiable for every real x.
@read prod02_calc_derivative_tangent_r
@step 10 | Let u and v be the first and second factors. Which identity differentiates f=uv?
@choice 11 | f'=u'v+uv'
@choice 13 | f'=u'+v'
@choice 12 | f'=u'v'
@answer 11
@feedback 13 | The original joins factors by multiplication. The derivative of a sum does not apply to that structure.
@feedback 12 | Differentiating both factors at once omits the two unchanged-factor contributions of the product rule.
@after f'=u'v+uv'
@wrong Keep the original multiplication structure in view.
@why Both the quadratic and linear factors vary with x and are differentiable. Each needs a contribution while the other factor is retained.
@step 20 | Compute (u',v') for u=2x^2-x+1 and v=x-1.
@choice 22 | (u',v')=(4x,1)
@choice 21 | (u',v')=(4x-1,1)
@choice 23 | (u',v')=(4x-1,0)
@answer 21
@feedback 22 | The term -x contributes derivative -1; it cannot disappear with the constant term.
@feedback 23 | The x in the second factor contributes derivative 1 even though the constant -1 differentiates to zero.
@after (u',v')=(4x-1,1)
@wrong Differentiate every nonconstant term and retain its sign.
@why Twice x squared differentiates to 4x, -x to -1, and the constant to zero. The derivative of x-1 is 1.
@step 30 | Combine both contributions to the derivative function.
@choice 32 | f'(x)=(4x-1)(x-1)
@choice 33 | f'(x)=4x-1
@choice 31 | f'(x)=(4x-1)(x-1)+(2x^2-x+1)
@answer 31
@feedback 32 | This is only the contribution from differentiating the first factor. The second factor's derivative adds the unchanged quadratic.
@feedback 33 | This multiplies the factor derivatives rather than applying the product rule to the original function.
@after f'(x)=(4x-1)(x-1)+(2x^2-x+1)=6x^2-6x+2
@wrong Expand only after both product terms are present.
@why The first term expands to 4x squared-5x+1. Adding 2x squared-x+1 gives 6x squared-6x+2.
@step 40 | Evaluate the original height and derivative slope at x0=2.
@choice 41 | (f(2),f'(2))=(7,14)
@choice 42 | (f(2),f'(2))=(14,7)
@choice 43 | (f(2),f'(2))=(7,7)
@answer 41
@feedback 42 | These reverse height and slope. Evaluate the original product for the first entry.
@feedback 43 | The slope 7 retains only (4 times 2-1)(2-1). The other product-rule contribution also equals 7 here and must be added.
@after (f(2),f'(2))=(7,14)
@wrong Evaluate the entire derivative, including both product contributions.
@why The original is (2 times 4-2+1)(2-1)=7 times 1=7. The derivative is 6 times 4-6 times 2+2=24-12+2=14.
@step 50 | Construct the tangent using this point and slope.
@choice 52 | y=14x+7
@choice 51 | y=14x-21
@choice 53 | y=7x-7
@answer 51
@feedback 52 | The line gives 35 at x=2, not 7. The original height at 2 is not the intercept at zero.
@feedback 53 | This passes through the point but has slope 7, not the derivative value 14.
@after y=14x-21
@wrong Use the displacement x-2 when starting from the original point.
@why The tangent is y=7+14(x-2)=14x-21. Its value at 2 is 28-21=7=f(2), and its derivative is 14=f'(2). These checks establish the tangent.
@end

@question prod02_calc_derivative_tangent_q05 | A shifted cube and its tangent
@template choices.v1
@version 1
@goal Find the derivative function, the height and slope at x0, and the tangent line; verify its point and slope.
@given f(x)=(3x-1)^3,\quad x_0=1
@domain All inputs are real. The given polynomial is differentiable for every real x.
@read prod02_calc_derivative_tangent_r
@step 10 | Let u be the inner expression and n the displayed exponent. Which identity differentiates f=u^n?
@choice 13 | f'=u^nu'
@choice 11 | f'=nu^{n-1}u'
@choice 12 | f'=nu^{n-1}
@answer 11
@feedback 13 | The outer cube must be differentiated; leaving its exponent and multiplier unchanged omits the outer rule.
@feedback 12 | This differentiates only the outer power. The inner expression 3x-1 also varies with x.
@after f'=nu^{n-1}u'
@wrong A composed power needs both outer and inner derivatives.
@why The outer cube and inner linear function are differentiable everywhere. Their derivative contributions multiply under the chain rule.
@step 20 | Compute f'(x) with all required factors.
@choice 22 | f'(x)=3(3x-1)^2
@choice 23 | f'(x)=9(3x-1)^3
@choice 21 | f'(x)=9(3x-1)^2
@answer 21
@feedback 22 | This omits the inner derivative 3. The exponent multiplier and the inner multiplier are separate factors.
@feedback 23 | The outer exponent must be reduced from 3 to 2 after differentiation.
@after f'(x)=9(3x-1)^2=81x^2-54x+9
@wrong Keep both multipliers and reduce the power once.
@why Differentiating the cube gives 3(3x-1) squared, then multiplying by the inner derivative 3 gives 9(3x-1) squared. The square is 9x squared-6x+1; multiplying by 9 gives 81x squared-54x+9.
@step 30 | Evaluate (f(1),f'(1)) in that order.
@choice 31 | (f(1),f'(1))=(8,36)
@choice 32 | (f(1),f'(1))=(36,8)
@choice 33 | (f(1),f'(1))=(8,12)
@answer 31
@feedback 32 | The cube gives the original height, and the derivative gives slope. These entries exchange them.
@feedback 33 | The slope 12 results from omitting the inner multiplier 3. Use the complete derivative function.
@after (f(1),f'(1))=(8,36)
@wrong Evaluate the original function and its full derivative separately.
@why The inner value is 3 times 1-1=2. The original gives 2 cubed=8, whereas the derivative gives 9 times 2 squared=36.
@step 40 | Select the tangent line at this input.
@choice 42 | y=36x+8
@choice 41 | y=36x-28
@choice 43 | y=12x-4
@answer 41
@feedback 42 | At x=1 this gives 44 instead of 8. Subtract the slope times x0 to find the intercept.
@feedback 43 | This passes through the original point but its slope is 12, the omitted-inner-factor result rather than 36.
@after y=36x-28
@wrong A correct point cannot compensate for the wrong derivative slope.
@why Point-slope form gives y=8+36(x-1)=36x-28. Incidence is 36-28=8=f(1); the line's derivative is 36=f'(1). Both conditions agree with the original.
@end

@question prod02_calc_derivative_tangent_q06 | A product at a negative input
@template choices.v1
@version 1
@goal Find the derivative function, the height and slope at x0, and the tangent line; verify its point and slope.
@given f(x)=(x^2-2)(-2x+1),\quad x_0=-1
@domain All inputs are real. The given polynomial is differentiable for every real x.
@read prod02_calc_derivative_tangent_r
@step 10 | Let u and v be the first and second factors. Which identity differentiates f=uv?
@choice 12 | f'=u'v'
@choice 13 | f'=u'+v'
@choice 11 | f'=u'v+uv'
@answer 11
@feedback 12 | Both derivatives multiplied together lose the required contributions from the original factors.
@feedback 13 | This applies a sum rule to a product. Retain the other factor in each derivative term.
@after f'=u'v+uv'
@wrong The connecting operation is multiplication, despite the minus signs inside the factors.
@why Both polynomial factors vary with x. The product rule contributes the derivative of each factor while retaining the other, then adds the results.
@step 20 | Compute (u',v') for u=x^2-2 and v=-2x+1.
@choice 21 | (u',v')=(2x,-2)
@choice 22 | (u',v')=(2x,2)
@choice 23 | (u',v')=(x^2-2,-2)
@answer 21
@feedback 22 | The coefficient of x in the second factor is -2. Differentiation retains that negative sign.
@feedback 23 | The first entry is the original factor, not its derivative. Its constant vanishes and its square differentiates to 2x.
@after (u',v')=(2x,-2)
@wrong Differentiate the signed linear term without changing its coefficient.
@why The derivative of x squared-2 is 2x. The derivative of -2x+1 is -2. Each constant term differentiates to zero.
@step 30 | Assemble f'(x) before evaluating the negative input.
@choice 32 | f'(x)=2x(-2x+1)
@choice 31 | f'(x)=2x(-2x+1)-2(x^2-2)
@choice 33 | f'(x)=-4x
@answer 31
@feedback 32 | The second factor also changes. Its derivative -2 multiplies the unchanged first factor and must contribute another term.
@feedback 33 | This multiplies 2x by -2 rather than computing the derivative of the original product.
@after f'(x)=2x(-2x+1)-2(x^2-2)=-6x^2+2x+4
@wrong Retain the negative sign on the entire second contribution.
@why The first contribution is -4x squared+2x. The second is -2x squared+4, since -2 multiplies both terms of x squared-2. Adding gives -6x squared+2x+4.
@step 40 | Evaluate the height and slope at x0=-1.
@choice 42 | (f(-1),f'(-1))=(-4,-3)
@choice 43 | (f(-1),f'(-1))=(-3,-6)
@choice 41 | (f(-1),f'(-1))=(-3,-4)
@answer 41
@feedback 42 | The entries interchange derivative slope and original height.
@feedback 43 | The slope -6 is only the first product contribution at -1. The other contribution is 2, so both are needed.
@after (f(-1),f'(-1))=(-3,-4)
@wrong Square the negative input before multiplying its coefficient.
@why The original gives (1-2)(2+1)=(-1) times 3=-3. The derivative gives -6 times 1+2 times (-1)+4=-6-2+4=-4.
@step 50 | Construct the tangent at the negative input.
@choice 51 | y=-4x-7
@choice 52 | y=-4x-3
@choice 53 | y=-3x-6
@answer 51
@feedback 52 | This uses -3 as the intercept. At x=-1 it gives 1 rather than the original height -3.
@feedback 53 | This passes through (-1,-3) but has slope -3, which is the height rather than the slope -4.
@after y=-4x-7
@wrong At x0=-1 the displacement is x+1, not x-1.
@why The line is y=-3-4(x+1)=-4x-7. Incidence check: -4 times (-1)-7=4-7=-3=f(-1). Its derivative is -4=f'(-1), verifying the slope as well.
@end

@question prod02_calc_derivative_tangent_q07 | Tangent to a fourth power
@template choices.v1
@version 1
@goal Find the derivative function, the height and slope at x0, and the tangent line; verify its point and slope.
@given f(x)=(2x+2)^4,\quad x_0=-1
@domain All inputs are real. The given polynomial is differentiable for every real x.
@read prod02_calc_derivative_tangent_r
@step 10 | Let u be the inner expression and n the displayed exponent. Which identity differentiates f=u^n?
@choice 11 | f'=nu^{n-1}u'
@choice 12 | f'=nu^{n-1}
@choice 13 | f'=u^nu'
@answer 11
@feedback 12 | This drops the inner derivative. Its omission changes the derivative function even if two expressions might agree at a particular input.
@feedback 13 | The outer exponent must be differentiated, not merely retained while multiplying by the inner derivative.
@after f'=nu^{n-1}u'
@wrong Choose a rule for the function before substituting x0.
@why The fourth power is composed with the linear expression 2x+2. The chain rule applies everywhere because both functions are polynomials.
@step 20 | Compute the derivative function for all real x, not just its value at x0.
@choice 22 | f'(x)=4(2x+2)^3
@choice 21 | f'(x)=8(2x+2)^3
@choice 23 | f'(x)=0
@answer 21
@feedback 22 | This omits the inner derivative 2. Agreement at a zero of the inner factor would not prove equality of the two derivative functions.
@feedback 23 | A derivative can vanish at one input without being the zero function. Differentiate before evaluating.
@after f'(x)=8(2x+2)^3=64x^3+192x^2+192x+64
@wrong A single derivative value cannot replace the derivative function.
@why The fourth power contributes 4(2x+2) cubed and the inner derivative contributes 2. The cube expands to 8x cubed+24x squared+24x+8; multiplication by 8 gives the displayed polynomial.
@step 30 | Now evaluate the original height and derivative slope at -1.
@choice 32 | (f(-1),f'(-1))=(0,8)
@choice 33 | (f(-1),f'(-1))=(16,0)
@choice 31 | (f(-1),f'(-1))=(0,0)
@answer 31
@feedback 32 | The coefficient 8 is multiplied by the inner expression cubed. Evaluate that entire product, not only its coefficient.
@feedback 33 | The height 16 evaluates the original at zero. The specified input is -1, so the inner value changes.
@after (f(-1),f'(-1))=(0,0)
@wrong Substitute the specified input into every occurrence of x.
@why The inner value at -1 is 2 times (-1)+2=0. Thus f(-1)=0 to the fourth power=0, and f'(-1)=8 times 0 cubed=0. The slope at this point vanishes although the derivative function does not vanish everywhere.
@step 40 | Select the tangent with the computed point and slope.
@choice 41 | y=0
@choice 42 | y=x+1
@choice 43 | y=1
@answer 41
@feedback 42 | This passes through (-1,0) but has slope 1, not zero.
@feedback 43 | This is horizontal but has height 1 rather than the required original height zero.
@after y=0
@wrong Zero slope and correct height are independent requirements.
@why Point-slope form gives y=0+0(x+1)=0. The line satisfies L(-1)=0=f(-1), and its derivative is zero=f'(-1). It is the horizontal tangent at the original point, not an assertion that f is a constant function.
@end

@question prod02_calc_derivative_tangent_q08 | A signed quadratic factor
@template choices.v1
@version 1
@goal Find the derivative function, the height and slope at x0, and the tangent line; verify its point and slope.
@given f(x)=(-x^2+x+2)(3x-1),\quad x_0=0
@domain All inputs are real. The given polynomial is differentiable for every real x.
@read prod02_calc_derivative_tangent_r
@step 10 | Let u and v be the first and second factors. Which identity differentiates f=uv?
@choice 13 | f'=u'+v'
@choice 11 | f'=u'v+uv'
@choice 12 | f'=u'v'
@answer 11
@feedback 13 | The original is not a sum of these factors. A product requires the other factor to remain in each contribution.
@feedback 12 | Multiplying factor derivatives is not the product rule; both original factors must contribute through two separate terms.
@after f'=u'v+uv'
@wrong A minus sign within a factor does not change the outer product structure.
@why The quadratic and linear factors are differentiable polynomials. Differentiate each factor once in its own contribution while retaining the other.
@step 20 | Compute (u',v') for u=-x^2+x+2 and v=3x-1.
@choice 22 | (u',v')=(-2x,3)
@choice 23 | (u',v')=(-2x+1,-1)
@choice 21 | (u',v')=(-2x+1,3)
@answer 21
@feedback 22 | The linear x term in u contributes derivative 1. Only the constant 2 disappears.
@feedback 23 | The derivative of v is the coefficient 3 of x, not its constant term -1.
@after (u',v')=(-2x+1,3)
@wrong Preserve the distinction between a linear coefficient and a constant term.
@why Differentiating -x squared gives -2x and differentiating x gives 1. The constant gives zero. The second factor's derivative is 3.
@step 30 | Assemble the complete derivative function.
@choice 31 | f'(x)=(-2x+1)(3x-1)+3(-x^2+x+2)
@choice 32 | f'(x)=(-2x+1)(3x-1)
@choice 33 | f'(x)=-6x+3
@answer 31
@feedback 32 | This omits the derivative contribution from the second factor, whose derivative is 3.
@feedback 33 | This multiplies the factor derivatives. The original product requires the two retained-factor contributions instead.
@after f'(x)=(-2x+1)(3x-1)+3(-x^2+x+2)=-9x^2+8x+5
@wrong Distribute signs through both product terms before combining them.
@why The first product expands to -6x squared+5x-1. The second expands to -3x squared+3x+6. Their sum is -9x squared+8x+5.
@step 40 | Evaluate the original output and the derivative value at zero.
@choice 42 | (f(0),f'(0))=(5,-2)
@choice 41 | (f(0),f'(0))=(-2,5)
@choice 43 | (f(0),f'(0))=(-2,-1)
@answer 41
@feedback 42 | These exchange the original height and the derivative slope.
@feedback 43 | The value -1 comes from only the first derivative contribution. The second adds 6 at zero.
@after (f(0),f'(0))=(-2,5)
@wrong Even at zero, a constant derivative contribution may survive.
@why The original is 2 times (-1)=-2. The derivative polynomial at zero equals its constant term 5; equivalently its two product contributions are -1 and 6.
@step 50 | Select the tangent at this point.
@choice 52 | y=-2x+5
@choice 53 | y=-5x-2
@choice 51 | y=5x-2
@answer 51
@feedback 52 | This swaps slope and intercept. At zero the height must be -2, and the slope must be 5.
@feedback 53 | This has the correct intercept but reverses the derivative slope's sign.
@after y=5x-2
@wrong Use the derivative value as the x coefficient, not the height.
@why The tangent is y=-2+5(x-0)=5x-2. At zero it gives -2=f(0), and its derivative is 5=f'(0). These two checks use the original data separately.
@end

@question prod02_calc_derivative_tangent_q09 | Combining a power and another factor
@template choices.v1
@version 1
@goal Find the derivative function, the height and slope at x0, and the tangent line; verify its point and slope.
@given f(x)=(x+1)^2(2x-1),\quad x_0=1
@domain All inputs are real. The given polynomial is differentiable for every real x.
@read prod02_calc_derivative_tangent_r
@step 10 | Set u=x+1, v=2x-1 and n=2. Which identity handles the whole function f=u^n v?
@choice 12 | f'=nu^{n-1}u'v'
@choice 13 | f'=nu^{n-1}u'v
@choice 11 | f'=nu^{n-1}u'v+u^nv'
@answer 11
@feedback 12 | This differentiates both factors and multiplies their derivatives. The product rule needs two separate contributions.
@feedback 13 | This handles the powered factor but treats v as unchanged throughout. The second factor also has a derivative contribution.
@after f'=nu^{n-1}u'v+u^nv'
@wrong Identify both the outer product and the power of the inner function.
@why Apply the product rule to u squared times v, and use the chain rule when differentiating u squared. All factors are polynomials, so both rules apply for every real x.
@step 20 | Compute derivatives of the inner linear expression u and the other factor v.
@choice 21 | (u',v')=(1,2)
@choice 22 | (u',v')=(1,1)
@choice 23 | (u',v')=(2,2)
@answer 21
@feedback 22 | The coefficient of x in v=2x-1 is 2, not 1.
@feedback 23 | Here u is x+1, not u squared. Its derivative is 1; the outer exponent is handled separately.
@after (u',v')=(1,2)
@wrong Differentiate the defined inner expression, not the whole powered factor at this stage.
@why The inner linear function x+1 has derivative 1. The second factor 2x-1 has derivative 2. Constants contribute zero in each.
@step 30 | Assemble f'(x) using both rules and every multiplier.
@choice 32 | f'(x)=4(x+1)
@choice 31 | f'(x)=2(x+1)(2x-1)+2(x+1)^2
@choice 33 | f'(x)=2(x+1)(2x-1)
@answer 31
@feedback 32 | This multiplies the two factor derivatives. It drops the original factors that belong in the two product-rule terms.
@feedback 33 | The contribution from differentiating the second factor is missing: its derivative 2 multiplies the unchanged square.
@after f'(x)=2(x+1)(2x-1)+2(x+1)^2=6x^2+6x
@wrong Preserve the second contribution even after applying the chain rule correctly to the first.
@why The first contribution expands to 4x squared+2x-2. The second is 2x squared+4x+2. The constants cancel and the sum is 6x squared+6x.
@step 40 | Evaluate height and slope at x0=1.
@choice 42 | (f(1),f'(1))=(12,4)
@choice 43 | (f(1),f'(1))=(4,4)
@choice 41 | (f(1),f'(1))=(4,12)
@answer 41
@feedback 42 | These exchange the original function output and derivative value.
@feedback 43 | The slope 4 uses only the first derivative contribution at 1; the second contributes another 8.
@after (f(1),f'(1))=(4,12)
@wrong Use the full derivative after computing the original product.
@why The original gives (1+1) squared times (2-1)=4 times 1=4. The derivative gives 6 times 1 squared+6 times 1=12.
@step 50 | Construct the tangent with this point and slope.
@choice 51 | y=12x-8
@choice 52 | y=12x+4
@choice 53 | y=4x
@answer 51
@feedback 52 | The height 4 occurs at x=1, not at zero. This line instead gives 16 at the required input.
@feedback 53 | This passes through (1,4) but has slope 4 instead of the derivative value 12.
@after y=12x-8
@wrong Preserve the distinction between height and derivative slope.
@why Point-slope form is y=4+12(x-1)=12x-8. The line gives L(1)=12-8=4=f(1), and L'=12=f'(1), so the tangent satisfies both checks.
@end

@question prod02_calc_derivative_tangent_q10 | A cubed factor in a product
@template choices.v1
@version 1
@goal Find the derivative function, the height and slope at x0, and the tangent line; verify its point and slope.
@given f(x)=(2x-1)^3(x+1),\quad x_0=0
@domain All inputs are real. The given polynomial is differentiable for every real x.
@read prod02_calc_derivative_tangent_r
@step 10 | Set u=2x-1, v=x+1 and n=3. Which identity handles the whole function f=u^n v?
@choice 11 | f'=nu^{n-1}u'v+u^nv'
@choice 13 | f'=nu^{n-1}u'v
@choice 12 | f'=nu^{n-1}u'v'
@answer 11
@feedback 13 | This differentiates the cubed factor but omits the contribution from the second factor's derivative.
@feedback 12 | The derivative of the product is not the product of the two factor derivatives. Retain an original factor in each contribution.
@after f'=nu^{n-1}u'v+u^nv'
@wrong The whole expression requires a product rule as well as a chain rule inside one factor.
@why Differentiate u cubed by the chain rule in the first product contribution. The other contribution retains u cubed and differentiates v. Both polynomial factors are differentiable at all real inputs.
@step 20 | Compute derivatives of the defined u and v.
@choice 22 | (u',v')=(1,1)
@choice 21 | (u',v')=(2,1)
@choice 23 | (u',v')=(2,0)
@answer 21
@feedback 22 | The inner function has coefficient 2 on x, so its derivative is 2 rather than 1.
@feedback 23 | The second factor contains x and is not constant. Its derivative is 1.
@after (u',v')=(2,1)
@wrong Differentiate the complete linear expressions, including their x coefficients.
@why Differentiating 2x-1 gives 2; differentiating x+1 gives 1. These are the inner multiplier and second-factor derivative needed in the chosen identity.
@step 30 | Compute f'(x) with the inner multiplier and both product terms.
@choice 32 | f'(x)=3(2x-1)^2(x+1)+(2x-1)^3
@choice 33 | f'(x)=6(2x-1)^2(x+1)
@choice 31 | f'(x)=6(2x-1)^2(x+1)+(2x-1)^3
@answer 31
@feedback 32 | The cube contributes exponent 3 and the inner function contributes another multiplier 2; the first coefficient must include both.
@feedback 33 | This first contribution is correct but incomplete. Differentiating the second factor adds the unchanged cube.
@after f'(x)=6(2x-1)^2(x+1)+(2x-1)^3=32x^3-12x^2-12x+5
@wrong Check for a missing multiplier separately from a missing product term.
@why The first contribution expands to 24x cubed-18x+6. The second cube is 8x cubed-12x squared+6x-1. Adding gives 32x cubed-12x squared-12x+5.
@step 40 | Evaluate the original height and full derivative slope at zero.
@choice 41 | (f(0),f'(0))=(-1,5)
@choice 42 | (f(0),f'(0))=(-1,2)
@choice 43 | (f(0),f'(0))=(-1,6)
@answer 41
@feedback 42 | A slope of 2 uses multiplier 3 instead of 6 in the first contribution, then adds the cube value -1, giving 3+(-1)=2. The missing inner derivative 2 is the error.
@feedback 43 | The first contribution at zero is 6, but the second contributes -1. Add both to get the complete slope.
@after (f(0),f'(0))=(-1,5)
@wrong An odd power of -1 remains negative when evaluating the second contribution.
@why The original gives (-1) cubed times 1=-1. The derivative gives 6 times (-1) squared times 1 plus (-1) cubed=6-1=5, agreeing with the expanded derivative's constant term.
@step 50 | Select the line matching the original point and derivative slope.
@choice 52 | y=-5x-1
@choice 51 | y=5x-1
@choice 53 | y=5x+1
@answer 51
@feedback 52 | The point at zero is correct, but the slope is the negative of the required derivative value 5.
@feedback 53 | The slope is correct, but the original value at zero is -1 rather than 1.
@after y=5x-1
@wrong Check the sign of the original height independently of the slope.
@why Point-slope form is y=-1+5(x-0)=5x-1. The line gives L(0)=-1=f(0), and L'=5=f'(0). Its incidence and slope both match the original function.
@end

@question prod02_calc_derivative_tangent_q11 | A reversed inner expression
@template choices.v1
@version 1
@goal Find the derivative function, the height and slope at x0, and the tangent line; verify its point and slope.
@given f(x)=(-x+1)^2(x+2),\quad x_0=-1
@domain All inputs are real. The given polynomial is differentiable for every real x.
@read prod02_calc_derivative_tangent_r
@step 10 | Set u=-x+1, v=x+2 and n=2. Which identity handles the whole function f=u^n v?
@choice 12 | f'=nu^{n-1}u'v'
@choice 11 | f'=nu^{n-1}u'v+u^nv'
@choice 13 | f'=nu^{n-1}u'v
@answer 11
@feedback 12 | This multiplies the derivatives of the two factors rather than retaining one original factor in each of two contributions.
@feedback 13 | The first contribution alone leaves out the derivative of v, which is not a constant function.
@after f'=nu^{n-1}u'v+u^nv'
@wrong Select an identity for the entire product, not only the squared factor.
@why The square requires a chain rule and the two-factor multiplication requires a product rule. The factors are differentiable polynomials at every real input.
@step 20 | Compute the derivatives of u=-x+1 and v=x+2.
@choice 22 | (u',v')=(1,1)
@choice 23 | (u',v')=(-1,0)
@choice 21 | (u',v')=(-1,1)
@answer 21
@feedback 22 | The derivative of the negative x term is -1. The constant 1 does not reverse that sign.
@feedback 23 | The derivative of x+2 is 1, not zero; only its constant term disappears.
@after (u',v')=(-1,1)
@wrong A negative inner multiplier affects the powered factor's derivative.
@why The inner derivative is -1 and the other factor's derivative is 1. Keep these signed multipliers separate from the outer exponent 2.
@step 30 | Form the derivative function with both signed contributions.
@choice 31 | f'(x)=-2(-x+1)(x+2)+(-x+1)^2
@choice 32 | f'(x)=2(-x+1)(x+2)+(-x+1)^2
@choice 33 | f'(x)=-2(-x+1)(x+2)
@answer 31
@feedback 32 | This loses the inner derivative -1 and reverses the sign of the first contribution.
@feedback 33 | This omits the contribution from differentiating the factor x+2, whose derivative is 1.
@after f'(x)=-2(-x+1)(x+2)+(-x+1)^2=3x^2-3
@wrong Add both contributions before deciding whether terms cancel.
@why The first contribution expands to 2x squared+2x-4. The square in the second contribution is x squared-2x+1. The linear terms cancel, leaving 3x squared-3.
@step 40 | Evaluate height and slope at x0=-1.
@choice 42 | (f(-1),f'(-1))=(0,4)
@choice 41 | (f(-1),f'(-1))=(4,0)
@choice 43 | (f(-1),f'(-1))=(4,-4)
@answer 41
@feedback 42 | These interchange the original height and derivative value. A zero slope does not imply a zero height.
@feedback 43 | The first derivative contribution is -4 at this input, but the second contributes +4. Both must be added.
@after (f(-1),f'(-1))=(4,0)
@wrong Preserve cancellation between the two derivative contributions.
@why The original gives (1+1) squared times (-1+2)=4 times 1=4. The derivative gives 3 times (-1) squared-3=0. The zero slope occurs at a nonzero original height.
@step 50 | Construct the tangent through the original point.
@choice 52 | y=4x+8
@choice 53 | y=0
@choice 51 | y=4
@answer 51
@feedback 52 | This passes through (-1,4) but has slope 4, the height rather than the computed slope zero.
@feedback 53 | This is horizontal but passes through height zero instead of the original height 4.
@after y=4
@wrong A horizontal tangent retains the original point's height.
@why Point-slope form gives y=4+0(x+1)=4. Its value at -1 is 4=f(-1) and its derivative is zero=f'(-1). Both conditions verify the horizontal tangent.
@end

@question prod02_calc_derivative_tangent_q12 | A fourth-power factor and a line
@template choices.v1
@version 1
@goal Find the derivative function, the height and slope at x0, and the tangent line; verify its point and slope.
@given f(x)=(x-1)^4(-x+2),\quad x_0=0
@domain All inputs are real. The given polynomial is differentiable for every real x.
@read prod02_calc_derivative_tangent_r
@step 10 | Set u=x-1, v=-x+2 and n=4. Which identity handles the whole function f=u^n v?
@choice 13 | f'=nu^{n-1}u'v
@choice 12 | f'=nu^{n-1}u'v'
@choice 11 | f'=nu^{n-1}u'v+u^nv'
@answer 11
@feedback 13 | The first contribution alone treats the second factor as constant. Its derivative must also contribute.
@feedback 12 | This multiplies the two factor derivatives instead of applying the two-term product rule.
@after f'=nu^{n-1}u'v+u^nv'
@wrong Choose the combined identity before inserting the signed derivatives.
@why The whole expression is a product and one factor is a fourth power of an inner function. Use the product rule and then the chain rule on that powered factor; all rule conditions hold for these polynomials.
@step 20 | Compute derivatives of the defined u and v.
@choice 21 | (u',v')=(1,-1)
@choice 22 | (u',v')=(1,1)
@choice 23 | (u',v')=(-1,-1)
@answer 21
@feedback 22 | The second factor has coefficient -1 on x, so its derivative retains a minus sign.
@feedback 23 | The -1 in u=x-1 is a constant. Its derivative is zero, while the derivative of x is +1.
@after (u',v')=(1,-1)
@wrong Distinguish a negative constant from a negative x coefficient.
@why The inner derivative is 1. The derivative of the second factor -x+2 is -1. These determine the different signs of the two product contributions.
@step 30 | Compute the complete derivative function.
@choice 32 | f'(x)=4(x-1)^3(-x+2)+(x-1)^4
@choice 31 | f'(x)=4(x-1)^3(-x+2)-(x-1)^4
@choice 33 | f'(x)=4(x-1)^3(-x+2)
@answer 31
@feedback 32 | The second contribution must multiply the unchanged fourth power by v'=-1, not +1.
@feedback 33 | This contains the first contribution but omits the entire second contribution from differentiating v.
@after f'(x)=4(x-1)^3(-x+2)-(x-1)^4=-5x^4+24x^3-42x^2+32x-9
@wrong Apply the sign of v' to the entire retained powered factor.
@why The first contribution expands to -4x to the fourth+20x cubed-36x squared+28x-8. Subtract the fourth power x to the fourth-4x cubed+6x squared-4x+1. Combining each power gives -5x to the fourth+24x cubed-42x squared+32x-9.
@step 40 | Evaluate the original function and derivative at zero.
@choice 42 | (f(0),f'(0))=(2,-7)
@choice 43 | (f(0),f'(0))=(2,-8)
@choice 41 | (f(0),f'(0))=(2,-9)
@answer 41
@feedback 42 | This adds the unchanged fourth power instead of subtracting it in the second derivative contribution.
@feedback 43 | The first derivative contribution is -8, but the second contributes another -1. Do not stop after one term.
@after (f(0),f'(0))=(2,-9)
@wrong The cube of -1 and fourth power of -1 have different signs.
@why The original gives (-1) to the fourth times 2=1 times 2=2. The derivative gives 4 times (-1) cubed times 2 minus (-1) to the fourth=-8-1=-9, matching the expanded constant term.
@step 50 | Select the tangent through the original point with the full derivative slope.
@choice 51 | y=-9x+2
@choice 52 | y=-7x+2
@choice 53 | y=-9x-2
@answer 51
@feedback 52 | This keeps the original point but uses the wrong slope from reversing the second product contribution's sign.
@feedback 53 | The derivative slope is correct, but at zero the line gives -2 instead of the original height 2.
@after y=-9x+2
@wrong A tangent needs both the original height and the complete derivative value.
@why The tangent is y=2-9(x-0)=-9x+2. Check incidence: L(0)=2=f(0). Check slope: differentiating the line gives -9=f'(0). Both conditions hold for the original degree-five polynomial.
@end
