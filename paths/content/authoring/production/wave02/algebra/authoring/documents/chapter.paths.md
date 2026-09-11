@paths 1
@subject algebra | Algebra
@chapter topic_0003 | Equations and Relations

@lesson prod02_algebra_full_linear_r | Solve a linear equation completely
@template lesson.v2
@block introduction | start | - | From the original equation to its solution set
@prose Solving means finding every real value that makes the original equality true. Work on complete sides, retain signs and brackets, and finish with a check of the original expressions. Some equations have one solution, some have none and some hold for every real value. The twelve linked problems carry each calculation through its conclusion.
@endblock
@block definition | terms | 1.1 | Terms, coefficients and solution sets
@prose An equality states that its left and right expressions have the same value. A term is a part joined to other parts by addition; subtraction adds the opposite term. In 3x-5, the variable term is 3x, its coefficient is 3 and the constant term is -5. A linear expression simplifies to a coefficient times x plus a constant. A denominator is the divisor below a fraction bar; it divides the whole numerator. All denominators here are fixed nonzero numbers.
@prose S denotes the solution set. Braces enclose the members of a set; {r} contains just the real value r. The empty-set symbol means there are no members, and the real-number symbol means every real number. L(t) and R(t) mean the original left and right expressions with the real input t substituted for x. The ordered pair (L(t),R(t)) lists the left value first and the right value second.
@display sets
S=\{r\},\qquad S=\varnothing,\qquad S=\mathbb{R}
@endblock
@block proposition | rule | 1.2 | Simplify and balance whole sides
@prose For a real multiplier k, coefficient a and constant b, distribution multiplies every term inside brackets: k(ax+b)=kax+kb. A negative multiplier also multiplies the signed constant. For example, -4(x-2)=-4x+8 because (-4)(-2)=8. Combine like terms only: coefficients of x combine with coefficients of x, and constants with constants. Subtracting a negative adds its opposite; removing a constant -5 means adding 5.
@prose In the rules below, k is a fixed real multiplier and u(x) is the same real linear expression added to each side. Adding or subtracting the same expression on both sides is reversible. Multiplying both complete sides by the same nonzero constant is reversed by dividing by it. To clear fractions, choose a positive integer divisible by every denominator, and multiply every term on both sides. The least such positive integer is the least common denominator. Multiplying only a numerator or only one side changes the equation.
@display balance
L(x)=R(x)\quad\Longleftrightarrow\quad L(x)+u(x)=R(x)+u(x)
@display nonzero_scale
L(x)=R(x)\quad\Longleftrightarrow\quad kL(x)=kR(x),\qquad k\ne0
@endblock
@block proposition | condition | 1.3 | Check the remaining coefficient before dividing
@prose After simplifying to ax+b=cx+d, a and c are real variable coefficients and b and d are real constants. Subtract cx and then b from both sides. The resulting coefficient is a-c. If it is nonzero, division gives exactly one candidate; reversing the operations proves that no other value solves the original equation. Substitute that candidate into the original brackets and fractions to check the arithmetic.
@display collect
ax+b=cx+d\quad\Longleftrightarrow\quad (a-c)x=d-b
@prose If a-c is zero, do not divide by it. Equal remaining constants give an identity that holds for every real x. Unequal remaining constants give a contradiction that no real x can satisfy; h below denotes the nonzero remaining real constant. A test at one value cannot prove an identity for all values; compare the original expressions symbolically. Their difference is identically zero for an identity and a fixed nonzero constant for a contradiction.
@display outcomes
0x=0\Rightarrow S=\mathbb{R},\qquad 0x=h,\ h\ne0\Rightarrow S=\varnothing
@endblock
@block example | worked | 1.4 | A separate complete solve
@prose Solve the displayed equation over the real numbers. Its numbers differ from every linked problem.
@display worked_given
-2(x+1)+3=\frac{x}{2}-4
@help hint
@prose Expand the negative bracket and combine the left constants. Clear the denominator on both whole sides, then collect the variable terms and constants separately.
@help answer
@display
S=\{2\}
@help solution
@prose Distribute -2 to x and 1, giving -2x-2+3=-2x+1 on the left. Multiply both sides by nonzero 2: -4x+2=x-8. Subtract x from both sides to obtain -5x+2=-8, then subtract 2 to get -5x=-10. Because -5 is nonzero, divide by -5 to obtain x=2. The operations are reversible, so this is the only candidate.
@display worked_route
-2x+1=\frac{x}{2}-4\quad\Longrightarrow\quad -4x+2=x-8\quad\Longrightarrow\quad -5x=-10\quad\Longrightarrow\quad x=2
@prose Check the original left side: -2(2+1)+3=-6+3=-3. The original right side is 2/2-4=1-4=-3. The same input gives equal sides, confirming the candidate.
@display worked_check
L(2)=R(2)=-3
@endblock
@block example | errors | 1.5 | Diagnose the operation, not just the final number
@prose In a negative bracket, losing one sign changes a product. In a denominator-clearing step, forgetting an outside constant changes a whole side. When removing a variable term from the right, subtract that term from the left too. A reversible alternative can still miss a requested local goal: multiplying by 2 is valid but does not clear a denominator 3. When all variable terms cancel, do not invent x=0; decide whether the remaining constant equality is always true or always false.
@endblock
@block exercise | practice | 1.6 | Build, vary and combine
@prose The first four problems establish signed collection, distribution and clearing denominators. The next four combine these skills and introduce cancellation outcomes. The final four mix fractions, negative groups and exceptional solution sets; two begin with selecting a denominator-clearing method. Every problem ends with original-expression evidence, not just a simplified answer.
@endblock
@block summary | summary | 1.7 | A complete answer includes its scope and check
@prose Simplify each complete side, collect terms with balanced operations, and check the remaining coefficient before division. State the full solution set. For a unique candidate, evaluate both original sides and use reversibility for uniqueness. For cancellation cases, establish an identity or contradiction for every real input. Keep exact fractions until the calculation is complete.
@endblock
@practice prod02_algebra_full_linear_q01
@practice prod02_algebra_full_linear_q02
@practice prod02_algebra_full_linear_q03
@practice prod02_algebra_full_linear_q04
@practice prod02_algebra_full_linear_q05
@practice prod02_algebra_full_linear_q06
@practice prod02_algebra_full_linear_q07
@practice prod02_algebra_full_linear_q08
@practice prod02_algebra_full_linear_q09
@practice prod02_algebra_full_linear_q10
@practice prod02_algebra_full_linear_q11
@practice prod02_algebra_full_linear_q12
@end

@question prod02_algebra_full_linear_q01 | Collect variables and a negative constant
@template choices.v1
@version 1
@goal Solve the original equation completely and verify the solution set.
@given 3x-5=x+7
@domain Work over the real numbers. All displayed denominators are nonzero constants. S is the complete solution set; L(t) and R(t) mean the original left and right expressions evaluated at t.
@read prod02_algebra_full_linear_r
@step 10 | Subtract the right-hand variable term from both sides. Which simplified equation results?
@choice 11 | 2x-5=7
@choice 12 | 3x-5=7
@choice 13 | 4x-5=7
@answer 11
@feedback 12 | This removed x only on the right. Subtract it on the left too: 3x-x=2x.
@feedback 13 | Adding the coefficients gives 4x, but the chosen operation subtracts x. The coefficient becomes 3-1=2.
@after 2x-5=7
@wrong Subtract the same x term from both complete sides.
@why Subtract x on both sides. The left coefficient is 3-1=2 and the right variable term cancels; the constants stay -5 and 7. Adding x back reverses this step.
@step 20 | Remove the constant on the variable side. Which equation results?
@choice 22 | 2x=2
@choice 21 | 2x=12
@choice 23 | 2x=7
@answer 21
@feedback 22 | Subtracting 5 from 7 uses the wrong inverse. Cancel -5 by adding 5; the right side becomes 7+5=12.
@feedback 23 | This deletes -5 without balancing the right side. Add 5 to both sides.
@after 2x=12
@wrong Cancel the signed constant with its opposite on both sides.
@why Add 5 to both sides: -5+5=0 and 7+5=12. This is reversible by subtracting 5.
@step 30 | Isolate x using the nonzero coefficient. Which candidate results?
@choice 32 | x=24
@choice 33 | x=\frac{1}{6}
@choice 31 | x=6
@answer 31
@feedback 32 | Multiplying 12 by 2 does not undo multiplication of x by 2. Divide both sides by 2.
@feedback 33 | This reverses the quotient. The right side is 12 divided by 2, not 2 divided by 12.
@after x=6
@wrong Divide the complete right side by the coefficient of x.
@why Because 2 is nonzero, divide by 2: 12/2=6. Reversing the balanced steps shows that this is the only possible solution.
@step 40 | At the candidate just found, what is the ordered pair of original values (L(6),R(6))?
@choice 41 | \left(13,13\right)
@choice 42 | \left(18,13\right)
@choice 43 | \left(13,6\right)
@answer 41
@feedback 42 | The left evaluation omitted -5. The original left side is 3(6)-5=18-5=13.
@feedback 43 | The right evaluation omitted +7. The original right side is 6+7=13.
@after S=\{6\},\quad (L(6),R(6))=(13,13)
@wrong Evaluate both original expressions, including their constants.
@why The original sides are 3(6)-5=13 and 6+7=13. Equality verifies existence; the reversible route with a nonzero final coefficient already proves uniqueness, so the full set is the singleton 6.
@end

@question prod02_algebra_full_linear_q02 | Retain a negative coefficient
@template choices.v1
@version 1
@goal Solve the original equation completely and verify the solution set.
@given -2x+3=x+9
@domain Work over the real numbers. All displayed denominators are nonzero constants. S is the complete solution set; L(t) and R(t) mean the original left and right expressions evaluated at t.
@read prod02_algebra_full_linear_r
@step 10 | Subtract the right-hand variable term from both sides. Which simplified equation results?
@choice 12 | -x+3=9
@choice 11 | -3x+3=9
@choice 13 | -2x+3=9
@answer 11
@feedback 12 | This adds 1 to the left coefficient. Subtracting x gives -2-1=-3, not -1.
@feedback 13 | This removes x from only the right side. Subtract x from the left side too.
@after -3x+3=9
@wrong Keep the sign of the left coefficient when subtracting x.
@why Subtract x from both sides: -2x-x=-3x and x-x=0. Both constants remain unchanged, and adding x back restores the original equation.
@step 20 | Remove the constant on the variable side. Which equation results?
@choice 22 | -3x=12
@choice 23 | -3x=9
@choice 21 | -3x=6
@answer 21
@feedback 22 | Adding 3 does not cancel the existing +3. Subtract 3 from each side; 9-3=6.
@feedback 23 | Deleting +3 on just one side changes equality. The right side must also lose 3.
@after -3x=6
@wrong Subtract the added constant from both sides.
@why Subtract 3 on both sides: 3-3=0 and 9-3=6. Adding 3 reverses the move.
@step 30 | Divide by the nonzero coefficient. Which candidate results?
@choice 31 | x=-2
@choice 32 | x=2
@choice 33 | x=-\frac{1}{2}
@answer 31
@feedback 32 | A positive number divided by a negative number is negative: 6/(-3)=-2.
@feedback 33 | The quotient is 6/(-3), not (-3)/6. Keep the right side as the numerator.
@after x=-2
@wrong Use signed division without reversing the quotient.
@why The coefficient -3 is nonzero, so division is reversible. Six divided by -3 is -2; no second value can satisfy the reduced equation.
@step 40 | What is the ordered pair of original values (L(-2),R(-2))?
@choice 42 | \left(-1,7\right)
@choice 41 | \left(7,7\right)
@choice 43 | \left(7,11\right)
@answer 41
@feedback 42 | The product -2 times -2 is +4, not -4. The left side is 4+3=7.
@feedback 43 | Substitute the signed value -2 on the right: -2+9=7, not 2+9.
@after S=\{-2\},\quad (L(-2),R(-2))=(7,7)
@wrong Substitute the same signed input into both original sides.
@why The original left side is -2(-2)+3=4+3=7, and the right side is -2+9=7. This checks the only candidate obtained by reversible operations.
@end

@question prod02_algebra_full_linear_q03 | Expand before collecting constants
@template choices.v1
@version 1
@goal Solve the original equation completely and verify the solution set.
@given 2(x-3)+1=9
@domain Work over the real numbers. All displayed denominators are nonzero constants. S is the complete solution set; L(t) and R(t) mean the original left and right expressions evaluated at t.
@read prod02_algebra_full_linear_r
@step 10 | Distribute and combine like terms on each side. Which simplified equation results?
@choice 12 | 2x-2=9
@choice 13 | 2x-7=9
@choice 11 | 2x-5=9
@answer 11
@feedback 12 | This leaves the inside -3 unmultiplied. Distribution gives 2(-3)=-6, then -6+1=-5.
@feedback 13 | The outside constant is +1, not -1. Combine -6+1=-5 after distributing.
@after 2x-5=9
@wrong Apply the bracket multiplier to both terms, then combine constants.
@why Distribution gives 2x+2(-3)+1=2x-6+1. The constants combine to -5. Rewriting an expression with the distributive law does not change its value at any real x.
@step 20 | Remove the constant on the variable side. Which equation results?
@choice 21 | 2x=14
@choice 22 | 2x=4
@choice 23 | 2x=9
@answer 21
@feedback 22 | Subtracting 5 from 9 has the wrong sign. Cancel -5 by adding 5 to both sides.
@feedback 23 | Removing -5 on only the left is not balanced. The right side becomes 9+5=14.
@after 2x=14
@wrong Add the opposite of the signed constant on both sides.
@why Add 5 to each side: -5+5=0 and 9+5=14. Subtracting 5 restores the preceding equation.
@step 30 | Divide by the nonzero coefficient. Which candidate results?
@choice 32 | x=28
@choice 31 | x=7
@choice 33 | x=\frac{1}{7}
@answer 31
@feedback 32 | Multiplication by 2 must be undone with division by 2, not another multiplication.
@feedback 33 | The required quotient is 14/2, not its reciprocal 2/14.
@after x=7
@wrong Divide both sides by the coefficient multiplying x.
@why Two is nonzero, so dividing gives 14/2=7. All transformations can be reversed, proving there is at most one solution.
@step 40 | What is the ordered pair of original values (L(7),R(7))?
@choice 42 | \left(12,9\right)
@choice 43 | \left(8,9\right)
@choice 41 | \left(9,9\right)
@answer 41
@feedback 42 | This evaluates 2(7)-3+1, which loses multiplication of the inside -3. Evaluate the original bracket first: 7-3=4.
@feedback 43 | This omits the outside +1. The left side is 2(4)+1=9.
@after S=\{7\},\quad (L(7),R(7))=(9,9)
@wrong Check the unexpanded original bracket and outside constant.
@why In the original expression, 2(7-3)+1=2(4)+1=9, matching the original right side 9. Existence and the earlier uniqueness argument give the complete singleton set.
@end

@question prod02_algebra_full_linear_q04 | Fractions on both sides
@template choices.v1
@version 1
@goal Solve the original equation completely and verify the solution set.
@given \frac{x+2}{3}=\frac{x-1}{2}
@domain Work over the real numbers. All displayed denominators are nonzero constants. S is the complete solution set; L(t) and R(t) mean the original left and right expressions evaluated at t.
@read prod02_algebra_full_linear_r
@step 10 | Multiply both complete sides by 6, then distribute. Which simplified equation results?
@choice 12 | 2x+2=3x-1
@choice 11 | 2x+4=3x-3
@choice 13 | 3x+6=2x-2
@answer 11
@feedback 12 | Each remaining multiplier acts on the whole numerator: 2(x+2)=2x+4 and 3(x-1)=3x-3.
@feedback 13 | The clearing factors are 6/3=2 on the left and 6/2=3 on the right, not the reverse.
@after 2x+4=3x-3
@wrong Divide 6 by each denominator, then multiply every numerator term.
@why Since 6 is nonzero, whole-side multiplication is reversible. Six divided by 3 is 2 and by 2 is 3, giving 2(x+2)=3(x-1). Distribution gives 2x+4=3x-3.
@step 20 | Subtract the right-hand variable term from both sides. Which simplified equation results?
@choice 22 | x+4=-3
@choice 23 | 2x+4=-3
@choice 21 | -x+4=-3
@answer 21
@feedback 22 | The left coefficient is 2-3=-1, not 3-2=1. Keep the subtraction order.
@feedback 23 | This removed 3x only from the right. Subtract 3x on the left too.
@after -x+4=-3
@wrong Subtract the same 3x from both sides.
@why Subtract 3x: the left coefficient becomes 2-3=-1 and the right variable terms cancel. Constants remain 4 and -3.
@step 30 | Remove the constant on the variable side. Which equation results?
@choice 31 | -x=-7
@choice 32 | -x=1
@choice 33 | -x=-3
@answer 31
@feedback 32 | Subtract 4 from -3, rather than adding it: -3-4=-7.
@feedback 33 | The constant cannot be deleted on only one side. Subtract 4 from both sides.
@after -x=-7
@wrong Subtract the left constant from both complete sides.
@why Subtracting 4 cancels 4 on the left and produces -3-4=-7 on the right. Adding 4 reverses the step.
@step 40 | Divide by the nonzero coefficient. Which candidate results?
@choice 42 | x=-7
@choice 41 | x=7
@choice 43 | x=\frac{1}{7}
@answer 41
@feedback 42 | The coefficient of x is -1. Dividing -7 by -1 gives +7, not -7.
@feedback 43 | This reverses the quotient: (-1)/(-7)=1/7, but the required division is (-7)/(-1)=7.
@after x=7
@wrong Treat the unwritten coefficient in -x as -1.
@why Divide both sides by nonzero -1: (-7)/(-1)=7. The entire route is reversible, so no other candidate is possible.
@step 50 | What is the ordered pair of original values (L(7),R(7))?
@choice 52 | \left(9,3\right)
@choice 53 | \left(3,6\right)
@choice 51 | \left(3,3\right)
@answer 51
@feedback 52 | The left numerator is 7+2=9, but the original side divides it by 3, giving 3.
@feedback 53 | The right numerator is 7-1=6, but the original side divides it by 2, giving 3.
@after S=\{7\},\quad (L(7),R(7))=(3,3)
@wrong Retain both original denominator divisions in the final check.
@why The original sides are (7+2)/3=9/3=3 and (7-1)/2=6/2=3. Equality confirms the unique candidate in the original, not merely the denominator-cleared equation.
@end

@question prod02_algebra_full_linear_q05 | A negative bracket with variables on both sides
@template choices.v1
@version 1
@goal Solve the original equation completely and verify the solution set.
@given -3(x-2)+4=2x-5
@domain Work over the real numbers. All displayed denominators are nonzero constants. S is the complete solution set; L(t) and R(t) mean the original left and right expressions evaluated at t.
@read prod02_algebra_full_linear_r
@step 10 | Distribute and combine like terms on each side. Which simplified equation results?
@choice 12 | -3x-2=2x-5
@choice 13 | -3x+2=2x-5
@choice 11 | -3x+10=2x-5
@answer 11
@feedback 12 | The inside product is (-3)(-2)=+6, not -6. Then add the outside 4 to get 10.
@feedback 13 | This leaves the inside -2 unmultiplied. Multiply both bracket terms by -3 before adding 4.
@after -3x+10=2x-5
@wrong Multiply the negative factor by both signed bracket terms.
@why Distribution gives -3x+(-3)(-2)+4=-3x+6+4=-3x+10. The right side is already simplified. This is an identity of expressions for every real x.
@step 20 | Subtract the right-hand variable term from both sides. Which equation results?
@choice 21 | -5x+10=-5
@choice 22 | -x+10=-5
@choice 23 | -3x+10=-5
@answer 21
@feedback 22 | Adding 2 to -3 gives -1, but the chosen operation subtracts 2x: -3-2=-5.
@feedback 23 | Subtract 2x from the left as well as the right; otherwise equality is changed.
@after -5x+10=-5
@wrong Subtract the right coefficient from the left coefficient in that order.
@why Subtract 2x from both sides. The left coefficient is -3-2=-5, and 2x-2x cancels on the right. This is reversed by adding 2x.
@step 30 | Remove the constant on the variable side. Which equation results?
@choice 32 | -5x=5
@choice 31 | -5x=-15
@choice 33 | -5x=-5
@answer 31
@feedback 32 | Adding 10 to -5 does not remove the existing +10. Subtract 10: -5-10=-15.
@feedback 33 | This drops the left constant without subtracting it from the right. Balance both sides.
@after -5x=-15
@wrong Subtract 10 from both complete sides.
@why Subtract 10 on both sides: 10-10=0 and -5-10=-15. The subtraction is reversible.
@step 40 | Divide by the nonzero coefficient. Which candidate results?
@choice 42 | x=-3
@choice 43 | x=\frac{1}{3}
@choice 41 | x=3
@answer 41
@feedback 42 | Both numerator and divisor are negative, so (-15)/(-5)=+3.
@feedback 43 | This is the reciprocal quotient. Divide the right side -15 by the coefficient -5.
@after x=3
@wrong Divide signed numbers in the correct order.
@why The coefficient -5 is nonzero. Dividing gives (-15)/(-5)=3, and reversibility makes this the only possible solution.
@step 50 | What is the ordered pair of original values (L(3),R(3))?
@choice 51 | \left(1,1\right)
@choice 52 | \left(-11,1\right)
@choice 53 | \left(1,6\right)
@answer 51
@feedback 52 | This uses -6 instead of +6 in the expanded left constant. In the original bracket, 3-2=1, so -3(1)+4=1.
@feedback 53 | The original right side includes -5: 2(3)-5=6-5=1.
@after S=\{3\},\quad (L(3),R(3))=(1,1)
@wrong Evaluate the original bracket and both outside constants.
@why The left side is -3(3-2)+4=-3+4=1, and the right is 2(3)-5=1. This verifies existence; the nonzero reduced coefficient and reversible route prove uniqueness.
@end

@question prod02_algebra_full_linear_q06 | Add two fractional expressions
@template choices.v1
@version 1
@goal Solve the original equation completely and verify the solution set.
@given \frac{2x-1}{3}+\frac{x+2}{2}=4
@domain Work over the real numbers. All displayed denominators are nonzero constants. S is the complete solution set; L(t) and R(t) mean the original left and right expressions evaluated at t.
@read prod02_algebra_full_linear_r
@step 10 | Multiply both complete sides by 6, distribute and combine like terms. Which equation results?
@choice 11 | 7x+4=24
@choice 12 | 7x+4=4
@choice 13 | 7x+8=24
@answer 11
@feedback 12 | Multiplication by 6 also applies to the original right side: 6(4)=24, not 4.
@feedback 13 | The first numerator contributes 2(-1)=-2, not +2. The constants are -2+6=4.
@after 7x+4=24
@wrong Multiply every original term by the common nonzero denominator.
@why Six is nonzero. Clearing denominators gives 2(2x-1)+3(x+2)=24. Distribution gives 4x-2+3x+6=24, so the coefficient is 4+3=7 and the constant is -2+6=4.
@step 20 | Remove the constant on the variable side. Which equation results?
@choice 22 | 7x=28
@choice 21 | 7x=20
@choice 23 | 7x=24
@answer 21
@feedback 22 | Cancel +4 by subtracting 4, not adding it. The right side is 24-4=20.
@feedback 23 | Dropping the constant only on the left is unbalanced. Subtract 4 from the right too.
@after 7x=20
@wrong Subtract the same added constant from both sides.
@why Subtract 4 from both sides: 4-4=0 and 24-4=20. The inverse operation adds 4.
@step 30 | Divide by the nonzero coefficient and retain an exact fraction. Which candidate results?
@choice 32 | x=\frac{7}{20}
@choice 33 | x=140
@choice 31 | x=\frac{20}{7}
@answer 31
@feedback 32 | The required quotient is 20/7, not 7/20. Divide the right side by the coefficient.
@feedback 33 | Multiplying 20 by 7 does not undo the factor 7 on x; divide by 7.
@after x=\frac{20}{7}
@wrong Preserve the exact quotient instead of rounding or reversing it.
@why Since 7 is nonzero, division gives x=20/7 exactly. Multiplying by 7 reverses this step, and the earlier transformations also preserve the full solution set.
@step 40 | What is the ordered pair of original values (L(20/7),R(20/7))?
@choice 41 | \left(4,4\right)
@choice 42 | \left(\frac{11}{7},4\right)
@choice 43 | \left(\frac{17}{7},4\right)
@answer 41
@feedback 42 | This includes only the first fraction. The second contributes (20/7+2)/2=17/7 and must also be added.
@feedback 43 | This includes only the second fraction. The first contributes (40/7-1)/3=11/7 and must also be added.
@after S=\{\frac{20}{7}\},\quad (L(\frac{20}{7}),R(\frac{20}{7}))=(4,4)
@wrong Evaluate and add both original fractional terms.
@why The first term is (40/7-1)/3=(33/7)/3=11/7. The second is (20/7+2)/2=(34/7)/2=17/7. Their sum is 28/7=4, equal to the original right side. Reversibility proves this is the unique solution.
@end

@question prod02_algebra_full_linear_q07 | Compare after expansion
@template choices.v1
@version 1
@goal Solve the original equation completely and verify the solution set.
@given 4(x-1)+2=4x+3
@domain Work over the real numbers. All displayed denominators are nonzero constants. S is the complete solution set; L(t) and R(t) mean the original left and right expressions evaluated at t.
@read prod02_algebra_full_linear_r
@step 10 | Distribute and combine like terms on each side. Which simplified equation results?
@choice 12 | 4x+2=4x+3
@choice 13 | 4x-6=4x+3
@choice 11 | 4x-2=4x+3
@answer 11
@feedback 12 | This omits the bracket's constant product 4(-1)=-4. Add the outside 2 afterward.
@feedback 13 | The outside constant is +2, not -2. Combine -4+2=-2.
@after 4x-2=4x+3
@wrong Multiply every bracket term, then combine signed constants.
@why Distribution gives 4x-4+2, and -4+2=-2. The resulting left expression is 4x-2 for every real input; the right side remains 4x+3.
@step 20 | Subtract the right-hand variable term from both sides. Which simplified equation remains?
@choice 21 | -2=3
@choice 22 | 0=0
@choice 23 | x=5
@answer 21
@feedback 22 | Only the matching variable terms cancel. The unequal constants -2 and 3 do not disappear.
@feedback 23 | Subtracting 4x from 4x leaves zero times x, not one times x. Keep the remaining constant comparison.
@after -2=3
@wrong Cancel variable terms without deleting constants or inventing a coefficient.
@why Subtract 4x on both sides: 4x-4x=0 on each side. The constants remain -2 and 3. The move is reversible by adding 4x; no division has been performed.
@step 30 | Which complete solution set follows from the remaining equality?
@choice 32 | S=\mathbb{R}
@choice 31 | S=\varnothing
@choice 33 | S=\{0\}
@answer 31
@feedback 32 | All real numbers would require a true constant equality. Here -2 and 3 are unequal, regardless of x.
@feedback 33 | Cancelling the variable does not imply x=0. The remaining false statement rules out every real value, including zero.
@after S=\varnothing
@wrong Decide whether the constant equality is true, without dividing by zero.
@why The statement -2=3 is false for every real input. Since the route used equivalent equations, no real value solves the original. Dividing by the cancelled zero coefficient is not permitted.
@step 40 | For an arbitrary real x, what is the original difference L(x)-R(x)?
@choice 42 | L(x)-R(x)=5
@choice 43 | L(x)-R(x)=0
@choice 41 | L(x)-R(x)=-5
@answer 41
@feedback 42 | This reverses the subtraction order. Left minus right gives -2-3=-5, not 3-(-2).
@feedback 43 | The variable terms cancel, but the constants differ: -2-3=-5.
@after S=\varnothing,\quad L(x)-R(x)=-5
@wrong Subtract the entire original right expression from the left.
@why From the original expressions, L(x)-R(x)=4x-4+2-(4x+3)=-4+2-3=-5 for every real x. A fixed nonzero difference proves that the sides never agree, confirming the empty solution set completely.
@end

@question prod02_algebra_full_linear_q08 | Collect matching variable terms
@template choices.v1
@version 1
@goal Solve the original equation completely and verify the solution set.
@given 2(x+3)-4=2x+2
@domain Work over the real numbers. All displayed denominators are nonzero constants. S is the complete solution set; L(t) and R(t) mean the original left and right expressions evaluated at t.
@read prod02_algebra_full_linear_r
@step 10 | Distribute and combine like terms on each side. Which simplified equation results?
@choice 11 | 2x+2=2x+2
@choice 12 | 2x-1=2x+2
@choice 13 | 2x+10=2x+2
@answer 11
@feedback 12 | This leaves the inside 3 unmultiplied. Its contribution is 2(3)=6, then 6-4=2.
@feedback 13 | The outside term is -4, not +4. Combine 6-4, not 6+4.
@after 2x+2=2x+2
@wrong Apply distribution before combining the outside signed constant.
@why The left side expands to 2x+6-4=2x+2. The right side is already 2x+2. Each expression simplification holds for every real x.
@step 20 | Subtract the right-hand variable term from both sides. Which simplified equation remains?
@choice 22 | x=0
@choice 21 | 2=2
@choice 23 | 0=2
@answer 21
@feedback 22 | The coefficient becomes 2-2=0, not 1. Cancellation leaves a comparison of constants, not a unique value of x.
@feedback 23 | Subtracting 2x removes only variable terms. The left constant 2 must remain.
@after 2=2
@wrong Retain both constants after subtracting the matching variable term.
@why Subtract 2x on both sides. Each variable coefficient becomes zero, while the two constants remain 2 and 2. Adding 2x reverses the step for every real input.
@step 30 | Which complete solution set follows from the remaining equality?
@choice 32 | S=\{0\}
@choice 33 | S=\varnothing
@choice 31 | S=\mathbb{R}
@answer 31
@feedback 32 | Zero is one possible input, but the true statement 2=2 imposes no restriction on x. The singleton omits other solutions.
@feedback 33 | The remaining equality is true, not a contradiction. It excludes no real input.
@after S=\mathbb{R}
@wrong A true constant equality imposes no restriction on the original real domain.
@why The statement 2=2 holds independently of x. Because all earlier steps were reversible, every real value solves the original equation. There is no need or permission to divide by a zero coefficient.
@step 40 | For an arbitrary real x, what is the original difference L(x)-R(x)?
@choice 41 | L(x)-R(x)=0
@choice 42 | L(x)-R(x)=4
@choice 43 | L(x)-R(x)=2
@answer 41
@feedback 42 | This adds the final right constant rather than subtracting it. The constant difference is 6-4-2=0.
@feedback 43 | This retains the left constant but omits subtracting the right constant 2. Both expressions must be included.
@after S=\mathbb{R},\quad L(x)-R(x)=0
@wrong Compare the entire original expressions, not a single trial value.
@why The original difference is 2x+6-4-(2x+2)=6-4-2=0 for every real x. This proves that the original sides agree for all real inputs, not just the example zero.
@end

@question prod02_algebra_full_linear_q09 | Choose a route for a difference of fractions
@template choices.v1
@version 1
@goal Solve the original equation completely and verify the solution set.
@given \frac{x-1}{2}-\frac{x+2}{3}=1
@domain Work over the real numbers. All displayed denominators are nonzero constants. S is the complete solution set; L(t) and R(t) mean the original left and right expressions evaluated at t.
@read prod02_algebra_full_linear_r
@step 10 | Choose the least positive integer k that clears every displayed denominator when multiplying both complete sides.
@choice 12 | k=2
@choice 11 | k=6
@choice 13 | k=3
@answer 11
@feedback 12 | Multiplying both sides by 2 is reversible, but 2/3 still leaves a fraction. It does not meet the denominator-clearing goal.
@feedback 13 | Multiplying both sides by 3 is reversible, but 3/2 still leaves a fraction. It does not clear both denominators.
@after k=6
@wrong The multiplier must be divisible by both 2 and 3.
@why The positive multiples of 2 begin 2,4,6; those of 3 begin 3,6. Their first common value is 6. It is nonzero, so multiplication of both sides by it is reversible.
@step 20 | Apply the selected multiplier to both complete sides, distribute and combine. Which equation results?
@choice 22 | x+1=6
@choice 23 | x-7=1
@choice 21 | x-7=6
@answer 21
@feedback 22 | The subtracted numerator contributes -2(x+2)=-2x-4. Its constant is -4, not +4; combine -3-4=-7.
@feedback 23 | The right side must also be multiplied by 6: 6(1)=6, not 1.
@after x-7=6
@wrong Carry the minus sign through both terms of the second numerator.
@why Clearing gives 3(x-1)-2(x+2)=6. Distribution gives 3x-3-2x-4=6. Combine 3x-2x=x and -3-4=-7. Every term was multiplied by the same nonzero 6.
@step 30 | Remove the constant on the variable side. Which equation results?
@choice 31 | x=13
@choice 32 | x=-1
@choice 33 | x=6
@answer 31
@feedback 32 | Cancel -7 by adding 7, not subtracting it: 6+7=13.
@feedback 33 | Deleting the left constant without adding 7 on the right is not balanced.
@after x=13
@wrong Add the opposite of the signed constant to both sides.
@why Add 7 to both sides: -7+7=0 and 6+7=13. The remaining coefficient is already 1, so no extra division is needed. The reversible steps leave exactly this candidate.
@step 40 | What is the ordered pair of original values (L(13),R(13))?
@choice 42 | \left(11,1\right)
@choice 41 | \left(1,1\right)
@choice 43 | \left(6,1\right)
@answer 41
@feedback 42 | This adds the two fractions. The original expression subtracts the second: 6-5=1.
@feedback 43 | This evaluates only the first fraction. Subtract the second fraction (13+2)/3=5 as well.
@after S=\{13\},\quad (L(13),R(13))=(1,1)
@wrong Preserve the original subtraction between the two fractions.
@why The left side is (13-1)/2-(13+2)/3=12/2-15/3=6-5=1. The right side is 1. The original equality and reversible route establish the unique solution set.
@end

@question prod02_algebra_full_linear_q10 | Select a method with a negative fractional group
@template choices.v1
@version 1
@goal Solve the original equation completely and verify the solution set.
@given 2-\frac{3x+1}{2}=\frac{x}{3}+3
@domain Work over the real numbers. All displayed denominators are nonzero constants. S is the complete solution set; L(t) and R(t) mean the original left and right expressions evaluated at t.
@read prod02_algebra_full_linear_r
@step 10 | Choose the least positive integer k that clears every displayed denominator when multiplying both complete sides.
@choice 11 | k=6
@choice 12 | k=2
@choice 13 | k=3
@answer 11
@feedback 12 | This is a reversible scaling, but x/3 becomes 2x/3. The denominator 3 is not cleared.
@feedback 13 | This is a reversible scaling, but the denominator 2 remains in 3(3x+1)/2. It misses the stated goal.
@after k=6
@wrong Select the least positive integer divisible by both denominators.
@why Six is the first positive common multiple of 2 and 3: 6/2=3 and 6/3=2 are integers. Because 6 is nonzero, whole-side multiplication preserves the solution set.
@step 20 | Apply the selected multiplier to every term, distribute and combine. Which equation results?
@choice 22 | -9x+9=2x+3
@choice 21 | -9x+9=2x+18
@choice 23 | -9x+15=2x+18
@answer 21
@feedback 22 | The outside right constant 3 must also be scaled by 6, giving 18. Clearing denominators applies to the whole side.
@feedback 23 | The left bracket is subtracted: -3(3x+1)=-9x-3. Its constant combines with 12 to give 9, not 15.
@after -9x+9=2x+18
@wrong Scale outside constants and distribute the negative bracket multiplier.
@why Multiplying by 6 gives 12-3(3x+1)=2x+18. Distribution produces 12-9x-3=-9x+9 on the left. The outside constants were scaled as 6(2)=12 and 6(3)=18.
@step 30 | Subtract the right-hand variable term from both sides. Which equation results?
@choice 32 | -7x+9=18
@choice 33 | -9x+9=18
@choice 31 | -11x+9=18
@answer 31
@feedback 32 | The left coefficient is -9-2=-11, not -9+2=-7. The chosen operation subtracts 2x.
@feedback 33 | Removing 2x only from the right changes the equation. Subtract it from the left too.
@after -11x+9=18
@wrong Subtract the same signed variable term on both sides.
@why Subtract 2x from both sides. The left coefficient becomes -9-2=-11, the right variable coefficient becomes 2-2=0, and the constants stay 9 and 18.
@step 40 | Remove the constant on the variable side. Which equation results?
@choice 41 | -11x=9
@choice 42 | -11x=27
@choice 43 | -11x=18
@answer 41
@feedback 42 | To cancel +9, subtract 9 rather than add it. The right side is 18-9=9.
@feedback 43 | The right side must change when removing 9 from the left. Apply subtraction to both sides.
@after -11x=9
@wrong Subtract the added constant from both complete sides.
@why Subtract 9: the left constant cancels and 18-9=9 on the right. Adding 9 back reverses this operation.
@step 50 | Divide by the nonzero coefficient and retain an exact fraction. Which candidate results?
@choice 52 | x=\frac{9}{11}
@choice 51 | x=-\frac{9}{11}
@choice 53 | x=-\frac{11}{9}
@answer 51
@feedback 52 | Nine divided by -11 is negative. Preserve the sign of the divisor.
@feedback 53 | This reverses the quotient. The right side 9 is divided by the coefficient -11, not the other way around.
@after x=-\frac{9}{11}
@wrong Divide in the correct order and keep the negative sign.
@why The coefficient -11 is nonzero, so division is permitted and reversible. Nine divided by -11 is -9/11. No other value can satisfy the reduced equation.
@step 60 | What is the ordered pair of original values (L(-9/11),R(-9/11))?
@choice 62 | \left(\frac{14}{11},\frac{30}{11}\right)
@choice 63 | \left(\frac{30}{11},\frac{36}{11}\right)
@choice 61 | \left(\frac{30}{11},\frac{30}{11}\right)
@answer 61
@feedback 62 | The original left side subtracts a negative fraction. Its numerator is -27/11+1=-16/11, so subtracting (-16/11)/2 adds 8/11.
@feedback 63 | On the right, (-9/11)/3=-3/11, not +3/11. Add it to 3=33/11.
@after S=\{-\frac{9}{11}\},\quad (L(-\frac{9}{11}),R(-\frac{9}{11}))=(\frac{30}{11},\frac{30}{11})
@wrong Evaluate the original fractions with the signed candidate.
@why The left numerator is 3(-9/11)+1=-16/11. Thus L=2-(-16/11)/2=22/11+8/11=30/11. On the right, R=(-9/11)/3+3=-3/11+33/11=30/11. This checks the only candidate from the reversible route.
@end

@question prod02_algebra_full_linear_q11 | Compare fractional sides after clearing
@template choices.v1
@version 1
@goal Solve the original equation completely and verify the solution set.
@given \frac{x+4}{2}=\frac{x}{2}+3
@domain Work over the real numbers. All displayed denominators are nonzero constants. S is the complete solution set; L(t) and R(t) mean the original left and right expressions evaluated at t.
@read prod02_algebra_full_linear_r
@step 10 | Multiply both complete sides by 2 and simplify. Which equation results?
@choice 12 | x+4=x+3
@choice 13 | x+2=x+6
@choice 11 | x+4=x+6
@answer 11
@feedback 12 | The outside right constant is also multiplied by 2: 2(3)=6, not 3.
@feedback 13 | Multiplying the whole left side by 2 cancels its denominator for the entire numerator, leaving x+4 rather than x+2.
@after x+4=x+6
@wrong Clear the whole fraction and scale every term on both sides.
@why Two is nonzero. On the left, 2 times (x+4)/2 gives x+4. On the right, 2 times x/2 gives x and 2 times 3 gives 6. Dividing both sides by 2 reverses the step.
@step 20 | Subtract the right-hand variable term from both sides. Which simplified equation remains?
@choice 21 | 4=6
@choice 22 | 0=0
@choice 23 | x=2
@answer 21
@feedback 22 | The equal variable terms cancel, but the constants 4 and 6 must remain.
@feedback 23 | The coefficient of x becomes 1-1=0, not 1. The difference between constants does not become a unique value of x.
@after 4=6
@wrong Do not invent a variable coefficient after cancellation.
@why Subtract x from both sides. Each variable term becomes zero, leaving 4=6. This is an equivalent equation, reached without dividing by the zero coefficient.
@step 30 | Which complete solution set follows from the remaining equality?
@choice 32 | S=\{0\}
@choice 31 | S=\varnothing
@choice 33 | S=\mathbb{R}
@answer 31
@feedback 32 | Cancellation does not select zero. The false statement 4=6 excludes zero along with every other real input.
@feedback 33 | All real numbers would require an identity. Four and six are unequal, so the remaining statement is false.
@after S=\varnothing
@wrong Classify the remaining constant equality before any attempted division.
@why The statement 4=6 is false independently of x. Reversible transformations therefore prove that the original equation has no real solution.
@step 40 | For an arbitrary real x, what is the original difference L(x)-R(x)?
@choice 42 | L(x)-R(x)=1
@choice 43 | L(x)-R(x)=0
@choice 41 | L(x)-R(x)=-1
@answer 41
@feedback 42 | This reverses left minus right. The original constants contribute 4/2-3=2-3=-1.
@feedback 43 | Only the x/2 terms cancel. The original constants 2 and 3 still differ.
@after S=\varnothing,\quad L(x)-R(x)=-1
@wrong Check the original fractions, not only their scaled equation.
@why The original difference is (x+4)/2-(x/2+3)=x/2+2-x/2-3=-1 for every real x. Since it never equals zero, the original sides never agree, confirming that the complete set is empty.
@end

@question prod02_algebra_full_linear_q12 | Combine a negative bracket and another variable term
@template choices.v1
@version 1
@goal Solve the original equation completely and verify the solution set.
@given -2(x-3)+x=6-x
@domain Work over the real numbers. All displayed denominators are nonzero constants. S is the complete solution set; L(t) and R(t) mean the original left and right expressions evaluated at t.
@read prod02_algebra_full_linear_r
@step 10 | Distribute and combine like terms on each side, writing variable terms before constants. Which equation results?
@choice 12 | -x-6=-x+6
@choice 11 | -x+6=-x+6
@choice 13 | -3x+6=-x+6
@answer 11
@feedback 12 | The product (-2)(-3) is +6, not -6. Both factors are negative.
@feedback 13 | The outside variable term is +x. Its coefficient combines as -2+1=-1, not -2-1=-3.
@after -x+6=-x+6
@wrong Distribute the negative multiplier, then combine signed coefficients.
@why The left side expands to -2x+6+x. Combining coefficients gives (-2+1)x=-x, so it is -x+6. The right side 6-x is the same expression written with the variable term first.
@step 20 | Subtract the right-hand variable term from both sides. Which simplified equation remains?
@choice 22 | x=0
@choice 23 | 0=6
@choice 21 | 6=6
@answer 21
@feedback 22 | Subtracting -x means adding x. The coefficients become -1+1=0, not 1, so this does not isolate a unique x.
@feedback 23 | Adding x cancels the left variable term but does not delete the left constant 6.
@after 6=6
@wrong Subtract a negative term by adding its opposite, retaining constants.
@why The right variable term is -x. Subtracting it adds x on each side: -x+x=0. Both constants remain 6. This reversible addition requires no division.
@step 30 | Which complete solution set follows from the remaining equality?
@choice 31 | S=\mathbb{R}
@choice 32 | S=\varnothing
@choice 33 | S=\{0\}
@answer 31
@feedback 32 | Six equals six is true for every real input, not a contradiction.
@feedback 33 | Zero is only one of the permitted inputs. The identity places no restriction on the real variable.
@after S=\mathbb{R}
@wrong State every permitted solution, not just one example.
@why The true constant equality holds independently of x. Reversing the preceding identities and balanced operations proves that all real values solve the original equation; dividing by zero is neither valid nor needed.
@step 40 | For an arbitrary real x, what is the original difference L(x)-R(x)?
@choice 42 | L(x)-R(x)=12
@choice 41 | L(x)-R(x)=0
@choice 43 | L(x)-R(x)=6
@answer 41
@feedback 42 | Subtract the right constant 6 instead of adding it. The constant contribution is 6-6=0.
@feedback 43 | This omits the right constant from the subtraction. Both original expressions must be included.
@after S=\mathbb{R},\quad L(x)-R(x)=0
@wrong Negate every term of the original right side when subtracting it.
@why The original difference is -2x+6+x-(6-x)=-2x+x+x+6-6=0 for every real x. This establishes the original identity for the entire domain, not only at a sampled input.
@end
