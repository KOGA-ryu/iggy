@paths 1
@subject algebra | Algebra
@chapter topic_0004 | Polynomials and Rational Expressions

@lesson prod03_algebra_quadratic_roots_r | Find every real quadratic root
@template lesson.v2
@block introduction | start | - | Solve and check the original equation
@prose A solution makes the original left and right sides equal. A quadratic equation can have two distinct real roots, one repeated real root, or no real roots. Rearrange with reversible operations, select a method suited to the expression, retain every branch, and check the original equation rather than only its rearranged version.
@endblock
@block definition | terms | 1.1 | Coefficients, roots and exact sets
@prose A quadratic has standard form ax squared plus bx plus c equals zero, where a, b and c are fixed real coefficients and a is nonzero. The letters a, b, c follow descending powers of x; a missing term has coefficient zero. Monic means that the highest-power coefficient is 1, as in the linear expression x-1. A root is a real input satisfying the equation. A factor is an expression multiplied by another expression. Expanding factors uses distribution: each term in one factor multiplies each term in the other.
@display standard
ax^2+bx+c=0,\qquad a\ne0
@prose S denotes the complete solution set. Braces list its distinct members; a repeated root appears once. The empty-set symbol means there are no real solutions. The principal square root of a nonnegative number is its nonnegative square root. The plus-or-minus sign asks for both values, except that they coincide when the square root is zero. For example, the square root of 12 is twice the square root of 3 because 12=4 times 3.
@display notation
S=\{r_1,r_2\},\qquad S=\{r\},\qquad S=\varnothing,\qquad \sqrt{12}=2\sqrt{3}
@prose L(t) and R(t) mean the original left and right expressions evaluated at t. At the final check, V is an ordered list: the left value then the right value at the smallest root, followed by the same two values at the next root if there is one. Equal adjacent entries verify each root. The order in a solution set does not matter, but this checking list uses increasing root order.
@display checks
V=(L(r_1),R(r_1),L(r_2),R(r_2)),\qquad r_1<r_2
@endblock
@block proposition | rule | 1.2 | Factoring and the quadratic formula
@prose Subtract the right side from both sides, expand brackets and combine like powers. Clear fixed denominators by multiplying every term on both sides by their least common positive multiple. Multiplying by a nonzero constant preserves all solutions. A negative leading coefficient may be made positive by multiplying the whole equation by -1. For example, 2x squared plus x minus 1 factors as (2x-1)(x+1): the products are 2x squared, 2x, -x and -1, so the middle coefficient is 1.
@prose The zero-product property says a real product is zero exactly when at least one factor is zero; u and v below denote the two real factor values. It applies after the equation has zero on one side. Solve each linear factor equation and collect every resulting root. A repeated factor gives the same root twice algebraically but only one distinct set member.
@display zero_product
u v=0\quad\Longleftrightarrow\quad u=0\ \text{or}\ v=0
@prose For a standard quadratic, D is the discriminant, b squared minus 4ac. If D is positive, the formula gives two distinct real roots. If D is zero, both signs give the same repeated root. If D is negative, no real square root of D exists and the quadratic has no real roots. A negative result is a complete mathematical conclusion, not an unfinished calculation.
@display formula
D=b^2-4ac,\qquad x=\frac{-b\pm\sqrt{D}}{2a}\quad(D\ge0)
@endblock
@block proposition | condition | 1.3 | Why every branch is necessary
@prose Completing the square rewrites the quadratic as a squared linear expression equal to a constant. Half the linear coefficient determines the added square: x squared plus 8x becomes (x+4) squared after adding 16. Add the same number to the other side. From a square equal to a positive number, take both the positive and negative square-root branches; for zero there is one branch; a real square cannot equal a negative number.
@display completion
ax^2+bx+c=a\left(x+\frac{b}{2a}\right)^2-\frac{D}{4a}
@prose Expanding the right side recovers ax squared plus bx plus c: the added b squared divided by 4a cancels the same term in D divided by 4a. Since a is nonzero, completing the square and isolating x gives exactly the formula above. This establishes completeness, not just two successful guesses. Never divide by x to remove a common factor: x may be zero. Keep the zero branch before solving the remaining factor.
@endblock
@block example | worked | 1.4 | A separate complete quadratic
@prose Solve the displayed original equation over the real numbers.
@display worked_given
x^2+6x=7
@help hint
@prose Half of 6 is 3. Add its square to both sides to create a single square. Keep both signs when undoing that square.
@help answer
@display
S=\{-7,1\}
@help solution
@prose Add 9 to both sides: x squared plus 6x plus 9 equals 16. Thus (x+3) squared equals 16. Both branches are x+3=4 and x+3=-4, giving x=1 and x=-7. Equivalently, standard form is x squared plus 6x minus 7 equals zero, and (x+7)(x-1)=0; the cross terms -x+7x give 6x. Either reversible method produces every root.
@display worked_route
(x+3)^2=16\quad\Longrightarrow\quad x+3=4\ \text{or}\ x+3=-4\quad\Longrightarrow\quad S=\{-7,1\}
@prose At -7 the original left side is 49+6(-7)=49-42=7; the right side is 7. At 1 the left side is 1+6=7 and the right side is 7. Both original equalities hold, and the two square-root branches exhaust the possibilities.
@display worked_check
V=(7,7,7,7)
@endblock
@block example | errors | 1.5 | Preserve signs, factors and the original equation
@prose A factorization must reproduce the constant and the middle coefficient, not only the leading term. In the formula, negate the signed b and divide the entire numerator by 2a. Do not replace a negative discriminant by its absolute value, discard the negative square-root branch or count a repeated root as two different numbers. Factoring a product equal to a nonzero number does not permit setting its factors to zero. During verification, zero may be the rearranged residual even when both original sides have a different common value.
@endblock
@block exercise | practice | 1.6 | Vary structure while completing the solve
@prose Begin with rearrangement and factors, including a repeated root and a zero root. Continue with a negative leading coefficient, constant fractions and exact irrational roots. The final problems ask you to select a useful transformation, handle exceptional real-root behavior and expand an original product before applying the zero-product property.
@endblock
@block summary | summary | 1.7 | A complete real solution
@prose Identify a nonzero quadratic coefficient, use balanced operations and a valid complete quadratic method, and state every distinct real root. Verify each against both original sides. For no real roots, give a reason valid for every real input, such as a negative discriminant or a strictly positive completed square, rather than a failed test at one number.
@endblock
@practice prod03_algebra_quadratic_roots_q01
@practice prod03_algebra_quadratic_roots_q02
@practice prod03_algebra_quadratic_roots_q03
@practice prod03_algebra_quadratic_roots_q04
@practice prod03_algebra_quadratic_roots_q05
@practice prod03_algebra_quadratic_roots_q06
@practice prod03_algebra_quadratic_roots_q07
@practice prod03_algebra_quadratic_roots_q08
@practice prod03_algebra_quadratic_roots_q09
@practice prod03_algebra_quadratic_roots_q10
@practice prod03_algebra_quadratic_roots_q11
@practice prod03_algebra_quadratic_roots_q12
@end

@question prod03_algebra_quadratic_roots_q01 | Quadratic equation 1
@template choices.v1
@version 1
@goal Find the complete real solution set and verify the original equation.
@given x^2=5x-6
@domain Work over the real numbers. S is the complete solution set. L(t) and R(t) evaluate the original left and right sides at t. At the final check, V lists L(r),R(r) for each distinct root r in increasing order.
@read prod03_algebra_quadratic_roots_r
@step 10 | Move all terms left and combine them in descending-power standard form, with zero on the right. Which equation results?
@choice 11 | x^2-5x+6=0
@choice 12 | x^2+5x-6=0
@choice 13 | x^2-5x-6=0
@answer 11
@feedback 12 | This copied the right-side terms without negating them. Subtract 5x-6, giving -5x+6 on the left.
@feedback 13 | Subtracting -6 adds 6. The constant is +6, not -6.
@after x^2-5x+6=0
@wrong Subtract the complete right side from both sides.
@why Subtract 5x and add 6 on both sides. The right side becomes zero and the left becomes x squared minus 5x plus 6. Both operations are reversible.
@step 20 | Express the entire left polynomial as a product of two linear factors equal to zero.
@choice 22 | (x+2)(x+3)=0
@choice 21 | (x-2)(x-3)=0
@choice 23 | (x-1)(x-6)=0
@answer 21
@feedback 22 | These factors give +2x+3x=5x. The required middle term is -5x.
@feedback 23 | These factors give -x-6x=-7x. Use numbers with product 6 and sum -5.
@after (x-2)(x-3)=0
@wrong Check both the product of constants and the cross-term sum.
@why Expanding gives x squared minus 3x minus 2x plus 6, which combines to x squared minus 5x plus 6. The factored equation is identical to the standard one.
@step 30 | Solve both zero-product branches. Which set contains every distinct real root?
@choice 32 | S=\{-3,-2\}
@choice 33 | S=\{2\}
@choice 31 | S=\{2,3\}
@answer 31
@feedback 32 | Solving x-2=0 adds 2, and solving x-3=0 adds 3. Both roots are positive.
@feedback 33 | This solves only x-2=0. The other factor gives the additional root 3.
@after S=\{2,3\}
@wrong Set each factor to zero and include both resulting values.
@why A zero product requires x-2=0 or x-3=0, so x is 2 or 3. These two linear branches exhaust the factored quadratic.
@step 40 | Evaluate both original sides at every root, in increasing order. Which list V is correct?
@choice 41 | V=\left(4,4,9,9\right)
@choice 42 | V=\left(4,10,9,15\right)
@choice 43 | V=\left(0,0,0,0\right)
@answer 41
@feedback 42 | The right side omitted -6. It is 10-6=4 at 2 and 15-6=9 at 3.
@feedback 43 | Zero is the rearranged residual, not each original side. The original common values are 4 and 9.
@after S=\{2,3\},\quad V=\left(4,4,9,9\right)
@wrong Substitute into the original x squared and 5x-6 separately.
@why At 2, the sides are 2 squared=4 and 5(2)-6=4. At 3, they are 3 squared=9 and 5(3)-6=9. Both roots check, and the complete factor branches rule out any other root.
@end

@question prod03_algebra_quadratic_roots_q02 | Quadratic equation 2
@template choices.v1
@version 1
@goal Find the complete real solution set and verify the original equation.
@given x^2+4=4x
@domain Work over the real numbers. S is the complete solution set. L(t) and R(t) evaluate the original left and right sides at t. At the final check, V lists L(r),R(r) for each distinct root r in increasing order.
@read prod03_algebra_quadratic_roots_r
@step 10 | Move all terms left and combine them in descending-power standard form, with zero on the right.
@choice 12 | x^2+4x+4=0
@choice 11 | x^2-4x+4=0
@choice 13 | x^2-4x-4=0
@answer 11
@feedback 12 | Moving 4x left subtracts it. The linear coefficient is -4, not +4.
@feedback 13 | The existing left constant stays +4 when 4x is subtracted.
@after x^2-4x+4=0
@wrong Retain the left constant and subtract the entire right side.
@why Subtract 4x from both sides. In descending powers the left is x squared minus 4x plus 4 and the right is zero.
@step 20 | Write the left polynomial as linear factors, allowing a repeated factor square.
@choice 22 | (x+2)^2=0
@choice 23 | (x-2)(x+2)=0
@choice 21 | (x-2)^2=0
@answer 21
@feedback 22 | Squaring x+2 gives +4x as the middle term, not -4x.
@feedback 23 | The cross terms cancel and the constant is -4. The required polynomial has middle term -4x and constant +4.
@after (x-2)^2=0
@wrong Expand the square, including both cross products.
@why Multiplying x-2 by itself gives x squared minus 2x minus 2x plus 4. Thus the middle term is -4x and both factors are identical.
@step 30 | Solve the factor equation and count each distinct root once.
@choice 31 | S=\{2\}
@choice 32 | S=\{-2,2\}
@choice 33 | S=\varnothing
@answer 31
@feedback 32 | The square is of x-2, not x. Its only zero occurs when x-2=0; -2 gives a square of 16.
@feedback 33 | A square can equal zero. Here x=2 makes x-2 zero.
@after S=\{2\}
@wrong A repeated linear factor gives one distinct root.
@why Both copies of x-2 give x=2. There is one distinct real root, recorded once in S; a real square is zero only when its base is zero.
@step 40 | What are the original left and right values at the distinct root?
@choice 42 | V=\left(4,8\right)
@choice 41 | V=\left(8,8\right)
@choice 43 | V=\left(0,0\right)
@answer 41
@feedback 42 | The left side omitted +4: 2 squared plus 4 is 4+4=8.
@feedback 43 | The rearranged square is zero, but the original sides are both 8.
@after S=\{2\},\quad V=\left(8,8\right)
@wrong Check the original equation, including its left constant.
@why At 2, L(2)=2 squared+4=8 and R(2)=4(2)=8. The check confirms existence; the repeated-square argument proves there is no other real root.
@end

@question prod03_algebra_quadratic_roots_q03 | Quadratic equation 3
@template choices.v1
@version 1
@goal Find the complete real solution set and verify the original equation.
@given 2x^2+x=3
@domain Work over the real numbers. S is the complete solution set. L(t) and R(t) evaluate the original left and right sides at t. At the final check, V lists L(r),R(r) for each distinct root r in increasing order.
@read prod03_algebra_quadratic_roots_r
@step 10 | Move all terms left and combine in descending-power standard form, with zero on the right.
@choice 12 | 2x^2+x+3=0
@choice 13 | x^2+x-3=0
@choice 11 | 2x^2+x-3=0
@answer 11
@feedback 12 | Subtract 3 from both sides, rather than adding it to the left.
@feedback 13 | The quadratic coefficient is still 2. Moving the constant does not change it.
@after 2x^2+x-3=0
@wrong Retain both variable coefficients and subtract 3.
@why Subtracting 3 produces 2x squared plus x minus 3 equals zero. Adding 3 reverses the step.
@step 20 | Express the entire left polynomial as two linear factors equal to zero.
@choice 21 | (2x+3)(x-1)=0
@choice 22 | (2x-3)(x+1)=0
@choice 23 | (2x+1)(x-3)=0
@answer 21
@feedback 22 | The cross terms 2x-3x give -x, but the required linear term is +x.
@feedback 23 | The cross terms -6x+x give -5x, not +x.
@after (2x+3)(x-1)=0
@wrong Check the leading product, both cross terms and the constant product.
@why Distribution gives 2x squared minus 2x plus 3x minus 3. Combining -2x+3x gives +x, so the factorization is an identity.
@step 30 | Solve both linear factor equations. Which set is complete?
@choice 32 | S=\{-3,1\}
@choice 31 | S=\{-\frac{3}{2},1\}
@choice 33 | S=\{-1,\frac{3}{2}\}
@answer 31
@feedback 32 | From 2x+3=0, divide -3 by 2. The root is -3/2, not -3.
@feedback 33 | These signs come from the opposite factors. Here 2x=-3 and x=1.
@after S=\{-\frac{3}{2},1\}
@wrong Isolate x in each factor, retaining its nonzero coefficient.
@why The first branch gives 2x=-3 and x=-3/2; the second gives x=1. The zero-product equivalence proves that these are all the roots.
@step 40 | Evaluate both original sides at the roots in increasing order.
@choice 42 | V=\left(\frac{9}{2},3,2,3\right)
@choice 43 | V=\left(0,0,0,0\right)
@choice 41 | V=\left(3,3,3,3\right)
@answer 41
@feedback 42 | This omitted the original +x term. Add -3/2 to 9/2 at the first root and add 1 to 2 at the second.
@feedback 43 | Zero belongs to the standardized residual. The original right side is 3 at both roots.
@after S=\{-\frac{3}{2},1\},\quad V=\left(3,3,3,3\right)
@wrong Square the signed root, multiply by 2, then add that root.
@why At -3/2, the left is 2(9/4)-3/2=9/2-3/2=3, equal to the right. At 1, the left is 2+1=3, also equal to the right. The two factor branches are complete.
@end

@question prod03_algebra_quadratic_roots_q04 | Quadratic equation 4
@template choices.v1
@version 1
@goal Find the complete real solution set and verify the original equation.
@given 3x^2=6x
@domain Work over the real numbers. S is the complete solution set. L(t) and R(t) evaluate the original left and right sides at t. At the final check, V lists L(r),R(r) for each distinct root r in increasing order.
@read prod03_algebra_quadratic_roots_r
@step 10 | Subtract the right side and combine in descending-power standard form without rescaling.
@choice 12 | 3x^2+6x=0
@choice 11 | 3x^2-6x=0
@choice 13 | 3x-6=0
@answer 11
@feedback 12 | Subtracting 6x gives -6x on the left, not +6x.
@feedback 13 | This divides by x and discards a possible zero solution. First move 6x left without dividing by an unknown.
@after 3x^2-6x=0
@wrong Subtract 6x from both sides without cancelling x.
@why The reversible subtraction gives 3x squared minus 6x equals zero. It remains quadratic and still permits x=0.
@step 20 | Factor the complete left side into linear factors, retaining any common variable factor.
@choice 22 | 3x(x+2)=0
@choice 23 | 3(x-2)=0
@choice 21 | 3x(x-2)=0
@answer 21
@feedback 22 | Expanding gives 3x squared plus 6x. The required linear term is negative.
@feedback 23 | Removing x loses its zero branch. Keep the product 3x times x-2.
@after 3x(x-2)=0
@wrong Keep the common x as a factor instead of dividing by it.
@why Factoring 3x gives 3x times (x-2), whose products are 3x squared and -6x. No division by x has occurred.
@step 30 | Solve every zero-product branch. Which complete solution set results?
@choice 31 | S=\{0,2\}
@choice 32 | S=\{2\}
@choice 33 | S=\{-2,0\}
@answer 31
@feedback 32 | This ignores 3x=0. The value zero satisfies the original equation and must remain.
@feedback 33 | The second factor is x-2, so its root is +2, not -2.
@after S=\{0,2\}
@wrong Include the root of the common variable factor.
@why Since 3 is nonzero, 3x=0 gives x=0; x-2=0 gives x=2. A real product is zero only through these branches, so no root is missing.
@step 40 | Check the original sides at both roots in increasing order.
@choice 42 | V=\left(0,0,4,12\right)
@choice 41 | V=\left(0,0,12,12\right)
@choice 43 | V=\left(0,0,12,6\right)
@answer 41
@feedback 42 | At 2, the left is 3 times 2 squared, or 3(4)=12. This omitted the coefficient 3.
@feedback 43 | At 2, the right is 6(2)=12, not the coefficient 6 alone.
@after S=\{0,2\},\quad V=\left(0,0,12,12\right)
@wrong Evaluate 3x squared and 6x separately at both roots.
@why At zero both sides are zero. At 2, the left is 3(4)=12 and the right is 6(2)=12. Both retained factor branches check in the original equation.
@end

@question prod03_algebra_quadratic_roots_q05 | Quadratic equation 5
@template choices.v1
@version 1
@goal Find the complete real solution set and verify the original equation.
@given -x^2+2x+8=0
@domain Work over the real numbers. S is the complete solution set. L(t) and R(t) evaluate the original left and right sides at t. At the final check, V lists L(r),R(r) for each distinct root r in increasing order.
@read prod03_algebra_quadratic_roots_r
@step 10 | Multiply the whole equation by -1 and write combined descending-power standard form with positive leading coefficient.
@choice 12 | x^2+2x+8=0
@choice 13 | x^2-2x+8=0
@choice 11 | x^2-2x-8=0
@answer 11
@feedback 12 | Only the leading sign was changed. Multiplication by -1 also changes +2x to -2x and +8 to -8.
@feedback 13 | The constant must change sign too: (-1)(8)=-8.
@after x^2-2x-8=0
@wrong Multiply every term, including zero on the right, by -1.
@why Multiplication by the nonzero constant -1 is reversible. The three left terms become x squared, -2x and -8, and the right stays zero.
@step 20 | Express the normalized left polynomial as two linear factors equal to zero.
@choice 21 | (x-4)(x+2)=0
@choice 22 | (x+4)(x-2)=0
@choice 23 | (x-4)(x-2)=0
@answer 21
@feedback 22 | The cross terms -2x+4x give +2x. The required middle term is -2x.
@feedback 23 | Both negative constants give +8 and cross terms -6x. The required constant is -8 and middle coefficient is -2.
@after (x-4)(x+2)=0
@wrong Check the signed constant product and the cross-term sum.
@why Distribution gives x squared plus 2x minus 4x minus 8. Combining the middle terms gives -2x, as required.
@step 30 | Solve both factor equations and state all distinct roots.
@choice 32 | S=\{-4,2\}
@choice 31 | S=\{-2,4\}
@choice 33 | S=\{4\}
@answer 31
@feedback 32 | Solving x-4=0 gives +4, while x+2=0 gives -2. These proposed signs reverse both roots.
@feedback 33 | This omits the second factor's root: x+2=0 gives -2.
@after S=\{-2,4\}
@wrong Set each linear factor equal to zero.
@why The factors give x=4 or x=-2. The zero-product equivalence and reversible sign normalization preserve exactly these two roots.
@step 40 | Evaluate the original negative-leading left side and the original right side at each root.
@choice 42 | V=\left(8,0,32,0\right)
@choice 43 | V=\left(-8,0,-8,0\right)
@choice 41 | V=\left(0,0,0,0\right)
@answer 41
@feedback 42 | This replaced the original -x squared by +x squared. Keep its minus sign: -4-4+8=0 and -16+8+8=0.
@feedback 43 | This omitted the original +8 constant at both roots.
@after S=\{-2,4\},\quad V=\left(0,0,0,0\right)
@wrong Return to the original expression, not a partly sign-changed version.
@why At -2 the original left is -4-4+8=0. At 4 it is -16+8+8=0. Both equal the original right side zero, and the complete factorization proves there are no other roots.
@end

@question prod03_algebra_quadratic_roots_q06 | Quadratic equation 6
@template choices.v1
@version 1
@goal Find the complete real solution set and verify the original equation.
@given \frac{x^2}{2}-\frac{x}{3}=\frac{1}{6}
@domain Work over the real numbers. S is the complete solution set. L(t) and R(t) evaluate the original left and right sides at t. At the final check, V lists L(r),R(r) for each distinct root r in increasing order.
@read prod03_algebra_quadratic_roots_r
@step 10 | Multiply every term by the least common denominator 6, move all terms left, and combine in descending-power standard form.
@choice 11 | 3x^2-2x-1=0
@choice 12 | 3x^2-2x-6=0
@choice 13 | 3x^2-x-1=0
@answer 11
@feedback 12 | The right side becomes 6(1/6)=1, not 6. Moving it left gives -1.
@feedback 13 | The linear term also receives 6: 6(-x/3)=-2x, not -x.
@after 3x^2-2x-1=0
@wrong Multiply every fraction by 6 before moving the constant.
@why The least common denominator is 6. The terms become 3x squared, -2x and 1 respectively. Subtract 1 to get zero on the right; multiplying by 6 is reversible because 6 is nonzero.
@step 20 | Write the entire integer-coefficient left polynomial as two linear factors equal to zero.
@choice 22 | (3x-1)(x+1)=0
@choice 21 | (3x+1)(x-1)=0
@choice 23 | (3x-1)(x-1)=0
@answer 21
@feedback 22 | The cross terms 3x-x give +2x, not -2x.
@feedback 23 | The cross terms total -4x and the constant is +1. Both disagree with the required -2x and -1.
@after (3x+1)(x-1)=0
@wrong Expand both cross terms before accepting the factors.
@why Multiplication gives 3x squared minus 3x plus x minus 1. The middle terms combine to -2x, proving the factorization.
@step 30 | Solve both linear branches and state the complete solution set.
@choice 32 | S=\{-1,1\}
@choice 33 | S=\{-1,\frac{1}{3}\}
@choice 31 | S=\{-\frac{1}{3},1\}
@answer 31
@feedback 32 | The branch 3x+1=0 gives 3x=-1 and x=-1/3. Divide by the coefficient 3.
@feedback 33 | These roots correspond to opposite factor signs. The actual branches are 3x=-1 and x=1.
@after S=\{-\frac{1}{3},1\}
@wrong Retain the sign and coefficient when solving each factor.
@why The first factor gives x=-1/3 and the second gives x=1. Constant denominator clearing and zero-product equivalence show that no other real roots are possible.
@step 40 | Evaluate the original fractional sides at the roots in increasing order.
@choice 41 | V=\left(\frac{1}{6},\frac{1}{6},\frac{1}{6},\frac{1}{6}\right)
@choice 42 | V=\left(0,0,0,0\right)
@choice 43 | V=\left(\frac{1}{18},\frac{1}{6},\frac{1}{2},\frac{1}{6}\right)
@answer 41
@feedback 42 | Zero is the residual after rearrangement. The original right side is still 1/6 at both roots.
@feedback 43 | This omitted -x/3. At -1/3 that term is +1/9; at 1 it is -1/3.
@after S=\{-\frac{1}{3},1\},\quad V=\left(\frac{1}{6},\frac{1}{6},\frac{1}{6},\frac{1}{6}\right)
@wrong Restore the original denominators for verification.
@why At -1/3, L=1/18+1/9=3/18=1/6. At 1, L=1/2-1/3=1/6. R is 1/6 at both inputs, so both complete factor branches verify in the original equation.
@end

@question prod03_algebra_quadratic_roots_q07 | Quadratic equation 7
@template choices.v1
@version 1
@goal Find the complete real solution set and verify the original equation.
@given x^2-4x=1
@domain Work over the real numbers. S is the complete solution set. L(t) and R(t) evaluate the original left and right sides at t. At the final check, V lists L(r),R(r) for each distinct root r in increasing order.
@read prod03_algebra_quadratic_roots_r
@step 10 | Move all terms left and combine in descending-power standard form, with zero on the right.
@choice 12 | x^2-4x+1=0
@choice 13 | x^2+4x-1=0
@choice 11 | x^2-4x-1=0
@answer 11
@feedback 12 | Subtracting the right side 1 produces -1 on the left.
@feedback 13 | The existing -4x remains negative when only 1 is subtracted.
@after x^2-4x-1=0
@wrong Subtract 1 while retaining both original variable terms.
@why Subtract 1 from both sides, obtaining coefficients a=1, b=-4 and c=-1 in descending powers. The quadratic coefficient is nonzero.
@step 20 | Calculate the discriminant D=b squared minus 4ac from the standard coefficients.
@choice 21 | D=20
@choice 22 | D=12
@choice 23 | D=-20
@answer 21
@feedback 22 | This treated c as +1. Here 4ac=-4, so subtracting it adds 4 to 16.
@feedback 23 | Squaring -4 gives +16. Then 16-(-4)=20, not a negative value.
@after D=20
@wrong Square the signed b and subtract the signed product 4ac.
@why D=(-4) squared-4(1)(-1)=16-(-4)=20. It is positive, so the complete formula has two distinct real branches.
@step 30 | Substitute a, b and D into the quadratic formula, retaining the unevaluated square root of D and the denominator 2a.
@choice 32 | x=\frac{-4\pm\sqrt{20}}{2}
@choice 31 | x=\frac{4\pm\sqrt{20}}{2}
@choice 33 | x=\frac{4\pm\sqrt{20}}{1}
@answer 31
@feedback 32 | The formula uses -b. Negating b=-4 gives +4.
@feedback 33 | The denominator is 2a=2(1)=2, not a alone.
@after x=\frac{4\pm\sqrt{20}}{2}
@wrong Negate b and divide the whole numerator by 2a.
@why With a=1, b=-4 and D=20, the numerator is 4 plus or minus the square root of 20 and the denominator is 2. Since a is nonzero and D is positive, both formula branches are valid.
@step 40 | Which exact solution set contains every distinct real root?
@choice 42 | S=\{2-\sqrt{20},2+\sqrt{20}\}
@choice 43 | S=\{2+\sqrt{5}\}
@choice 41 | S=\{2-\sqrt{5},2+\sqrt{5}\}
@answer 41
@feedback 42 | The denominator divides both numerator terms. Since the square root of 20 is twice the square root of 5, dividing that term by 2 leaves the square root of 5.
@feedback 43 | This keeps only the plus branch. The positive discriminant gives a distinct minus-branch root too.
@after S=\{2-\sqrt{5},2+\sqrt{5}\}
@wrong Reduce the square factor inside the radical and retain both signs.
@why Because 20=4 times 5, its square root is twice the square root of 5. Dividing 4 and each radical term by 2 gives 2 minus the square root of 5 and 2 plus the square root of 5. The formula proves completeness.
@step 50 | Evaluate both original sides at the two roots in increasing order.
@choice 51 | V=\left(1,1,1,1\right)
@choice 52 | V=\left(0,0,0,0\right)
@choice 53 | V=\left(1,-1,1,-1\right)
@answer 51
@feedback 52 | The standard polynomial vanishes, but the original right side is 1. The original left side equals 1 at each root.
@feedback 53 | The original right side is +1, not the moved constant -1 from standard form.
@after S=\{2-\sqrt{5},2+\sqrt{5}\},\quad V=\left(1,1,1,1\right)
@wrong Use the original x squared minus 4x, not the standardized residual.
@why At 2 minus the square root of 5, the square is 9 minus four times that square root, and -4x is -8 plus four times that square root; their sum is 1. At the plus root the two radical signs reverse and again cancel, leaving 9-8=1. Both original right values are 1. Both complete formula branches check exactly.
@end

@question prod03_algebra_quadratic_roots_q08 | Quadratic equation 8
@template choices.v1
@version 1
@goal Find the complete real solution set and verify the original equation.
@given x^2+2x=1
@domain Work over the real numbers. S is the complete solution set. L(t) and R(t) evaluate the original left and right sides at t. At the final check, V lists L(r),R(r) for each distinct root r in increasing order.
@read prod03_algebra_quadratic_roots_r
@step 10 | Move all terms left and combine in descending-power standard form, with zero on the right.
@choice 11 | x^2+2x-1=0
@choice 12 | x^2+2x+1=0
@choice 13 | x^2-2x-1=0
@answer 11
@feedback 12 | Subtract 1 from the left as well as the right. The new constant is -1.
@feedback 13 | The linear coefficient stays +2; subtracting 1 does not change it.
@after x^2+2x-1=0
@wrong Move the right-hand constant without changing existing coefficients.
@why Subtracting 1 gives standard coefficients a=1, b=2 and c=-1. This balanced step is reversible.
@step 20 | Evaluate D=b squared minus 4ac from these standard coefficients.
@choice 22 | D=0
@choice 21 | D=8
@choice 23 | D=-8
@answer 21
@feedback 22 | This uses c=+1. The actual product 4ac is -4, so 4-(-4)=8.
@feedback 23 | The square of 2 is +4, and subtracting -4 increases it to 8.
@after D=8
@wrong Keep the negative constant when forming 4ac.
@why D=2 squared-4(1)(-1)=4-(-4)=8. Its positive value means there are two distinct real roots.
@step 30 | Substitute into the quadratic formula, retaining the unevaluated square root of D and denominator 2a.
@choice 32 | x=\frac{2\pm\sqrt{8}}{2}
@choice 33 | x=\frac{-2\pm\sqrt{8}}{1}
@choice 31 | x=\frac{-2\pm\sqrt{8}}{2}
@answer 31
@feedback 32 | Negating b=2 gives -2, not +2, in the numerator.
@feedback 33 | Divide by 2a=2, not by a=1 alone.
@after x=\frac{-2\pm\sqrt{8}}{2}
@wrong Use -b and put the entire numerator over 2a.
@why The signed numerator begins with -2, D is 8 and 2a is 2. The formula therefore supplies both values of (-2 plus or minus the square root of 8) divided by 2.
@step 40 | Which exact solution set contains every distinct real root?
@choice 41 | S=\{-1-\sqrt{2},-1+\sqrt{2}\}
@choice 42 | S=\{1-\sqrt{2},1+\sqrt{2}\}
@choice 43 | S=\{-1+\sqrt{2}\}
@answer 41
@feedback 42 | The rational part is -2/2=-1. This changed it to +1.
@feedback 43 | The square root of 8 is positive, so the minus branch is distinct and cannot be omitted.
@after S=\{-1-\sqrt{2},-1+\sqrt{2}\}
@wrong Reduce the square root of 8 and divide both terms by 2.
@why Since 8=4 times 2, its square root is twice the square root of 2. The rational part becomes -1 and the radical part becomes plus or minus the square root of 2. Both formula branches give all roots.
@step 50 | What is the list of original left and right values at those roots, in increasing order?
@choice 52 | V=\left(0,0,0,0\right)
@choice 51 | V=\left(1,1,1,1\right)
@choice 53 | V=\left(1,-1,1,-1\right)
@answer 51
@feedback 52 | The rearranged polynomial is zero, but the original equation has right side 1.
@feedback 53 | The right-hand value is the original +1, not the standardized constant -1.
@after S=\{-1-\sqrt{2},-1+\sqrt{2}\},\quad V=\left(1,1,1,1\right)
@wrong Substitute into x squared plus 2x and compare with the original 1.
@why At -1 minus the square root of 2, x squared is 3 plus twice that square root, and 2x is -2 minus twice it, giving 1. At the plus root, x squared is 3 minus twice that square root and 2x is -2 plus twice it, again giving 1. Both right sides are 1, verifying both complete branches exactly.
@end

@question prod03_algebra_quadratic_roots_q09 | Quadratic equation 9
@template choices.v1
@version 1
@goal Find the complete real solution set and verify the original equation.
@given 2x^2+5x=0
@domain Work over the real numbers. S is the complete solution set. L(t) and R(t) evaluate the original left and right sides at t. At the final check, V lists L(r),R(r) for each distinct root r in increasing order.
@read prod03_algebra_quadratic_roots_r
@step 10 | Select a first transformation that exposes the common variable factor and preserves every real root.
@choice 12 | 2x+5=0
@choice 11 | x(2x+5)=0
@choice 13 | x(2x-5)=0
@answer 11
@feedback 12 | This divides by x, which may be zero. The original equation is satisfied at zero, so retain x as a factor.
@feedback 13 | Expanding this choice gives 2x squared minus 5x. The original linear term is +5x.
@after x(2x+5)=0
@wrong Use factoring, not division by the unknown common factor.
@why Both terms contain x. Factoring it gives x times (2x+5), whose expansion recovers the original left side exactly. This makes the zero-product method available without excluding zero.
@step 20 | Apply the zero-product property, retaining both linear equations with zero on the right.
@choice 22 | 2x+5=0
@choice 23 | x=0\ \text{or}\ 2x-5=0
@choice 21 | x=0\ \text{or}\ 2x+5=0
@answer 21
@feedback 22 | A product can vanish through its first factor x. This keeps only the second factor and loses zero.
@feedback 23 | The second factor has +5, not -5. Keep the factor unchanged when setting it equal to zero.
@after x=0\ \text{or}\ 2x+5=0
@wrong A zero product permits either factor to vanish.
@why For real factors, x times (2x+5) is zero if and only if x=0 or 2x+5=0. No division by x is needed.
@step 30 | Solve the branches and list every distinct real root.
@choice 31 | S=\{-\frac{5}{2},0\}
@choice 32 | S=\{-\frac{5}{2}\}
@choice 33 | S=\{0,\frac{5}{2}\}
@answer 31
@feedback 32 | The first branch already gives zero. It must be included along with the fractional root.
@feedback 33 | From 2x+5=0, subtract 5 and divide by 2: x=-5/2, not +5/2.
@after S=\{-\frac{5}{2},0\}
@wrong Solve 2x=-5 and retain the separate zero branch.
@why The second branch gives x=-5/2 and the first gives x=0. These are distinct and exhaust the original factored equation.
@step 40 | Evaluate the original sides at both roots in increasing order.
@choice 42 | V=\left(\frac{25}{2},0,0,0\right)
@choice 41 | V=\left(0,0,0,0\right)
@choice 43 | V=\left(-\frac{25}{2},0,0,0\right)
@answer 41
@feedback 42 | At -5/2 this kept only 2x squared=25/2. Add 5x=-25/2 to obtain zero.
@feedback 43 | At -5/2 this kept only 5x=-25/2. Add 2x squared=25/2 to obtain zero.
@after S=\{-\frac{5}{2},0\},\quad V=\left(0,0,0,0\right)
@wrong Include both original terms at the fractional root and at zero.
@why At -5/2, 2(25/4)+5(-5/2)=25/2-25/2=0. At zero both original terms vanish. The right side is zero at both inputs, confirming the full two-branch answer.
@end

@question prod03_algebra_quadratic_roots_q10 | Quadratic equation 10
@template choices.v1
@version 1
@goal Find the complete real solution set and verify the original equation.
@given x^2-2x=2
@domain Work over the real numbers. S is the complete solution set. L(t) and R(t) evaluate the original left and right sides at t. At the final check, V lists L(r),R(r) for each distinct root r in increasing order.
@read prod03_algebra_quadratic_roots_r
@step 10 | Select a first method transformation that writes the left side as one monic linear expression squared and the right as a constant, preserving all roots.
@choice 11 | (x-1)^2=3
@choice 12 | x(x-2)=2
@choice 13 | (x-1)^2=1
@answer 11
@feedback 12 | This factor rewrite is equivalent to the original equation, but it does not meet the requested single-square form. Complete the square by adding 1 to both sides.
@feedback 13 | Adding 1 to the left requires adding 1 to the right: 2+1=3, not 1.
@after (x-1)^2=3
@wrong Halve the linear coefficient and add its square on both sides.
@why Half of -2 is -1, whose square is 1. Adding 1 gives x squared minus 2x plus 1 equals 3. The left is (x-1) squared, so this method creates a single-square equation through a reversible addition.
@step 20 | Undo the square while leaving x-1 on the left of both branches.
@choice 22 | x-1=\sqrt{3}
@choice 21 | x-1=\sqrt{3}\ \text{or}\ x-1=-\sqrt{3}
@choice 23 | x-1=3\ \text{or}\ x-1=-3
@answer 21
@feedback 22 | A positive square has both positive and negative square-root branches. Retain x-1 equal to the negative square root of 3 too.
@feedback 23 | Squaring 3 or -3 gives 9, not 3. The branch values are the positive and negative square roots of 3.
@after x-1=\sqrt{3}\ \text{or}\ x-1=-\sqrt{3}
@wrong Retain both signs of the square root, not both signs of the original constant.
@why A real number has square 3 exactly when it is the positive or negative square root of 3. Applying that equivalence to x-1 gives the two complete branches.
@step 30 | Isolate x in both branches and give the complete root set.
@choice 32 | S=\{-1-\sqrt{3},-1+\sqrt{3}\}
@choice 33 | S=\{1+\sqrt{3}\}
@choice 31 | S=\{1-\sqrt{3},1+\sqrt{3}\}
@answer 31
@feedback 32 | Add 1 to both sides of x-1 equal to either branch value. The rational part is +1, not -1.
@feedback 33 | The negative square-root branch also gives a distinct root.
@after S=\{1-\sqrt{3},1+\sqrt{3}\}
@wrong Add 1 in each branch and keep both results.
@why Adding 1 gives 1 minus the square root of 3 and 1 plus the square root of 3. Reversible completion of the square and both square-root branches establish completeness.
@step 40 | What are both original values at each root, in increasing order?
@choice 41 | V=\left(2,2,2,2\right)
@choice 42 | V=\left(3,3,3,3\right)
@choice 43 | V=\left(0,0,0,0\right)
@answer 41
@feedback 42 | Three is the common value after adding 1, not the value of the original sides.
@feedback 43 | Zero is the original left-minus-right residual, not each original side.
@after S=\{1-\sqrt{3},1+\sqrt{3}\},\quad V=\left(2,2,2,2\right)
@wrong Evaluate x squared minus 2x and the original constant 2.
@why At the minus root, x squared is 4 minus twice the square root of 3 and -2x is -2 plus twice it, giving 2. At the plus root, the radical signs reverse and cancel, again giving 4-2=2. Both original right sides equal 2.
@end

@question prod03_algebra_quadratic_roots_q11 | Quadratic equation 11
@template choices.v1
@version 1
@goal Find the complete real solution set and justify the original-equation conclusion.
@given x^2+2x+3=0
@domain Work over the real numbers. S is the complete solution set. L(t) and R(t) evaluate the original left and right sides at t. At the final check, V lists L(r),R(r) for each distinct root r in increasing order.
@read prod03_algebra_quadratic_roots_r
@step 10 | The equation is in descending-power standard form. Which ordered coefficient list (a,b,c) matches it?
@choice 12 | (a,b,c)=\left(1,2,-3\right)
@choice 13 | (a,b,c)=\left(1,3,2\right)
@choice 11 | (a,b,c)=\left(1,2,3\right)
@answer 11
@feedback 12 | The left constant is +3. It has not been moved across the equality.
@feedback 13 | b is the coefficient of x, which is 2; c is the constant, which is 3. Do not exchange them.
@after (a,b,c)=\left(1,2,3\right)
@wrong Read the coefficients of x squared, x and the constant in order.
@why The implied coefficient of x squared is 1, the coefficient of x is 2 and the constant is 3. In particular a=1 is nonzero, so the quadratic discriminant criterion applies.
@step 20 | Calculate D=b squared minus 4ac.
@choice 21 | D=-8
@choice 22 | D=8
@choice 23 | D=16
@answer 21
@feedback 22 | This reverses the subtraction. The rule gives 4-12=-8, not 12-4.
@feedback 23 | This adds 4ac to b squared. The discriminant requires 4-12, not 4+12.
@after D=-8
@wrong Subtract the full positive product 4(1)(3) from 2 squared.
@why D=2 squared-4(1)(3)=4-12=-8. This is negative, so a real square-root branch cannot be formed.
@step 30 | Use the discriminant to state the complete solution set over the specified real domain.
@choice 32 | S=\{-1\}
@choice 31 | S=\varnothing
@choice 33 | S=\{-1-\sqrt{2},-1+\sqrt{2}\}
@answer 31
@feedback 32 | The value -b/(2a)=-1 is a root only when the discriminant is zero. Here substituting -1 gives 1-2+3=2, not zero.
@feedback 33 | These values replace D=-8 by +8. A negative real discriminant cannot be changed to its absolute value.
@after S=\varnothing
@wrong Negative discriminant means no real roots, not one root or an absolute-value radical.
@why The completed-square form behind the quadratic formula requires a real square equal to a negative value when D is negative. No real x can satisfy that requirement, so S has no members.
@step 40 | Give L(x)-R(x) as a single monic linear square plus a positive constant, proving the original sides cannot be equal.
@choice 42 | (x+1)^2-2
@choice 43 | (x-1)^2+2
@choice 41 | (x+1)^2+2
@answer 41
@feedback 42 | This expands to x squared plus 2x minus 1. The original constant is +3, obtained from 1+2, not 1-2.
@feedback 43 | This gives middle term -2x. The original middle term is +2x.
@after L(x)-R(x)=(x+1)^2+2>0,\quad S=\varnothing
@wrong Complete the square without changing the original polynomial.
@why Expanding (x+1) squared plus 2 gives x squared plus 2x plus 1 plus 2, exactly the original left-minus-right difference. The square is nonnegative for every real x, so adding 2 makes that difference strictly positive. Thus no original equality is possible; no root substitution has been omitted.
@end

@question prod03_algebra_quadratic_roots_q12 | Quadratic equation 12
@template choices.v1
@version 1
@goal Find the complete real solution set and verify the original equation.
@given (x-2)(x+1)=4
@domain Work over the real numbers. S is the complete solution set. L(t) and R(t) evaluate the original left and right sides at t. At the final check, V lists L(r),R(r) for each distinct root r in increasing order.
@read prod03_algebra_quadratic_roots_r
@step 10 | Expand, move all terms left and combine in descending-power standard form, with zero on the right.
@choice 12 | x^2-x-2=0
@choice 11 | x^2-x-6=0
@choice 13 | x^2+x-6=0
@answer 11
@feedback 12 | The expansion has constant -2, but subtracting the right side 4 changes it to -6.
@feedback 13 | The cross terms are +x and -2x, whose sum is -x, not +x.
@after x^2-x-6=0
@wrong Expand all four products, then subtract the original right side.
@why Expanding gives x squared plus x minus 2x minus 2, or x squared minus x minus 2. Subtracting 4 on both sides yields x squared minus x minus 6 equals zero.
@step 20 | Factor the complete standard polynomial into two linear factors equal to zero.
@choice 22 | (x-2)(x+1)=0
@choice 23 | (x+3)(x-2)=0
@choice 21 | (x-3)(x+2)=0
@answer 21
@feedback 22 | These are the original factors, whose product equals 4, not zero. Their constant product -2 does not match the new -6.
@feedback 23 | The cross terms -2x+3x give +x. The required middle term is -x.
@after (x-3)(x+2)=0
@wrong Factor the rearranged polynomial, not a product still equal to 4.
@why Distribution gives x squared plus 2x minus 3x minus 6, which combines to x squared minus x minus 6. Now the zero-product property applies because the right side is zero.
@step 30 | Solve both zero-product branches and give every real root.
@choice 31 | S=\{-2,3\}
@choice 32 | S=\{-1,2\}
@choice 33 | S=\{-3,2\}
@answer 31
@feedback 32 | These make the original factors zero, so their original product is zero rather than 4. Solve the factors of the rearranged equation instead.
@feedback 33 | The actual factors x-3 and x+2 give +3 and -2, respectively.
@after S=\{-2,3\}
@wrong Set x-3 or x+2 equal to zero after the valid rearrangement.
@why The branches give x=3 or x=-2. The expansion, balanced subtraction and complete factorization preserve precisely these original solutions.
@step 40 | Evaluate the original product and original right side at each root in increasing order.
@choice 42 | V=\left(0,0,0,0\right)
@choice 41 | V=\left(4,4,4,4\right)
@choice 43 | V=\left(-4,4,4,4\right)
@answer 41
@feedback 42 | The rearranged polynomial is zero, but the original product must be 4.
@feedback 43 | At -2, the factors are -4 and -1. Their product is positive 4, not -4.
@after S=\{-2,3\},\quad V=\left(4,4,4,4\right)
@wrong Use the original factors x-2 and x+1 in the final check.
@why At -2, the left is (-2-2)(-2+1)=(-4)(-1)=4. At 3, it is (3-2)(3+1)=1(4)=4. The right is 4 at both roots. These checks establish existence, and the full zero-product route establishes completeness.
@end
