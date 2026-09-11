@paths 1
@subject linear_algebra | Linear Algebra
@chapter worked_matrix_practice | Worked matrix practice

@lesson prod02_linear_full_systems_r | Solve and classify two-equation systems
@template lesson.v2
@block introduction | start | - | From two equations to every solution
@prose Learn to carry a system from its original augmented matrix through complete row operations to its entire solution set. The final answer might be one pair, no pair, or a family of pairs. Check which conclusion the original equations actually support.
@endblock
@block definition | terms | 2.1 | Columns, pivots and solution sets
@prose A coefficient a with subscripts i,j belongs to row i and unknown column j. Thus a11 and a12 multiply x and y in equation 1, while a21 and a22 multiply them in equation 2. The constants are b1 and b2. The augmented bar separates coefficients from constants. R1 and R2 name whole rows, not individual entries. A solution is one ordered pair (x,y) satisfying both equations simultaneously.
@display column_order
\left[\begin{array}{cc|c}a_{11}&a_{12}&b_1\\a_{21}&a_{22}&b_2\end{array}\right]\quad\longleftrightarrow\quad\begin{cases}a_{11}x+a_{12}y=b_1\\a_{21}x+a_{22}y=b_2\end{cases}
@prose A pivot is the first nonzero entry of a nonzero row in echelon form. In echelon form, zero rows are last and each lower pivot lies to the right of the one above it. Rank is the number of pivots, equivalently the number of independent rows. The coefficient matrix A omits the constants; the augmented matrix [A|b] includes the constant column b whose entries are b1 and b2. These two ranks need not be equal.
@prose The solution set S collects all permitted ordered pairs. A system is consistent if at least one pair exists. A free variable is an unknown whose column has no pivot in a consistent reduced system. A real parameter t can represent every value of that variable; it is not a particular number to guess.
@endblock
@block proposition | rule | 2.2 | Three reversible operations
@prose Swap both complete rows; divide a complete row by a nonzero number; or add a multiple of one old row to the other old row. Each operation preserves all common solutions in both directions. A second swap undoes a swap, multiplication undoes division, and the opposite multiple undoes row replacement. Apply the operation to both coefficients and the constant.
@display replacement_rule
R_2\leftarrow R_2+kR_1:\quad[a_{21},a_{22}|b_2]\longmapsto[a_{21}+ka_{11},a_{22}+ka_{12}|b_2+kb_1]
@prose If the first x entry is zero but the other x entry is nonzero, a swap can provide an x pivot. If the entire x column is zero, look for a y pivot instead. Keep fractions exact: a fractional multiple is just as reversible as an integer multiple. A valid move may preserve solutions while failing a particular goal, such as creating a unit pivot without introducing fractions.
@endblock
@block proposition | condition | 2.3 | Classify before dividing
@prose Never divide by a zero pivot. After elimination, inspect the whole augmented row. A row [0,0|r] with r nonzero asserts that zero equals a nonzero number, so there are no solutions. In that case the augmented rank exceeds the coefficient rank. The empty-set symbol below means that no real pair works.
@display inconsistent_rule
[0,0|r],\ r\ne0\quad\Longrightarrow\quad 0=r,\qquad S=\varnothing
@prose A row [0,0|0] imposes no restriction. If the remaining row has one coefficient pivot, the ranks are both one and there is exactly one free variable. Solve the pivot equation in terms of that variable. Every real parameter value must work, and every solution must be represented. A missing pivot alone does not imply infinitely many solutions: consistency must be checked first.
@prose If there are two coefficient pivots, both unknowns are fixed. For a two by two coefficient matrix this agrees with a nonzero determinant: a11 times a22 minus a12 times a21. A zero determinant by itself does not distinguish inconsistency from infinitely many solutions.
@display rank_criteria
\begin{aligned}\operatorname{rank}(A)&<\operatorname{rank}([A|b])&&\Rightarrow S=\varnothing\\
\operatorname{rank}(A)&=\operatorname{rank}([A|b])=2&&\Rightarrow |S|=1\\
\operatorname{rank}(A)&=\operatorname{rank}([A|b])=1&&\Rightarrow \text{one free real parameter}\end{aligned}
@endblock
@block example | worked | 2.4 | A separate complete solve
@prose Solve the original equations x-y=2 and 2x+y=7. Work from the augmented matrix, classify the result and check both original equations.
@display worked_given
\left[\begin{array}{cc|c}1&-1&2\\2&1&7\end{array}\right]
@help hint
@prose Use the first row to eliminate x from the second row. Include the constant column. Inspect the new pivot before dividing, then clear y from the first row.
@help answer
@display
S=\{\left(3,1\right)\}
@help solution
@prose Subtract twice row 1 from row 2: the entries are 2-2(1)=0, 1-2(-1)=3 and 7-2(2)=3. Adding twice row 1 would reverse this move.
@display worked_elimination
\left[\begin{array}{cc|c}1&-1&2\\0&3&3\end{array}\right]
@prose Divide the complete second row by 3, which is nonzero: 0/3=0, 3/3=1 and 3/3=1. Then add that row to row 1: 1+0=1, -1+1=0 and 2+1=3.
@display worked_reduction
\left[\begin{array}{cc|c}1&-1&2\\0&1&1\end{array}\right]\longrightarrow\left[\begin{array}{cc|c}1&0&3\\0&1&1\end{array}\right]
@prose There are two pivots, so the pair is unique. Check the original equations, not merely the last matrix.
@display worked_check
3-1=2,\qquad 2(3)+1=7
@help proof
@prose Every operation used above has an inverse, so the original and reduced systems have exactly the same solution set. The reduced rows each fix one coordinate. The substitution check proves the displayed pair exists, and the two pivot equations leave no other pair.
@endblock
@block example | errors | 2.5 | What an incomplete operation loses
@prose Swapping coefficient entries but not constants attaches an equation to the wrong right-hand side. Dividing coefficients while leaving a constant untouched changes its equality. A zero row does not tell you to set a free variable to zero; that produces at most one member of a larger family. In a parameter check, cancel the parameter coefficient in both original equations, rather than testing only one convenient value.
@prose In the questions, row-result choices list all three entries of the destination row. The reached matrix also shows the unchanged row. Final-check choices give the ordered values of the two original left sides, in the original row order.
@endblock
@block exercise | practice | 2.6 | From guided calculations to mixed systems
@prose Begin with four full solves using signed pivots, a row swap and exact fractions. Continue with negative and nonunit pivots, then compare incompatible equations with dependent complete equations. Finish by choosing a useful first move and handling systems whose x column is entirely zero. The same reversibility and original-equation checks apply throughout.
@endblock
@block summary | summary | 2.7 | A complete answer includes a reason
@prose Preserve x,y,constant order. Make every row operation complete and reversible. Classify from coefficient pivots and the augmented row together. For a unique pair, substitute into both originals; for no solution, display the contradiction; for an infinite family, define the parameter, verify identities for every real value and explain why no solutions are missing.
@endblock
@practice prod02_linear_full_systems_q01
@practice prod02_linear_full_systems_q02
@practice prod02_linear_full_systems_q03
@practice prod02_linear_full_systems_q04
@practice prod02_linear_full_systems_q05
@practice prod02_linear_full_systems_q06
@practice prod02_linear_full_systems_q07
@practice prod02_linear_full_systems_q08
@practice prod02_linear_full_systems_q09
@practice prod02_linear_full_systems_q10
@practice prod02_linear_full_systems_q11
@practice prod02_linear_full_systems_q12
@end

@question prod02_linear_full_systems_q01 | Eliminate and check
@template choices.v1
@version 1
@goal Solve the system completely, classify its solution set and check the result in both original equations.
@given \left[\begin{array}{cc|c}1&1&5\\2&-1&1\end{array}\right]
@domain x and y are real. Columns are x coefficient, y coefficient, then right-hand constant. R1 and R2 denote complete rows. A is the coefficient matrix, [A|b] the augmented matrix, and S the solution set.
@read prod02_linear_full_systems_r
@step 10 | Replace row 2 by row 2 minus twice row 1. Which complete destination row results?
@choice 11 | \left[\begin{array}{cc|c}0&-3&-9\end{array}\right]
@choice 12 | \left[\begin{array}{cc|c}0&-3&1\end{array}\right]
@choice 13 | \left[\begin{array}{cc|c}4&1&11\end{array}\right]
@answer 11
@feedback 12 | This leaves the old constant 1 unchanged. The constant must be 1-2(5)=-9.
@feedback 13 | This adds twice row 1. That reversible move gives x coefficient 2+2(1)=4 and misses the requested elimination.
@after \left[\begin{array}{cc|c}1&1&5\\0&-3&-9\end{array}\right]
@wrong Apply the signed multiple to all three entries, using the old rows.
@why The entries are 2-2(1)=0, -1-2(1)=-3 and 1-2(5)=-9. Row 1 stays unchanged. Adding twice row 1 reverses the replacement.
@step 20 | Divide every entry of row 2 by its nonzero pivot -3. Which row results?
@choice 22 | \left[\begin{array}{cc|c}0&1&-9\end{array}\right]
@choice 21 | \left[\begin{array}{cc|c}0&1&3\end{array}\right]
@choice 23 | \left[\begin{array}{cc|c}0&-1&-3\end{array}\right]
@answer 21
@feedback 22 | The coefficients were divided but the constant was not: -9 divided by -3 is 3.
@feedback 23 | This divides by positive 3. It preserves the equation but leaves the pivot -1 instead of the requested 1.
@after \left[\begin{array}{cc|c}1&1&5\\0&1&3\end{array}\right]
@wrong Divide the constant by the same signed, nonzero number as the coefficients.
@why The three divisions are 0/(-3)=0, (-3)/(-3)=1 and (-9)/(-3)=3. Multiplying the whole row by -3 restores the previous row.
@step 30 | Clear y from row 1 by subtracting row 2. Which complete row results?
@choice 32 | \left[\begin{array}{cc|c}1&0&5\end{array}\right]
@choice 33 | \left[\begin{array}{cc|c}1&2&8\end{array}\right]
@choice 31 | \left[\begin{array}{cc|c}1&0&2\end{array}\right]
@answer 31
@feedback 32 | The old constant 5 was retained. Subtract 3 there too: 5-3=2.
@feedback 33 | Adding row 2 gives y coefficient 2. It is reversible but does not clear y.
@after \left[\begin{array}{cc|c}1&0&2\\0&1&3\end{array}\right]
@wrong Include the constant when subtracting the complete row.
@why Subtract entry by entry: 1-0=1, 1-1=0 and 5-3=2. Row 2 stays fixed; adding it back reverses this move.
@step 40 | Read the reduced rows. Which ordered pair is the unique solution?
@choice 41 | S=\{\left(2,3\right)\}
@choice 42 | S=\{\left(-2,3\right)\}
@choice 43 | S=\{\left(2,-3\right)\}
@answer 41
@feedback 42 | The first pivot row says x=2, not x=-2. Do not reverse the constant's sign when reading it.
@feedback 43 | The second pivot row says y=3, not y=-3.
@after S=\{\left(2,3\right)\},\quad \left(\operatorname{rank}(A),\operatorname{rank}([A|b])\right)=(2,2)
@wrong Read x from the first pivot row and y from the second.
@why The rows are x=2 and y=3. Both coefficient columns have pivots, so both coordinates are fixed; the coefficient and augmented ranks are both two.
@step 50 | Substitute that pair into both ORIGINAL equations. What ordered values do their left sides take?
@choice 52 | \left(-1,1\right)
@choice 51 | \left(5,1\right)
@choice 53 | \left(5,7\right)
@answer 51
@feedback 52 | The first equation adds y: 2+3=5. The value -1 comes from subtracting it instead.
@feedback 53 | The second equation is 2x-y, so its value is 4-3=1; 7 reverses the sign of the y term.
@after \begin{gathered}(1)(2)+(1)(3)=5\\(2)(2)+(-1)(3)=1\\S=\{\left(2,3\right)\}\end{gathered}
@wrong Use the signs and order of the original equations.
@why The original left sides are 2+3=5 and 4-3=1, exactly their constants. This proves existence; the reversible route to two pivot equations proves uniqueness.
@end

@question prod02_linear_full_systems_q02 | Keep the coefficient signs
@template choices.v1
@version 1
@goal Solve the system completely, classify its solution set and check the result in both original equations.
@given \left[\begin{array}{cc|c}1&-2&1\\3&1&10\end{array}\right]
@domain x and y are real. Columns are x coefficient, y coefficient, then right-hand constant. R1 and R2 denote complete rows. A is the coefficient matrix, [A|b] the augmented matrix, and S the solution set.
@read prod02_linear_full_systems_r
@step 10 | Replace row 2 by row 2 minus three times row 1. Which complete row results?
@choice 12 | \left[\begin{array}{cc|c}0&7&10\end{array}\right]
@choice 11 | \left[\begin{array}{cc|c}0&7&7\end{array}\right]
@choice 13 | \left[\begin{array}{cc|c}6&-5&13\end{array}\right]
@answer 11
@feedback 12 | This omits the constant operation. Compute 10-3(1)=7.
@feedback 13 | This adds three times row 1. Its x coefficient is 6, so it misses the stated cancellation goal.
@after \left[\begin{array}{cc|c}1&-2&1\\0&7&7\end{array}\right]
@wrong Subtract the signed entries, including the negative y coefficient.
@why The entries are 3-3(1)=0, 1-3(-2)=7 and 10-3(1)=7. Row 1 is retained; adding three times row 1 reverses the move.
@step 20 | Divide the complete second row by 7. Which row results?
@choice 22 | \left[\begin{array}{cc|c}0&1&7\end{array}\right]
@choice 23 | \left[\begin{array}{cc|c}0&-1&-1\end{array}\right]
@choice 21 | \left[\begin{array}{cc|c}0&1&1\end{array}\right]
@answer 21
@feedback 22 | The constant must also be divided: 7/7=1.
@feedback 23 | Division by -7 yields this equivalent negative row but not the requested unit pivot.
@after \left[\begin{array}{cc|c}1&-2&1\\0&1&1\end{array}\right]
@wrong Divide all three entries by positive 7.
@why Since 7 is nonzero, division is reversible. The entries become 0/7=0, 7/7=1 and 7/7=1.
@step 30 | Clear y from row 1 by adding twice row 2. Which row results?
@choice 31 | \left[\begin{array}{cc|c}1&0&3\end{array}\right]
@choice 32 | \left[\begin{array}{cc|c}1&0&1\end{array}\right]
@choice 33 | \left[\begin{array}{cc|c}1&-4&-1\end{array}\right]
@answer 31
@feedback 32 | The old constant 1 is not retained: 1+2(1)=3.
@feedback 33 | Subtracting twice row 2 changes -2 to -4, rather than cancelling it. That move is reversible but misses this goal.
@after \left[\begin{array}{cc|c}1&0&3\\0&1&1\end{array}\right]
@wrong Cancel the negative y coefficient by addition.
@why Compute 1+2(0)=1, -2+2(1)=0 and 1+2(1)=3. Row 2 stays unchanged; subtracting twice row 2 reverses the operation.
@step 40 | Which ordered pair is fixed by the two pivot rows?
@choice 42 | S=\{\left(-3,1\right)\}
@choice 41 | S=\{\left(3,1\right)\}
@choice 43 | S=\{\left(3,-1\right)\}
@answer 41
@feedback 42 | The first row states x=3, not -3.
@feedback 43 | The second row states y=1, not -1.
@after S=\{\left(3,1\right)\},\quad \left(\operatorname{rank}(A),\operatorname{rank}([A|b])\right)=(2,2)
@wrong Each pivot row fixes its own coordinate without a sign change.
@why There is a pivot in each coefficient column. The rows fix x=3 and y=1, so the equal ranks of two give one possible pair.
@step 50 | Evaluate both ORIGINAL left sides at that pair. Which ordered values result?
@choice 52 | \left(5,10\right)
@choice 53 | \left(1,8\right)
@choice 51 | \left(1,10\right)
@answer 51
@feedback 52 | The first left side is 3-2(1)=1. The value 5 changes the negative y term to addition.
@feedback 53 | The second left side adds y: 3(3)+1=10, not 9-1=8.
@after \begin{gathered}(1)(3)+(-2)(1)=1\\(3)(3)+(1)(1)=10\\S=\{\left(3,1\right)\}\end{gathered}
@wrong Recheck the original coefficient signs, not just the last matrix.
@why The values 3-2=1 and 9+1=10 match both original constants. The inverse row operations and two fixed pivot coordinates exclude any second solution.
@end

@question prod02_linear_full_systems_q03 | Reorder complete equations
@template choices.v1
@version 1
@goal Solve the system completely, classify its solution set and check the result in both original equations.
@given \left[\begin{array}{cc|c}0&3&6\\1&2&7\end{array}\right]
@domain x and y are real. Columns are x coefficient, y coefficient, then right-hand constant. R1 and R2 denote complete rows. A is the coefficient matrix, [A|b] the augmented matrix, and S the solution set.
@read prod02_linear_full_systems_r
@step 10 | Swap the two complete rows to put a nonzero x coefficient first. Which matrix results?
@choice 12 | \left[\begin{array}{cc|c}1&2&6\\0&3&7\end{array}\right]
@choice 13 | \left[\begin{array}{cc|c}0&3&6\\1&2&7\end{array}\right]
@choice 11 | \left[\begin{array}{cc|c}1&2&7\\0&3&6\end{array}\right]
@answer 11
@feedback 12 | This swaps the coefficients but not the constants. The row with coefficients 1,2 must keep its constant 7.
@feedback 13 | This is the unchanged original system. It is valid but still has zero in the first x position, so the requested swap has not occurred.
@after \left[\begin{array}{cc|c}1&2&7\\0&3&6\end{array}\right]
@wrong Move each equation with all three of its entries.
@why The complete row [1,2|7] goes first and [0,3|6] goes second. No equation changes, and swapping again restores the original order. The x coefficient below the new pivot is already zero.
@step 20 | Divide every entry of row 2 by 3. Which row results?
@choice 21 | \left[\begin{array}{cc|c}0&1&2\end{array}\right]
@choice 22 | \left[\begin{array}{cc|c}0&1&6\end{array}\right]
@choice 23 | \left[\begin{array}{cc|c}0&-1&-2\end{array}\right]
@answer 21
@feedback 22 | The constant 6 must be divided too: 6/3=2.
@feedback 23 | This uses divisor -3. Its negative pivot is equivalent but does not perform the stated division by positive 3.
@after \left[\begin{array}{cc|c}1&2&7\\0&1&2\end{array}\right]
@wrong Normalize with the same nonzero divisor in all columns.
@why The divisions are 0/3=0, 3/3=1 and 6/3=2. Multiplication by 3 reverses the step.
@step 30 | Subtract twice row 2 from row 1 to clear y. Which row results?
@choice 32 | \left[\begin{array}{cc|c}1&0&7\end{array}\right]
@choice 31 | \left[\begin{array}{cc|c}1&0&3\end{array}\right]
@choice 33 | \left[\begin{array}{cc|c}1&4&11\end{array}\right]
@answer 31
@feedback 32 | This keeps 7 after changing the coefficients. The constant is 7-2(2)=3.
@feedback 33 | Addition gives y coefficient 4 and constant 11; it does not meet the elimination goal.
@after \left[\begin{array}{cc|c}1&0&3\\0&1&2\end{array}\right]
@wrong Subtract twice every entry in row 2.
@why The calculations are 1-2(0)=1, 2-2(1)=0 and 7-2(2)=3. Adding twice the unchanged row 2 reverses the operation.
@step 40 | Which ordered pair is the unique solution, in x,y order?
@choice 42 | S=\{\left(-3,2\right)\}
@choice 43 | S=\{\left(3,-2\right)\}
@choice 41 | S=\{\left(3,2\right)\}
@answer 41
@feedback 42 | The first pivot equation has positive right-hand constant 3.
@feedback 43 | The second pivot equation has positive right-hand constant 2.
@after S=\{\left(3,2\right)\},\quad \left(\operatorname{rank}(A),\operatorname{rank}([A|b])\right)=(2,2)
@wrong A row swap changes equation order, not the meanings of x and y.
@why The x column is still first and the y column second. Two pivots fix x=3 and y=2, with both ranks equal to two.
@step 50 | Check the pair in the ORIGINAL row order. Which left-side values result?
@choice 51 | \left(6,7\right)
@choice 52 | \left(-6,7\right)
@choice 53 | \left(6,-1\right)
@answer 51
@feedback 52 | The original first equation is 3y=6. At y=2 its left side is positive 6, not -6.
@feedback 53 | The original second left side is x+2y=3+4=7; -1 subtracts the y contribution instead.
@after \begin{gathered}(0)(3)+(3)(2)=6\\(1)(3)+(2)(2)=7\\S=\{\left(3,2\right)\}\end{gathered}
@wrong Use the originals in their original order after a swap.
@why The original values are 0+6=6 and 3+4=7. They match both constants; reversibility and two pivots prove that this is the whole solution set.
@end

@question prod02_linear_full_systems_q04 | Retain exact fractions
@template choices.v1
@version 1
@goal Solve the system completely, classify its solution set and check the result in both original equations.
@given \left[\begin{array}{cc|c}1&2&5\\2&-1&2\end{array}\right]
@domain x and y are real. Columns are x coefficient, y coefficient, then right-hand constant. R1 and R2 denote complete rows. A is the coefficient matrix, [A|b] the augmented matrix, and S the solution set.
@read prod02_linear_full_systems_r
@step 10 | Subtract twice row 1 from row 2. Which complete destination row results?
@choice 12 | \left[\begin{array}{cc|c}0&-5&2\end{array}\right]
@choice 11 | \left[\begin{array}{cc|c}0&-5&-8\end{array}\right]
@choice 13 | \left[\begin{array}{cc|c}4&3&12\end{array}\right]
@answer 11
@feedback 12 | The constant was left at 2. Subtract twice 5 to obtain -8.
@feedback 13 | This adds twice row 1 and gives x coefficient 4. It is reversible but fails to eliminate x.
@after \left[\begin{array}{cc|c}1&2&5\\0&-5&-8\end{array}\right]
@wrong Update both coefficient columns and the constant with the same subtraction.
@why Compute 2-2(1)=0, -1-2(2)=-5 and 2-2(5)=-8. Row 1 stays fixed; adding twice row 1 reverses the change.
@step 20 | Normalize row 2 by dividing all entries by -5. Which row results?
@choice 22 | \left[\begin{array}{cc|c}0&1&-8\end{array}\right]
@choice 23 | \left[\begin{array}{cc|c}0&-1&-\frac{8}{5}\end{array}\right]
@choice 21 | \left[\begin{array}{cc|c}0&1&\frac{8}{5}\end{array}\right]
@answer 21
@feedback 22 | The constant also needs division: (-8)/(-5)=8/5.
@feedback 23 | This uses divisor positive 5 and leaves a negative pivot. It is equivalent but not the requested normalization.
@after \left[\begin{array}{cc|c}1&2&5\\0&1&\frac{8}{5}\end{array}\right]
@wrong Keep the exact positive quotient of the two negative numbers.
@why Since -5 is nonzero, divide the entire row: 0/(-5)=0, (-5)/(-5)=1 and (-8)/(-5)=8/5. Multiplication by -5 restores it.
@step 30 | Subtract twice the normalized row 2 from row 1. Which row results?
@choice 31 | \left[\begin{array}{cc|c}1&0&\frac{9}{5}\end{array}\right]
@choice 32 | \left[\begin{array}{cc|c}1&0&5\end{array}\right]
@choice 33 | \left[\begin{array}{cc|c}1&4&\frac{41}{5}\end{array}\right]
@answer 31
@feedback 32 | The constant must change from 5 to 5-2(8/5)=9/5.
@feedback 33 | Adding twice row 2 gives 5+16/5=41/5 and y coefficient 4. It misses the requested cancellation.
@after \left[\begin{array}{cc|c}1&0&\frac{9}{5}\\0&1&\frac{8}{5}\end{array}\right]
@wrong Express 5 as 25/5 before subtracting the fractional constant.
@why The entries become 1-2(0)=1, 2-2(1)=0 and 25/5-16/5=9/5. Adding twice row 2 reverses the step.
@step 40 | Which exact ordered pair follows from the two pivot rows?
@choice 42 | S=\{\left(-\frac{9}{5},\frac{8}{5}\right)\}
@choice 41 | S=\{\left(\frac{9}{5},\frac{8}{5}\right)\}
@choice 43 | S=\{\left(\frac{9}{5},-\frac{8}{5}\right)\}
@answer 41
@feedback 42 | The first pivot row fixes positive x=9/5.
@feedback 43 | The second pivot row fixes positive y=8/5.
@after S=\{\left(\frac{9}{5},\frac{8}{5}\right)\},\quad \left(\operatorname{rank}(A),\operatorname{rank}([A|b])\right)=(2,2)
@wrong Read the exact constants without rounding or changing signs.
@why Two pivots fix both coordinates. Fractions do not change uniqueness: the coefficient and augmented ranks are both two.
@step 50 | Substitute the exact fractions in the ORIGINAL equations. Which left-side values result?
@choice 52 | \left(-\frac{7}{5},2\right)
@choice 53 | \left(5,\frac{26}{5}\right)
@choice 51 | \left(5,2\right)
@answer 51
@feedback 52 | The first left side adds 2y: 9/5+16/5=5. The value -7/5 subtracts that contribution.
@feedback 53 | The second left side is 2x-y=18/5-8/5=2. Adding 8/5 instead gives the incorrect 26/5.
@after \begin{gathered}(1)(\frac{9}{5})+(2)(\frac{8}{5})=5\\(2)(\frac{9}{5})+(-1)(\frac{8}{5})=2\\S=\{\left(\frac{9}{5},\frac{8}{5}\right)\}\end{gathered}
@wrong Substitute the same exact pair into both original equations.
@why The left sides are 25/5=5 and 10/5=2. Both original equations hold, and the reversible reduction to two pivots proves uniqueness.
@end

@question prod02_linear_full_systems_q05 | Work with a negative pivot
@template choices.v1
@version 1
@goal Solve the system completely, classify its solution set and check the result in both original equations.
@given \left[\begin{array}{cc|c}-2&1&3\\3&2&5\end{array}\right]
@domain x and y are real. Columns are x coefficient, y coefficient, then right-hand constant. R1 and R2 denote complete rows. A is the coefficient matrix, [A|b] the augmented matrix, and S the solution set.
@read prod02_linear_full_systems_r
@step 10 | Eliminate x from row 2 by adding three halves of row 1. Which whole row results?
@choice 12 | \left[\begin{array}{cc|c}0&\frac{7}{2}&5\end{array}\right]
@choice 13 | \left[\begin{array}{cc|c}6&\frac{1}{2}&\frac{1}{2}\end{array}\right]
@choice 11 | \left[\begin{array}{cc|c}0&\frac{7}{2}&\frac{19}{2}\end{array}\right]
@answer 11
@feedback 12 | The constant needs the same addition: 5+(3/2)(3)=19/2, not 5.
@feedback 13 | This subtracts three halves of row 1 and leaves x coefficient 6. It is reversible but misses cancellation.
@after \left[\begin{array}{cc|c}-2&1&3\\0&\frac{7}{2}&\frac{19}{2}\end{array}\right]
@wrong A fractional multiple reaches every entry, including the constant.
@why The entries are 3+(3/2)(-2)=0, 2+(3/2)(1)=7/2 and 5+(3/2)(3)=19/2. Row 1 is unchanged; subtracting three halves of it reverses the move.
@step 20 | Divide all entries of row 2 by seven halves. Which row results?
@choice 21 | \left[\begin{array}{cc|c}0&1&\frac{19}{7}\end{array}\right]
@choice 22 | \left[\begin{array}{cc|c}0&1&\frac{19}{2}\end{array}\right]
@choice 23 | \left[\begin{array}{cc|c}0&-1&-\frac{19}{7}\end{array}\right]
@answer 21
@feedback 22 | The constant was not divided. Dividing 19/2 by 7/2 multiplies it by 2/7, giving 19/7.
@feedback 23 | This uses the negative divisor -7/2. It produces an equivalent negative equation, not the requested unit pivot.
@after \left[\begin{array}{cc|c}-2&1&3\\0&1&\frac{19}{7}\end{array}\right]
@wrong Divide by a fraction by multiplying by its reciprocal.
@why Seven halves is nonzero. The divisions give 0, 1 and (19/2)(2/7)=19/7. Multiplying the whole row by 7/2 reverses them.
@step 30 | Subtract row 2 from row 1 to clear y. Which complete row results?
@choice 32 | \left[\begin{array}{cc|c}-2&0&3\end{array}\right]
@choice 31 | \left[\begin{array}{cc|c}-2&0&\frac{2}{7}\end{array}\right]
@choice 33 | \left[\begin{array}{cc|c}-2&2&\frac{40}{7}\end{array}\right]
@answer 31
@feedback 32 | The constant must be 3-19/7=2/7, not the old 3.
@feedback 33 | Addition gives y coefficient 2 and constant 40/7; it does not clear y.
@after \left[\begin{array}{cc|c}-2&0&\frac{2}{7}\\0&1&\frac{19}{7}\end{array}\right]
@wrong Subtract the fractional constant using a common denominator.
@why Compute -2-0=-2, 1-1=0 and 21/7-19/7=2/7. The second row is unchanged, and adding it back restores row 1.
@step 40 | Normalize row 1 by dividing its complete equation by -2. Which row results?
@choice 42 | \left[\begin{array}{cc|c}1&0&\frac{2}{7}\end{array}\right]
@choice 43 | \left[\begin{array}{cc|c}-1&0&\frac{1}{7}\end{array}\right]
@choice 41 | \left[\begin{array}{cc|c}1&0&-\frac{1}{7}\end{array}\right]
@answer 41
@feedback 42 | The constant also needs division: (2/7)/(-2)=-1/7.
@feedback 43 | Division by positive 2 gives a negative pivot. It is equivalent, but the requested divisor is -2.
@after \left[\begin{array}{cc|c}1&0&-\frac{1}{7}\\0&1&\frac{19}{7}\end{array}\right]
@wrong A positive constant divided by a negative coefficient becomes negative.
@why Since -2 is nonzero, divide all entries: (-2)/(-2)=1, 0/(-2)=0 and (2/7)/(-2)=-1/7. Multiplying by -2 restores the prior row.
@step 50 | Which exact ordered pair is fixed by the normalized rows?
@choice 51 | S=\{\left(-\frac{1}{7},\frac{19}{7}\right)\}
@choice 52 | S=\{\left(\frac{1}{7},\frac{19}{7}\right)\}
@choice 53 | S=\{\left(-\frac{1}{7},-\frac{19}{7}\right)\}
@answer 51
@feedback 52 | The first pivot equation fixes x=-1/7. Positive 1/7 loses the sign from division by -2.
@feedback 53 | The second pivot equation fixes positive y=19/7.
@after S=\{\left(-\frac{1}{7},\frac{19}{7}\right)\},\quad \left(\operatorname{rank}(A),\operatorname{rank}([A|b])\right)=(2,2)
@wrong Keep each coordinate's own sign when reading the pivot rows.
@why Two pivots fix x=-1/7 and y=19/7. Both ranks are two, so these rows permit exactly one pair.
@step 60 | Evaluate both ORIGINAL left sides, retaining exact fractions. Which values result?
@choice 62 | \left(-\frac{17}{7},5\right)
@choice 61 | \left(3,5\right)
@choice 63 | \left(3,-\frac{41}{7}\right)
@answer 61
@feedback 62 | The first left side adds y: 2/7+19/7=3. Subtracting it produces -17/7.
@feedback 63 | The second left side adds 2y: -3/7+38/7=5. Subtracting that contribution produces -41/7.
@after \begin{gathered}(-2)(-\frac{1}{7})+(1)(\frac{19}{7})=3\\(3)(-\frac{1}{7})+(2)(\frac{19}{7})=5\\S=\{\left(-\frac{1}{7},\frac{19}{7}\right)\}\end{gathered}
@wrong Retain both the coefficient sign and the coordinate sign in each product.
@why The original values are 21/7=3 and 35/7=5. Both equations hold. Every reduction was reversible and both coordinates were fixed, establishing existence and uniqueness.
@end

@question prod02_linear_full_systems_q06 | Finish both pivot normalizations
@template choices.v1
@version 1
@goal Solve the system completely, classify its solution set and check the result in both original equations.
@given \left[\begin{array}{cc|c}2&3&7\\4&-1&3\end{array}\right]
@domain x and y are real. Columns are x coefficient, y coefficient, then right-hand constant. R1 and R2 denote complete rows. A is the coefficient matrix, [A|b] the augmented matrix, and S the solution set.
@read prod02_linear_full_systems_r
@step 10 | Subtract twice row 1 from row 2. Which whole row results?
@choice 11 | \left[\begin{array}{cc|c}0&-7&-11\end{array}\right]
@choice 12 | \left[\begin{array}{cc|c}0&-7&3\end{array}\right]
@choice 13 | \left[\begin{array}{cc|c}8&5&17\end{array}\right]
@answer 11
@feedback 12 | The constant must be 3-2(7)=-11; leaving it at 3 changes the system.
@feedback 13 | This adds twice row 1. It preserves solutions but leaves x coefficient 8 instead of zero.
@after \left[\begin{array}{cc|c}2&3&7\\0&-7&-11\end{array}\right]
@wrong Operate on the complete row, including its right side.
@why The entries are 4-2(2)=0, -1-2(3)=-7 and 3-2(7)=-11. Row 1 is retained; addition of twice row 1 reverses the operation.
@step 20 | Normalize row 2 by dividing all three entries by -7. Which row results?
@choice 22 | \left[\begin{array}{cc|c}0&1&-11\end{array}\right]
@choice 21 | \left[\begin{array}{cc|c}0&1&\frac{11}{7}\end{array}\right]
@choice 23 | \left[\begin{array}{cc|c}0&-1&-\frac{11}{7}\end{array}\right]
@answer 21
@feedback 22 | Divide the constant as well: (-11)/(-7)=11/7.
@feedback 23 | This divides by positive 7, retaining a negative pivot instead of making it 1.
@after \left[\begin{array}{cc|c}2&3&7\\0&1&\frac{11}{7}\end{array}\right]
@wrong The same negative divisor must reach all three columns.
@why The divisions are 0/(-7)=0, (-7)/(-7)=1 and (-11)/(-7)=11/7. The nonzero divisor makes the move reversible.
@step 30 | Subtract three times row 2 from row 1. Which row results?
@choice 32 | \left[\begin{array}{cc|c}2&0&7\end{array}\right]
@choice 33 | \left[\begin{array}{cc|c}2&6&\frac{82}{7}\end{array}\right]
@choice 31 | \left[\begin{array}{cc|c}2&0&\frac{16}{7}\end{array}\right]
@answer 31
@feedback 32 | The old constant 7 must change: 7-3(11/7)=16/7.
@feedback 33 | Addition gives y coefficient 6 and constant 82/7, so it misses elimination.
@after \left[\begin{array}{cc|c}2&0&\frac{16}{7}\\0&1&\frac{11}{7}\end{array}\right]
@wrong Subtract all three entries of the multiple, not only the y coefficient.
@why Compute 2-3(0)=2, 3-3(1)=0 and 49/7-33/7=16/7. Row 2 stays fixed; adding three copies restores row 1.
@step 40 | Divide every entry of row 1 by 2. Which row results?
@choice 41 | \left[\begin{array}{cc|c}1&0&\frac{8}{7}\end{array}\right]
@choice 42 | \left[\begin{array}{cc|c}1&0&\frac{16}{7}\end{array}\right]
@choice 43 | \left[\begin{array}{cc|c}-1&0&-\frac{8}{7}\end{array}\right]
@answer 41
@feedback 42 | This omits division of the constant. The equation 2x=16/7 gives x=8/7.
@feedback 43 | This uses divisor -2. Its equation is equivalent but does not perform the requested positive division.
@after \left[\begin{array}{cc|c}1&0&\frac{8}{7}\\0&1&\frac{11}{7}\end{array}\right]
@wrong A pivot need not already be 1; normalize its constant too.
@why Division by nonzero 2 gives 2/2=1, 0/2=0 and (16/7)/2=8/7. Multiplying the row by 2 is the inverse.
@step 50 | Which ordered pair solves the two pivot equations?
@choice 52 | S=\{\left(-\frac{8}{7},\frac{11}{7}\right)\}
@choice 51 | S=\{\left(\frac{8}{7},\frac{11}{7}\right)\}
@choice 53 | S=\{\left(\frac{8}{7},-\frac{11}{7}\right)\}
@answer 51
@feedback 52 | The first pivot row gives positive x=8/7.
@feedback 53 | The second pivot row gives positive y=11/7.
@after S=\{\left(\frac{8}{7},\frac{11}{7}\right)\},\quad \left(\operatorname{rank}(A),\operatorname{rank}([A|b])\right)=(2,2)
@wrong Read both normalized rows in the original x,y column order.
@why Two coefficient pivots fix both coordinates, with coefficient and augmented ranks two. The fractions are exact solution coordinates.
@step 60 | Check the pair against both ORIGINAL equations. Which ordered left-side values result?
@choice 62 | \left(-\frac{17}{7},3\right)
@choice 63 | \left(7,\frac{43}{7}\right)
@choice 61 | \left(7,3\right)
@answer 61
@feedback 62 | The first equation adds 3y: 16/7+33/7=7. Subtracting 33/7 yields -17/7 instead.
@feedback 63 | The second equation subtracts y: 32/7-11/7=3. Adding it gives the incorrect 43/7.
@after \begin{gathered}(2)(\frac{8}{7})+(3)(\frac{11}{7})=7\\(4)(\frac{8}{7})+(-1)(\frac{11}{7})=3\\S=\{\left(\frac{8}{7},\frac{11}{7}\right)\}\end{gathered}
@wrong Use the original signed coefficients when evaluating the pair.
@why The two left sides are 49/7=7 and 21/7=3, matching the original constants. Reversible reduction to two fixed coordinates excludes other solutions.
@end

@question prod02_linear_full_systems_q07 | Classify after elimination
@template choices.v1
@version 1
@goal Solve the system completely, classify its solution set and check the result in both original equations.
@given \left[\begin{array}{cc|c}1&-2&3\\2&-4&8\end{array}\right]
@domain x and y are real. Columns are x coefficient, y coefficient, then right-hand constant. R1 and R2 denote complete rows. A is the coefficient matrix, [A|b] the augmented matrix, and S the solution set.
@read prod02_linear_full_systems_r
@step 10 | Subtract twice row 1 from row 2, including the constant. Which row results?
@choice 12 | \left[\begin{array}{cc|c}0&0&8\end{array}\right]
@choice 13 | \left[\begin{array}{cc|c}4&-8&14\end{array}\right]
@choice 11 | \left[\begin{array}{cc|c}0&0&2\end{array}\right]
@answer 11
@feedback 12 | This leaves the old constant 8. It must become 8-2(3)=2.
@feedback 13 | This adds rather than subtracts twice row 1. It is reversible but does not eliminate the second row's coefficients.
@after \left[\begin{array}{cc|c}1&-2&3\\0&0&2\end{array}\right]
@wrong A zero coefficient row still has a constant column to calculate.
@why Compute 2-2(1)=0, -4-2(-2)=0 and 8-2(3)=2. Adding twice the unchanged first row reverses this replacement.
@step 20 | Classify the system from coefficient rank and augmented rank. Which statement follows?
@choice 21 | \left(\operatorname{rank}(A),\operatorname{rank}([A|b])\right)=(1,2),\ S=\varnothing
@choice 22 | \left(\operatorname{rank}(A),\operatorname{rank}([A|b])\right)=(1,1),\ |S|=\infty
@choice 23 | \left(\operatorname{rank}(A),\operatorname{rank}([A|b])\right)=(2,2),\ |S|=1
@answer 21
@feedback 22 | This treats [0,0|2] as a zero row. Its nonzero constant is a pivot in the augmented matrix and asserts the contradiction 0=2.
@feedback 23 | The second row has no coefficient pivot. A pivot in the constant column does not fix an unknown; it makes the system inconsistent.
@after 0=2,\quad \left(\operatorname{rank}(A),\operatorname{rank}([A|b])\right)=(1,2),\ S=\varnothing
@wrong Distinguish coefficient pivots from a pivot in the augmented column.
@why Only the first row contributes a coefficient pivot, but the second row contributes an augmented pivot. Thus the ranks are one and two. No real pair can satisfy 0=2, so the entire solution set is empty.
@step 30 | Compare twice ORIGINAL equation 1 with ORIGINAL equation 2; both now have left side 2x-4y. What ordered constants do they require?
@choice 32 | \left(3,8\right)
@choice 31 | \left(6,8\right)
@choice 33 | \left(6,6\right)
@answer 31
@feedback 32 | Doubling the first equation also doubles its constant: 2(3)=6, not 3.
@feedback 33 | The original second equation requires 8, not 6. Replacing its constant would conceal the inconsistency.
@after \begin{gathered}2x-4y=6\\2x-4y=8\\0=2\\S=\varnothing\end{gathered}
@wrong Scale both complete sides of the first original equation and retain the second original constant.
@why Twice the first equation requires 2x-4y=6; the second requires the same expression to equal 8. Subtracting gives 0=8-6=2. This checks impossibility directly against the originals and excludes every real pair.
@end

@question prod02_linear_full_systems_q08 | Describe the whole solution set
@template choices.v1
@version 1
@goal Solve the system completely, classify its solution set and check the result in both original equations.
@given \left[\begin{array}{cc|c}2&4&6\\-1&-2&-3\end{array}\right]
@domain x and y are real. Columns are x coefficient, y coefficient, then right-hand constant. R1 and R2 denote complete rows. A is the coefficient matrix, [A|b] the augmented matrix, and S the solution set.
@read prod02_linear_full_systems_r
@step 10 | Add one half of row 1 to row 2. Which complete row results?
@choice 11 | \left[\begin{array}{cc|c}0&0&0\end{array}\right]
@choice 12 | \left[\begin{array}{cc|c}0&0&-3\end{array}\right]
@choice 13 | \left[\begin{array}{cc|c}-2&-4&-6\end{array}\right]
@answer 11
@feedback 12 | The constant also cancels: -3+(1/2)(6)=0. Leaving -3 would invent an inconsistency.
@feedback 13 | This subtracts half of row 1. It preserves solutions but repeats a nonzero equation instead of eliminating row 2.
@after \left[\begin{array}{cc|c}2&4&6\\0&0&0\end{array}\right]
@wrong Include the constant in the same half-row addition.
@why The entries are -1+(1/2)(2)=0, -2+(1/2)(4)=0 and -3+(1/2)(6)=0. The second equation becomes an identity; subtracting half the retained row reverses the move.
@step 20 | Divide the retained first row by 2 to normalize its pivot. Which row results?
@choice 22 | \left[\begin{array}{cc|c}1&2&6\end{array}\right]
@choice 21 | \left[\begin{array}{cc|c}1&2&3\end{array}\right]
@choice 23 | \left[\begin{array}{cc|c}-1&-2&-3\end{array}\right]
@answer 21
@feedback 22 | The constant must also be divided: 6/2=3.
@feedback 23 | Division by -2 gives an equivalent negative equation, but the stated divisor is positive 2.
@after \left[\begin{array}{cc|c}1&2&3\\0&0&0\end{array}\right]
@wrong Divide the nonzero row, not the zero row, by its nonzero pivot.
@why The entries become 2/2=1, 4/2=2 and 6/2=3. Multiplication by 2 restores the row. One pivot remains and the zero row imposes no further condition.
@step 30 | Let t be any real number and set the non-pivot variable y=t. Which expression describes ALL solutions?
@choice 32 | S=\{\left(3+2t,t\right):t\in\mathbb{R}\}
@choice 33 | S=\{\left(3,0\right)\}
@choice 31 | S=\{\left(3-2t,t\right):t\in\mathbb{R}\}
@answer 31
@feedback 32 | From x+2t=3, subtract 2t to obtain x=3-2t. Addition gives x+2y=3+4t, which is not always 3.
@feedback 33 | The pair (3,0) is a valid solution, but only the t=0 member. The zero row does not require y=0, so this singleton omits other solutions.
@after S=\{\left(3-2t,t\right):t\in\mathbb{R}\},\quad \left(\operatorname{rank}(A),\operatorname{rank}([A|b])\right)=(1,1)
@wrong A free variable may take every real value; solve the pivot equation in terms of it.
@why Both ranks are one and there is no contradictory row. Set y=t for arbitrary real t; then x+2t=3 forces x=3-2t. Each solution has exactly this form because its y coordinate determines t.
@step 40 | Substitute the family into both ORIGINAL left sides. Which identities hold for every real t?
@choice 41 | \left(6,-3\right)
@choice 42 | \left(6+8t,-3-4t\right)
@choice 43 | \left(6-4t,-3\right)
@answer 41
@feedback 42 | These are the left sides for the wrong family x=3+2t. The correct minus sign makes the parameter terms cancel in both equations.
@feedback 43 | The first expression omits the original +4y term. Including 4t cancels the -4t from 2x.
@after \begin{gathered}(2)(3-2t)+(4)(t)=6\\(-1)(3-2t)+(-2)(t)=-3\\S=\{\left(3-2t,t\right):t\in\mathbb{R}\}\end{gathered}
@wrong Check cancellation of the parameter coefficient, not merely a single test value.
@why The first left side is 6-4t+4t=6 and the second is -3+2t-2t=-3, identities for every real t. Conversely, the first original equation forces x=3-2y and the second adds no condition. The family includes all and only solutions.
@end

@question prod02_linear_full_systems_q09 | Choose a useful first move
@template choices.v1
@version 1
@goal Solve the system completely, classify its solution set and check the result in both original equations.
@given \left[\begin{array}{cc|c}0&2&3\\3&-1&5\end{array}\right]
@domain x and y are real. Columns are x coefficient, y coefficient, then right-hand constant. R1 and R2 denote complete rows. A is the coefficient matrix, [A|b] the augmented matrix, and S the solution set.
@read prod02_linear_full_systems_r
@step 10 | A nonzero x pivot is needed first. Which move puts an existing equation first without rescaling or combining equations?
@choice 12 | R_1\leftarrow 0R_1
@choice 11 | R_1\leftrightarrow R_2
@choice 13 | R_1\leftarrow R_1+R_2
@answer 11
@feedback 12 | Multiplication by zero erases the first equation and has no inverse. It also leaves the first x entry zero.
@feedback 13 | This valid row replacement would give [3,1|8]. It creates a nonzero x entry, but combines equations instead of putting an existing equation first as requested.
@after \left[\begin{array}{cc|c}3&-1&5\\0&2&3\end{array}\right]
@wrong Find a nonzero x coefficient already present and preserve each complete equation.
@why Swapping moves the existing complete row [3,-1|5] to the first position and [0,2|3] to the second. A second swap is its inverse. The lower x coefficient is already zero, so no x-elimination step is needed.
@step 20 | Normalize the second row by dividing all entries by 2. Which row results?
@choice 22 | \left[\begin{array}{cc|c}0&1&3\end{array}\right]
@choice 23 | \left[\begin{array}{cc|c}0&-1&-\frac{3}{2}\end{array}\right]
@choice 21 | \left[\begin{array}{cc|c}0&1&\frac{3}{2}\end{array}\right]
@answer 21
@feedback 22 | The constant must be divided too: 3/2, not 3.
@feedback 23 | This uses divisor -2. Its equation is equivalent, but it does not produce the requested positive unit pivot.
@after \left[\begin{array}{cc|c}3&-1&5\\0&1&\frac{3}{2}\end{array}\right]
@wrong Retain the fraction when dividing an odd constant by 2.
@why Nonzero division gives 0/2=0, 2/2=1 and 3/2. Multiplying the whole row by 2 reverses the operation.
@step 30 | Add row 2 to row 1 to clear its y coefficient. Which row results?
@choice 31 | \left[\begin{array}{cc|c}3&0&\frac{13}{2}\end{array}\right]
@choice 32 | \left[\begin{array}{cc|c}3&0&5\end{array}\right]
@choice 33 | \left[\begin{array}{cc|c}3&-2&\frac{7}{2}\end{array}\right]
@answer 31
@feedback 32 | This keeps the old constant. Addition must give 5+3/2=13/2.
@feedback 33 | Subtraction gives y coefficient -2 and constant 7/2. It is reversible but fails the requested cancellation.
@after \left[\begin{array}{cc|c}3&0&\frac{13}{2}\\0&1&\frac{3}{2}\end{array}\right]
@wrong The y coefficient is negative, so add the unit-y row.
@why Add entry by entry: 3+0=3, -1+1=0 and 10/2+3/2=13/2. Row 2 is unchanged; subtracting it reverses this move.
@step 40 | Divide the complete first row by 3. Which row results?
@choice 42 | \left[\begin{array}{cc|c}1&0&\frac{13}{2}\end{array}\right]
@choice 41 | \left[\begin{array}{cc|c}1&0&\frac{13}{6}\end{array}\right]
@choice 43 | \left[\begin{array}{cc|c}-1&0&-\frac{13}{6}\end{array}\right]
@answer 41
@feedback 42 | The constant also needs division by 3: (13/2)/3=13/6.
@feedback 43 | Division by -3 gives this equivalent negative row, but the requested divisor is positive 3.
@after \left[\begin{array}{cc|c}1&0&\frac{13}{6}\\0&1&\frac{3}{2}\end{array}\right]
@wrong Divide the fractional constant without dropping its denominator.
@why The entries become 3/3=1, 0/3=0 and (13/2)/3=13/6. The divisor is nonzero, and multiplication by 3 restores the previous row.
@step 50 | Which exact ordered pair follows from the pivot rows?
@choice 52 | S=\{\left(-\frac{13}{6},\frac{3}{2}\right)\}
@choice 53 | S=\{\left(\frac{13}{6},-\frac{3}{2}\right)\}
@choice 51 | S=\{\left(\frac{13}{6},\frac{3}{2}\right)\}
@answer 51
@feedback 52 | The first normalized row fixes positive x=13/6.
@feedback 53 | The second normalized row fixes positive y=3/2.
@after S=\{\left(\frac{13}{6},\frac{3}{2}\right)\},\quad \left(\operatorname{rank}(A),\operatorname{rank}([A|b])\right)=(2,2)
@wrong Read the unchanged x,y column meanings after the row swap.
@why The two coefficient pivots fix x=13/6 and y=3/2. The coefficient and augmented ranks are two, leaving no free variable.
@step 60 | Check both ORIGINAL equations in their original order. Which left-side values result?
@choice 61 | \left(3,5\right)
@choice 62 | \left(-3,5\right)
@choice 63 | \left(3,8\right)
@answer 61
@feedback 62 | The original first equation has +2y, so its value is 2(3/2)=3, not -3.
@feedback 63 | The original second left side is 3x-y=13/2-3/2=5. Adding y instead would produce 8.
@after \begin{gathered}(0)(\frac{13}{6})+(2)(\frac{3}{2})=3\\(3)(\frac{13}{6})+(-1)(\frac{3}{2})=5\\S=\{\left(\frac{13}{6},\frac{3}{2}\right)\}\end{gathered}
@wrong Return to the original equation signs and order for verification.
@why The original values are 3 and 10/2=5, matching both constants. The swap and subsequent operations are reversible, so the verified pair is the unique original solution.
@end

@question prod02_linear_full_systems_q10 | Choose an economical pivot
@template choices.v1
@version 1
@goal Solve the system completely, classify its solution set and check the result in both original equations.
@given \left[\begin{array}{cc|c}2&-3&1\\1&4&9\end{array}\right]
@domain x and y are real. Columns are x coefficient, y coefficient, then right-hand constant. R1 and R2 denote complete rows. A is the coefficient matrix, [A|b] the augmented matrix, and S the solution set.
@read prod02_linear_full_systems_r
@step 10 | Which first move gives row 1 a unit x pivot without introducing fractional entries?
@choice 11 | R_1\leftrightarrow R_2
@choice 12 | R_1\leftarrow \frac{1}{2}R_1
@choice 13 | R_2\leftarrow R_2-R_1
@answer 11
@feedback 12 | Dividing row 1 by 2 is valid and creates a unit x pivot, but also creates -3/2 and 1/2. It misses the requested no-new-fractions goal.
@feedback 13 | This is a valid reversible row replacement, but it changes only row 2. Row 1 still starts with 2, not the requested unit pivot.
@after \left[\begin{array}{cc|c}1&4&9\\2&-3&1\end{array}\right]
@wrong Compare the existing leading entries before choosing a normalization.
@why The existing second row already begins with 1 and contains only integers. Swapping complete rows brings it first without changing any entry; swapping again is the inverse.
@step 20 | Subtract twice row 1 from row 2. Which whole row results?
@choice 22 | \left[\begin{array}{cc|c}0&-11&1\end{array}\right]
@choice 21 | \left[\begin{array}{cc|c}0&-11&-17\end{array}\right]
@choice 23 | \left[\begin{array}{cc|c}4&5&19\end{array}\right]
@answer 21
@feedback 22 | The old constant 1 must change to 1-2(9)=-17.
@feedback 23 | Addition gives x coefficient 4 rather than zero, so it misses elimination.
@after \left[\begin{array}{cc|c}1&4&9\\0&-11&-17\end{array}\right]
@wrong Use the current rows after the swap and operate in all columns.
@why Compute 2-2(1)=0, -3-2(4)=-11 and 1-2(9)=-17. Row 1 stays fixed; adding twice row 1 reverses the step.
@step 30 | Divide every entry of row 2 by -11. Which row results?
@choice 32 | \left[\begin{array}{cc|c}0&1&-17\end{array}\right]
@choice 33 | \left[\begin{array}{cc|c}0&-1&-\frac{17}{11}\end{array}\right]
@choice 31 | \left[\begin{array}{cc|c}0&1&\frac{17}{11}\end{array}\right]
@answer 31
@feedback 32 | The constant must be divided along with the coefficients: (-17)/(-11)=17/11.
@feedback 33 | This uses positive 11 and leaves the pivot negative. The requested divisor is -11.
@after \left[\begin{array}{cc|c}1&4&9\\0&1&\frac{17}{11}\end{array}\right]
@wrong Two negative numbers give a positive quotient.
@why The nonzero divisor gives 0/(-11)=0, (-11)/(-11)=1 and (-17)/(-11)=17/11. Multiplying all entries by -11 reverses the operation.
@step 40 | Subtract four times row 2 from row 1. Which row results?
@choice 41 | \left[\begin{array}{cc|c}1&0&\frac{31}{11}\end{array}\right]
@choice 42 | \left[\begin{array}{cc|c}1&0&9\end{array}\right]
@choice 43 | \left[\begin{array}{cc|c}1&8&\frac{167}{11}\end{array}\right]
@answer 41
@feedback 42 | The constant is 9-4(17/11)=31/11, not the old 9.
@feedback 43 | Addition gives y coefficient 8 and constant 167/11. It is reversible but does not clear y.
@after \left[\begin{array}{cc|c}1&0&\frac{31}{11}\\0&1&\frac{17}{11}\end{array}\right]
@wrong Subtract the whole multiple, including four times the fractional constant.
@why The entries are 1-4(0)=1, 4-4(1)=0 and 99/11-68/11=31/11. Row 2 stays unchanged; adding four times it restores row 1.
@step 50 | Which exact ordered pair is fixed by the two pivots?
@choice 52 | S=\{\left(-\frac{31}{11},\frac{17}{11}\right)\}
@choice 51 | S=\{\left(\frac{31}{11},\frac{17}{11}\right)\}
@choice 53 | S=\{\left(\frac{31}{11},-\frac{17}{11}\right)\}
@answer 51
@feedback 52 | The first pivot row gives positive x=31/11.
@feedback 53 | The second pivot row gives positive y=17/11.
@after S=\{\left(\frac{31}{11},\frac{17}{11}\right)\},\quad \left(\operatorname{rank}(A),\operatorname{rank}([A|b])\right)=(2,2)
@wrong Read each positive constant with its own coordinate.
@why Both coefficient columns have pivots, fixing x=31/11 and y=17/11. Equal ranks of two establish that no free coordinate remains.
@step 60 | Substitute into the ORIGINAL equations rather than the swapped order. Which left-side values result?
@choice 62 | \left(\frac{113}{11},9\right)
@choice 63 | \left(1,-\frac{37}{11}\right)
@choice 61 | \left(1,9\right)
@answer 61
@feedback 62 | The original first equation subtracts 3y: 62/11-51/11=1. Addition would give 113/11.
@feedback 63 | The original second equation adds 4y: 31/11+68/11=9. Subtraction would give -37/11.
@after \begin{gathered}(2)(\frac{31}{11})+(-3)(\frac{17}{11})=1\\(1)(\frac{31}{11})+(4)(\frac{17}{11})=9\\S=\{\left(\frac{31}{11},\frac{17}{11}\right)\}\end{gathered}
@wrong Check the original order and signs even though the solution used a swap.
@why The original left sides are 11/11=1 and 99/11=9. This verifies existence; the reversible route to two pivot equations verifies uniqueness.
@end

@question prod02_linear_full_systems_q11 | Inspect the available coefficient column
@template choices.v1
@version 1
@goal Solve the system completely, classify its solution set and check the result in both original equations.
@given \left[\begin{array}{cc|c}0&2&5\\0&-6&-12\end{array}\right]
@domain x and y are real. Columns are x coefficient, y coefficient, then right-hand constant. R1 and R2 denote complete rows. A is the coefficient matrix, [A|b] the augmented matrix, and S the solution set.
@read prod02_linear_full_systems_r
@step 10 | Normalize the first nonzero coefficient in row 1 by dividing the whole row by 2. Which row results?
@choice 12 | \left[\begin{array}{cc|c}0&1&5\end{array}\right]
@choice 11 | \left[\begin{array}{cc|c}0&1&\frac{5}{2}\end{array}\right]
@choice 13 | \left[\begin{array}{cc|c}0&-1&-\frac{5}{2}\end{array}\right]
@answer 11
@feedback 12 | The constant needs division: 5/2, not 5.
@feedback 13 | Division by -2 gives a negative pivot; the stated divisor is positive 2.
@after \left[\begin{array}{cc|c}0&1&\frac{5}{2}\\0&-6&-12\end{array}\right]
@wrong The zero x column is not a divisor; use the nonzero y entry.
@why Divide by nonzero 2: 0/2=0, 2/2=1 and 5/2. The pivot is in the y column. Multiplying back by 2 restores the complete first row; consistency still needs checking.
@step 20 | Add six times row 1 to row 2 to clear y. Which whole row results?
@choice 22 | \left[\begin{array}{cc|c}0&0&-12\end{array}\right]
@choice 23 | \left[\begin{array}{cc|c}0&-12&-27\end{array}\right]
@choice 21 | \left[\begin{array}{cc|c}0&0&3\end{array}\right]
@answer 21
@feedback 22 | This omits the constant addition: -12+6(5/2)=-12+15=3.
@feedback 23 | Subtraction produces y coefficient -12 and constant -27. It is reversible but does not meet the stated elimination goal.
@after \left[\begin{array}{cc|c}0&1&\frac{5}{2}\\0&0&3\end{array}\right]
@wrong A fractional pivot-row constant must be included in the addition.
@why The entries are 0+6(0)=0, -6+6(1)=0 and -12+6(5/2)=3. Row 1 is unchanged; subtracting six times it reverses the move.
@step 30 | Does the augmented row permit any solution? Select the coefficient-rank and augmented-rank conclusion.
@choice 31 | \left(\operatorname{rank}(A),\operatorname{rank}([A|b])\right)=(1,2),\ S=\varnothing
@choice 32 | \left(\operatorname{rank}(A),\operatorname{rank}([A|b])\right)=(1,1),\ |S|=\infty
@choice 33 | \left(\operatorname{rank}(A),\operatorname{rank}([A|b])\right)=(2,2),\ |S|=1
@answer 31
@feedback 32 | A missing x pivot permits a free variable only in a consistent system. Here [0,0|3] is the contradiction 0=3, not an identity.
@feedback 33 | There is only one coefficient pivot, in y. The other pivot belongs to the constant column and signals inconsistency, not a second fixed unknown.
@after 0=3,\quad \left(\operatorname{rank}(A),\operatorname{rank}([A|b])\right)=(1,2),\ S=\varnothing
@wrong Check the augmented constant before concluding that an unpivoted variable is free.
@why The coefficient rank is one and the augmented rank is two. The second row asserts 0=3, impossible for every x and y. The entire solution set is empty despite the zero x column.
@step 40 | Compare -3 times ORIGINAL equation 1 with ORIGINAL equation 2; both now have left side -6y. What ordered constants do they require?
@choice 42 | \left(5,-12\right)
@choice 41 | \left(-15,-12\right)
@choice 43 | \left(-15,-15\right)
@answer 41
@feedback 42 | Multiplication by -3 must include the first constant: -3(5)=-15, not 5.
@feedback 43 | The second original constant is -12. Replacing it by -15 hides the actual contradiction.
@after \begin{gathered}-6y=-15\\-6y=-12\\0=3\\S=\varnothing\end{gathered}
@wrong Scale the complete first equation and retain the actual second equation.
@why Multiplying the first equation by -3 requires -6y=-15, while the second requires -6y=-12. Subtraction gives 0=-12-(-15)=3. Thus no value of y, and therefore no pair (x,y), satisfies both originals.
@end

@question prod02_linear_full_systems_q12 | Follow the pivot column
@template choices.v1
@version 1
@goal Solve the system completely, classify its solution set and check the result in both original equations.
@given \left[\begin{array}{cc|c}0&-3&6\\0&6&-12\end{array}\right]
@domain x and y are real. Columns are x coefficient, y coefficient, then right-hand constant. R1 and R2 denote complete rows. A is the coefficient matrix, [A|b] the augmented matrix, and S the solution set.
@read prod02_linear_full_systems_r
@step 10 | The x column is zero. Normalize the first nonzero coefficient of row 1 by dividing by -3. Which row results?
@choice 12 | \left[\begin{array}{cc|c}0&1&6\end{array}\right]
@choice 13 | \left[\begin{array}{cc|c}0&-1&2\end{array}\right]
@choice 11 | \left[\begin{array}{cc|c}0&1&-2\end{array}\right]
@answer 11
@feedback 12 | The constant must also be divided: 6/(-3)=-2.
@feedback 13 | This divides by positive 3 and leaves a negative pivot. It is equivalent but does not perform the requested division by -3.
@after \left[\begin{array}{cc|c}0&1&-2\\0&6&-12\end{array}\right]
@wrong Use the nonzero y coefficient and divide all three entries by the same signed number.
@why Since -3 is nonzero, the divisions are 0/(-3)=0, (-3)/(-3)=1 and 6/(-3)=-2. The pivot belongs to y, not x. Multiplication by -3 reverses the step.
@step 20 | Subtract six times row 1 from row 2. Which complete row results?
@choice 21 | \left[\begin{array}{cc|c}0&0&0\end{array}\right]
@choice 22 | \left[\begin{array}{cc|c}0&0&-12\end{array}\right]
@choice 23 | \left[\begin{array}{cc|c}0&12&-24\end{array}\right]
@answer 21
@feedback 22 | The constant cancels too: -12-6(-2)=0. Retaining -12 invents a contradiction.
@feedback 23 | Adding six times row 1 doubles the y coefficient to 12. It preserves the equation's solutions but does not eliminate that row.
@after \left[\begin{array}{cc|c}0&1&-2\\0&0&0\end{array}\right]
@wrong Subtracting a negative constant adds its positive magnitude.
@why Compute 0-6(0)=0, 6-6(1)=0 and -12-6(-2)=0. Row 1 is unchanged; adding six times it reverses the operation. The second row now imposes no condition.
@step 30 | Let t be any real number and set the non-pivot variable x=t. Which expression describes ALL solutions?
@choice 32 | S=\{\left(t,2\right):t\in\mathbb{R}\}
@choice 31 | S=\{\left(t,-2\right):t\in\mathbb{R}\}
@choice 33 | S=\{\left(0,-2\right)\}
@answer 31
@feedback 32 | The pivot equation fixes y=-2, not positive 2. The freedom in x does not change y.
@feedback 33 | This is the valid t=0 solution, but x is unrestricted. A singleton omits every solution with nonzero x.
@after S=\{\left(t,-2\right):t\in\mathbb{R}\},\quad \left(\operatorname{rank}(A),\operatorname{rank}([A|b])\right)=(1,1)
@wrong The free variable is the one without a coefficient pivot; here that is x.
@why The two ranks are one and there is no contradiction. The pivot equation fixes y=-2 while no equation restricts x. Set x=t for every real t to obtain the full family.
@step 40 | Substitute the family into both ORIGINAL left sides. Which identities hold for every real t?
@choice 42 | \left(-6,12\right)
@choice 43 | \left(0,-12\right)
@choice 41 | \left(6,-12\right)
@answer 41
@feedback 42 | These values use the incorrect y=2. With y=-2, the products are (-3)(-2)=6 and 6(-2)=-12.
@feedback 43 | The first equation includes -3y. Its left side is not merely 0x; including that term gives 6.
@after \begin{gathered}(0)(t)+(-3)(-2)=6\\(0)(t)+(6)(-2)=-12\\S=\{\left(t,-2\right):t\in\mathbb{R}\}\end{gathered}
@wrong Include the nonzero y terms even though the x contributions vanish.
@why The first original left side is 0t+6=6 and the second is 0t-12=-12 for every real t. Conversely, -3y=6 forces y=-2 and neither original equation restricts x. Every and only pair in the stated family works.
@end
