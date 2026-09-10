@paths 1
@subject algebra | Algebra
@chapter worked_linear_practice | Worked linear practice

@lesson linear_family_v1_sample_e763914cd65d_reading | Balanced equations: sample
@template lesson.v2
@block introduction | start | - | Read, change, and check an equality
@prose Our task is to find a number that makes an equation true. We will name the parts of the equation, use operations that preserve exactly its solutions, and check the final number in the original equation. The same reasoning works with negative numbers and exact fractions.
@prose The examples below explain the method. Each exercise asks for one particular decision; the final exercise asks for a complete answer without supplying intermediate steps. The related reading is always available when you want it.
@endblock

@block definition | terms | 1.1 | What each symbol means
@prose An equation states that two expressions have equal values. The unknown x represents a real number to be found. A solution is a value of x that makes the equality true when substituted into the original expressions. The solution set contains every such value.
@display form
ax+b=c,\qquad a\ne0
@prose The coefficient a multiplies x. The signed added constant b is a separate term. The number c is the right-hand constant in this written form. In 4x-3=9, the coefficient is 4 and the added constant is -3. Subtraction of 3 can be written as addition of -3; it does not make the coefficient negative.
@display signed_terms
4x-3=4x+(-3)
@prose Equality is symmetric: if one expression equals another, the second equals the first. The placement of the variable expression on the page does not change the equation's solutions.
@display symmetry
4x-3=9\quad\Longleftrightarrow\quad9=4x-3
@endblock

@block proposition | balance | 1.2 | Why the same operation acts on both sides
@prose We have ax+b=c and want to remove the added constant without changing the solutions. Subtract b from both complete sides. The added b and subtracted b cancel on the variable side; evaluate c-b on the other side. Adding b to both sides reverses the change.
@display subtract
ax+b-b=c-b\quad\Longleftrightarrow\quad ax=c-b
@help proof
@prose Every solution of ax+b=c still satisfies the equality after b is subtracted from both sides. Conversely, every solution of ax=c-b satisfies the original equality after b is added back. The two equations therefore have exactly the same solutions. Applying a change to only one side does not provide this inverse argument.
@endblock

@block example | sign_contrast | 1.3 | Change the constant's sign and watch the inverse change
@prose Compare these equations with the same coefficient and the same solution. Only the sign of the added constant changes as the deliberate teaching feature; the right-hand constant changes to keep the solution fixed. For the first equation, subtract 3 from both sides. For the second, subtract -3, which adds 3 to both sides.
@display positive
4x+3=15\quad\Longrightarrow\quad4x=15-3=12
@display negative
4x-3=9\quad\Longrightarrow\quad4x=9-(-3)=12
@prose Both reach the same equation. To check either change, add its original signed constant back to both sides. This is why naming the sign of the added constant matters before choosing an operation.
@endblock

@block proposition | coefficient | 1.4 | Undo multiplication by a nonzero coefficient
@prose After removing the constant, we have ax=d and want x alone. Divide both complete sides by a. The coefficient becomes 1, and the right side becomes d/a. Multiplication by the same nonzero a reverses that step.
@display divide
ax=d,\quad a\ne0\quad\Longleftrightarrow\quad x=\frac{d}{a}
@prose A negative coefficient is still a multiplier. For example, dividing -4x=12 by -4 gives x=-3. Check the sign in the original product: (-4)(-3)=12. A fractional quotient is also a valid exact answer; no rounding is needed.
@display negative_coefficient
-4x=12\quad\Longleftrightarrow\quad x=-3
@prose The nonzero condition is necessary. Multiplying 4x=12 by zero gives 0=0 and loses the restriction on x. If the original coefficient were zero, the equation could instead hold for every real x or for none. Those cases require a different problem family.
@display boundaries
0x=0\text{ holds for every real }x;\qquad0x=1\text{ holds for none.}
@help proof
@prose If u and v both solve ax+b=c, subtracting those two equalities gives a(u-v)=0. Since a is nonzero, divide by a to obtain u-v=0, hence u=v. Thus the solution found by reversible operations is the only real solution.
@endblock

@block example | worked | 1.5 | Follow one complete example
@prose Solve the following equation. Its coefficient is 4, its added constant is -3, and its right-hand constant is 6. We want a value for x, with a reason for each change and a final substitution check.
@display worked_given
4x-3=6
@help hint
@prose First undo addition of -3 on both sides. Then undo multiplication by 4 on both sides. Keep the quotient as a fraction if it is not an integer.
@help answer
@display
x=\frac{9}{4}
@help solution
@prose Subtract the signed added constant -3 from both complete sides. Subtracting a negative adds its positive opposite. The left constant cancels, and the right side becomes 9.
@display
4x-3-(-3)=6-(-3)\quad\Longrightarrow\quad4x=9
@prose Divide both sides by nonzero 4. Multiplying by 4 again would recover the reached equation, so the division preserves exactly its solutions.
@display
\frac{4x}{4}=\frac{9}{4}\quad\Longrightarrow\quad x=\frac{9}{4}
@prose Check the result in the original equation, including the original -3. The two sides have equal values, so the candidate is a solution. The nonzero coefficient proves it is unique.
@display
4\left(\frac{9}{4}\right)-3=9-3=6
@endblock

@block example | goal | 1.6 | A valid move can miss a requested goal
@prose Suppose the goal is to remove the added constant from 4x+3=15 while keeping the coefficient 4. Subtracting 3 on both sides reaches 4x=12 and achieves that goal. Adding 3 to both sides reaches 4x+6=18: it preserves the solution set, but the constant has not been removed. Removing 3 on the left only reaches 4x=15 and changes the solution set.
@display compare
\begin{aligned}4x=12&\quad\text{equivalent and goal achieved}\\4x+6=18&\quad\text{equivalent, goal not achieved}\\4x=15&\quad\text{not equivalent}\end{aligned}
@prose More than one complete method can be valid. We could divide 4x+3=15 by 4 first, giving x+3/4=15/4, then subtract 3/4 to get x=3. Removing the constant first also gives x=3 and keeps the intermediate arithmetic integral. The question's stated goal determines which next move it asks you to choose.
@endblock

@block exercise | self_check | 1.7 | Recognize the structure when the sides are reversed
@prose Try the equation below. Use the same definitions and equality rules even though the variable expression appears on the right. This example has different numbers from the final exercise.
@display reverse_given
7=4x+2
@help hint
@prose Equality has the same meaning in either orientation. Remove the added constant on both sides, then undo the nonzero multiplier.
@help answer
@display
x=\frac{5}{4}
@help solution
@prose Subtract 2 from both sides to obtain 5=4x. Divide both sides by 4 to obtain 5/4=x. The resulting fraction is exact. Substitute it into the variable expression to check the original equality.
@display
4\left(\frac{5}{4}\right)+2=5+2=7
@endblock

@block summary | finish | 1.8 | A short route with a reason at every step
@prose Read the signed coefficient and added constant. Identify the goal of the next move. Apply an inverse operation to both complete sides and evaluate its arithmetic. Check that the reached equation meets the goal. Finish by substituting the proposed value into the original equality. If reviewing a mistaken solution, find its first invalid transformation before repairing the arithmetic.
@prose The exercises use choices for notation, a calculation, a useful move, an inverse, error repair and a fresh equation. The last question checks a new application of these ideas. Completing it once does not by itself show long-term retention or the ability to write an unaided proof.
@endblock
@practice linear_family_v1_sample_read_notation_c633381fc7db
@practice linear_family_v1_sample_worked_check_191af608c76b
@practice linear_family_v1_sample_choose_next_step_dc1c24b720b1
@practice linear_family_v1_sample_explain_step_8ce66ab16575
@practice linear_family_v1_sample_repair_error_191af608c76b
@practice linear_family_v1_sample_independent_804227bab05d
@end

@question linear_family_v1_sample_read_notation_c633381fc7db | 01 Read the signed terms
@template choices.v1
@version 1
@goal Identify the coefficient and the added constant in an ordered pair.
@given 6x+8=26
@domain The unknown x is real. Match ax+b=c. In the ordered pair (a,b), a is the coefficient multiplying x and b is the separate signed added constant.
@read linear_family_v1_sample_e763914cd65d_reading
@step 10 | Which pair gives (a,b), in that order?
@choice 13 | (8,6)
@choice 12 | (6,-8)
@choice 11 | (6,8)
@answer 11
@feedback 12 | The original added term is positive 8. Replacing it by -8 changes the expression. Include the sign actually written.
@feedback 13 | This pair swaps the two jobs. The number multiplying x is 6; the separate added constant is 8.
@after (a,b)=(6,8)
@wrong Match each number to its job in ax+b=c before doing any arithmetic.
@why The term 6x is multiplication, so a=6. The separate term is +8, so b=8. This identifies the expression's parts; it does not yet solve the equation.
@end

@question linear_family_v1_sample_worked_check_191af608c76b | 02 Remove a negative constant
@template choices.v1
@version 1
@goal Evaluate the right side after removing a negative added constant.
@given 6x-8=10
@domain The unknown x is real. The added constant is -8. The worked step subtracts that same signed constant from both complete sides.
@read linear_family_v1_sample_e763914cd65d_reading
@step 10 | After the constant cancels on the left, what is 10-(-8) on the right?
@choice 13 | 2
@choice 11 | 18
@choice 12 | 10
@answer 11
@feedback 12 | 10 is the unchanged original right side. Subtracting the constant on the left requires subtracting it on the right too. Evaluate 10-(-8).
@feedback 13 | 2 comes from adding the negative constant again. To undo addition of -8, subtract -8; that adds 8.
@after 6x=18
@wrong Subtracting a negative adds its positive opposite. Apply that calculation to the original right side.
@why We have 6x+(-8)=10 and want no added constant. Subtracting -8 from both sides is reversible by adding it back. The constant cancels and 10-(-8)=18, giving the reached equation. The coefficient stays 6. This is one step; division by the nonzero coefficient would finish the solution.
@end

@question linear_family_v1_sample_choose_next_step_dc1c24b720b1 | 03 Choose a useful balanced move
@template choices.v1
@version 1
@goal Remove the added constant while preserving the coefficient and the solution set.
@given -6x+8=-10
@domain The unknown x is real. The goal is specifically to remove the added constant, retain the coefficient multiplying x, and preserve exactly the original solutions.
@read linear_family_v1_sample_e763914cd65d_reading
@step 10 | Which reached equation satisfies all three parts of this goal?
@choice 12 | -6x+16=-2
@choice 13 | -6x=-10
@choice 11 | -6x=-18
@answer 11
@feedback 12 | Adding 8 to both sides is reversible and preserves the solution set. It leaves the added constant 16, so this valid operation misses the requested cancellation goal.
@feedback 13 | This removes 8 on the left but leaves the right side unchanged. Apply the subtraction to both complete sides; otherwise the original solution no longer satisfies the reached equation.
@after -6x=-18
@wrong Check both equivalence and the stated goal. Preserving equality alone is not enough to remove the added constant.
@why Subtracting 8 from both sides cancels the added term and gives -10-8=-18 on the right. Adding 8 back recovers the given, proving equivalence. The coefficient remains -6; its negative sign belongs to the multiplication and is not cancelled with the constant.
@end

@question linear_family_v1_sample_explain_step_8ce66ab16575 | 04 Explain the inverse
@template choices.v1
@version 1
@goal Choose the inverse that recovers the equation before division.
@given -6x=18\quad\Longrightarrow\quad x=-3
@domain The unknown x is real. The arrow shows division of both complete sides by -6. Each choice below acts on both complete sides of the equation after the arrow.
@read linear_family_v1_sample_e763914cd65d_reading
@step 10 | Which operation reverses the division and restores the equation before the arrow?
@choice 11 | \times(-6)
@choice 13 | \times0
@choice 12 | \div(-6)
@answer 11
@feedback 12 | Dividing by -6 again is valid because it is nonzero, but it repeats the division. It does not restore the preceding coefficient. Undo division using multiplication by that divisor.
@feedback 13 | Multiplying by zero produces 0=0, which every real number satisfies. It erases the original restriction and cannot restore it.
@after -6x=18
@wrong An inverse must recover the actual preceding equation. A valid operation can still be the wrong inverse for this goal.
@why Multiplication by -6 undoes division by -6. It restores the coefficient of x to -6 and the right side to 18. Because the multiplier is nonzero, either equation can be recovered from the other. This demonstrates reversibility; choosing an inverse does not require writing a general proof.
@end

@question linear_family_v1_sample_repair_error_191af608c76b | 05 Find and repair the first error
@template choices.v1
@version 1
@goal Identify the first invalid transformation and repair its right side.
@given \begin{gathered}6x-8=10\\\begin{aligned}L_1 &: 6x=10+(-8)\\L_2 &: 6x=2\\L_3 &: x=\frac{1}{3}\end{aligned}\end{gathered}
@domain The first equation is the original problem with real unknown x. L1, L2 and L3 are a student's deliberately incorrect attempt, supplied for diagnosis. They are not accepted working.
@read linear_family_v1_sample_e763914cd65d_reading
@step 10 | Which is the first line that fails to preserve the original solution set?
@choice 11 | L_1
@choice 12 | L_2
@choice 13 | L_3
@answer 11
@feedback 12 | L2 correctly evaluates the expression already written in L1. Check whether L1 used the correct inverse operation before judging that arithmetic.
@feedback 13 | L3 correctly divides the equation in L2 by the nonzero coefficient. Its answer is wrong for the original problem because an earlier line changed that problem.
@after L_1
@wrong Read from the original equation downward. Find the first invalid transformation, even if later arithmetic follows it correctly.
@why L1 removes the negative constant on the left but adds that negative constant again on the right. Undoing addition requires subtraction of the same signed constant on both sides. L2 and L3 correctly follow the mistaken expression; they do not introduce the first error.
@step 20 | Repair the right side: what is 10-(-8)?
@choice 21 | 18
@choice 22 | 2
@choice 23 | 10
@answer 21
@feedback 22 | 2 repeats the original sign error. Subtracting -8 adds its opposite, 8.
@feedback 23 | 10 leaves the original right side untouched. Apply the balancing operation on that side too.
@after 6x=18
@wrong Calculate the inverse operation using the signed constant from the original equation.
@why The repaired right side is 18, so the equation is 6x=18. Dividing both sides by nonzero 6 gives x=3. Substitution checks the original: 6(3)+(-8)=10. Keeping the original constant in this check is essential.
@end

@question linear_family_v1_sample_independent_804227bab05d | 06 Solve a fresh equation
@template choices.v1
@version 1
@goal Select the exact value that satisfies the original equality.
@given 11=6x+8
@domain The unknown x is real. Each option is a candidate value. A fraction is an exact number.
@read linear_family_v1_sample_e763914cd65d_reading
@step 10 | Which value solves this equation?
@choice 13 | x=3
@choice 12 | x=-\frac{1}{2}
@choice 11 | x=\frac{1}{2}
@answer 11
@feedback 12 | With this candidate the variable expression evaluates to 5, while the constant side is 11. The signs matter when testing an equality.
@feedback 13 | 3 is the value of the product 6x after the added constant is removed. It is not x itself. With this choice the original variable expression would equal 26, not 11.
@after x=\frac{1}{2}
@wrong A solution must give equal values to the complete original expressions. The related reading remains available if you want help.
@why Substituting 1/2 gives 6(1/2)+8=11, matching the constant side. Reversing the written sides of an equality does not change its solutions. The nonzero coefficient also ensures uniqueness: two solutions would differ by a number whose product with 6 is zero, so their difference must be zero.
@end
