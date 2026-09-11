@paths 1
@subject algebra | Algebra
@chapter topic_0004 | Polynomials and Rational Expressions

@lesson prod04_algebra_rational_equations_r | Solve rational equations on their original domain
@template lesson.v2
@block introduction | start | - | Restrictions are part of the answer
@prose A rational equation contains a quotient of polynomials. Solving it means finding every real input that makes both original sides defined and equal. Removing a denominator can simplify the arithmetic, but it cannot make a previously undefined input legal. Record restrictions first and retain them to the end.
@endblock
@block definition | terms | 1.1 | Rational expressions and their domains
@prose A polynomial is a sum of constant multiples of whole-number powers of x. In a rational expression N(x)/D(x), N is the numerator above the fraction bar and D is the denominator below it. The denominator must not be zero. The domain is the set of allowed real inputs; E denotes the excluded inputs that make at least one original denominator zero. Braces list set members without ordering or repetition. The real-number symbol denotes all real inputs, and set subtraction removes the listed exclusions.
@display domain
\text{domain}=\mathbb{R}\setminus E,\qquad D(x)\ne0
@prose A factor is an expression multiplied by another expression. To find exclusions, factor each denominator and set each variable factor equal to zero. For example, (2x-1)(x+2) is zero at 1/2 or -2. A numerator zero is allowed unless a denominator is also zero there. Cancelling a common factor simplifies values only where that factor is nonzero.
@prose Lambda denotes a chosen least common denominator, abbreviated LCD: a polynomial multiple containing every required denominator factor, with enough copies of repeated factors and any required constant denominator. C denotes the candidate values obtained from a cleared polynomial equation before checking restrictions; S denotes the final solution set. An empty set contains no solutions. An identity holds at every allowed input, which need not mean every real number.
@display sets
C=\{c_1,c_2\},\qquad S=\{r\},\qquad S=\varnothing,\qquad S=\mathbb{R}\setminus E
@prose L(t) and R(t) are the original left and right sides evaluated at t. V lists these values at each final solution in increasing order: left then right, repeated for a second solution if present. Do not evaluate an original fraction at an excluded input; division by zero has no real value.
@display original_check
V=(L(r_1),R(r_1),L(r_2),R(r_2)),\qquad r_1<r_2
@endblock
@block proposition | rule | 1.2 | Multiply every term by a common denominator
@prose On the original domain, Lambda is nonzero. Multiplying both whole sides by Lambda is then reversible by division by Lambda. Apply it to every additive term, including constants outside fractions. With denominators x-1 and 2, one LCD is 2(x-1). Multiplication gives a factor 2 on the numerator over x-1 and a factor x-1 on the numerator over 2. Ignoring either term changes the equation.
@display multiplication
L(x)=R(x)\quad\Longleftrightarrow\quad \Lambda L(x)=\Lambda R(x)\qquad(\Lambda\ne0)
@prose Expand the cleared equation and combine like powers. A linear equation with nonzero x coefficient has one candidate. A factored quadratic equal to zero requires at least one factor to be zero; solve both linear branches and count a repeated root once. The zero-product rule does not apply to a product equal to a nonzero number. Filter every candidate through the original exclusions before calling it a solution.
@endblock
@block proposition | condition | 1.3 | Cancellation, impossible results and identities
@prose Cancellation removes a common multiplied factor, not matching pieces of sums. For example, (x+2)(x-1)/((x+2)(x+3)) becomes (x-1)/(x+3) only where x is neither -2 nor -3. The shorter denominator does not restore -2. Multiplying by an LCD without recording its zeros may produce polynomial candidates that were never in the original domain.
@prose If the cleared difference is a nonzero constant, no allowed input solves it. If the cleared difference is identically zero, every original-domain input solves it because Lambda is nonzero there. A single successful substitution cannot prove such an identity; prove it by expansion, factor equality or exact cancellation valid throughout the domain.
@endblock
@block example | worked | 1.4 | A separate complete rational solve
@prose Solve the displayed equation over its original real domain.
@display worked_given
\frac{2x-3}{x+4}=-1
@help hint
@prose Find the zero of the denominator before changing the equation. Then multiply both sides by that denominator, distribute the negative sign on the right and collect the x terms.
@help answer
@display
S=\left\{-\frac{1}{3}\right\}
@help solution
@prose The denominator x+4 is zero at -4, so exclude -4. On all other inputs, multiply both sides by x+4: the left becomes 2x-3 and the right becomes -(x+4)=-x-4. Add x and then add 3 to both sides, obtaining 3x=-1. Division by nonzero 3 gives the sole candidate -1/3, which is not excluded.
@display worked_route
2x-3=-(x+4)\quad\Longrightarrow\quad 3x=-1\quad\Longrightarrow\quad x=-\frac{1}{3}
@prose In the original fraction at -1/3, the numerator is -2/3-3=-11/3 and the denominator is -1/3+4=11/3, which is nonzero. Their quotient is -1, equal to the original right side. The cleared linear equation has exactly one candidate and the reversible route retained it, so the solution set is complete.
@display worked_check
L\left(-\frac{1}{3}\right)=R\left(-\frac{1}{3}\right)=-1
@endblock
@block example | errors | 1.5 | Do not erase a restriction or a term
@prose A denominator zero is excluded even if the numerator also vanishes there: zero divided by zero is undefined, not zero or one. A common factor can cancel, but a term joined by addition cannot cancel across a fraction bar. When clearing multiple fractions, distribute the LCD across the complete sum. A candidate set belongs to the cleared equation; a solution set also obeys every original restriction. An identity with holes in its domain is not an identity on all real inputs.
@endblock
@block exercise | practice | 1.6 | From one denominator to exceptional outcomes
@prose Begin with a single variable denominator. Then handle factored denominators, negative and fractional roots, and canceled factors that still exclude candidates. The last problems ask you to select cancellation or an LCD, distinguish an identity from a contradiction, and clear every term in a sum. Each finite answer ends with original-expression evidence; an identity is proved on its full allowed domain.
@endblock
@block summary | summary | 1.7 | Preserve the original question and its source
@prose Record E first, transform only on the original domain, solve the entire cleared equation, filter candidates and verify the original sides. For an identity, state all allowed inputs with their exclusions; for impossibility, justify why no allowed input remains.
@prose Adapted from David Lippman and Melonie Rasmussen, Precalculus: An Investigation of Functions, Edition 2.3, Section 3.7 Exercises 5, 6, 7, 8, 11, 12, 13 and 16. Chapter 3 includes material remixed with permission from Carl Stitz and Jeff Zeager, College Algebra (2013). Source: https://www.opentextbookstore.com/precalc/2.3/Chapter%203.pdf
@prose Changes: graphing and intercept/asymptote tasks became explicit real rational equations; selected constants, factorizations, two denominator exponents and related additive/identity equations were changed. Teaching, choices and feedback were newly written. This adapted lesson and its twelve questions are licensed under Creative Commons Attribution-ShareAlike 4.0: https://creativecommons.org/licenses/by-sa/4.0/ . No endorsement by the source authors is implied.
@endblock
@practice prod04_algebra_rational_equations_q01
@practice prod04_algebra_rational_equations_q02
@practice prod04_algebra_rational_equations_q03
@practice prod04_algebra_rational_equations_q04
@practice prod04_algebra_rational_equations_q05
@practice prod04_algebra_rational_equations_q06
@practice prod04_algebra_rational_equations_q07
@practice prod04_algebra_rational_equations_q08
@practice prod04_algebra_rational_equations_q09
@practice prod04_algebra_rational_equations_q10
@practice prod04_algebra_rational_equations_q11
@practice prod04_algebra_rational_equations_q12
@end

@question prod04_algebra_rational_equations_q01 | Rational equation 1
@template choices.v1
@version 1
@goal Solve the original rational equation completely on its real domain.
@given \frac{2x-3}{x+4}=1
@domain Work over real x where every original denominator is nonzero. E lists excluded inputs, C lists polynomial candidates, and S is the complete solution set. Lambda denotes the chosen LCD. L(t) and R(t) evaluate the original sides at t. V lists L(r),R(r) for each solution r in increasing order.
@read prod04_algebra_rational_equations_r
@step 10 | Which set E contains exactly the inputs excluded by the original denominators?
@choice 11 | E=\{-4\}
@choice 12 | E=\{4\}
@choice 13 | E=\{\frac{3}{2}\}
@answer 11
@feedback 12 | Solving x+4=0 gives x=-4, not +4.
@feedback 13 | The numerator vanishes at 3/2, but its denominator is 11/2 there. Numerator zeros are not automatically excluded.
@after E=\{-4\}
@wrong Find zeros of the denominator, not the numerator.
@why The only denominator is x+4. It equals zero exactly at -4, so all other real inputs are initially allowed.
@step 20 | Multiply every term by the LCD x+4 and cancel denominators. Which equation results?
@choice 22 | 2x-3=1
@choice 21 | 2x-3=x+4
@choice 23 | 2x-3=x-4
@answer 21
@feedback 22 | The right side must also be multiplied: 1 times (x+4) is x+4.
@feedback 23 | Multiplying 1 by x+4 keeps its +4 sign.
@after 2x-3=x+4
@wrong Multiply both whole sides by the same nonzero-on-domain LCD.
@why On x not equal to -4, x+4 is nonzero. It cancels the left denominator and multiplies the right constant 1, giving 2x-3=x+4. Division by x+4 reverses this step on the original domain.
@step 30 | Solve the cleared equation and retain only allowed inputs.
@choice 32 | S=\{-7\}
@choice 33 | S=\{-4,7\}
@choice 31 | S=\{7\}
@answer 31
@feedback 32 | Subtract x and add 3 to get x=4+3=7. The result is positive.
@feedback 33 | The input -4 is excluded and is not a candidate of the cleared equation. Only 7 remains.
@after S=\{7\}
@wrong Solve the linear equation, then check its candidate against E.
@why Subtracting x gives x-3=4, and adding 3 gives x=7. This unique linear candidate differs from the excluded input -4, so it is the only possible solution.
@step 40 | Evaluate the original left and right sides at the solution.
@choice 41 | V=\left(1,1\right)
@choice 42 | V=\left(11,1\right)
@choice 43 | V=\left(-1,1\right)
@answer 41
@feedback 42 | Eleven is the numerator 2(7)-3, not the quotient. Divide by 7+4=11.
@feedback 43 | Both original numerator and denominator are positive 11, so the quotient is +1.
@after S=\{7\},\quad V=\left(1,1\right)
@wrong Use the original fraction, including its denominator.
@why At 7, the numerator is 14-3=11 and the denominator is 7+4=11, nonzero. Thus L(7)=11/11=1=R(7). The only allowed candidate checks, proving the complete answer.
@end

@question prod04_algebra_rational_equations_q02 | Rational equation 2
@template choices.v1
@version 1
@goal Solve the original rational equation completely on its real domain.
@given \frac{x-5}{3x-1}=2
@domain Work over real x where every original denominator is nonzero. E lists excluded inputs, C lists polynomial candidates, and S is the complete solution set. Lambda denotes the chosen LCD. L(t) and R(t) evaluate the original sides at t. V lists L(r),R(r) for each solution r in increasing order.
@read prod04_algebra_rational_equations_r
@step 10 | Which set contains exactly the original excluded inputs?
@choice 12 | E=\{1\}
@choice 11 | E=\{\frac{1}{3}\}
@choice 13 | E=\{5\}
@answer 11
@feedback 12 | The denominator equation is 3x=1, so divide by 3 to get 1/3.
@feedback 13 | Five is a numerator zero. Its denominator is 14, so it is not excluded.
@after E=\{\frac{1}{3}\}
@wrong Set the entire denominator 3x-1 equal to zero.
@why The denominator vanishes when 3x=1, namely x=1/3. No other input makes it zero.
@step 20 | Multiply every term by LCD 3x-1 and cancel denominators.
@choice 22 | x-5=2
@choice 23 | x-5=6x-1
@choice 21 | x-5=2(3x-1)
@answer 21
@feedback 22 | The constant 2 also receives the LCD: its new term is 2(3x-1).
@feedback 23 | Distribution multiplies both terms: 2(3x-1)=6x-2, not 6x-1.
@after x-5=2(3x-1)
@wrong Multiply the complete right side, including the signed constant in its new bracket.
@why Away from 1/3 the LCD is nonzero. Its multiplication cancels the left denominator and gives 2(3x-1) on the right. Expanding the right gives 6x-2.
@step 30 | Solve the cleared equation and filter its candidate through E.
@choice 31 | S=\{-\frac{3}{5}\}
@choice 32 | S=\{\frac{3}{5}\}
@choice 33 | S=\{\frac{1}{3}\}
@answer 31
@feedback 32 | From x-5=6x-2, subtract x and add 2: -3=5x, so the root is negative.
@feedback 33 | This is the excluded denominator zero, not a solution. The linear candidate is -3/5.
@after S=\{-\frac{3}{5}\}
@wrong Expand the bracket, solve the signed linear equation and retain the original restriction.
@why Expanding gives x-5=6x-2. Subtract x and add 2 to get -3=5x, hence x=-3/5. It differs from 1/3 and is the unique allowed candidate.
@step 40 | What are the original side values at the solution?
@choice 42 | V=\left(-\frac{28}{5},2\right)
@choice 41 | V=\left(2,2\right)
@choice 43 | V=\left(-2,2\right)
@answer 41
@feedback 42 | This reports only the numerator -3/5-5=-28/5. Divide it by the denominator -14/5.
@feedback 43 | Both numerator and denominator are negative, so their quotient is positive 2.
@after S=\{-\frac{3}{5}\},\quad V=\left(2,2\right)
@wrong Divide the evaluated original numerator by its nonzero denominator.
@why At -3/5, the numerator is -28/5 and the denominator is 3(-3/5)-1=-14/5. Their quotient is 2, equal to the original right side. The denominator is nonzero and the complete linear route leaves no other solution.
@end

@question prod04_algebra_rational_equations_q03 | Rational equation 3
@template choices.v1
@version 1
@goal Solve the original rational equation completely on its real domain.
@given \frac{4}{x-2}=2
@domain Work over real x where every original denominator is nonzero. E lists excluded inputs, C lists polynomial candidates, and S is the complete solution set. Lambda denotes the chosen LCD. L(t) and R(t) evaluate the original sides at t. V lists L(r),R(r) for each solution r in increasing order.
@read prod04_algebra_rational_equations_r
@step 10 | State exactly the original denominator exclusions.
@choice 12 | E=\{-2\}
@choice 13 | E=\{0\}
@choice 11 | E=\{2\}
@answer 11
@feedback 12 | Solving x-2=0 adds 2, giving +2 rather than -2.
@feedback 13 | At zero the denominator is -2, not zero. The excluded input is 2.
@after E=\{2\}
@wrong Find the input that makes x-2 vanish.
@why The denominator x-2 is zero exactly when x=2, so exclude that input before multiplying.
@step 20 | Multiply both sides by LCD x-2 and cancel the denominator.
@choice 21 | 4=2(x-2)
@choice 22 | 4=2
@choice 23 | 4=2x-2
@answer 21
@feedback 22 | The right constant must receive x-2 as well.
@feedback 23 | Multiplication gives 2x-4 because 2 multiplies both x and -2.
@after 4=2(x-2)
@wrong Multiply the entire right-hand constant by the LCD.
@why For x not equal to 2, multiplying by nonzero x-2 gives 4 on the left and 2(x-2) on the right. The cancellation is valid on precisely the original domain.
@step 30 | Solve the cleared equation and state the complete allowed set.
@choice 32 | S=\{2\}
@choice 31 | S=\{4\}
@choice 33 | S=\{0\}
@answer 31
@feedback 32 | Dividing 4=2(x-2) by 2 gives x-2=2, not x=2. Also 2 is excluded.
@feedback 33 | From x-2=2, add 2 to obtain 4, not zero.
@after S=\{4\}
@wrong Undo multiplication by 2 and then subtracting 2.
@why Divide by nonzero 2 to get 2=x-2, then add 2 to get x=4. This unique candidate is not the excluded input 2.
@step 40 | Evaluate both original sides at the allowed solution.
@choice 42 | V=\left(4,2\right)
@choice 43 | V=\left(2,4\right)
@choice 41 | V=\left(2,2\right)
@answer 41
@feedback 42 | The left fraction is 4 divided by 4-2=2, not its numerator alone.
@feedback 43 | Four is the cleared right value 2(4-2). The original right side is still 2.
@after S=\{4\},\quad V=\left(2,2\right)
@wrong Return to the original fraction and original constant.
@why At 4, the original denominator is 2 and L(4)=4/2=2. R(4)=2. The allowed candidate satisfies the original equation and the linear route proves completeness.
@end

@question prod04_algebra_rational_equations_q04 | Rational equation 4
@template choices.v1
@version 1
@goal Solve the original rational equation completely on its real domain.
@given \frac{5}{x+1}=-1
@domain Work over real x where every original denominator is nonzero. E lists excluded inputs, C lists polynomial candidates, and S is the complete solution set. Lambda denotes the chosen LCD. L(t) and R(t) evaluate the original sides at t. V lists L(r),R(r) for each solution r in increasing order.
@read prod04_algebra_rational_equations_r
@step 10 | Which inputs are excluded by the original denominator?
@choice 12 | E=\{1\}
@choice 11 | E=\{-1\}
@choice 13 | E=\{5\}
@answer 11
@feedback 12 | Solving x+1=0 gives -1, not +1.
@feedback 13 | The numerator constant 5 never vanishes and does not determine the domain. The denominator vanishes at -1.
@after E=\{-1\}
@wrong Set x+1, not the numerator, equal to zero.
@why The only denominator is zero at -1. Every other real input is in the original domain.
@step 20 | Multiply every term by LCD x+1 and cancel denominators.
@choice 22 | 5=-1
@choice 23 | 5=-x+1
@choice 21 | 5=-(x+1)
@answer 21
@feedback 22 | The right side also receives x+1, giving -(x+1).
@feedback 23 | The minus multiplies both terms: -(x+1)=-x-1, not -x+1.
@after 5=-(x+1)
@wrong Keep the negative multiplier on the complete bracket.
@why Since x+1 is nonzero on the original domain, it cancels on the left. On the right, -1 times (x+1) becomes -x-1.
@step 30 | Solve the cleared equation and retain the allowed candidate.
@choice 31 | S=\{-6\}
@choice 32 | S=\{6\}
@choice 33 | S=\{-4\}
@answer 31
@feedback 32 | From 5=-x-1, adding 1 gives 6=-x, so x=-6.
@feedback 33 | This would follow from the incorrect expansion -x+1. The actual expansion is -x-1.
@after S=\{-6\}
@wrong Distribute the negative sign before isolating x.
@why Expand to 5=-x-1, add 1 and multiply by -1 to obtain x=-6. It is not excluded, and the nonzero linear coefficient gives exactly one candidate.
@step 40 | Evaluate the two original sides at the solution.
@choice 42 | V=\left(1,-1\right)
@choice 41 | V=\left(-1,-1\right)
@choice 43 | V=\left(5,-1\right)
@answer 41
@feedback 42 | At -6 the denominator is -5. Positive 5 divided by -5 is -1.
@feedback 43 | This omitted division by the original denominator -5.
@after S=\{-6\},\quad V=\left(-1,-1\right)
@wrong Keep the signed original denominator during evaluation.
@why At -6, L(-6)=5/(-6+1)=5/(-5)=-1, equal to R(-6). The denominator is nonzero, so the unique candidate is a valid solution.
@end

@question prod04_algebra_rational_equations_q05 | Rational equation 5
@template choices.v1
@version 1
@goal Solve the original rational equation completely on its real domain.
@given \frac{x^2+2x-3}{(x-1)(x+1)}=2
@domain Work over real x where every original denominator is nonzero. E lists excluded inputs, C lists polynomial candidates, and S is the complete solution set. Lambda denotes the chosen LCD. L(t) and R(t) evaluate the original sides at t. V lists L(r),R(r) for each solution r in increasing order.
@read prod04_algebra_rational_equations_r
@step 10 | Which set contains every input excluded by the original factored denominator?
@choice 12 | E=\{-1\}
@choice 13 | E=\{-3,1\}
@choice 11 | E=\{-1,1\}
@answer 11
@feedback 12 | This omits x=1, where the original factor x-1 is zero. A later cancellation cannot restore it.
@feedback 13 | The value -3 comes from a numerator factor. The original denominator factors vanish at -1 and 1.
@after E=\{-1,1\}
@wrong Set each original denominator factor to zero.
@why The factor x-1 vanishes at 1 and x+1 at -1. Exclude both before simplifying the numerator or cancelling anything.
@step 20 | Multiply every term by LCD (x-1)(x+1) and cancel denominators.
@choice 21 | x^2+2x-3=2(x-1)(x+1)
@choice 22 | x^2+2x-3=2
@choice 23 | x^2+2x-3=2(x+1)
@answer 21
@feedback 22 | The right constant must receive the entire LCD, not remain 2.
@feedback 23 | This omits the factor x-1 from the right-side multiplication.
@after x^2+2x-3=2(x-1)(x+1)
@wrong Use both denominator factors on the constant side.
@why On the domain excluding -1 and 1, the LCD is nonzero. Multiplication leaves the numerator on the left and 2(x-1)(x+1) on the right. Expanding the right gives 2x squared minus 2.
@step 30 | Solve the cleared polynomial before filtering. Which candidate set C is complete?
@choice 32 | C=\{-1,1\}
@choice 31 | C=\{1\}
@choice 33 | C=\{-1\}
@answer 31
@feedback 32 | Collecting gives (x-1) squared equal to zero, not x squared equal to 1. Its repeated root is only 1.
@feedback 33 | The repeated factor is x-1; setting it to zero gives +1.
@after C=\{1\}
@wrong Collect all terms and count a repeated root once.
@why From x squared+2x-3=2x squared-2, moving all terms and changing the overall sign gives x squared-2x+1=0. This is (x-1) squared=0, whose only candidate is 1.
@step 40 | Apply the original restrictions to C. What is the complete solution set?
@choice 42 | S=\{1\}
@choice 43 | S=\{-1,1\}
@choice 41 | S=\varnothing
@answer 41
@feedback 42 | At 1 the original denominator is (1-1)(1+1)=0. The numerator is also zero, but 0/0 is undefined.
@feedback 43 | Both listed inputs make the original denominator zero. Neither can be a solution.
@after S=\varnothing
@wrong A polynomial candidate is a solution only if the original fraction is defined there.
@why The only candidate is 1, where the denominator is 0 times 2=0 and the numerator is 1+2-3=0. The original left side is undefined, not 2. No candidate survives; completeness of the repeated-square solve proves that the original solution set is empty.
@end

@question prod04_algebra_rational_equations_q06 | Rational equation 6
@template choices.v1
@version 1
@goal Solve the original rational equation completely on its real domain.
@given \frac{x^2-x-6}{(x-2)(x+2)}=2
@domain Work over real x where every original denominator is nonzero. E lists excluded inputs, C lists polynomial candidates, and S is the complete solution set. Lambda denotes the chosen LCD. L(t) and R(t) evaluate the original sides at t. V lists L(r),R(r) for each solution r in increasing order.
@read prod04_algebra_rational_equations_r
@step 10 | State exactly the excluded inputs from the original denominator.
@choice 11 | E=\{-2,2\}
@choice 12 | E=\{2\}
@choice 13 | E=\{-2,3\}
@answer 11
@feedback 12 | The original factor x+2 also vanishes, at -2. Keep that exclusion even if it later cancels.
@feedback 13 | Three is a numerator zero, not a denominator zero. The second excluded input is 2.
@after E=\{-2,2\}
@wrong Use the original denominator factors, not canceled expressions.
@why The factors x-2 and x+2 give excluded inputs 2 and -2. The original domain is all other real inputs.
@step 20 | Multiply each side by LCD (x-2)(x+2) and cancel denominators.
@choice 22 | x^2-x-6=2(x-2)
@choice 21 | x^2-x-6=2(x-2)(x+2)
@choice 23 | x^2-x-6=2x^2-4
@answer 21
@feedback 22 | This omits the right-side factor x+2.
@feedback 23 | The denominator product is x squared-4, so multiplying by 2 gives 2x squared-8, not 2x squared-4.
@after x^2-x-6=2(x-2)(x+2)
@wrong Multiply the complete denominator product by the right constant.
@why The LCD is nonzero on the original domain. It cancels on the left and gives 2(x squared-4)=2x squared-8 on the right.
@step 30 | Solve the resulting polynomial before applying the original exclusions.
@choice 32 | C=\{1\}
@choice 33 | C=\{-1,2\}
@choice 31 | C=\{-2,1\}
@answer 31
@feedback 32 | The cleared polynomial factors as (x+2)(x-1), so -2 is also a polynomial candidate before filtering.
@feedback 33 | The factors x+2 and x-1 give -2 and +1, not their opposite signs.
@after C=\{-2,1\}
@wrong Include every zero-product branch in the candidate set.
@why Rearranging x squared-x-6=2x squared-8 gives x squared+x-2=0. Its factors are (x+2)(x-1), since the cross terms -x+2x give x and the constant product is -2. Therefore C contains -2 and 1.
@step 40 | Filter the candidates using the original denominator. Which solution set remains?
@choice 41 | S=\{1\}
@choice 42 | S=\{-2,1\}
@choice 43 | S=\varnothing
@answer 41
@feedback 42 | At -2 the original denominator is (-4)(0)=0. Reject that candidate even though its numerator is also zero.
@feedback 43 | The candidate 1 has denominator (-1)(3)=-3, nonzero, so it survives.
@after S=\{1\}
@wrong Test both candidates against the original factors.
@why At -2 the denominator vanishes, so that candidate is extraneous to the rational equation. At 1 the denominator is -3, so it is allowed. All polynomial candidates have now been considered.
@step 50 | Evaluate both original sides at the surviving solution.
@choice 52 | V=\left(-6,2\right)
@choice 51 | V=\left(2,2\right)
@choice 53 | V=\left(-2,2\right)
@answer 51
@feedback 52 | The numerator at 1 is -6, but it must be divided by the denominator -3.
@feedback 53 | The original numerator and denominator are both negative, giving positive 2.
@after S=\{1\},\quad V=\left(2,2\right)
@wrong Use the unreduced original numerator and denominator.
@why At 1, the numerator is 1-1-6=-6 and the denominator is (1-2)(1+2)=-3. Their quotient is 2, equal to the original right side. The rejected input remains excluded, so this singleton is complete.
@end

@question prod04_algebra_rational_equations_q07 | Rational equation 7
@template choices.v1
@version 1
@goal Solve the original rational equation completely on its real domain.
@given \frac{5-x}{(2x+1)(x+3)}=\frac{1}{3}
@domain Work over real x where every original denominator is nonzero. E lists excluded inputs, C lists polynomial candidates, and S is the complete solution set. Lambda denotes the chosen LCD. L(t) and R(t) evaluate the original sides at t. V lists L(r),R(r) for each solution r in increasing order.
@read prod04_algebra_rational_equations_r
@step 10 | Identify every original denominator-zero input.
@choice 12 | E=\{-3,\frac{1}{2}\}
@choice 13 | E=\{-3\}
@choice 11 | E=\{-3,-\frac{1}{2}\}
@answer 11
@feedback 12 | Solving 2x+1=0 gives x=-1/2, not +1/2.
@feedback 13 | The factor 2x+1 contributes the additional exclusion -1/2.
@after E=\{-3,-\frac{1}{2}\}
@wrong Set both variable denominator factors equal to zero; the constant 3 is already nonzero.
@why The variable factors vanish at -1/2 and -3. The other denominator is the fixed nonzero number 3, so it adds no excluded input.
@step 20 | Multiply every term by LCD 3(2x+1)(x+3) and cancel denominators.
@choice 21 | 3(5-x)=(2x+1)(x+3)
@choice 22 | 5-x=(2x+1)(x+3)
@choice 23 | 3(5-x)=3(2x+1)(x+3)
@answer 21
@feedback 22 | After cancelling the left variable factors, the constant factor 3 still multiplies 5-x.
@feedback 23 | On the right the LCD's factor 3 cancels the denominator 3. Only (2x+1)(x+3) remains.
@after 3(5-x)=(2x+1)(x+3)
@wrong Cancel only the factors present in each term's own denominator.
@why The left numerator receives factor 3, giving 15-3x. The right numerator 1 receives (2x+1)(x+3), giving 2x squared+7x+3. The LCD is nonzero on the recorded domain, so the transformation is reversible there.
@step 30 | Collect to a positive-leading polynomial equal to zero, then express that whole polynomial as two linear factors.
@choice 32 | (2x+2)(x-6)=0
@choice 31 | (2x-2)(x+6)=0
@choice 33 | (2x-6)(x+2)=0
@answer 31
@feedback 32 | These factors give 2x squared-10x-12. The collected middle term is +10x.
@feedback 33 | These factors give 2x squared-2x-12, not the required +10x term.
@after (2x-2)(x+6)=0
@wrong Expand and collect before checking both cross terms of the factors.
@why From 15-3x=2x squared+7x+3, collection gives 2x squared+10x-12=0. The proposed product expands to 2x squared+12x-2x-12, exactly that polynomial.
@step 40 | Solve both factor branches and retain all original-domain roots.
@choice 42 | S=\{1\}
@choice 43 | S=\{-6,-1\}
@choice 41 | S=\{-6,1\}
@answer 41
@feedback 42 | The factor x+6 gives the additional allowed root -6.
@feedback 43 | From 2x-2=0, divide 2 by 2 to obtain +1, not -1.
@after S=\{-6,1\}
@wrong Solve both linear factors and compare with -3 and -1/2.
@why The branches give x=1 and x=-6. Neither equals -3 or -1/2, so both are allowed. The factor identity and complete zero-product branches prove that there are no others.
@step 50 | Evaluate both original sides at each solution in increasing order.
@choice 51 | V=\left(\frac{1}{3},\frac{1}{3},\frac{1}{3},\frac{1}{3}\right)
@choice 52 | V=\left(11,\frac{1}{3},4,\frac{1}{3}\right)
@choice 53 | V=\left(3,\frac{1}{3},3,\frac{1}{3}\right)
@answer 51
@feedback 52 | These are only the numerators. Divide 11 by 33 and 4 by 12.
@feedback 53 | This reverses each quotient. The original numerator is over the denominator, yielding 1/3, not 3.
@after S=\{-6,1\},\quad V=\left(\frac{1}{3},\frac{1}{3},\frac{1}{3},\frac{1}{3}\right)
@wrong Evaluate both denominator factors at each solution.
@why At -6 the numerator is 11 and the denominator is (-11)(-3)=33, giving 1/3. At 1 the numerator is 4 and denominator is (3)(4)=12, again giving 1/3. Both denominators are nonzero and both right values are 1/3.
@end

@question prod04_algebra_rational_equations_q08 | Rational equation 8
@template choices.v1
@version 1
@goal Solve the original rational equation completely on its real domain.
@given \frac{2x^2+x-1}{x-4}=-2
@domain Work over real x where every original denominator is nonzero. E lists excluded inputs, C lists polynomial candidates, and S is the complete solution set. Lambda denotes the chosen LCD. L(t) and R(t) evaluate the original sides at t. V lists L(r),R(r) for each solution r in increasing order.
@read prod04_algebra_rational_equations_r
@step 10 | Identify the complete set of original excluded inputs.
@choice 11 | E=\{4\}
@choice 12 | E=\{-4\}
@choice 13 | E=\{-1,\frac{1}{2}\}
@answer 11
@feedback 12 | The denominator is x-4, whose zero is +4.
@feedback 13 | These are numerator zeros from (2x-1)(x+1), not denominator zeros.
@after E=\{4\}
@wrong Restrictions come from the original denominator.
@why The denominator x-4 vanishes only at 4. The numerator may vanish without making the fraction undefined.
@step 20 | Multiply every term by LCD x-4 and cancel the denominator.
@choice 22 | 2x^2+x-1=-2
@choice 21 | 2x^2+x-1=-2(x-4)
@choice 23 | 2x^2+x-1=-2x-8
@answer 21
@feedback 22 | The constant -2 must also receive the factor x-4.
@feedback 23 | Distributing -2 gives -2x+8, since (-2)(-4)=+8.
@after 2x^2+x-1=-2(x-4)
@wrong Multiply the complete right side and retain both signs.
@why On x not equal to 4, multiplying by nonzero x-4 leaves the original numerator on the left and -2x+8 on the right.
@step 30 | Collect to a positive-leading polynomial equal to zero and factor its whole left side into linear factors.
@choice 32 | (2x+3)(x-3)=0
@choice 33 | (2x-1)(x+9)=0
@choice 31 | (2x-3)(x+3)=0
@answer 31
@feedback 32 | These factors give middle term -3x, not +3x.
@feedback 33 | These factors give middle term 17x and constant -9. The required middle term is only 3x.
@after (2x-3)(x+3)=0
@wrong Collect both sides before selecting factors.
@why Moving -2x+8 left gives 2x squared+3x-9=0. The product (2x-3)(x+3) expands to 2x squared+6x-3x-9, which combines correctly.
@step 40 | Solve every factor branch and filter using the original exclusion.
@choice 41 | S=\{-3,\frac{3}{2}\}
@choice 42 | S=\{-3,3\}
@choice 43 | S=\{-\frac{3}{2},3\}
@answer 41
@feedback 42 | The factor 2x-3 gives 2x=3, so divide by 2 to obtain 3/2.
@feedback 43 | These signs are reversed. The factors give x=3/2 and x=-3.
@after S=\{-3,\frac{3}{2}\}
@wrong Keep the coefficient 2 when solving the first factor.
@why The branches yield x=3/2 or x=-3. Neither is 4, so neither is excluded. Both branches are necessary and complete.
@step 50 | Evaluate the original sides at the allowed roots, in increasing order.
@choice 52 | V=\left(14,-2,5,-2\right)
@choice 51 | V=\left(-2,-2,-2,-2\right)
@choice 53 | V=\left(2,-2,2,-2\right)
@answer 51
@feedback 52 | These are numerator values. Divide 14 by -7 and 5 by -5/2.
@feedback 53 | Both denominators are negative while the numerators are positive, giving -2 rather than +2.
@after S=\{-3,\frac{3}{2}\},\quad V=\left(-2,-2,-2,-2\right)
@wrong Substitute into the original quadratic numerator and original linear denominator.
@why At -3, the numerator is 18-3-1=14 and denominator -3-4=-7, giving -2. At 3/2, the numerator is 2(9/4)+3/2-1=5 and denominator 3/2-4=-5/2, giving -2. Both original right values are -2 and both denominators are nonzero.
@end

@question prod04_algebra_rational_equations_q09 | Rational equation 9
@template choices.v1
@version 1
@goal Solve the original rational equation completely on its real domain.
@given \frac{(x-1)(x+3)}{(x-1)(x+1)}=\frac{3}{2}
@domain Work over real x where every original denominator is nonzero. E lists excluded inputs, C lists polynomial candidates, and S is the complete solution set. Lambda denotes the chosen LCD. L(t) and R(t) evaluate the original sides at t. V lists L(r),R(r) for each solution r in increasing order.
@read prod04_algebra_rational_equations_r
@step 10 | Determine all original excluded inputs before choosing a simplification.
@choice 12 | E=\{-1\}
@choice 11 | E=\{-1,1\}
@choice 13 | E=\{-3,1\}
@answer 11
@feedback 12 | The original factor x-1 excludes 1 even if that factor can later cancel.
@feedback 13 | The numerator factor x+3 does not exclude -3. The denominator's second factor is x+1.
@after E=\{-1,1\}
@wrong Inspect the original denominator before canceling factors.
@why The original denominator vanishes at 1 or -1. Record both restrictions now; the fixed right denominator 2 adds none.
@step 20 | Choose safe common-factor cancellation: reduce the left fraction fully while leaving the right side unchanged.
@choice 22 | \frac{x-1}{x+1}=\frac{3}{2}
@choice 23 | x+3=\frac{3}{2}
@choice 21 | \frac{x+3}{x+1}=\frac{3}{2}
@answer 21
@feedback 22 | The common factor is x-1. Cancelling it leaves x+3 in the numerator, not x-1.
@feedback 23 | Only x-1 cancels. The denominator x+1 remains.
@after \frac{x+3}{x+1}=\frac{3}{2}
@wrong Cancel an identical multiplied factor and retain every original exclusion.
@why On the original domain, x-1 is nonzero and can cancel from numerator and denominator. The reduced equation is (x+3)/(x+1)=3/2, but both original restrictions -1 and 1 still apply.
@step 30 | Multiply every term of the reduced equation by its LCD 2(x+1).
@choice 31 | 2(x+3)=3(x+1)
@choice 32 | x+3=3(x+1)
@choice 33 | 2(x+3)=3
@answer 31
@feedback 32 | After the left denominator cancels, the LCD factor 2 still multiplies x+3.
@feedback 33 | After the right denominator 2 cancels, x+1 still multiplies the numerator 3.
@after 2(x+3)=3(x+1)
@wrong Cancel each term's denominator from the complete LCD.
@why The reduced LCD is nonzero on the original domain. Its left product is 2(x+3)=2x+6 and its right product is 3(x+1)=3x+3.
@step 40 | Solve the cleared equation and retain exactly the original-domain solutions.
@choice 42 | S=\{-3\}
@choice 41 | S=\{3\}
@choice 43 | S=\{1,3\}
@answer 41
@feedback 42 | From 2x+6=3x+3, subtract 2x and then 3 to get x=3.
@feedback 43 | One is still excluded by the original denominator and is not a root of the cleared linear equation.
@after S=\{3\}
@wrong Solve the linear equation without restoring a canceled exclusion.
@why Collection gives 6=x+3, hence x=3. It is neither -1 nor 1. The cancellation and clearing were reversible on that original domain, so this is the only possible solution.
@step 50 | Check the original factored fraction, not only the shortened one.
@choice 52 | V=\left(12,\frac{3}{2}\right)
@choice 53 | V=\left(\frac{2}{3},\frac{3}{2}\right)
@choice 51 | V=\left(\frac{3}{2},\frac{3}{2}\right)
@answer 51
@feedback 52 | Twelve is the original numerator at 3. Divide by the original denominator 8.
@feedback 53 | This reverses the original quotient: numerator 12 divided by denominator 8 is 3/2.
@after S=\{3\},\quad V=\left(\frac{3}{2},\frac{3}{2}\right)
@wrong Substitute into both original products and confirm a nonzero denominator.
@why At 3, the original numerator is (2)(6)=12 and denominator is (2)(4)=8, nonzero. Their quotient is 3/2, equal to the original right side. No canceled exclusion has been reintroduced.
@end

@question prod04_algebra_rational_equations_q10 | Rational equation 10
@template choices.v1
@version 1
@goal Solve the original rational equation completely on its real domain.
@given \frac{(x-3)(x+2)}{(x-2)(x+2)}=\frac{x-3}{x-2}
@domain Work over real x where every original denominator is nonzero. E lists excluded inputs, C lists polynomial candidates, and S is the complete solution set. Lambda denotes the chosen LCD. L(t) and R(t) evaluate the original sides at t. V lists L(r),R(r) for each solution r in increasing order.
@read prod04_algebra_rational_equations_r
@step 10 | Find every original excluded input before combining or canceling expressions.
@choice 11 | E=\{-2,2\}
@choice 12 | E=\{2\}
@choice 13 | E=\{-2,2,3\}
@answer 11
@feedback 12 | The left original denominator also contains x+2, excluding -2.
@feedback 13 | At 3 both denominators are nonzero. A zero numerator is permitted.
@after E=\{-2,2\}
@wrong Include factors from both original denominators, without adding numerator zeros.
@why The left denominator requires x not equal to 2 or -2; the right repeats the restriction at 2. Their union is exactly the two-element excluded set.
@step 20 | Select the LCD method multiplier containing each needed variable factor once and no extra constant multiple.
@choice 22 | \Lambda=x-2
@choice 21 | \Lambda=(x-2)(x+2)
@choice 23 | \Lambda=(x-2)(x+2)^2
@answer 21
@feedback 22 | This does not contain x+2, so it cannot clear the full original left denominator.
@feedback 23 | This is a valid common multiple but not the requested least one: a second copy of x+2 is unnecessary.
@after \Lambda=(x-2)(x+2)
@wrong Use each required factor to its highest original multiplicity.
@why Both denominators divide (x-2)(x+2). Only one copy of each factor is needed. This multiplier is nonzero exactly on the original domain.
@step 30 | Multiply both original sides by that LCD; leave the resulting numerator products factored.
@choice 32 | x-3=x-3
@choice 33 | (x-3)(x+2)=x-3
@choice 31 | (x-3)(x+2)=(x-3)(x+2)
@answer 31
@feedback 32 | This equality is valid on the original domain after additional cancellation, but it is not the immediate factored result of multiplying by the selected LCD.
@feedback 33 | On the right, cancelling x-2 leaves the LCD factor x+2 multiplying x-3.
@after (x-3)(x+2)=(x-3)(x+2)
@wrong Cancel each denominator from the LCD and keep the remaining numerator products.
@why On the left the complete denominator cancels, leaving (x-3)(x+2). On the right only x-2 cancels, so the remaining x+2 multiplies x-3. Both sides are the identical polynomial x squared-x-6.
@step 40 | State the complete original solution set, using the identity only where the original equation is defined.
@choice 41 | S=\mathbb{R}\setminus\{-2,2\}
@choice 42 | S=\mathbb{R}
@choice 43 | S=\mathbb{R}\setminus\{2\}
@answer 41
@feedback 42 | The original equation remains undefined at -2 and 2. An identity after clearing does not restore them.
@feedback 43 | The canceled factor x+2 still excludes -2 in the original left fraction.
@after S=\mathbb{R}\setminus\{-2,2\}
@wrong Retain the original domain in an identity's solution set.
@why The two cleared products are identically equal by expansion, not merely at sample inputs. For every x other than -2 and 2, the LCD is nonzero, so dividing that polynomial identity by it proves the original equality. At either excluded input an original denominator is zero. Therefore precisely the original domain is the solution set.
@end

@question prod04_algebra_rational_equations_q11 | Rational equation 11
@template choices.v1
@version 1
@goal Solve the original rational equation completely on its real domain.
@given \frac{2x-3}{x+4}=2
@domain Work over real x where every original denominator is nonzero. E lists excluded inputs, C lists polynomial candidates, and S is the complete solution set. Lambda denotes the chosen LCD. L(t) and R(t) evaluate the original sides at t. V lists L(r),R(r) for each solution r in increasing order.
@read prod04_algebra_rational_equations_r
@step 10 | Identify the exact set of original denominator exclusions.
@choice 12 | E=\{4\}
@choice 13 | E=\{\frac{3}{2}\}
@choice 11 | E=\{-4\}
@answer 11
@feedback 12 | Solving x+4=0 gives -4, not +4.
@feedback 13 | The numerator's zero is allowed because its denominator is nonzero. Restrictions come from x+4.
@after E=\{-4\}
@wrong Find the zero of the original denominator.
@why The original expression is defined for every real x except -4.
@step 20 | Multiply both sides by LCD x+4 and cancel denominators.
@choice 21 | 2x-3=2(x+4)
@choice 22 | 2x-3=2
@choice 23 | 2x-3=2x+4
@answer 21
@feedback 22 | The right constant 2 must also be multiplied by x+4.
@feedback 23 | Distribution gives 2x+8, since 2 multiplies the constant 4 as well.
@after 2x-3=2(x+4)
@wrong Multiply every term on both original sides.
@why On the original domain x+4 is nonzero. The multiplied sides are 2x-3 and 2x+8.
@step 30 | Subtract the expanded right side from the left and combine completely, leaving zero on the right.
@choice 32 | 0=0
@choice 31 | -11=0
@choice 33 | 11=0
@answer 31
@feedback 32 | The x terms cancel, but the constants do not: -3-8=-11.
@feedback 33 | This is also an impossible equality, but it negates the requested left-minus-right result. The stated subtraction gives -3-8=-11.
@after -11=0
@wrong Cancel equal variable terms while retaining their unequal constants.
@why Subtracting 2x+8 from 2x-3 gives 2x-2x-3-8=-11. Thus the cleared equation requires -11=0, which no input can make true.
@step 40 | What complete solution set follows for the original rational equation?
@choice 42 | S=\{-4\}
@choice 43 | S=\mathbb{R}\setminus\{-4\}
@choice 41 | S=\varnothing
@answer 41
@feedback 42 | The input -4 is undefined in the original equation and cannot repair a contradiction.
@feedback 43 | Cancellation of x terms does not imply an identity; the remaining constants are unequal.
@after S=\varnothing
@wrong Distinguish a nonzero constant difference from an identically zero difference.
@why Every allowed original solution would satisfy -11=0 after multiplication by the nonzero LCD, which is impossible. Equivalently, the original left-minus-right difference is -11/(x+4), nonzero at every allowed input. Hence the solution set is empty without relying on sample substitutions.
@end

@question prod04_algebra_rational_equations_q12 | Rational equation 12
@template choices.v1
@version 1
@goal Solve the original rational equation completely on its real domain.
@given \frac{x-5}{3x-1}+\frac{1}{3x-1}=\frac{1}{2}
@domain Work over real x where every original denominator is nonzero. E lists excluded inputs, C lists polynomial candidates, and S is the complete solution set. Lambda denotes the chosen LCD. L(t) and R(t) evaluate the original sides at t. V lists L(r),R(r) for each solution r in increasing order.
@read prod04_algebra_rational_equations_r
@step 10 | List all inputs excluded by any original denominator.
@choice 12 | E=\{\frac{1}{3},2\}
@choice 11 | E=\{\frac{1}{3}\}
@choice 13 | E=\{5\}
@answer 11
@feedback 12 | The constant denominator 2 is never zero; it does not exclude x=2.
@feedback 13 | Five is a zero of one numerator, not a denominator. Solve 3x-1=0 instead.
@after E=\{\frac{1}{3}\}
@wrong Repeated denominator factors give the same exclusion once.
@why Both left denominators vanish only at 1/3. The right denominator is the fixed nonzero number 2, so the excluded set has one member.
@step 20 | Choose the LCD multiplier that clears every original fraction without unnecessary factors.
@choice 22 | \Lambda=3x-1
@choice 23 | \Lambda=2
@choice 21 | \Lambda=2(3x-1)
@answer 21
@feedback 22 | This misses the fixed denominator 2 on the right.
@feedback 23 | This cannot cancel the variable denominator 3x-1 on either left fraction.
@after \Lambda=2(3x-1)
@wrong Include the constant denominator and the variable factor once.
@why The denominators are 3x-1, another copy of 3x-1 and 2. Their LCD is 2(3x-1), not a square of the repeated factor. It is nonzero on the recorded domain.
@step 30 | Apply that LCD to every additive term and cancel each denominator.
@choice 31 | 2(x-5)+2=3x-1
@choice 32 | 2(x-5)+1=3x-1
@choice 33 | 2(x-5)+2=2(3x-1)
@answer 31
@feedback 32 | The second left fraction also receives the LCD: 2(3x-1) times 1/(3x-1) gives 2, not 1.
@feedback 33 | On the right the denominator 2 cancels the LCD factor 2, leaving only 3x-1.
@after 2(x-5)+2=3x-1
@wrong Multiply the complete sum, not just its first fraction.
@why The three original terms become 2(x-5), 2 and 3x-1 respectively. The left expands to 2x-10+2=2x-8. Every original term has received the same nonzero-on-domain multiplier.
@step 40 | Solve the resulting equation and retain exactly the allowed solutions.
@choice 42 | S=\{7\}
@choice 41 | S=\{-7\}
@choice 43 | S=\{-8\}
@answer 41
@feedback 42 | From 2x-8=3x-1, subtract 2x and add 1 to get -7=x, not +7.
@feedback 43 | After subtracting 2x the equation is -8=x-1. Add 1 to both sides before reading x.
@after S=\{-7\}
@wrong Collect the original cleared equation's constants with their signs.
@why Expanding gives 2x-8=3x-1. Subtract 2x and add 1 to get x=-7. This unique candidate differs from 1/3, so it is allowed.
@step 50 | Evaluate the complete original sum and right side at the solution.
@choice 52 | V=\left(\frac{6}{11},\frac{1}{2}\right)
@choice 53 | V=\left(\frac{13}{22},\frac{1}{2}\right)
@choice 51 | V=\left(\frac{1}{2},\frac{1}{2}\right)
@answer 51
@feedback 52 | This includes only the first original fraction. Add the second value -1/22 to 6/11.
@feedback 53 | The second denominator is -22, so its contribution is -1/22, not +1/22.
@after S=\{-7\},\quad V=\left(\frac{1}{2},\frac{1}{2}\right)
@wrong Evaluate both original fractions before adding them.
@why At -7 the shared denominator is 3(-7)-1=-22, nonzero. The first fraction is (-12)/(-22)=6/11 and the second is -1/22. Their sum is 12/22-1/22=11/22=1/2, equal to the original right side. The complete linear solve proves there are no other solutions.
@end
