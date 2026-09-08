# Discrete Math

## Subcategory Map

1. Logic and Proof
   - Basic Connectives
   - Proof Systems
2. Set Theory Foundations
   - Set Operations
   - Relations and Functions
3. Counting and Combinatorics
   - Permutation and Combination
   - Inclusion-Exclusion and Recurrence Counting
4. Graph Theory
   - Basic Objects
   - Traversal and Eulerian Ideas
5. Number Theory
   - Divisibility
   - Modular Arithmetic
6. Recursion and Sequences
   - Recurrences
   - Mathematical Induction
7. Algorithms and Complexity
   - Complexity Classes
   - Growth Bounds
8. Boolean Algebra and Logic Circuits
   - Boolean Operations
   - Propositional Forms

## Table of Contents

1. [Logic and Proof](#logic-and-proof)
   1. [Basic Connectives](#basic-connectives)
   2. [Proof Systems](#proof-systems)
2. [Set Theory Foundations](#set-theory-foundations)
   1. [Set Operations](#set-operations)
   2. [Relations and Functions](#relations-and-functions)
3. [Counting and Combinatorics](#counting-and-combinatorics)
   1. [Permutation and Combination](#permutation-and-combination)
   2. [Inclusion-Exclusion and Recurrence Counting](#inclusion-exclusion-and-recurrence-counting)
4. [Graph Theory](#graph-theory)
   1. [Basic Objects](#basic-objects)
   2. [Traversal and Eulerian Ideas](#traversal-and-eulerian-ideas)
5. [Number Theory](#number-theory)
   1. [Divisibility](#divisibility)
   2. [Modular Arithmetic](#modular-arithmetic)
6. [Recursion and Sequences](#recursion-and-sequences)
   1. [Recurrences](#recurrences)
   2. [Mathematical Induction](#mathematical-induction)
7. [Algorithms and Complexity](#algorithms-and-complexity)
   1. [Complexity Classes](#complexity-classes)
   2. [Growth Bounds](#growth-bounds)
8. [Boolean Algebra and Logic Circuits](#boolean-algebra-and-logic-circuits)
   1. [Boolean Operations](#boolean-operations)
   2. [Propositional Forms](#propositional-forms)
9. [Round 6 Supplemental Terms](#round-6-supplemental-terms)
   1. [Discrete Extensions](#discrete-extensions)
10. [Round 7 Supplemental Terms](#round-7-supplemental-terms)
   1. [Advanced Discrete Structures](#advanced-discrete-structures)
11. [Round 8 Supplemental Terms](#round-8-supplemental-terms)
   1. [Computability and Formal Systems](#computability-and-formal-systems)
12. [Round 9 Supplemental Terms](#round-9-supplemental-terms)
   1. [Advanced Counting Structures](#advanced-counting-structures)
13. [Round 11 Supplemental Terms](#round-11-supplemental-terms)
   1. [Randomized and Finite Methodologies](#randomized-and-finite-methodologies)
14. [Round 12 Supplemental Terms](#round-12-supplemental-terms)
   1. [Advanced Counting and Complexity](#advanced-counting-and-complexity)
15. [Round 13 Supplemental Terms](#round-13-supplemental-terms)
   1. [Advanced Probabilistic Counting](#advanced-probabilistic-counting)
- [Round 14 Supplemental Terms](#round-14-supplemental-terms)
   1. [Probabilistic Method in Discrete Math](#probabilistic-method-in-discrete-math)
- [Round 15 Supplemental Terms](#round-15-supplemental-terms)
   1. [Randomized Algorithmic Counting](#randomized-algorithmic-counting)
- [Round 16 Supplemental Terms](#round-16-supplemental-terms)
   1. [Extremal and Probabilistic Structures](#extremal-and-probabilistic-structures)
- [Round 17 Supplemental Terms](#round-17-supplemental-terms)
   1. [Martingale Concentration Toolkit](#martingale-concentration-toolkit)
- [Round 18 Supplemental Terms](#round-18-supplemental-terms)
   1. [Large Deviations and Random Structures](#large-deviations-and-random-structures)
- [Round 19 Supplemental Terms](#round-19-supplemental-terms)
   1. [Combinatorial Probability at Scale](#combinatorial-probability-at-scale)
- [Round 20 Supplemental Terms](#round-20-supplemental-terms)
   1. [Derandomization and Pseudorandomness](#derandomization-and-pseudorandomness)
- [Round 21 Supplemental Terms](#round-21-supplemental-terms)
   1. [Ramsey and Extremal Random Structures](#ramsey-and-extremal-random-structures)
- [Round 22 Supplemental Terms](#round-22-supplemental-terms)
   1. [Extremal Combinatorial Theorems](#extremal-combinatorial-theorems)
- [Round 23 Supplemental Terms](#round-23-supplemental-terms)
   1. [Advanced Extremal and Probabilistic Tools](#advanced-extremal-and-probabilistic-tools)

## Logic and Proof

### Basic Connectives

#### Term: Proposition
A truth-valued statement.

#### Term: Negation
\(\neg p\) reverses the truth of \(p\).

#### Term: Conjunction
\(p\wedge q\) is true iff both \(p\) and \(q\) are true.

### Proof Systems

#### Term: Logical Equivalence
\(P\iff Q\).

#### Term: Proof by Contradiction
Assume negation, derive contradiction, conclude statement.

## Set Theory Foundations

### Set Operations

#### Term: Union
\(A\cup B=\{x\mid x\in A\lor x\in B\}\).

#### Term: Intersection
\(A\cap B=\{x\mid x\in A\land x\in B\}\).

#### Term: Cartesian Product
\(A\times B=\{(a,b):a\in A,b\in B\}\).

### Relations and Functions

#### Term: Equivalence Relation
Reflexive, symmetric, transitive.

#### Term: Function
A special relation assigning one codomain element to each domain element.

#### Term: Bijection
Injective and surjective function.

## Counting and Combinatorics

### Permutation and Combination

#### Term: Binomial Coefficient
\({n\choose k}=\frac{n!}{k!(n-k)!}\).

#### Term: Permutation
\(P(n,k)=\frac{n!}{(n-k)!}\).

#### Term: Combination with Repetition
\({n+k-1\choose k}\) gives multisets.

### Inclusion-Exclusion and Recurrence Counting

#### Term: Factorial
\(n!=n(n-1)\cdots1,\;0!=1\).

#### Term: Inclusion-Exclusion Principle
\(|A\cup B|=|A|+|B|-|A\cap B|\).

#### Term: Catalan Count (Contextual)
\(C_n=\frac1{n+1}{2n\choose n}\) counts balanced parenthesis strings.

## Graph Theory

### Basic Objects

#### Term: Graph
\((V,E)\) with vertices and edges.

#### Term: Subgraph
A graph formed by subsets of vertices and edges.

### Traversal and Eulerian Ideas

#### Term: Eulerian Path
Path using each edge exactly once.

#### Term: Hamiltonian Cycle
Cycle visiting every vertex exactly once.

## Number Theory

### Divisibility

#### Term: Divisibility
\(a\mid b\iff \exists k\in\mathbb Z: b=ka\).

#### Term: Prime Factorization
Every \(n>1\) factors uniquely into primes up to order.

### Modular Arithmetic

#### Term: Congruence
\(a\equiv b\pmod n\iff n\mid(a-b)\).

#### Term: Euler's Totient
\(\varphi(n)\) counts \(1\le k\le n\) coprime to \(n\).

## Recursion and Sequences

### Recurrences

#### Term: Recurrence Relation
\(a_n=a_{n-1}+a_{n-2}\) is a sample second-order recurrence.

#### Term: Linear Homogeneous Recurrence
Form \(a_n=c_1a_{n-1}+\cdots+c_k a_{n-k}\).

### Mathematical Induction

#### Term: Strong Induction
Assume statement true for all \(k\le n\) to prove for \(n+1\).

#### Term: Induction on Two Parameters
Nested base and step proving on pairs \((m,n)\).

## Algorithms and Complexity

### Complexity Classes

#### Term: Big-O
\(f(n)=O(g(n))\).

#### Term: Theta and Omega
\(f=\Theta(g)\) and \(f=\Omega(g)\) denote tight/lower bounds.

### Growth Bounds

#### Term: Polynomial Time
\(T(n)=O(n^k)\) for some \(k\).

#### Term: Exponential Time
\(T(n)=O(a^n)\), typically much slower than polynomial.

## Boolean Algebra and Logic Circuits

### Boolean Operations

#### Term: Disjunction
\(p\vee q\).

#### Term: Exclusive Or
\(p\oplus q\) true exactly when exactly one is true.

### Propositional Forms

#### Term: Implication
\(p\to q\) is false only when \(p\) true and \(q\) false.

#### Term: Biconditional
\(p\iff q\).

## Formal Theorems

### Theorem (Pigeonhole Principle)
If \(n\) objects are placed into \(m\) boxes and \(n>m\), then at least one box contains at least two objects.

### Theorem (Inclusion–Exclusion for Two Sets)
For finite sets \(A,B\):
\[
|A\cup B|=|A|+|B|-|A\cap B|.
\]

### Theorem (Euler's Theorem in Number Theory)
If \(\gcd(a,n)=1\), then
\[
a^{\varphi(n)}\equiv 1\pmod n,
\]
where \(\varphi\) is Euler's totient.

### Theorem (Handshake Lemma)
In any finite graph, sum of degrees equals twice number of edges:
\[
\sum_{v\in V}\deg(v)=2|E|.
\]

## Worked Examples

### Example (Pigeonhole Application)
Among 13 integers, at least two have the same remainder when divided by 12.

### Example (Use Inclusion–Exclusion)
Let \(|A|=10\), \(|B|=8\), \(|A\cap B|=3\). Then
\[
|A\cup B|=10+8-3=15.
\]

### Example (Euler Congruence)
Take \(a=3,n=10\): \(\varphi(10)=4\),
\[
3^4=81\equiv 1\pmod{10}.
\]

### Example (Handshaking Count)
For triangle graph with \(3\) edges,
\[\deg(v_1)=\deg(v_2)=\deg(v_3)=2\]
so \(\sum\deg=6=2|E|\) consistent with theorem.

## Worked Examples (Q to A Mini-Proofs)

### Q1. Apply the Pigeonhole Principle to 13 numbers into 12 remainder classes mod 12.

**Answer.** The remainders mod 12 are \(0,1,\dots,11\): 12 boxes. Placing 13 integers into 12 boxes, by pigeonhole at least one box has at least two integers. Hence two integers have the same remainder mod 12.

### Q2. Compute \(|A\cup B|\) from \(|A|=10\), \(|B|=8\), \(|A\cap B|=3\).

**Answer.** Use inclusion-exclusion:
\[
|A\cup B|=|A|+|B|-|A\cap B|=10+8-3=15.
\]
So the union has 15 distinct elements.

### Q3. Verify Euler theorem for \(a=3, n=10\).

**Answer.** First \(\gcd(3,10)=1\), so theorem applies. Compute \(\varphi(10)=4\). Then
\[
3^4=81\equiv1\pmod{10},
\]
since \(81-1=80\) is divisible by 10.

### Q4. Check handshake lemma on triangle graph.

**Answer.** A triangle has \(3\) edges. Each edge contributes degree \(+2\) total across endpoints, so total degree should be \(2\cdot3=6\).
Directly, each of 3 vertices has degree 2:
\[
\sum_{v\in V}\deg(v)=2+2+2=6.
\]
Both sides match, so lemma holds.

## Additional Mini-Proof Examples (Round 3)

### Q5. Prove inclusion-exclusion for finite sets with \(A\cap B=\varnothing\).

**Answer.** If \(A\cap B=\varnothing\), then \(|A\cap B|=0\). Substituting into inclusion-exclusion:
\[
|A\cup B|=|A|+|B|-|A\cap B|=|A|+|B|-0=|A|+|B|,
\]
which is exactly disjoint union cardinality.

### Q6. Show there is no graph on 3 vertices with 4 edges.

**Answer.** A simple graph with 3 vertices has at most \(\binom{3}{2}=3\) edges (all pairs).
So 4 edges is impossible because that would require multiple edges between some vertex pair, i.e., not simple. Hence no such simple graph.

### Q7. Compute \(5\)th Fibonacci number by recurrence.

**Answer.** Using \(F_0=0,F_1=1\),
\[
F_2=1,\;F_3=2,\;F_4=3,\;F_5=5.
\]
So \(F_5=5\).

### Q8. Prove \(\sum_{v\in V}\deg(v)=2|E|\) for square graph.

**Answer.** A square has 4 vertices and 4 edges arranged in cycle length 4.
Each edge has exactly 2 endpoints; counting incidences by edges gives 8.
Each vertex degree is 2, so total from vertices is
\[
\sum_{v\in V}\deg(v)=2+2+2+2=8.
\]
Both counts match, proving the lemma in this case.

## Formal Theorems (Round 4: Standard Statement Pack)

### Theorem (Fermat's Little Theorem)
If \(p\) is prime and \(a\in\mathbb Z\), then
\[
a^p\equiv a\pmod p.
\]
If additionally \(\gcd(a,p)=1\), then \(a^{p-1}\equiv 1\pmod p\).

### Theorem (Euler-Fermat Theorem)
If \(\gcd(a,n)=1\), then
\[
a^{\varphi(n)}\equiv 1\pmod n,
\]
where \(\varphi\) is Euler's totient.

### Theorem (Dirichlet's Principle / Pigeonhole in General Form)
If \(N\) objects are placed into \(k\) boxes, then some box contains at least
\(\left\lceil \frac{N}{k} \right\rceil\) objects.

### Theorem (Eulerian Path Criterion for Finite Connected Graphs)
A finite connected graph has an Eulerian trail iff it has 0 or 2 vertices of odd degree.
It has an Eulerian circuit iff all vertex degrees are even.

### Theorem (Hall's Marriage Theorem)
For bipartite graph \((L\cup R,E)\), there is a matching that saturates \(L\) iff for every subset \(S\subseteq L\),
\[
|N(S)|\ge |S|,
\]
where \(N(S)\) is the neighborhood of \(S\).

## Round 4 Theorem Proof Sketches

### Q1. Fermat's Little Theorem (Proof idea)
If \(p\) is prime and \(\gcd(a,p)=1\), then multiplication by \(a\) permutes residue classes \(\{1,2,\dots,p-1\}\), so
\[
\prod_{k=1}^{p-1}k \equiv \prod_{k=1}^{p-1}ak = a^{p-1}(p-1)! \pmod p.
\]
Since \((p-1)!\) has inverse mod \(p\), get \(a^{p-1}\equiv1\pmod p\). Multiplying by \(a\) gives \(a^p\equiv a\pmod p\).

### Q2. Euler-Fermat (Proof idea)
Consider reduced residue system mod \(n\), numbers \(a_1,\dots,a_{\varphi(n)}\) coprime to \(n\). Multiply by \(a\) with \(\gcd(a,n)=1\): residues permute the same set modulo \(n\).
Hence
\[
a_1a_2\cdots a_{\varphi(n)}\equiv (a a_1)(a a_2)\cdots(a a_{\varphi(n)})=a^{\varphi(n)}\prod_{i}a_i \pmod n.
\]
Cancel product to conclude \(a^{\varphi(n)}\equiv1\pmod n\).

### Q3. Dirichlet Principle (Proof idea)
Assume every box has at most \(\lceil N/k\rceil-1\). Then total objects \(<k\cdot\lceil N/k\rceil\). If \(N\) does not divide \(k\), this still can be too small only if all boxes satisfy this strict upper bound; but by definition of ceiling, this contradicts pigeonhole count. Therefore some box has at least \(\lceil N/k\rceil\).

### Q4. Eulerian Trail Criterion (Proof idea)
Each edge contributes 2 to total degree. In a trail that uses all edges exactly once, every time edges enter a vertex there is a matching exit, except possibly endpoints. Thus odd-degree vertices can only be 0 (closed circuit) or 2 (open trail). Connectivity and these degree conditions are also sufficient by repeatedly following unused edges and splicing cycles.

### Q5. Hall's Marriage Theorem (Forward direction idea)
Assume a perfect matching saturating \(L\) exists. Then each \(v\in S\subseteq L\) matches to a distinct vertex in \(N(S)\), so \(|N(S)|\ge|S|\).
The reverse direction (sufficiency) constructs matching via augmenting-path argument or network flow; under \(|N(S)|\ge|S|\) for all \(S\), no blocking sets remain, so augmenting algorithm finds full matching.

## Round 5 Supplemental Terms

### Advanced Discrete Topics

#### Term: Bipartite Graph

a graph \\((V,E)\\) with partition \\(V=A\cup B\), no edge between two vertices in same part.

#### Term: Generating Function

a formal power series \\(
G(x)=\sum_{n\ge0}a_nx^n
\\) encoding a sequence \(a_n\).

#### Term: Topological Order

a linear extension of a DAG order where every directed edge \\(u\to v\\) satisfies \\(u\) before \\(v\).

#### Term: Eulerian Circuit

a trail in a connected graph that uses every edge exactly once and starts/ends at same vertex.

#### Term: Strongly Connected

a directed graph in which every vertex is reachable from every other via directed paths.

## Round 6 Supplemental Terms

### Discrete Extensions

#### Term: Tree

a connected graph with no cycles.

#### Term: Spanning Tree

a subgraph containing all vertices, connected, and cycle-free.

#### Term: Isomorphic Graphs

graphs \(G\) and \(H\) are isomorphic if there exists a bijection of vertices preserving adjacency.

#### Term: Chromatic Number

the smallest \(k\) such that a graph is properly vertex-colorable with \(k\) colors.

#### Term: Finite Automaton

a 5-tuple \((Q,\Sigma,\delta,q_0,F)\) recognizing regular languages.

## Round 7 Supplemental Terms

### Advanced Discrete Structures

#### Term: Planar Graph

A graph that can be drawn in the plane with edges intersecting only at shared vertices.

#### Term: Directed Acyclic Graph

A directed graph with no directed cycles.

#### Term: Topological Sort

A linear order of vertices in a DAG where every directed edge \(u\to v\) has \(u\) before \(v\).

#### Term: Pigeonhole Principle

With \(n+1\) objects and \(n\) boxes, at least one box contains at least two objects.

#### Term: Regular Language

The set of strings recognized by some finite automaton.

## Round 8 Supplemental Terms

### Computability and Formal Systems

#### Term: Generating Function
A **generating function** encodes sequence 


$(a_n)$


as 


$G(x)=\sum_{n\ge0}a_nx^n$.

#### Term: Finite Automaton
A **finite automaton** is a 5-tuple 


$(Q,\Sigma,\delta,q_0,F)$ with finite states 


$Q$, alphabet 


$\Sigma$, transition function 


$\delta$, start 


$\delta,q_0$, and accepting set 


$F$.

#### Term: Regular Language
A language is **regular** iff recognized by a finite automaton.

#### Term: Context-Free Language
A language is **context-free** if it is generated by a context-free grammar 


$G=(V,\Sigma,R,S)$.

#### Term: NP-Complete
A decision problem is **NP-complete** when it is in 


$\text{NP}$


and every problem in 


$\text{NP}$


reduces to it in polynomial time.

## Round 9 Supplemental Terms

### Advanced Counting Structures

#### Term: Burnside's Lemma
For finite group action 


$G\curvearrowright X$,


the number of orbits is


\[
|X/G|=\frac1{|G|}\sum_{g\in G}|\mathrm{Fix}(g)|.

\]

#### Term: Pólya Enumeration
Pólya enumeration uses cycle index polynomials to count distinct colorings under symmetries.

#### Term: Möbius Inversion
For arithmetic functions 


$f,g$


with 


$f(n)=\sum_{d\mid n}g(d)$, we have


\[

g(n)=\sum_{d\mid n}\mu(d)f(n/d).

\]

#### Term: Inclusion–Exclusion for n Sets
For finite sets 


$A_1,\dots,A_n$,



\[
\left|\bigcup_{i=1}^n A_i\right|=\sum_{k\ge1}(-1)^{k+1}\sum_{1\le i_1<\cdots<i_k\le n}|A_{i_1}\cap\cdots\cap A_{i_k}|.

\]

#### Term: Catalan Numbers
The 


$n$th 


Catalan number


\(C_n\)


counts balanced combinatorial structures and is


\[


C_n=\frac{1}{n+1}{2n\choose n},\quad n\ge0.

\]

## Round 10 Supplemental Terms

### Finite Construction Systems

#### Term: Prüfer Sequence
A **Prüfer sequence** is a length-


$(n-2)$


sequence of labels from 


$\{1,\dots,n\}$


encoding a labeled tree on 


$n\) vertices.

#### Term: Cayley Formula
The number of labeled trees on 


$n$ 


vertices is 


$n^{n-2}$.

#### Term: Planar Graph
A graph is **planar** if it can be embedded in the plane without edge crossings.

#### Term: Euler Characteristic
For connected planar graph, 


$V-E+F=2$.

#### Term: Graph Coloring
A **proper coloring** of graph 


$G=(V,E)$


assigns colors to vertices so adjacent vertices differ.


## Round 11 Supplemental Terms

### Randomized and Finite Methodologies

#### Term: Random Graph
A **random graph** on 


$n$ vertices, typically denoted 


$G(n,p)$, has each edge present independently with probability 


$p$.

#### Term: Expectation Linearity
For random variables 


$X_i$, 


\(\mathbb E[\sum_i X_i]=\sum_i\mathbb EX_i\), regardless of dependence.

#### Term: Markov Chain
A **Markov chain** is a stochastic process with transition probabilities 


$P(X_{t+1}=j\mid X_t=i)=p_{ij}$.

#### Term: Random Walk on a Graph
A random walk on graph 


$G=(V,E)$ updates current vertex by choosing random neighboring vertices via transition matrix.

#### Term: Monte Carlo Method
A **Monte Carlo method** uses repeated random sampling to estimate quantities 


\(\mathbb E[f(X)]\n\approx \frac{1}{N}\sum_{k=1}^N f(X_k)\).


## Round 12 Supplemental Terms

### Advanced Counting and Complexity

#### Term: Big-Oh Complexity
\(f(n)=O(g(n))\) means there exist \(C,n_0>0\) such that \(f(n)\le C g(n)\) for all \(n\ge n_0\).

#### Term: Polynomial Reduction
A problem \(A\) reduces to \(B\) in polynomial time if an instance of \(A\) can be mapped to one of \(B\) by a polynomial-time computable function preserving yes/no answers.

#### Term: NP-Intermediate
A language is in **NP-intermediate** if it lies in \(\mathrm{NP}\) but is neither in \(\mathrm{P}\) nor NP-complete (assuming \(\mathrm{P}\neq\mathrm{NP}\)).

#### Term: Counting Problem #P
Class \(#\mathrm P\) contains counts of accepting paths of NP machines; e.g., number of satisfying assignments of a Boolean formula.

#### Term: Randomized Approximation Scheme
A **FPRAS** is a randomized algorithm giving \((1\pm\epsilon)\)-approximation in time polynomial in input size and \(1/\epsilon\).

## Round 13 Supplemental Terms

### Advanced Probabilistic Counting

#### Term: Union Bound
For events \(A_1,\dots,A_n\),
\[
\Pr\left(\bigcup_{i=1}^n A_i\right)\le\sum_{i=1}^n\Pr(A_i).
\]

#### Term: Linearity of Expectation
For random variables \(X_i\),
\[
\mathbb E\left[\sum_i X_i\right]=\sum_i\mathbb E[X_i].
\]

#### Term: Markov Inequality
If \(X\ge0\) then for \(a>0\),
\[
\Pr(X\ge a)\le \frac{\mathbb E[X]}a.
\]

#### Term: Chebyshev Inequality
For \(\sigma^2=\operatorname{Var}(X)\) and \(k>0\),
\[
\Pr(|X-\mu|\ge k\sigma)\le\frac1{k^2}.
\]

#### Term: Martingale
A sequence \((X_n,\mathcal F_n)\) is a **martingale** if \(\mathbb E[|X_n|]<\infty\), is adapted, and
\[
\mathbb E[X_{n+1}\mid\mathcal F_n]=X_n.
\]

## Round 14 Supplemental Terms

### Probabilistic Method in Discrete Math

#### Term: Linearity of Expectation
For random variables \(X_1,\dots,X_n\),
\[
E\left[\sum_{i=1}^n X_i\right]=\sum_{i=1}^n E[X_i].
\]
No independence assumption is needed.

#### Term: Union Bound
For events \(E_1,\dots,E_m\),
\[\Pr\left(\bigcup_{i=1}^m E_i\right)\le\sum_{i=1}^m\Pr(E_i).\]

#### Term: Lovász Local Lemma
If events are sparse-dependent and \(P(E_i)\le p\) with neighborhood size \(d\), then under suitable conditions, all events can avoid simultaneously with nonzero probability.

#### Term: Randomized Rounding
Given fractional solution \(x\in[0,1]^n\), convert to integral vector by setting each bit \(X_i=1\) with probability \(x_i\).

#### Term: Probabilistic Existence
To prove existence of a combinatorial object, show 
\(P(\text{bad event})<1\), so a good object must exist.

## Round 15 Supplemental Terms

### Randomized Algorithmic Counting

#### Term: Linearity of Variance
For pairwise uncorrelated variables,
\[
\mathrm{Var}\left(\sum_i X_i\right)=\sum_i \mathrm{Var}(X_i).
\]

#### Term: Chernoff Bound
For \(X=\sum_i X_i\) Bernoulli-sum and \(\delta>0\),
\[
\Pr[X>(1+\delta)\mathbb E[X]]\le \exp\!\left(-\frac{\delta^2\mathbb E[X]}{2+\delta}\right).
\]

#### Term: Randomized Algorithm
An algorithm that makes random choices internally and succeeds with probability at least \(p>0\) (repeat to amplify success).

#### Term: Probabilistic Method
To prove existence, show that \(\Pr(\text{failure})<1\), then a favorable object must exist.

#### Term: Monte Carlo vs Las Vegas
Monte Carlo algorithms have bounded runtime with possible error; Las Vegas algorithms are exact when they terminate.

## Round 16 Supplemental Terms

### Extremal and Probabilistic Structures

#### Term: Markov's Inequality
For nonnegative \(X\) and \(a>0\),
\[
\Pr(X\ge a)\le \frac{\mathbb E[X]}{a}.
\]

#### Term: Cantelli's Inequality
If \(\mathbb E[X]=0\) and \(\sigma^2=\mathrm{Var}(X)\), then for \(t>0\),
\[
\Pr(X\ge t)\le \frac{\sigma^2}{\sigma^2+t^2}.
\]

#### Term: Janson Inequality
An upper-tail bound for sums of dependent Bernoulli variables via a dependency graph parameter \(\Delta\).

#### Term: Entropy Method in Combinatorics
Use entropy \(H(X)\) and subadditivity to bound numbers of combinatorial configurations.

#### Term: Concentration of Measure
Lipschitz functions on high-dimensional products are tightly concentrated around expectation, often with Gaussian-type tails.

## Round 17 Supplemental Terms

### Martingale Concentration Toolkit

#### Term: Martingale
A sequence \((X_k)\) is a martingale wrt filtration \((\mathcal F_k)\) if \(E[|X_k|]<\infty\), \(X_k\) is \(\mathcal F_k\)-measurable, and \(E[X_{k+1}\mid\mathcal F_k]=X_k\).

#### Term: Azuma--Hoeffding Inequality
If \(X_k\) has bounded differences \(|X_k-X_{k-1}|\le c_k\), then
\[
\Pr(|X_n-X_0|\ge t)\le 2\exp\left(-\frac{t^2}{2\sum_{k=1}^n c_k^2}\right).
\]

#### Term: Doob Decomposition
Any submartingale \(X_n\) can be uniquely decomposed as martingale part plus predictable increasing process.

#### Term: Optional Stopping
Under regularity, for martingale \(M_n\) and stopping time \(\tau\),
\(E[M_\tau]=E[M_0]\).

#### Term: McDiarmid's Inequality
For function with bounded coordinate sensitivity \(c_i\):
\[
\Pr(f- E[f]\ge t)\le \exp\left(-\frac{2t^2}{\sum c_i^2}\right).
\]

## Round 18 Supplemental Terms

### Large Deviations and Random Structures

#### Term: Large Deviation Principle
A family\(X_n\) satisfies LDP with rate function \(I\) if
\[
\Pr(X_n\in A)\approx \exp\{-n\inf_{x\in A}I(x)\}.
\]

#### Term: Cramér's Theorem
For i.i.d. \(X_i\), empirical means satisfy LDP with Legendre transform of log-mgf as rate function.

#### Term: Chernoff Bound (Moment Form)
For \(\lambda>0\),
\[
\Pr(X\ge t)\le\inf_{\lambda>0} e^{-\lambda t}E[e^{\lambda X}].
\]

#### Term: Vapnik-Chervonenkis Dimension
\(\mathrm{VC}(\mathcal H)=\) largest shattered finite set size; controls uniform convergence rates.

#### Term: Sanov's Theorem
For empirical measure \(\hat\mu_n\) of i.i.d. samples, deviations obey rate \(D(\nu\|\mu)\), the KL-divergence.

## Round 19 Supplemental Terms

### Combinatorial Probability at Scale

#### Term: Bounded Differences Inequality
If \(X=f(X_1,\dots,X_n)\) with \(|X(X_1,\dots,x_i,\dots)-X(X_1,\dots,x'_i,\dots)|\le c_i\),
then
\[
\Pr\{|X-E[X]|\ge t\}\le2\exp\left(-\frac{2t^2}{\sum c_i^2}\right).
\]

#### Term: Paley--Zygmund Inequality
For nonnegative \(X\) and \(\theta\in(0,1)\),
\[
\Pr(X\ge\theta E[X])\ge (1-\theta)^2\frac{E[X]^2}{E[X^2]}.
\]

#### Term: Efron--Stein Inequality
For \(X= f(X_1,\dots,X_n)\),
\[
\mathrm{Var}(X)\le \frac12\sum_{i=1}^n E\big[(X-X^{(i)})^2\big],
\]
where \(X^{(i)}\) replaces only coordinate \(i\).

#### Term: Talagrand Convex Distance
For set \(A\subseteq\Omega^n\), Talagrand distance lower-bounds concentration via convex functional on Hamming perturbations.

#### Term: Russo--Margulis Formula
For monotone boolean\(f\),
\[
\frac{d}{dp}E_p[f]=\sum_i \Pr_p[\partial_i f=1],
\]
where \(\partial_i f\) is edge influence at coordinate \(i\).

## Round 20 Supplemental Terms

### Derandomization and Pseudorandomness

#### Term: Method of Conditional Expectation
Given random variable \(X\), one derandomizes by fixing variables one-by-one while preserving \(E[X]\), yielding a deterministic object.

#### Term: Pairwise Independent Spaces
A distribution is pairwise independent if every pair of coordinates is independent, a weaker notion useful in pseudorandom constructions.

#### Term: Nisan-Wigderson Generator
A pseudorandom generator based on hard functions expands short seeds to fool small circuits.

#### Term: k-Wise Independence
A family is \(k\)-wise independent if every subset of \(k\) variables has joint distribution equal to product.

#### Term: Small-Bias Spaces
A \(\varepsilon\)-biased space makes parity tests close to unbiased while using short seeds.

## Round 21 Supplemental Terms

### Ramsey and Extremal Random Structures

#### Term: Ramsey Number
The Ramsey number \(R(s,t)\) is smallest \(n\) such that any red-blue coloring of \(K_n\) contains a red \(K_s\) or blue \(K_t\).

#### Term: Erdős–Szekeres Bound
For \(R(k,k)\),
\(
2^{k/2}\le R(k,k)\le 4^k,
\)
with classical probabilistic and counting arguments.

#### Term: Turán Density
For \(k\)-uniform hypergraph family \(\mathcal F\), Turán density
\(\pi(\mathcal F)\) is supremum edge density avoiding copies of \(\mathcal F\).

#### Term: Hypergraph Container Theorem
A container theorem gives a small collection of "containers" that includes all independent sets, enabling counting in sparse random structures.

#### Term: Threshold Function
For monotone property \(\mathcal P\) on \(G(n,p)\), threshold \(p(n)\) satisfies
\(\Pr[G(n,p)\in\mathcal P]\to 0\) below and \(\to1\) above this scaling.

## Round 22 Supplemental Terms

### Extremal Combinatorial Theorems

#### Term: Hall's Marriage Theorem
In bipartite \(G=(L,R,E)\), a matching saturating \(L\) exists iff every \(S\subseteq L\) satisfies
\[
|N(S)|\ge|S|.
\]

#### Term: Sperner Theorem
Among subsets of \([n]\), the largest antichain has size \(\binom{n}{\lfloor n/2\rfloor}\).

#### Term: Erdős-Ko-Rado Theorem
For \(n\ge 2r\), the largest intersecting family \(\mathcal F\subseteq \binom{[n]}{r}\) has
\[
|\mathcal F|\le \binom{n-1}{r-1}.
\]

#### Term: Hales-Jewett Theorem
For every \(k,m\), some \(N\) ensures every \(m\)-coloring of \([k]^N\) contains a monochromatic combinatorial line.

#### Term: Szemerédi Theorem
If \(\delta>0\), then for sufficiently large \(n\), every subset \(A\subseteq\{1,\dots,n\}\) with \(|A|\ge\delta n\) contains arithmetic progressions of any fixed length.

## Round 23 Supplemental Terms

### Advanced Extremal and Probabilistic Tools

#### Term: Lovász Local Lemma
For events \(A_i\), if \(\Pr(A_i)\le p\), each event depends on at most \(d\) others, and \(ep(d+1)\le1\), then \(\Pr(\bigcap\overline{A_i})>0\).

#### Term: Chernoff Bound
For \(X=\sum_{i=1}^n X_i\) independent Bernoulli, \(\Pr(|X-\mu|>\delta\mu)\le 2e^{-c\delta^2\mu}\) for \(0<\delta<1\).

#### Term: Janson's Inequality
For monotone decreasing events in dependency graph settings, \(\Pr(X\le (1-\delta)\mu)\) has exponential upper bounds involving pairwise correlation terms.

#### Term: Erdos-Rényi Random Graph
The graph \(G(n,p)\) has \(n\) labeled vertices with each edge included independently with probability \(p\).

#### Term: Threshold Phenomenon
For a graph property \(\mathcal P\), a threshold \(p_0(n)\) satisfies transition from \(\Pr[G(n,p)\in \mathcal P]\to0\) to \(\to1\) as \(p\) crosses \(p_0\).
