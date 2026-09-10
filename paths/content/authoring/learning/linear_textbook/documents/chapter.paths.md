@paths 1
@subject algebra | Algebra
@chapter linear_textbook | Linear equations: a textbook companion

@lesson linear_textbook_reading | From equal values to an unknown
@template lesson.v2
@block introduction | start | - | Before you begin
@prose Our aim is to find the value of an unknown number and explain why it is the only value that works. We will read an equation, undo its operations one at a time, and test the result in the original equation.
@prose You need addition, subtraction, multiplication and division of small numbers. The first question uses positive integers. The second introduces subtracting a constant; the third asks you to recognize why a step is valid. All three offer answer buttons.
@prose Work over the real numbers. These examples have one unknown with a nonzero coefficient. Equations with the unknown on both sides, zero coefficients, or powers such as x squared need further discussion.
@endblock

@block definition | expression | 1.1 | An expression names a value
@prose An expression combines numbers, symbols and operations to describe a value. The letter x stands for a number whose value may not yet be known. It is called a variable; in the problem we are solving, it is the unknown.
@display
3x+5
@prose Writing a number immediately beside x means multiplication. Read this expression as "three times x, then add five". Multiplication is performed before the addition. It does not mean three times the sum of x and five.
@display
3x+5=3\cdot x+5
@prose To evaluate an expression, supply a value for x and carry out its operations. For example, replacing x by 2 gives:
@display
3(2)+5=6+5=11
@endblock

@block definition | parts | 1.2 | Coefficient, term and constant
@prose In this expression, the coefficient 3 multiplies x. The added constant is 5: its value does not depend on x. The expression has two terms, 3x and 5.
@display
\underbrace{3x}_{\text{variable term}}+\underbrace{5}_{\text{constant term}}
@prose A term is a part separated by addition or subtraction at the outermost level. Keep a minus sign with the term it belongs to. In 4x minus 7, the two terms are 4x and negative 7; the coefficient is 4.
@display
4x-7=4x+(-7)
@reference expression | 1.1 / Reading multiplication beside a letter
@endblock

@block definition | equality | 1.3 | An equation makes a claim
@prose An equation states that the expression on its left and the expression on its right have equal values. The equals sign is a relationship between those values. It is not an instruction to calculate only the next expression.
@display
3x+5=20
@prose Here the left side depends on x. We are looking for values of x that make the left side equal to 20. The value x = 2 does not work: it makes the left side 11 while the right side remains 20.
@prose The word "side" means the complete expression on one side of the equals sign. When we divide a side, we divide all of that expression, including any added constant.
@reference parts | 1.2 / Identify the terms in each side
@endblock

@block definition | solution | 1.4 | A solution makes the original equation true
@prose A solution is a value of the unknown that makes the equation true. The solution set is the collection of all such values. Solving an equation means finding that set, not merely producing another expression.
@prose Substitution means replacing the unknown with a proposed value. To check a solution, substitute it into the original equation and calculate both sides. If their values disagree, the proposed value is not a solution.
@prose One successful substitution proves that the value works. To conclude that no other value works, we will also explain why our solving steps preserve the entire solution set.
@reference equality | 1.3 / What the equals sign asserts
@endblock

@block proposition | balance | 1.5 | Reversible operations preserve the solutions
@prose Adding the same number to both complete sides, or subtracting the same number from both complete sides, preserves an equation's solution set. Multiplying or dividing both complete sides by the same nonzero number does too.
@prose Equal amounts remain equal after the same change. Reversibility is the second part of the argument: undoing the change recovers the old equation, so the new equation cannot introduce extra solutions.
@display
u=v\quad\Longleftrightarrow\quad u-k=v-k
@display
u=v\quad\Longleftrightarrow\quad \frac{u}{a}=\frac{v}{a}\qquad(a\ne0)
@prose Here u and v stand for the values of the two sides. The symbol k is the number subtracted; a is the divisor. The double arrow means that each equation implies the other.
@help proof
@prose If u equals v, subtracting k from each produces equal values. Conversely, adding k to each side of the new equation gives u = v again. The two implications establish exactly the same solutions.
@prose If a is nonzero, division by a can be reversed by multiplication by a. This establishes the same two implications for division. Division by zero is undefined, so zero cannot be used as the divisor.
@prose Multiplication by zero is defined, but it is not reversible. For example, x = 1 would become 0 = 0. The new equation allows every real x and has lost the original restriction.
@reference solution | 1.4 / Why preserving every solution matters
@endblock

@block definition | inverse | 1.6 | Undo operations in a useful order
@prose An inverse operation undoes another operation: subtraction undoes addition, and division by a nonzero number undoes multiplication by that number.
@prose In 3x + 5, a starting value x is multiplied by 3 and then increased by 5. A convenient solving plan reverses that sequence: first remove the added 5, then undo multiplication by 3.
@prose Isolating x means reaching an equivalent equation with x alone on one side. Choose a move for its purpose, and apply it to both sides using Proposition 1.5.
@prose This order is convenient, not compulsory. Dividing first would also be valid if the whole left side were divided. It introduces a fraction, so removing the constant first keeps this example simpler.
@reference balance | 1.5 / The rule that justifies both moves
@endblock

@block example | worked | 1.7 | Solve 3x + 5 = 20
@prose Identify the coefficient and added constant. Then choose which operation will remove the constant. Open the hint for a direction, the answer for the final value, or the solution for every intermediate calculation.
@display
3x+5=20
@help hint
@prose The added constant is positive 5. Its additive inverse is negative 5: together they make zero. Subtract 5 on both complete sides, then inspect the coefficient still multiplying x.
@help answer
@display
x=5
@help solution
@prose Step 1: remove the added constant. Subtract 5 from each complete side, writing the subtraction explicitly before simplifying.
@display
(3x+5)-5=20-5
@prose On the left, positive 5 and negative 5 sum to zero. On the right, 20 minus 5 is 15. The multiplication by 3 has not changed.
@display
3x+(5-5)=15
@display
3x=15
@prose Step 2: find the value of one x. The last equation says that three times x equals 15. Divide both sides by the nonzero coefficient 3.
@display
\frac{3x}{3}=\frac{15}{3}
@prose On the left, 3 divided by 3 is 1, so the result is 1 times x, which is x. On the right, 15 divided by 3 is 5.
@display
x=5
@prose Step 3: check in the original equation. Replace x by 5, multiply first, and then add the original constant.
@display
3(5)+5=15+5=20
@prose Both sides have value 20. Substitution confirms that 5 is a solution. Every solving step was reversible and the final equation x = 5 has just one solution, so the original equation has exactly that solution.
@reference parts | 1.2 / Which number multiplies x?
@reference balance | 1.5 / Why the moves preserve all solutions
@reference inverse | 1.6 / Why remove the constant first?
@endblock

@block example | sign | 1.8 | Removing a negative constant
@prose The same plan works when the constant is subtracted. Pay attention to its sign. To remove negative 7, add positive 7, since their sum is zero.
@display
-7+7=0
@prose For an equation with left side 4x - 7, subtracting another 7 would leave 4x - 14. That is a valid change if applied to both sides, but it does not remove the constant. Adding 7 to both sides is the direct cancellation.
@prose In Question 2, choose the value after this cancellation. The next step will undo multiplication by 4. The worked solution to that question stays closed in Exercise 1.10 below.
@reference parts | 1.2 / Keep the sign with the constant
@reference inverse | 1.6 / Choose the inverse operation
@endblock

@block example | mistake | 1.9 | Divide the whole side
@prose Suppose someone starts with the following equation and divides only the term containing x on the left, while dividing the complete right side.
@display
2x+8=18
@prose The claimed result below is incorrect:
@display
x+8=9
@prose Dividing the entire left side by 2 must divide both of its terms. Distribute the division across the sum:
@display
\frac{2x+8}{2}=\frac{2x}{2}+\frac{8}{2}
@display
x+4=9
@prose The original equation has solution x = 5. The incorrect equation x + 8 = 9 has solution x = 1, which gives 10 rather than 18 when substituted into the original. This is a concrete way to detect the lost equivalence.
@reference equality | 1.3 / What counts as a complete side?
@reference balance | 1.5 / Apply the same operation on both sides
@endblock

@block exercise | guided | 1.10 | Practise with 4x - 7 = 13
@prose In Question 2, first choose the right side after cancelling negative 7. Then choose the value of x. Before each click, identify the operation, its purpose, and why it is allowed.
@display
4x-7=13
@help hint
@prose Add 7 to both sides to remove the negative constant. The coefficient 4 stays attached to x. Then divide by that coefficient.
@help answer
@display
x=5
@help solution
@prose Add 7 to both complete sides. This produces zero from the constants on the left and 20 on the right.
@display
4x-7+7=13+7
@display
4x=20
@prose Divide both complete sides by the nonzero number 4.
@display
\frac{4x}{4}=\frac{20}{4}
@display
x=5
@prose Verify the answer in the original equation, including its negative constant.
@display
4(5)-7=20-7=13
@prose The check works, and reversibility proves that no other solution was lost or introduced.
@reference sign | 1.8 / Cancelling a negative constant
@reference worked | 1.7 / The same plan with a positive constant
@endblock

@block exercise | explain | 1.11 | Explain a change, not just its arithmetic
@prose Question 3 asks why a proposed subtraction preserves the solutions. A useful explanation must account for both directions: old solutions still work, and adding back the removed number recovers the old equation.
@prose A rule such as "remove every matching number" would confuse coefficients with constants. A rule that changes only the left side could change the solution. Look for the operation applied to both complete sides.
@help hint
@prose Revisit Proposition 1.5. Which answer describes both the matching change and its inverse?
@reference balance | 1.5 / Read or open the proof
@endblock

@block summary | carry | - | What to carry forward
@prose Read the equation before calculating. Identify the unknown, its coefficient and the signed constant. Choose an inverse operation, apply it to both complete sides, simplify, and repeat until the unknown is isolated. Finally substitute into the original equation.
@prose Use Learn for the current worked explanation and the open textbook. Practice keeps the answer buttons while closing the guidance; Help and Textbook remain available. Solve and Write offer optional written work. Changing the support level closes the textbook disclosures, but it does not erase guidance already used.
@prose The gold Given is the original problem. Cyan Working and the answer buttons stay together while this reading scrolls. A correct result becomes green and stays until you choose Next. Reading a solution does not itself complete the question.
@endblock
@practice linear_book_worked
@practice linear_book_guided
@practice linear_book_reason
@end

@question linear_book_worked | 01 · Understand each move
@template linear.v1
@version 1
@goal Solve for x. Read the current step in Learn; use the textbook for definitions and the full explanation.
@given 3x+5=20
@domain x is real.
@read linear_textbook_reading
@step 10 | Subtract 5 from both sides. What is the new right side?
@choice 11 | 25
@choice 12 | 15
@choice 13 | 20
@answer 12
@after 3x=15
@feedback 11 | 25 comes from adding 5 to 20. The left side needs subtraction to cancel its added 5; apply that same subtraction on the right.
@feedback 13 | 20 leaves the right side unchanged. Removing 5 from only the left can change the solutions. Subtract 5 from the right side too.
@wrong Subtract the added constant from both complete sides.
@hint The added 5 is separate from the coefficient 3. Cancel the added term while leaving 3x in place.
@why Subtracting 5 from both sides gives 3x = 15. Adding 5 back recovers the original equation, so the solutions are unchanged.
@definitions
The unknown x is the number we seek. In $3x+5$, the coefficient 3 multiplies x and the constant 5 is added afterward. An equation says its two complete sides have equal values. Textbook Definitions 1.1-1.4 explain these words with examples.
@teaching
Our purpose is to isolate x. First undo the last operation in $3x+5$: the addition of 5. Subtract 5 from both sides, so the equality and its solution set are preserved.

$$ (3x+5)-5=20-5 $$

The constants on the left add to zero. On the right, 20 minus 5 is 15.

$$ 3x+(5-5)=15 $$

$$ 3x=15 $$

The coefficient 3 remains. We have removed the addition, but have not yet undone the multiplication. Textbook Proposition 1.5 explains why the subtraction can be reversed.
@step 20 | Divide both sides by 3. What is x?
@choice 21 | 15
@choice 22 | 5
@choice 23 | 20/3
@answer 22
@after x=5
@feedback 21 | 15 is the value of three times x. Divide it by 3 to find the value of one x.
@feedback 23 | 20/3 uses the original right side. Working now shows 3x = 15 because the added constant was removed. Divide the current 15 by 3.
@wrong Divide the current right side by the coefficient 3.
@hint In 3x = 15, three copies of x total 15. Use division to find one copy.
@why Divide both sides by nonzero 3 to reach x = 5. Substituting gives 3(5) + 5 = 20. Reversible steps preserve the complete solution set.
@definitions
An inverse operation undoes an operation. Division by nonzero 3 undoes multiplication by 3. Substitution replaces the unknown with a proposed value so we can test the original equation.
@teaching
Working now says three times x is 15. Divide both complete sides by 3. This is allowed because 3 is nonzero, and multiplication by 3 can reverse the move.

$$ \frac{3x}{3}=\frac{15}{3} $$

On the left, $3/3=1$ and $1x=x$. On the right, $15/3=5$.

$$ x=5 $$

Check in the original equation, including its added constant:

$$ 3(5)+5=15+5=20 $$

Both sides agree. This verifies the candidate; reversibility also establishes that it is the unique solution. Textbook Example 1.7 collects the entire argument.
@end

@question linear_book_guided | 02 · Remove a negative constant
@template linear.v1
@version 1
@goal Solve for x. Try Practice for answer buttons with guidance closed, or Learn for the current explanation.
@given 4x-7=13
@domain x is real.
@read linear_textbook_reading
@step 10 | Add 7 to both sides to cancel -7. What is the new right side?
@choice 11 | 6
@choice 12 | 13
@choice 13 | 20
@answer 13
@after 4x=20
@feedback 11 | 6 comes from subtracting 7 from 13. The left constant is already negative 7, so add positive 7 on both sides to cancel it.
@feedback 12 | 13 leaves the right side unchanged. Adding 7 to the left requires the same addition to the right.
@wrong Add positive 7 to both complete sides.
@hint Keep the sign with the constant: negative 7 plus positive 7 is zero.
@why Adding 7 to both sides gives 4x = 20. Subtracting 7 again reverses this change, preserving the solutions.
@definitions
In $4x-7$, the coefficient is 4 and the constant term is negative 7. An additive inverse is a number that sums with another to make zero. Positive 7 is the additive inverse of negative 7.
@teaching
We want to remove the constant, so add positive 7 to both complete sides.

$$ 4x-7+7=13+7 $$

The left constants sum to zero; the right side becomes 20.

$$ 4x=20 $$

Subtracting another 7 would be reversible if done on both sides, but it would leave a constant of negative 14. Adding 7 is the useful move here. Textbook Example 1.8 explains this sign choice.
@step 20 | Divide both sides by 4. What is x?
@choice 21 | 5
@choice 22 | 20
@choice 23 | 13/4
@answer 21
@after x=5
@feedback 22 | 20 is the value of four times x. Divide it by the coefficient 4.
@feedback 23 | 13/4 divides the original right side. After the addition, the current equation is 4x = 20. Use that current value.
@wrong Divide the current right side 20 by 4.
@hint Four times x is 20. Undo multiplication by the nonzero coefficient 4.
@why Dividing both sides by 4 gives x = 5. The original equation checks as 4(5) - 7 = 13, and both steps were reversible.
@definitions
Isolating x means reaching an equivalent equation with x alone on one side. Equivalent equations have exactly the same solutions. A check substitutes the proposed value into the original equation.
@teaching
Divide both complete sides by 4, which is nonzero.

$$ \frac{4x}{4}=\frac{20}{4} $$

The left becomes x and the right becomes 5. Verify this value in the original equation:

$$ 4(5)-7=20-7=13 $$

The original minus sign matters in this check. The check establishes that 5 works; reversibility establishes uniqueness. Open Exercise 1.10 in the textbook for the complete sequence.
@end

@question linear_book_reason | 03 · Explain why subtraction is allowed
@template choices.v1
@version 1
@goal Choose the reason that establishes the same solutions before and after the subtraction.
@given 5x+6=21
@domain x is real.
@read linear_textbook_reading
@step 10 | Why does subtracting 6 from both sides give an equivalent equation?
@textchoice 11 | Every 6 must disappear
@textchoice 12 | Subtract equally; adding back reverses it
@textchoice 13 | Only the left side needs to change
@answer 12
@after 5x=15
@feedback 11 | Matching numbers do not disappear by a special rule. Subtracting the same 6 from both complete sides preserves equality; the coefficient 5 remains.
@feedback 13 | Changing only the left side can change which x values work. Both complete sides must undergo the same subtraction.
@wrong Account for the operation on both sides and the operation that reverses it.
@why If a value of x solves the original equation, subtracting 6 from both sides leaves a true equation. Conversely, adding 6 back to both sides of 5x = 15 recovers 5x + 6 = 21. Both implications establish identical solution sets; this question checks that reason rather than requiring the final value of x.
@end
