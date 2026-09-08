# Linear Algebra

## Subcategory Map

1. Vector Foundation
   - Vector Representation
   - Inner Product Basics
2. Vector Spaces and Subspaces
   - Axioms
   - Spans and Independence
3. Linear Maps
   - Transformation Definition
   - Kernel and Image
4. Matrix Algebra
   - Matrix Operations
   - Inverses
5. Determinants
   - Properties
   - Expansion Methods
6. Linear Systems
   - Row Reduction
   - Rank and Nullity
7. Orthogonality
   - Orthonormal Sets
   - Projections
8. Spectral Theory
   - Eigenvalues
   - Diagonalization and Decomposition

## Table of Contents

1. [Vector Foundation](#vector-foundation)
   1. [Vector Representation](#vector-representation)
   2. [Inner Product Basics](#inner-product-basics)
2. [Vector Spaces and Subspaces](#vector-spaces-and-subspaces)
   1. [Axioms](#axioms)
   2. [Spans and Independence](#spans-and-independence)
3. [Linear Maps](#linear-maps)
   1. [Transformation Definition](#transformation-definition)
   2. [Kernel and Image](#kernel-and-image)
4. [Matrix Algebra](#matrix-algebra)
   1. [Matrix Operations](#matrix-operations)
   2. [Inverses](#inverses)
5. [Determinants](#determinants)
   1. [Properties](#properties)
   2. [Expansion Methods](#expansion-methods)
6. [Linear Systems](#linear-systems)
   1. [Row Reduction](#row-reduction)
   2. [Rank and Nullity](#rank-and-nullity)
7. [Orthogonality](#orthogonality)
   1. [Orthonormal Sets](#orthonormal-sets)
   2. [Projections](#projections)
8. [Spectral Theory](#spectral-theory)
   1. [Eigenvalues](#eigenvalues)
   2. [Diagonalization and Decomposition](#diagonalization-and-decomposition)
9. [Round 6 Supplemental Terms](#round-6-supplemental-terms)
   1. [Further Linear Algebra Terms](#further-linear-algebra-terms)
10. [Round 7 Supplemental Terms](#round-7-supplemental-terms)
   1. [Advanced Linear Operators](#advanced-linear-operators)
11. [Round 8 Supplemental Terms](#round-8-supplemental-terms)
   1. [Advanced Linear Operator Extensions](#advanced-linear-operator-extensions)
12. [Round 9 Supplemental Terms](#round-9-supplemental-terms)
   1. [Structured Linear Spaces](#structured-linear-spaces)
13. [Round 10 Supplemental Terms](#round-10-supplemental-terms)
   1. [Operator Decompositions](#operator-decompositions)
14. [Round 11 Supplemental Terms](#round-11-supplemental-terms)
   1. [High-Order Operator Structures](#high-order-operator-structures)
15. [Round 12 Supplemental Terms](#round-12-supplemental-terms)
   1. [Spectral and Operator Analysis](#spectral-and-operator-analysis)
16. [Round 13 Supplemental Terms](#round-13-supplemental-terms)
   1. [Tensor, Bilinear and Multilinear Forms](#tensor-bilinear-and-multilinear-forms)
- [Round 14 Supplemental Terms](#round-14-supplemental-terms)
   1. [Tensor and Factorization Structures](#tensor-and-factorization-structures)
- [Round 15 Supplemental Terms](#round-15-supplemental-terms)
   1. [Operator Algebra and Geometry](#operator-algebra-and-geometry)
- [Round 16 Supplemental Terms](#round-16-supplemental-terms)
   1. [Structured Transform Methods](#structured-transform-methods)
- [Round 17 Supplemental Terms](#round-17-supplemental-terms)
   1. [Structured Decompositions and Stability](#structured-decompositions-and-stability)
- [Round 18 Supplemental Terms](#round-18-supplemental-terms)
   1. [Geometric and Structured Factorizations](#geometric-and-structured-factorizations)
- [Round 19 Supplemental Terms](#round-19-supplemental-terms)
   1. [Functional-Analytic Spectral Tools](#functional-analytic-spectral-tools)
- [Round 20 Supplemental Terms](#round-20-supplemental-terms)
   1. [Operator Perturbation and Stability](#operator-perturbation-and-stability)
- [Round 21 Supplemental Terms](#round-21-supplemental-terms)
   1. [Subspace Approximation and Geometry](#subspace-approximation-and-geometry)
- [Round 22 Supplemental Terms](#round-22-supplemental-terms)
   1. [Canonical Decompositions and Matrix Geometry](#canonical-decompositions-and-matrix-geometry)
- [Round 23 Supplemental Terms](#round-23-supplemental-terms)
   1. [Unitary Orbits and Operator Geometry](#unitary-orbits-and-operator-geometry)

## Vector Foundation

### Vector Representation

#### Term: Vector
A vector in \(\mathbb R^n\) is \((v_1,\dots,v_n)\).

#### Term: Zero Vector
The additive identity in a vector space, denoted \(\mathbf{0}\).

### Inner Product Basics

#### Term: Dot Product
\(u\cdot v=\sum_{i=1}^n u_i v_i\).

#### Term: Norm
\(\|v\|=\sqrt{v\cdot v}\).

## Vector Spaces and Subspaces

### Axioms

#### Term: Vector Space
A set with operations satisfying vector space axioms over a field.

#### Term: Subfield and Scalar
Scalars come from a field \(\mathbb F\) in which vectors are scaled.

### Spans and Independence

#### Term: Span
\(\text{span}(S)\) is all linear combinations of vectors in \(S\).

#### Term: Linear Independence
\(\sum c_i v_i=0\Rightarrow c_i=0\) for all \(i\).

#### Term: Basis Dimension
Dimension is size of any basis.

## Linear Maps

### Transformation Definition

#### Term: Linear Transformation
\(T(u+v)=T(u)+T(v),\;T(cu)=cT(u)\).

#### Term: Matrix Representation
A linear map \(T\) has matrix \([T]\) in chosen bases.

### Kernel and Image

#### Term: Kernel
\(\ker(T)=\{v\mid T(v)=0\}\).

#### Term: Image
\(\operatorname{im}(T)=\{T(v):v\in V\}\).

## Matrix Algebra

### Matrix Operations

#### Term: Matrix Multiplication
\((AB)_{ij}=\sum_k A_{ik}B_{kj}\).

#### Term: Matrix Transpose
\((A^T)_{ij}=A_{ji}\).

### Inverses

#### Term: Matrix Inverse
\(AA^{-1}=A^{-1}A=I\) when \(A^{-1}\) exists.

#### Term: Orthogonal Matrix
\(Q^TQ=I\) implies \(Q^{-1}=Q^T\).

## Determinants

### Properties

#### Term: Determinant
\(\det(A)=0\) iff \(A\) is singular.

#### Term: Determinant Multiplicativity
\(\det(AB)=\det(A)\det(B)\).

### Expansion Methods

#### Term: Cofactor Expansion
\(\det(A)=\sum_j a_{ij}C_{ij}\).

#### Term: Laplace Expansion
General recursive expansion by cofactors along a row or column.

## Linear Systems

### Row Reduction

#### Term: Gaussian Elimination
Systematic elimination by row operations.

#### Term: Reduced Row Echelon Form
RREF gives unique canonical form by pivoting.

### Rank and Nullity

#### Term: Rank
Dimension of column space.

#### Term: Nullity
Dimension of kernel; \(\text{rank} + \text{nullity}=\dim V\).

## Orthogonality

### Orthonormal Sets

#### Term: Orthonormal Set
Pairwise orthogonal with unit norm vectors.

#### Term: Orthonormal Basis
A basis where all vectors are orthonormal.

### Projections

#### Term: Orthogonal Projection
\(\text{proj}_v(u)=\frac{u\cdot v}{v\cdot v}v\).

#### Term: Gram-Schmidt
Constructs orthonormal basis from independent vectors.

## Spectral Theory

### Eigenvalues

#### Term: Eigenpair
\(Av=\lambda v\), \(v\neq 0\).

#### Term: Characteristic Polynomial
\(p(\lambda)=\det(A-\lambda I)\).

#### Term: Algebraic Multiplicity
Multiplicity of eigenvalue as root multiplicity of characteristic polynomial.

### Diagonalization and Decomposition

#### Term: Diagonalizable
\(A=PDP^{-1}\) with diagonal \(D\).

#### Term: Singular Value Decomposition
\(A=U\Sigma V^\top\) with orthogonal/unitary factors.

#### Term: Rank-Nullity Theorem
\(\dim V = \dim\ker T + \dim\operatorname{im}T\).

## Formal Theorems

### Theorem (Rank-Nullity)
For linear map \(T:V\to W\) with \(\dim V=n\):
\[
\operatorname{rank}(T)+\operatorname{nullity}(T)=n.
\]

### Theorem (Determinant and Invertibility)
A square matrix \(A\) is invertible iff \(\det(A)\neq 0\).

### Theorem (Spectral Decomposition Criterion)
If matrix \(A\) has \(n\) linearly independent eigenvectors, then \(A\) is diagonalizable.

### Theorem (Cauchy-Schwarz Inequality)
For \(u,v\in\mathbb R^n\),
\[
|u\cdot v|\le \|u\|\,\|v\|,
\]
and equality holds iff \(u\) and \(v\) are linearly dependent.

## Worked Examples

### Example (Find Inverse of a Matrix)
For
\[
A=\begin{pmatrix}1&2\\0&1\end{pmatrix},\quad \det(A)=1\neq 0,
\]
so
\[
A^{-1}=\begin{pmatrix}1&-2\\0&1\end{pmatrix}.
\]

### Example (Rank-Nullity)
For \(T:\mathbb R^3\to\mathbb R^2\) with matrix
\[
\begin{pmatrix}1&0&0\\0&1&0\end{pmatrix},
\]
rank is 2, so nullity is \(3-2=1\).
Kernel is spanned by \((0,0,1)\).

### Example (Dot Product Orthogonality)
For \(u=(1,2,2)\), \(v=(2,-1,1)\):
\[
u\cdot v=1\cdot2+2\cdot(-1)+2\cdot1=2-2+2=2.\]
Hence not orthogonal.

### Example (Eigenpair)
For \(A=\begin{pmatrix}2&0\\0&3\end{pmatrix}\),
\(\lambda_1=2\), \(\lambda_2=3\) with eigenvectors \((1,0)\), \((0,1)\).
\(A\) is already diagonal.

## Worked Examples (Q to A Mini-Proofs)

### Q1. Prove that \(A=\begin{pmatrix}1&2\\0&1\end{pmatrix}\) is invertible and find \(A^{-1}\).

**Answer.** Compute determinant:
\[
\det A=1\cdot1-0\cdot2=1\neq0.
\]
Nonzero determinant is equivalent to invertibility for square matrices. So \(A^{-1}\) exists and is
\[
\frac{1}{\det A}\begin{pmatrix}1&-2\\0&1\end{pmatrix}=\begin{pmatrix}1&-2\\0&1\end{pmatrix}.
\]
Check:
\[
\begin{pmatrix}1&2\\0&1\end{pmatrix}\begin{pmatrix}1&-2\\0&1\end{pmatrix}
=\begin{pmatrix}1&0\\0&1\end{pmatrix}=I.
\]

### Q2. Compute rank and nullity of
\(T:\mathbb R^3\to\mathbb R^2\) with matrix \(\begin{pmatrix}1&0&0\\0&1&0\end{pmatrix}\).

**Answer.** The two pivot columns are the first two, so rank is 2. By rank-nullity with domain dimension 3,
\[
\text{nullity}=3-\text{rank}=1.
\]
Indeed kernel equations are \(x_1=0, x_2=0\); free variable \(x_3\) gives vectors \((0,0,t)\), a one-dimensional kernel.

### Q3. Show \(u=(1,2,2)\) and \(v=(2,-1,1)\) are not orthogonal.

**Answer.** Compute dot product:
\[
u\cdot v=1\cdot2+2\cdot(-1)+2\cdot1=2-2+2=2\neq0.
\]
Since orthogonality requires dot product zero, \(u\) and \(v\) are not orthogonal.

### Q4. Find eigenpairs of \(A=\begin{pmatrix}2&0\\0&3\end{pmatrix}\).

**Answer.** For diagonal matrix, action is coordinate-wise scaling:
\[
A(1,0)^T=(2,0)^T=2(1,0)^T,
\qquad
A(0,1)^T=(0,3)^T=3(0,1)^T.
\]
So eigenpairs are \((\lambda,v)=(2,(1,0)^T)\) and \((3,(0,1)^T)\). Matrix is already diagonal, so diagonalization is trivial with \(P=I\), \(D=A\).

## Additional Mini-Proof Examples (Round 3)

### Q5. Show the vectors \((1,0,0)\), \((0,1,0)\), \((0,0,1)\) form a basis for \(\mathbb R^3\).

**Answer.** Let \((a,b,c)\in\mathbb R^3\) and consider
\[
a(1,0,0)+b(0,1,0)+c(0,0,1)=(a,b,c).
\]
Every vector in \(\mathbb R^3\) is represented with unique coefficients \((a,b,c)\), so the set spans and is independent; hence it is a basis.

### Q6. Prove vectors are orthogonal implies dot product is zero.

**Answer.** By definition of orthogonality in Euclidean inner-product spaces, \(u\perp v\) means angle is \(90^\circ\). For angle \(\theta\),\(u\cdot v=\|u\|\|v\|\cos\theta\). With \(\theta=\pi/2\), \(\cos\theta=0\), so \(u\cdot v=0\).

### Q7. Compute \(\det\) of upper triangular matrix.

**Answer.** For 
\[
A=\begin{pmatrix}3&4&1\\0&2&-1\\0&0&5\end{pmatrix},
\]
determinant is product of diagonal entries:
\[
\det(A)=3\cdot2\cdot5=30.
\]
Hence \(A\) is invertible.

### Q8. Verify \(\operatorname{rank} A + \operatorname{nullity} A = \dim(\mathbb R^3)\) for a simple map.

**Answer.** Let
\[
A=\begin{pmatrix}1&0&1\\0&1&1\end{pmatrix},\quad T(x)=Ax.
\]
Two pivot columns imply rank 2. Since domain is \(\mathbb R^3\), nullity is \(3-2=1\).
Then
\[
\operatorname{rank}(T)+\operatorname{nullity}(T)=2+1=3=\dim\mathbb R^3.
\]
This matches rank-nullity.

## Formal Theorems (Round 4: Standard Statement Pack)

### Theorem (Existence of Basis)
Every vector space \(V\) over a field \(F\) has a basis.

### Theorem (Dimension Formula for Direct Sum)
If \(U,W\subseteq V\) are subspaces with \(U\cap W=\{0\}\), then
\[
\dim(U\oplus W)=\dim U+\dim W.
\]

### Theorem (Cayley-Hamilton Theorem)
For \(A\in M_n(F)\), if \(p_A(t)=\det(tI-A)\) is the characteristic polynomial, then
\[
p_A(A)=0
\]
(the zero matrix).

### Theorem (Spectral Theorem for Real Symmetric Matrices)
If \(A\) is a real symmetric matrix, then \(A\) is diagonalizable by an orthogonal matrix: there exists orthogonal \(Q\) such that \(Q^TAQ=\Lambda\) is diagonal with real entries.

### Theorem (Cauchy-Binet Formula)
For \(A\in M_{m\times n}\), \(B\in M_{n\times m}\),
\[
\det(AB)=\sum_{S\subseteq\{1,\dots,n\},\,|S|=m}\det(A_{[:,S]})\det(B_{[S,:]}).
\]
\(A_{[:,S]}\) and \(B_{[S,:]}\) denote selected-column/row submatrices.

## Round 4 Theorem Proof Sketches

### Q1. Existence of a Basis (Proof idea)
For vector space \(V\), consider set of linearly independent subsets ordered by inclusion. Chains have upper bounds via unions. By Zorn's Lemma, there exists a maximal linearly independent set \(B\). Maximality implies it spans \(V\), otherwise one could adjoin a nonspanned vector and remain independent.
Hence \(B\) is a basis.

### Q2. Dimension formula for direct sum (Proof idea)
Assume \(U\cap W=\{0\}\). Let \(\{u_1,\dots,u_m\}\) be a basis of \(U\) and \(\{w_1,\dots,w_n\}\) be a basis of \(W\).
Any vector in \(U\oplus W\) is written uniquely as \(u+w\), and can be written
\[
\sum_i \alpha_i u_i + \sum_j \beta_j w_j.
\]
Independence: if this sum is 0 then \(\sum_i \alpha_i u_i = -\sum_j \beta_j w_j\in U\cap W=\{0\}\), so both sides zero and coefficients vanish. So concatenation is a basis. Size is \(m+n\).

### Q3. Cayley-Hamilton (Proof idea)
Let \(A\in M_n(F)\), \(p_A(t)=\det(tI-A)=t^n+c_1t^{n-1}+\cdots+c_n\). Define adjugate matrix \(\operatorname{adj}(tI-A)\). Identity:
\[
\operatorname{adj}(tI-A)(tI-A)=p_A(t)I.
\]
Substitute matrix argument \(t=A\):
\[
\operatorname{adj}(AI-A)(AI-A)=\operatorname{adj}(0)(0)=p_A(A)I.
\]
Since \(AI-A=0\), left side is zero, hence \(p_A(A)=0\).

### Q4. Spectral theorem for real symmetric matrices (Proof idea)
For real symmetric \(A\), characteristic polynomial has at least one real eigenvalue \(\lambda\) and unit eigenvector \(v\). Decompose
\[
\mathbb R^n=\operatorname{span}\{v\}\oplus v^\perp.
\]
Space \(v^\perp\) is \(A\)-invariant. Apply induction on dimension to get orthonormal eigenbasis of \(v^\perp\), then add \(v\). Construct orthogonal \(Q\) with these basis vectors as columns so \(Q^TAQ\) is diagonal.

### Q5. Cauchy-Binet (Proof idea)
\(\det(AB)\) for \(m\times m\) product expanded as multilinear alternating form in columns of \(AB\). Replacing each column by linear combinations of columns of \(A\) with coefficients from \(B\) yields sum over all \(m\)-index subsets \(S\) of columns:
\[
\det(AB)=\sum_{|S|=m}\det(A_{[:,S]})\det(B_{[S,:]}).
\]
All other repeated-column terms vanish by alternation, leaving exactly these minors.

## Round 5 Supplemental Terms

### Extended Linear Concepts

#### Term: Affine Subspace
a set \\(v+W=\{v+w\mid w\in W\}\) where \\(W\subseteq V\) is a subspace and \\(v\in V\).

#### Term: Column Space

the subspace \\(\mathcal{C}(A)=\{Ax\mid x\in\mathbb F^n\}\subseteq\mathbb F^m\\) for an \\m\times n\\) matrix \\A.

#### Term: Left Null Space

the null space of \\(^T\!
A\): \\(
\{y\in\mathbb F^m\mid y^TA=0\}.
\\)

#### Term: Orthogonal Complement

the set \\(W^\perp=\{v\mid v\cdot w=0\ \forall w\in W\}\).

#### Term: Symmetric Matrix

a matrix \\(A\\) with \\(A^T=A\).

## Round 6 Supplemental Terms

### Further Linear Algebra Terms

#### Term: Adjugate Matrix

the matrix of cofactors transposed, satisfying
\[
A\,\operatorname{adj}(A)=\det(A)I.
\]

#### Term: Trace

the sum of diagonal entries of a square matrix:
\[
\operatorname{tr}(A)=\sum_i a_{ii}.
\]

#### Term: Jordan Normal Form

a block-diagonal canonical form composed of Jordan blocks, similar to a matrix over algebraically closed fields.

#### Term: Bilinear Form

a map \(B:V\times V\to\mathbb F\) linear in each argument separately.

#### Term: Gram Matrix

the matrix \(G=(\langle v_i,v_j\rangle)_{ij}\) associated with vectors \(v_1,\dots,v_n\).

## Round 7 Supplemental Terms

### Advanced Linear Operators

#### Term: Singular Value Decomposition

Every real matrix \(A\in\mathbb R^{m\times n}\) has factorization
\[
A=U\Sigma V^\top
\]
with \(U,V\) orthogonal and \(\Sigma\) diagonal nonnegative.

#### Term: Orthogonal Projection

Projection onto a subspace \(W\subseteq\mathbb R^n\) is the linear map \(P_W\) with \(P_W^2=P_W\), \(P_W^T=P_W\), and \(P_W v=v\) for \(v\in W\).

#### Term: Moore-Penrose Pseudoinverse

For a matrix \(A\), the pseudoinverse \(A^\dagger\) is the unique matrix satisfying \(AA^\dagger A=A\), \(A^\dagger A A^\dagger=A^\dagger\), and symmetry conditions on \(AA^\dagger\) and \(A^\dagger A\).

#### Term: QR Decomposition

Any full matrix \(A\in\mathbb R^{m\times n}\) can be written as \(A=QR\), with orthogonal \(Q\) and upper-triangular \(R\).

#### Term: Spectral Radius

The spectral radius of \(A\) is
\[
\rho(A)=\max\{|\lambda|\mid \lambda\in\sigma(A)\},
\]
the maximum absolute eigenvalue.

## Round 8 Supplemental Terms

### Advanced Linear Operator Extensions

#### Term: Bilinear Form
A **bilinear form** on a vector space 


$V$


over 


$\mathbb F\) is a map 


$B:V\times V\to\mathbb F$


linear in each argument.

#### Term: Quadratic Form
A **quadratic form** is a map 


$q:V\to\mathbb F\) of the form 


$q(v)=B(v,v)\) for some bilinear form 


$B$.

#### Term: Linear Functional
A **linear functional** is a linear map 


$f:V\to\mathbb F\).


A linear functional belongs to the dual space 


$V^*=\mathrm{Hom}(V,\mathbb F)$.

#### Term: Tensor Product of Vector Spaces
For spaces 


$V,W$, the **tensor product** 


$V\otimes W$


is the universal bilinear target supporting 


$V\times W\to V\otimes W$.

#### Term: Adjoint Operator
For inner-product spaces and 


$T:V\to V$,


its **adjoint** 


$T^*\) satisfies 


$\langle Tv,w\rangle=\langle v,T^*w\rangle$.

## Round 9 Supplemental Terms

### Structured Linear Spaces

#### Term: Normed Vector Space
A vector space 


$V$


with norm 


$\|\cdot\|:V\to[0,\infty)$


satisfying positivity, homogeneity, and triangle inequality is a **normed vector space**.

#### Term: Inner Product Space
A **inner product space** has map 


$\langle\cdot,\cdot\rangle:V\times V\to\mathbb F$


that is conjugate-symmetric, linear in first slot, and positive-definite.

#### Term: Banach Space
A normed vector space is a 


**Banach space** if it is complete in the norm metric 


$d(u,v)=\|u-v\|$.

#### Term: Hilbert Space
A Banach space whose norm comes from an inner product is a 


**Hilbert space**.

#### Term: Compact Operator
A bounded linear operator 


$T:X\to Y$


is **compact** when it sends bounded sets to relatively compact sets.

## Round 10 Supplemental Terms

### Operator Decompositions

#### Term: Singular Value Decomposition
Any matrix 


$A\in\mathbb R^{m\times n}$


factors as


\[A=U\Sigma V^T\]


with orthogonal 


$U,V$ and diagonal singular values in 


$\Sigma$.

#### Term: QR Decomposition
For 


$A\in\mathbb R^{m\times n}$, 


**QR decomposition** writes 


$A=QR$ with orthonormal 


$Q$ and upper-triangular 


$R$.

#### Term: LU Decomposition
**LU decomposition** expresses a matrix as


\(A=LU\)


with 


$L$ lower triangular, 


$U$ upper triangular (under pivot conditions).

#### Term: Cholesky Decomposition
For positive definite 


$A$, 


\(A=LL^T\)


with lower triangular 


$L$ and positive diagonal entries.

#### Term: Polar Decomposition
Every square 


$A\in\mathbb R^{n\times n}$


can be written as 


\(A=UP\)


with orthogonal 


$U$ and symmetric positive semidefinite 


$P$.


## Round 11 Supplemental Terms

### High-Order Operator Structures

#### Term: Normal Operator
An operator 


$T\) on inner product space is **normal** if 


$TT^*=T^*T$.

#### Term: Self-Adjoint Operator

a linear operator 


$T$ is **self-adjoint** if 


$T=T^*$.

#### Term: Unitary Operator


$U$ is **unitary** if 


$U^*U=UU^*=I$.

#### Term: Orthogonal Projection Operator
For closed subspace 


$W\subset V$, 


$P_W^2=P_W$, 


$P_W^*=P_W$, and 


$\operatorname{im}P_W=W$.

#### Term: Functional Calculus for Normal Operators
If 


$T$ is normal and diagonalizable via spectral theorem, one may define 


$f(T)$


for suitable Borel/continuous functions 


$f$ on its spectrum.


## Round 12 Supplemental Terms

### Spectral and Operator Analysis

#### Term: Bounded Operator
A linear operator \(T:X\to Y\) is **bounded** if there exists \(C\ge0\) such that
\[
\|Tx\|\le C\|x\|\quad\forall x\in X.
\]

#### Term: Operator Norm
The **operator norm** of bounded \(T\) is
\[
\|T\|=\sup_{\|x\|=1}\|Tx\|.
\]

#### Term: Spectrum
For operator \(T\), its **spectrum** is
\[
\sigma(T)=\{\lambda\in\mathbb C: T-\lambda I\text{ not invertible}\}.
\]

#### Term: Resolvent
The **resolvent set** is
\[
\rho(T)=\{\lambda\in\mathbb C: (T-\lambda I)^{-1}\text{ exists and is bounded}\},
\]
and resolvent is \(R(\lambda,T)=(T-\lambda I)^{-1}\).

#### Term: Fredholm Operator
A bounded operator \(T\) is **Fredholm** if both kernel and cokernel are finite-dimensional and \(\operatorname{im}T\) is closed.

#### Term: Index of Fredholm Operator
For Fredholm \(T\), 
\(
\mathrm{ind}(T)=\dim\ker T-\dim\ker T^*.
\)

## Round 13 Supplemental Terms

### Tensor, Bilinear and Multilinear Forms

#### Term: Tensor Product
For vector spaces \(V,W\), the **tensor product** \(V\otimes W\) is universal for bilinear maps from \(V\times W\).

#### Term: Exterior Product
The **exterior product** \(v\wedge w\) is alternating, with \(v\wedge v=0\), and extends to exterior powers \(\Lambda^k V\).

#### Term: Symmetric Power
The **symmetric power** \(\mathrm{Sym}^k(V)\) is the quotient of \(V^{\otimes k}\) by swapping factor permutations.

#### Term: Multilinear Map
A map \(T:V_1\times\cdots\times V_k\to W\) is **multilinear** when linear in each argument separately.

#### Term: Tensor Contraction
**Tensor contraction** pairs one covariant and one contravariant index, reducing tensor order by 2.

## Round 14 Supplemental Terms

### Tensor and Factorization Structures

#### Term: Kronecker Product
For matrices \(A\in\mathbb R^{m\times n}\), \(B\in\mathbb R^{p\times q}\),
\(A\otimes B\in\mathbb R^{mp\times nq}\) stacks scaled blocks \(a_{ij}B\).

#### Term: Tensor Rank
The tensor rank of \(\mathcal T\) is the minimum \(r\) such that
\(\mathcal T=\sum_{i=1}^r u_i\otimes v_i\otimes w_i\).

#### Term: Tensor Contraction
Contraction is an index summation reducing order, e.g. from \(T_{ijk}\) to \(A_{jk}=\sum_i T_{ijk}u_i\).

#### Term: Schur Complement
For block matrix
\(\begin{pmatrix}A&B\\C&D\end{pmatrix}\), if \(A\) invertible, then
\(S=D-CA^{-1}B\) is its Schur complement.

#### Term: Moore-Penrose Pseudoinverse
\(A^+\) satisfies \(AA^+A=A\), \(A^+AA^+=A^+\), and minimal-norm least squares properties.

## Round 15 Supplemental Terms

### Operator Algebra and Geometry

#### Term: Polar Decomposition
For bounded operator \(T\),\(T=U|T|\) with partial isometry \(U\) and \(|T|=(T^*T)^{1/2}\).

#### Term: Functional Calculus
For normal operator \(A\), continuous \(f\) defines \(f(A)\) by spectral theorem.

#### Term: C*-Algebra
A Banach *-algebra \(A\) with \(\|a^*a\|=\|a\|^2\) for all \(a\).

#### Term: Polar Decomposition of Matrices
Any matrix \(A\in\mathbb C^{m\times n}\) has \(A=U\Sigma V^*\) (SVD), separating unitary and positive-semidefinite parts.

#### Term: Canonical Polyadic Decomposition
A tensor \(\mathcal T\) is written as a sum of rank-one terms
\(\mathcal T=\sum_{r=1}^R a_r\circ b_r\circ c_r\).

## Round 16 Supplemental Terms

### Structured Transform Methods

#### Term: Polar Decomposition
For any complex matrix \(A\), there exists \(A=UP\) with unitary \(U\) and positive semidefinite \(P=\sqrt{A^*A}\).

#### Term: Functional Calculus (Self-Adjoint)
If \(A\) is symmetric and diagonalizable \(A=Q\Lambda Q^T\), then for polynomial/continuous \(f\),
\(f(A)=Qf(\Lambda)Q^T\).

#### Term: Spectral Radius
\(
\rho(A)=\max\{||:\lambda\in\sigma(A)\}
\)
is the radius of smallest disk centered at origin containing spectrum.

#### Term: Numerical Range
The numerical range of \(A\) is \(W(A)=\{x^*Ax:\|x\|=1\}\), and \(\sigma(A)\subseteq \overline{W(A)}\).

#### Term: Schur Decomposition
Every square \(A\in\mathbb C^{n\times n}\) factors as \(A=UTU^*\) with \(U\) unitary and \(T\) upper triangular.

## Round 17 Supplemental Terms

### Structured Decompositions and Stability

#### Term: QR Decomposition
Any full-rank matrix \(A\in\mathbb R^{m\times n}\) can be written as \(A=QR\) where \(Q\) has orthonormal columns and \(R\) is upper triangular.

#### Term: Cholesky Decomposition
For symmetric positive-definite \(A\), there exists unique lower triangular \(L\) with
\(A=LL^T\).

#### Term: Jordan Canonical Form
For linear map \(T\), there is basis with block-diagonal
\(J\) (Jordan form) such that \(A=PJP^{-1}\).

#### Term: Singular Values
Singular values \(\sigma_i\) are square roots of eigenvalues of \(A^TA\), sorted \(\sigma_1\ge\dots\ge\sigma_r\ge0\).

#### Term: Numerical Condition Number
For invertible \(A\),\(\kappa(A)=\|A\|\,\|A^{-1}\|\), measuring sensitivity of linear solves.

## Round 18 Supplemental Terms

### Geometric and Structured Factorizations

#### Term: Hessenberg Reduction
Any matrix can be orthogonally transformed to Hessenberg form for efficient eigenvalue computation.

#### Term: Bidiagonal Decomposition
Any matrix \(A\) admits \(A=U B V^*\) with orthogonal/unitary \(U,V\) and bidiagonal \(B\) (via Golub-Kahan).

#### Term: Householder Reflection
A Householder matrix has form
\(H=I-2\frac{uu^*}{u^*u}\) and reflects vectors across a hyperplane.

#### Term: Givens Rotation
A Givens rotation
\(
G(i,j,\theta)
\)
annihilates a chosen coordinate while preserving orthogonality via local \(2\times2\) plane rotation.

#### Term: Cosine-Sine Decomposition
For unitary \(U\), CSD writes \(U\) via cosine-sine blocks and principal angles between subspaces.

## Round 19 Supplemental Terms

### Functional-Analytic Spectral Tools

#### Term: Operator Norm
For linear \(T\) on a normed space,
\[
\|T\|=\sup_{\|x\|\le1}\|Tx\|.
\]

#### Term: Resolvent Set
For operator \(A\), resolvent set
\(\rho(A)=\{\lambda\in\mathbb C:(\lambda I-A)^{-1}\text{ exists and bounded}\}\).

#### Term: Spectral Radius Formula
For bounded operator \(A\),
\[
\rho(A)=\lim_{n\to\infty}\|A^n\|^{1/n}.
\]

#### Term: Compact Operator
A bounded linear operator \(K\) is **compact** if it maps unit balls to relatively compact sets.

#### Term: Fredholm Alternative
For compact \(K\), either \((I-K)u=f\) is uniquely solvable for all \(f\), or homogeneous equation has nonzero solutions and compatibility conditions are required.

## Round 20 Supplemental Terms

### Operator Perturbation and Stability

#### Term: Perturbation Theory
For matrix \(A+\epsilon B\), eigenvalues, eigenvectors, and conditionally spectral data vary with \(\epsilon\), with first-order perturbation formulas.

#### Term: Bauer-Fike Bound
If \(A=V\Lambda V^{-1}\), then every eigenvalue \(\mu\) of \(A+E\) satisfies
\(
\min_j|\mu-\lambda_j|\le \kappa(V)\|E\|.
\)

#### Term: Pseudospectrum
The \(\epsilon\)-pseudospectrum is
\(\Lambda_\epsilon(A)=\{z:\|(zI-A)^{-1}\|>\epsilon^{-1}\}\cup\sigma(A)\).

#### Term: Numerical Radius
\(w(A)=\sup_{\|x\|=1}|x^*Ax|\), with \(w(A)\le \|A\|\le2w(A)\).

#### Term: Structured Backward Error
Given \(\widetilde\lambda\), backward error is smallest \(\delta A\) with \(\widetilde\lambda\in\sigma(A+\delta A)\), measuring robustness of computed eigenpairs.

## Round 21 Supplemental Terms

### Subspace Approximation and Geometry

#### Term: Wedin's \(\sin\theta\) Theorem
For perturbed matrix \(A+E\), canonical angles \(\theta_i\) between singular subspaces satisfy bounds by \(\|E\|ig/\delta\), where \(\delta\) is spectral gap.

#### Term: Davis--Kahan Theorem
If \(A\) is symmetric and \(\lambda\) isolated, perturbation of eigenvectors is bounded by
\[
\sin\theta\le \frac{\|E\|_2}{\mathrm{gap}}.
\]

#### Term: Principal Angles
For subspaces \(\mathcal U,\mathcal V\), principal angles \(\theta_i\) are defined recursively via 
\(\cos\theta_i=\max_{u\in\mathcal U,v\in\mathcal V}\langle u,v\rangle\), orthogonalizing each step.

#### Term: Subspace Distance
The distance between subspaces can be measured by
\(
\|P_{\mathcal U}-P_{\mathcal V}\|_2=\sin\theta_{\max}.
\)

#### Term: Procrustes Problem
Given \(A,B\), minimize \(\|A-RB\|_F\) over orthogonal \(R\); solution uses SVD of \(AB^T\).

## Round 22 Supplemental Terms

### Canonical Decompositions and Matrix Geometry

#### Term: Polar Decomposition
Every matrix \(A\in\mathbb C^{m\times n}\) has \(A=UP\), where \(U\) is partial unitary and \(P\) is positive semidefinite.

#### Term: QR Decomposition
A full-rank matrix \(A\in\mathbb R^{m\times n}\) has \(A=QR\), with \(Q^TQ=I\) and \(R\) upper triangular.

#### Term: Schur Decomposition
Over \(\mathbb C\), \(A=QTQ^*\) where \(Q\) is unitary and \(T\) is upper triangular.

#### Term: Jordan Canonical Form
If \(A\) is similar to \(PJP^{-1}\), then \(J\) is block diagonal with Jordan blocks \(J_k(\lambda)\) and \(A\) and \(J\) share characteristic polynomial.

#### Term: Smith Normal Form
For integer matrix \(A\), unimodular \(U,V\) give \(UAV=\mathrm{diag}(d_1,\dots,d_r,0,\dots,0)\) with \(d_i|d_{i+1}\).

## Round 23 Supplemental Terms

### Unitary Orbits and Operator Geometry

#### Term: Unitary Matrix
A matrix \(U\) is unitary if \(U^*U=I\), equivalently \(\|Ux\|_2=\|x\|_2\) for all vectors \(x\).

#### Term: Unitary Congruence
Matrices \(A,B\) are **unitarily congruent** if \(B=U^T A U\) for some unitary \(U\).

#### Term: Unitary Equivalence
Matrices \(A,B\) are unitarily equivalent if \(B=UAU^*\) for some unitary \(U\).

#### Term: Numerical Range
The **numerical range** is \(\{x^*Ax:\|x\|_2=1\}\), a convex subset of \(\mathbb C\).

#### Term: Rayleigh Quotient
For Hermitian \(A\), \(R_A(x)=\frac{x^*Ax}{x^*x}\); its extrema over nonzero \(x\) are min/max eigenvalues.
