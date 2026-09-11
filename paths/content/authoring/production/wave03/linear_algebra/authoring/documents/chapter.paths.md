@paths 1
@subject linear_algebra | Linear Algebra
@chapter topic_0092 | Matrix Algebra

@lesson prod03_linear_matrix_inverse_r | Constructing and using an inverse
@template lesson.v2
@block introduction | start | 3.1 | Undoing a matrix
@prose An inverse undoes a square matrix on every vector, not just on one test vector. We will construct a candidate, verify it in both multiplication orders, and use it to solve an equation. Some matrices cannot be undone: a nonzero vector sent to zero gives a precise obstruction.
@endblock
@block definition | terms | 3.2 | Entries, products and augmentation
@prose Read a matrix by rows. The entry with indices i,j lies in row i and column j. A column vector has two ordered coordinates. In the display below, a and b occupy the first row, c and d the second; the identity I leaves every vector unchanged. To form entry i,j of a product, multiply row i of the first factor by column j of the second and add. Do not multiply corresponding entries or exchange the factors.
@display
A=\begin{bmatrix}a&b\\c&d\end{bmatrix},\quad I=\begin{bmatrix}1&0\\0&1\end{bmatrix},\quad
\begin{bmatrix}a&b\\c&d\end{bmatrix}\begin{bmatrix}p&q\\r&s\end{bmatrix}
=\begin{bmatrix}ap+br&aq+bs\\cp+dr&cq+ds\end{bmatrix}
@prose An inverse B satisfies both AB=I and BA=I. Augmenting A by I means adjoining both columns of I in their original order. The vertical bar separates blocks, not rows. Every row operation must include all four entries.
@display
[A\mid I]=\left[\begin{array}{cc|cc}a&b&1&0\\c&d&0&1\end{array}\right]
@endblock
@block proposition | rule | 3.3 | Two constructions
@prose The determinant of a two by two matrix is the first diagonal product minus the other diagonal product. The adjugate J swaps the diagonal entries and negates the off-diagonal entries. When the determinant is nonzero, divide every entry of J by it. For example, subtracting a negative product adds its magnitude; keep that sign before dividing.
@display
\Delta=\det(A)=ad-bc,\quad J=\operatorname{adj}(A)=\begin{bmatrix}d&-b\\-c&a\end{bmatrix},\quad
\Delta\ne0\ \Longrightarrow\ B=\frac{1}{\Delta}J
@prose Alternatively, reduce the left block of [A|I] to I. A row swap exchanges entire rows and is undone by swapping again. Division of an entire row by a nonzero number is undone by multiplication by that number. Replacing one row by itself plus k times the other row is undone by subtracting k times that unchanged other row. Use the old row entries for the whole replacement. These operations preserve both column systems represented by the augmentation.
@display
[A\mid I]\longrightarrow[I\mid B]
@prose A pivot is the leading nonzero entry in a row during elimination. Normalize it to one and clear other entries in its column. If the left block cannot become I, the right block is not an inverse.
@endblock
@block proposition | condition | 3.4 | Conditions and proof
@prose All entries here are real and all calculations are exact. The adjugate formula requires a nonzero determinant; division by zero is undefined. In Ax=b, x is an unknown column vector and b is a given column vector, not the scalar entry called b in the general two by two formula. Once B is verified, multiplying Ax=b on the left by B gives x=Bb. This proves uniqueness as well as a way to find the solution, and substitution into original A verifies existence.
@help proof
@prose Multiplication with symbolic entries gives A times J and J times A equal to the determinant times I. Division is therefore justified precisely when the determinant is nonzero. The null space of A is the set of column vectors v satisfying Av=0. Suppose the determinant is zero and the first row [a,b] is nonzero. The column [-b,a] is then nonzero, and its product with A is [-ab+ba,-cb+da]=[0,ad-bc]=[0,0]. If the first row is zero but A is nonzero, the second row [c,d] is nonzero; the column [-d,c] is nonzero and gives product [0,-cd+dc]=[0,0]. Thus either case supplies a nonzero null vector. If an inverse C existed, multiplying Av=0 by C would force this nonzero v to be zero, a contradiction.
@display
AJ=JA=(ad-bc)I,\qquad
Av=0,\ v\ne0,\ CA=I\ \Longrightarrow\ v=(CA)v=C(Av)=C0=0
@endblock
@block example | worked | 3.5 | A separate construction
@prose Construct the inverse of the matrix below by row reduction, then check both multiplication orders.
@display
H=\begin{bmatrix}1&-1\\0&2\end{bmatrix}
@help hint
@prose Begin with [H|I]. Normalize the second pivot using a nonzero divisor, then clear the entry above it. Keep both identity columns in each operation.
@help answer
@display
H^{-1}=\begin{bmatrix}1&\frac{1}{2}\\0&\frac{1}{2}\end{bmatrix}
@help solution
@prose Divide row 2 by 2, giving [0,1|0,1/2]. Add that row to row 1: 1+0=1, -1+1=0, 1+0=1 and 0+1/2=1/2. These operations are reversed by subtraction and multiplication by 2.
@display
\left[\begin{array}{cc|cc}1&-1&1&0\\0&2&0&1\end{array}\right]
\longrightarrow\left[\begin{array}{cc|cc}1&-1&1&0\\0&1&0&\frac{1}{2}\end{array}\right]
\longrightarrow\left[\begin{array}{cc|cc}1&0&1&\frac{1}{2}\\0&1&0&\frac{1}{2}\end{array}\right]
@prose In H times the candidate, the first row is [1,1/2-1/2]=[1,0] and the second is [0,2(1/2)]=[0,1]. In the reverse order the first row is [1,-1+2(1/2)]=[1,0] and the second is [0,2(1/2)]=[0,1]. Both products are exactly I.
@endblock
@block proposition | errors | 3.6 | Preserve the whole object
@prose Leaving either identity column unchanged while changing the left block breaks the construction. The adjugate [d,-b;-c,a] swaps diagonal entries and negates off-diagonal entries in place. A transpose instead exchanges rows with columns, so it moves the off-diagonal entries to each other's positions. A reciprocal scales every entry, not just one row. Matrix multiplication uses row times column. Finally, the zero vector always satisfies Av=0, so it cannot prove singularity; a nonzero witness is essential.
@endblock
@block exercise | practice | 3.7 | From structure to application
@prose Begin with diagonal, triangular and off-diagonal matrices. Then construct inverses with signed and fractional entries. Finish by choosing a method, solving a vector equation and proving the exceptional cases. In every case keep the original matrix available for the final check.
@endblock
@block summary | summary | 3.8 | Construction plus verification
@prose Respect row and column order. Use only complete reversible row operations, or form the adjugate and divide by a verified nonzero determinant. Check both products against the original matrix. For a vector equation, substitute the computed vector into the original equation. If a nonzero vector is sent to zero, explain the contradiction that an inverse would create.
@endblock
@practice prod03_linear_matrix_inverse_q01
@practice prod03_linear_matrix_inverse_q02
@practice prod03_linear_matrix_inverse_q03
@practice prod03_linear_matrix_inverse_q04
@practice prod03_linear_matrix_inverse_q05
@practice prod03_linear_matrix_inverse_q06
@practice prod03_linear_matrix_inverse_q07
@practice prod03_linear_matrix_inverse_q08
@practice prod03_linear_matrix_inverse_q09
@practice prod03_linear_matrix_inverse_q10
@practice prod03_linear_matrix_inverse_q11
@practice prod03_linear_matrix_inverse_q12
@end

@question prod03_linear_matrix_inverse_q01 | Matrix investigation 1
@template choices.v1
@version 1
@goal Construct the inverse of A and verify both AB=I and BA=I against the original matrix.
@given A=\begin{bmatrix}2&0\\0&-3\end{bmatrix}
@domain All entries and vectors are real. Rows precede columns. I is the 2 by 2 identity, B is a candidate inverse, and R1 and R2 denote complete rows.
@read prod03_linear_matrix_inverse_r
@step 10 | Begin with [A|I]. Divide the complete first row by 2. Which augmented matrix results?
@choice 11 | \left[\begin{array}{cc|cc}1&0&\frac{1}{2}&0\\0&-3&0&1\end{array}\right]
@choice 12 | \left[\begin{array}{cc|cc}1&0&1&0\\0&-3&0&1\end{array}\right]
@choice 13 | \left[\begin{array}{cc|cc}4&0&2&0\\0&-3&0&1\end{array}\right]
@answer 11
@feedback 12 | The first identity entry must also be divided: 1/2, not 1.
@feedback 13 | Multiplying by 2 gives [4,0|2,0]. That is reversible but is not the requested division or unit pivot.
@after \left[\begin{array}{cc|cc}1&0&\frac{1}{2}&0\\0&-3&0&1\end{array}\right]
@wrong Divide all four entries of the first row by the same nonzero number.
@why The first row becomes [2/2,0/2|1/2,0/2]=[1,0|1/2,0]. Row 2 stays fixed. Multiplication by 2 reverses this move.
@step 20 | Divide the complete second row by -3. Which augmented matrix results?
@choice 22 | \left[\begin{array}{cc|cc}1&0&\frac{1}{2}&0\\0&1&0&1\end{array}\right]
@choice 21 | \left[\begin{array}{cc|cc}1&0&\frac{1}{2}&0\\0&1&0&-\frac{1}{3}\end{array}\right]
@choice 23 | \left[\begin{array}{cc|cc}1&0&\frac{1}{2}&0\\0&-1&0&\frac{1}{3}\end{array}\right]
@answer 21
@feedback 22 | The last entry also changes: 1/(-3)=-1/3.
@feedback 23 | This divides by positive 3. It is reversible but leaves the second pivot -1, not the requested 1.
@after \left[\begin{array}{cc|cc}1&0&\frac{1}{2}&0\\0&1&0&-\frac{1}{3}\end{array}\right]
@wrong Keep the negative divisor on the identity side too.
@why Row 2 becomes [0/(-3),(-3)/(-3)|0/(-3),1/(-3)]=[0,1|0,-1/3]. The divisor is nonzero; multiplying by -3 reverses it.
@step 30 | The left block is I. Read the right block as B with every entry evaluated.
@choice 32 | B=\begin{bmatrix}2&0\\0&-3\end{bmatrix}
@choice 33 | B=\begin{bmatrix}\frac{1}{2}&0\\0&\frac{1}{3}\end{bmatrix}
@choice 31 | B=\begin{bmatrix}\frac{1}{2}&0\\0&-\frac{1}{3}\end{bmatrix}
@answer 31
@feedback 32 | This copies original A. The transformed right block has 1/2 and -1/3, not 2 and -3.
@feedback 33 | The last entry is negative because 1 was divided by -3.
@after B=\begin{bmatrix}\frac{1}{2}&0\\0&-\frac{1}{3}\end{bmatrix}
@wrong Read the transformed identity columns, retaining their order.
@why The right block is [1/2,0;0,-1/3]. We now verify this candidate directly against original A rather than relying on its appearance.
@step 40 | Multiply original A by B using row times column. Which evaluated product results?
@choice 41 | AB=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@choice 42 | AB=\begin{bmatrix}-1&0\\0&1\end{bmatrix}
@choice 43 | AB=\begin{bmatrix}1&1\\0&1\end{bmatrix}
@answer 41
@feedback 42 | The first diagonal entry is 2(1/2)+0(0)=1, not -1.
@feedback 43 | The upper-right entry is 2(0)+0(-1/3)=0, not 1.
@after AB=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@wrong Use a row of A and a column of B for each entry.
@why The entries are 2(1/2)+0(0)=1, 2(0)+0(-1/3)=0, 0(1/2)+(-3)(0)=0 and 0(0)+(-3)(-1/3)=1.
@step 50 | Now multiply B by original A. Which evaluated product completes the inverse check?
@choice 52 | BA=\begin{bmatrix}1&0\\1&1\end{bmatrix}
@choice 51 | BA=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@choice 53 | BA=\begin{bmatrix}0&1\\1&0\end{bmatrix}
@answer 51
@feedback 52 | The lower-left entry is 0(2)+(-1/3)(0)=0.
@feedback 53 | The first row is [(1/2)2+0(0),(1/2)0+0(-3)]=[1,0], not [0,1].
@after BA=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@wrong Recompute in the reverse factor order.
@why The first row is [1,0] and the second is [0,(-1/3)(-3)]=[0,1]. Both products are I, so the constructed B is the inverse of original A.
@end

@question prod03_linear_matrix_inverse_q02 | Matrix investigation 2
@template choices.v1
@version 1
@goal Construct the inverse of A and verify both AB=I and BA=I against the original matrix.
@given A=\begin{bmatrix}1&2\\0&1\end{bmatrix}
@domain All entries and vectors are real. Rows precede columns. I is the 2 by 2 identity, B is a candidate inverse, and R1 and R2 denote complete rows.
@read prod03_linear_matrix_inverse_r
@step 10 | Begin with [A|I]. Replace row 1 by row 1 minus twice row 2. Which complete augmented matrix results?
@choice 12 | \left[\begin{array}{cc|cc}1&0&1&0\\0&1&0&1\end{array}\right]
@choice 11 | \left[\begin{array}{cc|cc}1&0&1&-2\\0&1&0&1\end{array}\right]
@choice 13 | \left[\begin{array}{cc|cc}1&4&1&2\\0&1&0&1\end{array}\right]
@answer 11
@feedback 12 | The last entry of row 1 must become 0-2(1)=-2. It cannot be left at zero.
@feedback 13 | Adding twice row 2 gives [1,4|1,2]. It is reversible but does not perform the requested subtraction.
@after \left[\begin{array}{cc|cc}1&0&1&-2\\0&1&0&1\end{array}\right]
@wrong Apply the subtraction to both identity columns.
@why Row 1 becomes [1-2(0),2-2(1)|1-2(0),0-2(1)]=[1,0|1,-2]. Row 2 is unchanged; adding twice row 2 reverses the move.
@step 20 | Read the transformed right block as B with evaluated entries.
@choice 22 | B=\begin{bmatrix}1&2\\0&1\end{bmatrix}
@choice 23 | B=\begin{bmatrix}1&0\\-2&1\end{bmatrix}
@choice 21 | B=\begin{bmatrix}1&-2\\0&1\end{bmatrix}
@answer 21
@feedback 22 | The subtraction produced -2 in the upper-right position, not +2.
@feedback 23 | This transposes the right block. The -2 belongs in row 1, column 2.
@after B=\begin{bmatrix}1&-2\\0&1\end{bmatrix}
@wrong Preserve row and column order when reading the right block.
@why The left block is I and the right block is [1,-2;0,1]. Direct multiplication will check that it undoes original A.
@step 30 | Compute original A times B. Which evaluated product results?
@choice 31 | AB=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@choice 32 | AB=\begin{bmatrix}-1&0\\0&1\end{bmatrix}
@choice 33 | AB=\begin{bmatrix}1&1\\0&1\end{bmatrix}
@answer 31
@feedback 32 | The upper-left entry is 1(1)+2(0)=1.
@feedback 33 | The upper-right entry is 1(-2)+2(1)=0, not 1.
@after AB=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@wrong Multiply rows by columns and add both terms.
@why The four entries are 1+0=1, -2+2=0, 0+0=0 and 0+1=1. Thus AB=I.
@step 40 | Compute B times original A to finish the two-sided check.
@choice 42 | BA=\begin{bmatrix}1&0\\1&1\end{bmatrix}
@choice 41 | BA=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@choice 43 | BA=\begin{bmatrix}0&1\\1&0\end{bmatrix}
@answer 41
@feedback 42 | The lower-left entry is 0(1)+1(0)=0.
@feedback 43 | Row 1 is [1(1)+(-2)0,1(2)+(-2)1]=[1,0], not [0,1].
@after BA=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@wrong Keep B on the left for this second check.
@why The first row is [1,2-2]=[1,0]; the second is [0,1]. The two exact identity products establish that B is the inverse.
@end

@question prod03_linear_matrix_inverse_q03 | Matrix investigation 3
@template choices.v1
@version 1
@goal Construct the inverse of A and verify both AB=I and BA=I against the original matrix.
@given A=\begin{bmatrix}1&0\\-2&3\end{bmatrix}
@domain All entries and vectors are real. Rows precede columns. I is the 2 by 2 identity, B is a candidate inverse, and R1 and R2 denote complete rows.
@read prod03_linear_matrix_inverse_r
@step 10 | Begin with [A|I]. Add twice row 1 to row 2. Which complete augmented matrix results?
@choice 12 | \left[\begin{array}{cc|cc}1&0&1&0\\0&3&0&1\end{array}\right]
@choice 13 | \left[\begin{array}{cc|cc}1&0&1&0\\-4&3&-2&1\end{array}\right]
@choice 11 | \left[\begin{array}{cc|cc}1&0&1&0\\0&3&2&1\end{array}\right]
@answer 11
@feedback 12 | Row 2's first identity entry must change from 0 to 0+2(1)=2.
@feedback 13 | Subtracting twice row 1 gives -2-2=-4. That reversible move does not clear the first column.
@after \left[\begin{array}{cc|cc}1&0&1&0\\0&3&2&1\end{array}\right]
@wrong Add the signed multiple across all four columns.
@why Row 2 becomes [-2+2(1),3+2(0)|0+2(1),1+2(0)]=[0,3|2,1]. Subtracting twice unchanged row 1 reverses the operation.
@step 20 | Divide the complete second row by 3. Which augmented matrix results?
@choice 21 | \left[\begin{array}{cc|cc}1&0&1&0\\0&1&\frac{2}{3}&\frac{1}{3}\end{array}\right]
@choice 22 | \left[\begin{array}{cc|cc}1&0&1&0\\0&1&2&1\end{array}\right]
@choice 23 | \left[\begin{array}{cc|cc}1&0&1&0\\0&-1&-\frac{2}{3}&-\frac{1}{3}\end{array}\right]
@answer 21
@feedback 22 | Both right-block entries must be divided too: 2/3 and 1/3.
@feedback 23 | This divides by -3. It preserves the column systems but misses the requested positive unit pivot.
@after \left[\begin{array}{cc|cc}1&0&1&0\\0&1&\frac{2}{3}&\frac{1}{3}\end{array}\right]
@wrong Scale the whole row, including both augmented entries.
@why The entries are 0/3=0, 3/3=1, 2/3 and 1/3. Since 3 is nonzero, multiplication by 3 restores the preceding row.
@step 30 | Read the right block as B with evaluated entries.
@choice 32 | B=\begin{bmatrix}1&0\\-\frac{2}{3}&\frac{1}{3}\end{bmatrix}
@choice 31 | B=\begin{bmatrix}1&0\\\frac{2}{3}&\frac{1}{3}\end{bmatrix}
@choice 33 | B=\begin{bmatrix}1&\frac{2}{3}\\0&\frac{1}{3}\end{bmatrix}
@answer 31
@feedback 32 | The lower-left entry came from positive 2 divided by positive 3, so it is +2/3.
@feedback 33 | This exchanges the off-diagonal positions. The 2/3 is in row 2, column 1.
@after B=\begin{bmatrix}1&0\\\frac{2}{3}&\frac{1}{3}\end{bmatrix}
@wrong Read entries by rows, without transposing them.
@why The candidate is [1,0;2/3,1/3]. Its lower-left entry will cancel the -2 in original A when multiplied in the proper order.
@step 40 | Compute original A times B. Which evaluated product results?
@choice 42 | AB=\begin{bmatrix}-1&0\\0&1\end{bmatrix}
@choice 43 | AB=\begin{bmatrix}1&1\\0&1\end{bmatrix}
@choice 41 | AB=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@answer 41
@feedback 42 | The upper-left entry is 1(1)+0(2/3)=1.
@feedback 43 | The upper-right entry is 1(0)+0(1/3)=0.
@after AB=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@wrong Include both terms of each dot product.
@why Row 1 is [1,0]. Row 2 is [-2(1)+3(2/3),-2(0)+3(1/3)]=[0,1]. Thus AB=I.
@step 50 | Compute B times original A. Which evaluated product completes the check?
@choice 51 | BA=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@choice 52 | BA=\begin{bmatrix}1&0\\1&1\end{bmatrix}
@choice 53 | BA=\begin{bmatrix}0&1\\1&0\end{bmatrix}
@answer 51
@feedback 52 | The lower-left entry is (2/3)1+(1/3)(-2)=0, not 1.
@feedback 53 | The first row is [1(1)+0(-2),1(0)+0(3)]=[1,0].
@after BA=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@wrong Recompute with B's rows and A's columns.
@why The first row is [1,0]. The second is [2/3-2/3,0+1]=[0,1]. Both original-matrix products are I, establishing the inverse.
@end

@question prod03_linear_matrix_inverse_q04 | Matrix investigation 4
@template choices.v1
@version 1
@goal Construct the inverse of A and verify both AB=I and BA=I against the original matrix.
@given A=\begin{bmatrix}0&2\\-1&0\end{bmatrix}
@domain All entries and vectors are real. Rows precede columns. I is the 2 by 2 identity, B is a candidate inverse, and R1 and R2 denote complete rows.
@read prod03_linear_matrix_inverse_r
@step 10 | Begin with [A|I]. Swap the two complete rows to move an existing nonzero entry into the first pivot position.
@choice 11 | \left[\begin{array}{cc|cc}-1&0&0&1\\0&2&1&0\end{array}\right]
@choice 12 | \left[\begin{array}{cc|cc}-1&0&1&0\\0&2&0&1\end{array}\right]
@choice 13 | \left[\begin{array}{cc|cc}0&2&1&0\\-1&0&0&1\end{array}\right]
@answer 11
@feedback 12 | This swaps only the left block. The identity entries must travel with their whole rows: [0,1] moves to row 1.
@feedback 13 | This is the unchanged augmentation. Its first pivot position remains zero, so it misses the requested swap.
@after \left[\begin{array}{cc|cc}-1&0&0&1\\0&2&1&0\end{array}\right]
@wrong Swap all four entries, not just the original matrix columns.
@why The original second row [-1,0|0,1] becomes first and [0,2|1,0] becomes second. Swapping again restores the original augmentation.
@step 20 | Divide the whole first row by -1. Which augmented matrix results?
@choice 22 | \left[\begin{array}{cc|cc}1&0&0&1\\0&2&1&0\end{array}\right]
@choice 21 | \left[\begin{array}{cc|cc}1&0&0&-1\\0&2&1&0\end{array}\right]
@choice 23 | \left[\begin{array}{cc|cc}-1&0&0&1\\0&2&1&0\end{array}\right]
@answer 21
@feedback 22 | Dividing the last entry by -1 gives -1, not +1.
@feedback 23 | Leaving the row unchanged does not normalize its -1 pivot.
@after \left[\begin{array}{cc|cc}1&0&0&-1\\0&2&1&0\end{array}\right]
@wrong Apply the negative divisor to the full first row.
@why The four entries become [1,0|0,-1]. Division by -1 is reversible and multiplication by -1 restores the previous row.
@step 30 | Divide the whole second row by 2. Which augmented matrix results?
@choice 32 | \left[\begin{array}{cc|cc}1&0&0&-1\\0&1&1&0\end{array}\right]
@choice 33 | \left[\begin{array}{cc|cc}1&0&0&-1\\0&4&2&0\end{array}\right]
@choice 31 | \left[\begin{array}{cc|cc}1&0&0&-1\\0&1&\frac{1}{2}&0\end{array}\right]
@answer 31
@feedback 32 | The first right-block entry is 1/2 after division, not 1.
@feedback 33 | This multiplies by 2 rather than dividing. It is reversible but gives a pivot 4 instead of 1.
@after \left[\begin{array}{cc|cc}1&0&0&-1\\0&1&\frac{1}{2}&0\end{array}\right]
@wrong Divide all four entries by nonzero 2.
@why The row becomes [0/2,2/2|1/2,0/2]=[0,1|1/2,0]. The left block is now I.
@step 40 | Read B from the right block with evaluated entries.
@choice 41 | B=\begin{bmatrix}0&-1\\\frac{1}{2}&0\end{bmatrix}
@choice 42 | B=\begin{bmatrix}0&\frac{1}{2}\\-1&0\end{bmatrix}
@choice 43 | B=\begin{bmatrix}0&1\\\frac{1}{2}&0\end{bmatrix}
@answer 41
@feedback 42 | This transposes the right block. The -1 is in row 1, column 2, and 1/2 in row 2, column 1.
@feedback 43 | The upper-right entry is -1 from division by -1; do not discard its sign.
@after B=\begin{bmatrix}0&-1\\\frac{1}{2}&0\end{bmatrix}
@wrong Preserve the row order after the swap and later scalings.
@why The candidate is [0,-1;1/2,0]. An inverse need not have nonzero diagonal entries; multiplication, not diagonal appearance, decides.
@step 50 | Compute original A times B. Which evaluated product results?
@choice 52 | AB=\begin{bmatrix}-1&0\\0&1\end{bmatrix}
@choice 51 | AB=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@choice 53 | AB=\begin{bmatrix}1&1\\0&1\end{bmatrix}
@answer 51
@feedback 52 | The upper-left entry is 0(0)+2(1/2)=1.
@feedback 53 | The upper-right entry is 0(-1)+2(0)=0.
@after AB=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@wrong Each diagonal product uses off-diagonal entries of these factors.
@why The four dot products give 0+1=1, 0+0=0, 0+0=0 and (-1)(-1)+0=1.
@step 60 | Compute B times original A. Which evaluated product completes the check?
@choice 62 | BA=\begin{bmatrix}1&0\\1&1\end{bmatrix}
@choice 63 | BA=\begin{bmatrix}0&1\\1&0\end{bmatrix}
@choice 61 | BA=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@answer 61
@feedback 62 | The lower-left entry is (1/2)0+0(-1)=0.
@feedback 63 | The first row is [0(0)+(-1)(-1),0(2)+(-1)0]=[1,0], not [0,1].
@after BA=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@wrong Reverse the factor order, not the entry order within a row.
@why The second row is [(1/2)0+0(-1),(1/2)2+0(0)]=[0,1], and the first is [1,0]. Both products are I, so B is the inverse.
@end

@question prod03_linear_matrix_inverse_q05 | Matrix investigation 5
@template choices.v1
@version 1
@goal Construct the inverse of A and verify both AB=I and BA=I against the original matrix.
@given A=\begin{bmatrix}2&1\\1&3\end{bmatrix}
@domain All entries and vectors are real. Rows precede columns. I is the 2 by 2 identity, B is a candidate inverse, and R1 and R2 denote complete rows.
@read prod03_linear_matrix_inverse_r
@step 10 | Compute the determinant ad-bc before attempting division. Which value results?
@choice 12 | \det(A)=7
@choice 11 | \det(A)=5
@choice 13 | \det(A)=-5
@answer 11
@feedback 12 | This adds the diagonal products 6+1. The determinant subtracts: 6-1=5.
@feedback 13 | This reverses their order: 1-6=-5. Use ad-bc=6-1.
@after \det(A)=5
@wrong Subtract the off-diagonal product from the diagonal product.
@why With a=2, b=1, c=1 and d=3, the determinant is 2(3)-1(1)=6-1=5. It is nonzero, so the inverse formula is available.
@step 20 | Form the adjugate J: swap diagonal entries and negate off-diagonal entries. Which evaluated matrix results?
@choice 22 | J=\begin{bmatrix}3&1\\1&2\end{bmatrix}
@choice 23 | J=\begin{bmatrix}2&-1\\-1&3\end{bmatrix}
@choice 21 | J=\begin{bmatrix}3&-1\\-1&2\end{bmatrix}
@answer 21
@feedback 22 | Both off-diagonal entries must be negated: -1 and -1, not +1 and +1.
@feedback 23 | This keeps the original diagonal order. The top-left becomes d=3 and bottom-right a=2.
@after J=\begin{bmatrix}3&-1\\-1&2\end{bmatrix}
@wrong Use [d,-b;-c,a], keeping the off-diagonal positions.
@why Swapping 2 and 3 gives diagonal entries 3 and 2. Negating the original off-diagonal 1 and 1 gives -1 in each off-diagonal position.
@step 30 | Divide every entry of J by the nonzero determinant. Give B with the scalar multiplication fully evaluated inside the matrix.
@choice 31 | B=\begin{bmatrix}\frac{3}{5}&-\frac{1}{5}\\-\frac{1}{5}&\frac{2}{5}\end{bmatrix}
@choice 32 | B=\begin{bmatrix}3&-1\\-1&2\end{bmatrix}
@choice 33 | B=\begin{bmatrix}-\frac{3}{5}&\frac{1}{5}\\\frac{1}{5}&-\frac{2}{5}\end{bmatrix}
@answer 31
@feedback 32 | This is J before division. Each entry must be divided by 5, giving 3/5, -1/5, -1/5 and 2/5.
@feedback 33 | This uses -5 as divisor. The computed determinant is +5, so these four signs are reversed.
@after B=\begin{bmatrix}\frac{3}{5}&-\frac{1}{5}\\-\frac{1}{5}&\frac{2}{5}\end{bmatrix}
@wrong Scale every entry by the reciprocal of the actual determinant.
@why Since 5 is nonzero, B=J/5 is defined. The entries are 3/5, (-1)/5, (-1)/5 and 2/5; the matrix products now test this construction.
@step 40 | Compute original A times B. Which evaluated product results?
@choice 42 | AB=\begin{bmatrix}-1&0\\0&1\end{bmatrix}
@choice 41 | AB=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@choice 43 | AB=\begin{bmatrix}1&1\\0&1\end{bmatrix}
@answer 41
@feedback 42 | The first diagonal entry is 2(3/5)+1(-1/5)=6/5-1/5=1.
@feedback 43 | The upper-right entry is 2(-1/5)+1(2/5)=0.
@after AB=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@wrong Add both rational products in every entry.
@why The entries are (6-1)/5=1, (-2+2)/5=0, (3-3)/5=0 and (-1+6)/5=1. Thus AB=I.
@step 50 | Compute B times original A. Which evaluated product completes the check?
@choice 52 | BA=\begin{bmatrix}1&0\\1&1\end{bmatrix}
@choice 53 | BA=\begin{bmatrix}0&1\\1&0\end{bmatrix}
@choice 51 | BA=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@answer 51
@feedback 52 | The lower-left entry is (-1/5)2+(2/5)1=0.
@feedback 53 | Row 1 is [(3/5)2+(-1/5)1,(3/5)1+(-1/5)3]=[1,0].
@after BA=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@wrong Use B's rows and original A's columns.
@why The first row is [(6-1)/5,(3-3)/5]=[1,0]; the second is [(-2+2)/5,(-1+6)/5]=[0,1]. Both products establish the inverse.
@end

@question prod03_linear_matrix_inverse_q06 | Matrix investigation 6
@template choices.v1
@version 1
@goal Construct the inverse of A and verify both AB=I and BA=I against the original matrix.
@given A=\begin{bmatrix}1&3\\2&1\end{bmatrix}
@domain All entries and vectors are real. Rows precede columns. I is the 2 by 2 identity, B is a candidate inverse, and R1 and R2 denote complete rows.
@read prod03_linear_matrix_inverse_r
@step 10 | Compute ad-bc to determine whether the adjugate formula permits division.
@choice 12 | \det(A)=7
@choice 13 | \det(A)=5
@choice 11 | \det(A)=-5
@answer 11
@feedback 12 | Adding 1 and 6 gives 7, but the determinant is their ordered difference 1-6=-5.
@feedback 13 | The difference is 1-6, not 6-1. Retain the negative sign.
@after \det(A)=-5
@wrong A determinant may be negative and still be nonzero.
@why Compute 1(1)-3(2)=1-6=-5. Nonzero, rather than positive, is the condition for the inverse formula.
@step 20 | Form the evaluated adjugate J=[d,-b;-c,a].
@choice 21 | J=\begin{bmatrix}1&-3\\-2&1\end{bmatrix}
@choice 22 | J=\begin{bmatrix}1&3\\2&1\end{bmatrix}
@choice 23 | J=\begin{bmatrix}1&-2\\-3&1\end{bmatrix}
@answer 21
@feedback 22 | The off-diagonal 3 and 2 must become -3 and -2.
@feedback 23 | This exchanges the off-diagonal positions. The upper-right is -b=-3, not -c=-2.
@after J=\begin{bmatrix}1&-3\\-2&1\end{bmatrix}
@wrong Negate the off-diagonal entries without swapping their positions.
@why Both diagonal entries are 1, so their swap is invisible here. The off-diagonal entries become -3 in row 1 and -2 in row 2.
@step 30 | Divide J by the nonzero determinant. Give B with every entry evaluated, not an outside scalar.
@choice 32 | B=\begin{bmatrix}\frac{1}{5}&-\frac{3}{5}\\-\frac{2}{5}&\frac{1}{5}\end{bmatrix}
@choice 31 | B=\begin{bmatrix}-\frac{1}{5}&\frac{3}{5}\\\frac{2}{5}&-\frac{1}{5}\end{bmatrix}
@choice 33 | B=\begin{bmatrix}1&-3\\-2&1\end{bmatrix}
@answer 31
@feedback 32 | This divides by +5. Division by -5 makes the diagonal entries negative and both off-diagonal entries positive.
@feedback 33 | This is the undivided adjugate; the divisor -5 must apply to every entry.
@after B=\begin{bmatrix}-\frac{1}{5}&\frac{3}{5}\\\frac{2}{5}&-\frac{1}{5}\end{bmatrix}
@wrong Preserve the sign of the determinant in every quotient.
@why The entries are 1/(-5)=-1/5, (-3)/(-5)=3/5, (-2)/(-5)=2/5 and 1/(-5)=-1/5.
@step 40 | Multiply original A by B. Which evaluated product results?
@choice 42 | AB=\begin{bmatrix}-1&0\\0&1\end{bmatrix}
@choice 43 | AB=\begin{bmatrix}1&1\\0&1\end{bmatrix}
@choice 41 | AB=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@answer 41
@feedback 42 | The first diagonal entry is 1(-1/5)+3(2/5)=(-1+6)/5=1.
@feedback 43 | The upper-right entry is 1(3/5)+3(-1/5)=0.
@after AB=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@wrong A negative first product can be cancelled or exceeded by the second product.
@why The entries are (-1+6)/5=1, (3-3)/5=0, (-2+2)/5=0 and (6-1)/5=1.
@step 50 | Multiply B by original A. Which evaluated product completes the check?
@choice 51 | BA=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@choice 52 | BA=\begin{bmatrix}1&0\\1&1\end{bmatrix}
@choice 53 | BA=\begin{bmatrix}0&1\\1&0\end{bmatrix}
@answer 51
@feedback 52 | The lower-left entry is (2/5)1+(-1/5)2=0.
@feedback 53 | Row 1 is [(-1/5)1+(3/5)2,(-1/5)3+(3/5)1]=[1,0].
@after BA=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@wrong Keep the factor order as B then A.
@why The first row is [(-1+6)/5,(-3+3)/5]=[1,0]; the second is [(2-2)/5,(6-1)/5]=[0,1]. Both products are exactly I.
@end

@question prod03_linear_matrix_inverse_q07 | Matrix investigation 7
@template choices.v1
@version 1
@goal Construct the inverse of A and verify both AB=I and BA=I against the original matrix.
@given A=\begin{bmatrix}-2&1\\3&2\end{bmatrix}
@domain All entries and vectors are real. Rows precede columns. I is the 2 by 2 identity, B is a candidate inverse, and R1 and R2 denote complete rows.
@read prod03_linear_matrix_inverse_r
@step 10 | Evaluate the determinant ad-bc before constructing an inverse.
@choice 11 | \det(A)=-7
@choice 12 | \det(A)=-1
@choice 13 | \det(A)=7
@answer 11
@feedback 12 | This adds the off-diagonal product: -4+3=-1. The determinant is -4-3=-7.
@feedback 13 | Reversing the subtraction gives 3-(-4)=7. The prescribed order is ad-bc.
@after \det(A)=-7
@wrong Include the sign of -2 in the diagonal product.
@why The diagonal product is (-2)2=-4 and the other product is 1(3)=3. Their difference is -4-3=-7, which is nonzero.
@step 20 | Form the evaluated adjugate J by swapping the diagonal and negating the off-diagonal.
@choice 22 | J=\begin{bmatrix}2&1\\3&-2\end{bmatrix}
@choice 21 | J=\begin{bmatrix}2&-1\\-3&-2\end{bmatrix}
@choice 23 | J=\begin{bmatrix}-2&-1\\-3&2\end{bmatrix}
@answer 21
@feedback 22 | The off-diagonal entries must be -1 and -3, not 1 and 3.
@feedback 23 | The diagonal has not been swapped. Put d=2 first and a=-2 last.
@after J=\begin{bmatrix}2&-1\\-3&-2\end{bmatrix}
@wrong Swapping a negative diagonal entry does not negate it.
@why The diagonal becomes 2,-2. The original off-diagonal 1,3 becomes -1,-3, giving J=[2,-1;-3,-2].
@step 30 | Divide every entry of J by -7 and display B with evaluated entries.
@choice 32 | B=\begin{bmatrix}\frac{2}{7}&-\frac{1}{7}\\-\frac{3}{7}&-\frac{2}{7}\end{bmatrix}
@choice 33 | B=\begin{bmatrix}2&-1\\-3&-2\end{bmatrix}
@choice 31 | B=\begin{bmatrix}-\frac{2}{7}&\frac{1}{7}\\\frac{3}{7}&\frac{2}{7}\end{bmatrix}
@answer 31
@feedback 32 | These signs come from dividing by positive 7; the determinant is -7.
@feedback 33 | This leaves J undivided. Each of 2,-1,-3,-2 needs division by -7.
@after B=\begin{bmatrix}-\frac{2}{7}&\frac{1}{7}\\\frac{3}{7}&\frac{2}{7}\end{bmatrix}
@wrong A negative divided by a negative is positive.
@why The quotients are -2/7, 1/7, 3/7 and 2/7. No division is singular because -7 is nonzero.
@step 40 | Compute original A times B. Which evaluated product results?
@choice 41 | AB=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@choice 42 | AB=\begin{bmatrix}-1&0\\0&1\end{bmatrix}
@choice 43 | AB=\begin{bmatrix}1&1\\0&1\end{bmatrix}
@answer 41
@feedback 42 | The upper-left entry is (-2)(-2/7)+1(3/7)=(4+3)/7=1.
@feedback 43 | The upper-right entry is (-2)(1/7)+1(2/7)=0.
@after AB=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@wrong Apply both original row signs in each dot product.
@why The four entries are (4+3)/7=1, (-2+2)/7=0, (-6+6)/7=0 and (3+4)/7=1.
@step 50 | Compute B times original A. Which evaluated product completes the check?
@choice 52 | BA=\begin{bmatrix}1&0\\1&1\end{bmatrix}
@choice 51 | BA=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@choice 53 | BA=\begin{bmatrix}0&1\\1&0\end{bmatrix}
@answer 51
@feedback 52 | The lower-left entry is (3/7)(-2)+(2/7)3=0.
@feedback 53 | Row 1 is [(-2/7)(-2)+(1/7)3,(-2/7)1+(1/7)2]=[1,0].
@after BA=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@wrong Take rows from B and columns from original A.
@why Row 1 is [(4+3)/7,(-2+2)/7]=[1,0]. Row 2 is [(-6+6)/7,(3+4)/7]=[0,1]. The verified candidate is the inverse.
@end

@question prod03_linear_matrix_inverse_q08 | Matrix investigation 8
@template choices.v1
@version 1
@goal Construct the inverse of A and verify both AB=I and BA=I against the original matrix.
@given A=\begin{bmatrix}3&-2\\1&2\end{bmatrix}
@domain All entries and vectors are real. Rows precede columns. I is the 2 by 2 identity, B is a candidate inverse, and R1 and R2 denote complete rows.
@read prod03_linear_matrix_inverse_r
@step 10 | Compute ad-bc, keeping the sign of the off-diagonal product.
@choice 12 | \det(A)=4
@choice 11 | \det(A)=8
@choice 13 | \det(A)=-8
@answer 11
@feedback 12 | This treats subtracting -2 as subtracting +2. The value is 6-(-2)=8, not 4.
@feedback 13 | This reverses the difference: -2-6=-8. Use 6-(-2).
@after \det(A)=8
@wrong Subtracting a negative product adds its magnitude.
@why The diagonal product is 3(2)=6 and the off-diagonal product is (-2)1=-2. Thus the determinant is 6-(-2)=8, which permits division.
@step 20 | Form the evaluated adjugate J=[d,-b;-c,a].
@choice 22 | J=\begin{bmatrix}2&-2\\1&3\end{bmatrix}
@choice 23 | J=\begin{bmatrix}2&-1\\2&3\end{bmatrix}
@choice 21 | J=\begin{bmatrix}2&2\\-1&3\end{bmatrix}
@answer 21
@feedback 22 | Negation changes the original -2 to +2 and +1 to -1.
@feedback 23 | This exchanges the off-diagonal positions. The upper-right is -b=2; the lower-left is -c=-1.
@after J=\begin{bmatrix}2&2\\-1&3\end{bmatrix}
@wrong Negate each off-diagonal entry in place.
@why The diagonal 3,2 becomes 2,3. The off-diagonal -2,1 becomes 2,-1. Therefore J=[2,2;-1,3].
@step 30 | Divide J by 8 and give B with all scalar multiplication evaluated inside the matrix.
@choice 31 | B=\begin{bmatrix}\frac{1}{4}&\frac{1}{4}\\-\frac{1}{8}&\frac{3}{8}\end{bmatrix}
@choice 32 | B=\begin{bmatrix}2&2\\-1&3\end{bmatrix}
@choice 33 | B=\begin{bmatrix}\frac{1}{4}&\frac{1}{4}\\-1&3\end{bmatrix}
@answer 31
@feedback 32 | This is the undivided adjugate. Its entries must all be scaled by 1/8.
@feedback 33 | Only the first row was divided. The second row must be [-1/8,3/8] too.
@after B=\begin{bmatrix}\frac{1}{4}&\frac{1}{4}\\-\frac{1}{8}&\frac{3}{8}\end{bmatrix}
@wrong The scalar 1/8 multiplies the entire matrix, not one row.
@why Dividing gives 2/8=1/4, 2/8=1/4, -1/8 and 3/8. Equivalent exact fractional spellings have the same values.
@step 40 | Compute original A times B. Which evaluated product results?
@choice 42 | AB=\begin{bmatrix}-1&0\\0&1\end{bmatrix}
@choice 41 | AB=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@choice 43 | AB=\begin{bmatrix}1&1\\0&1\end{bmatrix}
@answer 41
@feedback 42 | The upper-left entry is 3(1/4)+(-2)(-1/8)=3/4+1/4=1.
@feedback 43 | The upper-right entry is 3(1/4)+(-2)(3/8)=3/4-3/4=0.
@after AB=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@wrong Use a common denominator when adding each pair of products.
@why The first row is [3/4+1/4,3/4-3/4]=[1,0]. The second is [1/4-2/8,1/4+6/8]=[0,1].
@step 50 | Compute B times original A. Which evaluated product completes the check?
@choice 52 | BA=\begin{bmatrix}1&0\\1&1\end{bmatrix}
@choice 53 | BA=\begin{bmatrix}0&1\\1&0\end{bmatrix}
@choice 51 | BA=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@answer 51
@feedback 52 | The lower-left entry is (-1/8)3+(3/8)1=0.
@feedback 53 | Row 1 is [(1/4)3+(1/4)1,(1/4)(-2)+(1/4)2]=[1,0].
@after BA=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@wrong Keep the sign of -2 in the second column of A.
@why Row 1 is [3/4+1/4,-1/2+1/2]=[1,0]. Row 2 is [-3/8+3/8,2/8+6/8]=[0,1]. Both products verify the inverse.
@end

@question prod03_linear_matrix_inverse_q09 | Matrix investigation 9
@template choices.v1
@version 1
@goal Construct the inverse of A, verify both products, then solve Ax=b and check the original equation.
@given A=\begin{bmatrix}2&-1\\1&1\end{bmatrix},\quad b=\begin{bmatrix}1\\4\end{bmatrix}
@domain All entries and vectors are real. Rows precede columns. I is the 2 by 2 identity, B is a candidate inverse, and R1 and R2 denote complete rows.
@read prod03_linear_matrix_inverse_r
@step 10 | Choose the determinant-based inverse construction with its valid condition, then carry it out before solving Ax=b.
@choice 12 | B=\frac{A}{\det(A)},\quad\det(A)\ne0
@choice 13 | B=\frac{\operatorname{adj}(A)}{\det(A)},\quad\det(A)=0
@choice 11 | B=\frac{\operatorname{adj}(A)}{\det(A)},\quad\det(A)\ne0
@answer 11
@feedback 12 | Division must act on the adjugate, not original A. For this A, the upper-left adjugate entry is 1 rather than 2.
@feedback 13 | A zero determinant makes the displayed division undefined. The condition must be nonzero.
@after B=\frac{\operatorname{adj}(A)}{\det(A)},\quad\det(A)\ne0
@wrong Select a construction whose numerator and division condition are both valid.
@why The adjugate construction divides [d,-b;-c,a] by ad-bc when that scalar is nonzero. Here the vector named b on the original right-hand side is separate from the formula's upper-right entry notation; we will use actual matrix entries next.
@step 20 | Evaluate the determinant of the original matrix to check the selected condition.
@choice 21 | \det(A)=3
@choice 22 | \det(A)=1
@choice 23 | \det(A)=-3
@answer 21
@feedback 22 | The off-diagonal product is (-1)1=-1, so subtracting it gives 2-(-1)=3, not 1.
@feedback 23 | This reverses the difference: -1-2=-3 instead of 2-(-1).
@after \det(A)=3
@wrong Subtract the signed off-diagonal product.
@why The determinant is 2(1)-(-1)(1)=2+1=3. The chosen construction is licensed because 3 is nonzero.
@step 30 | Construct the evaluated adjugate J from the original entries.
@choice 32 | J=\begin{bmatrix}1&-1\\1&2\end{bmatrix}
@choice 31 | J=\begin{bmatrix}1&1\\-1&2\end{bmatrix}
@choice 33 | J=\begin{bmatrix}2&1\\-1&1\end{bmatrix}
@answer 31
@feedback 32 | The off-diagonal -1 becomes +1 and the off-diagonal +1 becomes -1.
@feedback 33 | The diagonal entries must be swapped: 1 first, 2 last.
@after J=\begin{bmatrix}1&1\\-1&2\end{bmatrix}
@wrong Build the adjugate before dividing by the determinant.
@why Swapping diagonal 2,1 gives 1,2. Negating off-diagonal -1,1 gives 1,-1. Therefore J=[1,1;-1,2].
@step 40 | Divide J by the nonzero determinant and give B with every entry evaluated.
@choice 42 | B=\begin{bmatrix}1&1\\-1&2\end{bmatrix}
@choice 43 | B=\begin{bmatrix}-\frac{1}{3}&-\frac{1}{3}\\\frac{1}{3}&-\frac{2}{3}\end{bmatrix}
@choice 41 | B=\begin{bmatrix}\frac{1}{3}&\frac{1}{3}\\-\frac{1}{3}&\frac{2}{3}\end{bmatrix}
@answer 41
@feedback 42 | This omits division by 3. All four entries require the factor 1/3.
@feedback 43 | This divides by -3, but the determinant is positive 3.
@after B=\begin{bmatrix}\frac{1}{3}&\frac{1}{3}\\-\frac{1}{3}&\frac{2}{3}\end{bmatrix}
@wrong Evaluate the scalar multiplication throughout the matrix.
@why The four quotients are 1/3, 1/3, -1/3 and 2/3. Next verify this candidate before using it on the right-hand side.
@step 50 | Compute original A times B. Which evaluated product results?
@choice 51 | AB=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@choice 52 | AB=\begin{bmatrix}-1&0\\0&1\end{bmatrix}
@choice 53 | AB=\begin{bmatrix}1&1\\0&1\end{bmatrix}
@answer 51
@feedback 52 | The upper-left entry is 2(1/3)+(-1)(-1/3)=(2+1)/3=1.
@feedback 53 | The upper-right entry is 2(1/3)+(-1)(2/3)=0.
@after AB=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@wrong Use the original matrix, not its adjugate, as the first factor.
@why The first row is [(2+1)/3,(2-2)/3]=[1,0]; the second is [(1-1)/3,(1+2)/3]=[0,1].
@step 60 | Compute B times original A. Which evaluated product completes the inverse check?
@choice 62 | BA=\begin{bmatrix}1&0\\1&1\end{bmatrix}
@choice 61 | BA=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@choice 63 | BA=\begin{bmatrix}0&1\\1&0\end{bmatrix}
@answer 61
@feedback 62 | The lower-left entry is (-1/3)2+(2/3)1=0.
@feedback 63 | Row 1 is [(1/3)2+(1/3)1,(1/3)(-1)+(1/3)1]=[1,0].
@after BA=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@wrong Recompute in reverse order before treating B as the inverse.
@why Row 1 is [1,0]. Row 2 is [(-2+2)/3,(1+2)/3]=[0,1]. Both products are I, so multiplying Ax=b by B is valid.
@step 70 | Compute x=Bb from the original right-hand-side column. Which evaluated vector results?
@choice 72 | x=\begin{bmatrix}\frac{7}{3}\\\frac{5}{3}\end{bmatrix}
@choice 73 | x=\begin{bmatrix}\frac{5}{3}\\3\end{bmatrix}
@choice 71 | x=\begin{bmatrix}\frac{5}{3}\\\frac{7}{3}\end{bmatrix}
@answer 71
@feedback 72 | This reverses the two coordinates. The first row gives (1+4)/3=5/3; the second gives (-1+8)/3=7/3.
@feedback 73 | The second row begins with -1/3, so its value is -1/3+8/3=7/3, not 1/3+8/3=3.
@after x=\begin{bmatrix}\frac{5}{3}\\\frac{7}{3}\end{bmatrix}
@wrong Multiply each row of B by the column [1,4].
@why The coordinates are (1/3)1+(1/3)4=5/3 and (-1/3)1+(2/3)4=7/3. The verified inverse makes this the only possible solution.
@step 80 | Substitute x into the ORIGINAL equation. Which evaluated Ax confirms the right-hand side?
@choice 81 | Ax=\begin{bmatrix}1\\4\end{bmatrix}
@choice 82 | Ax=\begin{bmatrix}\frac{17}{3}\\4\end{bmatrix}
@choice 83 | Ax=\begin{bmatrix}1\\-\frac{2}{3}\end{bmatrix}
@answer 81
@feedback 82 | Row 1 subtracts the second coordinate: 2(5/3)-7/3=1. Adding it instead gives 17/3.
@feedback 83 | Row 2 adds both coordinates: 5/3+7/3=4. Subtracting gives -2/3.
@after Ax=\begin{bmatrix}1\\4\end{bmatrix}
@wrong Return to original A and its original right-hand side.
@why The original products are 10/3-7/3=1 and 5/3+7/3=4, exactly b. This proves existence; the inverse argument proves uniqueness.
@end

@question prod03_linear_matrix_inverse_q10 | Matrix investigation 10
@template choices.v1
@version 1
@goal Construct the inverse of A, verify both products, then solve Ax=b and check the original equation.
@given A=\begin{bmatrix}0&1\\2&1\end{bmatrix},\quad b=\begin{bmatrix}3\\-1\end{bmatrix}
@domain All entries and vectors are real. Rows precede columns. I is the 2 by 2 identity, B is a candidate inverse, and R1 and R2 denote complete rows.
@read prod03_linear_matrix_inverse_r
@step 10 | Begin with [A|I]. Choose and apply a first move that places an existing row with a nonzero first entry on top, without combining or scaling rows.
@choice 11 | R_1\leftrightarrow R_2
@choice 12 | R_1\leftarrow 0R_1
@choice 13 | R_1\leftarrow R_1+R_2
@answer 11
@feedback 12 | Multiplying row 1 by zero destroys its information and is not reversible; its first entry would still be zero.
@feedback 13 | Adding row 2 produces [2,2|1,1], a reversible nonzero pivot move, but it combines rows and misses the stated existing-row goal.
@after \left[\begin{array}{cc|cc}2&1&0&1\\0&1&1&0\end{array}\right]
@wrong Move the existing complete row, including both identity entries.
@why Swap the complete rows: [2,1|0,1] moves above [0,1|1,0]. This puts a nonzero first entry on top without altering either row's contents. Swapping again reverses it.
@step 20 | Replace row 1 by row 1 minus row 2. Which complete augmented matrix results?
@choice 22 | \left[\begin{array}{cc|cc}2&0&0&1\\0&1&1&0\end{array}\right]
@choice 21 | \left[\begin{array}{cc|cc}2&0&-1&1\\0&1&1&0\end{array}\right]
@choice 23 | \left[\begin{array}{cc|cc}2&2&1&1\\0&1&1&0\end{array}\right]
@answer 21
@feedback 22 | The first right-block entry must become 0-1=-1, not remain zero.
@feedback 23 | This adds the rows, giving second entry 1+1=2. It is reversible but misses the requested subtraction.
@after \left[\begin{array}{cc|cc}2&0&-1&1\\0&1&1&0\end{array}\right]
@wrong Subtract all four entries of row 2 from row 1.
@why The new first row is [2-0,1-1|0-1,1-0]=[2,0|-1,1]. Row 2 stays fixed; adding it back reverses the operation.
@step 30 | Divide every entry of row 1 by 2. Which augmented matrix results?
@choice 32 | \left[\begin{array}{cc|cc}1&0&-1&1\\0&1&1&0\end{array}\right]
@choice 33 | \left[\begin{array}{cc|cc}-1&0&\frac{1}{2}&-\frac{1}{2}\\0&1&1&0\end{array}\right]
@choice 31 | \left[\begin{array}{cc|cc}1&0&-\frac{1}{2}&\frac{1}{2}\\0&1&1&0\end{array}\right]
@answer 31
@feedback 32 | Both augmented entries need division: -1/2 and 1/2.
@feedback 33 | This divides by -2. It is reversible but gives pivot -1 instead of the requested 1.
@after \left[\begin{array}{cc|cc}1&0&-\frac{1}{2}&\frac{1}{2}\\0&1&1&0\end{array}\right]
@wrong Scale the entire first row, not just the left block.
@why The entries are 2/2=1, 0/2=0, -1/2 and 1/2. Since 2 is nonzero, the step is reversible; the left block is now I.
@step 40 | Read B from the transformed right block with evaluated entries.
@choice 41 | B=\begin{bmatrix}-\frac{1}{2}&\frac{1}{2}\\1&0\end{bmatrix}
@choice 42 | B=\begin{bmatrix}\frac{1}{2}&\frac{1}{2}\\1&0\end{bmatrix}
@choice 43 | B=\begin{bmatrix}-\frac{1}{2}&1\\\frac{1}{2}&0\end{bmatrix}
@answer 41
@feedback 42 | The upper-left right-block entry is -1 divided by 2, so it is -1/2.
@feedback 43 | This transposes the right block. The upper-right is 1/2 and the lower-left is 1.
@after B=\begin{bmatrix}-\frac{1}{2}&\frac{1}{2}\\1&0\end{bmatrix}
@wrong Keep the right block in row-first order.
@why The candidate is [-1/2,1/2;1,0]. We will verify it against the original matrix before applying it to b.
@step 50 | Compute original A times B. Which evaluated product results?
@choice 52 | AB=\begin{bmatrix}-1&0\\0&1\end{bmatrix}
@choice 51 | AB=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@choice 53 | AB=\begin{bmatrix}1&1\\0&1\end{bmatrix}
@answer 51
@feedback 52 | The upper-left entry is 0(-1/2)+1(1)=1.
@feedback 53 | The upper-right entry is 0(1/2)+1(0)=0.
@after AB=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@wrong Use original A, before any row swap or scaling.
@why Row 1 is [0+1,0+0]=[1,0]; row 2 is [2(-1/2)+1(1),2(1/2)+1(0)]=[0,1].
@step 60 | Compute B times original A. Which evaluated product completes the check?
@choice 62 | BA=\begin{bmatrix}1&0\\1&1\end{bmatrix}
@choice 63 | BA=\begin{bmatrix}0&1\\1&0\end{bmatrix}
@choice 61 | BA=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@answer 61
@feedback 62 | The lower-left entry is 1(0)+0(2)=0.
@feedback 63 | The first row is [(-1/2)0+(1/2)2,(-1/2)1+(1/2)1]=[1,0].
@after BA=\begin{bmatrix}1&0\\0&1\end{bmatrix}
@wrong Compute the reverse product without assuming commutativity.
@why Row 1 is [1,-1/2+1/2]=[1,0]; row 2 is [0,1]. Both products are I, so B is an inverse of original A.
@step 70 | Compute x=Bb with the original column [3,-1]. Which evaluated vector results?
@choice 71 | x=\begin{bmatrix}-2\\3\end{bmatrix}
@choice 72 | x=\begin{bmatrix}-1\\3\end{bmatrix}
@choice 73 | x=\begin{bmatrix}3\\-2\end{bmatrix}
@answer 71
@feedback 72 | The first coordinate is (-1/2)3+(1/2)(-1)=-3/2-1/2=-2. Replacing the last minus with plus gives -1.
@feedback 73 | The coordinates have been exchanged. Row 1 gives -2, while row 2 gives 1(3)+0(-1)=3.
@after x=\begin{bmatrix}-2\\3\end{bmatrix}
@wrong Keep the negative second entry of b.
@why The product gives [-3/2-1/2,3+0]=[-2,3]. Multiplying any solution of Ax=b by B forces this same vector, so it is the only candidate.
@step 80 | Evaluate original Ax for the candidate. Which vector verifies the original equation?
@choice 82 | Ax=\begin{bmatrix}3\\7\end{bmatrix}
@choice 81 | Ax=\begin{bmatrix}3\\-1\end{bmatrix}
@choice 83 | Ax=\begin{bmatrix}-2\\3\end{bmatrix}
@answer 81
@feedback 82 | The second row gives 2(-2)+3=-4+3=-1. Using +2 instead gives 7.
@feedback 83 | This repeats x instead of multiplying by A. Row 1 of A returns the second coordinate, 3.
@after Ax=\begin{bmatrix}3\\-1\end{bmatrix}
@wrong Evaluate the original two rows, not the reduced identity rows.
@why The original products are 0(-2)+1(3)=3 and 2(-2)+1(3)=-1, exactly the given b. This verifies existence and completes the unique solution.
@end

@question prod03_linear_matrix_inverse_q11 | Matrix investigation 11
@template choices.v1
@version 1
@goal Determine whether A has an inverse and prove the conclusion using its determinant and a nonzero vector v with Av=0.
@given A=\begin{bmatrix}1&2\\2&4\end{bmatrix}
@domain All entries and vectors are real. Rows precede columns. I is the 2 by 2 identity, B is a candidate inverse, and R1 and R2 denote complete rows.
@read prod03_linear_matrix_inverse_r
@step 10 | Compute ad-bc to check whether division by the determinant is permitted.
@choice 12 | \det(A)=8
@choice 11 | \det(A)=0
@choice 13 | \det(A)=4
@answer 11
@feedback 12 | Adding 4+4 gives 8; the determinant subtracts them, giving 0.
@feedback 13 | This keeps only ad=4 and omits bc=4. Their difference is zero.
@after \det(A)=0
@wrong Do not divide until the determinant has been checked.
@why The determinant is 1(4)-2(2)=4-4=0. The adjugate formula would divide by zero and cannot construct an inverse. We will prove nonexistence using the original matrix.
@step 20 | Choose a nonzero vector v with second coordinate 1 for which Av=0.
@choice 22 | v=\begin{bmatrix}2\\1\end{bmatrix}
@choice 23 | v=\begin{bmatrix}0\\0\end{bmatrix}
@choice 21 | v=\begin{bmatrix}-2\\1\end{bmatrix}
@answer 21
@feedback 22 | The first row gives 1(2)+2(1)=4, not zero. With second coordinate 1, the first must be -2.
@feedback 23 | A sends this zero vector to zero, but it is not a nonzero witness and its second coordinate is not 1. It cannot prove singularity.
@after v=\begin{bmatrix}-2\\1\end{bmatrix}
@wrong Solve the first row equation with the stated second coordinate, then keep the witness nonzero.
@why The first row requires v1+2(1)=0, so v1=-2. The second coordinate 1 ensures v is nonzero. Next check both original rows.
@step 30 | Evaluate Av using that witness and the original matrix.
@choice 31 | Av=\begin{bmatrix}0\\0\end{bmatrix}
@choice 32 | Av=\begin{bmatrix}4\\8\end{bmatrix}
@choice 33 | Av=\begin{bmatrix}0\\4\end{bmatrix}
@answer 31
@feedback 32 | These values use +2 instead of the witness's -2. The actual products are -2+2=0 and -4+4=0.
@feedback 33 | The second row includes 2(-2)=-4 as well as 4(1)=4; omitting the first term leaves the incorrect 4.
@after Av=\begin{bmatrix}0\\0\end{bmatrix}
@wrong Both rows must annihilate the same nonzero vector.
@why Row 1 gives 1(-2)+2(1)=0 and row 2 gives 2(-2)+4(1)=0. Thus original A sends the nonzero vector [-2,1] to zero.
@step 40 | Suppose an inverse C existed. Using CA=I and Av=0, which numeric equality would v=C(Av) force?
@choice 42 | \begin{bmatrix}-2\\1\end{bmatrix}=\begin{bmatrix}-2\\1\end{bmatrix}
@choice 41 | \begin{bmatrix}-2\\1\end{bmatrix}=\begin{bmatrix}0\\0\end{bmatrix}
@choice 43 | \begin{bmatrix}-2\\1\end{bmatrix}=\begin{bmatrix}2\\-1\end{bmatrix}
@answer 41
@feedback 42 | This tautology is true, but it does not evaluate C(Av). Since Av=0, linear multiplication gives C(Av)=C0=0, not a retained copy of v.
@feedback 43 | Multiplication by C sends the zero vector to zero, not to the negative of v.
@after \begin{bmatrix}-2\\1\end{bmatrix}=\begin{bmatrix}0\\0\end{bmatrix},\quad\nexists A^{-1}
@wrong Evaluate C on the zero result before interpreting the contradiction.
@why If CA=I, then (CA)v=v. But C(Av)=C0=0, so associativity would force [-2,1]=[0,0], which is false. Therefore no inverse exists, agreeing with the zero determinant.
@end

@question prod03_linear_matrix_inverse_q12 | Matrix investigation 12
@template choices.v1
@version 1
@goal Determine whether A has an inverse and prove the conclusion using its determinant and a nonzero vector v with Av=0.
@given A=\begin{bmatrix}0&0\\-3&0\end{bmatrix}
@domain All entries and vectors are real. Rows precede columns. I is the 2 by 2 identity, B is a candidate inverse, and R1 and R2 denote complete rows.
@read prod03_linear_matrix_inverse_r
@step 10 | Compute the determinant ad-bc of original A.
@choice 12 | \det(A)=-3
@choice 13 | \det(A)=3
@choice 11 | \det(A)=0
@answer 11
@feedback 12 | The entry -3 is not itself a determinant. The off-diagonal product is 0(-3)=0.
@feedback 13 | Negating -3 does not compute ad-bc. Both products are zero.
@after \det(A)=0
@wrong Each diagonal product contains two factors, including zeros.
@why Compute 0(0)-0(-3)=0-0=0. Division by this determinant is undefined. A nonzero null vector will show why no inverse can exist.
@step 20 | Choose a nonzero vector v with second coordinate 1 for which Av=0.
@choice 21 | v=\begin{bmatrix}0\\1\end{bmatrix}
@choice 22 | v=\begin{bmatrix}1\\1\end{bmatrix}
@choice 23 | v=\begin{bmatrix}0\\0\end{bmatrix}
@answer 21
@feedback 22 | The second row would give -3(1)+0(1)=-3, not zero. The first coordinate must be 0.
@feedback 23 | The zero vector is annihilated but is not a nonzero witness and does not have second coordinate 1.
@after v=\begin{bmatrix}0\\1\end{bmatrix}
@wrong The zero first row imposes no restriction; use the nonzero second row.
@why The second row requires -3v1=0, so v1=0. Set the specified second coordinate to 1. This gives a nonzero vector without dividing by a zero pivot.
@step 30 | Multiply original A by the chosen witness. Which evaluated vector results?
@choice 32 | Av=\begin{bmatrix}0\\-3\end{bmatrix}
@choice 31 | Av=\begin{bmatrix}0\\0\end{bmatrix}
@choice 33 | Av=\begin{bmatrix}0\\1\end{bmatrix}
@answer 31
@feedback 32 | The -3 multiplies the first coordinate, which is zero: -3(0)+0(1)=0.
@feedback 33 | This copies v rather than computing Av. The second row gives 0, not 1.
@after Av=\begin{bmatrix}0\\0\end{bmatrix}
@wrong Multiply each row by the witness in its original coordinate order.
@why Row 1 gives 0(0)+0(1)=0; row 2 gives (-3)(0)+0(1)=0. The original matrix therefore annihilates the nonzero vector [0,1].
@step 40 | Suppose an inverse C existed. Using CA=I and Av=0, which numeric equality would v=C(Av) force?
@choice 42 | \begin{bmatrix}0\\1\end{bmatrix}=\begin{bmatrix}0\\1\end{bmatrix}
@choice 43 | \begin{bmatrix}0\\1\end{bmatrix}=\begin{bmatrix}0\\-1\end{bmatrix}
@choice 41 | \begin{bmatrix}0\\1\end{bmatrix}=\begin{bmatrix}0\\0\end{bmatrix}
@answer 41
@feedback 42 | This true tautology omits the consequence of Av=0. The expression C(Av) must equal C0=0.
@feedback 43 | Any matrix times the zero vector is zero, not the negative of this witness.
@after \begin{bmatrix}0\\1\end{bmatrix}=\begin{bmatrix}0\\0\end{bmatrix},\quad\nexists A^{-1}
@wrong The proposed inverse must send Av=0 back to v, producing the contradiction.
@why CA=I would give v=(CA)v=C(Av)=0, or [0,1]=[0,0]. Its second coordinates contradict each other. No inverse exists, exactly as the zero determinant indicates.
@end
