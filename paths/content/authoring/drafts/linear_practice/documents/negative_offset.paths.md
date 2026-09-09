@paths 1
@subject algebra | Algebra
@chapter worked_linear_practice | Worked linear practice

@lesson linear_worked_v1_negative_offset_reading | Negative offsets
@template lesson.v2
@block introduction | negative_offset_start | 1 | One unknown, balanced operations
@prose Find the real value that makes the original equation true. Apply the same operation to both sides and keep fractions exact.
@endblock
@block definition | negative_offset_terms | 1.1 | Coefficient and constant
@prose A coefficient multiplies the unknown. The added constant is a separate term. A nonzero coefficient gives a unique solution.
@display
ax+b=c,\qquad a\ne0
@endblock
@block introduction | negative_offset_controls | 1.2 | Choose your support
@prose Learn explains each balanced step. Practice keeps the symbolic choices and opens help on request. Terms defines notation; Hint gives a direction; Next line reveals one equation; Solution reveals the full route. Solve and Write accept your own working. Completion stays until Next.
@endblock
@practice linear_worked_v1_1632b7c413d645500ba1c1d2837fa304
@practice linear_worked_v1_8ced78eeee3258ed29fabfe81dd8b20b
@end

@question linear_worked_v1_1632b7c413d645500ba1c1d2837fa304 | Negative offsets · Exercise 1
@template linear.v1
@version 1
@goal Solve for x and check the result in the original equation.
@given 6x-8=10
@domain x is real. Keep fractions exact.

@step 10 | Subtract (-8) on both sides. Complete 6x = ?
@choice 11 | 10
@choice 12 | 18
@choice 13 | 2
@answer 12
@after 6x=18
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

$$6x-8=10.$$

Here the coefficient is $6$, the added constant is $-8$, and the right-hand side is $10$. Subtract the added constant from both complete sides:

$$6x-8 -\left(-8\right)=10 -\left(-8\right).$$

Subtracting a negative number adds its positive opposite. On the left, the constant cancels:

$$(-8)-(-8)=0.$$

On the right, calculate

$$10 -\left(-8\right)=18.$$

The reached equation is

$$6x=18.$$

The coefficient has not changed. Adding the original constant back to both sides recovers the starting equation.

@step 20 | Divide both sides by (6). Complete x = ?
@choice 21 | 5/3
@choice 22 | 18
@choice 23 | 3
@answer 23
@after x=3
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

$$\frac{ 6x }{ 6 }=\frac{ 18 }{ 6 }.$$

On the left, the coefficient divided by itself equals one. On the right, the exact quotient is

$$\frac{ 18 }{ 6 }=3.$$

Therefore

$$x=3.$$

Check this value in the original equation, including its original added constant:

$$6\left(3\right)-8=10.$$

Both sides agree. Every step is reversible and the original coefficient is nonzero, so this is the unique solution.
@end

@question linear_worked_v1_8ced78eeee3258ed29fabfe81dd8b20b | Negative offsets · Exercise 2
@template linear.v1
@version 1
@goal Solve for x and check the result in the original equation.
@given 4x-6=10
@domain x is real. Keep fractions exact.

@step 10 | Subtract (-6) on both sides. Complete 4x = ?
@choice 11 | 10
@choice 12 | 4
@choice 13 | 16
@answer 13
@after 4x=16
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

$$4x-6=10.$$

Here the coefficient is $4$, the added constant is $-6$, and the right-hand side is $10$. Subtract the added constant from both complete sides:

$$4x-6 -\left(-6\right)=10 -\left(-6\right).$$

Subtracting a negative number adds its positive opposite. On the left, the constant cancels:

$$(-6)-(-6)=0.$$

On the right, calculate

$$10 -\left(-6\right)=16.$$

The reached equation is

$$4x=16.$$

The coefficient has not changed. Adding the original constant back to both sides recovers the starting equation.

@step 20 | Divide both sides by (4). Complete x = ?
@choice 21 | 16
@choice 22 | 5/2
@choice 23 | 4
@answer 23
@after x=4
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
The coefficient is $4$, which is nonzero. Divide both sides of the reached equation by it:

$$\frac{ 4x }{ 4 }=\frac{ 16 }{ 4 }.$$

On the left, the coefficient divided by itself equals one. On the right, the exact quotient is

$$\frac{ 16 }{ 4 }=4.$$

Therefore

$$x=4.$$

Check this value in the original equation, including its original added constant:

$$4\left(4\right)-6=10.$$

Both sides agree. Every step is reversible and the original coefficient is nonzero, so this is the unique solution.
@end
