@paths 1
@subject linear_algebra | Linear Algebra
@chapter published_matrix_foundations | Matrix foundations

@lesson published_matrix_reading | Matrix foundations: two equations, one solution
@template lesson.v1
@include shared/row_rules.inc.md

$$\begin{aligned}x+y&=5\\2x-y&=1\end{aligned}$$

We will eliminate x from the second row, turn its leading entry into 1,
then eliminate y from the first row. The identity coefficient block lets
us read the answer directly. Substitution checks it in both original equations.
@practice published_matrix_question
@end

@question published_matrix_question | Matrix foundations practice
@template matrix.v1
@version 1
@goal Solve the two equations by row reduction.
@given [1, 1 | 5] [2, -1 | 1]
@domain Real x and y; the last column holds constants.

@step 10 | Eliminate x from row 2. Choose the multiple of row 1 to add.
@operation add_row_1_to_2
@choice 11 | 2
@choice 12 | -2
@choice 13 | -1
@answer 12
@after [1, 1 | 5] [0, -3 | -9]
@wrong The new first entry is 2 plus your multiplier times 1. Make it zero.
@why Adding -2 times row 1 to row 2 gives [0, -3 | -9]. Row 1 stays unchanged.
@definitions
A coefficient multiplies an unknown. Elimination makes a chosen coefficient
zero. R1 and R2 name complete equations, including their constants.
@teaching
@include shared/row_rules.inc.md

The first coefficient in row 2 is 2; the first coefficient in row 1 is 1.
Add -2 times row 1 to cancel the 2. Adding 2 times row 1 back reverses it.

$$2-2(1)=0,\quad -1-2(1)=-3,\quad 1-2(5)=-9.$$

The entire first row stays as it was. Only row 2 changes.

@step 20 | Make the leading entry of row 2 equal to 1. Choose its divisor.
@operation divide_row_2
@choice 21 | 3
@choice 22 | -9
@choice 23 | -3
@answer 23
@after [1, 1 | 5] [0, 1 | 3]
@wrong Divide -3 by the same nonzero number to obtain 1.
@why Dividing the complete second row by -3 gives [0, 1 | 3], so y=3.
@definitions
A pivot is a leading nonzero entry used in elimination. Dividing a row by
a nonzero number preserves its solutions; multiplying back reverses it.
@teaching
The pivot is the leading nonzero entry of a row. Here it is -3. Dividing
the complete row by this nonzero pivot turns it into 1. Apply the division
to every entry, including the constant. Multiplying back reverses the move.

$$0/(-3)=0,\quad (-3)/(-3)=1,\quad (-9)/(-3)=3.$$

This leaves the equation y=3. Row 1 has not changed.

@step 30 | Eliminate y from row 1. Choose the multiple of row 2 to add.
@operation add_row_2_to_1
@choice 31 | -1
@choice 32 | 1
@choice 33 | -2
@answer 31
@after [1, 0 | 2] [0, 1 | 3]
@wrong Make the second entry of row 1 zero: 1 plus your multiplier times 1.
@why Subtracting row 2 from row 1 gives x=2. With y=3, both original equations hold: 2+3=5 and 2(2)-3=1.
@definitions
The identity matrix has 1 on its diagonal and 0 elsewhere. Its first row
isolates x; its second row isolates y. Substitution checks the original equations.
@teaching
The identity coefficient block isolates one variable per row. Add -1 times
row 2 to row 1 across all three columns; adding row 2 back reverses it.

$$1-0=1,\quad 1-1=0,\quad 5-3=2.$$

Read x=2 from the first row and y=3 from the second. Substituting both values
gives 2+3=5 and 2(2)-3=1, so the pair satisfies both original equations.
@end
