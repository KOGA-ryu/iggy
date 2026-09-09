@paths 1
@subject linear_algebra | Linear Algebra
@chapter matrix_reference | Matrix reference

@lesson matrix_reference_reading | Fractional solutions
@template lesson.v2
@block introduction | reference_start | 1 | Solve two equations together
@prose Each row represents one equation. Find the pair of values that satisfies both equations. Keep fractions exact throughout the calculation.
@endblock
@block definition | reference_rows | 1.1 | Read an augmented matrix
@prose The first two columns contain the coefficients of the unknowns. The column after the bar contains the constants. A row operation acts on every entry of a row, including its constant.
@display
\left[\begin{array}{cc|c}a&b&c\end{array}\right]\quad\longleftrightarrow\quad ax+by=c
@endblock
@block introduction | reference_controls | 1.2 | Choose your support
@prose Learn explains the current step. Practice keeps the symbolic choices and opens help on request. Terms defines the notation; Hint gives a direction; Next line reveals one reached matrix; Solution reveals the full reference route. Solve and Write accept your own working. Completion stays until Next.
@endblock
@practice matrix_reference_fraction_01
@end

@question matrix_reference_fraction_01 | Fractional solutions · Exercise 1
@template matrix.v1
@version 1
@goal Find the common solution of the two equations by row reduction.
@given [1, 2 | -10/3] [-3, -8 | 13]
@domain Real x and y; the column after the bar holds constants. Keep fractions exact.

@step 10 | Cancel x in row 2. Choose the multiple of row 1 to add.
@operation add_row_1_to_2
@choice 11 | 3
@choice 12 | 6
@choice 13 | -3
@answer 11
@after [1, 2 | -10/3] [0, -2 | 3]
@hint
Compare the first entries of the two rows. Choose a multiplier that makes their sum zero, then apply the addition to every column. Keep the original first row.
@wrong Check the sign and size of the multiplier. The first entry of row 2 must become zero; changing only its constant cannot cancel the unknown.
@why The first unknown is eliminated from row 2. Row 1 is unchanged. Subtracting the same multiple of row 1 reverses the operation, so both systems have exactly the same solutions.
@definitions
A coefficient multiplies an unknown: in $ax$, the coefficient is $a$.
The labels $R_1$ and $R_2$ refer to complete equations, including their constants.

Elimination makes a chosen coefficient zero. A row addition has the form

$$R_2\leftarrow R_2+mR_1.$$

Subtracting the same multiple reverses this change. A zero multiplier leaves the row unchanged; to eliminate a nonzero entry, choose a multiplier that cancels it.
@teaching
The first entries are $1$ in row 1 and $-3$ in row 2. Choose the multiplier $m$ so that

$$-3+m(1)=0.$$

Therefore $m=3$, and the operation is

$$R_2\leftarrow R_2+3R_1.$$

Apply it to the coefficient of each unknown and to the constant:

$$\begin{aligned}-3+3(1)&=0\\-8+3(2)&=-2\\13+3\left(-\frac{10}{3}\right)&=3.\end{aligned}$$

The reached matrix is

$$\left[\begin{array}{cc|c}1&2&-\frac{10}{3}\\0&-2&3\end{array}\right].$$

The first row stays unchanged. Only the second row has been replaced.

@step 20 | Make the leading entry of row 2 equal to 1. Choose its divisor.
@operation divide_row_2
@choice 21 | -4
@choice 22 | -2
@choice 23 | 2
@answer 22
@after [1, 2 | -10/3] [0, 1 | -3/2]
@hint
Divide the leading entry by itself to obtain one. Use that same nonzero divisor for every entry of the row, and keep track of its sign.
@wrong The leading entry divided by your chosen number must be one. Dividing by its opposite gives the wrong sign; dividing by twice its value gives half the required coefficient.
@why The leading coefficient in row 2 is now one, so this row gives the second unknown directly. Multiplying the entire row by the original nonzero divisor reverses the operation.
@definitions
A pivot is a leading nonzero entry used in elimination. Normalizing a pivot means making that entry equal to $1$.

For a nonzero divisor $d$, row division is

$$R_2\leftarrow\frac{1}{d}R_2,\qquad d\ne0.$$

Every entry must be divided by the same number. Division by zero is undefined. An exact fraction retains its numerator and denominator instead of rounding to a decimal.
@teaching
The pivot is $-2$. Divide the complete second row by $-2$:

$$R_2\leftarrow-\frac{1}{2}R_2.$$

The three entries become

$$\begin{aligned}\frac{0}{-2}&=0\\\frac{-2}{-2}&=1\\\frac{3}{-2}&=-\frac{3}{2}.\end{aligned}$$

The reached matrix is

$$\left[\begin{array}{cc|c}1&2&-\frac{10}{3}\\0&1&-\frac{3}{2}\end{array}\right].$$

Its second row now says $y=-\frac{3}{2}$. Keep the minus sign and the exact fraction. The first row stays unchanged.

@step 30 | Cancel y in row 1. Choose the multiple of row 2 to add.
@operation add_row_2_to_1
@choice 31 | -2
@choice 32 | 2
@choice 33 | -4
@answer 31
@after [1, 0 | -1/3] [0, 1 | -3/2]
@hint
Use the second row's unit coefficient to cancel the second entry of row 1. Choose the opposite multiple, then apply it to all three entries of row 1.
@wrong The second entry of row 1 must become zero. Adding the same sign or twice the cancelling multiple will leave a nonzero entry.
@why The coefficient block is now the identity matrix, so each row isolates one unknown. Substitution into both original equations confirms the pair. The reversible operations preserve the full solution set, which proves this solution is unique.
@definitions
The identity coefficient matrix has $1$ on its diagonal and $0$ elsewhere:

$$I=\begin{bmatrix}1&0\\0&1\end{bmatrix}.$$

With this coefficient block, the constants give the unknowns directly. A simultaneous solution must satisfy both original equations. Substitution verifies a candidate; reversible row operations show that no other solutions were lost or added.
@teaching
The second entries are $2$ in row 1 and $1$ in row 2. Choose $m$ so that

$$2+m(1)=0.$$

Thus $m=-2$, and the operation is

$$R_1\leftarrow R_1-2R_2.$$

The entries of row 1 become

$$\begin{aligned}1-2(0)&=1\\2-2(1)&=0\\-\frac{10}{3}-2\left(-\frac{3}{2}\right)&=-\frac{10}{3}+3=-\frac{1}{3}.\end{aligned}$$

The final matrix and answer are

$$\left[\begin{array}{cc|c}1&0&-\frac{1}{3}\\0&1&-\frac{3}{2}\end{array}\right],\qquad x=-\frac{1}{3},\quad y=-\frac{3}{2}.$$

Check the first original equation:

$$-\frac{1}{3}+2\left(-\frac{3}{2}\right)=-\frac{1}{3}-3=-\frac{10}{3}.$$

Check the second original equation:

$$-3\left(-\frac{1}{3}\right)-8\left(-\frac{3}{2}\right)=1+12=13.$$

Both original equations hold. The reversible reductions reached an identity coefficient matrix, so this is their unique common solution.
@end
