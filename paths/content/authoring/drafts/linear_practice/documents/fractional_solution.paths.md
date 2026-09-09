@paths 1
@subject algebra | Algebra
@chapter worked_linear_practice | Worked linear practice

@lesson linear_worked_v1_fractional_solution_reading | Fractional solutions
@template lesson.v2
@block introduction | fractional_solution_start | 1 | One unknown, balanced operations
@prose Find the real value that makes the original equation true. Apply the same operation to both sides and keep fractions exact.
@endblock
@block definition | fractional_solution_terms | 1.1 | Coefficient and constant
@prose A coefficient multiplies the unknown. The added constant is a separate term. A nonzero coefficient gives a unique solution.
@display
ax+b=c,\qquad a\ne0
@endblock
@block introduction | fractional_solution_controls | 1.2 | Choose your support
@prose Learn explains each balanced step. Practice keeps the symbolic choices and opens help on request. Terms defines notation; Hint gives a direction; Next line reveals one equation; Solution reveals the full route. Solve and Write accept your own working. Completion stays until Next.
@endblock
@practice linear_worked_v1_4e918a51594be31d0612884381c801e5
@practice linear_worked_v1_0c06f7f7e21899760d8471dff70e2822
@end

@question linear_worked_v1_4e918a51594be31d0612884381c801e5 | Fractional solutions · Exercise 1
@template linear.v1
@version 1
@goal Solve for x and check the result in the original equation.
@given 9x+1=-77/4
@domain x is real. Keep fractions exact.

@step 10 | Subtract (1) on both sides. Complete 9x = ?
@choice 11 | -81/4
@choice 12 | -73/4
@choice 13 | -77/4
@answer 11
@after 9x=-81/4
@hint
Identify the term added to the unknown's multiple. Undo that addition on both sides, paying attention to its sign. Compute the new right-hand side before dividing.
@wrong Check the sign of the added constant. Subtract it from the entire right-hand side as well as the left; erasing it on just one side changes the equation.
@why The added constant has been cancelled. The unknown is still multiplied by its coefficient. Adding the same constant back reverses this step, so the solution set is unchanged.
@definitions
An equation states that two expressions have equal values. Its solution makes that equality true.

In $ax+b=c$, $x$ is the unknown, $a$ is its coefficient, $b$ is the added constant and $c$ is the original right-hand side. The coefficient multiplies the unknown; the constant is a separate term.

Subtracting the same number from both sides preserves equality:

$$u=v\quad\Longrightarrow\quad u-b=v-b.$$

The inverse operation is addition of that number to both sides.
@teaching
The original equation is

$$9x+1=-\frac{77}{4}.$$

Here the coefficient is $9$, the added constant is $1$, and the right-hand side is $-\frac{77}{4}$. Subtract the added constant from both complete sides:

$$9x+1 -\left(1\right)=-\frac{77}{4} -\left(1\right).$$

Subtracting the added positive constant cancels it. On the left, the constant cancels:

$$(1)-(1)=0.$$

On the right, calculate

$$-\frac{77}{4} -\left(1\right)=-\frac{81}{4}.$$

The reached equation is

$$9x=-\frac{81}{4}.$$

The coefficient has not changed. Adding the original constant back to both sides recovers the starting equation.

@step 20 | Divide both sides by (9). Complete x = ?
@choice 21 | -9/4
@choice 22 | -81/4
@choice 23 | -77/36
@answer 21
@after x=-9/4
@hint
The unknown is still multiplied by a nonzero coefficient. Undo that multiplication on both sides. Divide the entire right-hand side, keeping its sign and any fraction exact.
@wrong Divide the complete right-hand side by the same coefficient as the left. Keep a negative divisor's sign and do not round a fractional answer.
@why Dividing both sides by the nonzero coefficient isolates the unknown. Multiplication by that coefficient reverses the step. Substitution into the original equation verifies the answer; reversibility establishes uniqueness.
@definitions
Dividing by a nonzero coefficient reverses multiplication. For $a\ne0$,

$$ax=d\quad\Longrightarrow\quad x=\frac{d}{a}.$$

The division must apply to both complete sides. Division by zero is undefined. An exact fraction retains its numerator and denominator instead of rounding to a decimal. A zero numerator divided by a nonzero number is zero.

Substitution replaces the unknown with a candidate value in the original equation. Equality after substitution verifies that value; reversible steps show that no other solutions were lost or added.
@teaching
The coefficient is $9$, which is nonzero. Divide both sides of the reached equation by it:

$$\frac{ 9x }{ 9 }=\frac{ -\frac{81}{4} }{ 9 }.$$

On the left, the coefficient divided by itself equals one. On the right, the exact quotient is

$$\frac{ -\frac{81}{4} }{ 9 }=-\frac{9}{4}.$$

Therefore

$$x=-\frac{9}{4}.$$

Check this value in the original equation, including its original added constant:

$$9\left(-\frac{9}{4}\right)+1=-\frac{77}{4}.$$

Both sides agree. Every step is reversible and the original coefficient is nonzero, so this is the unique solution.
@end

@question linear_worked_v1_0c06f7f7e21899760d8471dff70e2822 | Fractional solutions · Exercise 2
@template linear.v1
@version 1
@goal Solve for x and check the result in the original equation.
@given 6x+5=25/2
@domain x is real. Keep fractions exact.

@step 10 | Subtract (5) on both sides. Complete 6x = ?
@choice 11 | 15/2
@choice 12 | 35/2
@choice 13 | 25/2
@answer 11
@after 6x=15/2
@hint
Identify the term added to the unknown's multiple. Undo that addition on both sides, paying attention to its sign. Compute the new right-hand side before dividing.
@wrong Check the sign of the added constant. Subtract it from the entire right-hand side as well as the left; erasing it on just one side changes the equation.
@why The added constant has been cancelled. The unknown is still multiplied by its coefficient. Adding the same constant back reverses this step, so the solution set is unchanged.
@definitions
An equation states that two expressions have equal values. Its solution makes that equality true.

In $ax+b=c$, $x$ is the unknown, $a$ is its coefficient, $b$ is the added constant and $c$ is the original right-hand side. The coefficient multiplies the unknown; the constant is a separate term.

Subtracting the same number from both sides preserves equality:

$$u=v\quad\Longrightarrow\quad u-b=v-b.$$

The inverse operation is addition of that number to both sides.
@teaching
The original equation is

$$6x+5=\frac{25}{2}.$$

Here the coefficient is $6$, the added constant is $5$, and the right-hand side is $\frac{25}{2}$. Subtract the added constant from both complete sides:

$$6x+5 -\left(5\right)=\frac{25}{2} -\left(5\right).$$

Subtracting the added positive constant cancels it. On the left, the constant cancels:

$$(5)-(5)=0.$$

On the right, calculate

$$\frac{25}{2} -\left(5\right)=\frac{15}{2}.$$

The reached equation is

$$6x=\frac{15}{2}.$$

The coefficient has not changed. Adding the original constant back to both sides recovers the starting equation.

@step 20 | Divide both sides by (6). Complete x = ?
@choice 21 | 5/4
@choice 22 | 15/2
@choice 23 | 25/12
@answer 21
@after x=5/4
@hint
The unknown is still multiplied by a nonzero coefficient. Undo that multiplication on both sides. Divide the entire right-hand side, keeping its sign and any fraction exact.
@wrong Divide the complete right-hand side by the same coefficient as the left. Keep a negative divisor's sign and do not round a fractional answer.
@why Dividing both sides by the nonzero coefficient isolates the unknown. Multiplication by that coefficient reverses the step. Substitution into the original equation verifies the answer; reversibility establishes uniqueness.
@definitions
Dividing by a nonzero coefficient reverses multiplication. For $a\ne0$,

$$ax=d\quad\Longrightarrow\quad x=\frac{d}{a}.$$

The division must apply to both complete sides. Division by zero is undefined. An exact fraction retains its numerator and denominator instead of rounding to a decimal. A zero numerator divided by a nonzero number is zero.

Substitution replaces the unknown with a candidate value in the original equation. Equality after substitution verifies that value; reversible steps show that no other solutions were lost or added.
@teaching
The coefficient is $6$, which is nonzero. Divide both sides of the reached equation by it:

$$\frac{ 6x }{ 6 }=\frac{ \frac{15}{2} }{ 6 }.$$

On the left, the coefficient divided by itself equals one. On the right, the exact quotient is

$$\frac{ \frac{15}{2} }{ 6 }=\frac{5}{4}.$$

Therefore

$$x=\frac{5}{4}.$$

Check this value in the original equation, including its original added constant:

$$6\left(\frac{5}{4}\right)+5=\frac{25}{2}.$$

Both sides agree. Every step is reversible and the original coefficient is nonzero, so this is the unique solution.
@end
