# Reading mathematics

The syntax reference. Every symbol you will meet in the first three books, how
to **say it out loud**, and what it does.

Saying it matters. A symbol you cannot pronounce is one you skim, and skimming
notation is how a line becomes unreadable. `$A \in \mathbb{C}^{m \times m}$` is
not decoration — it is a sentence, and it says *A is an m-by-m complex matrix*.

Read one section a day. They repeat forever after.

---

## 1. Belonging, and where a thing lives

| written | say it | means |
|---|---|---|
| $x \in S$ | "x in S", "x is in S" | x is one of the things in the collection S |
| $x \notin S$ | "x not in S" | it is not |
| $A \subset B$ | "A is contained in B" | every element of A is also in B |
| $\mathbb{R}$ | "R", "the reals" | every ordinary number: −2, 0, 1/3, π |
| $\mathbb{C}$ | "C", "the complexes" | numbers with a real and imaginary part, $a + bi$ |
| $\mathbb{Z}$ | "Z", "the integers" | whole numbers, negative ones too |
| $\mathbb{N}$ | "N", "the naturals" | counting numbers, 1, 2, 3 (sometimes from 0) |
| $\emptyset$ | "the empty set" | nothing in it |
| $\{x : P(x)\}$ | "the set of x such that P of x" | everything satisfying the condition after the colon |

The colon or vertical bar inside braces is always **"such that"**.
$\{x \in \mathbb{R} : x > 0\}$ is "the set of real x such that x is positive" —
the positive reals.

**A superscript on a set is a shape.**

| written | say it | means |
|---|---|---|
| $\mathbb{R}^n$ | "R n" | a list of n real numbers. A point, or a vector |
| $\mathbb{R}^{m \times n}$ | "R m by n" | a table of real numbers, m rows and n columns. A matrix |
| $\mathbb{C}^{m \times m}$ | "C m by m" | a square complex matrix |

So `$A \in \mathbb{C}^{m \times m}$` reads: **A is a square complex matrix with
m rows.** That is all it says. It appears in almost every Trefethen exercise.

---

## 2. Naming a piece of something

| written | say it | means |
|---|---|---|
| $a_{ij}$ | "a i j", "a sub i j" | the entry of A in row i, column j |
| $x_k$ | "x k", "x sub k" | the k-th thing in a list |
| $A_{1:k,1:k}$ | "A one to k, one to k" | the top-left k-by-k block of A |
| $A^T$ | "A transpose" | flip rows and columns |
| $A^*$ | "A star", "A adjoint" | transpose, and flip the sign of the imaginary parts |
| $A^{-1}$ | "A inverse" | the matrix that undoes A |
| $\|x\|$ | "norm of x", "the norm" | how big x is. One number, never negative |
| $\langle x, y \rangle$ | "inner product of x and y" | how much x and y point the same way |
| $\det A$ | "determinant of A" | one number; zero exactly when A is not invertible |

**Subscripts name, superscripts transform.** `$a_{ij}$` picks an entry out;
`$A^T$` makes a new matrix. Where a superscript is a plain number it is usually
a power — but `$A^{-1}$` is not "one over A", it is a specific matrix.

---

## 3. Sizes, distances and comparison

| written | say it | means |
|---|---|---|
| $\|i - j\| > p$ | "the absolute value of i minus j is greater than p" | i and j are more than p apart |
| $\leq$, $\geq$ | "less than or equal to", "greater than or equal" | |
| $\approx$ | "is approximately" | |
| $\neq$ | "is not equal to" | |
| $\epsilon$ | "epsilon" | a small positive number. Almost always "as small as you like" |
| $\delta$ | "delta" | another small one, usually the one you get to choose |
| $\infty$ | "infinity" | not a number; a direction |

`$\epsilon$` and `$\delta$` travel together and they have a fixed meaning:
**someone challenges you with an ε, you answer with a δ.** Every definition of a
limit, a continuous function, or a convergent sequence is that exchange.

---

## 4. Doing something to everything

| written | say it | means |
|---|---|---|
| $\sum_{i=1}^{n} x_i$ | "sum from i equals 1 to n of x i" | add them all up |
| $\prod_{i=1}^{n} x_i$ | "product from i equals 1 to n" | multiply them all |
| $\int_a^b f(x)\,dx$ | "the integral from a to b of f of x d x" | the area under f between a and b |
| $\lim_{n \to \infty} x_n$ | "the limit as n goes to infinity of x n" | what the list settles down to |
| $\sup$, $\inf$ | "soup", "inf" | the least upper bound, the greatest lower bound |
| $\max$, $\min$ | "max", "min" | the largest, the smallest |

`$\sup$` is short for *supremum* and everyone says "soup". It is the smallest
number nothing exceeds — the same as the maximum when a maximum exists, and it
exists in cases where a maximum does not.

---

## 5. Functions and arrows

| written | say it | means |
|---|---|---|
| $f : A \to B$ | "f from A to B" | f takes things in A and returns things in B |
| $x \mapsto x^2$ | "x maps to x squared" | the rule itself, without naming it |
| $f \circ g$ | "f composed with g", "f after g" | do g first, then f |
| $\Rightarrow$ | "implies" | if the left is true, so is the right |
| $\Leftrightarrow$, "iff" | "if and only if" | each implies the other. Both directions |
| $\forall$ | "for all", "for every" | |
| $\exists$ | "there exists" | |

**The two arrows are different.** `$\to$` in `$f : A \to B$` names the source and
target — the types. `$\mapsto$` gives the rule. Together:
`$f : \mathbb{R} \to \mathbb{R}, \; x \mapsto x^2$`.

**"If and only if" means two proofs.** When an exercise says *show that A holds
if and only if B*, it is two jobs: A implies B, and B implies A.

---

## 6. Words that are really instructions

These are not symbols but they are just as much syntax, and each one tells you
what shape your work has to take.

| the words | what they are asking for |
|---|---|
| "Show that", "Prove that" | an argument, not a number. **The exercises this directory uses are the other kind** |
| "Determine", "Compute", "Find" | a number, a matrix, or a formula |
| "Verify that" | you are given the answer; confirm it |
| "What can you say about" | an open question. State a property and justify it |
| "Suppose", "Let" | an assumption you are handed. Everything after depends on it |
| "such that", "s.t." | the condition that follows restricts what came before |
| "without loss of generality", "WLOG" | the other cases work the same way, by symmetry |
| "It follows that", "Hence", "Thus" | the next line is a consequence of the last |
| "Conversely" | now the other direction |
| "nonsingular", "invertible" | $A^{-1}$ exists. $\det A \neq 0$. Same thing said three ways |

**"Suppose A satisfies the condition of Exercise 20.1"** means go back and read
20.1. The books do this constantly and it is not optional — the condition is
usually what makes the method work at all.

---

## 7. The words for a matrix's behaviour

The vocabulary of the first three books, which the exercises assume.

| word | what it means, plainly |
|---|---|
| **singular** | has no inverse. Squashes some direction to nothing |
| **nonsingular** | has an inverse |
| **banded**, bandwidth | the nonzeros sit in a stripe along the diagonal |
| **sparse** | mostly zeros |
| **symmetric** | $A = A^T$. Mirror-image across the diagonal |
| **orthogonal** | its columns are perpendicular and length 1. It rotates without stretching |
| **rank** | how many genuinely independent directions the columns span |
| **eigenvalue** | a number λ where $Ax = \lambda x$ — A only stretches x, never turns it |
| **condition number** | how much the answer can move when the question moves a little. Big means fragile |
| **residual** | $b - Ax$. What is left over. Small residual means the equation nearly holds |
| **factorization** | writing A as a product of simpler matrices, like $A = LU$ |
| **least squares** | no exact answer exists, so find the one with the smallest residual |

**Condition number is the one to learn first.** It is the difference between
*the equation holds* and *the answer is right*, and it decides the tolerance on
half the checks in this directory.

---

## How to use this

Read one numbered section a day. Seven days covers all of it, and then it is
repetition rather than learning.

When a page's **Notation** table has a symbol this file does not, add it here.
The file is meant to grow.
