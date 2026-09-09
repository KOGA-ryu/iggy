@paths 1
@subject linear_algebra | Linear Algebra
@chapter document_matrix_rows | Matrix rows from documents

@lesson document_matrix_reading | Solving two equations with matrix rows
@template lesson.v1
An augmented matrix writes one equation in each row. The first two columns
are the coefficients of x and y. The column after the bar holds the constants.

$$\begin{aligned}x+y&=3\\2x-y&=0\end{aligned}$$

Adding a multiple of one row to the other row preserves the common solution.
Dividing an entire row by a nonzero number also preserves that solution.
Always apply an operation to all three entries, including the constant.

The aim is to make the coefficient columns the identity matrix. The right
column then gives x and y directly. Check both values in both original equations.
@practice document_matrix_question
@end

@question document_matrix_question | Document matrix practice
@template matrix.v1
@version 1
@goal Solve the two equations by row reduction.
@given [1, 1 | 3] [2, -1 | 0]
@domain Real x and y; the last column holds constants.

@step 10 | Eliminate x from row 2. Choose the multiple of row 1 to add.
@operation add_row_1_to_2
@choice 11 | 2
@choice 12 | -2
@choice 13 | -1
@answer 12
@after [1, 1 | 3] [0, -3 | -6]
@wrong The new first entry is 2 plus your multiplier times 1. Make it zero.
@why Adding -2 times row 1 to row 2 gives [0, -3 | -6]. Row 1 stays unchanged.
@definitions
A coefficient multiplies an unknown. Elimination makes a chosen coefficient
zero. R1 and R2 name complete equations, including their constants. Replacing
R2 by R2 plus a multiple of R1 is reversible by subtracting that same multiple.
@teaching
Each row lists the multipliers of x and y, followed by the constant. R1 and
R2 name those complete equations. Elimination cancels one multiplier by adding
a multiple of the other row; adding the opposite multiple reverses it.

The first coefficient in row 2 is 2 and the first coefficient in row 1 is 1.
Add -2 times row 1 to cancel the 2. Apply the same operation in every column:

$$2-2(1)=0,\quad -1-2(1)=-3,\quad 0-2(3)=-6.$$

The entire first row stays as it was. Only row 2 changes.

@step 20 | Make the leading entry of row 2 equal to 1. Choose its divisor.
@operation divide_row_2
@choice 21 | 3
@choice 22 | -6
@choice 23 | -3
@answer 23
@after [1, 1 | 3] [0, 1 | 2]
@wrong Divide -3 by the same nonzero number to obtain 1.
@why Dividing the complete second row by -3 gives [0, 1 | 2], so y=2.
@definitions
A pivot is a leading nonzero entry used in elimination. Scaling a row by a
nonzero factor preserves its solutions; multiplying back reverses it.
Division applies to every entry of the row, including the constant.
@teaching
The pivot is the leading nonzero entry of a row. Dividing the complete row by
that entry turns the pivot into 1. Multiplying back reverses this operation,
which is why the divisor must be nonzero.

The leading entry of row 2 is -3. Divide all three entries by -3:

$$0/(-3)=0,\quad (-3)/(-3)=1,\quad (-6)/(-3)=2.$$

This leaves the equation y=2. Row 1 has not changed.

@step 30 | Eliminate y from row 1. Choose the multiple of row 2 to add.
@operation add_row_2_to_1
@choice 31 | -1
@choice 32 | 1
@choice 33 | -2
@answer 31
@after [1, 0 | 1] [0, 1 | 2]
@wrong Make the second entry of row 1 zero: 1 plus your multiplier times 1.
@why Subtracting row 2 from row 1 gives x=1. Together with y=2, both original equations hold: 1+2=3 and 2(1)-2=0.
@definitions
The identity matrix has 1 on its diagonal and 0 elsewhere. With this coefficient
block, the first row states x equals its constant and the second states y equals
its constant. Substitution tests those values in the original equations.
@teaching
The identity coefficient block has 1 on the diagonal and 0 elsewhere. It makes
the first row an equation for x alone and the second an equation for y alone.

Add -1 times row 2 to row 1, across all three columns:

$$1-0=1,\quad 1-1=0,\quad 3-2=1.$$

Read x from the first row and y from the second. Substitute both values in
both original equations; a pair must satisfy them together.
@end
