@paths 1
@subject algebra | Algebra
@chapter document_algebra | Equations from documents

@lesson document_balance_reading | Keeping an equation balanced
@template lesson.v1
The goal is to isolate the unknown while keeping the equation's solutions.

@include shared/equality.inc.md

For a nonzero coefficient, dividing both sides by that coefficient reverses
multiplication. Check the final value in the original equation.
@practice document_balance_question
@end

@question document_balance_question | Document algebra practice
@template linear.v1
@version 1
@goal Solve for x.
@given 4x-3=17
@domain x is real.

@step 10 | Subtract -3 on both sides. Complete 4x = ?
@choice 11 | 14
@choice 12 | 20
@choice 13 | 17
@answer 12
@after 4x=20
@wrong Subtracting a negative adds its positive opposite. Keep both sides balanced.
@why Adding 3 to both sides cancels the -3 and leaves 4x=20.
@definitions
@include shared/equality.inc.md
@teaching
The added constant is -3. Undo it by subtracting -3, which is adding 3.
The left side becomes 4x because -3+3=0; compute 17+3 on the right.
@include shared/equality.inc.md

@step 20 | Divide both sides by 4. Complete x = ?
@choice 21 | 5
@choice 22 | 20
@choice 23 | 17/4
@answer 21
@after x=5
@wrong Divide the complete right-hand side by the same nonzero coefficient.
@why Dividing by 4 leaves x=5. In the original equation, 4(5)-3=17.
@definitions
A coefficient is the multiplier attached to an unknown. Division by a
nonzero coefficient undoes multiplication without changing the solutions.
@teaching
The coefficient is 4, so divide both complete sides of 4x=20 by 4.
Because 4 is nonzero, multiplication by 4 reverses this operation.
After isolating x, substitute it into the original equation to verify it.
@end
