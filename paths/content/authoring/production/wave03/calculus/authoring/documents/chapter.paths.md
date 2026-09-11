@paths 1
@subject calculus | Calculus
@chapter topic_0062 | Integration

@lesson prod03_calc_integration_r | Polynomial integration
@template lesson.v2
@block introduction | start | 1.1 | Three related questions
@prose Integration can ask for a family of functions, a particular function through a specified point, or a signed number between two endpoints. Begin by identifying which object is requested. Polynomial arithmetic and differentiation provide exact checks for all three.
@endblock
@block definition | terms | 1.2 | Functions, constants and endpoints
@prose The integrand p(x) is the function being integrated; x is the variable of integration. The symbol dx identifies that variable. An antiderivative, also called a primitive, is a differentiable function P whose derivative P'(x) equals p(x) for every input in the stated interval. A prime means differentiation with respect to x. Here all function identities hold on the real line R.
@prose A family is a collection of functions. In P(x)+C, C is an arbitrary real constant: each choice of C gives one member of the family. It is not a variable that changes with x. A fixed added number can be absorbed into C, so P+C and P+5+C describe the same family. We use P for the particular primitive with constant term zero, equivalently P(0)=0, and F for a general or condition-selected antiderivative.
@prose An initial condition F(x0)=y0 fixes a function's value at the input x0; x0 and y0 are given numbers, not new variables. The definite integral I from a to b is a signed accumulation, not a family. Here a is the lower displayed limit and b is the upper displayed limit; these names refer to positions on the integral sign, not necessarily numerical size. With increasing limits, portions below the horizontal axis contribute negatively. Unsigned area cannot be negative and is a different question.
@display objects
P'(x)=p(x),\qquad F(x)=P(x)+C,\quad C\in\mathbb{R},\qquad I=\int_a^b p(x)\,dx
@endblock
@block proposition | rule | 1.3 | Reverse the derivative coefficient
@prose For a term a_j times x to the integer power j, integration raises the exponent to j+1 and divides its coefficient by j+1. The index j labels the original power, and a_j is its coefficient. Sums integrate term by term. A constant term is power zero, so it becomes a multiple of x rather than disappearing. Differentiate the result to check every coefficient.
@display power
\int a_jx^j\,dx=\frac{a_j}{j+1}x^{j+1}+C,\qquad j\ne-1
@prose To find all antiderivatives, include one arbitrary real constant for the entire sum. To meet an initial condition, substitute its input into P+C and solve C=y0-P(x0). An expanded, collected polynomial has one term for each nonzero power, with no unexpanded product of sums. Reordering those terms does not change that form or the function.
@help proof
@prose Differentiating (a_j/(j+1)) times x to the power j+1 multiplies the coefficient by j+1 and restores a_j times x to power j. This works because j+1 is nonzero. If F and P have the same derivative on an interval, their difference has derivative zero. The mean value theorem then makes the difference constant between any two inputs in that interval. Thus P+C includes every antiderivative, not merely some examples. An initial condition fixes that one constant uniquely.
@endblock
@block proposition | condition | 1.4 | Conditions and signed evaluation
@prose The displayed power rule cannot be used with exponent -1, because its denominator would be zero. These problems use only nonnegative integer powers, so that obstruction does not occur. No logarithmic or substitution rule is needed. Every polynomial is differentiable on R and continuous on every closed real interval.
@prose The Fundamental Theorem of Calculus therefore permits evaluation using any primitive: subtract its value at the lower displayed limit from its value at the upper displayed limit. Adding a constant to the primitive does not change the difference, because the two constants cancel. Reversing the limits negates the signed integral. Equal limits give zero even if the integrand is not zero there.
@display theorem
\int_a^b p(x)\,dx=P(b)-P(a),\qquad P'=p
@display orientation
\int_b^a p(x)\,dx=-\int_a^b p(x)\,dx,\qquad\int_a^a p(x)\,dx=0
@prose A second exact check starts from the original coefficients: each degree j contributes a_j times (b to power j+1 minus a to power j+1), divided by j+1. Sum those signed contributions. This checks the original integrand and endpoint order independently of the chosen primitive. Agreement of polynomial coefficients, not agreement at a few sample inputs, establishes a derivative identity.
@endblock
@block example | worked | 1.5 | A function and an accumulation
@prose Find the function F satisfying the derivative and initial condition, then calculate the signed integral I.
@display worked_given
F'(x)=3x^2-2,\qquad F(-1)=4,\qquad I=\int_{-1}^{1}(3x^2-2)\,dx
@help hint
@prose First find a zero-constant primitive and differentiate it. The initial condition selects one constant; the definite integral instead uses the difference of two endpoint values.
@help answer
@display worked_answer
F(x)=x^3-2x+3,\qquad I=-2
@help solution
@prose Divide the coefficient 3 by the new exponent 3; the constant -2 becomes -2x. This gives P=x cubed-2x. Differentiating returns 3x squared-2, so every antiderivative is P+C by the constant-difference theorem. At -1, P(-1)=(-1) cubed-2(-1)=-1+2=1. The initial condition is 1+C=4, hence C=3.
@display worked_function
P(x)=x^3-2x,\qquad 1+C=4,\qquad F(x)=x^3-2x+3
@prose Check the original data: F'=3x squared-2 and F(-1)=-1+2+3=4. Both derivative and condition hold. For the integral, P(1)=1-2=-1 and P(-1)=1. The polynomial is continuous on [-1,1], so the theorem applies.
@display worked_integral
I=P(1)-P(-1)=-1-1=-2
@prose Independently, the quadratic contributes (3/3)(1 cubed-(-1) cubed)=2, and the constant contributes -2(1-(-1))=-4. Their sum is 2-4=-2. A signed accumulation may be negative; replacing it by a positive area would answer a different problem. Using F instead of P gives endpoint values 2 and 4, whose difference is also -2 because the added 3 cancels.
@endblock
@block example | errors | 1.6 | Check the missing operation
@prose Raising the exponent without dividing changes the derivative coefficient. Copying the integrand does not reverse differentiation. Omitting C gives one function rather than all antiderivatives. Adding a fixed number beside an arbitrary C does not make a new family. An initial-value solution must pass both derivative and value checks. In a definite integral, subtracting endpoint values in numerical-size order instead of displayed-limit order can reverse the sign. Taking an absolute value silently changes signed accumulation into a different question.
@endblock
@block exercise | practice | 1.7 | Complete each requested object
@prose Begin with four antiderivative-family problems. Then use four initial conditions to select particular functions. Finish with four definite integrals, retaining the original endpoint order and checking signed contributions. Each route carries the original polynomial through to its requested result and an exact check.
@endblock
@block summary | summary | 1.8 | Identity, condition and orientation
@prose Verify a primitive by differentiating it for every real input. Use an arbitrary real constant and the constant-difference fact for a complete family; use the initial value to fix that constant when a particular function is requested. For a signed integral, continuity justifies endpoint evaluation and the displayed order determines the subtraction. Keep these three conclusions distinct.
@endblock
@practice prod03_calc_integration_q01
@practice prod03_calc_integration_q02
@practice prod03_calc_integration_q03
@practice prod03_calc_integration_q04
@practice prod03_calc_integration_q05
@practice prod03_calc_integration_q06
@practice prod03_calc_integration_q07
@practice prod03_calc_integration_q08
@practice prod03_calc_integration_q09
@practice prod03_calc_integration_q10
@practice prod03_calc_integration_q11
@practice prod03_calc_integration_q12
@end

@question prod03_calc_integration_q01 | A polynomial integral
@template choices.v1
@version 1
@goal Find all antiderivatives of p and prove the family complete.
@given p(x)=6x^2-4x+3
@domain All identities hold for real x on R. P denotes a primitive with P(0)=0; C is an arbitrary real constant.
@read prod03_calc_integration_r
@step 10 | Choose an expanded, collected primitive P with constant term zero.
@choice 11 | P(x)=2x^3-2x^2+3x
@choice 12 | P(x)=6x^2-4x+3
@choice 13 | P(x)=6x^3-4x^2+3x
@answer 11
@feedback 12 | This copies p. Its derivative is 12x-4, not 6x squared-4x+3, and its value at zero is 3 rather than zero.
@feedback 13 | Raising the powers without dividing gives derivative 18x squared-8x+3. Divide 6 by 3 and -4 by 2 when raising those exponents.
@after P(x)=2x^3-2x^2+3x
@wrong Check each new exponent and its coefficient, including the constant term's integral.
@why The quadratic term gives (6/3)x cubed=2x cubed. The linear term gives (-4/2)x squared=-2x squared. The constant gives 3x. Every exponent divisor is positive, and P(0)=0.
@step 20 | Differentiate the chosen P to check the original integrand as a function.
@choice 22 | P'(x)=6x^2-4x
@choice 21 | P'(x)=6x^2-4x+3
@choice 23 | P'(x)=-6x^2+4x-3
@answer 21
@feedback 22 | The term 3x contributes derivative 3. A linear term does not disappear like a constant.
@feedback 23 | This negates every derivative term. Differentiation multiplies 2 by 3 and -2 by 2 without reversing all signs.
@after P'(x)=6x^2-4x+3
@wrong Compare every coefficient with the original p.
@why Differentiating gives 3 times 2=6 on x squared, 2 times (-2)=-4 on x, and 3 from 3x. Thus P'=p for every real x, not only at one test input.
@step 30 | Which expression describes all antiderivatives, with C arbitrary real?
@choice 32 | F(x)=2x^3-2x^2+3x
@choice 33 | F(x)=6x^3-4x^2+3x+C
@choice 31 | F(x)=2x^3-2x^2+3x+C
@answer 31
@feedback 32 | This is the single primitive with zero constant. It omits, for example, that same function plus 1, whose derivative is also p.
@feedback 33 | An arbitrary constant cannot repair the derivative 18x squared-8x+3. The nonconstant coefficients must first be correct.
@after F(x)=2x^3-2x^2+3x+C
@wrong Distinguish a complete family from one member of it.
@why Adding any real constant changes no derivative. Hence every displayed member has derivative 6x squared-4x+3. The next check explains why there are no additional antiderivatives outside this family.
@step 40 | Let F be any antiderivative of p. Which derivative and difference statements establish completeness on R?
@choice 41 | (F-P)'=0,\quad F-P=C
@choice 42 | (F-P)'=0,\quad F-P=x
@choice 43 | (F-P)'=1,\quad F-P=C
@answer 41
@feedback 42 | The difference x has derivative 1, not zero. Equal original derivatives allow a constant difference, not an x-dependent shift.
@feedback 43 | F' and P' both equal 6x squared-4x+3, so their subtraction is zero, not 1. A constant also has derivative zero.
@after (F-P)'=0,\quad F-P=C
@wrong Apply the constant-difference fact on the whole connected real interval.
@why Subtracting the identical derivatives gives (6x squared-4x+3)-(6x squared-4x+3)=0. By the mean value theorem F-P is constant on R. Thus F=P+C accounts for every antiderivative and completes the original request.
@end

@question prod03_calc_integration_q02 | An integral with signed coefficients
@template choices.v1
@version 1
@goal Find all antiderivatives of p and prove the family complete.
@given p(x)=-3x^2+2x-1
@domain All identities hold for real x on R. P denotes a primitive with P(0)=0; C is an arbitrary real constant.
@read prod03_calc_integration_r
@step 10 | Choose an expanded, collected primitive P with constant term zero.
@choice 12 | P(x)=-3x^2+2x-1
@choice 11 | P(x)=-x^3+x^2-x
@choice 13 | P(x)=-3x^3+2x^2-x
@answer 11
@feedback 12 | Copying p gives derivative -6x+2 rather than the quadratic integrand. Its constant term is also -1, not zero.
@feedback 13 | This raises the powers but keeps coefficients -3 and 2. Its derivative is -9x squared+4x-1; divide by the new exponents 3 and 2.
@after P(x)=-x^3+x^2-x
@wrong Preserve signs while dividing by positive new exponents.
@why The three primitives are (-3/3)x cubed=-x cubed, (2/2)x squared=x squared, and -x. Their sum has zero constant and retains the negative signs from the original first and last terms.
@step 20 | Differentiate P and compare with the original p for every real x.
@choice 23 | P'(x)=-3x^2-2x-1
@choice 22 | P'(x)=-3x^2+2x
@choice 21 | P'(x)=-3x^2+2x-1
@answer 21
@feedback 22 | Differentiating -x gives -1; the last term must not be dropped.
@feedback 23 | The coefficient of x squared in P is +1, so that derivative contribution is +2x, not -2x.
@after P'(x)=-3x^2+2x-1
@wrong Differentiate signed terms separately before combining.
@why The derivative of -x cubed is -3x squared, of x squared is 2x, and of -x is -1. The result equals all three original coefficients.
@step 30 | Select the complete antiderivative family.
@choice 31 | F(x)=-x^3+x^2-x+C
@choice 33 | F(x)=-3x^3+2x^2-x+C
@choice 32 | F(x)=-x^3+x^2-x
@answer 31
@feedback 32 | This supplies only the zero-constant member. Adding any other real constant gives another antiderivative that must be included.
@feedback 33 | The constant differentiates to zero and cannot change -9x squared+4x-1 into the required -3x squared+2x-1.
@after F(x)=-x^3+x^2-x+C
@wrong Include arbitrary vertical shifts only after finding a valid primitive.
@why Every member has the checked derivative because C is constant with respect to x. Keeping C arbitrary represents the entire family rather than selecting one function.
@step 40 | For any antiderivative F, which pair proves there are no other functions outside that family?
@choice 42 | (F-P)'=0,\quad F-P=x
@choice 41 | (F-P)'=0,\quad F-P=C
@choice 43 | (F-P)'=1,\quad F-P=C
@answer 41
@feedback 42 | A difference equal to x would add 1 to the derivative, so it cannot join two antiderivatives of the same p.
@feedback 43 | The identical derivatives subtract to zero, and the derivative of a constant is zero as well.
@after (F-P)'=0,\quad F-P=C
@wrong Use equality of derivatives on an interval, not agreement at sampled points.
@why Both derivatives are -3x squared+2x-1, so their difference is zero everywhere on R. The mean value theorem forces F-P to be a constant. This proves the family is complete.
@end

@question prod03_calc_integration_q03 | An integral with fractional coefficients
@template choices.v1
@version 1
@goal Find all antiderivatives of p and prove the family complete.
@given p(x)=\frac{1}{2}x^3-\frac{3}{2}x+2
@domain All identities hold for real x on R. P denotes a primitive with P(0)=0; C is an arbitrary real constant.
@read prod03_calc_integration_r
@step 10 | Choose an expanded, collected primitive P with constant term zero.
@choice 13 | P(x)=\frac{1}{2}x^4-\frac{3}{2}x^2+2x
@choice 12 | P(x)=\frac{1}{2}x^3-\frac{3}{2}x+2
@choice 11 | P(x)=\frac{1}{8}x^4-\frac{3}{4}x^2+2x
@answer 11
@feedback 12 | This is the original integrand. Its derivative is (3/2)x squared-3/2, not the required cubic, and P(0) would be 2.
@feedback 13 | Dividing 1/2 by 4 gives 1/8, and dividing -3/2 by 2 gives -3/4. Without those divisions the derivative is 2x cubed-3x+2.
@after P(x)=\frac{1}{8}x^4-\frac{3}{4}x^2+2x
@wrong Divide the complete rational coefficient by the new exponent.
@why The cubic contributes (1/2)/4=1/8 on x to the fourth. The linear term contributes (-3/2)/2=-3/4 on x squared. The constant contributes 2x. Each calculation is exact rational arithmetic.
@step 20 | Which derivative checks P against every coefficient of p?
@choice 21 | P'(x)=\frac{1}{2}x^3-\frac{3}{2}x+2
@choice 22 | P'(x)=\frac{1}{2}x^3-\frac{3}{4}x+2
@choice 23 | P'(x)=\frac{1}{2}x^3-\frac{3}{2}x
@answer 21
@feedback 22 | Differentiating (-3/4)x squared multiplies -3/4 by 2, producing -3/2 on x.
@feedback 23 | The term 2x contributes derivative 2. It is not an added constant in P.
@after P'(x)=\frac{1}{2}x^3-\frac{3}{2}x+2
@wrong Restore the original coefficient by multiplying by the primitive's exponent.
@why Four times 1/8 is 1/2, twice -3/4 is -3/2, and the derivative of 2x is 2. The derivative identity matches p on all of R.
@step 30 | Which expression includes every antiderivative?
@choice 32 | F(x)=\frac{1}{8}x^4-\frac{3}{4}x^2+2x
@choice 31 | F(x)=\frac{1}{8}x^4-\frac{3}{4}x^2+2x+C
@choice 33 | F(x)=\frac{1}{2}x^4-\frac{3}{2}x^2+2x+C
@answer 31
@feedback 32 | This is one antiderivative, but the requested family also contains its shifts by every real constant.
@feedback 33 | This family's derivative is 2x cubed-3x+2. No choice of C changes those wrong nonconstant coefficients.
@after F(x)=\frac{1}{8}x^4-\frac{3}{4}x^2+2x+C
@wrong An arbitrary constant enlarges a correct primitive into a family; it does not fix its derivative.
@why C may take any real value, and differentiating it gives zero. Thus every member retains the checked cubic derivative. The interval condition will establish that all antiderivatives have this form.
@step 40 | Which derivative and difference pair justifies completeness for any antiderivative F on R?
@choice 43 | (F-P)'=1,\quad F-P=C
@choice 42 | (F-P)'=0,\quad F-P=x
@choice 41 | (F-P)'=0,\quad F-P=C
@answer 41
@feedback 42 | The function x changes the derivative by 1. Equal derivatives require a constant difference, not that function.
@feedback 43 | Subtracting the same cubic derivative gives zero, not 1; a constant difference cannot have derivative 1 either.
@after (F-P)'=0,\quad F-P=C
@wrong The constant-difference theorem requires derivative equality throughout the interval.
@why F' and P' both equal (1/2)x cubed-(3/2)x+2 everywhere on R. Their difference derivative is zero. The mean value theorem makes F-P constant, proving completeness of the stated family.
@end

@question prod03_calc_integration_q04 | An integral containing a fourth power
@template choices.v1
@version 1
@goal Find all antiderivatives of p and prove the family complete.
@given p(x)=\frac{5}{2}x^4-2x^3+\frac{1}{2}
@domain All identities hold for real x on R. P denotes a primitive with P(0)=0; C is an arbitrary real constant.
@read prod03_calc_integration_r
@step 10 | Choose an expanded, collected primitive P with constant term zero.
@choice 12 | P(x)=\frac{5}{2}x^4-2x^3+\frac{1}{2}
@choice 11 | P(x)=\frac{1}{2}x^5-\frac{1}{2}x^4+\frac{1}{2}x
@choice 13 | P(x)=\frac{5}{2}x^5-2x^4+\frac{1}{2}x
@answer 11
@feedback 12 | This copies p, whose derivative is 10x cubed-6x squared. It neither integrates the powers nor has zero constant.
@feedback 13 | Raising exponents without division yields derivative (25/2)x to the fourth-8x cubed+1/2. Divide 5/2 by 5 and -2 by 4.
@after P(x)=\frac{1}{2}x^5-\frac{1}{2}x^4+\frac{1}{2}x
@wrong Integrate the constant as a linear term and divide each other coefficient by its new exponent.
@why The coefficients become (5/2)/5=1/2 and -2/4=-1/2, with new powers 5 and 4. The constant 1/2 gives (1/2)x. There is no added constant in P.
@step 20 | Differentiate P as an identity on R.
@choice 22 | P'(x)=\frac{1}{2}x^4-\frac{1}{2}x^3+\frac{1}{2}
@choice 23 | P'(x)=\frac{5}{2}x^4-2x^3
@choice 21 | P'(x)=\frac{5}{2}x^4-2x^3+\frac{1}{2}
@answer 21
@feedback 22 | Decreasing exponents is not enough: multiply the first coefficient by 5 and the second by 4.
@feedback 23 | Differentiating (1/2)x supplies the missing constant 1/2.
@after P'(x)=\frac{5}{2}x^4-2x^3+\frac{1}{2}
@wrong Check each degree, including absent powers whose coefficients remain zero.
@why Five times 1/2 is 5/2, four times -1/2 is -2, and the linear term gives 1/2. No x squared or x term is introduced. The original polynomial is recovered exactly.
@step 30 | Select the complete family rather than a single primitive.
@choice 31 | F(x)=\frac{1}{2}x^5-\frac{1}{2}x^4+\frac{1}{2}x+C
@choice 32 | F(x)=\frac{1}{2}x^5-\frac{1}{2}x^4+\frac{1}{2}x
@choice 33 | F(x)=\frac{5}{2}x^5-2x^4+\frac{1}{2}x+C
@answer 31
@feedback 32 | This fixes the constant at zero. Other constant shifts have the same derivative and belong in the answer.
@feedback 33 | This derivative has coefficients 25/2 and -8 instead of 5/2 and -2. C cannot alter either coefficient.
@after F(x)=\frac{1}{2}x^5-\frac{1}{2}x^4+\frac{1}{2}x+C
@wrong Include C only with a primitive that already differentiates to p.
@why Adding arbitrary C preserves the verified derivative. Since every real value of C is permitted, the expression includes all constant shifts of P.
@step 40 | Which pair proves every antiderivative F belongs to this family?
@choice 43 | (F-P)'=1,\quad F-P=C
@choice 41 | (F-P)'=0,\quad F-P=C
@choice 42 | (F-P)'=0,\quad F-P=x
@answer 41
@feedback 42 | The difference x would have derivative 1 and would therefore change the original derivative p.
@feedback 43 | The two derivatives are identical polynomials, so their difference is zero. The derivative of C is also zero.
@after (F-P)'=0,\quad F-P=C
@wrong Completeness uses a constant difference over the connected domain R.
@why Subtracting p from itself gives zero at every real input. The mean value theorem forces F-P to be constant throughout R. Together with the derivative check, this proves precisely all antiderivatives have the displayed form.
@end

@question prod03_calc_integration_q05 | A derivative with a specified value
@template choices.v1
@version 1
@goal Find the unique function F satisfying both the derivative and the initial condition, and check both.
@given F'(x)=4x-3,\quad F(1)=2
@domain All identities hold for real x on R. P denotes a primitive with P(0)=0; write F=P+C and determine the real constant C from the given condition.
@read prod03_calc_integration_r
@step 10 | Choose an expanded, collected primitive P with constant term zero.
@choice 13 | P(x)=4x^2-3x
@choice 12 | P(x)=4x-3
@choice 11 | P(x)=2x^2-3x
@answer 11
@feedback 12 | This copies the derivative data. Its derivative is 4, not 4x-3, and its value at zero is -3.
@feedback 13 | The x term's coefficient must be divided by 2. Otherwise this polynomial differentiates to 8x-3.
@after P(x)=2x^2-3x
@wrong Find a primitive before applying the point condition.
@why Integrating 4x gives (4/2)x squared=2x squared, and integrating -3 gives -3x. Differentiating P gives 4x-3 for every real x; P(0)=0. Thus all derivative solutions are P+C.
@step 20 | Substitute the specified input into P+C and retain the given value on the right.
@choice 21 | C-1=2
@choice 22 | C+1=2
@choice 23 | C-3=2
@answer 21
@feedback 22 | P(1)=2-3=-1, not +1. The initial condition adds C to that signed value.
@feedback 23 | This retains only -3 times 1 and omits 2 times 1 squared. Include both terms before imposing F(1)=2.
@after C-1=2
@wrong Evaluate every term of P at x0, then add the unknown constant.
@why At x=1, P(1)=2(1 squared)-3(1)=2-3=-1. Therefore F(1)=P(1)+C=-1+C, and the original condition requires C-1=2.
@step 30 | Solve that condition for the constant C.
@choice 32 | C=1
@choice 31 | C=3
@choice 33 | C=2
@answer 31
@feedback 32 | Substitution gives 1-1=0 rather than 2. Add 1 to both sides of C-1=2.
@feedback 33 | This ignores P(1)=-1. It would give F(1)=2-1=1, not 2.
@after C=3
@wrong Undo the added value P(x0) by subtracting it from y0.
@why C=2-(-1)=3. Adding 1 reverses the subtraction in the condition, and checking 3-1=2 confirms the constant.
@step 40 | Insert this constant and write the particular function as an expanded, collected polynomial.
@choice 43 | F(x)=2x^2-3x
@choice 42 | F(x)=2x^2-3x+1
@choice 41 | F(x)=2x^2-3x+3
@answer 41
@feedback 42 | This chooses C=1. Although its derivative is correct, at x=1 it gives 2-3+1=0, not 2.
@feedback 43 | This chooses C=0 and gives F(1)=-1. A correct derivative alone does not satisfy the initial value.
@after F(x)=2x^2-3x+3
@wrong Retain the integrated terms and change only the constant selected by the condition.
@why Substituting C=3 into P+C yields 2x squared-3x+3. The derivative condition allowed arbitrary shifts; the point condition selects this one shift.
@step 50 | Check the derivative function first and the original point value second.
@choice 51 | (F'(x),F(1))=(4x-3,2)
@choice 52 | (F'(x),F(1))=(4x-3,3)
@choice 53 | (F'(x),F(1))=(4x-2,2)
@answer 51
@feedback 52 | The constant 3 is not F(1). Evaluate the other terms too: 2-3+3=2.
@feedback 53 | The added constant differentiates to zero, not 1. Differentiating 2x squared-3x+3 gives 4x-3.
@after (F'(x),F(1))=(4x-3,2)
@wrong Verify derivative and initial value independently against the original data.
@why Differentiation gives 4x-3 throughout R, and evaluation gives F(1)=2-3+3=2. Any other derivative solution differs by a constant; satisfying the same point forces that difference to zero. Hence this function is the unique solution.
@end

@question prod03_calc_integration_q06 | A condition at a negative input
@template choices.v1
@version 1
@goal Find the unique function F satisfying both the derivative and the initial condition, and check both.
@given F'(x)=-\frac{3}{2}x^2+2,\quad F(-2)=1
@domain All identities hold for real x on R. P denotes a primitive with P(0)=0; write F=P+C and determine the real constant C from the given condition.
@read prod03_calc_integration_r
@step 10 | Choose an expanded, collected primitive P with constant term zero.
@choice 11 | P(x)=-\frac{1}{2}x^3+2x
@choice 13 | P(x)=-\frac{3}{2}x^3+2x
@choice 12 | P(x)=-\frac{3}{2}x^2+2
@answer 11
@feedback 12 | The derivative of this copied integrand is -3x, not (-3/2)x squared+2, and its constant is 2.
@feedback 13 | Divide -3/2 by the new exponent 3 to obtain -1/2. Keeping -3/2 gives derivative (-9/2)x squared+2.
@after P(x)=-\frac{1}{2}x^3+2x
@wrong Divide a negative rational coefficient without dropping its sign.
@why The quadratic integrates to ((-3/2)/3)x cubed=-(1/2)x cubed; 2 integrates to 2x. Differentiating multiplies -1/2 by 3 and restores the original derivative. P(0)=0.
@step 20 | Evaluate P(-2) and impose the original condition on P+C.
@choice 22 | C+2=1
@choice 23 | C-2=1
@choice 21 | C+0=1
@answer 21
@feedback 22 | P(-2) is not the constant 2 from the integrand. Its two integrated contributions are 4 and -4, whose sum is zero.
@feedback 23 | The input -2 is not the value of P there. Cubing -2 and evaluating 2x gives 4-4=0.
@after C+0=1
@wrong An odd power retains the sign of a negative input before the coefficient is applied.
@why At -2, the cubic is -8, so (-1/2)(-8)=4. The linear contribution is 2(-2)=-4. Thus P(-2)=0 and F(-2)=C+0 must equal 1.
@step 30 | Determine C from this condition.
@choice 31 | C=1
@choice 32 | C=-1
@choice 33 | C=0
@answer 31
@feedback 32 | This would give F(-2)=-1, reversing the given ordinate 1.
@feedback 33 | P(-2)=0 does not require C=0. The required value is 1, so a shift is still necessary.
@after C=1
@wrong Use the given ordinate even when the primitive contributions cancel.
@why C=1-0=1. Substituting it back gives 1+0=1, exactly the initial condition.
@step 40 | Select the particular function as an expanded, collected polynomial.
@choice 42 | F(x)=-\frac{1}{2}x^3+2x-1
@choice 41 | F(x)=-\frac{1}{2}x^3+2x+1
@choice 43 | F(x)=-\frac{1}{2}x^3+2x
@answer 41
@feedback 42 | This has C=-1 and gives 4-4-1=-1 at the given input. Its derivative is correct but the initial condition is not.
@feedback 43 | This zero-constant function gives 4-4=0 rather than 1. Do not leave the condition unused.
@after F(x)=-\frac{1}{2}x^3+2x+1
@wrong The initial condition selects the constant without changing the nonconstant terms.
@why Insert C=1 into the primitive family. The resulting function differs from P by exactly the shift needed at -2.
@step 50 | Which ordered pair verifies both original requirements?
@choice 53 | (F'(x),F(-2))=(-\frac{3}{2}x^2+3,1)
@choice 52 | (F'(x),F(-2))=(-\frac{3}{2}x^2+2,0)
@choice 51 | (F'(x),F(-2))=(-\frac{3}{2}x^2+2,1)
@answer 51
@feedback 52 | This evaluates P instead of F. Include the selected constant: 4-4+1=1.
@feedback 53 | Differentiating the added 1 gives zero. The derivative's constant term is 2 from 2x, not 3.
@after (F'(x),F(-2))=(-\frac{3}{2}x^2+2,1)
@wrong Keep function evaluation distinct from differentiation of its constant.
@why F'=(-3/2)x squared+2 is the original derivative everywhere, and F(-2)=4-4+1=1. The equal-derivative theorem leaves only a constant difference between solutions; the same initial value makes that difference zero. Uniqueness follows.
@end

@question prod03_calc_integration_q07 | A condition at a fractional input
@template choices.v1
@version 1
@goal Find the unique function F satisfying both the derivative and the initial condition, and check both.
@given F'(x)=3x^2-x+\frac{1}{2},\quad F(\frac{1}{2})=2
@domain All identities hold for real x on R. P denotes a primitive with P(0)=0; write F=P+C and determine the real constant C from the given condition.
@read prod03_calc_integration_r
@step 10 | Choose an expanded, collected primitive P with constant term zero.
@choice 12 | P(x)=3x^2-x+\frac{1}{2}
@choice 13 | P(x)=3x^3-x^2+\frac{1}{2}x
@choice 11 | P(x)=x^3-\frac{1}{2}x^2+\frac{1}{2}x
@answer 11
@feedback 12 | This differentiates to 6x-1 rather than the given quadratic. Copying the original derivative is not integration.
@feedback 13 | Divide 3 by 3 and -1 by 2. Without division the derivative becomes 9x squared-2x+1/2.
@after P(x)=x^3-\frac{1}{2}x^2+\frac{1}{2}x
@wrong Integrate each term with its own new exponent.
@why The three coefficients become 3/3=1, -1/2 and 1/2, with powers 3, 2 and 1. Differentiating gives 3x squared-x+1/2 and P(0)=0, so P+C is the full derivative family.
@step 20 | Evaluate the primitive at 1/2 and form the initial-condition equation.
@choice 22 | C+\frac{1}{8}=2
@choice 21 | C+\frac{1}{4}=2
@choice 23 | C-\frac{1}{4}=2
@answer 21
@feedback 22 | The cube contributes 1/8, but it is not the whole value. The quadratic contributes -1/8 and the linear term contributes 1/4.
@feedback 23 | The first two contributions cancel. The remaining (1/2)(1/2)=1/4 is positive, not negative.
@after C+\frac{1}{4}=2
@wrong Raise the fractional input to each power before multiplying its coefficient.
@why P(1/2)=1/8-(1/2)(1/4)+(1/2)(1/2)=1/8-1/8+1/4=1/4. Thus C+1/4 must equal the original value 2.
@step 30 | Solve for C exactly.
@choice 33 | C=2
@choice 32 | C=\frac{9}{4}
@choice 31 | C=\frac{7}{4}
@answer 31
@feedback 32 | Adding 1/4 to 2 gives 9/4, but the condition requires subtraction. This choice checks to 9/4+1/4=5/2, not 2.
@feedback 33 | Leaving C at 2 ignores the primitive value. It gives 2+1/4=9/4 at the initial input.
@after C=\frac{7}{4}
@wrong Write 2 as 8/4 before subtracting the primitive's value.
@why C=2-1/4=8/4-1/4=7/4. Checking 7/4+1/4=8/4=2 confirms the original condition.
@step 40 | Write the resulting F as an expanded, collected polynomial.
@choice 41 | F(x)=x^3-\frac{1}{2}x^2+\frac{1}{2}x+\frac{7}{4}
@choice 42 | F(x)=x^3-\frac{1}{2}x^2+\frac{1}{2}x+\frac{9}{4}
@choice 43 | F(x)=x^3-\frac{1}{2}x^2+\frac{1}{2}x
@answer 41
@feedback 42 | This selects 9/4 instead of 7/4 and yields F(1/2)=1/4+9/4=5/2. Its derivative alone cannot certify the point condition.
@feedback 43 | This selects C=0 and gives F(1/2)=1/4, not 2.
@after F(x)=x^3-\frac{1}{2}x^2+\frac{1}{2}x+\frac{7}{4}
@wrong Substitute the solved constant, not the original ordinate, into the primitive family.
@why The initial ordinate is 2, but the required vertical shift is 7/4 because P already contributes 1/4. Only the constant changes when selecting the particular function.
@step 50 | Verify the derivative function and the value at the original fractional input.
@choice 52 | (F'(x),F(\frac{1}{2}))=(3x^2-x+\frac{1}{2},\frac{7}{4})
@choice 51 | (F'(x),F(\frac{1}{2}))=(3x^2-x+\frac{1}{2},2)
@choice 53 | (F'(x),F(\frac{1}{2}))=(3x^2-x+\frac{3}{2},2)
@answer 51
@feedback 52 | The second entry is C, not F(1/2). Add P(1/2)=1/4 to obtain 2.
@feedback 53 | The added constant 7/4 differentiates to zero. The derivative's constant term stays 1/2, not 3/2.
@after (F'(x),F(\frac{1}{2}))=(3x^2-x+\frac{1}{2},2)
@wrong Verify both original requirements using exact fractions.
@why Differentiation gives 3x squared-x+1/2 for every real x. Evaluation gives 1/8-1/8+1/4+7/4=2. Any alternative solution has the same derivative, so differs by a constant; the shared value at 1/2 forces that difference to zero.
@end

@question prod03_calc_integration_q08 | A cubic derivative and a value
@template choices.v1
@version 1
@goal Find the unique function F satisfying both the derivative and the initial condition, and check both.
@given F'(x)=-2x^3+3x^2-1,\quad F(-1)=-2
@domain All identities hold for real x on R. P denotes a primitive with P(0)=0; write F=P+C and determine the real constant C from the given condition.
@read prod03_calc_integration_r
@step 10 | Choose an expanded, collected primitive P with constant term zero.
@choice 11 | P(x)=-\frac{1}{2}x^4+x^3-x
@choice 12 | P(x)=-2x^3+3x^2-1
@choice 13 | P(x)=-2x^4+3x^3-x
@answer 11
@feedback 12 | This copied cubic differentiates to -6x squared+6x, not the given cubic derivative, and its constant is -1.
@feedback 13 | Raising powers without dividing gives derivative -8x cubed+9x squared-1. Divide -2 by 4 and 3 by 3.
@after P(x)=-\frac{1}{2}x^4+x^3-x
@wrong Keep separate divisors for the cubic and quadratic terms.
@why Integration yields (-2/4)x to the fourth+(3/3)x cubed-x=-(1/2)x to the fourth+x cubed-x. Differentiating returns -2x cubed+3x squared-1 exactly, and P(0)=0.
@step 20 | Substitute -1 into P+C and use the given ordinate -2.
@choice 22 | C+\frac{1}{2}=-2
@choice 23 | C+\frac{3}{2}=-2
@choice 21 | C-\frac{1}{2}=-2
@answer 21
@feedback 22 | The fourth-power contribution is -1/2. The cubic and linear contributions are -1 and +1, which cancel; they do not reverse that -1/2.
@feedback 23 | This results from treating (-1) cubed as +1: -1/2+1+1=3/2. The cube is -1, so P(-1)=-1/2-1+1=-1/2.
@after C-\frac{1}{2}=-2
@wrong Even and odd powers of the same negative input have different signs.
@why At -1, the fourth power is 1 and the cube is -1. Thus P(-1)=-(1/2)(1)+(-1)-(-1)=-1/2. Adding C and equating to -2 gives the displayed condition.
@step 30 | Solve the signed condition for C.
@choice 31 | C=-\frac{3}{2}
@choice 33 | C=\frac{3}{2}
@choice 32 | C=-\frac{5}{2}
@answer 31
@feedback 32 | This adds -1/2 to -2 instead of subtracting it. Checking gives -5/2-1/2=-3 rather than -2.
@feedback 33 | This reverses the result's sign. It gives 3/2-1/2=1, not the negative ordinate -2.
@after C=-\frac{3}{2}
@wrong Subtracting a negative primitive value adds its magnitude.
@why C=-2-(-1/2)=-4/2+1/2=-3/2. Substitution confirms -3/2-1/2=-4/2=-2.
@step 40 | Insert C and write the particular function in expanded, collected form.
@choice 42 | F(x)=-\frac{1}{2}x^4+x^3-x-\frac{5}{2}
@choice 41 | F(x)=-\frac{1}{2}x^4+x^3-x-\frac{3}{2}
@choice 43 | F(x)=-\frac{1}{2}x^4+x^3-x
@answer 41
@feedback 42 | This has the right derivative but gives F(-1)=-1/2-5/2=-3, failing the initial condition.
@feedback 43 | C=0 gives F(-1)=-1/2. The original ordinate -2 requires the solved constant -3/2.
@after F(x)=-\frac{1}{2}x^4+x^3-x-\frac{3}{2}
@wrong A function with the correct derivative can still be the wrong member of the family.
@why Insert -3/2 into P+C without changing any nonconstant coefficient. The resulting function is selected by the original value at -1.
@step 50 | Check both derivative identity and the original point value.
@choice 53 | (F'(x),F(-1))=(-2x^3+3x^2,-2)
@choice 52 | (F'(x),F(-1))=(-2x^3+3x^2-1,-\frac{1}{2})
@choice 51 | (F'(x),F(-1))=(-2x^3+3x^2-1,-2)
@answer 51
@feedback 52 | This evaluates P but omits C=-3/2. The full function gives -1/2-3/2=-2.
@feedback 53 | The term -x differentiates to -1. Only the separate constant -3/2 disappears.
@after (F'(x),F(-1))=(-2x^3+3x^2-1,-2)
@wrong Differentiate the whole function and then evaluate the whole function in separate checks.
@why F'=-2x cubed+3x squared-1 equals the original data for every real x. At -1, F=-1/2-1+1-3/2=-2. The derivative fixes all nonconstant behavior and the initial condition fixes the remaining constant, so the solution is unique.
@end

@question prod03_calc_integration_q09 | A polynomial between two endpoints
@template choices.v1
@version 1
@goal Calculate the signed definite integral from its original polynomial and ordered limits, and check the result.
@given p(x)=3x^2-2x+1,\quad I=\int_{-1}^{2}p(x)\,dx
@domain The polynomial is defined on R and continuous on the closed interval between the given limits. a is the lower displayed limit and b the upper displayed limit; retain their order. P is a primitive with P(0)=0.
@read prod03_calc_integration_r
@step 10 | Which method uses a primitive of p to evaluate the original signed integral?
@choice 12 | I=P(a)-P(b),\quad P'=p
@choice 11 | I=P(b)-P(a),\quad P'=p
@choice 13 | I=p(b)-p(a),\quad P'=p
@answer 11
@feedback 12 | This subtracts in the opposite displayed-limit order. It would compute the integral from 2 to -1, the negative of the requested one.
@feedback 13 | Endpoint values of the integrand are not accumulated change. Here p(2)=9 and p(-1)=6; their difference 3 does not use a primitive.
@after I=P(b)-P(a),\quad P'=p
@wrong Identify the primitive and the displayed endpoint order before evaluating numbers.
@why The polynomial is continuous on [-1,2], so the Fundamental Theorem applies to any P with P'=p. It gives P(2)-P(-1). A constant in P would cancel, allowing the zero-constant choice.
@step 20 | Choose an expanded, collected primitive P with constant term zero.
@choice 23 | P(x)=3x^3-2x^2+x
@choice 22 | P(x)=3x^2-2x+1
@choice 21 | P(x)=x^3-x^2+x
@answer 21
@feedback 22 | This copies p and differentiates to 6x-2. A primitive must differentiate to 3x squared-2x+1.
@feedback 23 | Its derivative is 9x squared-4x+1. Raising powers must be accompanied by divisions 3/3 and -2/2.
@after P(x)=x^3-x^2+x
@wrong Check the primitive by differentiation before using its endpoint values.
@why Integration gives x cubed-x squared+x. Differentiating yields 3x squared-2x+1, exactly the original integrand for every real x, and P(0)=0.
@step 30 | Evaluate P(b) first and P(a) second, using the displayed upper and lower limits.
@choice 31 | (P(b),P(a))=(6,-3)
@choice 32 | (P(b),P(a))=(-3,6)
@choice 33 | (P(b),P(a))=(9,6)
@answer 31
@feedback 32 | These reverse the named entries. The upper displayed limit is b=2, so P(2) belongs first.
@feedback 33 | These are p(2) and p(-1), not P(2) and P(-1). Evaluate the integrated polynomial x cubed-x squared+x.
@after (P(b),P(a))=(6,-3)
@wrong Preserve signs when evaluating an odd power and then subtracting a square.
@why P(2)=8-4+2=6. P(-1)=-1-1-1=-3 because the cube and linear term are negative while the squared term is subtracted. The ordered pair is therefore (6,-3).
@step 40 | Show the numerical subtraction P(b)-P(a), followed by its exact value.
@choice 42 | I=-3-6=-9
@choice 41 | I=6-(-3)=9
@choice 43 | I=6+(-3)=3
@answer 41
@feedback 42 | This is lower-display value minus upper-display value and reverses the requested orientation.
@feedback 43 | The theorem subtracts P(a); it does not add it. Subtracting -3 adds 3, so 6-(-3)=9.
@after I=6-(-3)=9
@wrong Subtract the complete signed lower-endpoint value.
@why Upper minus lower gives 6-(-3)=6+3=9. Both endpoint evaluations come from a primitive whose derivative was checked against the original polynomial.
@step 50 | Check I directly from the original coefficients: show constant, linear and quadratic contributions in that order, then their sum.
@choice 53 | I=3-3+3=3
@choice 52 | I=3+3+9=15
@choice 51 | I=3-3+9=9
@answer 51
@feedback 52 | The linear coefficient is -2, so its contribution is (-2/2)(2 squared-(-1) squared)=-3, not +3.
@feedback 53 | The quadratic contribution is (3/3)(2 cubed-(-1) cubed)=8-(-1)=9, not 3.
@after I=3-3+9=9
@wrong Recompute each signed contribution from the original coefficient and ordered endpoints.
@why The constant contributes 1(2-(-1))=3. The linear term contributes (-2/2)(4-1)=-3. The quadratic contributes (3/3)(8-(-1))=9. Their sum is 9, independently confirming the endpoint calculation from the original data.
@end

@question prod03_calc_integration_q10 | A linear polynomial between limits
@template choices.v1
@version 1
@goal Calculate the signed definite integral from its original polynomial and ordered limits, and check the result.
@given p(x)=2x-3,\quad I=\int_{2}^{0}p(x)\,dx
@domain The polynomial is defined on R and continuous on the closed interval between the given limits. a is the lower displayed limit and b the upper displayed limit; retain their order. P is a primitive with P(0)=0.
@read prod03_calc_integration_r
@step 10 | Which primitive-based method respects the original displayed limits?
@choice 11 | I=P(b)-P(a),\quad P'=p
@choice 12 | I=P(a)-P(b),\quad P'=p
@choice 13 | I=p(b)-p(a),\quad P'=p
@answer 11
@feedback 12 | Here a=2 and b=0. Sorting them into numerical order changes the requested signed integral. The upper displayed value P(0) must come first.
@feedback 13 | This uses p(0)-p(2)=-3-1=-4. Integrand endpoint differences are not primitive endpoint differences.
@after I=P(b)-P(a),\quad P'=p
@wrong A lower displayed limit can be numerically larger than the upper displayed limit.
@why The polynomial is continuous on [0,2]. The theorem and reversal convention give the requested integral as P(0)-P(2), despite the decreasing limits. There is no absolute-value operation.
@step 20 | Choose an expanded, collected primitive P with constant term zero.
@choice 22 | P(x)=2x-3
@choice 21 | P(x)=x^2-3x
@choice 23 | P(x)=2x^2-3x
@answer 21
@feedback 22 | This differentiates to 2, not 2x-3, and its constant is -3. It is the integrand rather than a primitive.
@feedback 23 | Integrating 2x divides 2 by the new exponent 2. Keeping coefficient 2 gives derivative 4x-3 instead.
@after P(x)=x^2-3x
@wrong Build a valid primitive independently of which endpoint is numerically larger.
@why The linear term integrates to (2/2)x squared=x squared, and the constant to -3x. Differentiating P returns 2x-3 for all real x; P(0)=0.
@step 30 | Evaluate P(b) first and P(a) second for these displayed limits.
@choice 32 | (P(b),P(a))=(-2,0)
@choice 33 | (P(b),P(a))=(-3,1)
@choice 31 | (P(b),P(a))=(0,-2)
@answer 31
@feedback 32 | These entries are sorted by input size rather than their names. P(b)=P(0) must be first and P(a)=P(2) second.
@feedback 33 | These are values of p: p(0)=-3 and p(2)=1. The theorem uses P=x squared-3x instead.
@after (P(b),P(a))=(0,-2)
@wrong Evaluate the primitive at both named endpoints without changing their order.
@why At b=0, P(0)=0-0=0. At a=2, P(2)=4-6=-2. Hence the named ordered pair is (0,-2), not its reversal.
@step 40 | Show the numerical subtraction P(b)-P(a), followed by its exact value.
@choice 41 | I=0-(-2)=2
@choice 42 | I=-2-0=-2
@choice 43 | I=0-(-3)=3
@answer 41
@feedback 42 | This reverses the original integral and computes the value from 0 to 2. The requested limits run from 2 to 0.
@feedback 43 | The number -3 is p(0), not the required primitive value P(2). Evaluate P(2)=4-6=-2, then subtract that complete value.
@after I=0-(-2)=2
@wrong The orientation and the sign inside the lower-endpoint value are separate checks.
@why The requested value is 0-(-2)=2. In the opposite direction P(2)-P(0)=-2, so reversing that direction negates the value, confirming the positive result here.
@step 50 | Check from the original coefficients: show constant and linear contributions in that order, then their sum.
@choice 52 | I=-6+4=-2
@choice 51 | I=6-4=2
@choice 53 | I=6-2=4
@answer 51
@feedback 52 | Both contributions use the reversed endpoints. Retaining a=2 and b=0 gives -3(0-2)=6 and (2/2)(0 squared-2 squared)=-4.
@feedback 53 | The linear contribution is (2/2)(0-4)=-4, not -2. Divide the coefficient by 2 without halving the squared-endpoint difference again.
@after I=6-4=2
@wrong Carry the decreasing endpoint order into every original-coefficient contribution.
@why Directly, the constant contributes -3(0-2)=6 and the linear term contributes (2/2)(0-4)=-4. Their sum is 2. This confirms the same signed result without relying on the selected endpoint pair.
@end

@question prod03_calc_integration_q11 | A quadratic polynomial between limits
@template choices.v1
@version 1
@goal Calculate the signed definite integral from its original polynomial and ordered limits, and check the result.
@given p(x)=\frac{3}{2}x^2+x-4,\quad I=\int_{-1}^{1}p(x)\,dx
@domain The polynomial is defined on R and continuous on the closed interval between the given limits. a is the lower displayed limit and b the upper displayed limit; retain their order. P is a primitive with P(0)=0.
@read prod03_calc_integration_r
@step 10 | Which primitive-based method calculates this signed integral?
@choice 13 | I=p(b)-p(a),\quad P'=p
@choice 11 | I=P(b)-P(a),\quad P'=p
@choice 12 | I=P(a)-P(b),\quad P'=p
@answer 11
@feedback 12 | Lower minus upper reverses the requested interval. A signed integral must retain upper-display minus lower-display order even when the result is negative.
@feedback 13 | This uses p(1)-p(-1)=-3/2-(-7/2)=2. The theorem requires values of an antiderivative rather than values of p itself.
@after I=P(b)-P(a),\quad P'=p
@wrong Keep signed accumulation distinct from unsigned area or integrand endpoint differences.
@why The polynomial is continuous on [-1,1], so P'=p justifies I=P(1)-P(-1). Continuity and a verified derivative identity, not the sign of p, are the theorem's conditions here.
@step 20 | Choose an expanded, collected primitive P with constant term zero.
@choice 21 | P(x)=\frac{1}{2}x^3+\frac{1}{2}x^2-4x
@choice 22 | P(x)=\frac{3}{2}x^2+x-4
@choice 23 | P(x)=\frac{3}{2}x^3+x^2-4x
@answer 21
@feedback 22 | This copied integrand has derivative 3x+1, not (3/2)x squared+x-4, and does not have zero constant.
@feedback 23 | The derivative becomes (9/2)x squared+2x-4. Divide 3/2 by 3 and 1 by 2 when raising the powers.
@after P(x)=\frac{1}{2}x^3+\frac{1}{2}x^2-4x
@wrong Check the fractional primitive coefficients by differentiating them.
@why Integration gives ((3/2)/3)x cubed+(1/2)x squared-4x. Differentiation restores (3/2)x squared+x-4 coefficient by coefficient. The primitive has zero constant.
@step 30 | Evaluate P(b) first and P(a) second.
@choice 32 | (P(b),P(a))=(4,-3)
@choice 31 | (P(b),P(a))=(-3,4)
@choice 33 | (P(b),P(a))=(-\frac{3}{2},-\frac{7}{2})
@answer 31
@feedback 32 | This reverses P(1) and P(-1). The upper displayed limit b=1 belongs first.
@feedback 33 | These are p's endpoint values, not P's. The primitive has powers 3, 2 and 1 rather than 2, 1 and 0.
@after (P(b),P(a))=(-3,4)
@wrong At -1, the cubic changes sign while the squared contribution does not.
@why P(1)=1/2+1/2-4=-3. P(-1)=-1/2+1/2+4=4. These are values of the checked primitive in the required order.
@step 40 | Show the numerical subtraction P(b)-P(a), followed by its exact value.
@choice 43 | I=-3+4=1
@choice 42 | I=4-(-3)=7
@choice 41 | I=-3-4=-7
@answer 41
@feedback 42 | This reverses the subtraction and gives the positive opposite. The request is a signed integral from -1 to 1, not unsigned area.
@feedback 43 | The lower endpoint value 4 must be subtracted, not added. Upper minus lower is -3-4=-7.
@after I=-3-4=-7
@wrong A negative answer is permitted for a signed integral.
@why The theorem gives -3-4=-7. For -1<=x<=1, (3/2)x squared+x-4 is at most 3/2+1-4=-3/2, so the integrand is strictly negative throughout. The negative sign is consistent with increasing limits; it is not an arithmetic defect to remove.
@step 50 | Check from the original coefficients: show constant, linear and quadratic contributions in that order, then their sum.
@choice 51 | I=-8+0+1=-7
@choice 52 | I=-8+2+1=-5
@choice 53 | I=-8+0+2=-6
@answer 51
@feedback 52 | The linear contribution is (1/2)(1 squared-(-1) squared)=0, not 2. The negative and positive contributions of x cancel.
@feedback 53 | The quadratic contribution is ((3/2)/3)(1 cubed-(-1) cubed)=(1/2)(2)=1, not 2.
@after I=-8+0+1=-7
@wrong Retain cancellation of odd terms without discarding even or constant terms.
@why The constant contributes -4(1-(-1))=-8. The linear term contributes (1/2)(1-1)=0. The quadratic term contributes (1/2)(1-(-1))=1. Their sum -8+0+1=-7 independently confirms the original signed integral.
@end

@question prod03_calc_integration_q12 | An integral with specified endpoints
@template choices.v1
@version 1
@goal Calculate the signed definite integral from its original polynomial and ordered limits, and check the result.
@given p(x)=2x^3-x+\frac{1}{2},\quad I=\int_{-1}^{-1}p(x)\,dx
@domain The polynomial is defined on R and continuous on the closed interval between the given limits. a is the lower displayed limit and b the upper displayed limit; retain their order. P is a primitive with P(0)=0.
@read prod03_calc_integration_r
@step 10 | Choose an expanded, collected primitive P with constant term zero to verify endpoint evaluation.
@choice 12 | P(x)=2x^3-x+\frac{1}{2}
@choice 13 | P(x)=2x^4-x^2+\frac{1}{2}x
@choice 11 | P(x)=\frac{1}{2}x^4-\frac{1}{2}x^2+\frac{1}{2}x
@answer 11
@feedback 12 | This is the integrand, whose derivative is 6x squared-1. It is not a primitive of the original cubic.
@feedback 13 | Raising powers without dividing gives derivative 8x cubed-2x+1/2. Divide 2 by 4 and -1 by 2.
@after P(x)=\frac{1}{2}x^4-\frac{1}{2}x^2+\frac{1}{2}x
@wrong The derivative identity must hold on R even when the two integration limits coincide.
@why The primitive coefficients are 2/4=1/2, -1/2 and 1/2. Differentiating gives 2x cubed-x+1/2 everywhere and P(0)=0. The equal-limit convention will be consistent with this primitive's endpoint difference.
@step 20 | Evaluate the named endpoint pair (P(b),P(a)).
@choice 21 | (P(b),P(a))=(-\frac{1}{2},-\frac{1}{2})
@choice 22 | (P(b),P(a))=(\frac{1}{2},-\frac{1}{2})
@choice 23 | (P(b),P(a))=(-\frac{1}{2},\frac{1}{2})
@answer 21
@feedback 22 | Both named inputs are -1, so their primitive values must agree. The first value is 1/2-1/2-1/2=-1/2, not +1/2.
@feedback 23 | The second input is also -1. Its linear contribution is negative, giving -1/2 rather than +1/2.
@after (P(b),P(a))=(-\frac{1}{2},-\frac{1}{2})
@wrong Identical inputs cannot give different values of the same function.
@why At either endpoint, P(-1)=(1/2)(1)-(1/2)(1)+(1/2)(-1)=-1/2. Both positions therefore contain the same value, even though the primitive is not zero there.
@step 30 | Show the numerical subtraction P(b)-P(a), followed by its exact value.
@choice 32 | I=-\frac{1}{2}+(-\frac{1}{2})=-1
@choice 31 | I=-\frac{1}{2}-(-\frac{1}{2})=0
@choice 33 | I=\frac{1}{2}-(-\frac{1}{2})=1
@answer 31
@feedback 32 | This adds the two values instead of subtracting them. A value minus itself is zero, not twice the value.
@feedback 33 | The upper-display primitive value is -1/2, not +1/2. Both endpoints are the same original input.
@after I=-\frac{1}{2}-(-\frac{1}{2})=0
@wrong Equal limits give zero accumulation, not an assertion that the integrand vanishes.
@why Subtracting gives -1/2-(-1/2)=0. Independently, a=b=-1 makes every original-coefficient contribution contain b to power j+1 minus a to power j+1=0. The signed interval width is zero. In fact p(-1)=-2+1+1/2=-1/2 is nonzero, so the zero integral comes from equal limits, not a zero integrand.
@end
