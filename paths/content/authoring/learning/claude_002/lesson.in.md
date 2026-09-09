@paths 1
@subject linear_algebra | Linear Algebra
@chapter source_quadratic_systems | Three points, one formula

@lesson source_002_reading | 002: Three points, one formula
@template lesson.v2
@practice source_002_full_problem
@practice source_002_reduction

@block introduction | source_002_start | - | From observations to equations
@prose A formula can be nonlinear in its input and still give a linear system for its unknown coefficients. This lesson starts with three observations and asks you to recover one quadratic polynomial.
@display
f(x)=ax^2+bx+c,\qquad (-1,1),\ (0,0),\ (1,2)
@prose First form the linear system for a, b and c, then solve it and check the polynomial against all three observations. Work over the real numbers. You need substitution, signed arithmetic and fractions.
@prose Question 1 is the full problem in 13 short choices. Question 2 is separate row-reduction practice, with four support levels, after the original system has been reduced to two unknowns. The question stays beside your working while you solve.
@prose Adapted from the current source card 002 and the existing Paths guided route, attributed there to Meckes and Meckes, Linear Algebra, chapter 1, exercise 1.1.7.
@endblock

@block definition | source_002_roles | 002.1 | Inputs and unknown coefficients
@prose The coefficients a, b and c are the unknown numbers. The symbol x names the function input. Each observation supplies an input and its output; those values are known.
@display
(u,v)\text{ on the graph}\quad\Longleftrightarrow\quad f(u)=v
@prose For example, the point (-1,1) supplies input -1 and output 1. A symbol's role comes from what the problem gives and asks for. A letter does not always mean an unknown just because it did in another problem.
@endblock

@block definition | source_002_linearity | 002.2 | Linear in the quantities being solved for
@prose A linear equation in a, b and c is a sum of known multiples of those unknowns, equal to a known constant. No unknown coefficient is squared or multiplied by another unknown coefficient. Once an input is supplied, its square is also a known number.
@display 002.1
u^2a+ub+1c=v
@prose The constant term c has coefficient 1. It is not absent just because the 1 is usually left unwritten. A collection of equations required to hold together is a system. A solution satisfies every equation in that system.
@reference source_002_roles | Return to the roles of inputs and coefficients
@endblock

@block example | source_002_row | 002.3 | Turn one observation into one row
@prose Substitute the known input into each part of the polynomial, and put the observed output on the right. Keep the unknown order fixed as a, b, c. The vertical bar separates coefficients from the constant on the right.
@display 002.2
f(u)=v\quad\Longrightarrow\quad [\,u^2,\ u,\ 1\mid v\,]
@prose Repeat that construction once for each of the three supplied points. This produces three equations in the same three unknowns. Squaring a negative input gives a positive number; its unsquared value keeps its negative sign.
@reference source_002_linearity | Why these equations are linear
@endblock

@block exercise | source_002_full | 002.4 | Build the system and recover the polynomial
@prose Use Question 1 for the complete guided route. Identify the symbol roles, substitute the three observations, eliminate an unknown, recover the coefficients, and check the resulting function. The final choice asks what uniqueness means within this polynomial model.
@help hint
@prose Start with input 0. Its squared and unsquared terms vanish, leaving the constant term. For input -1, take care with the square before combining terms. Add the remaining two equations to cancel the opposite coefficients of b.
@help answer
@display
a=\frac32,\qquad b=\frac12,\qquad c=0,\qquad f(x)=\frac32x^2+\frac12x
@help solution
@prose The middle observation gives c=0. Substituting -1 and 1 supplies the other two equations. Add those two equations after substituting c=0, then recover b from either one.
@display 002.3
\begin{aligned}a-b+c&=1\\c&=0\\a+b+c&=2\end{aligned}
@display 002.4
\begin{aligned}a-b&=1\\a+b&=2\\2a&=3\\a&=\frac32\\b&=2-\frac32=\frac12\end{aligned}
@prose Check each original observation, including the point that made the constant term easy to find. This tests the original problem rather than only the last reduced equation.
@display 002.5
f(-1)=\frac32-\frac12=1,\qquad f(0)=0,\qquad f(1)=\frac32+\frac12=2
@reference source_002_row | Return to the observation-to-row construction
@endblock

@block definition | source_002_operations | 002.5 | Preserve the solution while changing the equations
@prose {{row_addition}}
@display
R_i\leftarrow R_i+kR_j,\qquad i\ne j
@prose {{row_scaling}}
@display
R_i\leftarrow R_i/k,\qquad k\ne0
@prose Reversibility explains why the solutions stay the same: subtract the multiple to undo an addition; multiply by the same nonzero number to undo a division. Apply either move to every entry of the target row, including the constant after the bar.
@endblock

@block exercise | source_002_reduced | 002.6 | Practise the smaller system at four support levels
@prose Question 2 begins after the constant coefficient has been found and removed from the other two equations. This is a separate reduction exercise. It does not ask you to construct the entire three-variable system again.
@prose The matrix workspace calls its two unknowns x and y. In this exercise only, x stands for coefficient a and y stands for coefficient b. Use t for the polynomial's input so that it is not confused with either matrix unknown.
@display
x=a,\qquad y=b,\qquad f(t)=at^2+bt+c
@prose Level 1 gives definitions, teaching and operation tiles. Level 2 asks for the numeric operand, with teaching available on request. Levels 3 and 4 let you enter complete matrix lines; the last level starts with the problem and your own working. Fractions stay exact. These controls check row-equivalent matrices and the final solution, not arbitrary written proofs about polynomials.
@help hint
@prose Subtract the first row from the second to remove the first variable. Divide the new second row by its pivot. Then use that row to remove the second variable from the first row. The goal is one isolated unknown in each row.
@help answer
@display
\begin{bmatrix}1&0&\frac32\\0&1&\frac12\end{bmatrix}
@prose The two matrix unknowns give a=3/2 and b=1/2. The earlier substitution supplied c=0. Substitute those three coefficients into all three original observations.
@reference source_002_operations | Review reversible row operations
@endblock

@block proposition | source_002_unique | 002.7 | The coefficients are unique within this model
@prose For these three distinct inputs, the coefficient matrix is invertible. Therefore exactly one coefficient vector fits these observations within the model of polynomials of degree at most two. This does not claim that every possible kind of function is determined by three points.
@help proof
@prose Keep unknown order a, b, c and row order -1, 0, 1. Expanding the determinant along the middle row gives the negative of the two-by-two minor. Its value is nonzero, so the linear system has a unique solution.
@display
M=\begin{bmatrix}1&-1&1\\0&0&1\\1&1&1\end{bmatrix},\qquad \det M=-\det\begin{bmatrix}1&-1\\1&1\end{bmatrix}=-2\ne0
@reference source_002_linearity | Recall the fixed unknown order and linear model
@endblock

@block summary | source_002_summary | - | What to carry forward
@prose Separate the supplied inputs from the unknown coefficients. Use one observation per equation, keep the constant coefficient, preserve the solution during elimination, and substitute the final coefficients into the original observations. Always say which model a uniqueness claim refers to.
@endblock
@end

{{prepared_route}}

@question source_002_reduction | 002: Reduced system, four support levels
@template matrix.v1
@version 1
@goal Find a and b after c=0 has been extracted. In this matrix workspace x=a and y=b; the polynomial input is t. This is the reduced-system practice, not the full setup task.
@given [1, -1 | 1] [1, 1 | 2]
@domain Real coefficients x=a and y=b. The constant c=0 is already given. The last column contains the right-hand constants.

@step 10 | Eliminate x from row 2. Choose the multiple of row 1 to add.
@operation add_row_1_to_2
@choice 11 | 1
@choice 12 | -1
@choice 13 | -2
@answer 12
@after [1, -1 | 1] [0, 2 | 1]
@wrong Make the first entry of row 2 zero: 1 plus your multiple times 1. Keep row 1 unchanged.
@why Adding -1 times row 1 to row 2 gives [0, 2 | 1]. Subtracting a negative second coefficient adds 1, so the second entry becomes 2.
@definitions A coefficient multiplies an unknown. R1 and R2 name complete equations. Here x=a and y=b; the polynomial's input t is not an unknown in this system.
@teaching {{row_addition}}
Use multiplier -1. Work across the entire second row:
$$1-1=0,\qquad 1-(-1)=2,\qquad 2-1=1.$$
The first row is unchanged. Adding the first row back reverses this move. The new second equation is 2y=1.

@step 20 | Make the leading entry of row 2 equal to 1. Choose its divisor.
@operation divide_row_2
@choice 21 | -2
@choice 22 | 2
@choice 23 | 1/2
@answer 22
@after [1, -1 | 1] [0, 1 | 1/2]
@wrong The leading entry is 2. Divide every entry of the row by the same nonzero number that turns 2 into 1.
@why Dividing row 2 by 2 gives [0, 1 | 1/2], so y=1/2. The constant must be divided as well as the coefficient.
@definitions A pivot is a leading nonzero entry used during elimination. A nonzero divisor is required so division is defined and reversible. A fraction such as 1/2 is exact.
@teaching {{row_scaling}}
The pivot is 2. Dividing every entry by 2 gives:
$$0/2=0,\qquad 2/2=1,\qquad 1/2=\frac12.$$
Multiplying the complete row by 2 reverses the step. Read the new equation as y=1/2; in the polynomial this is coefficient b.

@step 30 | Eliminate y from row 1. Choose the multiple of row 2 to add.
@operation add_row_2_to_1
@choice 31 | -1
@choice 32 | 1
@choice 33 | 2
@answer 32
@after [1, 0 | 3/2] [0, 1 | 1/2]
@wrong The second entry of row 1 is -1. Add a multiple of the pivot 1 to make it zero, and change the constant with the same operation.
@why Add row 2 to row 1. Then x=3/2 and y=1/2: 3/2-1/2=1 and 3/2+1/2=2. Thus a=3/2, b=1/2; c=0 was given at the start of this reduction practice.
@definitions The identity coefficient block has diagonal entries 1 and all other entries 0. It isolates one unknown per row. Substitution checks the original equations and observations.
@teaching {{row_addition}}
Use multiplier 1 across the first row:
$$1+0=1,\qquad -1+1=0,\qquad 1+\frac12=\frac32.$$
Subtracting row 2 reverses the move. Translate x and y back to a and b. With c=0, the polynomial is f(t)=(3/2)t^2+(1/2)t.
$$f(-1)=\frac32-\frac12=1,\qquad f(0)=0,\qquad f(1)=\frac32+\frac12=2.$$
Both the reduced system and all three original observations agree.
@end
