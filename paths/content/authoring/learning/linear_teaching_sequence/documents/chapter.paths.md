@paths 1
@subject algebra | Algebra
@chapter linear_teaching_sequence | Linear equations: understand each move

@lesson linear_teaching_overview | Start here: balance, choose, explain
@template lesson.v2
@block introduction | goal | 1 | What you will learn
@prose Solve an equation by preserving equality, choose a useful operation, and check an answer in the original equation. You will also explain a rule and repair a mistake. Every activity offers buttons; typing is optional.
@endblock
@block definition | prerequisites | 1.1 | Before you start
@prose You need addition, subtraction, multiplication and division of small numbers. The final two cards use negative numbers and an exact fraction. An unknown is a value we are trying to find. A coefficient multiplies it; an added constant is a separate term.
@display
9x+9=36
@prose Here the coefficient and added constant both happen to be 9. They have different jobs: the first multiplies x and the second is added afterward.
@endblock
@block proposition | balance | 1.2 | Keep both sides equal
@prose Equal expressions stay equal when the same number is added to or subtracted from both. Dividing both complete sides by the same nonzero number also preserves equality. These changes can be reversed.
@display
u=v\ \Longrightarrow\ u-k=v-k,\qquad u=v\ \Longrightarrow\ \frac{u}{a}=\frac{v}{a}\quad(a\ne0)
@help proof
@prose Add k back to undo subtraction, or multiply by a to undo division. Each original solution still works and every transformed solution also works in the original equation.
@endblock
@block example | example | 1.3 | One worked example
@prose Remove the added constant first. Then undo the multiplication.
@display
9x+9=36\quad\Longrightarrow\quad9x=27\quad\Longrightarrow\quad x=3
@help solution
@prose Subtract 9 from both complete sides. Divide both complete sides by 9. Check the resulting value in the original equation.
@display
9(3)+9=36
@endblock
@block introduction | route | 1.4 | Follow the eight cards
@prose 01 is a worked example: use Learn and read its open explanation. For 02, choose Practice to keep the arithmetic guidance but close the explanation. Cards 03-05 ask for a reason, a useful operation and a repair. Cards 06-07 give fewer cues. Use Next after finishing a card; the finished working stays until you do.
@prose Reading a revealed answer is useful practice, but it is not an independent check. Getting a tile right is one piece of evidence; explaining the rule and solving a fresh problem give different evidence.
@endblock
@block summary | return | 1.5 | Come back to card 08
@prose After a break, or tomorrow, try 08 before reopening this reading. You may try it now, but that is immediate practice rather than a delayed check. This chapter does not schedule reminders or claim mastery from a completion mark.
@prose If the negative or fractional arithmetic is unfamiliar, return to Worked linear practice for those cases. Come back here to practise choosing and explaining the method.
@endblock
@practice linear_teach_01_worked
@practice linear_teach_02_guided
@practice linear_teach_03_reason
@practice linear_teach_04_method
@practice linear_teach_05_repair
@practice linear_teach_06_less_cued
@practice linear_teach_07_mixed
@practice linear_teach_08_return
@end

@question linear_teach_01_worked | 01 · Read a worked example
@template linear.v1
@version 1
@goal Solve for x. In Learn, read the worked step before choosing; this is an example, not an independent test.
@given 9x+9=36
@domain x is real.

@step 10 | Subtract 9 from both sides. Complete 9x = ?
@choice 11 | 45
@choice 12 | 36
@choice 13 | 27
@answer 13
@after 9x=27
@feedback 11 | 45 comes from adding 9 to 36. To cancel the added 9 on the left, subtract 9 from the right too.
@feedback 12 | 36 leaves the right side unchanged. Removing 9 on the left requires subtracting 9 on the right as well.
@wrong Subtract the added constant from both complete sides.
@hint Undo the addition before undoing the multiplication.
@why Subtracting 9 from both sides removes the added constant. Adding 9 back reverses the change.
@definitions
An equation says two expressions have equal values. Subtracting the same number from both sides preserves that equality. In $9x+9$, the first 9 multiplies x; the second 9 is added.
@teaching
Subtract 9 from both complete sides to remove the added constant.

$$9x+9-9=36-9.$$

The left constants cancel and the right side is 27:

$$9x=27.$$

The coefficient multiplying x is still 9.

@step 20 | Divide both sides by 9. Complete x = ?
@choice 21 | 4
@choice 22 | 3
@choice 23 | 27
@answer 22
@after x=3
@feedback 21 | 4 comes from dividing the original 36 by 9. Use the current right side, 27, after removing the constant.
@feedback 23 | 27 is the value of 9x. Divide that value by 9 to find one x.
@wrong Divide the current right side by the coefficient of x.
@hint Undo multiplication by using the same nonzero divisor on both sides.
@why Dividing both sides by 9 isolates x. Substitution gives 9 times 3 plus 9 equals 36, verifying the original equation.
@definitions
A coefficient multiplies the unknown. Dividing both sides by the same nonzero coefficient reverses that multiplication. Substitution means replacing x with a proposed value.
@teaching
Divide both complete sides by the nonzero coefficient 9:

$$\frac{9x}{9}=\frac{27}{9},\qquad x=3.$$

Check the value in the original equation:

$$9(3)+9=27+9=36.$$

Both sides agree. The reversible steps preserve all solutions, so x=3 is the unique solution.
@end

@question linear_teach_02_guided | 02 · Try the two balanced steps
@template linear.v1
@version 1
@goal Solve for x. Choose Practice to try the guided steps with explanations closed; Help remains available.
@given 5x+4=29
@domain x is real.

@step 10 | Remove the added 4 from both sides. Complete 5x = ?
@choice 11 | 33
@choice 12 | 25
@choice 13 | 29
@answer 12
@after 5x=25
@feedback 11 | 33 adds 4 to the right side. Cancelling the added 4 requires subtracting 4 from both sides.
@feedback 13 | The left side changed but 29 did not. Subtract 4 from the right side too.
@wrong Remove the same added constant from both sides.
@hint The inverse of adding 4 is subtracting 4.
@why Subtracting 4 from both sides leaves 5x=25; adding it back recovers the original equation.
@definitions
An inverse operation undoes another operation. Subtraction undoes addition when applied to both sides of an equation.
@teaching
Subtract 4 from both complete sides.

$$5x+4-4=29-4,\qquad5x=25.$$

The coefficient 5 stays attached to x.

@step 20 | Undo multiplication by 5. Complete x = ?
@choice 21 | 5
@choice 22 | 29/5
@choice 23 | 25
@answer 21
@after x=5
@feedback 22 | 29/5 uses the original constant on the right. After subtracting 4, divide the current right side, 25.
@feedback 23 | 25 equals five copies of x. Divide by 5 to find one copy.
@wrong Divide the current right side by 5.
@hint Use the right side shown in Working.
@why Divide both sides by 5. Substitution into the original gives 5 times 5 plus 4 equals 29.
@definitions
Dividing both sides by a nonzero coefficient preserves the solutions. Check a proposed solution in the original equation, including its added constant.
@teaching
Divide both complete sides by 5, then check:

$$\frac{5x}{5}=\frac{25}{5},\qquad x=5.$$

$$5(5)+4=25+4=29.$$
@end

@question linear_teach_03_reason | 03 · Explain why balance matters
@template choices.v1
@version 1
@goal Choose the rule that justifies subtracting 9 from both sides. This activity checks a reason, not a new calculation.
@given 9x+9=36
@domain x is real.
@read linear_teaching_overview
@step 10 | Why does subtracting 9 from both sides preserve the solutions?
@textchoice 11 | Every 9 must disappear
@textchoice 12 | Both sides change equally
@textchoice 13 | Only x terms can change
@answer 12
@after 9x=27
@feedback 11 | The 9 multiplying x must remain at this step. We remove the added 9 by subtracting it on both sides.
@feedback 13 | Constants can change too. The rule concerns the same operation on both complete sides, not a special permission to change x terms.
@wrong Equality must be preserved by the operation on both complete sides.
@why Equal expressions remain equal after subtracting the same number. Adding 9 back recovers the original equation, so no solutions were lost or added.
@end

@question linear_teach_04_method | 04 · Choose a useful operation
@template choices.v1
@version 1
@goal Choose an operation that directly removes the added constant, then solve.
@given 4x-7=13
@domain x is real.
@read linear_teaching_overview
@step 10 | Which operation directly cancels the added term -7?
@textchoice 11 | Subtract 7 on both sides
@textchoice 12 | Divide both sides by 4
@textchoice 13 | Add 7 to both sides
@answer 13
@after 4x=20
@feedback 11 | Subtracting 7 preserves equality, but changes -7 to -14. It does not cancel the added term.
@feedback 12 | Dividing by 4 is valid, but leaves an added term of -7/4. This question asks you to cancel the added term directly.
@wrong Choose the additive inverse of the added term.
@why Add 7 to both sides: -7+7 is zero and 13+7 is 20. Other equivalent routes can work; this question specifically asks for direct cancellation.
@step 20 | Which value of x follows from the current equation?
@choice 21 | 5
@choice 22 | 20
@choice 23 | 13/4
@answer 21
@after x=5
@feedback 22 | 20 is four copies of x. Divide by the coefficient 4 to find x.
@feedback 23 | 13/4 ignores the addition of 7 already recorded in Working. Use the current right side.
@wrong Use the coefficient and right-hand side of the current equation.
@why Divide 4x=20 by 4. Check in the original: 4 times 5 minus 7 equals 13.
@end

@question linear_teach_05_repair | 05 · Find and repair a mistake
@template choices.v1
@version 1
@goal A learner divides both sides by 6 and writes x+12=5. Identify what they missed, then finish the corrected equation.
@given 6x+12=30
@domain x is real.
@read linear_teaching_overview
@step 10 | In the proposed line x+12=5, which part missed the division by 6?
@textchoice 11 | The coefficient of x
@textchoice 12 | The right side
@textchoice 13 | The added constant
@answer 13
@after x+2=5
@feedback 11 | The coefficient was divided correctly: 6x divided by 6 is x. Check the other term on that same side.
@feedback 12 | The right side was divided correctly: 30 divided by 6 is 5. Check every term on the left.
@wrong Dividing a complete sum divides each of its terms.
@why The added constant must also be divided: 12 divided by 6 is 2. The corrected line is x+2=5. Division first is a valid alternative route when applied to every term.
@step 20 | Solve the corrected equation shown in Working.
@choice 21 | 7
@choice 22 | 3
@choice 23 | 5/2
@answer 22
@after x=3
@feedback 21 | 7 adds 2 to the right side. Undo the added 2 by subtracting it on both sides.
@feedback 23 | The 2 is added to x, not multiplying it. Use subtraction to undo that addition.
@wrong Distinguish an added constant from a coefficient.
@why Subtract 2 from both sides of x+2=5. Checking the original gives 6 times 3 plus 12 equals 30.
@end

@question linear_teach_06_less_cued | 06 · Choose your next line
@template choices.v1
@version 1
@goal Solve for x by choosing equivalent lines. The operation is yours to recognise.
@given 7x-5=16
@domain x is real.
@read linear_teaching_overview
@step 10 | Which next line preserves the original solution?
@choice 11 | 7x=11
@choice 12 | x-5=16/7
@choice 13 | 7x=21
@answer 13
@after 7x=21
@feedback 11 | 11 subtracts 5 from the right but removes -5 on the left. Cancelling -5 requires adding 5 to both sides.
@feedback 12 | Dividing by 7 must divide the added term too. That route would give x-5/7=16/7, not the proposed line.
@wrong Test whether the same operation was applied to both complete sides.
@why Adding 5 to both sides gives 7x=21. The rejected lines changed the original solution.
@step 20 | Which next line solves the current equation?
@choice 21 | x=21
@choice 22 | x=3
@choice 23 | x=7
@answer 22
@after x=3
@feedback 21 | 21 is the value of 7x. The coefficient must be undone to isolate x.
@feedback 23 | 7 is the coefficient, not the quotient. Divide the current right side by it.
@wrong Check the value by multiplying it by the coefficient.
@why Divide by 7 to obtain x=3. In the original, 7 times 3 minus 5 equals 16.
@end

@question linear_teach_07_mixed | 07 · Solve a fresh signed equation
@template choices.v1
@version 1
@goal Choose the value that satisfies the original equation. No operation is supplied.
@given -3x+6=15
@domain x is real.
@read linear_teaching_overview
@step 10 | Which value satisfies the original equation?
@choice 11 | x=3
@choice 12 | x=-7
@choice 13 | x=-3
@answer 13
@after x=-3
@feedback 11 | Substitution gives -9+6=-3, not 15. Check the sign when dividing by the negative coefficient.
@feedback 12 | Substitution gives 21+6=27, not 15. This result can come from adding the constant when it should be removed.
@wrong Substitute your candidate into the original equation and compare both sides.
@why Subtract 6 to get -3x=9, then divide by -3. Checking x=-3 gives -3 times -3 plus 6 equals 15. Reversible operations give the unique solution.
@end

@question linear_teach_08_return | 08 · Return later: an exact fraction
@template choices.v1
@version 1
@goal After a break, solve a fresh equation before opening Method. Trying it now is immediate practice; no reminder is scheduled.
@given 4x+3=5
@domain x is real. Keep the answer exact.
@read linear_teaching_overview
@step 10 | Which value satisfies the original equation?
@choice 11 | x=2
@choice 12 | x=1/2
@choice 13 | x=3/2
@answer 12
@after x=1/2
@feedback 11 | Substituting 2 gives 8+3=11, not 5. After removing 3, the remaining 2 is four copies of x.
@feedback 13 | Substituting 3/2 gives 6+3=9, not 5. Recheck the constant removed from the right before dividing.
@wrong Keep the quotient exact and substitute it into the original equation.
@why Subtract 3 to get 4x=2, then divide by 4. The exact quotient 2/4 equals 1/2.
@step 20 | Substitute your answer. What is the value of the original left side?
@choice 21 | 2
@choice 22 | 8
@choice 23 | 5
@answer 23
@after 4(1/2)+3=5
@feedback 21 | 2 is only the product 4 times 1/2. The original left side also includes the added 3.
@feedback 22 | 8 does not equal 4 times 1/2 plus 3. Multiplication by one half halves 4; it does not double it.
@wrong Evaluate the multiplication first, then include the original added constant.
@why Four times one half is 2, then 2+3=5. This equals the original right side and verifies the answer.
@end
