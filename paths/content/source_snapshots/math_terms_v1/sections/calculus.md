# Calculus

## Subcategory Map

1. Limits and Continuity
   - Limit Definitions
   - Continuity and Intermediate Value
2. Differentiation
   - Definition and Rules
   - Higher Derivatives
3. Applications of Derivatives
   - Extrema and Monotonicity
   - Concavity and Curvature
   - Linear Approximation
4. Integration
   - Antiderivatives
   - Definite Integral as Area/Accumulation
5. Integration Techniques
   - Algebraic Techniques
   - Trigonometric and Improper Integrals
6. Sequences and Series
   - Limits of Sequences
   - Convergence Tests
   - Power Series
7. Multivariable Calculus
   - Partial Derivatives
   - Multiple Integrals
8. Differential Equations
   - First-Order ODEs
   - Linear Equations

## Table of Contents

1. [Limits and Continuity](#limits-and-continuity)
   1. [Limit Definitions](#limit-definitions)
   2. [Continuity and Intermediate Value](#continuity-and-intermediate-value)
2. [Differentiation](#differentiation)
   1. [Definition and Rules](#definition-and-rules)
   2. [Higher Derivatives](#higher-derivatives)
3. [Applications of Derivatives](#applications-of-derivatives)
   1. [Extrema and Monotonicity](#extrema-and-monotonicity)
   2. [Concavity and Curvature](#concavity-and-curvature)
   3. [Linear Approximation](#linear-approximation)
4. [Integration](#integration)
   1. [Antiderivatives](#antiderivatives)
   2. [Definite Integral](#definite-integral)
5. [Integration Techniques](#integration-techniques)
   1. [Algebraic Techniques](#algebraic-techniques)
   2. [Trigonometric and Improper Integrals](#trigonometric-and-improper-integrals)
6. [Sequences and Series](#sequences-and-series)
   1. [Limits of Sequences](#limits-of-sequences)
   2. [Convergence Tests](#convergence-tests)
   3. [Power Series](#power-series)
7. [Multivariable Calculus](#multivariable-calculus)
   1. [Partial Derivatives](#partial-derivatives)
   2. [Multiple Integrals](#multiple-integrals)
8. [Differential Equations](#differential-equations)
   1. [First-Order ODEs](#first-order-odes)
   2. [Linear Equations](#linear-equations)
9. [Round 6 Supplemental Terms](#round-6-supplemental-terms)
   1. [Advanced Calculus Concepts](#advanced-calculus-concepts)
10. [Round 7 Supplemental Terms](#round-7-supplemental-terms)
   1. [Transform Methods and Asymptotics](#transform-methods-and-asymptotics)
11. [Round 8 Supplemental Terms](#round-8-supplemental-terms)
   1. [Laplace and Asymptotic Methods](#laplace-and-asymptotic-methods)
12. [Round 9 Supplemental Terms](#round-9-supplemental-terms)
   1. [Complex Integral Operators](#complex-integral-operators)
13. [Round 10 Supplemental Terms](#round-10-supplemental-terms)
   1. [Geometric and Asymptotic Analysis](#geometric-and-asymptotic-analysis)
14. [Round 11 Supplemental Terms](#round-11-supplemental-terms)
   1. [Special Integrals and Advanced Asymptotics](#special-integrals-and-advanced-asymptotics)
15. [Round 12 Supplemental Terms](#round-12-supplemental-terms)
   1. [Numerical Integration and Functional Expansions](#numerical-integration-and-functional-expansions)
16. [Round 13 Supplemental Terms](#round-13-supplemental-terms)
   1. [Distribution and Variational Techniques](#distribution-and-variational-techniques)
- [Round 14 Supplemental Terms](#round-14-supplemental-terms)
   1. [Measure and Distribution Techniques](#measure-and-distribution-techniques)
- [Round 15 Supplemental Terms](#round-15-supplemental-terms)
   1. [Asymptotic Integral Methods](#asymptotic-integral-methods)
- [Round 16 Supplemental Terms](#round-16-supplemental-terms)
   1. [Advanced Variational and Integral Geometry](#advanced-variational-and-integral-geometry)
- [Round 17 Supplemental Terms](#round-17-supplemental-terms)
   1. [Stokes and Divergence Framework](#stokes-and-divergence-framework)
- [Round 18 Supplemental Terms](#round-18-supplemental-terms)
   1. [Transform Kernels and PDE Links](#transform-kernels-and-pde-links)
- [Round 19 Supplemental Terms](#round-19-supplemental-terms)
   1. [Variational PDE and Distributional Calculus](#variational-pde-and-distributional-calculus)
- [Round 20 Supplemental Terms](#round-20-supplemental-terms)
   1. [Geometric Measure and Operator Theory](#geometric-measure-and-operator-theory)
- [Round 21 Supplemental Terms](#round-21-supplemental-terms)
   1. [Nonlinear Dynamics and PDE Invariants](#nonlinear-dynamics-and-pde-invariants)
- [Round 22 Supplemental Terms](#round-22-supplemental-terms)
   1. [Functional-Analytic Differential Structure](#functional-analytic-differential-structure)
- [Round 23 Supplemental Terms](#round-23-supplemental-terms)
   1. [Measure and Geometric Integration](#measure-and-geometric-integration)

## Limits and Continuity

### Limit Definitions

#### Term: Limit
\(\lim_{x\to a}f(x)=L\).

#### Term: Epsilon-Delta Definition
\(\lim_{x\to a}f(x)=L\) if for all \(\epsilon>0\), exists \(\delta>0\) with \(0<|x-a|<\delta \Rightarrow |f(x)-L|<\epsilon\).

#### Term: Left and Right Limits
\(\lim_{x\to a^-}f(x)\), \(\lim_{x\to a^+}f(x)\).

### Continuity and Intermediate Value

#### Term: Continuity at a Point
\(f\) continuous at \(a\) if \(\lim_{x\to a}f(x)=f(a)\).

#### Term: Intermediate Value Theorem
If \(f\) is continuous on \([a,b]\) and \(y\) lies between \(f(a),f(b)\), then \(\exists c\in(a,b):f(c)=y\).

#### Term: Types of Discontinuity
Removable, jump, and infinite discontinuities classify non-continuity points.

## Differentiation

### Definition and Rules

#### Term: Derivative
\(f'(x)=\lim_{h\to0}\frac{f(x+h)-f(x)}{h}\).

#### Term: Chain Rule
\(\frac{d}{dx}f(g(x))=f'(g(x))g'(x)\).

#### Term: Product Rule
\((uv)'=u'v+uv'\).

#### Term: Quotient Rule
\(\left(\frac{u}{v}\right)'=\frac{u'v-uv'}{v^2}\).

### Higher Derivatives

#### Term: Second Derivative
\(f''(x)=\frac{d}{dx}f'(x)\).

#### Term: Hessian (second derivative matrix)
\(H_{ij}=\frac{\partial^2 f}{\partial x_i\partial x_j}\).

## Applications of Derivatives

### Extrema and Monotonicity

#### Term: Critical Point
\(x=a\) with \(f'(a)=0\) or undefined.

#### Term: Monotonicity
If \(f'(x)>0\), \(f\) increases on interval.

#### Term: Fermat's Theorem
If \(f\) has local extremum at interior \(a\) and is differentiable there, then \(f'(a)=0\).

### Concavity and Curvature

#### Term: Concavity
\(f''(x)>0\): concave up; \(f''(x)<0\): concave down.

#### Term: Inflection Point
Point where concavity changes sign.

### Linear Approximation

#### Term: Differential Approximation
\(f(x+h)\approx f(x)+f'(x)h\).

#### Term: Mean Value Theorem
Exists \(c\in(a,b)\) with \(f'(c)=\frac{f(b)-f(a)}{b-a}\).

## Integration

### Antiderivatives

#### Term: Indefinite Integral
\(\int f(x)\,dx\).

#### Term: Constant of Integration
Any antiderivative differs by additive constant \(C\).

### Definite Integral

#### Term: Definite Integral
\(\int_a^b f(x)\,dx\).

#### Term: Fundamental Theorem of Calculus
\(\frac{d}{dx}\int_a^x f(t)dt = f(x)\).

## Integration Techniques

### Algebraic Techniques

#### Term: Substitution
Replace \(u=g(x)\) to simplify integrals.

#### Term: Integration by Parts
\(\int u\,dv = uv-\int v\,du\).

### Trigonometric and Improper Integrals

#### Term: Trig Substitution
Substitute \(\sin\), \(\cos\), or \(\tan\) for \(\sqrt{a^2-x^2}\)-style forms.

#### Term: Improper Integral
Integral where limits or integrand are unbounded.

## Sequences and Series

### Limits of Sequences

#### Term: Convergent Sequence
\(a_n\to L\).

#### Term: Cauchy Sequence
Sequence with \(\forall\epsilon>0\,\exists N:\forall m,n\ge N,\ |a_n-a_m|<\epsilon\).

### Convergence Tests

#### Term: Comparison Test
If \(0\le a_n\le b_n\) and \(\sum b_n\) converges, then \(\sum a_n\) converges.

#### Term: Ratio Test
If \(\lim_{n\to\infty}\left|\frac{a_{n+1}}{a_n}\right|=L\): converges for \(L<1\), diverges for \(L>1\).

### Power Series

#### Term: Power Series
\(\sum_{n=0}^\infty c_n(x-a)^n\).

#### Term: Radius of Convergence
Set of \(x\) where series converges, centered at \(a\), with radius \(R\).

## Multivariable Calculus

### Partial Derivatives

#### Term: Partial Derivative
\(\frac{\partial f}{\partial x}\).

#### Term: Mixed Partial Derivative
\(\frac{\partial^2 f}{\partial x\partial y}\).

### Multiple Integrals

#### Term: Double Integral
\(\iint_R f(x,y)\,dA\) accumulates over regions in the plane.

#### Term: Polar Double Integral
\(\iint_R f(r,\theta)\,r\,dr\,d\theta\).

## Differential Equations

### First-Order ODEs

#### Term: Separable ODE
\(\frac{dy}{dx}=g(x)h(y)\) can be separated.

#### Term: Exact Equation
\(M(x,y)+N(x,y)\) with \(M_y=N_x\) gives \(\partial F/\partial x=M,\partial F/\partial y=N\).

### Linear Equations

#### Term: Linear First-Order ODE
\(\frac{dy}{dx}+P(x)y=Q(x)\).

#### Term: Integrating Factor
\(\mu(x)=e^{\int P(x)\,dx}\) multiplies linear first-order ODE.

## Formal Theorems

### Theorem (Fundamental Theorem of Calculus, Part I)
If \(f\) is continuous on \([a,b]\), then
\[
\frac{d}{dx}\left(\int_a^x f(t)\,dt\right)=f(x),\quad x\in(a,b).
\]

### Theorem (Fundamental Theorem of Calculus, Part II)
If \(f\) is continuous and \(F\) is any antiderivative, then
\[
\int_a^b f(x)\,dx=F(b)-F(a).
\]

### Theorem (Mean Value Theorem)
If \(f\) is continuous on \([a,b]\) and differentiable on \((a,b)\), there exists \(c\in(a,b)\) such that
\[
f'(c)=\frac{f(b)-f(a)}{b-a}.
\]

### Theorem (Integration by Parts)
For differentiable \(u,v\):
\[
\int u\,dv = uv - \int v\,du.
\]

## Worked Examples

### Example (Differentiate a Polynomial)
If \(f(x)=x^4-2x^2+3x-1\), then
\[
f'(x)=4x^3-4x+3.
\]

### Example (Compute a Definite Integral)
\[
\int_0^1 (3x^2+2x)\,dx=\left[x^3+x^2\right]_0^1=2.
\]

### Example (Apply MVT)
For \(f(x)=x^2\) on \([1,3]\):
\[
\frac{f(3)-f(1)}{3-1} = \frac{9-1}{2}=4.
\]
There exists \(c\in(1,3)\) with \(f'(c)=2c=4\), so \(c=2\).

### Example (Solve ODE by Separation)
For \(\frac{dy}{dx}=3y\), separate: \(\frac{1}{y}dy=3dx\).
Integrating:
\[
\ln|y|=3x+C\Rightarrow y=Ce^{3x}.
\]

## Worked Examples (Q to A Mini-Proofs)

### Q1. Differentiate \(f(x)=x^4-2x^2+3x-1\).

**Answer.** Apply the power rule to each term:
\[
\frac{d}{dx}(x^4)=4x^3,
\quad
\frac{d}{dx}(-2x^2)=-4x,
\quad
\frac{d}{dx}(3x)=3,
\quad
\frac{d}{dx}(-1)=0.
\]
So
\[
f'(x)=4x^3-4x+3.
\]

### Q2. Compute \(\int_0^1 (3x^2+2x)\,dx\).

**Answer.** Find an antiderivative:
\[
\int(3x^2+2x)dx=x^3+x^2+C.
\]
Apply FTC:
\[
\int_0^1(3x^2+2x)dx=[x^3+x^2]_0^1=(1+1)-0=2.
\]

### Q3. Apply the Mean Value Theorem to \(f(x)=x^2\) on \([1,3]\).

**Answer.** \(f\) is continuous on \([1,3]\) and differentiable on \((1,3)\), so MVT applies. Compute:
\[
\frac{f(3)-f(1)}{3-1}=\frac{9-1}{2}=4.
\]
Set \(f'(c)=2c=4\), get \(c=2\). Since \(2\in(1,3)\), conditions are satisfied.

### Q4. Solve \(\frac{dy}{dx}=3y\) with variable separation.

**Answer.** Rearrange (for \(y\neq0\), then extend to all solutions):
\[
\frac1y\,dy=3\,dx.
\]
Integrate:
\[
\int \frac1y\,dy=\int 3\,dx
\Rightarrow \ln|y|=3x+C.
\]
Exponentiate:
\[
|y|=e^{3x+C}=Ce^{3x},
\]
absorbing sign into \(C\),
\[
y=Ce^{3x}.
\]
This is the full solution family.

## Additional Mini-Proof Examples (Round 3)

### Q5. Prove that if \(f'\equiv 0\) on interval \((a,b)\), then \(f\) is constant on \((a,b)\).

**Answer.** By Mean Value Theorem, for any \(x_1<x_2\) in \((a,b)\), there exists \(c\in(x_1,x_2)\) such that
\[
f(x_2)-f(x_1)=f'(c)(x_2-x_1).
\]
Given \(f'(c)=0\), we get \(f(x_2)-f(x_1)=0\), so \(f(x_2)=f(x_1)\). As \(x_1,x_2\) arbitrary, \(f\) is constant.

### Q6. Compute derivative of \(f(x)=\frac{1}{x}\) for \(x\neq 0\).

**Answer.** Write \(f(x)=x^{-1}\). By power rule,
\[
f'(x)=-1\cdot x^{-2}=-\frac{1}{x^2}.
\]

### Q7. Evaluate \(\int_1^2 2x\,dx\).

**Answer.** Antiderivative is \(x^2\), so
\[
\int_1^2 2x\,dx=[x^2]_1^2=4-1=3.
\]

### Q8. Verify FTC on \(g(x)=\int_0^x \sin t\,dt\).

**Answer.** By FTC, \(g'(x)=\sin x\). Compute directly from antiderivative:
\[
\int_0^x \sin t\,dt=-\cos x+\cos 0=1-\cos x,
\]
thus \(g'(x)=\sin x\). The direct derivative confirms the theorem.

## Formal Theorems (Round 4: Standard Statement Pack)

### Theorem (Extreme Value Theorem)
If \(f\) is continuous on a closed interval \([a,b]\), then \(f\) attains both a minimum and a maximum on \([a,b]\).

### Theorem (Rolle's Theorem)
If \(f\) is continuous on \([a,b]\), differentiable on \((a,b)\), and \(f(a)=f(b)\), then there exists \(c\in(a,b)\) such that \(f'(c)=0\).

### Theorem (Lagrange Mean Value Theorem)
If \(f\) is continuous on \([a,b]\) and differentiable on \((a,b)\), then \(\exists c\in(a,b)\) with
\[
f'(c)=\frac{f(b)-f(a)}{b-a}.
\]

### Theorem (Taylor's Theorem with Remainder, Lagrange Form)
If \(f\) is \((n+1)\)-times differentiable on interval containing \([a,x]\), then
\[
 f(x)=\sum_{k=0}^{n}\frac{f^{(k)}(a)}{k!}(x-a)^k + \frac{f^{(n+1)}(\xi)}{(n+1)!}(x-a)^{n+1}
\]
for some \(\xi\) between \(a\) and \(x\).

### Theorem (Cauchy Integral Test)
Let \((f(x))\) be positive, decreasing on \([1,\infty)\) and continuous. Then \(\sum_{n=1}^{\infty}f(n)\) converges iff \(\int_1^\infty f(x)\,dx\) converges.

## Round 4 Theorem Proof Sketches

### Q1. Extreme Value Theorem (Proof idea)
A continuous image of a compact set is compact. \([a,b]\) is compact in \(\mathbb R\), so \(f([a,b])\) is compact, hence closed and bounded. Closed+bounded subsets of \(\mathbb R\) contain min and max values, so \(f\) attains both extrema.

### Q2. Rolle's Theorem (Proof idea)
Since \(f\) is continuous on \([a,b]\), attains max/min. If both endpoints equal, either one endpoint and nearby interior gives max/min at interior point or one endpoint dominates.
If interior extremum exists, derivative there is 0. If not, then \(f\) constant and all derivatives are 0. Therefore there exists \(c\in(a,b)\) with \(f'(c)=0\).

### Q3. Lagrange Mean Value Theorem (Proof idea)
Define \(g(x)=f(x)-\frac{f(b)-f(a)}{b-a}(x-a)\). Then \(g(a)=g(b)\). Apply Rolle to \(g\): there exists \(c\in(a,b)\) with \(g'(c)=0\). Since \(g'(c)=f'(c)-\frac{f(b)-f(a)}{b-a}\), we get the MVT formula.

### Q4. Taylor's Theorem (Lagrange form) (Proof idea)
Apply the integral form recursively or apply mean-value theorem to the remainder function
\[
R_n(x)=f(x)-\sum_{k=0}^n\frac{f^{(k)}(a)}{k!}(x-a)^k.
\]
Then \(R_n^{(k)}(a)=0\) for \(k=0,\dots,n\). Repeatedly applying Rolle/MVT on suitable auxiliary functions gives
\[
R_n(x)=\frac{f^{(n+1)}(\xi)}{(n+1)!}(x-a)^{n+1}
\]
for some \(\xi\in(a,x)\).

### Q5. Cauchy Integral Test (Proof idea)
Partition integral into unit intervals:
\[
\int_1^{\infty}f(x)dx\asymp \sum_{n\ge1}\int_n^{n+1}f(x)dx.
\]
Since \(f\) positive decreasing,
\[
\int_{n+1}^{n+2}f(x)dx\le f(n+1)\le f(x)\le f(n)\le \int_n^{n+1}f(x)dx
\]
for \(x\in[n,n+1]\). So each term compares with adjacent series terms and tail sums. Therefore convergence of one implies convergence of the other.

## Round 5 Supplemental Terms

### Calculus Convergence Extensions

#### Term: Uniform Convergence

a sequence \\((f_n)\) of functions on \\((E\\) converges uniformly to \\(f\\) if for all \\(
\varepsilon>0\\), there exists \\N\\) such that \\(
|f_n(x)-f(x)|<\varepsilon
\) for all \\(
n\ge N\\), all \\x\in E\
\).

#### Term: Riemann Sum

a finite sum \\(
\sum_{i=1}^n f(c_i)\,\Delta x_i\\) approximating a definite integral.

#### Term: Arc Length

the length of curve \\(
\gamma:[a,b]\to\mathbb R^n\\) defined by \\(
L=\int_a^b \|\gamma'(t)\|\,dt.
\\)

#### Term: Differential Equation Initial Value
a differential equation together with \\(y(x_0)=y_0\
\) (and possibly higher-order initial data).

#### Term: Improper Integral at Infinity

a limit form \\(
\int_a^\infty f(x)\,dx=\lim_{b\to\infty}\int_a^b f(x)\,dx\\) when the limit exists.

## Round 6 Supplemental Terms

### Advanced Calculus Concepts

#### Term: Uniform Continuity

a function \(f\) on a set \(E\) is uniformly continuous if for every \(\varepsilon>0\) there exists \(\delta>0\) such that for all \(x,y\in E\),
\[
|x-y|<\delta \implies |f(x)-f(y)|<\varepsilon.
\]

#### Term: Improper Integral of Type II

a limit form where the integrand is unbounded on the interval or endpoint, e.g. \(\int_a^b f(x)\,dx\) with vertical asymptote in \((a,b)\).

#### Term: Absolute Continuity

a stronger property than absolute continuity of measure: for every \(\varepsilon>0\), there is \(\delta>0\) so that sums of lengths of disjoint intervals with total length <\(\delta\) force corresponding total variation of \(f\) to be <\(\varepsilon\).

#### Term: Line Integral

a contour sum
\[
\int_C \mathbf F\cdot d\mathbf r
\]
defining work of vector field \(\mathbf F\) along curve \(C\).

#### Term: Directional Derivative

the derivative of \(f\) at \(x\) in unit direction \(\mathbf u\):
\[
D_{\mathbf u}f(x)=\lim_{h\to0}\frac{f(x+h\mathbf u)-f(x)}{h}.
\]

## Round 7 Supplemental Terms

### Transform Methods and Asymptotics

#### Term: Fourier Transform

For an integrable function \(f\),
\[
\mathcal F\{f\}(\omega)=\int_{-\infty}^{\infty} f(x)e^{-i\omega x}\,dx.
\]

#### Term: Laplace Transform

For \(s\) with convergent integral,
\[
\mathcal L\{f\}(s)=\int_0^\infty e^{-st}f(t)\,dt.
\]

#### Term: Convolution

The convolution of \(f,g\) is
\[
(f*g)(x)=\int_{-\infty}^{\infty} f(t)g(x-t)\,dt.
\]

#### Term: Asymptotic Order

We write \(f(x)=O(g(x))\) as \(x\to a\) if \(|f(x)|\le C|g(x)|\) near \(a\) for some constant \(C>0\).

#### Term: Partial Differential Equation

An equation involving partial derivatives, such as
\[
F\!\left(x,u,\partial u,\partial^2u,\dots\right)=0,
\]
where \(\partial u\) denotes a collection of first derivatives.

## Round 8 Supplemental Terms

### Laplace and Asymptotic Methods

#### Term: Laplace Transform
The **Laplace transform** of 


$f(t)$


(for 


$t\ge0$) is 


\(\mathcal L\{f\}(s)=\int_0^{\infty}e^{-st}f(t)\,dt\), where it converges.

#### Term: Inverse Laplace Transform
The **inverse Laplace transform** is denoted 


\(\mathcal L^{-1}\{F\}(t)\)


and satisfies 


\(\mathcal L^{-1}\{\mathcal L\{f\}\}=f\).

#### Term: Fourier Transform
For integrable 


$f\),


\(\widehat f(\omega)=\int_{-\infty}^{\infty}f(t)e^{-i\omega t}\,dt\).

#### Term: Convolution
For 


$f,g\),


\((f*g)(t)=\int_0^t f(u)g(t-u)\,du\).

#### Term: Asymptotic Expansion
An 


**asymptotic expansion** 


\(f(x)\sim\sum_{n=0}^\infty a_n\phi_n(x)\)


as 


$x\to\infty\) means 


\(f(x)-\sum_{n=0}^{N-1}a_n\phi_n(x)=o(\phi_{N}(x))\) for each fixed 


$N$.

## Round 9 Supplemental Terms

### Complex Integral Operators

#### Term: Complex Integral
For a contour 


$\gamma$


in a domain 


$\mathbb C$, the contour integral is


\[
\int_{\gamma}f(z)\,dz.

\]

#### Term: Cauchy Integral Formula
If 


$f\) holomorphic and 


$\gamma$


 encloses 


$a$, then


\[


f(a)=\frac{1}{2\pi i}\int_{\gamma}\frac{f(z)}{z-a}\,dz.

\]

#### Term: Residue
If 


$f(z)=\sum_{n=-\infty}^{\infty}a_n(z-a)^n\) near isolated singularity 


$a$, then 


$\operatorname{Res}(f,a)=a_{-1}$.


The residue theorem gives


\[
\int_{\gamma}f(z)\,dz=2\pi i\sum\operatorname{Res}(f,a_k).

\]

#### Term: Contour Deformation
If 


$f\) is holomorphic on a homotopic region, contour integrals over homotopic paths are equal.

#### Term: Jordan's Lemma
For suitable decaying 


$e^{iaz}g(z)$


along semicircular arcs with 


$a>0$, the arc integral goes to zero as radius \(R\to\infty\).

## Round 10 Supplemental Terms

### Geometric and Asymptotic Analysis

#### Term: Arc Length of a Parametric Curve
For 


$\mathbf r(t)$


with smooth components on 


$[a,b]$, arc length is


\[
L=\int_a^b\|\mathbf r'(t)\|\,dt.

\]

#### Term: Surface of Revolution by Shells
The volume generated by revolving 


$y(x)$


about the y-axis is


\[V=2\pi\int_a^b x y(x)\,dx.\]

#### Term: Differentiation Under the Integral Sign
For smooth 


$F(x)=\int_{a(x)}^{b(x)}f(x,t)dt$,



\[
F'(x)=f(x,b(x))b'(x)-f(x,a(x))a'(x)+\int_{a(x)}^{b(x)}\frac{\partial f}{\partial x}(x,t)dt.

\]

#### Term: Asymptotic Equivalence
For positive functions 


$f,g$, write 


$f\sim g


\]


if 


\(\lim_{x\to\infty}f(x)/g(x)=1\).

#### Term: Dominant Balance
For 


$\sum_i a_i(x)$,


 dominant terms are those with largest growth order as 


$x\to\infty$, giving leading asymptotic behavior.


## Round 11 Supplemental Terms

### Special Integrals and Advanced Asymptotics

#### Term: Laplace's Method
For large 


$\lambda$, integrals of the form 


\(I(\lambda)=\int_a^b e^{\lambda\phi(x)}\psi(x)\,dx\)


approximate from maxima of 


$\phi$.

#### Term: Stationary Phase Approximation
For oscillatory integrals


\(\int e^{i\lambda\phi(x)}a(x)\,dx\)


with large 


$\lambda$, leading behavior is driven by points where 


$\phi'(x)=0$.

#### Term: Gamma Function
\[
\Gamma(z)=\int_0^\infty t^{z-1}e^{-t}\,dt,\quad \Re(z)>0,
\]


generalizes factorial by 


$\Gamma(n+1)=n!$.

#### Term: Beta Function
\[
B(x,y)=\int_0^1 t^{x-1}(1-t)^{y-1}\,dt
\]


and


\(B(x,y)=\frac{\Gamma(x)\Gamma(y)}{\Gamma(x+y)}\).

#### Term: Saddle-Point Method
For contour-integral forms 


\(\int e^{\lambda f(z)}dz\),


steepest-descent paths through saddle points give asymptotic expansions.


## Round 12 Supplemental Terms

### Numerical Integration and Functional Expansions

#### Term: Riemann Sum
A **Riemann sum** approximates integral by
\[
\sum_{i=1}^{n} f(\xi_i)\Delta x_i.
\]

#### Term: Trapezoidal Rule
For partition step \(h\) on \([a,b]\),
\[
\int_a^b f(x)\,dx\approx\frac h2\big(f(x_0)+2\sum_{i=1}^{n-1}f(x_i)+f(x_n)\big).
\]

#### Term: Simpson's Rule
With even \(n\),
\[
\int_a^b f(x)\,dx\approx\frac h3\left[f(x_0)+4\sum_{\text{odd }i}f(x_i)+2\sum_{\text{even }i\neq0,n}f(x_i)+f(x_n)\right].
\]

#### Term: Euler-Maclaurin Formula
For smooth \(f\),
\[
\sum_{k=m}^n f(k)=\int_m^n f(x)\,dx+\frac{f(m)+f(n)}2+\sum_{j=1}^p\frac{B_{2j}}{(2j)!}\big(f^{(2j-1)}(n)-f^{(2j-1)}(m)\big)+R_p.
\]

#### Term: Asymptotic Error Estimate
For method order \(p\), global error scales like \(C h^{p}\) as partition size \(h\to0\).
\[
\|E\|\le C h^{p}.
\]

## Round 13 Supplemental Terms

### Distribution and Variational Techniques

#### Term: Distribution
A **distribution** is a continuous linear functional on a space of test functions, extending classical functions.

#### Term: Dirac Delta
The distribution \(\delta\) satisfies
\[
\int_{-\infty}^{\infty}\delta(x-a)\phi(x)\,dx=\phi(a)
\]
for any test function \(\phi\).

#### Term: Weak Derivative
A function \(u\in L^1_{\mathrm{loc}}\) has **weak derivative** \(v\) if
\[
\int u\,\phi'=-\int v\,\phi
\]
for all smooth compactly supported test \(\phi\).

#### Term: Euler-Lagrange Equation
For functional
\[
J[y]=\int_a^b L(x,y,y')\,dx,
\]
critical points satisfy
\[
\frac{\partial L}{\partial y}-\frac{d}{dx}\left(\frac{\partial L}{\partial y'}\right)=0.
\]

#### Term: Functional Derivative
For functional \(\mathcal F[f]\), a **functional derivative** \(\frac{\delta\mathcal F}{\delta f}\) gives first-order variation of \(\mathcal F\).

## Round 14 Supplemental Terms

### Measure and Distribution Techniques

#### Term: Lebesgue Integral
For measurable \(f\), the Lebesgue integral is defined via approximation by simple functions and measure 
\(\int f\,d\mu\).

#### Term: Dominated Convergence Theorem
If \(f_n\to f\) pointwise and \(|f_n|\le g\) with \(\int g<\infty\), then
\(\lim_{n\to\infty}\int f_n=\int\lim_{n\to\infty}f_n.\)

#### Term: Distribution Derivative
The distributional derivative \(T'\) of a distribution \(T\) satisfies
\(\langle T',\varphi\rangle=-\langle T,\varphi'\rangle\) for all test functions \(\varphi\).

#### Term: Variational Derivative
For functional \(J[y]=\int_a^b L(x,y,y')dx\), a stationarity condition is
\(\frac{\partial L}{\partial y}-\frac{d}{dx}\frac{\partial L}{\partial y'}=0\).

#### Term: Fubini-Tonelli Theorem
If \(f\ge0\) or \(\int\!\int |f|<\infty\), then
\[
\int\!\left(\int f(x,y)\,dx\right)dy=\int\!\left(\int f(x,y)\,dy\right)dx.
\]

## Round 15 Supplemental Terms

### Asymptotic Integral Methods

#### Term: Laplace's Method
For \(I(\lambda)=\int_a^b e^{\lambda\phi(x)}\psi(x)dx\) with \(\phi\) peaked at interior maximizer \(x_0\),

\[
I(\lambda)\sim e^{\lambda\phi(x_0)}\psi(x_0)\sqrt{\frac{2\pi}{-\lambda\phi''(x_0)}}\quad(\lambda\to\infty).
\]

#### Term: Stationary Phase Approximation
For oscillatory integral \(\int e^{i\lambda S(x)}a(x)dx\), main contribution comes from points where \(S'(x)=0\).

#### Term: Steepest Descent
A contour deformation in complex plane chooses paths where\(\Im S(x)\) is constant and\(\Re S\) decreases fastest.

#### Term: Euler--Maclaurin Formula
For smooth \(f\),
\[
\sum_{k=a}^b f(k)=\int_a^b f(x)dx+\frac{f(a)+f(b)}2+\sum_{m=1}^{p}\frac{B_{2m}}{(2m)!}(f^{(2m-1)}(b)-f^{(2m-1)}(a))+R_p.
\]

#### Term: Mellin Transform
\[
\mathcal M\{f\}(s)=\int_0^{\infty}x^{s-1}f(x)\,dx,
\]
used to convert scaling into shifts in parameter \(s\).

## Round 16 Supplemental Terms

### Advanced Variational and Integral Geometry

#### Term: Euler--Lagrange Equation
For functional \(J[y]=\int_a^b L(x,y,y')\,dx\), stationary points satisfy
\[
\frac{\partial L}{\partial y}-\frac{d}{dx}\left(\frac{\partial L}{\partial y'}\right)=0.
\]

#### Term: First Variation
If \(\delta J(y)[\eta]=0\) for all admissible variations \(\eta\), then \(y\) is a critical function of \(J\).

#### Term: Lagrange Multiplier (Variational Form)
Extrema with constraint \(\int G(x,y,y')dx=0\) satisfy augmented Euler--Lagrange equation with \(\lambda\):
\[
\frac{\partial}{\partial y}(L+\lambda G)-\frac{d}{dx}\frac{\partial}{\partial y'}(L+\lambda G)=0.
\]

#### Term: Line Integral of Vector Field
For path \(C\),\(
\int_C \mathbf{F}\cdot d\mathbf r=\int_a^b \mathbf F(\mathbf r(t))\cdot \mathbf r'(t)\,dt.
\)

#### Term: Green's Theorem
For positively oriented \(C=\partial D\),
\[
\oint_C (P\,dx+Q\,dy)=\iint_D\left(\frac{\partial Q}{\partial x}-\frac{\partial P}{\partial y}\right)dA.
\]

## Round 17 Supplemental Terms

### Stokes and Divergence Framework

#### Term: Green's Theorem in the Plane
For planar region \(D\) with boundary \(\partial D\),
\[
\oint_{\partial D}(P\,dx+Q\,dy)=\iint_D\left(\frac{\partial Q}{\partial x}-\frac{\partial P}{\partial y}\right)dA.
\]

#### Term: Stokes' Theorem
For oriented surface \(S\) with boundary \(\partial S\):
\[
\oint_{\partial S}\omega=\int_S d\omega.
\]

#### Term: Divergence Theorem
For vector field \(\mathbf F\) on volume \(V\),
\[
\iiint_V \nabla\cdot \mathbf F\,dV=\iint_{\partial V}\mathbf F\cdot \mathbf n\,dS.
\]

#### Term: Change of Variables
For diffeomorphism \(\Phi\),
\[
\int_{\Phi(U)} f(y)\,dy=\int_U f(\Phi(x))\,|\det D\Phi(x)|\,dx.
\]

#### Term: Flux Through Surface
Flux across surface \(S\) is
\[
\iint_S \mathbf F\cdot \mathbf n\,dS,
\]
with orientation given by unit normal \(\mathbf n\).

## Round 18 Supplemental Terms

### Transform Kernels and PDE Links

#### Term: Laplace Transform
\[
\mathcal L\{f\}(s)=\int_0^\infty e^{-sx}f(x)\,dx.
\]

#### Term: Fourier Transform
\[
\mathcal F\{f\}(\xi)=\int_{-\infty}^{\infty} f(x)e^{-2\pi i\xi x}\,dx.
\]

#### Term: Convolution Theorem
For suitable \(f,g\),
\[
\mathcal F\{f\ast g\}=\mathcal F\{f\}\cdot \mathcal F\{g\}.
\]

#### Term: Heat Kernel
Solution to \(u_t=\Delta u\) on \(\mathbb R^n\) uses
\[
K_t(x)=\frac{1}{(4\pi t)^{n/2}}\exp\left(-\frac{|x|^2}{4t}\right),\quad t>0.
\]

#### Term: Green's Function
For operator \(L\), Green's function \(G\) satisfies \(L G(x,\cdot)=\delta_x\), so \(Lu=f\) is represented via kernel integral against \(G\).

## Round 19 Supplemental Terms

### Variational PDE and Distributional Calculus

#### Term: Weak Solution
A function \(u\in H_0^1(\Omega)\) is a **weak solution** of
\(-\Delta u=f\) on \(\Omega\) if
\[
\int_\Omega \nabla u\cdot\nabla\varphi\,dx=\int_\Omega f\varphi\,dx
\]
for all test functions \(\varphi\in H_0^1(\Omega)\).

#### Term: Euler--Lagrange Equation (Constrained)
With constraint \(\int_\Omega u\,dx=c\), extremals satisfy
\[
\frac{\partial L}{\partial u}-\frac{d}{dx}\left(\frac{\partial L}{\partial u'}\right)=\lambda,
\]
with multiplier \(\lambda\).

#### Term: Lax-Milgram Theorem
For bounded coercive bilinear form \(a(\cdot,\cdot)\) on Hilbert space \(H\), each continuous linear functional \(f\in H'\) has unique \(u\in H\) with
\[a(u,v)=f(v)\quad\forall v\in H.\]

#### Term: Distributional Derivative Revisited
A locally integrable \(u\) has distributional derivative \(u'\) defined by
\(\langle u',\phi\rangle=-\langle u,\phi'\rangle\).

#### Term: Sobolev Space Norm
The \(H^1(\Omega)\)-norm is
\[
\|u\|_{H^1}=\left(\int_\Omega |u|^2+|\nabla u|^2\,dx\right)^{1/2}.
\]

## Round 20 Supplemental Terms

### Geometric Measure and Operator Theory

#### Term: Coarea Formula
For Lipschitz \(f:\mathbb R^n\to\mathbb R\) and integrable \(g\),
\[
\int_{\mathbb R^n} g(x)|\nabla f(x)|dx=\int_{\mathbb R}\left(\int_{f^{-1}(t)} g\,d\sigma\right)dt.
\]

#### Term: Coarea in 2D (Polar)
For polar coordinates, area element
\(dx\,dy=r\,dr\,d\theta\), yielding perimeter/area decompositions via level sets.

#### Term: BV Function
A function \(u\in L^1\) has bounded variation if
\(\sup\{\int u\,\mathrm{div}\,\varphi:\ \varphi\in C_c^1,\|\varphi\|_{\infty}\le1\}<\infty\).

#### Term: Hadamard Variational Formula
For perturbed domain \(\Omega_t\), first variations of functionals are computed via boundary integrals involving normal velocity and shape derivative.

#### Term: Green’s Function (Boundary)
Boundary formulation writes
\(u(x)=\int_{\partial\Omega}\left(G\frac{\partial u}{\partial n}-u\frac{\partial G}{\partial n}\right)dS+\int_\Omega Gf\,dV\).

## Round 22 Supplemental Terms

### Functional-Analytic Differential Structure

#### Term: Gâteaux Derivative
For \(f:V\to W\), direction \(h\) and point \(x\), the **Gâteaux derivative** is
\[
D_G f(x)(h)=\lim_{t\to0}\frac{f(x+th)-f(x)}{t},
\]
when this directional limit exists.

#### Term: Fréchet Derivative
The map \(f:V\to W\) is **Fréchet differentiable** at \(x\) if there is linear \(L\) with
\[
\lim_{\|h\|\to0}\frac{\|f(x+h)-f(x)-Lh\|}{\|h\|}=0.
\]

#### Term: Inverse Function Theorem
If \(Df(x_0)\) is invertible, then \(f\) is locally a \(C^1\) bijection near \(x_0\), and \(Df^{-1}(f(x_0))=(Df(x_0))^{-1}\).

#### Term: Morse Lemma
If \(f:\mathbb R^n\to\mathbb R\) is \(C^2\) and \(\nabla f(0)=0\), \(Hf(0)\) nonsingular, then there exists chart in which
\[
f(x)=f(0)-\sum_{i=1}^k x_i^2+\sum_{i=k+1}^{n}x_i^2.
\]

#### Term: Coarea Formula
For Lipschitz \(u:\mathbb R^n\to\mathbb R\) and integrable \(g\),
\[
\int_{\mathbb R^n} g(x)\|\nabla u(x)\|\,dx=
\int_{\mathbb R}\left(\int_{u^{-1}(t)}g\,d\mathcal H^{n-1}\right)dt.
\]

## Round 23 Supplemental Terms

### Measure and Geometric Integration

#### Term: Dominated Convergence Theorem
If \(f_n\to f\) a.e. and \(|f_n|\le g\in L^1\), then
\[
\lim_{n\to\infty}\int f_n=\int f.
\]

#### Term: Fubini--Tonelli Theorem
For nonnegative \(f\) on product spaces,
\[
\int\left(\int f(x,y)\,dy\right)\,dx
=\int\left(\int f(x,y)\,dx\right)\,dy.
\]

#### Term: Tonelli’s Theorem
For \(\sigma\)-finite measure spaces, if \(f\ge0\), iterated integrals equal the double integral whenever one side is finite in \([0,\infty]\).

#### Term: Arzela--Ascoli Compactness
A uniformly bounded, equicontinuous family on compact \(K\) has a uniformly convergent subsequence.

#### Term: Sobolev Space
The Sobolev space \(W^{k,p}(\Omega)\) is the \(L^p\)-space of functions with weak derivatives up to order \(k\) in \(L^p\).

## Round 21 Supplemental Terms

### Nonlinear Dynamics and PDE Invariants

#### Term: Liouville Integrable System
A Hamiltonian system \(\dot q=\partial H/\partial p\),\(\dot p=-\partial H/\partial q\) is Liouville integrable if it has \(n\) independent first integrals in involution.

#### Term: Poincaré--Bendixson Trichotomy
In a planar ODE flow, every nonempty compact \(\omega\)-limit set is either fixed point, periodic orbit, or union of equilibria and orbits between them.

#### Term: Conservation Law
A PDE \(u_t+\nabla\cdot F(u)=0\) expresses conservation of integral quantity:
\[
\frac{d}{dt}\int_\Omega u\,dx= -\int_{\partial\Omega}F(u)\cdot n\,dS.
\]

#### Term: Weak-* Convergence
A sequence \(u_n\) converges weak-* to \(u\) in dual space if
\(\langle u_n,\varphi\rangle\to\langle u,\varphi\rangle\) for all test \(\varphi\).

#### Term: Compactness by Arzelà--Ascoli
For equicontinuous uniformly bounded family on compact set, there exists uniformly convergent subsequence.
