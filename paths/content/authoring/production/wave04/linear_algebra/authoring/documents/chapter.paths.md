@paths 1
@subject linear_algebra | Linear Algebra
@chapter topic_0090 | Vector Spaces and Subspaces

@lesson prod04_linear_span_basis_r | Spanning sets, bases and coordinates
@template lesson.v2
@block introduction | start | 4.1 | What can these vectors make?
@prose A list of vectors can generate a line, a plane or an entire ambient space. We will determine which targets can be made, whether any generator is redundant, and which coefficients describe a target once an ordered basis is chosen. The ambient space and the generated subspace are different objects unless spanning has been proved.
@endblock
@block definition | terms | 4.2 | Columns and combinations
@prose A real vector is an ordered column of real coordinates; a scalar is a real number. A linear combination multiplies each vector by a scalar and adds coordinate by coordinate. Let U be the original vector list u1 through un, and put these vectors into the COLUMNS of A. If c is their coefficient column, Ac is their combination. The number of rows m is the ambient dimension; n is the number of generators. The symbol w denotes a target.
@display
A=[u_1\ \cdots\ u_n],\quad c=\begin{bmatrix}c_1\\ \vdots\\c_n\end{bmatrix},\qquad
Ac=c_1u_1+\cdots+c_nu_n
@prose The span of U is the set of all such combinations with real coefficients. It is a subspace: sums and scalar multiples of combinations remain combinations. A proper subspace is smaller than the ambient space. A set is independent when Ac=0 forces every coefficient to be zero. It is dependent when some nonzero coefficient column c gives Ac=0. The zero coefficient column does not prove dependence.
@endblock
@block proposition | rule | 4.3 | Solve the right system
@prose To test membership solve Ac=w; to test independence solve the homogeneous system Ac=0. In an augmented matrix, the final column is the target or zero. Swap whole rows, divide a whole row by a nonzero scalar, or add a multiple of another whole row. The inverse operations are the same swap, multiplication by the divisor, or addition of the opposite multiple. None may omit the augmented entry.
@display
[A\mid w]\qquad [A\mid0]
@prose A pivot is a leading nonzero entry after elimination. The rank r is the number of independent columns, equal to the number of pivots. A column with no pivot leaves a free coefficient. A zero coefficient row with a nonzero augmented entry is a contradiction. A column ell with ell transpose times A equal to zero is an annihilator of the columns. It proves non-membership when its transpose times the target is nonzero. A transpose changes a column to a row so that these products are dot products.
@display
\ell^TA=0,\quad \ell^Tw\ne0
\quad\Longrightarrow\quad w\notin\operatorname{span}(U)
@endblock
@block definition | condition | 4.4 | A basis is two claims
@prose A basis of a space W is an ordered list of independent vectors spanning W. Its length is the dimension of W. The coordinate column of w lists its unique combination coefficients in that basis order. Independence makes the coefficients unique; spanning makes them exist for every w in W. Two independent vectors in R3 can form a basis of their own plane without forming a basis of R3.
@display
[w]_B=c\quad\Longleftrightarrow\quad w=Bc
@prose To select a column-space basis, locate pivot positions by row reduction and then take those columns from the ORIGINAL matrix in increasing index order. Reduced columns generally belong to a different subspace. Different bases can describe the same space, and changing the ordered basis usually changes coordinates.
@help proof
@prose Complete reversible row operations preserve every coefficient relation among the columns. Thus pivot-column independence and the expressions of other columns in them transfer back to the original columns. If Bc=Bd, subtract to obtain B(c-d)=0. Independence forces c-d=0, proving coordinate uniqueness. These are statements about all real coefficients, not a few numerical samples.
@endblock
@block example | worked | 4.5 | A plane and a target
@prose Let W be the plane x=z. Using parameter order x=s,y=t, construct a basis of W and find the coordinates of the target below. Then reconstruct the target from the original basis vectors.
@display
W=\{(x,y,z)^T:x=z\},\quad w=\begin{bmatrix}2\\3\\2\end{bmatrix}
@help hint
@prose Express a general plane vector using the free parameters s and t. Their two coefficient columns suggest a basis. Check the homogeneous system for independence and the target system for coordinates.
@help answer
@display
B=\begin{bmatrix}1&0\\0&1\\1&0\end{bmatrix},\quad [w]_B=\begin{bmatrix}2\\3\end{bmatrix},\quad\dim W=2
@help solution
@prose Every vector in W is (s,t,s), or s(1,0,1)+t(0,1,0), so those columns span W. A zero combination forces s=0 from the first coordinate and t=0 from the second, proving independence. For the target, subtract row 1 from row 3 across all columns.
@display
\left[\begin{array}{cc|c}1&0&2\\0&1&3\\1&0&2\end{array}\right]
\longrightarrow\left[\begin{array}{cc|c}1&0&2\\0&1&3\\0&0&0\end{array}\right]
@prose The coefficients are 2 and 3 in the chosen order. Original reconstruction gives 2(1,0,1)+3(0,1,0)=(2,3,2). The plane has dimension two; it does not equal R3 because every vector in it has first and third coordinates equal.
@endblock
@block proposition | errors | 4.6 | State what a witness proves
@prose The zero coefficient column always solves a homogeneous system; dependence requires another solution. Any nonzero multiple of a dependence relation proves the same dependence, so a request for one normalized relation must specify the normalization. A valid alternative basis can miss a request for original pivot columns. Coordinates cannot be reordered unless the basis order is changed too. A rank below the ambient dimension does not by itself imply dependence: compare rank with the number of columns.
@endblock
@block exercise | practice | 4.7 | From membership to coordinates
@prose First construct coefficients or a contradiction witness for a target. Next solve homogeneous systems, including a list containing zero. Finally prove both parts of the basis definition and reconstruct vectors from their coordinates, keeping the named subspace and ordered columns explicit.
@endblock
@block summary | summary | 4.8 | Definitions with evidence
@prose Membership needs a combination or an impossibility proof. Independence requires only the zero homogeneous solution; dependence needs a nonzero relation. A basis needs independence and spanning of the named space. Verify every coefficient against the original columns.
@prose Adapted from Jim Hefferon, Linear Algebra, fourth edition (2020-Apr-26), exercises in Two.I.2, Two.II.1 and Two.III.1: https://jheffero.w3.uvm.edu/linearalgebra/book.pdf . These adapted lesson and questions are licensed under Creative Commons Attribution-ShareAlike 3.0 United States: https://creativecommons.org/licenses/by-sa/3.0/us/ . Paths changed specified numbers and tasks and added solving routes, teaching, choices and corrections. The separate worked example adapts Two.I.2.27(a). No Hefferon endorsement is implied. This content notice does not license the surrounding application.
@endblock
@practice prod04_linear_span_basis_q01
@practice prod04_linear_span_basis_q02
@practice prod04_linear_span_basis_q03
@practice prod04_linear_span_basis_q04
@practice prod04_linear_span_basis_q05
@practice prod04_linear_span_basis_q06
@practice prod04_linear_span_basis_q07
@practice prod04_linear_span_basis_q08
@practice prod04_linear_span_basis_q09
@practice prod04_linear_span_basis_q10
@practice prod04_linear_span_basis_q11
@practice prod04_linear_span_basis_q12
@end

@question prod04_linear_span_basis_q01 | Vector investigation 1
@template choices.v1
@version 1
@goal Decide whether w is in the span of the columns of A; construct coefficients and verify them in the original vectors.
@given A=\begin{bmatrix}1&0\\0&0\\1&2\end{bmatrix},\quad w=\begin{bmatrix}3\\0\\5\end{bmatrix}
@domain Scalars are real. The displayed matrices list the original vectors as COLUMNS in the given order. Coefficients and coordinates use that same order. Row operations include the entire augmented row.
@read prod04_linear_span_basis_r
@step 10 | Form [A|w] and replace row 3 by row 3 minus row 1. Which complete matrix results?
@choice 11 | \left[\begin{array}{cc|c}1&0&3\\0&0&0\\0&2&2\end{array}\right]
@choice 12 | \left[\begin{array}{cc|c}1&0&3\\0&0&0\\0&2&5\end{array}\right]
@choice 13 | \left[\begin{array}{cc|c}1&0&3\\0&0&0\\2&2&8\end{array}\right]
@answer 11
@feedback 12 | The target entry also changes: 5-3=2, not 5.
@feedback 13 | Adding row 1 gives [2,2|8]. That is reversible but is not the requested subtraction.
@after \left[\begin{array}{cc|c}1&0&3\\0&0&0\\0&2&2\end{array}\right]
@wrong Include the target column in the subtraction.
@why Row 3 becomes [1-1,2-0|5-3]=[0,2|2]. The other rows stay fixed. Adding row 1 back reverses the operation.
@step 20 | Swap the complete second and third rows to put the zero row last.
@choice 22 | \left[\begin{array}{cc|c}1&0&3\\0&2&0\\0&0&2\end{array}\right]
@choice 21 | \left[\begin{array}{cc|c}1&0&3\\0&2&2\\0&0&0\end{array}\right]
@choice 23 | \left[\begin{array}{cc|c}1&0&3\\0&0&0\\0&2&2\end{array}\right]
@answer 21
@feedback 22 | The target entries must travel with their rows. The row [0,2] keeps target 2.
@feedback 23 | This is unchanged. It still places the zero row before a nonzero row.
@after \left[\begin{array}{cc|c}1&0&3\\0&2&2\\0&0&0\end{array}\right]
@wrong Move the whole row, not only its coefficients.
@why Swap [0,0|0] with [0,2|2]. Swapping again is the inverse; no coefficient equation changes.
@step 30 | Divide the complete second row by 2.
@choice 32 | \left[\begin{array}{cc|c}1&0&3\\0&1&2\\0&0&0\end{array}\right]
@choice 33 | \left[\begin{array}{cc|c}1&0&3\\0&-1&-1\\0&0&0\end{array}\right]
@choice 31 | \left[\begin{array}{cc|c}1&0&3\\0&1&1\\0&0&0\end{array}\right]
@answer 31
@feedback 32 | The target entry must also be divided: 2/2=1.
@feedback 33 | Division by -2 gives an equivalent equation, but not the specified division by +2.
@after \left[\begin{array}{cc|c}1&0&3\\0&1&1\\0&0&0\end{array}\right]
@wrong Use the same nonzero divisor throughout the row.
@why Row 2 becomes [0/2,2/2|2/2]=[0,1|1]. Multiplication by 2 restores it; the last row imposes no restriction.
@step 40 | Read the coefficient column c in the original column order.
@choice 41 | c=\begin{bmatrix}3\\1\end{bmatrix}
@choice 42 | c=\begin{bmatrix}1\\3\end{bmatrix}
@choice 43 | c=\begin{bmatrix}3\\2\end{bmatrix}
@answer 41
@feedback 42 | This reverses the column order. The first pivot fixes c1=3, the second c2=1.
@feedback 43 | The second coefficient is 1 after division, not the earlier target entry 2.
@after c=\begin{bmatrix}3\\1\end{bmatrix}
@wrong Coefficients correspond to original columns, not reordered equations.
@why The pivot equations give c1=3 and c2=1. They determine one candidate combination; verify it in original A.
@step 50 | Compute Ac using the ORIGINAL columns to verify membership.
@choice 52 | Ac=\begin{bmatrix}3\\0\\1\end{bmatrix}
@choice 51 | Ac=\begin{bmatrix}3\\0\\5\end{bmatrix}
@choice 53 | Ac=\begin{bmatrix}3\\0\\3\end{bmatrix}
@answer 51
@feedback 52 | The last coordinate adds contributions: 3(1)+1(2)=5. Subtracting gives 1.
@feedback 53 | This omits the second column's last-coordinate contribution 1(2)=2.
@after Ac=\begin{bmatrix}3\\0\\5\end{bmatrix},\quad w\in\operatorname{span}(U)
@wrong Reconstruct from the original vectors, including every coordinate.
@why The combination is 3(1,0,1)+1(0,0,2)=(3,0,5)=w. This explicitly proves membership; two coefficient pivots also show the coefficients are unique.
@end

@question prod04_linear_span_basis_q02 | Vector investigation 2
@template choices.v1
@version 1
@goal Decide whether w is in the span of the columns of A and give an exact inconsistency witness.
@given A=\begin{bmatrix}2&1\\1&-1\\-1&1\end{bmatrix},\quad w=\begin{bmatrix}1\\0\\3\end{bmatrix}
@domain Scalars are real. The displayed matrices list the original vectors as COLUMNS in the given order. Coefficients and coordinates use that same order. Row operations include the entire augmented row.
@read prod04_linear_span_basis_r
@step 10 | Form [A|w] and swap its complete first and second rows.
@choice 12 | \left[\begin{array}{cc|c}1&-1&1\\2&1&0\\-1&1&3\end{array}\right]
@choice 11 | \left[\begin{array}{cc|c}1&-1&0\\2&1&1\\-1&1&3\end{array}\right]
@choice 13 | \left[\begin{array}{cc|c}2&1&1\\1&-1&0\\-1&1&3\end{array}\right]
@answer 11
@feedback 12 | The target must move with its equation. Row [1,-1] has target 0, not 1.
@feedback 13 | The original augmentation has not been swapped.
@after \left[\begin{array}{cc|c}1&-1&0\\2&1&1\\-1&1&3\end{array}\right]
@wrong Swap both coefficients and the augmented entry.
@why The row [1,-1|0] moves first and [2,1|1] second. Row 3 remains [-1,1|3]. Swapping again restores the original system.
@step 20 | Subtract twice row 1 from row 2 across the entire row.
@choice 22 | \left[\begin{array}{cc|c}1&-1&0\\0&-1&1\\-1&1&3\end{array}\right]
@choice 23 | \left[\begin{array}{cc|c}1&-1&0\\4&-1&1\\-1&1&3\end{array}\right]
@choice 21 | \left[\begin{array}{cc|c}1&-1&0\\0&3&1\\-1&1&3\end{array}\right]
@answer 21
@feedback 22 | The second coefficient is 1-2(-1)=3, not 1-2=-1.
@feedback 23 | Adding twice row 1 gives [4,-1|1]. It is reversible but misses the subtraction goal.
@after \left[\begin{array}{cc|c}1&-1&0\\0&3&1\\-1&1&3\end{array}\right]
@wrong Subtract the negative second entry with its sign intact.
@why Row 2 becomes [2-2(1),1-2(-1)|1-2(0)]=[0,3|1]. Adding twice unchanged row 1 reverses it.
@step 30 | Add row 1 to row 3. Which complete matrix results?
@choice 31 | \left[\begin{array}{cc|c}1&-1&0\\0&3&1\\0&0&3\end{array}\right]
@choice 32 | \left[\begin{array}{cc|c}1&-1&0\\0&3&1\\0&0&0\end{array}\right]
@choice 33 | \left[\begin{array}{cc|c}1&-1&0\\0&3&1\\-2&2&3\end{array}\right]
@answer 31
@feedback 32 | The target is 3+0=3, not zero. Erasing it would hide the contradiction.
@feedback 33 | Subtraction gives [-2,2|3]; the requested addition cancels both coefficients.
@after \left[\begin{array}{cc|c}1&-1&0\\0&3&1\\0&0&3\end{array}\right]
@wrong Zero coefficients do not imply a zero target entry.
@why Row 3 becomes [-1+1,1+(-1)|3+0]=[0,0|3]. This asserts 0=3 and excludes every coefficient pair.
@step 40 | Find an annihilator ell of the ORIGINAL columns, normalized so its second coordinate is 1.
@choice 42 | \ell=\begin{bmatrix}0\\1\\-1\end{bmatrix}
@choice 41 | \ell=\begin{bmatrix}0\\1\\1\end{bmatrix}
@choice 43 | \ell=\begin{bmatrix}1\\1\\1\end{bmatrix}
@answer 41
@feedback 42 | Its dot product with original column 1 is 0+1+1=2, not zero.
@feedback 43 | Its dot product with original column 1 is 2+1-1=2, not zero.
@after \ell=\begin{bmatrix}0\\1\\1\end{bmatrix}
@wrong Solve both original zero dot-product conditions with the stated normalization.
@why Original rows 2 and 3 have opposite coefficients. Their sum is zero in both columns, so ell=(0,1,1) annihilates the column list.
@step 50 | Evaluate ell transpose times [A|w] in the ORIGINAL order. Which row proves impossibility?
@choice 52 | \ell^T[A\mid w]=\begin{bmatrix}0&0&0\end{bmatrix}
@choice 53 | \ell^T[A\mid w]=\begin{bmatrix}2&-2&-3\end{bmatrix}
@choice 51 | \ell^T[A\mid w]=\begin{bmatrix}0&0&3\end{bmatrix}
@answer 51
@feedback 52 | The last dot product is 0(1)+1(0)+1(3)=3, not zero.
@feedback 53 | These values use ell=(0,1,-1) instead of the selected (0,1,1).
@after \ell^T[A\mid w]=\begin{bmatrix}0&0&3\end{bmatrix},\quad w\notin\operatorname{span}(U)
@wrong Test the target with the same annihilator as the generators.
@why The generator dot products are 1-1=0 and -1+1=0, while the target product is 3. Every linear combination would have dot product zero; w does not, so no combination produces it.
@end

@question prod04_linear_span_basis_q03 | Vector investigation 3
@template choices.v1
@version 1
@goal Decide whether w is in the span of the columns of A; construct coefficients and verify them in the original vectors.
@given A=\begin{bmatrix}1&3\\1&0\\0&0\end{bmatrix},\quad w=\begin{bmatrix}2\\1\\0\end{bmatrix}
@domain Scalars are real. The displayed matrices list the original vectors as COLUMNS in the given order. Coefficients and coordinates use that same order. Row operations include the entire augmented row.
@read prod04_linear_span_basis_r
@step 10 | Form [A|w] and subtract row 1 from row 2.
@choice 12 | \left[\begin{array}{cc|c}1&3&2\\0&-3&1\\0&0&0\end{array}\right]
@choice 13 | \left[\begin{array}{cc|c}1&3&2\\2&3&3\\0&0&0\end{array}\right]
@choice 11 | \left[\begin{array}{cc|c}1&3&2\\0&-3&-1\\0&0&0\end{array}\right]
@answer 11
@feedback 12 | The target becomes 1-2=-1, not its original value 1.
@feedback 13 | This adds the rows. It is reversible but leaves first coefficient 2 rather than clearing it.
@after \left[\begin{array}{cc|c}1&3&2\\0&-3&-1\\0&0&0\end{array}\right]
@wrong Subtract the complete row including its target.
@why Row 2 becomes [1-1,0-3|1-2]=[0,-3|-1]. Adding row 1 back reverses the change.
@step 20 | Divide the complete second row by -3.
@choice 21 | \left[\begin{array}{cc|c}1&3&2\\0&1&\frac{1}{3}\\0&0&0\end{array}\right]
@choice 22 | \left[\begin{array}{cc|c}1&3&2\\0&1&-1\\0&0&0\end{array}\right]
@choice 23 | \left[\begin{array}{cc|c}1&3&2\\0&-1&-\frac{1}{3}\\0&0&0\end{array}\right]
@answer 21
@feedback 22 | The target also needs division: (-1)/(-3)=1/3.
@feedback 23 | This divides by +3, an equivalent operation that does not give the requested unit pivot.
@after \left[\begin{array}{cc|c}1&3&2\\0&1&\frac{1}{3}\\0&0&0\end{array}\right]
@wrong Two negative numbers give a positive quotient.
@why Division by nonzero -3 gives [0,1|1/3]. Multiplication by -3 restores the row.
@step 30 | Clear the second coefficient in row 1 by subtracting three times row 2.
@choice 32 | \left[\begin{array}{cc|c}1&0&2\\0&1&\frac{1}{3}\\0&0&0\end{array}\right]
@choice 31 | \left[\begin{array}{cc|c}1&0&1\\0&1&\frac{1}{3}\\0&0&0\end{array}\right]
@choice 33 | \left[\begin{array}{cc|c}1&6&3\\0&1&\frac{1}{3}\\0&0&0\end{array}\right]
@answer 31
@feedback 32 | The target must be 2-3(1/3)=1, not 2.
@feedback 33 | Adding three times row 2 gives second coefficient 6. It misses the requested cancellation.
@after \left[\begin{array}{cc|c}1&0&1\\0&1&\frac{1}{3}\\0&0&0\end{array}\right]
@wrong Apply the multiple to the fractional target too.
@why Row 1 becomes [1-3(0),3-3(1)|2-3(1/3)]=[1,0|1]. Adding the same multiple reverses this move.
@step 40 | Read the exact coefficient column c.
@choice 42 | c=\begin{bmatrix}\frac{1}{3}\\1\end{bmatrix}
@choice 43 | c=\begin{bmatrix}1\\-\frac{1}{3}\end{bmatrix}
@choice 41 | c=\begin{bmatrix}1\\\frac{1}{3}\end{bmatrix}
@answer 41
@feedback 42 | This reverses the original column order. The first coefficient is 1.
@feedback 43 | The second coefficient is positive because (-1)/(-3)=1/3.
@after c=\begin{bmatrix}1\\\frac{1}{3}\end{bmatrix}
@wrong Use the normalized rows, preserving signs and column order.
@why The pivots give c1=1 and c2=1/3. The zero third row imposes no additional condition.
@step 50 | Reconstruct Ac from the ORIGINAL columns.
@choice 51 | Ac=\begin{bmatrix}2\\1\\0\end{bmatrix}
@choice 52 | Ac=\begin{bmatrix}0\\1\\0\end{bmatrix}
@choice 53 | Ac=\begin{bmatrix}2\\0\\0\end{bmatrix}
@answer 51
@feedback 52 | The first coordinate is 1(1)+(1/3)3=2. Subtracting the second contribution gives 0.
@feedback 53 | The first column contributes 1 to the second coordinate; it cannot be omitted.
@after Ac=\begin{bmatrix}2\\1\\0\end{bmatrix},\quad w\in\operatorname{span}(U)
@wrong Add every coordinate contribution.
@why The combination is (1,1,0)+(1/3)(3,0,0)=(2,1,0)=w. Membership of this target does not prove spanning R3: every generated vector has third coordinate zero, excluding (0,0,1).
@end

@question prod04_linear_span_basis_q04 | Vector investigation 4
@template choices.v1
@version 1
@goal Decide whether w is in the span of the columns of A and give an exact inconsistency witness.
@given A=\begin{bmatrix}2&0\\0&0\\0&1\end{bmatrix},\quad w=\begin{bmatrix}1\\1\\2\end{bmatrix}
@domain Scalars are real. The displayed matrices list the original vectors as COLUMNS in the given order. Coefficients and coordinates use that same order. Row operations include the entire augmented row.
@read prod04_linear_span_basis_r
@step 10 | Form [A|w] and swap complete rows 2 and 3.
@choice 11 | \left[\begin{array}{cc|c}2&0&1\\0&1&2\\0&0&1\end{array}\right]
@choice 12 | \left[\begin{array}{cc|c}2&0&1\\0&1&1\\0&0&2\end{array}\right]
@choice 13 | \left[\begin{array}{cc|c}2&0&1\\0&0&1\\0&1&2\end{array}\right]
@answer 11
@feedback 12 | The target entries did not move with the coefficient rows. The [0,1] row requires target 2.
@feedback 13 | This is the original augmentation, not the requested swap.
@after \left[\begin{array}{cc|c}2&0&1\\0&1&2\\0&0&1\end{array}\right]
@wrong Carry the augmented entry with its entire row.
@why Swapping gives [0,1|2] above [0,0|1]. The final row already states the contradiction 0=1.
@step 20 | Choose ell annihilating the ORIGINAL columns, normalized to second coordinate 1.
@choice 22 | \ell=\begin{bmatrix}1\\1\\0\end{bmatrix}
@choice 21 | \ell=\begin{bmatrix}0\\1\\0\end{bmatrix}
@choice 23 | \ell=\begin{bmatrix}0\\1\\1\end{bmatrix}
@answer 21
@feedback 22 | Its dot product with column 1 is 1(2)=2, not zero.
@feedback 23 | Its dot product with column 2 is 1, not zero.
@after \ell=\begin{bmatrix}0\\1\\0\end{bmatrix}
@wrong Use the original missing coordinate, not a reordered row index.
@why Both original generators have second coordinate zero. The column ell=(0,1,0) extracts exactly that coordinate and annihilates both.
@step 30 | Evaluate ell transpose times the ORIGINAL [A|w].
@choice 32 | \ell^T[A\mid w]=\begin{bmatrix}0&0&0\end{bmatrix}
@choice 33 | \ell^T[A\mid w]=\begin{bmatrix}0&1&2\end{bmatrix}
@choice 31 | \ell^T[A\mid w]=\begin{bmatrix}0&0&1\end{bmatrix}
@answer 31
@feedback 32 | The target's second coordinate is 1, so its dot product is 1, not zero.
@feedback 33 | This extracts the original third row instead of the second; it uses ell=(0,0,1).
@after \ell^T[A\mid w]=\begin{bmatrix}0&0&1\end{bmatrix},\quad w\notin\operatorname{span}(U)
@wrong Apply one and the same original-coordinate functional to every column and the target.
@why The generators give zero and zero, while w gives one. Every generated vector lies in the xz-plane, but w has second coordinate one. Hence it cannot belong to the span.
@end

@question prod04_linear_span_basis_q05 | Vector investigation 5
@template choices.v1
@version 1
@goal Determine independence from the full homogeneous system, and distinguish the column span from the ambient space.
@given A=\begin{bmatrix}1&-1\\2&1\\0&0\end{bmatrix}
@domain Scalars are real. The displayed matrices list the original vectors as COLUMNS in the given order. Coefficients and coordinates use that same order. Row operations include the entire augmented row.
@read prod04_linear_span_basis_r
@step 10 | Form the HOMOGENEOUS augmentation [A|0]. Subtract twice row 1 from row 2.
@choice 12 | \left[\begin{array}{cc|c}1&-1&0\\0&-1&0\\0&0&0\end{array}\right]
@choice 11 | \left[\begin{array}{cc|c}1&-1&0\\0&3&0\\0&0&0\end{array}\right]
@choice 13 | \left[\begin{array}{cc|c}1&-1&0\\4&-1&0\\0&0&0\end{array}\right]
@answer 11
@feedback 12 | The second coefficient is 1-2(-1)=3, not -1.
@feedback 13 | This adds twice row 1. Its first coefficient is 4, so it does not perform the requested elimination.
@after \left[\begin{array}{cc|c}1&-1&0\\0&3&0\\0&0&0\end{array}\right]
@wrong The zero target belongs to the dependence test, not to either original vector.
@why We are solving c1(1,2,0)+c2(-1,1,0)=0. Row 2 becomes [2-2,1-2(-1)|0]=[0,3|0]; the operation is reversible.
@step 20 | Divide the complete second row by 3.
@choice 22 | \left[\begin{array}{cc|c}1&-1&0\\0&1&1\\0&0&0\end{array}\right]
@choice 23 | \left[\begin{array}{cc|c}1&-1&0\\0&-1&0\\0&0&0\end{array}\right]
@choice 21 | \left[\begin{array}{cc|c}1&-1&0\\0&1&0\\0&0&0\end{array}\right]
@answer 21
@feedback 22 | The zero target stays zero: 0/3=0, not 1.
@feedback 23 | Division by -3 gives a negative pivot; the requested divisor is +3.
@after \left[\begin{array}{cc|c}1&-1&0\\0&1&0\\0&0&0\end{array}\right]
@wrong Divide the coefficient and target by the same nonzero number.
@why The row becomes [0,1|0], fixing c2=0. Multiplying by 3 reverses the step.
@step 30 | Add row 2 to row 1 to clear its second coefficient.
@choice 31 | \left[\begin{array}{cc|c}1&0&0\\0&1&0\\0&0&0\end{array}\right]
@choice 32 | \left[\begin{array}{cc|c}1&-2&0\\0&1&0\\0&0&0\end{array}\right]
@choice 33 | \left[\begin{array}{cc|c}1&0&1\\0&1&0\\0&0&0\end{array}\right]
@answer 31
@feedback 32 | Subtracting row 2 gives -1-1=-2. Adding it gives zero.
@feedback 33 | The target is 0+0=0, not 1.
@after \left[\begin{array}{cc|c}1&0&0\\0&1&0\\0&0&0\end{array}\right]
@wrong Keep the homogeneous target zero throughout.
@why Row 1 becomes [1+0,-1+1|0+0]=[1,0|0]. Subtracting row 2 reverses it. Both coefficients are now fixed.
@step 40 | Which coefficient column is the ONLY solution of Ac=0?
@choice 42 | c=\begin{bmatrix}1\\0\end{bmatrix}
@choice 41 | c=\begin{bmatrix}0\\0\end{bmatrix}
@choice 43 | c=\begin{bmatrix}0\\1\end{bmatrix}
@answer 41
@feedback 42 | This produces original column 1=(1,2,0), not the zero vector.
@feedback 43 | This produces original column 2=(-1,1,0), not the zero vector.
@after c=\begin{bmatrix}0\\0\end{bmatrix}
@wrong Independence requires that no nonzero coefficient column work.
@why The two pivot equations require c1=c2=0. Conversely those coefficients reconstruct zero. Thus the only relation is trivial and the columns are independent.
@step 50 | Let r be rank(A) and m the ambient dimension. Which pair explains why these independent columns still do not span the ambient space?
@choice 52 | (r,m)=(1,3)
@choice 53 | (r,m)=(3,3)
@choice 51 | (r,m)=(2,3)
@answer 51
@feedback 52 | There are two independent columns and two pivots, so rank is 2, not 1.
@feedback 53 | Two columns cannot have rank 3. The third coordinate of every combination is zero.
@after (r,m)=(2,3)
@wrong Compare rank with the number of columns for independence, but with ambient dimension for spanning.
@why Rank equals the two-column count, proving independence, but is below ambient dimension 3. Every combination has third coordinate zero, so (0,0,1) is excluded. The columns form a basis of their plane, not of R3.
@end

@question prod04_linear_span_basis_q06 | Vector investigation 6
@template choices.v1
@version 1
@goal Determine dependence from a homogeneous system, construct a nonzero coefficient relation and verify it in the original columns.
@given A=\begin{bmatrix}1&2&3\\7&7&7\\7&7&7\end{bmatrix}
@domain Scalars are real. The displayed matrices list the original vectors as COLUMNS in the given order. Coefficients and coordinates use that same order. Row operations include the entire augmented row.
@read prod04_linear_span_basis_r
@step 10 | Form [A|0] and subtract seven times row 1 from row 2.
@choice 12 | \left[\begin{array}{ccc|c}1&2&3&0\\0&7&7&0\\7&7&7&0\end{array}\right]
@choice 13 | \left[\begin{array}{ccc|c}1&2&3&0\\14&21&28&0\\7&7&7&0\end{array}\right]
@choice 11 | \left[\begin{array}{ccc|c}1&2&3&0\\0&-7&-14&0\\7&7&7&0\end{array}\right]
@answer 11
@feedback 12 | Every coefficient changes: 7-7(2)=-7 and 7-7(3)=-14.
@feedback 13 | This adds seven copies, giving first coefficient 14, rather than cancelling it.
@after \left[\begin{array}{ccc|c}1&2&3&0\\0&-7&-14&0\\7&7&7&0\end{array}\right]
@wrong Apply the multiple to all three coefficient columns.
@why Row 2 becomes [7-7,7-14,7-21|0]=[0,-7,-14|0]. Adding seven times row 1 restores it.
@step 20 | Subtract seven times unchanged row 1 from row 3.
@choice 21 | \left[\begin{array}{ccc|c}1&2&3&0\\0&-7&-14&0\\0&-7&-14&0\end{array}\right]
@choice 22 | \left[\begin{array}{ccc|c}1&2&3&0\\0&-7&-14&0\\0&7&7&0\end{array}\right]
@choice 23 | \left[\begin{array}{ccc|c}1&2&3&0\\0&-7&-14&0\\14&21&28&0\end{array}\right]
@answer 21
@feedback 22 | Only the first coefficient was updated. The other two are 7-14=-7 and 7-21=-14.
@feedback 23 | This adds seven times row 1; the requested move subtracts it.
@after \left[\begin{array}{ccc|c}1&2&3&0\\0&-7&-14&0\\0&-7&-14&0\end{array}\right]
@wrong Use the retained first row for each entry of the replacement.
@why Row 3 has the same original entries as row 2, so this subtraction also gives [0,-7,-14|0]. The move is reversible.
@step 30 | Subtract row 2 from row 3.
@choice 32 | \left[\begin{array}{ccc|c}1&2&3&0\\0&-7&-14&0\\0&0&-14&0\end{array}\right]
@choice 31 | \left[\begin{array}{ccc|c}1&2&3&0\\0&-7&-14&0\\0&0&0&0\end{array}\right]
@choice 33 | \left[\begin{array}{ccc|c}1&2&3&0\\0&-7&-14&0\\0&-14&-28&0\end{array}\right]
@answer 31
@feedback 32 | The third coefficient also cancels: -14-(-14)=0.
@feedback 33 | Adding equal rows doubles them; subtracting produces the zero row.
@after \left[\begin{array}{ccc|c}1&2&3&0\\0&-7&-14&0\\0&0&0&0\end{array}\right]
@wrong Subtract every signed entry, including negatives.
@why Row 3 becomes [0-0,-7-(-7),-14-(-14)|0]=[0,0,0|0]. It adds no condition on the coefficients.
@step 40 | Divide the complete second row by -7.
@choice 42 | \left[\begin{array}{ccc|c}1&2&3&0\\0&1&-14&0\\0&0&0&0\end{array}\right]
@choice 43 | \left[\begin{array}{ccc|c}1&2&3&0\\0&-1&-2&0\\0&0&0&0\end{array}\right]
@choice 41 | \left[\begin{array}{ccc|c}1&2&3&0\\0&1&2&0\\0&0&0&0\end{array}\right]
@answer 41
@feedback 42 | The third coefficient must also divide: (-14)/(-7)=2.
@feedback 43 | This divides by positive 7, leaving pivot -1 instead of the requested 1.
@after \left[\begin{array}{ccc|c}1&2&3&0\\0&1&2&0\\0&0&0&0\end{array}\right]
@wrong Divide every coefficient by the same signed nonzero number.
@why The second row becomes [0,1,2|0], meaning c2+2c3=0. Multiplication by -7 reverses it.
@step 50 | Subtract twice row 2 from row 1 to finish the coefficient reduction.
@choice 51 | \left[\begin{array}{ccc|c}1&0&-1&0\\0&1&2&0\\0&0&0&0\end{array}\right]
@choice 52 | \left[\begin{array}{ccc|c}1&0&3&0\\0&1&2&0\\0&0&0&0\end{array}\right]
@choice 53 | \left[\begin{array}{ccc|c}1&4&7&0\\0&1&2&0\\0&0&0&0\end{array}\right]
@answer 51
@feedback 52 | The third coefficient changes too: 3-2(2)=-1.
@feedback 53 | This adds twice row 2, giving [1,4,7|0], not the requested cancellation.
@after \left[\begin{array}{ccc|c}1&0&-1&0\\0&1&2&0\\0&0&0&0\end{array}\right]
@wrong Keep the free-column coefficient in the row calculation.
@why Row 1 becomes [1,2-2,3-4|0]=[1,0,-1|0]. The equations give c1=c3 and c2=-2c3, with c3 free.
@step 60 | Choose the nonzero relation normalized to c3=1.
@choice 62 | c=\begin{bmatrix}-1\\-2\\1\end{bmatrix}
@choice 61 | c=\begin{bmatrix}1\\-2\\1\end{bmatrix}
@choice 63 | c=\begin{bmatrix}1\\2\\1\end{bmatrix}
@answer 61
@feedback 62 | The first equation is c1-c3=0, so c1=1, not -1.
@feedback 63 | The second equation is c2+2c3=0, so c2=-2, not +2.
@after c=\begin{bmatrix}1\\-2\\1\end{bmatrix}
@wrong Use the fixed normalization in both pivot equations.
@why Set c3=1, then c1=1 and c2=-2. This coefficient column is nonzero and is a candidate dependence relation.
@step 70 | Evaluate Ac in the ORIGINAL columns to verify the nonzero relation.
@choice 72 | Ac=\begin{bmatrix}8\\28\\28\end{bmatrix}
@choice 73 | Ac=\begin{bmatrix}-2\\-14\\-14\end{bmatrix}
@choice 71 | Ac=\begin{bmatrix}0\\0\\0\end{bmatrix}
@answer 71
@feedback 72 | These values use +2 instead of c2=-2: the required first coordinate is 1-4+3=0.
@feedback 73 | These values use c1=-1. The selected relation uses c1=1 and gives 7-14+7=0 in each remaining coordinate.
@after Ac=\begin{bmatrix}0\\0\\0\end{bmatrix},\quad c\ne0
@wrong Verify every original coordinate with the same coefficient triple.
@why The original combination is (1,7,7)-2(2,7,7)+(3,7,7)=(0,0,0). Since c3=1, the relation is nontrivial and the columns are dependent.
@end

@question prod04_linear_span_basis_q07 | Vector investigation 7
@template choices.v1
@version 1
@goal Determine dependence from a homogeneous system, construct a nonzero coefficient relation and verify it in the original columns.
@given A=\begin{bmatrix}0&1&0\\0&0&0\\-1&4&0\end{bmatrix}
@domain Scalars are real. The displayed matrices list the original vectors as COLUMNS in the given order. Coefficients and coordinates use that same order. Row operations include the entire augmented row.
@read prod04_linear_span_basis_r
@step 10 | Form [A|0] and swap complete rows 1 and 3.
@choice 11 | \left[\begin{array}{ccc|c}-1&4&0&0\\0&0&0&0\\0&1&0&0\end{array}\right]
@choice 12 | \left[\begin{array}{ccc|c}-1&1&0&0\\0&0&0&0\\0&4&0&0\end{array}\right]
@choice 13 | \left[\begin{array}{ccc|c}0&1&0&0\\0&0&0&0\\-1&4&0&0\end{array}\right]
@answer 11
@feedback 12 | The second coefficients 4 and 1 must travel with their rows; swapping only the first column is not a row swap.
@feedback 13 | This leaves the original zero first pivot unchanged.
@after \left[\begin{array}{ccc|c}-1&4&0&0\\0&0&0&0\\0&1&0&0\end{array}\right]
@wrong Exchange all four entries of each selected row.
@why The complete row [-1,4,0|0] moves first. The row [0,1,0|0] moves third; swapping again is the inverse.
@step 20 | Swap rows 2 and 3 to move the zero row last.
@choice 22 | \left[\begin{array}{ccc|c}0&1&0&0\\0&0&0&0\\-1&4&0&0\end{array}\right]
@choice 21 | \left[\begin{array}{ccc|c}-1&4&0&0\\0&1&0&0\\0&0&0&0\end{array}\right]
@choice 23 | \left[\begin{array}{ccc|c}-1&4&0&0\\0&0&0&0\\0&1&0&0\end{array}\right]
@answer 21
@feedback 22 | This swaps rows 1 and 3 instead. The requested move retains the first row.
@feedback 23 | This performs no swap and leaves a zero row above a nonzero one.
@after \left[\begin{array}{ccc|c}-1&4&0&0\\0&1&0&0\\0&0&0&0\end{array}\right]
@wrong Retain row 1 while exchanging the other two complete rows.
@why The rows [0,0,0|0] and [0,1,0|0] exchange positions. Their coefficient equations are unchanged.
@step 30 | Subtract four times row 2 from row 1.
@choice 32 | \left[\begin{array}{ccc|c}-1&4&0&0\\0&1&0&0\\0&0&0&0\end{array}\right]
@choice 33 | \left[\begin{array}{ccc|c}-1&8&0&0\\0&1&0&0\\0&0&0&0\end{array}\right]
@choice 31 | \left[\begin{array}{ccc|c}-1&0&0&0\\0&1&0&0\\0&0&0&0\end{array}\right]
@answer 31
@feedback 32 | This leaves 4 unchanged rather than computing 4-4(1)=0.
@feedback 33 | Adding four times row 2 gives 8; subtraction gives zero.
@after \left[\begin{array}{ccc|c}-1&0&0&0\\0&1&0&0\\0&0&0&0\end{array}\right]
@wrong Apply the requested signed multiple.
@why Row 1 becomes [-1-4(0),4-4(1),0|0]=[-1,0,0|0]. Adding four times row 2 reverses the change.
@step 40 | Divide the complete first row by -1.
@choice 41 | \left[\begin{array}{ccc|c}1&0&0&0\\0&1&0&0\\0&0&0&0\end{array}\right]
@choice 42 | \left[\begin{array}{ccc|c}-1&0&0&0\\0&1&0&0\\0&0&0&0\end{array}\right]
@choice 43 | \left[\begin{array}{ccc|c}1&0&0&1\\0&1&0&0\\0&0&0&0\end{array}\right]
@answer 41
@feedback 42 | The leading entry must become (-1)/(-1)=1.
@feedback 43 | The homogeneous target remains 0/(-1)=0, not 1.
@after \left[\begin{array}{ccc|c}1&0&0&0\\0&1&0&0\\0&0&0&0\end{array}\right]
@wrong Division by a nonzero scalar does not turn zero into one.
@why The pivot equations are c1=0 and c2=0. There is no equation restricting c3, because the original third column is zero.
@step 50 | Choose the nonzero relation normalized to c3=1.
@choice 52 | c=\begin{bmatrix}1\\0\\1\end{bmatrix}
@choice 51 | c=\begin{bmatrix}0\\0\\1\end{bmatrix}
@choice 53 | c=\begin{bmatrix}0\\1\\1\end{bmatrix}
@answer 51
@feedback 52 | The first pivot equation requires c1=0. Adding original column 1 does not give zero.
@feedback 53 | The second pivot equation requires c2=0. Adding original column 2 does not give zero.
@after c=\begin{bmatrix}0\\0\\1\end{bmatrix}
@wrong A nonzero coefficient may multiply a zero generator.
@why The free coefficient c3 can be 1 while c1=c2=0. The coefficient column is nonzero even though its resulting vector will be zero.
@step 60 | Verify Ac using the ORIGINAL three columns.
@choice 62 | Ac=\begin{bmatrix}0\\0\\-1\end{bmatrix}
@choice 63 | Ac=\begin{bmatrix}1\\0\\4\end{bmatrix}
@choice 61 | Ac=\begin{bmatrix}0\\0\\0\end{bmatrix}
@answer 61
@feedback 62 | This is column 1, but its coefficient is zero.
@feedback 63 | This is column 2, but its coefficient is also zero.
@after Ac=\begin{bmatrix}0\\0\\0\end{bmatrix},\quad c\ne0
@wrong Distinguish the nonzero coefficient column from the zero combination.
@why The combination is 0(0,0,-1)+0(1,0,4)+1(0,0,0)=0. It has c3=1, proving dependence. A list containing a zero generator cannot be independent.
@end

@question prod04_linear_span_basis_q08 | Vector investigation 8
@template choices.v1
@version 1
@goal Determine dependence from a homogeneous system, construct a nonzero coefficient relation and verify it in the original columns.
@given A=\begin{bmatrix}1&-1&-1\\1&2&5\\1&3&7\end{bmatrix}
@domain Scalars are real. The displayed matrices list the original vectors as COLUMNS in the given order. Coefficients and coordinates use that same order. Row operations include the entire augmented row.
@read prod04_linear_span_basis_r
@step 10 | Form [A|0] and subtract row 1 from row 2.
@choice 12 | \left[\begin{array}{ccc|c}1&-1&-1&0\\0&1&4&0\\1&3&7&0\end{array}\right]
@choice 11 | \left[\begin{array}{ccc|c}1&-1&-1&0\\0&3&6&0\\1&3&7&0\end{array}\right]
@choice 13 | \left[\begin{array}{ccc|c}1&-1&-1&0\\2&1&4&0\\1&3&7&0\end{array}\right]
@answer 11
@feedback 12 | Subtracting the negative entries gives 2-(-1)=3 and 5-(-1)=6.
@feedback 13 | This adds the rows instead of subtracting them.
@after \left[\begin{array}{ccc|c}1&-1&-1&0\\0&3&6&0\\1&3&7&0\end{array}\right]
@wrong Preserve both negative signs in the source row.
@why Row 2 becomes [1-1,2-(-1),5-(-1)|0]=[0,3,6|0]. Adding row 1 reverses it.
@step 20 | Subtract row 1 from row 3.
@choice 22 | \left[\begin{array}{ccc|c}1&-1&-1&0\\0&3&6&0\\0&2&6&0\end{array}\right]
@choice 23 | \left[\begin{array}{ccc|c}1&-1&-1&0\\0&3&6&0\\2&2&6&0\end{array}\right]
@choice 21 | \left[\begin{array}{ccc|c}1&-1&-1&0\\0&3&6&0\\0&4&8&0\end{array}\right]
@answer 21
@feedback 22 | The last two coefficients are 3-(-1)=4 and 7-(-1)=8, not 2 and 6.
@feedback 23 | Adding produces [2,2,6|0]. The requested subtraction clears the first coefficient.
@after \left[\begin{array}{ccc|c}1&-1&-1&0\\0&3&6&0\\0&4&8&0\end{array}\right]
@wrong Subtract the signed source entry in every column.
@why Row 3 becomes [0,4,8|0]. Row 1 remains fixed, so adding it back reverses the move.
@step 30 | Divide the complete second row by 3.
@choice 31 | \left[\begin{array}{ccc|c}1&-1&-1&0\\0&1&2&0\\0&4&8&0\end{array}\right]
@choice 32 | \left[\begin{array}{ccc|c}1&-1&-1&0\\0&1&6&0\\0&4&8&0\end{array}\right]
@choice 33 | \left[\begin{array}{ccc|c}1&-1&-1&0\\0&-1&-2&0\\0&4&8&0\end{array}\right]
@answer 31
@feedback 32 | The third coefficient must also be divided: 6/3=2.
@feedback 33 | This divides by -3, not by the specified positive 3.
@after \left[\begin{array}{ccc|c}1&-1&-1&0\\0&1&2&0\\0&4&8&0\end{array}\right]
@wrong Normalize with a nonzero divisor applied to the entire row.
@why Row 2 is [0,1,2|0], or c2+2c3=0. Multiplying by 3 restores the earlier equation.
@step 40 | Subtract four times row 2 from row 3.
@choice 42 | \left[\begin{array}{ccc|c}1&-1&-1&0\\0&1&2&0\\0&0&8&0\end{array}\right]
@choice 41 | \left[\begin{array}{ccc|c}1&-1&-1&0\\0&1&2&0\\0&0&0&0\end{array}\right]
@choice 43 | \left[\begin{array}{ccc|c}1&-1&-1&0\\0&1&2&0\\0&8&16&0\end{array}\right]
@answer 41
@feedback 42 | The third coefficient cancels too: 8-4(2)=0.
@feedback 43 | This adds four times row 2, doubling the nonzero entries rather than cancelling them.
@after \left[\begin{array}{ccc|c}1&-1&-1&0\\0&1&2&0\\0&0&0&0\end{array}\right]
@wrong Apply the same multiple to both nonzero coefficients.
@why Row 3 becomes [0,4-4,8-8|0]=[0,0,0|0]. This identity leaves a free coefficient.
@step 50 | Add row 2 to row 1 to clear its second coefficient.
@choice 52 | \left[\begin{array}{ccc|c}1&0&-1&0\\0&1&2&0\\0&0&0&0\end{array}\right]
@choice 53 | \left[\begin{array}{ccc|c}1&-2&-3&0\\0&1&2&0\\0&0&0&0\end{array}\right]
@choice 51 | \left[\begin{array}{ccc|c}1&0&1&0\\0&1&2&0\\0&0&0&0\end{array}\right]
@answer 51
@feedback 52 | The third coefficient also changes: -1+2=1, not -1.
@feedback 53 | Subtracting row 2 gives [1,-2,-3|0]. Addition is required to clear -1.
@after \left[\begin{array}{ccc|c}1&0&1&0\\0&1&2&0\\0&0&0&0\end{array}\right]
@wrong Add the whole row, including its free-column coefficient.
@why The equations are c1+c3=0 and c2+2c3=0, so c1=-c3 and c2=-2c3. Every real c3 gives a homogeneous solution.
@step 60 | Select the nonzero relation normalized to c3=1.
@choice 61 | c=\begin{bmatrix}-1\\-2\\1\end{bmatrix}
@choice 62 | c=\begin{bmatrix}1\\-2\\1\end{bmatrix}
@choice 63 | c=\begin{bmatrix}-1\\2\\1\end{bmatrix}
@answer 61
@feedback 62 | The first equation requires c1=-c3=-1, not +1.
@feedback 63 | The second equation requires c2=-2c3=-2, not +2.
@after c=\begin{bmatrix}-1\\-2\\1\end{bmatrix}
@wrong Substitute the normalization into both pivot equations.
@why Setting c3=1 gives (-1,-2,1). The coefficient column is nonzero and satisfies the entire reduced homogeneous system.
@step 70 | Check Ac against the ORIGINAL vector entries.
@choice 72 | Ac=\begin{bmatrix}2\\2\\2\end{bmatrix}
@choice 71 | Ac=\begin{bmatrix}0\\0\\0\end{bmatrix}
@choice 73 | Ac=\begin{bmatrix}-4\\8\\12\end{bmatrix}
@answer 71
@feedback 72 | These values use c1=+1 instead of -1. Changing that coefficient adds twice column 1.
@feedback 73 | These values use c2=+2 instead of -2, adding four times column 2=(-1,2,3).
@after Ac=\begin{bmatrix}0\\0\\0\end{bmatrix},\quad c\ne0
@wrong Reuse one coefficient triple in every original coordinate.
@why The coordinates are -1+2-1=0, -1-4+5=0 and -1-6+7=0. Hence -u1-2u2+u3=0 is a nontrivial relation and the columns are dependent.
@end

@question prod04_linear_span_basis_q09 | Vector investigation 9
@template choices.v1
@version 1
@goal Verify that the stated ordered columns form a basis of the ambient space, then find and check the coordinates of w.
@given A=\begin{bmatrix}1&-1\\1&1\end{bmatrix},\quad w=\begin{bmatrix}1\\2\end{bmatrix}
@domain Scalars are real. The displayed matrices list the original vectors as COLUMNS in the given order. Coefficients and coordinates use that same order. Row operations include the entire augmented row.
@read prod04_linear_span_basis_r
@step 10 | Choose the coefficient system reconstructing w from the ORIGINAL columns in their stated order.
@choice 12 | A^Tc=w
@choice 13 | Ac=0
@choice 11 | Ac=w
@answer 11
@feedback 12 | Transposing uses columns (1,-1) and (1,1), not the displayed (1,1) and (-1,1).
@feedback 13 | The homogeneous system tests independence; it does not reconstruct the nonzero target (1,2).
@after \left[\begin{array}{cc|c}1&-1&1\\1&1&2\end{array}\right]
@wrong Match the system to the requested combination.
@why The equation c1(1,1)+c2(-1,1)=(1,2) is Ac=w. Its scalar equations are c1-c2=1 and c1+c2=2.
@step 20 | Subtract row 1 from row 2 across the complete augmentation.
@choice 21 | \left[\begin{array}{cc|c}1&-1&1\\0&2&1\end{array}\right]
@choice 22 | \left[\begin{array}{cc|c}1&-1&1\\0&2&2\end{array}\right]
@choice 23 | \left[\begin{array}{cc|c}1&-1&1\\2&0&3\end{array}\right]
@answer 21
@feedback 22 | The second target becomes 2-1=1, not 2.
@feedback 23 | This adds row 1, giving [2,0|3]. It is reversible but not the requested subtraction.
@after \left[\begin{array}{cc|c}1&-1&1\\0&2&1\end{array}\right]
@wrong Include the target in the subtraction.
@why Row 2 is [1-1,1-(-1)|2-1]=[0,2|1]. Adding row 1 reverses the operation.
@step 30 | Divide the complete second row by 2.
@choice 32 | \left[\begin{array}{cc|c}1&-1&1\\0&1&1\end{array}\right]
@choice 31 | \left[\begin{array}{cc|c}1&-1&1\\0&1&\frac{1}{2}\end{array}\right]
@choice 33 | \left[\begin{array}{cc|c}1&-1&1\\0&4&2\end{array}\right]
@answer 31
@feedback 32 | The target also divides by 2, giving 1/2 rather than 1.
@feedback 33 | Multiplication gives [0,4|2]; division was requested.
@after \left[\begin{array}{cc|c}1&-1&1\\0&1&\frac{1}{2}\end{array}\right]
@wrong Keep exact fractions.
@why Row 2 becomes [0/2,2/2|1/2]=[0,1|1/2]. Multiplication by the nonzero divisor reverses it.
@step 40 | Add row 2 to row 1 to clear its second coefficient.
@choice 42 | \left[\begin{array}{cc|c}1&-1&1\\0&1&\frac{1}{2}\end{array}\right]
@choice 43 | \left[\begin{array}{cc|c}1&-2&\frac{1}{2}\\0&1&\frac{1}{2}\end{array}\right]
@choice 41 | \left[\begin{array}{cc|c}1&0&\frac{3}{2}\\0&1&\frac{1}{2}\end{array}\right]
@answer 41
@feedback 42 | This is unchanged; the -1 coefficient has not been cleared.
@feedback 43 | Subtraction gives coefficient -2 and target 1/2. Addition is required to clear -1.
@after \left[\begin{array}{cc|c}1&0&\frac{3}{2}\\0&1&\frac{1}{2}\end{array}\right]
@wrong Update every entry of row 1.
@why Row 1 becomes [1+0,-1+1|1+1/2]=[1,0|3/2]. Subtracting row 2 reverses it.
@step 50 | Let r be rank(A) and m the ambient dimension. Which pair establishes a basis of R2?
@choice 51 | (r,m)=(2,2)
@choice 52 | (r,m)=(1,2)
@choice 53 | (r,m)=(2,3)
@answer 51
@feedback 52 | Both coefficient columns have pivots; rank is 2, not 1.
@feedback 53 | Each original vector has two entries, so the ambient dimension is 2, not 3.
@after (r,m)=(2,2)
@wrong A basis needs independence and spanning of the named space.
@why Two column pivots force the homogeneous coefficients to zero. A pivot in every ambient row makes the system solvable for every R2 target. Thus these original columns are independent and span R2.
@step 60 | Read c in the original basis order.
@choice 62 | c=\begin{bmatrix}\frac{1}{2}\\\frac{3}{2}\end{bmatrix}
@choice 61 | c=\begin{bmatrix}\frac{3}{2}\\\frac{1}{2}\end{bmatrix}
@choice 63 | c=\begin{bmatrix}1\\2\end{bmatrix}
@answer 61
@feedback 62 | Reversed coefficients give first coordinate 1/2-3/2=-1, not 1.
@feedback 63 | Copying target entries as coefficients gives first coordinate 1-2=-1, not 1.
@after c=\begin{bmatrix}\frac{3}{2}\\\frac{1}{2}\end{bmatrix}
@wrong Coordinates are basis coefficients, not necessarily the target entries.
@why The reduced equations give c1=3/2 and c2=1/2. Independence makes these coefficients unique.
@step 70 | Evaluate Ac using the ORIGINAL columns.
@choice 72 | Ac=\begin{bmatrix}-1\\2\end{bmatrix}
@choice 73 | Ac=\begin{bmatrix}2\\1\end{bmatrix}
@choice 71 | Ac=\begin{bmatrix}1\\2\end{bmatrix}
@answer 71
@feedback 72 | This uses reversed coefficients: (1/2)(1,1)+(3/2)(-1,1)=(-1,2).
@feedback 73 | This reverses target entries. The first combination coordinate is 3/2-1/2=1.
@after Ac=\begin{bmatrix}1\\2\end{bmatrix}
@wrong Multiply the coefficients by original columns.
@why The original reconstruction is (3/2)(1,1)+(1/2)(-1,1)=(3/2-1/2,3/2+1/2)=(1,2)=w.
@end

@question prod04_linear_span_basis_q10 | Vector investigation 10
@template choices.v1
@version 1
@goal Construct a basis of W=span(U) from original pivot columns in increasing order; find and verify the coordinates of w in that ordered basis.
@given A=\begin{bmatrix}2&3&5&6\\1&0&1&0\\1&1&2&2\end{bmatrix},\quad w=\begin{bmatrix}-1\\1\\0\end{bmatrix}
@domain Scalars are real. The displayed matrices list the original vectors as COLUMNS in the given order. Coefficients and coordinates use that same order. Row operations include the entire augmented row.
@read prod04_linear_span_basis_r
@step 10 | Choose a method placing EXISTING row 2 at the top as a unit-pivot row, while preserving integer entries.
@choice 11 | R_1\leftrightarrow R_2
@choice 12 | R_1\leftarrow\frac{1}{2}R_1
@choice 13 | R_1\leftarrow R_1+R_2
@answer 11
@feedback 12 | Scaling is reversible, but does not move existing row 2 and introduces entries 3/2 and 5/2.
@feedback 13 | Addition gives first entry 2+1=3, not a unit pivot, and does not place existing row 2 on top.
@after \left[\begin{array}{cccc|c}1&0&1&0&1\\2&3&5&6&-1\\1&1&2&2&0\end{array}\right]
@wrong Satisfy all the specified method conditions.
@why Swapping complete rows puts [1,0,1,0|1] first and keeps integer entries. Repeating the swap reverses it.
@step 20 | Subtract row 1 from row 3 across all five columns.
@choice 22 | \left[\begin{array}{cccc|c}1&0&1&0&1\\2&3&5&6&-1\\0&1&1&2&0\end{array}\right]
@choice 21 | \left[\begin{array}{cccc|c}1&0&1&0&1\\2&3&5&6&-1\\0&1&1&2&-1\end{array}\right]
@choice 23 | \left[\begin{array}{cccc|c}1&0&1&0&1\\2&3&5&6&-1\\2&1&3&2&1\end{array}\right]
@answer 21
@feedback 22 | The target must become 0-1=-1, not 0.
@feedback 23 | This adds row 1, giving [2,1,3,2|1], instead of subtracting.
@after \left[\begin{array}{cccc|c}1&0&1&0&1\\2&3&5&6&-1\\0&1&1&2&-1\end{array}\right]
@wrong Include the target in the subtraction.
@why Row 3 is [1-1,1-0,2-1,2-0|0-1]=[0,1,1,2|-1]. Adding row 1 reverses it.
@step 30 | Subtract twice row 1 from row 2.
@choice 32 | \left[\begin{array}{cccc|c}1&0&1&0&1\\0&3&3&6&-1\\0&1&1&2&-1\end{array}\right]
@choice 33 | \left[\begin{array}{cccc|c}1&0&1&0&1\\4&3&7&6&1\\0&1&1&2&-1\end{array}\right]
@choice 31 | \left[\begin{array}{cccc|c}1&0&1&0&1\\0&3&3&6&-3\\0&1&1&2&-1\end{array}\right]
@answer 31
@feedback 32 | The target changes to -1-2(1)=-3, not -1.
@feedback 33 | This adds twice row 1, giving [4,3,7,6|1], rather than subtracting.
@after \left[\begin{array}{cccc|c}1&0&1&0&1\\0&3&3&6&-3\\0&1&1&2&-1\end{array}\right]
@wrong Use the signed multiple throughout the row.
@why Row 2 is [2-2,3-0,5-2,6-0|-1-2]=[0,3,3,6|-3]. Adding twice row 1 reverses it.
@step 40 | Subtract three times row 3 from row 2.
@choice 41 | \left[\begin{array}{cccc|c}1&0&1&0&1\\0&0&0&0&0\\0&1&1&2&-1\end{array}\right]
@choice 42 | \left[\begin{array}{cccc|c}1&0&1&0&1\\0&0&0&0&-3\\0&1&1&2&-1\end{array}\right]
@choice 43 | \left[\begin{array}{cccc|c}1&0&1&0&1\\0&6&6&12&-6\\0&1&1&2&-1\end{array}\right]
@answer 41
@feedback 42 | The target also cancels: -3-3(-1)=0. Retaining -3 invents a contradiction.
@feedback 43 | This adds three times row 3, giving [0,6,6,12|-6].
@after \left[\begin{array}{cccc|c}1&0&1&0&1\\0&0&0&0&0\\0&1&1&2&-1\end{array}\right]
@wrong A redundant equation cancels on both sides.
@why Row 2 is [0-0,3-3,3-3,6-6|-3-(-3)]=[0,0,0,0|0]. Adding three times row 3 reverses it.
@step 50 | Swap complete rows 2 and 3 to put the zero row last.
@choice 52 | \left[\begin{array}{cccc|c}1&0&1&0&1\\0&1&1&2&0\\0&0&0&0&-1\end{array}\right]
@choice 51 | \left[\begin{array}{cccc|c}1&0&1&0&1\\0&1&1&2&-1\\0&0&0&0&0\end{array}\right]
@choice 53 | \left[\begin{array}{cccc|c}1&0&1&0&1\\0&0&0&0&0\\0&1&1&2&-1\end{array}\right]
@answer 51
@feedback 52 | The target -1 belongs to [0,1,1,2]; move it with that row.
@feedback 53 | This is unchanged, with the zero row still second.
@after \left[\begin{array}{cccc|c}1&0&1&0&1\\0&1&1&2&-1\\0&0&0&0&0\end{array}\right]
@wrong Swap targets with their coefficient rows.
@why The coefficient pivots are in columns 1 and 2; columns 3 and 4 are nonpivot columns. The same row swap is the inverse.
@step 60 | Choose B from the ORIGINAL pivot columns in increasing order as a basis of W=span(U).
@choice 62 | B=\begin{bmatrix}2&5\\1&1\\1&2\end{bmatrix}
@choice 63 | B=\begin{bmatrix}1&0\\0&1\\0&0\end{bmatrix}
@choice 61 | B=\begin{bmatrix}2&3\\1&0\\1&1\end{bmatrix}
@answer 61
@feedback 62 | Columns 1 and 3 are another valid basis of W because u3=u1+u2, but column 3 is not a pivot column. The requested original indices are 1 and 2.
@feedback 63 | These are reduced columns. For example (1,0,0) is outside W: coordinate 2 forces the u1 coefficient to zero, then coordinate 3 forces the u2 coefficient to zero.
@after B=\begin{bmatrix}2&3\\1&0\\1&1\end{bmatrix}
@wrong Retrieve pivot columns from the original A, not its reduction.
@why A zero combination of u1,u2 forces its first coefficient to zero from coordinate 2 and its second to zero from coordinate 3, proving independence. Also u3=u1+u2 and u4=2u2, proving spanning of W. Thus dim(W)=2; W is proper in R3.
@step 70 | Find the two coordinates c in B, not four coefficients for the redundant list.
@choice 71 | c=\begin{bmatrix}1\\-1\end{bmatrix}
@choice 72 | c=\begin{bmatrix}-1\\1\end{bmatrix}
@choice 73 | c=\begin{bmatrix}1\\1\end{bmatrix}
@answer 71
@feedback 72 | This gives -u1+u2=(1,-1,0), the negative of w.
@feedback 73 | This gives u1+u2=(5,1,2), not w.
@after c=\begin{bmatrix}1\\-1\end{bmatrix}
@wrong Set the nonbasis coefficients to zero and retain the basis order.
@why With coefficients of u3,u4 zero, the reduced equations give c1=1,c2=-1. These basis coordinates are unique, although the four-generator representation is not.
@step 80 | Reconstruct Bc from the original selected columns.
@choice 82 | Bc=\begin{bmatrix}1\\-1\\0\end{bmatrix}
@choice 81 | Bc=\begin{bmatrix}-1\\1\\0\end{bmatrix}
@choice 83 | Bc=\begin{bmatrix}5\\1\\2\end{bmatrix}
@answer 81
@feedback 82 | This uses -u1+u2, reversing both coefficient signs.
@feedback 83 | This uses u1+u2; the second coefficient is -1, not 1.
@after Bc=\begin{bmatrix}-1\\1\\0\end{bmatrix}
@wrong Use the original basis, not reduced columns.
@why Bc=1(2,1,1)-1(3,0,1)=(2-3,1-0,1-1)=(-1,1,0)=w. Both parts of the basis proof apply to W, not to R3.
@end

@question prod04_linear_span_basis_q11 | Vector investigation 11
@template choices.v1
@version 1
@goal Construct an ordered basis for the specified plane W using parameter order x=s,z=t, then find and verify the coordinates of w.
@given W=\{(x,y,z)^T:x+y=0\},\quad w=\begin{bmatrix}2\\-2\\3\end{bmatrix}
@domain Scalars are real. The displayed matrices list the original vectors as COLUMNS in the given order. Coefficients and coordinates use that same order. Row operations include the entire augmented row.
@read prod04_linear_span_basis_r
@step 10 | Choose a parametrization method by solving the plane equation for y, with x and z free.
@choice 12 | y=x
@choice 11 | y=-x
@choice 13 | y=0
@answer 11
@feedback 12 | Substitution gives x+y=2x, not zero for every free x.
@feedback 13 | Substitution gives x+y=x, forcing x=0 and losing most of the plane.
@after y=-x
@wrong Preserve all solutions of the plane equation.
@why Subtract x to get y=-x; z is unrestricted. This equivalence describes every vector in the plane.
@step 20 | Set x=s,z=t. Put the coefficient of s first and of t second as columns of B.
@choice 22 | B=\begin{bmatrix}1&0\\1&0\\0&1\end{bmatrix}
@choice 23 | B=\begin{bmatrix}1&0\\-1&1\\0&0\end{bmatrix}
@choice 21 | B=\begin{bmatrix}1&0\\-1&0\\0&1\end{bmatrix}
@answer 21
@feedback 22 | The first vector has x+y=2 and is outside the plane.
@feedback 23 | The second vector (0,1,0) violates x+y=0 and cannot supply the free z coordinate.
@after B=\begin{bmatrix}1&0\\-1&0\\0&1\end{bmatrix}
@wrong Read the coefficient columns of (s,-s,t).
@why Every plane vector is (s,-s,t)=s(1,-1,0)+t(0,0,1). Both columns lie in W, and these combinations cover every member, proving spanning.
@step 30 | To check independence form [B|0] and add row 1 to row 2.
@choice 31 | \left[\begin{array}{cc|c}1&0&0\\0&0&0\\0&1&0\end{array}\right]
@choice 32 | \left[\begin{array}{cc|c}1&0&0\\-2&0&0\\0&1&0\end{array}\right]
@choice 33 | \left[\begin{array}{cc|c}1&0&0\\0&0&1\\0&1&0\end{array}\right]
@answer 31
@feedback 32 | Subtraction gives -1-1=-2; addition gives -1+1=0.
@feedback 33 | The homogeneous target remains 0+0=0. A 1 here invents inconsistency.
@after \left[\begin{array}{cc|c}1&0&0\\0&0&0\\0&1&0\end{array}\right]
@wrong Independence uses the zero target, not w.
@why Row 2 becomes [-1+1,0+0|0+0]=[0,0|0]. Subtracting row 1 reverses it.
@step 40 | Swap complete rows 2 and 3 to identify the homogeneous solutions.
@choice 42 | \left[\begin{array}{cc|c}1&0&0\\0&0&0\\0&1&0\end{array}\right]
@choice 41 | \left[\begin{array}{cc|c}1&0&0\\0&1&0\\0&0&0\end{array}\right]
@choice 43 | \left[\begin{array}{cc|c}0&0&0\\0&1&0\\1&0&0\end{array}\right]
@answer 41
@feedback 42 | This is unchanged; the zero row remains second.
@feedback 43 | This changes the first row too; the requested swap leaves [1,0|0] first.
@after \left[\begin{array}{cc|c}1&0&0\\0&1&0\\0&0&0\end{array}\right]
@wrong Keep the unselected row unchanged.
@why Both homogeneous coefficients must be zero, so B is independent. Together with the general parametrization this proves B is a basis of W. Its two vectors imply dim(W)=2, not 3.
@step 50 | Find coordinates c of w in the ordered parameter basis B.
@choice 52 | c=\begin{bmatrix}3\\2\end{bmatrix}
@choice 53 | c=\begin{bmatrix}2\\-2\end{bmatrix}
@choice 51 | c=\begin{bmatrix}2\\3\end{bmatrix}
@answer 51
@feedback 52 | Reordering s and t gives (3,-3,2), not w.
@feedback 53 | The second coefficient is free coordinate z=3, not dependent coordinate y=-2.
@after c=\begin{bmatrix}2\\3\end{bmatrix}
@wrong Parameter order is x=s first and z=t second.
@why The free coordinates of w give s=2,t=3. The remaining coordinate -s=-2 agrees with w, so w belongs to W. Basis independence ensures uniqueness.
@step 60 | Reconstruct Bc in the ORIGINAL coordinates.
@choice 61 | Bc=\begin{bmatrix}2\\-2\\3\end{bmatrix}
@choice 62 | Bc=\begin{bmatrix}3\\-3\\2\end{bmatrix}
@choice 63 | Bc=\begin{bmatrix}2\\2\\3\end{bmatrix}
@answer 61
@feedback 62 | This uses coefficients (3,2), in the wrong order.
@feedback 63 | The first basis vector has second entry -1, giving 2(-1)=-2, not 2.
@after Bc=\begin{bmatrix}2\\-2\\3\end{bmatrix}
@wrong Retain signs from the original basis vectors.
@why Bc=2(1,-1,0)+3(0,0,1)=(2,-2,3)=w. Also 2+(-2)=0 verifies the original plane equation. This basis spans W, not all R3.
@end

@question prod04_linear_span_basis_q12 | Vector investigation 12
@template choices.v1
@version 1
@goal Verify the two ordered bases and find the coordinates of w in each, checking both original reconstructions.
@given A_1=\begin{bmatrix}1&1\\-1&1\end{bmatrix},\quad A_2=\begin{bmatrix}1&1\\2&3\end{bmatrix},\quad w=\begin{bmatrix}3\\-1\end{bmatrix}
@domain Scalars are real. The displayed matrices list the original vectors as COLUMNS in the given order. Coefficients and coordinates use that same order. Row operations include the entire augmented row.
@read prod04_linear_span_basis_r
@step 10 | Start with [A1|w]. Add row 1 to row 2.
@choice 12 | \left[\begin{array}{cc|c}1&1&3\\0&2&-1\end{array}\right]
@choice 13 | \left[\begin{array}{cc|c}1&1&3\\-2&0&-4\end{array}\right]
@choice 11 | \left[\begin{array}{cc|c}1&1&3\\0&2&2\end{array}\right]
@answer 11
@feedback 12 | The target changes to -1+3=2; it does not remain -1.
@feedback 13 | Subtraction gives [-2,0|-4], not the requested addition.
@after \left[\begin{array}{cc|c}1&1&3\\0&2&2\end{array}\right]
@wrong Use the first basis and original target.
@why Row 2 becomes [-1+1,1+1|-1+3]=[0,2|2]. Subtracting row 1 reverses it.
@step 20 | Divide the complete second row by 2.
@choice 21 | \left[\begin{array}{cc|c}1&1&3\\0&1&1\end{array}\right]
@choice 22 | \left[\begin{array}{cc|c}1&1&3\\0&1&2\end{array}\right]
@choice 23 | \left[\begin{array}{cc|c}1&1&3\\0&4&4\end{array}\right]
@answer 21
@feedback 22 | The target is 2/2=1, not 2.
@feedback 23 | This multiplies instead of dividing the row by 2.
@after \left[\begin{array}{cc|c}1&1&3\\0&1&1\end{array}\right]
@wrong Divide every entry.
@why Row 2 becomes [0/2,2/2|2/2]=[0,1|1]. Multiplication by 2 reverses it.
@step 30 | Subtract row 2 from row 1.
@choice 32 | \left[\begin{array}{cc|c}1&0&3\\0&1&1\end{array}\right]
@choice 31 | \left[\begin{array}{cc|c}1&0&2\\0&1&1\end{array}\right]
@choice 33 | \left[\begin{array}{cc|c}1&2&4\\0&1&1\end{array}\right]
@answer 31
@feedback 32 | The target becomes 3-1=2, not 3.
@feedback 33 | Addition gives [1,2|4]; subtraction clears the coefficient.
@after \left[\begin{array}{cc|c}1&0&2\\0&1&1\end{array}\right]
@wrong Use subtraction in every entry.
@why Row 1 is [1-0,1-1|3-1]=[1,0|2]. Adding row 2 reverses it. The identity coefficient matrix proves independence and solvability for every R2 target, so the first pair is a basis.
@step 40 | Read c(1), the coordinates in the FIRST ordered basis.
@choice 42 | c^{(1)}=\begin{bmatrix}1\\2\end{bmatrix}
@choice 43 | c^{(1)}=\begin{bmatrix}3\\-1\end{bmatrix}
@choice 41 | c^{(1)}=\begin{bmatrix}2\\1\end{bmatrix}
@answer 41
@feedback 42 | Reversed coefficients give (3,1), not (3,-1).
@feedback 43 | Copying w as coefficients gives (2,-4), not w.
@after c^{(1)}=\begin{bmatrix}2\\1\end{bmatrix}
@wrong Retain the first basis order.
@why The reduced equations give c1=2,c2=1. The first basis is independent, so these coefficients are unique.
@step 50 | Restart from ORIGINAL [A2|w]. Subtract twice row 1 from row 2.
@choice 51 | \left[\begin{array}{cc|c}1&1&3\\0&1&-7\end{array}\right]
@choice 52 | \left[\begin{array}{cc|c}1&1&3\\0&1&-1\end{array}\right]
@choice 53 | \left[\begin{array}{cc|c}1&1&3\\4&5&5\end{array}\right]
@answer 51
@feedback 52 | The target becomes -1-2(3)=-7, not -1.
@feedback 53 | Addition gives [4,5|5], not the requested subtraction.
@after \left[\begin{array}{cc|c}1&1&3\\0&1&-7\end{array}\right]
@wrong A new basis requires a new system with the same w.
@why Original row 2 [2,3|-1] minus twice [1,1|3] is [2-2,3-2|-1-6]=[0,1|-7]. Adding twice row 1 reverses it.
@step 60 | Subtract row 2 from row 1 in the SECOND system.
@choice 62 | \left[\begin{array}{cc|c}1&1&3\\0&1&-7\end{array}\right]
@choice 61 | \left[\begin{array}{cc|c}1&0&10\\0&1&-7\end{array}\right]
@choice 63 | \left[\begin{array}{cc|c}1&2&-4\\0&1&-7\end{array}\right]
@answer 61
@feedback 62 | This is unchanged; the second coefficient in row 1 remains 1.
@feedback 63 | Addition gives [1,2|-4]; subtraction is required to clear the coefficient.
@after \left[\begin{array}{cc|c}1&0&10\\0&1&-7\end{array}\right]
@wrong Subtracting a negative target increases the first target.
@why Row 1 is [1-0,1-1|3-(-7)]=[1,0|10]. Adding row 2 reverses it. Two pivots prove independence and spanning of R2 for this second pair.
@step 70 | Read c(2) in the SECOND ordered basis.
@choice 72 | c^{(2)}=\begin{bmatrix}-7\\10\end{bmatrix}
@choice 73 | c^{(2)}=\begin{bmatrix}2\\1\end{bmatrix}
@choice 71 | c^{(2)}=\begin{bmatrix}10\\-7\end{bmatrix}
@answer 71
@feedback 72 | Reversed coefficients give second coordinate -14+30=16, not -1.
@feedback 73 | These first-basis coordinates give (3,7) in the second basis, not w.
@after c^{(2)}=\begin{bmatrix}10\\-7\end{bmatrix}
@wrong Coordinates depend on the ordered basis.
@why The second equations give c1=10,c2=-7. This unique representation differs from (2,1) in the first basis.
@step 80 | Evaluate BOTH reconstructions using their ORIGINAL basis columns.
@choice 81 | A_1c^{(1)}=A_2c^{(2)}=\begin{bmatrix}3\\-1\end{bmatrix}
@choice 82 | A_1c^{(1)}=A_2c^{(2)}=\begin{bmatrix}3\\1\end{bmatrix}
@choice 83 | A_1c^{(1)}=A_2c^{(2)}=\begin{bmatrix}3\\7\end{bmatrix}
@answer 81
@feedback 82 | In the first basis the second coordinate is 2(-1)+1(1)=-1, not 1.
@feedback 83 | This incorrectly reuses (2,1) in the second basis: 2(2)+1(3)=7.
@after A_1c^{(1)}=A_2c^{(2)}=\begin{bmatrix}3\\-1\end{bmatrix}
@wrong Each basis must use its own coordinate column.
@why First, 2(1,-1)+1(1,1)=(3,-1). Second, 10(1,2)-7(1,3)=(10-7,20-21)=(3,-1). Both equal original w, with uniqueness supplied by each basis proof.
@end
